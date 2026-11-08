// 원문 가상 +0x68 객체 이름(004ace20 base 이름·00454d30 Factory 이름)을 옮긴 모듈이다(cpppj `o/RawSquidName.cpp/.h` 의 RawSquidName·SquidNameEntry·SquidLevelName, 10.78 전용).
// 순서: TYPE 번호를 언어 조회 전에 선택하고(언어 0 은 한 번, 그 밖은 두 번 조회해 두 번째가 1 이 아닐 때만 설명 번역), 설명은 언어 조회 효과 뒤의 현재 값을 읽는다.
//   설명(첫 NUL 까지)→번역→80바이트 경계 확인→설정 치환(00441740)→다시 80바이트 경계 확인. Factory 는 base 이름 뒤 현재 번역 패턴(" Level %s", 언어 관문 없음)→현재 가상 레벨→로마숫자→CRT 서식→접미사(256바이트 경계)→80바이트 경계 순서다.
// 가상 표는 보호 PE 의 83개만 분배하고 미확인 표에 이름 몸체를 임의로 붙이지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// 문자열 길이는 원본의 Windows-1252 바이트 길이를 JS 문자 수로 센다(원본 영어 이름과 같음). 한국어 표시 부호화의 바이트 길이는 이름 표시 단위에서 다시 정한다.
// (2026-11-08 WB04-E58: 객체 이름 이식으로 추가)

import { SQUID_NAME_ENTRIES } from './squid-name-vtables.js';

// 이름 지역 버퍼(0049af80)와 Factory 접미사 버퍼(00454d30)의 바이트 크기, base 이름 진입점, 188칸 TYPE 표 길이이다. (2026-11-08 WB04-E58)
const NAME_BYTES = 80;
const SUFFIX_BYTES = 256;
const BASE_ENTRY = 0x4ace20;
const TYPE_COUNT = 188;
const LEVEL_PATTERN = ' Level %s';
const TYPE_OFFSET = 10;

// PE 00542420 의 로마숫자 10개 표이다(레벨 0..9). 범위 밖은 0xff 한 글자이다. (2026-11-08 WB04-E58)
const LEVEL_NAMES = Object.freeze(['I', 'II', 'III', 'IV', 'V', 'VI', 'VII', 'VIII', 'IX', 'X']);

// 가상 표 → 이름 진입점의 조회 표이다. (2026-11-08 WB04-E58)
const ENTRY_BY_TABLE = new Map(SQUID_NAME_ENTRIES.map(([table, entry]) => [table, entry]));

// 선택된 이름 하위가 없으면 빈 문자열로 대신하지 않고 거부한다. (2026-11-08 WB04-E58)
function need(device) {
    if (typeof device !== 'function') throw new Error('객체 이름 실제 하위 미연결');
    return device;
}

// 원본 C 문자열의 첫 NUL 까지만 취하고 버퍼 크기 이상이면 거부한다. (2026-11-08 WB04-E58)
function textOf(value, capacity) {
    const end = value.indexOf('\0');
    const cut = end === -1 ? value : value.slice(0, end);
    if (cut.length >= capacity) throw new RangeError('객체 이름 원본 버퍼 범위');
    return cut;
}

// 가상 표 주소에서 이름 진입점을 찾는다. 확인되지 않은 표는 base 이름으로 대신하지 않는다. (2026-11-08 WB04-E58)
export function squidNameEntry(table) {
    const entry = ENTRY_BY_TABLE.get(table >>> 0);
    if (entry === undefined) throw new Error('객체 이름 가상 표 미복원');
    return entry;
}

// 004a8980 의 로마숫자(레벨 0 은 I)이며 범위 밖은 마지막 0xff sentinel 한 글자이다. (2026-11-08 WB04-E58)
export function squidLevelName(level) {
    const number = level >>> 0;
    return number < LEVEL_NAMES.length ? LEVEL_NAMES[number] : String.fromCharCode(0xff);
}

// CRT snprintf 의 이 모듈이 쓰는 부분집합이다: `%%` 와 `%[-][폭][.정밀도]s` 한 종류의 문자열 인자만 소비하고 다른 변환은 거부한다.
// 번역 패턴이 이름 접미사를 만드는 유일한 경로이며 출력은 256바이트 버퍼 경계로 검사한다. (2026-11-08 WB04-E58)
export function formatSuffix(pattern, text) {
    let result = '';
    let index = 0;
    // 패턴 글자를 순서대로 복사하거나 변환 지정을 해석한다.
    while (index < pattern.length) {
        const char = pattern[index];
        if (char === '\0') break;
        if (char !== '%') {
            result += char;
            index += 1;
            continue;
        }
        const match = /^%(-?)(\d*)(?:\.(\d+))?s|^%%/.exec(pattern.slice(index));
        if (!match) throw new Error('객체 이름 접미사 서식 미복원');
        if (match[0] === '%%') {
            result += '%';
        } else {
            let part = match[3] === undefined ? text : text.slice(0, Number(match[3]));
            const width = match[2] === '' ? 0 : Number(match[2]);
            if (part.length < width) part = match[1] === '-' ? part.padEnd(width) : part.padStart(width);
            result += part;
        }
        index += match[0].length;
    }
    if (result.length >= SUFFIX_BYTES) throw new RangeError('Factory 이름 접미사 원본 버퍼 범위');
    return result;
}

// 원본 객체 가상 +68 이름 몸체이다. pool 은 SidPool(slot(sid) 사본으로 TYPE·가상 표를 읽음), types 는 188칸 {description} 표(현재 값을 매번 읽음),
//   hooks = { language() → 번호, translate(문장) → 문장, substitute(문장) → 문장, level(sid) → 가상 레벨 }. 생성 때는 이름·장치 조회를 실행하지 않는다. (2026-11-08 WB04-E58)
export class RawSquidName {
    // 같은 월드 풀·TYPE 표·장치 경계이다.
    #pool;
    #types;
    #hooks;

    // 188칸 TYPE 표(10.78)만 받는다. (2026-11-08 WB04-E58)
    constructor(pool, types, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT) throw new TypeError('객체 이름 같은PATCH/188TYPE 누락');
        this.#pool = pool;
        this.#types = types;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드 연결을 돌려준다. (2026-11-08 WB04-E58)
    get pool() { return this.#pool; }

    // 004ace20 base 이름이다: TYPE 번호 선택→언어 조회→설명 읽기(현재 값)→번역→80바이트 확인→설정 치환→80바이트 확인.
    //   cpppj 기준선 뒤 BaseName 공개 접근자(8ee7666b)와 같다. (2026-11-08 WB04-E58)
    baseName(parent) {
        const type = this.#pool.slot(parent)[TYPE_OFFSET];
        if (type >= this.#types.length) throw new RangeError('객체 이름 TYPE 범위');
        const first = need(this.#hooks.language)() >>> 0;
        const translated = first !== 0 && (need(this.#hooks.language)() >>> 0) !== 1;
        let description = this.#types[type].description;
        const end = description.indexOf('\0');
        if (end !== -1) description = description.slice(0, end);
        if (translated) description = need(this.#hooks.translate)(description);
        description = textOf(description, NAME_BYTES);
        return textOf(need(this.#hooks.substitute)(description), NAME_BYTES);
    }

    // 가상 +68 이름이다. 가상 표의 진입점이 base 이면 base 이름, 아니면 Factory 이름(base→번역 패턴→현재 레벨→로마숫자→접미사)이다. (2026-11-08 WB04-E58)
    name(parent) {
        const raw = this.#pool.slot(parent);
        const table = (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
        const entry = squidNameEntry(table);
        let name = this.baseName(parent);
        if (entry === BASE_ENTRY) return name;
        const pattern = need(this.#hooks.translate)(LEVEL_PATTERN);
        const level = squidLevelName(need(this.#hooks.level)(parent));
        name += formatSuffix(pattern, level);
        return textOf(name, NAME_BYTES);
    }
}
