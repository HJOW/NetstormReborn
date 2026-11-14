// StyleText `~[...]` 대괄호 명령(그림·소리·태그·전경/배경 색·그림자)을 같은 조각 저장소와 그림(Markup)에 연결한다.
// (cpppj client/StyleTextMarkup.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72의 I→T 순서·W 소리 형식·판본 인자는 옮기지 않는다)
// 원본 정규식 여섯 개의 "부분 검색 + greedy 캡처 + 뒤로 되돌리기"를 정규식 엔진 없이 같은 순서로 구현한다. 문자열은 바이트 하나가 문자 하나인 JS 문자열이다.
// 기준선 뒤 변경 하나(알려진 버그 NB02: E 명령 두 좌표 모두 -4..4일 때만 적용)는 3.6절 ②로 판정해 함께 옮겼다. 전용 PE fixture는 없다.
// (2026-11-14 WB04-E421: RawMenuControls 선행 StyleTextMarkup 이식)
import { configParseLong } from '../assets/config.js';
import { PATCH_TYPE_COUNT, FIRST_ASSET_TYPE_NUMBER } from '../assets/type-table.js';
import { appendStyleTextFragment } from './style-text-fragments.js';

// Regexp::Get 이 각 캡처에서 복사하는 최대 바이트 수다. 프레임 식의 원본 지역 배열도 80바이트다.
const CAPTURE_BYTES = 80;
// 10.78 패치의 검사 순서다. 한 대괄호에서는 처음 성립한 형식 하나만 처리한다.
const COMMAND_ORDER = ['I', 'S', 'T', 'C', 'B', 'E'];
// E 좌표의 허용 범위다(알려진 버그 NB02 수정 뒤에는 두 좌표가 모두 이 범위 안이어야 한다).
const OFFSET_MINIMUM = -4;
const OFFSET_MAXIMUM = 4;
// 다중 줄의 제한 그림이 맞추는 너비/높이와 SHP 헤더 +1a signed 높이 허용 상한(10.78)이다.
const RESTRICTED_WIDTH = 99999;
const RESTRICTED_HEIGHT = 12;
const PATCH_HEIGHT_LIMIT = 16;
// 글자 구간 표의 글자 수('A'~'P')와 첫 글자 'A'의 코드다.
const LETTER_COUNT = 16;
const LETTER_A = 65;

// 원본 zone 옵션과 그림 맞춤 전역이다. 허용 값은 bool 로 자르지 않고 0/비0 로 읽는다(실제 패치 PE 기본값은 모두 1, 맞춤 상자는 65×60).
// 사용: new StyleTextMarkupState() 를 만들고 loadStyleTextMarkupOptions 로 설정 파일 값을 읽는다.
export class StyleTextMarkupState {
    // 소리/배경 색/전경 색/오프셋 명령의 허용 여부(0이면 형식은 소비하지만 효과를 적용하지 않는다).
    allowSounds = 1; allowBackgroundColor = 1; allowForegroundColor = 1; allowOffset = 1;
    // `#`·`&` 그림의 상자 맞춤 너비/높이다.
    fitWidth = 65; fitHeight = 60;
}

// 원본 타입 +114/+118/+11c/+12c/+130 의 그림 선택 필드 빈 보기다(내장 무자산 타입). 글자 표의 빈 칸은 -1이다.
// 사용: type 훅이 무자산 타입에 이 값을 돌려준다.
export function emptyStyleTextMarkupType() {
    return { frameCount: 0, defaultFrame: 0, gumpFrame: 0, letterKinds: 0, first: new Array(LETTER_COUNT).fill(-1) };
}

// 채팅 글자의 ~[E.x# y#] 엠보스 오프셋이 허용 범위(-4..4) 안인지 판정한다.
// 원문은 두 좌표 중 하나만 범위 안이어도 둘 다 적용해 다른 좌표를 크게 줘서 글자를 뒤로 밀 수 있었다(알려진 버그 NB02). 두 좌표가 모두 범위 안일 때만 허용한다.
// 사용: styleTextOffsetAllowed(0, 1000) → false. (2026-11-14 WB04-E421: cpppj 기준선 뒤 NB02 수정 반영, 3.6절 ②)
export function styleTextOffsetAllowed(x, y) {
    return x >= OFFSET_MINIMUM && x <= OFFSET_MAXIMUM && y >= OFFSET_MINIMUM && y <= OFFSET_MAXIMUM;
}

// 도달한 가상 장치만 요구한다. 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 알린다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }
// 원본 [^.\r\n ]+ 에 들어갈 수 있는 글자인지 판정한다. 탭은 캡처에 포함되며 NUL 은 호출자가 미리 자른다.
function isToken(character) { return character !== '.' && character !== '\r' && character !== '\n' && character !== ' '; }
// 원본 공백 반복은 ASCII space 만 소비한다(일반 isspace 로 넓히지 않는다). greedy 공백 반복의 가장 긴 끝을 돌려준다.
function skipSpaces(text, position) { while (position < text.length && text[position] === ' ') position += 1; return position; }
// 원본 캡처의 가장 긴 끝을 찾는다. 이후 형식이 실패하면 호출자가 끝점을 한 글자씩 줄인다.
function tokenEnd(text, position) { while (position < text.length && isToken(text[position])) position += 1; return position; }
// Regexp::Get(capture, 0, 80, 0) 처럼 캡처의 첫 80바이트만 숫자/이름 처리에 쓴다.
function capture(value) { return value.slice(0, CAPTURE_BYTES); }
// 캡처를 CRT 정수(공백/부호/첫 숫자 구간/DWORD 감김)로 읽는다. 결과는 signed 32비트다.
function number(value) { return configParseLong(capture(value)); }
// 프레임 글자 구간 첫 인덱스와 숫자 suffix 의 원본 DWORD 덧셈이다(signed 32비트로 감긴다).
function add(first, second) { return (first + second) | 0; }

// ' *. *([^...]+)' 의 첫 캡처다. 원본의 점은 이스케이프되지 않아 임의 글자 한 개(공백·CR·LF 포함)를 소비한다. 없으면 null 이다.
// 사용: separated('. abc', 0) → 'abc'. 앞 공백은 greedy 끝에서 한 칸씩 되돌리며 성립하는 첫 조합을 고른다.
function separated(text, position) {
    let separator = skipSpaces(text, position);
    // 공백 반복을 가장 긴 끝부터 되돌리며 점 + 공백 + 캡처가 성립하는 첫 조합을 찾는다.
    for (;;) {
        if (separator < text.length) {
            const begin = skipSpaces(text, separator + 1);
            const end = tokenEnd(text, begin);
            if (end > begin) return text.slice(begin, end);
        }
        if (separator === position) return null;
        separator -= 1;
    }
}

// ROM 의 제한된 형식을 부분 검색한다(시작/끝 앵커 없음, 대소문자 구분). { first, second } 또는 null 이다. 전체 Regexp 엔진은 아니다.
// 사용: matchCommand('I mana . 2', 'I') → { first: 'mana', second: '2' }.
function matchCommand(text, command) {
    // 정규식의 첫 일치 위치를 앞에서부터 찾는다.
    for (let mark = 0; mark < text.length; mark += 1) {
        if (text[mark] !== command) continue;
        const begin = skipSpaces(text, mark + 1);
        if (command === 'T') {
            // T 는 숫자 캡처만 허용한다. 부호/다른 글자는 뒤의 부분 검색 후보로 남긴다.
            let end = begin;
            while (end < text.length && text[end] >= '0' && text[end] <= '9') end += 1;
            if (end > begin) return { first: text.slice(begin, end), second: '' };
        } else if (command === 'I' || command === 'E') {
            // 첫 캡처의 가장 긴 길이부터 두 번째 캡처가 성립하는 첫 조합을 선택한다.
            for (let end = tokenEnd(text, begin); end > begin; end -= 1) {
                const second = separated(text, end);
                if (second !== null) return { first: text.slice(begin, end), second };
            }
        } else {
            const first = separated(text, mark + 1);
            if (first !== null) return { first, second: '' };
        }
    }
    return null;
}

// 원본 호출 단위의 StyleText 대괄호 처리기다. 공통 조각/그림(Markup)/zone 옵션/자산·팔레트·소리 경계는 생성한 bracket 훅보다 오래 살아야 한다.
// 사용: const markup = new StyleTextMarkup(fragments, pictures, state, hooks); styleHooks = markup.hooks(styleHooks); new StyleText(fragments, styleHooks).
export class StyleTextMarkup {
    // 같은 공통 조각 저장소·그림 표·옵션 상태·외부 조회 훅이다.
    #fragments; #pictures; #state; #hooks;
    // 호출자가 같은 옵션/타입/조각/출력 저장소를 소유한다(원본 전역 대신).
    constructor(fragments, pictures, state, hooks = {}) { this.#fragments = fragments; this.#pictures = pictures; this.#state = state; this.#hooks = hooks; }

    // 타입 번호의 현재 선택 필드를 조회한다. 호스트 범위 밖 입력은 공급 경계가 RangeError 로 거부한다. 패치의 타입 helper 는 호출마다 다시 조회한다.
    #type(type) { return need(this.#hooks.type, 'StyleText 대괄호 현재 타입 경계 필요')(type | 0); }

    // 타입 캡처를 CRT 정수로 읽어 0이면 같은 문자열을 타입 이름 표에서 다시 조회한다(-1 은 호출자가 그림 생성을 생략한다). 결과는 signed 32비트다.
    #resolveType(name, message) {
        let type = number(name);
        if (!type) type = need(this.#hooks.findType, message)(name) | 0;
        return type;
    }

    // 프레임 글자 식의 접두를 해석한다. 첫 글자가 signed 비교로 'A' 이상이고 글자 종류 수 안이면 그 구간 첫 인덱스를 돌려주고 접두 한 글자를 소비한다.
    // 반환 { frame, suffix }. 타입 helper 는 patch 순서대로 두 번(글자 종류 → 첫 인덱스) 조회한다.
    #letterFrame(type, expression, rangeMessage) {
        let frame = 0; let suffix = expression;
        const letter = (expression.charCodeAt(0) << 24) >> 24;
        if (letter >= LETTER_A) {
            const view = this.#type(type);
            if (view.letterKinds < 0 || view.letterKinds > LETTER_COUNT) throw new RangeError(rangeMessage);
            if (letter < LETTER_A + view.letterKinds) { frame = this.#type(type).first[letter - LETTER_A] | 0; suffix = suffix.slice(1); }
        }
        return { frame: add(frame, number(suffix)), suffix };
    }

    // 원본 상한은 count-1 이 아닌 count 다(count 프레임은 이후 Markup.lookup 에서 무자산이 된다). 타입 helper 를 판본 순서대로 다시 조회한다.
    #clamp(type, frame) {
        const count = this.#type(type).frameCount | 0;
        if (Math.min(frame, count) < 0) return 0;
        if (frame >= (this.#type(type).frameCount | 0)) return this.#type(type).frameCount | 0;
        return frame;
    }

    // I 명령: 타입/프레임 식을 원본 signed 비교·감김·클램프로 해석해 그림 조각을 만들고 공통 체인 뒤에 추가한다. head 는 실제 소유 필드 별칭({ value })이다.
    // 순서: 그림 생성(현재 태그) → mapped BYTE → 필요하면 Fit → 공통 꼬리 추가.
    #picture(typeExpression, frameExpression, head, metadata) {
        const name = capture(typeExpression); const expression = capture(frameExpression);
        // 원본은 80바이트 지역 배열에 NUL 까지 strcpy 하므로 경계 밖 쓰기가 되는 캡처만 거부한다.
        if (expression.length >= CAPTURE_BYTES) throw new RangeError('StyleText 그림 프레임 지역 배열 범위 초과');
        const type = this.#resolveType(name, 'StyleText 대괄호 타입 이름 경계 필요');
        if (type === -1) return;
        const first = expression[0];
        let frame = 0; let fit = false; let mapped = false;
        if (first === '*' || first === '#' || first === '&') {
            // `*`는 gumpFrame(-1 이면 defaultFrame), `#`는 상자 맞춤, `&`는 상자 맞춤 + 색 치환 BYTE 1이다.
            fit = first !== '*'; mapped = first === '&';
            const gump = this.#type(type).gumpFrame | 0;
            frame = gump === -1 ? this.#type(type).defaultFrame | 0 : this.#type(type).gumpFrame | 0;
        } else frame = this.#letterFrame(type, expression, 'StyleText 프레임 글자 표 범위 초과').frame;
        frame = this.#clamp(type, frame);
        const picture = this.#pictures.newPicture(type >>> 0, frame >>> 0, metadata);
        this.#pictures.setMapped(picture, mapped ? 1 : 0);
        if (fit) this.#pictures.fit(picture, this.#state.fitWidth, this.#state.fitHeight);
        appendStyleTextFragment(this.#fragments, head, picture);
    }

    // 형식 일치 뒤 수행하는 소리/태그/색/그림자다. 비활성 옵션도 호출자는 이미 소비한 것으로 처리한다. tag 는 현재 공통 태그 별칭({ value })이다.
    #option(code, first, second, context, tag) {
        switch (code) {
            case 'S':
                if (this.#state.allowSounds) need(this.#hooks.sound, 'StyleText 대괄호 소리 경계 필요')(capture(first));
                break;
            case 'T': tag.value = number(first) >>> 0; break;
            case 'C': if (this.#state.allowForegroundColor) context[2] = number(first) >>> 0; break;
            case 'B': if (this.#state.allowBackgroundColor) context[5] = number(first) >>> 0; break;
            case 'E':
                if (this.#state.allowOffset) {
                    const x = number(first); const y = number(second);
                    if (styleTextOffsetAllowed(x, y)) {
                        // 검정 별칭 훅이 던져도 이미 더한 그림자 비트는 남는다(원본 순서: 비트 → 색 → 좌표).
                        const black = need(this.#hooks.black, 'StyleText 대괄호 검정색 경계 필요');
                        context[0] |= 2; context[5] = black(); context[6] = x >>> 0; context[7] = y >>> 0;
                    }
                }
                break;
            default: break;
        }
    }

    // 닫는 대괄호 내부의 첫 NUL 까지만 읽고 I→S→T→C→B→E 중 처음 성립한 형식만 처리한다. 모르는 형식은 문맥/태그/체인을 바꾸지 않는다.
    // 사용: apply('I mana.2', context, head, tag) — context 는 Uint32Array(8), head/tag 는 { value } 별칭이다.
    apply(command, context, head, tag) {
        const nul = command.indexOf('\0'); if (nul >= 0) command = command.slice(0, nul);
        // 형식이 성립하면 옵션이 비활성이어도 다음 형식을 시도하지 않고 소비한다.
        for (const code of COMMAND_ORDER) {
            const match = matchCommand(command, code);
            if (!match) continue;
            if (code === 'I') this.#picture(match.first, match.second, head, tag.value); else this.#option(code, match.first, match.second, context, tag);
            return;
        }
    }

    // 다중 줄 그림: 프레임 식은 `*`와 `#`만 특수 처리하고 소비한 접두 뒤의 `#`가 상자 맞춤을 결정한다. 숫자 타입은 현재 타입 수 상한 안일 때만 허용한다.
    // restricted() 가 참이면(+d4 제한 모드) 타입 1 이상·프레임 범위·SHP 헤더 +1a 높이 ≤16 인 그림만 너비 99999/높이 12 로 맞춘다. appendPicture(type, frame, width, height) 가 실제 버퍼에 연결된다.
    #scrollingPicture(typeExpression, frameExpression, restricted, appendPicture) {
        const name = capture(typeExpression); const expression = capture(frameExpression);
        if (expression.length >= CAPTURE_BYTES) throw new RangeError('다중 줄 그림 프레임 지역 배열 범위 초과');
        let type = number(name);
        if (!type) type = need(this.#hooks.findType, '다중 줄 그림 타입 이름 경계 필요')(name) | 0;
        else if (type >= need(this.#hooks.typeCount, '다중 줄 그림 현재 타입 수 경계 필요')()) return;
        if (type === -1) return;
        let frame = 0; let suffix = expression;
        if (expression[0] === '*' || expression[0] === '#') {
            const gump = this.#type(type).gumpFrame | 0;
            frame = gump === -1 ? this.#type(type).defaultFrame | 0 : this.#type(type).gumpFrame | 0;
            suffix = suffix.slice(1);
        } else ({ frame, suffix } = this.#letterFrame(type, expression, '다중 줄 그림 글자 표 범위 초과'));
        let width = 0; let height = 0;
        if (suffix.length > 0 && suffix[0] === '#') { width = this.#state.fitWidth | 0; height = this.#state.fitHeight | 0; }
        frame = this.#clamp(type, frame);
        need(restricted, '다중 줄 그림 현재 제한 필드 경계 필요');
        if (restricted()) {
            const typeCount = need(this.#hooks.typeCount, '다중 줄 제한 그림 타입 수 경계 필요')();
            if (type <= 0 || type >= typeCount || frame < 0 || frame >= (this.#type(type).frameCount | 0)) return;
            const heightWord = need(this.#hooks.frameHeight, '다중 줄 제한 그림 SHP 헤더 경계 필요')(type, frame);
            // 무자산(nullopt)은 생성하지 않고, 값이 있으면 0 높이도 허용한다(원본은 값 유무와 상한만 검사한다).
            if (heightWord === null || heightWord === undefined || heightWord > PATCH_HEIGHT_LIMIT) return;
            width = RESTRICTED_WIDTH; height = RESTRICTED_HEIGHT;
        }
        need(appendPicture, '다중 줄 그림 실제 버퍼 추가 경계 필요')(type >>> 0, frame >>> 0, width, height);
    }

    // 004ba8f0 의 다중 줄 대괄호 처리다. 패치는 한 줄 파서와 같은 I→S→T→C→B→E 순서이며 처음 성립한 형식은 무효 타입/그림 제한/옵션 비활성에서도 다음 형식으로 넘어가지 않는다.
    // 사용: applyScrolling(text, context, tag, () => restricted, (type, frame, w, h) => buffer.append(...)).
    applyScrolling(command, context, tag, restricted, appendPicture) {
        const nul = command.indexOf('\0'); if (nul >= 0) command = command.slice(0, nul);
        // 한 줄 파서와 같은 순서로 처음 성립한 형식 하나만 처리하고 끝낸다(그림은 다중 줄 버퍼 경로).
        for (const code of COMMAND_ORDER) {
            const match = matchCommand(command, code);
            if (!match) continue;
            if (code === 'I') this.#scrollingPicture(match.first, match.second, restricted, appendPicture); else this.#option(code, match.first, match.second, context, tag);
            return;
        }
    }

    // 기존 글꼴/팔레트/그림 가상 훅을 유지한 새 묶음에 대괄호 처리를 이 객체로 연결해 돌려준다(원래 묶음은 바꾸지 않는다).
    hooks(hooks) { return { ...hooks, bracket: (source, context, head, tag) => this.apply(source, context, head, tag) }; }
}

// 실제 GameAssets(타입 이름·논리 프레임·특수 프레임·A~P 첫 인덱스·SHP 헤더)를 대괄호 훅에 연결한다.
// 타입 번호는 내장 무자산 슬롯(188칸 중 70 미만과 186~187)도 포함하며, 프레임은 물리 레이어가 아닌 논리 클러스터에서 공급한다.
// 사용: bindStyleTextMarkupAssets(hooks, gameAssets).
export function bindStyleTextMarkupAssets(hooks, assets) {
    const table = assets.typeTable;
    // 타입 수는 내장 무자산 포함 188 이다(원본 signed 상한 안).
    hooks.typeCount = () => PATCH_TYPE_COUNT;
    // 제한 그림은 추가 헤더 +1a 의 원본 signed 높이를 읽는다. 무자산·범위 밖 프레임은 null 이다.
    hooks.frameHeight = (type, frame) => {
        if (type < FIRST_ASSET_TYPE_NUMBER || frame < 0) return null;
        const index = type - FIRST_ASSET_TYPE_NUMBER; const types = assets.types();
        if (index >= types.length) return null;
        const asset = types[index];
        if (frame >= (asset.definition?.clusters.length ?? 0)) return null;
        const header = assets.shapes.squidHeader(asset.block, frame);
        return ((header[0x1a] | (header[0x1b] << 8)) << 16) >> 16;
    };
    // 이름 표는 로딩 순서 파일 이름이며 대소문자를 무시한다. 없으면 -1 이다.
    hooks.findType = (name) => table.find(name);
    // 원본 내장 무자산 타입은 빈 선택 필드를 돌려주고 잘못된 타입 번호는 거부한다.
    hooks.type = (type) => {
        const view = table.view(type);
        if (!view) throw new RangeError('StyleText 대괄호 실제 타입 범위 초과');
        if (view.frameCount > 0x7fffffff) throw new RangeError('StyleText 대괄호 실제 프레임 수 범위 초과');
        return { frameCount: view.frameCount, defaultFrame: view.defaultFrame, gumpFrame: view.gumpFrame, letterKinds: view.letterKinds, first: Array.from(view.first) };
    };
}

// E 명령이 현재 기본 검정 별칭(PaletteColorTable.basic[0])을 설치하도록 연결한다.
export function bindStyleTextMarkupPalette(hooks, palette) { hooks.black = () => palette.basic[0]; }

// 원본 Client 초기화의 ReadInt 처럼 키가 있으면 현재 옵션을 갱신하고 없으면 기본/이전 값을 유지한다(allowzonesounds/zonebgcolor/zonefgcolor/zoneoffset).
// 사용: loadStyleTextMarkupOptions(state, gameData) — gameData.readInt(key, 현재) → { changed, value }.
export function loadStyleTextMarkupOptions(state, configuration) {
    state.allowSounds = configuration.readInt('allowzonesounds', state.allowSounds).value;
    state.allowBackgroundColor = configuration.readInt('zonebgcolor', state.allowBackgroundColor).value;
    state.allowForegroundColor = configuration.readInt('zonefgcolor', state.allowForegroundColor).value;
    state.allowOffset = configuration.readInt('zoneoffset', state.allowOffset).value;
}

// S 명령은 위치·반복·우선·동시 재생 한도를 추가하지 않고 이름 재생만 요청한다(playByName(name, repeat, volume, pan, priority, limit) 모두 0).
// 소리 장치 초기화/실제 재생은 ClientAudio 단위 몫이다.
export function bindStyleTextMarkupSound(hooks, sound) { hooks.sound = (name) => sound.playByName(name, 0, 0, 0, 0, 0); }
