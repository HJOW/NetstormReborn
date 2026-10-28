// 원본 설정 객체(Config) 계층의 읽기 전용 구현이다. 키 조회·값 해석·`{키}` 치환·섹션 검색·객체 층(등록 순서)을 옮겼다.
// 원본 주소(10.78): 키 찾기 0043fd70, 원시 값 읽기 00440150, 파일 읽기 00440380, 객체 등록 0043fc20/0043fcd0/0043fab0,
//                  조회 00440760, 괄호 00440a00, 치환 00440b60, 섹션 0043fe60·00440570·00440640, 줄 덧붙임 0043ffa0.
// 규칙의 근거는 docs/formats/config.md 이고, 줄 이동·값 해석·섹션 경계의 세부는 원본 x86 기대값(cpppj/tests/fixtures/config-x86.tsv, 두 판본 동일 결과)과
// 대조해 검증한다(webpj/tests/config.test.mjs). 값 쓰기(Set)·줄 삭제(RemoveKey)·레지스트리 조회·파일 저장은 웹 판본에 필요해질 때 옮긴다.
// 문자열은 바이트 하나가 문자 하나인 JavaScript 문자열로 다룬다(원본 텍스트는 Windows-1252, 디코딩은 assets/text-encoding.js).
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-10-28 WB02-D: 메뉴 대화상자 본문과 용어표를 읽기 위해 추가)
import { xorDecode } from './taff.js';

// 설정 텍스트의 특수 글자이다(원본 전역 0x532538). 이스케이프 '`', 치환 괄호 '{' '}', 기본값 구분 '|', 섹션 '[' ']', 객체 접두어 구분 '.'. (2026-10-28 WB02-D)
const ESCAPE = '`';
const OPEN_BRACE = '{';
const CLOSE_BRACE = '}';
const DEFAULT_SEPARATOR = '|';
const OPEN_SECTION = '[';
const CLOSE_SECTION = ']';
const OBJECT_SEPARATOR = '.';

// 암호화된 설정 파일을 풀면 맨 앞에 붙는 서명이다. 풀기 전 첫·세 번째 바이트가 0 이면 인코딩된 파일로 본다. (2026-10-28 WB02-D)
export const CONFIG_SIGNATURE = 'mQdsT';

// 치환 괄호 안에서 키·기본값으로 모으는 최대 글자 수이다(원본 패치판 지역 버퍼 0x2000바이트). (2026-10-28 WB02-D)
const LONGEST_SYMBOL = 0x2000;

// 치환이 중첩될 수 있는 최대 깊이이다. 자기 자신을 참조하는 값은 원본에서 스택이 넘치므로 이 깊이부터는 찾지 못한 것으로 처리한다. (2026-10-28 WB02-D)
const MAX_SUBSTITUTION_DEPTH = 32;

// 이 글자가 줄 사이를 건너뛰는 글자(LF·CR·공백·탭)인지 확인한다. (2026-10-28 WB02-D)
function isLineGap(character) {
    return character === '\n' || character === '\r' || character === ' ' || character === '\t';
}

// 이 글자가 줄 앞·키 뒤에서 건너뛰는 글자(공백·탭)인지 확인한다. (2026-10-28 WB02-D)
function isBlank(character) {
    return character === ' ' || character === '\t';
}

// ASCII 대소문자만 바꾸는 소문자 변환이다(CRT 의 "C" 로케일 동작). 비ASCII 는 그대로 둔다. (2026-10-28 WB02-D)
export function asciiLower(text) {
    return text.replace(/[A-Z]/g, (character) => String.fromCharCode(character.charCodeAt(0) + 32));
}

// ASCII 대소문자만 바꾸는 대문자 변환이다. 치환 키는 대문자로 모아 비교한다. (2026-10-28 WB02-D)
function asciiUpper(text) {
    return text.replace(/[a-z]/g, (character) => String.fromCharCode(character.charCodeAt(0) - 32));
}

// text 의 pos 위치가 prefix 로 시작하는지 대소문자 무시로 비교한다(CRT strnicmp 접두어 비교). 남은 글자가 짧으면 다르다. (2026-10-28 WB02-D)
function startsWithFolded(text, pos, prefix) {
    return pos + prefix.length <= text.length && asciiLower(text.substr(pos, prefix.length)) === asciiLower(prefix);
}

// C 문자열처럼 첫 NUL 앞까지만 돌려준다. (2026-10-28 WB02-D)
function untilNul(text) {
    const nul = text.indexOf('\0');
    return nul < 0 ? text : text.slice(0, nul);
}

// pos 가 가리키는 줄의 다음 줄 첫 글자 위치를 구한다. LF 까지 간 뒤 LF·CR·공백·탭을 모두 건너뛴다(빈 줄과 들여쓰기 포함). LF 가 없으면 끝이다. (2026-10-28 WB02-D)
function nextLine(text, pos) {
    let cursor = text.indexOf('\n', pos);
    if (cursor < 0) return text.length;
    // 빈 줄과 다음 줄의 들여쓰기까지 함께 건너뛴다.
    while (cursor < text.length && isLineGap(text[cursor])) cursor += 1;
    return cursor;
}

// 원본 0043fd70: 버퍼 앞에서부터 줄 단위로 훑어 키의 첫 일치 위치를 돌려준다. 없으면 -1 이다.
// 줄 앞 공백·탭을 건너뛰고, 키 길이만큼 대소문자 무시로 비교한 뒤 공백·탭을 지나 '=' 가 나오면 일치다. CR 단독은 줄바꿈이 아니다. 빈 키는 어느 줄과도 맞지 않는다.
// 사용: findKey(버퍼, 'currentLanguage'). (2026-10-28 WB02-D)
export function findKey(textInput, key) {
    const text = untilNul(textInput);
    if (key.length === 0) return -1;
    let pos = 0;
    // 줄마다 키가 시작하는지 확인한다.
    while (pos < text.length) {
        while (pos < text.length && isBlank(text[pos])) pos += 1;
        const start = pos;
        if (startsWithFolded(text, pos, key)) {
            pos += key.length;
            while (pos < text.length && isBlank(text[pos])) pos += 1;
            if (pos < text.length && text[pos] === '=') return start;
        }
        pos = nextLine(text, start);
    }
    return -1;
}

// 원본 00440150: 첫 일치 키의 원시 값을 읽는다. 없으면 null 이다.
// '=' 와 이어진 공백·탭을 건너뛴 뒤 줄 끝(CR/LF)이나 따옴표 밖의 "//" 전까지 복사한다. 큰따옴표는 따옴표 모드를 켜고 끄며 값에서 빠지고,
// 바로 앞 글자가 '`' 인 큰따옴표는 모드를 바꾸지 않고 두 글자가 그대로 남는다. (2026-10-28 WB02-D)
export function getRaw(textInput, key) {
    const text = untilNul(textInput);
    const found = findKey(text, key);
    if (found < 0) return null;
    let pos = text.indexOf('=', found);
    // 원본은 연속된 '=' 와 공백·탭을 값 앞에서 건너뛴다.
    while (pos < text.length && (text[pos] === '=' || isBlank(text[pos]))) pos += 1;
    let result = '';
    let quoted = false;
    let previous = '\0';
    // 줄 끝이나 따옴표 밖 주석 앞까지 한 글자씩 옮긴다.
    for (; pos < text.length; pos += 1) {
        const character = text[pos];
        if (character === '\r' || character === '\n' || (!quoted && character === '/' && text[pos + 1] === '/')) break;
        if (character === '"' && previous !== ESCAPE) quoted = !quoted;
        else result += character;
        previous = character;
    }
    return result;
}

// 원본 0043fe60: from 위치부터 헤더(예: "[Credits]")로 시작하는 섹션 줄을 찾는다. 찾으면 헤더 줄 다음 줄의 시작 위치(본문 시작)를 돌려주고,
// 없으면 -1 이다. 줄의 첫 글자(앞 공백·탭 제외)가 '[' 인 줄에서만 그 줄 안의 모든 '[' 위치를 헤더와 대소문자 무시로 비교한다.
// 헤더가 접두어이므로 "[Credits]" 는 "[Credits] 뒤에 글자가 더 있는 줄" 과도 맞는다. 이름을 받는 호출(matchedName 객체)에는 '[' 뒤 ']' 앞 글자를 value 에 넣는다.
// 헤더 줄에 LF 가 없거나(원본은 assert) 본문이 비어 버퍼가 거기서 끝나면 -1 이다. 헤더 줄의 LF 바로 뒤 CR 하나는 건너뛴다.
// 사용: findSection(버퍼, 0, '[Credits]'). (2026-10-28 WB02-D)
export function findSection(textInput, from, header, matchedName = null) {
    const text = untilNul(textInput);
    let pos = from;
    // 줄 단위로 훑는다.
    while (pos < text.length) {
        while (pos < text.length && isBlank(text[pos])) pos += 1;
        const lineStart = pos;
        let matched = false;
        if (pos < text.length && text[pos] === OPEN_SECTION) {
            // 이 줄 안의 '[' 마다 비교한다.
            for (;;) {
                if (startsWithFolded(text, pos, header)) {
                    matched = true;
                    break;
                }
                pos += 1;
                // 같은 줄의 다음 '[' 까지 간다.
                while (pos < text.length && text[pos] !== '\n' && text[pos] !== OPEN_SECTION) pos += 1;
                if (pos >= text.length || text[pos] === '\n') break;
            }
        }
        if (!matched) {
            pos = nextLine(text, lineStart);
            continue;
        }
        if (matchedName) {
            // '[' 다음부터 ']' 앞까지가 이름이다. ']' 가 없으면 버퍼 끝까지다(원본은 끝없이 읽는다).
            let close = text.indexOf(CLOSE_SECTION, pos + 1);
            if (close < 0) close = text.length;
            matchedName.value = text.slice(pos + 1, close);
            pos = close;
        }
        const newline = text.indexOf('\n', pos);
        if (newline < 0) return -1;
        pos = newline + 1;
        // LF 바로 뒤의 CR 하나도 건너뛴다.
        if (pos < text.length && text[pos] === '\r') pos += 1;
        // 본문이 비어 버퍼 끝이면 원본 호출자들은 찾지 못한 것으로 다룬다.
        return pos >= text.length ? -1 : pos;
    }
    return -1;
}

// 이름의 섹션 본문 위치와 길이를 구한다. 이름이 '[' 로 시작하면 그대로 헤더이고, 아니면 "[이름]" 이 헤더다.
// 본문은 헤더 줄 다음부터, 줄 첫 글자(앞 글자가 LF·CR)가 '[' 인 위치 직전까지다. 없으면 null. (2026-10-28 WB02-D)
export function sectionRange(textInput, name) {
    const text = untilNul(textInput);
    const header = name.length > 0 && name[0] === OPEN_SECTION ? name : `${OPEN_SECTION}${name}${CLOSE_SECTION}`;
    const start = findSection(text, 0, header);
    if (start < 0) return null;
    let end = start;
    // 다음 섹션 줄(줄 맨 앞의 '[')이나 버퍼 끝까지 간다.
    while (end < text.length && !(text[end] === OPEN_SECTION && (text[end - 1] === '\n' || text[end - 1] === '\r'))) end += 1;
    return { start, length: end - start };
}

// 설정 객체 등록부이다. 설정 객체는 등록 순서대로 쌓이고, 조회는 마지막에 등록한 것부터 한다(원본 전역 배열).
// hooks 는 선택 사항이다: environment(이름) → 문자열|null(환경 변수, 허용한 객체만), registryValue(경로) → 문자열|null(레지스트리, 웹 판본은 쓰지 않음),
// loadMission(임시 설정, 미션 이름) → void(`{@미션.키}` 조회가 임시 설정에 미션 파일을 읽어 넣는 자리).
// 사용: const registry = new ConfigRegistry(); new Config(registry, 이름?). (2026-10-28 WB02-D)
export class ConfigRegistry {
    // 훅 묶음을 받아 빈 등록부를 만든다.
    constructor(hooks = {}) {
        // 등록된 설정 객체 목록(등록 순서)이다.
        this._objects = [];
        // 환경 변수 조회 훅이다.
        this.environment = hooks.environment ?? null;
        // 레지스트리 값 조회 훅이다.
        this.registryValue = hooks.registryValue ?? null;
        // 미션 설정 적재 훅이다.
        this.loadMission = hooks.loadMission ?? null;
    }

    // 등록된 객체 목록(읽기 전용 복사본)이다.
    get objects() {
        return this._objects.slice();
    }

    // 객체를 등록 목록 끝에 더한다. 원본 패치판은 한도 1000개를 넘으면 assert 하며, 여기서는 오류를 던진다.
    register(config) {
        if (this._objects.length >= 1000) throw new RangeError('ConfigRegistry: 등록 한도(1000)를 넘었습니다.');
        this._objects.push(config);
    }

    // 객체를 목록에서 뺀다. 나머지의 순서는 유지한다.
    unregister(config) {
        this._objects = this._objects.filter((object) => object !== config);
    }

    // 원본 0043fb90: 이름에 '.' 을 붙여 객체 접두어와 대소문자 무시로 비교해 먼저 등록한 객체부터 찾는다. 없으면 null 이다.
    findByName(name) {
        const wanted = asciiLower(`${name}${OBJECT_SEPARATOR}`);
        return this._objects.find((object) => asciiLower(object.prefix) === wanted) ?? null;
    }
}

// 설정 객체 한 개이다. 텍스트 버퍼(줄 모음)와 선택적 이름(접두어)을 가진다.
// 이름 있는 객체(예: 'local')는 "이름." 을 접두어로 삼아 접두어가 맞는 키만 받고, 이름 없는 객체는 모든 키를 받는다.
// 사용: const config = new Config(registry, 'local'); config.append('1="tutorial1"'); config.substitute('{local.1}').
// 생성하면 등록부에 올라가므로 임시 객체는 dispose() 로 내려야 한다. (2026-10-28 WB02-D)
export class Config {
    // 등록부와 이름(생략하면 이름 없음)으로 객체를 만들고 등록부에 올린다.
    constructor(registry, name = null) {
        // 이 객체가 속한 등록부다.
        this.registry = registry;
        // 객체 접두어("이름.")이다. 이름이 없으면 빈 문자열이다.
        this.prefix = name === null ? '' : `${untilNul(name)}${OBJECT_SEPARATOR}`;
        // 텍스트 버퍼이다. 버퍼가 없는 객체(null)는 조회에 걸리지 않는다.
        this.text = null;
        // 환경 변수(`{_이름}`) 조회를 허용하는지 여부다.
        this.environmentEnabled = false;
        registry.register(this);
    }

    // 등록부에서 내려온다. 이후 이 객체로 조회하지 않는다.
    dispose() {
        this.registry.unregister(this);
    }

    // 버퍼를 통째로 바꾼다. 첫 NUL 에서 끝나는 C 문자열처럼 자른다.
    assign(text) {
        this.text = untilNul(text);
    }

    // 버퍼를 비운다(없는 상태로 되돌린다).
    clear() {
        this.text = null;
    }

    // 원본 0043ffa0: 줄 하나를 덧붙인다. 입력 끝의 CR/LF 를 모두 지운 뒤 CRLF 하나를 붙이며, 버퍼가 없으면 새로 만든다.
    append(text) {
        let value = untilNul(text);
        while (value.length > 0 && (value[value.length - 1] === '\r' || value[value.length - 1] === '\n')) value = value.slice(0, -1);
        this.text = `${this.text ?? ''}${value}\r\n`;
    }

    // 원본 00440380: 설정 파일 바이트를 읽어 버퍼 끝에 덧붙인다. 첫·세 번째 바이트가 0 이면 XOR 인코딩된 파일로 보고 풀며(키 "mydoghasfleas"),
    // 풀린 내용의 서명 "mQdsT" 는 떼지 않고 그대로 붙인다(원본처럼 인코딩된 파일의 첫 줄은 키로 조회되지 않는 죽은 줄이 된다).
    // 첫 NUL 에서 끝나는 C 문자열로 다루며, decode(bytes) 는 본문 글자 해독(원본 Windows-1252 또는 UTF-8)을 맡는 호출자 함수다.
    // 서명 뒤(또는 파일 맨 앞)의 UTF-8 BOM 은 새 UTF-8 파일을 가리키는 표시로 decode 가 처리한다. 사용: config.loadBytes(bytes, decodeOriginalText).
    loadBytes(bytes, decode) {
        let plain = bytes;
        // 첫·세 번째 바이트가 0 이면 XOR 인코딩된 파일이다.
        if (plain.length >= 3 && plain[0] === 0 && plain[2] === 0) plain = xorDecode(plain);
        const nul = plain.indexOf(0);
        if (nul >= 0) plain = plain.subarray(0, nul);
        const hasSignature = plain.length >= CONFIG_SIGNATURE.length && CONFIG_SIGNATURE.split('').every((character, index) => plain[index] === character.charCodeAt(0));
        const skip = hasSignature ? CONFIG_SIGNATURE.length : 0;
        this.append(`${hasSignature ? CONFIG_SIGNATURE : ''}${decode(plain.subarray(skip))}`);
    }

    // 버퍼에서 키의 원시 값을 읽는다(이 객체 하나만 본다). 버퍼가 없거나 키가 없으면 null 이다.
    getRaw(key) {
        return this.text === null ? null : getRaw(this.text, key);
    }

    // 섹션 본문(헤더 다음 줄부터 다음 섹션 직전까지) 문자열을 돌려준다. 없으면 null 이다.
    section(name) {
        if (this.text === null) return null;
        const range = sectionRange(this.text, name);
        return range ? this.text.slice(range.start, range.start + range.length) : null;
    }

    // 원본 004409d0: 모든 등록 객체를 뒤에서부터 조회해 값을 치환까지 마친 문자열로 돌려준다. 이 객체에 버퍼가 없으면 null,
    // 찾지 못하면(표시 없이) null 이다.
    get(key) {
        if (this.text === null) return null;
        const output = { text: '' };
        return this._lookup(untilNul(key), '', output, false, 0) ? output.text : null;
    }

    // 원본 00440b60: 문자열의 이스케이프와 `{키}` 치환을 풀어 새 문자열로 돌려준다. 이 객체에 버퍼가 없거나 입력이 비면 빈 문자열이다.
    substitute(source) {
        const output = { text: '' };
        this._substituteInto(output, untilNul(source), 0);
        return output.text;
    }

    // 치환 본체이다. output.text 끝에 결과를 더한다. 끝의 공백·탭은 잘라 내되 이 호출이 쓴 첫 글자는 남긴다.
    _substituteInto(output, source, depth) {
        const start = output.text.length;
        if (this.text === null || source.length === 0) return;
        const cursor = { pos: 0 };
        // 입력을 한 글자씩 옮기며 이스케이프와 괄호를 푼다.
        while (cursor.pos < source.length) {
            const character = source[cursor.pos];
            if (character === ESCAPE) {
                cursor.pos += 1;
                // 끝의 외톨이 이스케이프는 버린다.
                if (cursor.pos < source.length) {
                    const escaped = source[cursor.pos];
                    cursor.pos += 1;
                    output.text += escaped === 'n' ? '\n' : escaped === 'r' ? '\r' : escaped === 't' ? '\t' : escaped;
                }
            } else if (character === OPEN_BRACE) {
                this._expandBrace(output, source, cursor, depth);
            } else {
                output.text += character;
                cursor.pos += 1;
            }
        }
        let end = output.text.length;
        // 첫 글자만 남을 때까지 끝의 공백·탭을 지운다.
        while (end > start + 1 && isBlank(output.text[end - 1])) end -= 1;
        output.text = output.text.slice(0, end);
    }

    // 원본 00440a00: `{키|기본값}` 한 덩어리를 푼다. 키는 대문자로 모으고(안쪽 괄호의 결과는 그대로), 기본값은 글자 그대로 모은다.
    // cursor.pos 는 여는 괄호 위치에서 시작해 닫는 괄호 다음으로 옮겨진다.
    _expandBrace(output, source, cursor, depth) {
        cursor.pos += 1;
        const key = { text: '' };
        // 키: 끝·'}'·'|' 전까지(안쪽 '{' 는 먼저 푼다). 길이 한도는 원본 지역 버퍼 크기다.
        while (cursor.pos < source.length && source[cursor.pos] !== CLOSE_BRACE && source[cursor.pos] !== DEFAULT_SEPARATOR && key.text.length < LONGEST_SYMBOL) {
            if (source[cursor.pos] === OPEN_BRACE) {
                this._expandBrace(key, source, cursor, depth);
            } else {
                key.text += asciiUpper(source[cursor.pos]);
                cursor.pos += 1;
            }
        }
        const fallback = { text: '' };
        if (cursor.pos < source.length && source[cursor.pos] === DEFAULT_SEPARATOR) {
            cursor.pos += 1;
            // 기본값: 끝·'}' 전까지다. '|' 는 여기서는 보통 글자다.
            while (cursor.pos < source.length && source[cursor.pos] !== CLOSE_BRACE && fallback.text.length < LONGEST_SYMBOL) {
                if (source[cursor.pos] === OPEN_BRACE) {
                    this._expandBrace(fallback, source, cursor, depth);
                } else {
                    fallback.text += source[cursor.pos];
                    cursor.pos += 1;
                }
            }
        }
        this._lookup(key.text, fallback.text, output, true, depth + 1);
        // 닫는 '}'(또는 한도에서 멈춘 글자)를 건너뛴다.
        if (cursor.pos < source.length) cursor.pos += 1;
    }

    // 원본 00440760: 키를 조회해 치환한 값을 output.text 에 더한다. 반환값은 "찾았는가"이며 기본값으로 채운 경우는 false 다.
    // 키가 '&' 로 시작하면 레지스트리 값(훅), '@' 로 시작하면 `{@미션.키}`, '_' 로 시작하고 이 객체가 허용하면 환경 변수, 그 밖에는 등록 객체를 뒤에서부터 찾는다.
    // reportMissing 이 true 이고 기본값도 없으면 `{키}` 를 남긴다(키는 괄호 해석에서 이미 대문자).
    _lookup(keyInput, fallback, output, reportMissingInput, depth) {
        let key = keyInput;
        let reportMissing = reportMissingInput;
        let found = false;
        let raw = '';
        if (depth > MAX_SUBSTITUTION_DEPTH) {
            // 새 보호 장치: 여기서 멈추고 아래의 "찾지 못함" 처리로 간다.
        } else if (key.length > 0 && key[0] === '&') {
            // 레지스트리 값: '{' '}' 앞에 이스케이프를 붙여 그대로 쓰며 다시 치환하지 않는다(원본 00440090).
            reportMissing = false;
            const value = this.registry.registryValue ? this.registry.registryValue(key.slice(1)) : null;
            if (value !== null && value !== undefined) {
                // 값의 괄호가 다시 치환되지 않게 이스케이프를 앞에 붙인다.
                for (const character of value) {
                    if (character === OPEN_BRACE || character === CLOSE_BRACE) output.text += ESCAPE;
                    output.text += character;
                }
                return true;
            }
        } else if (key.length > 0 && key[0] === '@') {
            const dot = key.indexOf(OBJECT_SEPARATOR, 1);
            // 원본은 점이 없는 `{@이름}` 을 빈 값을 찾은 것으로 처리한다.
            if (dot < 0) return true;
            const mission = key.slice(1, dot);
            const missionKey = key.slice(dot + 1);
            // 원본은 '.' 자리에 NUL 을 써서, 못 찾았을 때 표시되는 키가 여기서 잘린다.
            key = key.slice(0, dot);
            // 임시 객체는 미션 이름을 접두어로 전역 목록에 잠시 올라간다.
            const temporary = new Config(this.registry, mission);
            try {
                this.registry.loadMission?.(temporary, mission);
                const value = temporary.getRaw(missionKey);
                if (value !== null) {
                    raw = value;
                    found = true;
                }
            } finally {
                temporary.dispose();
            }
        } else {
            // 환경 변수: '_' 로 시작하고 이 객체가 허용할 때만이다.
            if (key.length > 0 && key[0] === '_' && this.environmentEnabled && this.registry.environment) {
                const value = this.registry.environment(key.slice(1));
                if (value !== null && value !== undefined) {
                    raw = value;
                    found = true;
                }
            }
            const objects = this.registry.objects;
            // 마지막에 등록한 객체부터 찾는다. 이름 있는 객체는 접두어가 맞는 키만 받고 접두어를 뗀 나머지로 찾는다.
            for (let index = objects.length - 1; !found && index >= 0; index -= 1) {
                const object = objects[index];
                let value = null;
                if (object.prefix.length === 0) value = object.getRaw(key);
                else if (startsWithFolded(key, 0, object.prefix)) value = object.getRaw(key.slice(object.prefix.length));
                if (value !== null) {
                    raw = value;
                    found = true;
                }
            }
        }
        if (found) {
            this._substituteInto(output, raw, depth);
            return true;
        }
        if (fallback.length > 0) this._substituteInto(output, fallback, depth);
        else if (reportMissing) output.text += `${OPEN_BRACE}${key}${CLOSE_BRACE}`;
        return false;
    }
}

