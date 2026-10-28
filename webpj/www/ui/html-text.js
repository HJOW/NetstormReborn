// 원본 HTML 대화상자(HtmlWindowGump)의 본문 처리를 복원하는 모듈이다: HTML 유사 본문 → 스타일 문자열 변환, `$Button=` 같은 지시문 추출,
// 그리고 두 번 변환하는 전체 준비 절차(원본 공장 004cf270)다.
//  - 기본 변환(원본 0046bf00 HtmlTextParser::Parse): `<h2>` `<p>` `<br>` `<i>` `<b>` `<c>` `<q>` `<a>` `<background=...>` 태그를 StyleText 제어문자열로 바꾼다.
//  - 지시문 추출(원본 004c83b0): `$Button` `$ButtonInstant` `$Menu` `$Checked` `$Timeout` `$OnExit` `$Key` `$Input` `$Zsend` 아홉 종류를 우선순위대로 찾아 컨트롤로 만들고 본문에서 지운다.
//  - 준비 절차(원본 004cf270): 설정 치환 → 첫 변환 → 지시문 추출(버튼·메뉴가 없으면 기본 확인 버튼 `OkButton`) → 모든 LF 를 `<br>` 로 바꿔 두 번째 변환 → 끝 LF 추가.
// 복원 기준: cpppj HtmlTextParser.cpp·HtmlWindowMetadata.cpp·HtmlWindowFactory.cpp(10.78 경로). 조건 태그 `<?`·그림 `<!`·링크 `<a>` 의 효과와 목록 메뉴 `@`·`#`·`%`·`*TECHLIST*` 등은 호출자가 훅으로 공급한다.
// (2026-10-28 WB03-D: Tell 대화상자 본문을 처리하기 위해 추가)

// 변환 결과 최대 길이(바이트)이다. 원본 출력 버퍼 8192바이트에서 NUL 자리를 뺀 한도로 쓴다. (2026-10-28 WB03-D)
const OUTPUT_CAPACITY = 8192;

// 평문 순회가 태그·마지막 LF 를 위해 남기는 출력 여유(바이트)이다. 출력이 (용량 - 256) 보다 길어지면 평문 처리를 멈춘다. (2026-10-28 WB03-D)
const OUTPUT_RESERVE = 256;

// 대화상자에 만들 수 있는 최대 버튼 수이다(원본 +0x20c4 의 20행 표). (2026-10-28 WB03-D)
export const MAX_DIALOG_BUTTONS = 20;

// 지시문 아홉 종류의 이름과 정규식이다(원본 4c83b0 의 9 regex 우선순위 순서). 앞 종류가 문서 뒤쪽에 있어도 뒤 종류의 앞쪽 위치보다 먼저 처리된다.
// 캡처 1 = 라벨(또는 초 수), 캡처 2 = 명령 이름, 캡처 3 = 인자(공백이나 `$` 앞에서 끝남). `$Checked` 는 캡처 5개다. (2026-10-28 WB03-D)
const DIRECTIVES = Object.freeze([
    Object.freeze({ kind: 'button', pattern: /\$Button[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/ }),
    Object.freeze({ kind: 'buttonInstant', pattern: /\$ButtonInstant[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/ }),
    Object.freeze({ kind: 'menu', pattern: /\$Menu[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/ }),
    Object.freeze({ kind: 'checked', pattern: /\$Checked[ \t]*=[ \t]*([^,]+),([^,]+),([^,]+),([^,]+),([^$ ]+)/ }),
    Object.freeze({ kind: 'timeout', pattern: /\$Timeout[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/ }),
    Object.freeze({ kind: 'onExit', pattern: /\$OnExit[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/ }),
    Object.freeze({ kind: 'key', pattern: /\$Key[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/ }),
    Object.freeze({ kind: 'input', pattern: /\$Input[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/ }),
    Object.freeze({ kind: 'zsend', pattern: /\$Zsend[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/ }),
]);

// 기본 확인 버튼 지시문의 대체값이다. 설정 키 OkButton 이 없을 때만 쓴다(원본 config.english: "$Button=OK,DoNothing,0"). (2026-10-28 WB03-D)
const DEFAULT_OK_BUTTON = '$Button=OK,DoNothing,0';

// 기본 변환의 현재 상태를 만든다(원본 HtmlTextParser 필드). shown 은 평문·태그를 표시하는지(조건 태그가 숨기면 0)이다. (2026-10-28 WB03-D)
export function createParserState() {
    return { shown: 1, hiddenDepth: 0, inTag: 0, textAllowed: 0, afterHeading: 0, inputOffset: 0, output: '' };
}

// 같은 대화상자 처리가 공유하는 전역이다(원본 565ec0·565e70). suppressParagraph 는 다음 `<p>` 의 빈 줄 출력을 한 번 막고,
// background 는 가장 최근 `<background="이름">` 의 이름이다. (2026-10-28 WB03-D)
export function createParserGlobals() {
    return { suppressParagraph: 0, background: '' };
}

// HTML 유사 본문을 StyleText 제어문자열로 바꾼다(원본 HtmlTextParser::Parse, 10.78 경로). 개행 문자는 평문 구분에 쓰고, 태그는 아래처럼 바뀐다.
//  `<br>` → LF, `<p>` → `~*` LF [+ 빈 줄], `<h1>` → `~N~4~E`, `<h2>` → `~N~3~E`, `<h3>` → `~N~5~B~E`, `</h?>` → `~*~N~.` LF, `<b>` → `~N~B`, `<i>` → `~N~j`,
//  `<c>` → `~N~i`, `<q>` → `~N~q`, `</b>`·`</i>`·`</c>`·`</q>` → `~N` 또는 `~.`, `<a href>` → 링크 훅 + `~l~E`, `</a>` → 링크 훅 + `~N~.`, `<background="이름">` → 전역 이름 저장.
// state 와 globals 는 호출자가 소유하고 hooks 는 선택 태그의 효과를 공급한다: { command, condition, picture, link, background }. 공급되지 않은 효과가 필요한 태그는 무시한다(호출자가 onUnsupported 로 알 수 있음).
// 마지막에 LF 하나를 붙여 돌려준다. 출력이 용량을 넘으면 RangeError 를 던진다.
// 사용: parseHtml('<h2>제목</h2>', state, globals, hooks) → '~N~3~E제목~*~N~.\n\n'. (2026-10-28 WB03-D)
export function parseHtml(source, state, globals, hooks = {}, capacity = OUTPUT_CAPACITY) {
    const text = source.includes('\0') ? source.slice(0, source.indexOf('\0')) : source;
    state.inputOffset = 0;
    state.output = '';
    state.afterHeading = 0;
    state.textAllowed = state.shown;
    state.inTag = 0;

    // 입력의 offset 위치 글자를 돌려준다(끝을 넘으면 NUL 로 취급한다).
    const byteAt = (offset) => (offset < text.length ? text[offset] : '\0');
    // 출력 뒤에 문자열을 붙인다(원본은 출력 버퍼 물리 한도에서 오버런하므로 여기서는 오류로 거부한다).
    const append = (piece) => {
        if (state.output.length + piece.length >= capacity) throw new RangeError('HTML 변환 출력이 버퍼 한도를 넘었습니다.');
        state.output += piece;
    };

    // 태그 하나를 처리한다. offset 은 '<'(또는 닫는 태그의 '/') 다음 글자 위치다.
    const tag = (offset, closing) => {
        const rawFirst = byteAt(offset);
        const first = rawFirst >= 'A' && rawFirst <= 'Z' ? rawFirst.toLowerCase() : rawFirst;
        const second = byteAt(offset + 1);
        const remaining = text.slice(Math.min(offset, text.length));
        let result = '';
        switch (first) {
            case '!':
                if (hooks.picture) append(hooks.picture(state, remaining));
                break;
            case '$':
                hooks.command?.(state, remaining.slice(1));
                break;
            case '?':
                hooks.condition?.(state, remaining, closing);
                break;
            case 'a':
                if (closing) {
                    if (hooks.link) append(hooks.link(state, '', true));
                    result = '~N~.';
                } else if (byteAt(offset + 2) === 'h') {
                    if (hooks.link) append(hooks.link(state, remaining, false));
                    result = '~l~E';
                }
                break;
            case 'b':
                if (second === 'r') {
                    result = '\n';
                    state.afterHeading = 0;
                } else if (second === 'a') {
                    // 배경 이름은 '=' 뒤 첫 따옴표를 건너뛰고 다음 따옴표·'>'·끝 앞까지다.
                    const end = remaining.indexOf('>');
                    const equal = remaining.indexOf('=');
                    if (equal >= 0 && (end < 0 || equal < end)) {
                        let start = equal + 1;
                        if (remaining[start] === '"') start += 1;
                        let finish = start;
                        // 이름의 끝을 찾는다.
                        while (finish < remaining.length && remaining[finish] !== '>' && remaining[finish] !== '"') finish += 1;
                        const name = remaining.slice(start, finish);
                        if (name.length >= 80) throw new RangeError('HTML 배경 이름이 80바이트를 넘었습니다.');
                        globals.background = name;
                        hooks.background?.(state, name);
                    }
                } else {
                    result = closing ? '~N' : '~N~B';
                }
                break;
            case 'c':
                result = closing ? '~.' : '~N~i';
                break;
            case 'h':
                if (closing) {
                    result = '~*~N~.\n';
                    state.afterHeading = 1;
                } else if (second === '3') result = '~N~5~B~E';
                else if (second === '1') result = '~N~4~E';
                else if (second === '2') result = '~N~3~E';
                else result = '~N~1~E';
                break;
            case 'i':
                result = closing ? '~.' : '~N~j';
                break;
            case 'p':
                if (globals.suppressParagraph) {
                    globals.suppressParagraph = 0;
                } else {
                    result = state.afterHeading ? '~*\n' : '~*\n \n';
                    state.afterHeading = 0;
                }
                break;
            case 'q':
                result = closing ? '~.' : '~N~q';
                break;
            case '~':
                append('~');
                if (second !== '\0') append(second);
                break;
            default:
                break;
        }
        append(result);
    };

    // 입력을 한 글자씩 읽으며 태그·평문을 처리한다. 태그 훅이 바꾼 현재 위치는 다음 반복에서 다시 읽는다.
    while (byteAt(state.inputOffset) !== '\0') {
        const char = byteAt(state.inputOffset);
        if (char === '<') {
            state.inTag = 1;
            if (!state.hiddenDepth && byteAt(state.inputOffset + 1) === 'a') state.textAllowed += 1;
            const closing = byteAt(state.inputOffset + 1) === '/';
            if (state.shown || (state.hiddenDepth && closing && byteAt(state.inputOffset + 2) === '?')) {
                if (closing) {
                    state.inputOffset += 1;
                    if (byteAt(state.inputOffset + 1) === 'a') state.textAllowed -= 1;
                    hooks.beforeClose?.(state, text);
                }
                tag(state.inputOffset + 1, closing);
            }
        } else if (char === '>') {
            state.inTag = 0;
            if (state.shown) hooks.afterTag?.(state, text);
        } else if (!state.inTag) {
            if (state.shown) {
                if ((state.textAllowed | 0) < 1 || state.output.length > capacity - OUTPUT_RESERVE) break;
                const code = char.charCodeAt(0);
                if (code > 0x1f) {
                    append(char);
                    if (state.afterHeading) state.afterHeading = char === ' ' ? 1 : 0;
                }
                // 소스의 줄바꿈은 앞 글자가 공백·LF 가 아니면 공백 하나가 된다.
                const last = state.output[state.output.length - 1];
                if (char === '\n' && state.output.length > 0 && last !== ' ' && last !== '\n') append(' ');
            }
        } else {
            hooks.insideTag?.(state, text);
        }
        state.inputOffset += 1;
    }
    append('\n');
    return state.output;
}

// 처리한 문자열에 영문·숫자가 하나라도 있는지 검사한다(원본 004c8d50 관문). 스타일 제어문자(`~w` 등)의 글자도 포함한다.
// 없으면(참이 아니면) 대화상자를 만들지 않고 빈 본문 경로로 간다. (2026-10-28 WB03-D)
export function hasAlphanumeric(text) {
    return /[A-Za-z0-9]/.test(text);
}

// 지시문 이름을 명령 객체로 바꾼다. 원본 4c83b0 은 명령 이름을 명령 번호로 풀지만(ResolveDialogExitCommand), 웹은 이름과 인자를 그대로 두고
// 실행하는 쪽(Tell 창의 onCommand)이 해석한다. command 는 대소문자 구분 없이 비교하도록 이름을 그대로 보존한다. (2026-10-28 WB03-D)
function makeCommand(name, argument) {
    return Object.freeze({ name, argument });
}

// 본문 문자열에서 지시문을 우선순위대로 모두 찾아 처리하고 본문에서 지운다(원본 HtmlWindowMetadata::Process, 4c83b0).
// 처리할 때마다 처음(버튼)부터 다시 검색한다. 지워지는 범위는 일치 시작부터 다음 LF·`$`·끝 앞까지(LF 가 끝이면 LF 포함)이다.
// 결과: { text(남은 본문), buttons[{label, command, instant}], menuItems[{label, command, kind, ...}], menuStarted, timeout, onExit, handledAny }.
//  - buttons: `$Button`·`$ButtonInstant`(instant=true). 20개를 넘는 단추는 만들지 않는다(처리한 수는 센다).
//  - menuItems: `$Menu`(일반 항목 또는 `@목록`·`#파일목록`·`%인라인목록`·특수 이름), `$Checked`(checked 값·활성 여부 포함).
//  - timeout: { seconds, command } 이며 대화상자가 열린 뒤 seconds 초 지나면 command 를 실행한다.
// hooks.expandMenu(label, command, argument) 가 있으면 `@`·`#` 목록 메뉴 항목 배열을 돌려받아 menuItems 에 이어 붙이고, hooks.inlineList 가 있으면 `%` 목록의 삽입 문자열을 받는다.
// 알 수 없거나 지원하지 않는 지시문은 hooks.onUnsupported 로 알리고 본문에서는 지운다. (2026-10-28 WB03-D)
export function extractDirectives(source, hooks = {}) {
    let text = source;
    const result = { text, buttons: [], menuItems: [], menuStarted: false, timeout: null, onExit: null, handledAny: false };
    let processedButtons = 0;
    // 지시문이 더 없을 때까지 우선순위 순서대로 첫 일치를 처리한다.
    for (;;) {
        let kindIndex = 0;
        let match = null;
        // 앞 종류의 첫 일치가 뒤 종류의 더 이른 위치보다 우선한다.
        for (; kindIndex < DIRECTIVES.length; kindIndex += 1) {
            match = DIRECTIVES[kindIndex].pattern.exec(text);
            if (match) break;
        }
        if (!match) break;
        const kind = DIRECTIVES[kindIndex].kind;
        const label = match[1];
        const command = match[2];
        const argument = match[3];
        let replacement = null;
        if (kind === 'checked') {
            result.menuStarted = true;
            result.menuItems.push({ kind: 'checked', label, command: makeCommand(command, argument), checked: Number.parseInt(match[4], 10) | 0, enabled: Number.parseInt(match[5], 10) | 0 });
        } else if (kind === 'menu') {
            result.menuStarted = true;
            const lower = label.toLowerCase();
            const resolved = makeCommand(command, argument);
            if (lower === '*sololist*' || lower === '*fortlist*' || lower === '*techlist*' || lower === '*themetechlist*') {
                hooks.onUnsupported?.(`메뉴 목록 ${label}`, match[0]);
            } else if (label.startsWith('@') || label.startsWith('#')) {
                const items = hooks.expandMenu?.(label, command, argument);
                if (items) result.menuItems.push(...items);
                else hooks.onUnsupported?.(`메뉴 목록 ${label}`, match[0]);
            } else if (label.startsWith('%')) {
                replacement = hooks.inlineList?.(label.slice(1), command, argument) ?? null;
                if (replacement === null) hooks.onUnsupported?.(`인라인 목록 ${label}`, match[0]);
            } else {
                result.menuItems.push({ kind: 'text', label, command: resolved });
            }
        } else if (kind === 'timeout') {
            result.timeout = { seconds: Number.parseInt(label, 10) | 0, command: makeCommand(command, argument) };
        } else if (kind === 'onExit') {
            result.onExit = makeCommand(command, argument);
        } else if (kind === 'input' || kind === 'zsend') {
            hooks.onUnsupported?.(`지시문 ${kind}`, match[0]);
        } else if (kind !== 'key') {
            // `$Button`·`$ButtonInstant` 는 단추로 만든다(20개 한도). `$Key` 는 해석만 하고 컨트롤을 만들지 않는다.
            if (result.buttons.length < MAX_DIALOG_BUTTONS) result.buttons.push({ label, command: makeCommand(command, argument), instant: kind === 'buttonInstant' });
            processedButtons += 1;
        }
        // 일치 시작부터 다음 LF·`$`·끝 앞까지 지운다(`$` 로 시작하면 그 글자는 건너뛰고 센다).
        const start = match.index;
        let end = start + (text[start] === '$' ? 1 : 0);
        while (end < text.length && text[end] !== '\n' && text[end] !== '$') end += 1;
        if (end < text.length && text[end] === '\n') end += 1;
        text = text.slice(0, start) + text.slice(end);
        // `%` 인라인 목록은 지운 자리에 삽입 문자열을 넣는다(원본은 지운 뒤 길이만큼만 앞에 복사하는 특이한 방식이라 일반 삽입과 다르다 — 웹은 일반 삽입).
        if (replacement !== null) text = text.slice(0, start) + replacement + text.slice(start);
    }
    result.text = text;
    result.handledAny = processedButtons > 0 || result.menuStarted;
    return result;
}

// 대화상자 본문 하나를 준비한다(원본 공장 004cf270 의 본문 처리 부분).
//  1) 설정 치환(substitute) → 2) 첫 변환 → 3) 영숫자가 없으면 빈 본문(content 없음, empty=true)
//  4) 지시문 추출(버튼·메뉴가 없으면 OkButton 지시문을 한 번 더 추출) → 5) 모든 LF 를 `<br>` 로 바꿔 두 번째 변환 → 6) 끝 LF 한 개 더 추가.
// 반환: { empty, styleText(두 번째 변환 결과 + LF), directives(추출 결과), background(마지막 배경 이름) }.
// options: { substitute(text) → 설정 치환 함수, okButton(OkButton 설정값), hooks(변환·지시문 훅), globals(공유 전역) }.
// 사용: prepareDialogContent(tellSectionBody, { substitute: data.substitute.bind(data), okButton: data.get('OkButton') }). (2026-10-28 WB03-D)
export function prepareDialogContent(body, options = {}) {
    const substitute = options.substitute ?? ((text) => text);
    const globals = options.globals ?? createParserGlobals();
    const hooks = options.hooks ?? {};
    const state = createParserState();
    const substituted = substitute(body.length > OUTPUT_CAPACITY ? body.slice(0, OUTPUT_CAPACITY) : body);
    const first = parseHtml(substituted, state, globals, hooks);
    if (!hasAlphanumeric(first)) return { empty: true, styleText: '', directives: null, background: globals.background };
    let directives = extractDirectives(first, hooks);
    if (!directives.handledAny) {
        // 버튼·메뉴가 하나도 없으면 기본 확인 버튼 지시문을 추출해 단추를 만든다.
        const ok = extractDirectives(options.okButton ?? DEFAULT_OK_BUTTON, hooks);
        directives = { ...directives, buttons: [...directives.buttons, ...ok.buttons], menuItems: [...directives.menuItems, ...ok.menuItems], timeout: directives.timeout ?? ok.timeout, onExit: directives.onExit ?? ok.onExit };
    }
    // 남은 본문의 LF 를 모두 `<br>` 로 바꿔 같은 파서 상태(조건 숨김 깊이 포함)와 전역으로 두 번째 변환한다.
    const second = parseHtml(directives.text.replaceAll('\n', '<br>'), state, globals, hooks);
    return { empty: false, styleText: `${second}\n`, directives, background: globals.background };
}
