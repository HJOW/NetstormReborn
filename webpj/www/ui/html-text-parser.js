// 원본 HtmlGump의 HTML 유사 본문을 실제 StyleText 입력 문자열로 바꾸는 기본 변환기를 옮긴다(cpppj client/HtmlTextParser.cpp/.h, 기준선 1f00a25a, 10.78 경로).
// 문자열은 한 글자가 한 BYTE인 원문 BYTE 문자열이다(0..255). 태그 분기·조건별 현재 상태 재조회·원본 출력 바이트 순서·콜백이 바꾼 현재 위치 재조회·물리 출력 한도를 그대로 유지한다.
// CD 10.72 판본 인자(H 글꼴 차이)는 웹 범위 밖이라 옮기지 않는다(AGENTS 웹 범위·10.78 전용).
// 사용: const parser = new HtmlTextParser(createHtmlTextParserState(), createHtmlTextParserGlobals(), hooks); parser.parse('<h2>제목</h2>'); (2026-11-14 WB04-E460)

// 원본 평문 순회가 태그와 마지막 LF 를 위해 남기는 논리 여유 BYTE 수다. 출력이 (용량 - 256) 을 넘으면 평문 처리를 멈춘다.
const OUTPUT_RESERVE = 256;
// 원본 출력 버퍼의 기본 물리 용량이다(NUL 자리 포함).
export const HTML_TEXT_CAPACITY = 8192;
// 565e70 배경 이름 저장소의 BYTE 수다.
export const HTML_BACKGROUND_SIZE = 80;

// 기본 파서의 현재 필드를 만든다. 출력 설정은 afterHeading 만 지우고 조건 가림 상태(shown/hiddenDepth)는 유지한다.
// shown: +1c 현재 평문/태그 효과 표시, hiddenDepth: +20 닫힘 조건 태그 허용 깊이, inTag: +24 '<' 뒤 '>' 앞, textAllowed: +28 shown 에서 시작해 소문자 a 열림/닫힘이 증감하는 signed 판정 카운터,
// afterHeading: +2c H 닫힘 뒤 공백만 출력한 상태, inputOffset: +4 현재 처리 위치(콜백이 바꿀 수 있음), output: 실제 StyleText 제어문자를 포함한 출력(길이가 출력 포인터 이동량).
export function createHtmlTextParserState() {
    return { shown: 1, hiddenDepth: 0, inTag: 0, textAllowed: 0, afterHeading: 0, inputOffset: 0, output: '' };
}

// 같은 미션/Tell 의 HTML 변환이 읽는 장기 전역을 만든다. suppressParagraph 는 다음 P 에서 출력 대신 0 으로 돌아가고, background 는 565e70..565ebf 의 실제 80BYTE 이름 저장소다.
// 페이지 교체와 짧은 덮어쓰기에서 나머지 바이트를 보존하기 위해 문자열이 아닌 Uint8Array 다.
export function createHtmlTextParserGlobals() {
    return { suppressParagraph: 0, background: new Uint8Array(HTML_BACKGROUND_SIZE) };
}

// 첫 NUL 앞까지의 원문 BYTE 문자열을 돌려준다(C++ string_view 의 find('\0') 절단).
function cString(value) { const end = value.indexOf('\0'); return end < 0 ? value : value.slice(0, end); }
// NUL 문자열 끝의 가상 0 을 제공한다. 호스트 문자열 밖은 읽지 않는다.
function byteAt(source, offset) { return offset < source.length ? source[offset] : '\0'; }
// 필수 효과가 없으면 해당 실제 분기를 거부한다. 기본 VT 의 검증된 noop 와 구분한다(C++ std::logic_error).
function required(hook, name) {
    if (typeof hook !== 'function') throw new Error(`HTML 실제 효과 미연결: ${name}`);
    return hook;
}
// 그림 태그 `<!"이름.프레임">` 의 원문에서 StyleText 그림 명령 `~[I이름.프레임]` 을 만든다(PATCH0046bf40, 첫 따옴표의 내부를 옮긴다). 자산 해석/그림 출력은 하지 않는다.
// 인용 전/안의 '>' 와 첫 NUL 은 원본처럼 중단하며 이름/프레임 바이트는 정규화하지 않는다. 따옴표가 없으면 `~[I]` 다. 사용: htmlPictureText('!"fortgump.3">') → '~[Ifortgump.3]'.
export function htmlPictureText(raw) {
    const source = cString(raw); let result = '~[I'; let position = 0;
    // 태그 이름 뒤 공백/속성을 해석하지 않고 원본 첫 따옴표(또는 '>')까지 간다.
    while (position < source.length && source[position] !== '>' && source[position] !== '"') position += 1;
    if (position < source.length && source[position] === '"') {
        position += 1;
        // 인용 내부도 다음 따옴표/꺾쇠에서 끝나며 HTML entity 나 이스케이프를 풀지 않는다.
        while (position < source.length && source[position] !== '>' && source[position] !== '"') { result += source[position]; position += 1; }
    }
    return `${result}]`;
}
// 실제 그림 문자열 helper 를 picture 효과에 연결한다(StyleTextMarkup 의 실제 자산 연결은 별도). hooks 를 제자리에서 바꾼다.
export function bindHtmlPictureText(hooks) {
    hooks.picture = (_state, raw) => htmlPictureText(raw); // 원본 helper 의 정확한 바이트 출력을 공급한다.
}
// 배경 태그의 쓰기를 같은 Client 의 80BYTE 에 연결한다. 이미 태그 파서가 구한 원문 이름을 NUL 까지 복사하며 뒤쪽 기존 바이트를 지우지 않는다. 원본 범위 밖 입력은 명시 거부한다(std::length_error).
export function bindHtmlBackground(hooks, globals) {
    hooks.background = (_state, name) => { // 실제 쓰기 직전까지 같은 Client 저장소의 수명을 빌린다.
        const value = cString(name);
        if (value.length >= globals.background.length) throw new RangeError('HTML 배경80BYTE NUL 범위 초과');
        // 원문 BYTE 만 복사하고 UTF16 을 암묵 절단하지 않는다. 쓰기 전에 전체를 검사해 일부만 쓰인 저장소를 남기지 않는다.
        for (let index = 0; index < value.length; index += 1) if (value.charCodeAt(index) > 255) throw new RangeError('HTML 배경 원문 BYTE 문자열 필요');
        for (let index = 0; index < value.length; index += 1) globals.background[index] = value.charCodeAt(index);
        globals.background[value.length] = 0;
    };
}
// 현재 첫 NUL 앞 배경 이름을 BYTE 문자열로 읽는다. 태그가 없는 다음 페이지나 숨긴 태그도 직전 전역 이름을 유지한다. NUL 이 없으면 거부한다.
export function htmlBackgroundName(globals) {
    const end = globals.background.indexOf(0);
    if (end < 0) throw new RangeError('HTML 배경80BYTE NUL 없음');
    let name = '';
    // 첫 NUL 앞 BYTE 를 원문 문자열로 되돌린다.
    for (let index = 0; index < end; index += 1) name += String.fromCharCode(globals.background[index]);
    return name;
}

// PATCH0046bea0/bed0/bf00/c090/c810 의 기본 변환이다. Config 나 게임 명령을 직접 재구현하지 않고 hooks 로 공급받는다.
// hooks: command(state, 원시명령) · condition(state, 원문, 닫힘) · picture(state, 원문)→문자열 · link(state, 원문, 닫힘)→문자열 · background(state, 이름)
// · insideTag/beforeClose/afterTag(state, source) 파생 슬롯(기본 VT 는 noop). 훅 표는 생성 때 값복사한다(C++ std::function 값 복사).
export class HtmlTextParser {
    // 같은 현재 상태/전역을 참조한다. 파생/외부 효과는 해당 분기가 선택될 때만 요구한다.
    #state; #globals; #hooks;
    constructor(state, globals, hooks = {}) { this.#state = state; this.#globals = globals; this.#hooks = { ...hooks }; }

    // 다음 제어문자/평문을 물리 출력 버퍼 범위에서만 추가한다. 원본의 NUL 쓰기 자리도 물리 한도에 포함한다.
    #append(text, capacity) {
        if (this.#state.output.length + text.length >= capacity) throw new RangeError('HTML 원본 물리 출력 버퍼 초과');
        this.#state.output += text;
    }

    // 콜백이 입력 포인터나 출력 포인터를 현재 버퍼 밖으로 바꾸는 경우만 거부한다(std::out_of_range).
    #validate(source, capacity) {
        if (this.#state.inputOffset > source.length || this.#state.output.length >= capacity) throw new RangeError('HTML callback 현재 버퍼 위치 오류');
    }

    // 원본의 첫 바이트만 ASCII 소문자로 정규화해 분기를 선택한다(원본은 signed char 로 확장하지만 0x80 이상은 어느 분기와도 같지 않아 같은 결과). 태그 전체를 소문자로 바꾸지 않고 둘째/셋째 글자는 literal 비교다.
    // offset 은 '<'(닫힘이면 '/') 다음 글자 위치다. 입력 밖 위치는 NUL 이라 어느 분기와도 같지 않다.
    #tag(source, offset, closing, capacity) {
        const state = this.#state; const globals = this.#globals; const hooks = this.#hooks;
        const code = source.charCodeAt(offset); const first = code >= 65 && code <= 90 ? code + 32 : code; const second = byteAt(source, offset + 1); // 위치 밖이면 NaN → NUL 글자다.
        const remaining = source.slice(Math.min(offset, source.length)); let result = '';
        switch (String.fromCharCode(first)) {
            case '!': this.#append(required(hooks.picture, 'picture')(state, remaining), capacity); break;
            case '$': required(hooks.command, 'command')(state, remaining.slice(1)); break;
            case '?': required(hooks.condition, 'condition')(state, remaining, closing); break;
            case 'a':
                if (closing) { this.#append(required(hooks.link, 'link-close')(state, '', true), capacity); result = '~N~.'; }
                else if (byteAt(source, offset + 2) === 'h') { this.#append(required(hooks.link, 'link-open')(state, remaining, false), capacity); result = '~l~E'; }
                break;
            case 'b':
                if (second === 'r') { result = '\n'; state.afterHeading = 0; }
                else if (second === 'a') {
                    const end = remaining.indexOf('>'); const equal = remaining.indexOf('=');
                    if (equal >= 0 && (end < 0 || equal < end)) {
                        let start = equal + 1; if (byteAt(remaining, start) === '"') start += 1; let finish = start;
                        // 배경 이름은 첫 따옴표/꺾쇠/NUL 앞에서 끝난다. 공백이나 대소문자는 바꾸지 않는다.
                        while (finish < remaining.length && remaining[finish] !== '>' && remaining[finish] !== '"') finish += 1;
                        required(hooks.background, 'background')(state, remaining.slice(start, finish));
                    }
                } else result = closing ? '~N' : '~N~B';
                break;
            case 'c': result = closing ? '~.' : '~N~i'; break;
            case 'h':
                if (closing) { result = '~*~N~.\n'; state.afterHeading = 1; }
                else if (second === '3') result = '~N~5~B~E';
                else if (second === '1') result = '~N~4~E';
                else if (second === '2') result = '~N~3~E';
                else result = '~N~1~E';
                break;
            case 'i': result = closing ? '~.' : '~N~j'; break;
            case 'p':
                if (globals.suppressParagraph) globals.suppressParagraph = 0;
                else { result = state.afterHeading ? '~*\n' : '~*\n \n'; state.afterHeading = 0; }
                break;
            case 'q': result = closing ? '~.' : '~N~q'; break;
            case '~': this.#append('~', capacity); if (second !== '\0') this.#append(source[offset + 1], capacity); break;
            default: break;
        }
        this.#validate(source, capacity); this.#append(result, capacity);
    }

    // NUL 앞 원본 바이트를 변환해 StyleText 입력을 돌려준다. logical capacity-256 의 strict 경계와 마지막 LF 를 유지한다.
    // 태그가 실제 물리 capacity 밖을 쓰려는 원본 오버런은 RangeError 로 거부한다. 반환 문자열의 NUL 은 길이에 포함하지 않는다.
    parse(input, capacity = HTML_TEXT_CAPACITY) {
        const state = this.#state; const hooks = this.#hooks; capacity |= 0;
        if (capacity < 2) throw new RangeError('HTML 마지막 LF/NUL 을 담을 물리 버퍼 필요');
        const source = cString(input); state.inputOffset = 0; state.output = ''; state.afterHeading = 0; state.textAllowed = state.shown >>> 0; state.inTag = 0;
        // 태그 callback 이 바꾼 현재 cursor 를 다음 반복에서 다시 읽는다. 태그 전체를 한 번에 건너뛰지 않는다.
        while (byteAt(source, state.inputOffset) !== '\0') {
            const byte = source.charCodeAt(state.inputOffset);
            if (byte === 0x3c) { // '<'
                state.inTag = 1;
                if (!state.hiddenDepth && byteAt(source, state.inputOffset + 1) === 'a') state.textAllowed = (state.textAllowed + 1) >>> 0;
                const closing = byteAt(source, state.inputOffset + 1) === '/';
                if (state.shown || (state.hiddenDepth && closing && byteAt(source, state.inputOffset + 2) === '?')) {
                    if (closing) {
                        state.inputOffset += 1; if (byteAt(source, state.inputOffset + 1) === 'a') state.textAllowed = (state.textAllowed - 1) >>> 0;
                        if (hooks.beforeClose) hooks.beforeClose(state, source);
                        this.#validate(source, capacity);
                    }
                    this.#tag(source, state.inputOffset + 1, closing, capacity);
                }
            } else if (byte === 0x3e) { // '>'
                state.inTag = 0; if (state.shown && hooks.afterTag) hooks.afterTag(state, source);
            } else if (!state.inTag) {
                if (state.shown) {
                    if ((state.textAllowed | 0) < 1 || state.output.length > capacity - OUTPUT_RESERVE) break;
                    if (byte > 0x1f) {
                        this.#append(source[state.inputOffset], capacity);
                        if (state.afterHeading) state.afterHeading = byteAt(source, state.inputOffset) === ' ' ? 1 : 0;
                    }
                    const last = state.output[state.output.length - 1];
                    if (byteAt(source, state.inputOffset) === '\n' && state.output.length > 0 && last !== ' ' && last !== '\n') this.#append(' ', capacity);
                }
            } else if (hooks.insideTag) hooks.insideTag(state, source);
            this.#validate(source, capacity); state.inputOffset += 1;
        }
        this.#append('\n', capacity); return state.output;
    }
}
