// 원본 Editgump(VT 00508e10)의 실제 문자 편집 노드와 Console 파생(VT 00506a6c)을 공통 Gump/문자 버퍼에 연결한다(cpppj client/TextEntryGump.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 실제 0x28 편집 버퍼(TextEntryBuffer)와 두 Slider 를 공유하며 원본의 현재 필드 재조회/사건 순서를 유지한다. 노드/버퍼/Slider/글꼴·픽셀 출력은 모두 호스트 훅이고 빈 성공 노드를 만들지 않는다.
// 문자열 인자(span<const char>)는 NUL 로 끝나는 Uint8Array(또는 바이트 문자열)이며 훅이 받는 글 바이트도 같다. 기준선 뒤 K1 한국어 확장 토큰 변경은 이 파일에 없다(Buffer 쪽 기록 참고). 전용 PE fixture 는 없다.
// 사용: const gump = new TextEntryGump(state, hooks); gump.construct(token, 200, 60, 'hello', fontToken, backgroundToken, color); gump.handle(token, 0x41, 0, 0)(문자 'A' 입력); gump.draw(token).
// (2026-11-14 WB04-E439: RawMenuControls 선행 문자 편집 Gump 이식)
import { GumpBackgroundNode } from './gump-background-create.js';
import { cameraFtolLow } from '../render/camera-view.js';

// 원본 10.78 의 가상 표와 기본 Gump 비트다(표시/void/dead).
const ENTRY_VTABLE = 0x508e10; const CONSOLE_VTABLE = 0x506a6c; const VISIBLE = 1; const VOID = 8; const DEAD = 0x10000;
// 원본 문자 입력 관문이다: 사건 코드 마스크, 명령/뗌/Shift/Ctrl 비트.
const CODE_MASK = 0x03ffffff; const COMMAND = 0x02000000; const RELEASE = 0x80000000; const SHIFT = 0x20000000; const CONTROL = 0x10000000;
// 00503350 의 caret 점멸 주기(초)다.
const BLINK_PERIOD = 0.5;
// 원본 005052fc 의 줄/열 크기 측정 문자열 "M\0" 이다.
const MEASURE_LETTER = Uint8Array.of(0x4d, 0);

// 원본 signed DWORD 산술의 하위 비트를 유지한다(감김).
const add = (first, second) => (first + second) | 0;
const sub = (first, second) => (first - second) | 0;
// 입력(문자열/바이트 배열/null)을 첫 NUL 앞 바이트 + NUL 하나의 안정된 배열로 만든다(선택 삽입 API 가 요구하는 실제 NUL 포함 입력).
function terminated(text) {
    if (text === null || text === undefined) return Uint8Array.of(0);
    const source = typeof text === 'string' ? Uint8Array.from(text, (character) => character.charCodeAt(0) & 0xff) : text;
    const end = source.indexOf(0);
    const body = end < 0 ? source : source.subarray(0, end);
    const result = new Uint8Array(body.length + 1); result.set(body); return result;
}
// 첫 NUL 까지(NUL 포함)의 범위만 읽는다. NUL 이 없는 입력은 호스트 오류로 거부한다(Error).
function cString(bytes) {
    const end = bytes.indexOf(0);
    if (end < 0) throw new Error('TextEntry NUL 문자열 범위 없음');
    return bytes.subarray(0, end + 1);
}
// 바이트 배열을 바이트 문자열로 바꾼다.
function toBinary(bytes) { return Array.from(bytes, (byte) => String.fromCharCode(byte)).join(''); }
// Console 단어 찾기가 쓰는 원본 기본 C locale 의 ASCII 공백이다.
const isSpace = (value) => value === 0x20 || (value >= 9 && value <= 13);
// 훅 하나가 있어야 하는 위치에서만 요구한다(TypeError, 원문 invalid_argument).
function need(hook, message) { if (typeof hook !== 'function') throw new TypeError(message); return hook; }

// VT 00508e10 의 +b8..108 필드다. 실제 배경/기본 Gump 주소를 그대로 상속한다.
export class TextEntryGumpNode extends GumpBackgroundNode {
    // +b8/+bc/+c0/+c4: 세로/가로 Slider 토큰, 버퍼 소유 여부, 생성자가 쓰지 않는 예약 DWORD 다.
    verticalSlider = 0; horizontalSlider = 0; ownsBuffer = 0; reservedC4 = 0;
    // +c8 다음 caret 점멸 시각(초), +d0 caret 위상, +d4 입력/점멸 금지 관문이다.
    blinkAt = 0; blinkPhase = 0; readOnly = 0;
    // +d8/+dc/+e0/+e4: shift 선택 기준/끝과 현재 표시 원점(줄/열)이다.
    shiftAnchor = 0; shiftEnd = 0; scrollLine = 0; scrollColumn = 0;
    // +e8..fc: 세로/가로 Slider 표시, 입력 전달, 여러 줄, 변경 통지, 수신 대상 강조의 원시 DWORD 다.
    showVertical = 0; showHorizontal = 0; forwardInput = 0; multiline = 0; changeNotice = 0; recipientHighlight = 0;
    // +100: 양수이면 현재 length+1 과 엄격 비교하는 문자 입력 제한이다.
    maxLength = 0;
    // +104 별도 소유 NUL 문자열(제외문자) 토큰, +108 실제 0x28 버퍼 토큰이다.
    excludedChars = 0; buffer = 0;
}

// VT 00506a6c Console 은 같은 TextEntry 위에 autocomplete/제출 callback 을 더한다. 앞뒤 예약 DWORD 는 생성자가 쓰지 않는다.
export class ConsoleGumpNode extends TextEntryGumpNode {
    // +10c 예약, +110 autocomplete 토큰, +114 제출 토큰, +118 단어 재설정, +11c 단어 시작, +120 단어 길이, +124 예약이다.
    reserved10C = 0; autocomplete = 0; submit = 0; resetWord = 0; wordStart = 0; wordLength = 0; reserved124 = 0;
}

// 런타임 종류 BYTE·팔레트/사건 전역이다(실제 Client 가 갱신한다).
export class TextEntryGumpState {
    // 0054042a/425 의 signed 종류 BYTE 다(TextEntry 15, Console 10).
    kind = 15; consoleKind = 10;
    // 005acd24/005a4058/0059afd8 의 현재 파일 팔레트 색(갈색/검정/노랑)이다.
    brown = 0; black = 0; yellow = 0;
    // 005427d8/5429f0/5427d0/5427b0 의 실제 사건(Enter/변경/슬라이더 변경/슬라이더 단계)이다.
    enterEvent = 0x02000038; changeEvent = 0x020000bf; sliderChange = 0x02000036; sliderStep = 0x0200002e;
}

// 같은 문자 버퍼와 Gump 저장소에 원본 필수 생성/입력/기하/Draw 를 연결한다. 조회 훅(node/base/buffer)만 생성 시 검증하고 나머지 장치는 실제 도달할 때 요구한다.
// hooks 는 cpppj TextEntryGumpHooks 와 같은 이름이다: node/console/base/buffer/slider(조회), allocate/releaseBuffer/constructBackground/constructSlider/baseAttach/attach/push/idle/redraw/bubble,
// sliderRange/sliderPage/sliderPosition, now, measure(token, 글, 길이) → [너비, 높이], position(token) → {x, y}, drawBackground/drawText(token, 글, {x, y}, 문맥|null)/fill({left,top,right,bottom}, 색),
// recipient/allocateBytes/releaseBytes/bytes/complete/submit/assertion/destructBase.
export class TextEntryGump {
    // 현재 팔레트/사건/종류 전역과 훅 묶음이다.
    #state; #hooks;
    constructor(state, hooks) {
        if (typeof hooks?.node !== 'function' || typeof hooks.base !== 'function' || typeof hooks.buffer !== 'function') throw new TypeError('TextEntry 같은 노드/편집 버퍼 필요');
        this.#state = state; this.#hooks = hooks;
    }

    // 콜백 중에도 기본 Gump 와 파생 저장소는 같은 객체(주소)에서 살아 있어야 한다.
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node || node !== this.#hooks.base(token)) throw new Error('TextEntry 공유 노드 없음');
        return node;
    }
    // Console 의 기본 TextEntry 주소도 같아야 한다.
    #console(token) {
        const node = token && this.#hooks.console ? this.#hooks.console(token) : null;
        if (!node || node !== this.#node(token)) throw new Error('Console 공유 파생 노드 없음');
        return node;
    }
    // 원본 자식 포인터의 유효성은 실제 도달한 때만 검사한다.
    #base(token) {
        const node = token ? this.#hooks.base(token) : null;
        if (!node) throw new Error('TextEntry 실제 자식 없음');
        return node;
    }
    // 현재 buffer 토큰을 복사한 과거 State 로 대체하지 않고 매번 노드의 +108 을 다시 읽는다.
    #buffer(token) {
        const current = this.#node(token).buffer;
        const buffer = current ? this.#hooks.buffer(current) : null;
        if (!buffer) throw new Error('TextEntry 현재 편집 버퍼 없음');
        return buffer;
    }
    // 실제 ScrollGump 와 같은 기본 객체의 현재 position 을 읽는다.
    #slider(token) {
        const node = token && this.#hooks.slider ? this.#hooks.slider(token) : null;
        if (!node || node !== this.#base(token)) throw new Error('TextEntry 실제 Slider 없음');
        return node;
    }
    // 원문 재그리기는 현재 가상 +8(false)에만 요청한다.
    #redraw(token) { need(this.#hooks.redraw, 'TextEntry Redraw 필요')(token, false); }
    // 부모 통지는 첫 성공 처리기까지 가는 공통 Bubble 에 맡긴다.
    #bubble(token, event, first, second) { need(this.#hooks.bubble, 'TextEntry 부모 사건 필요')(token, event, first, second); }
    // 원문의 기본 문맥을 각 측정 시점에 실제 글꼴 계층에서 읽는다([너비, 높이]).
    #measure(token, text, length = -1) { return need(this.#hooks.measure, 'TextEntry 실제 글꼴 측정 필요')(token, text, length); }
    // 별도 제외문자 문자열은 0(null)과 빈 문자열을 구별한다: 0 이면 빈 범위, 아니면 NUL 포함 문자열 범위다.
    #excluded(token) {
        const address = this.#node(token).excludedChars;
        if (!address) return new Uint8Array(0);
        return cString(need(this.#hooks.bytes, 'TextEntry 제외문자 저장소 필요')(address));
    }

    // 00452430: 기본 배경/문자 버퍼/idle/두 실제 Slider 를 순서대로 생성한다. Console 의 기본 생성도 이 몸체다. initial 이 있으면 전체를 선택한다. 실제 버퍼 확보 실패(0)는 assertion(0x31) 뒤 계속한다.
    construct(token, width, height, initial, font, background, color) {
        const node = this.#node(token); const hooks = this.#hooks; const state = this.#state;
        if (['constructBackground', 'allocate', 'constructSlider', 'idle', 'attach', 'now'].some((name) => typeof hooks[name] !== 'function')) throw new TypeError('TextEntry 실제 생성 경계 필요');
        width |= 0; height |= 0;
        hooks.constructBackground(token, background);
        node.vtable = ENTRY_VTABLE; node.blinkAt = 0; node.shiftAnchor = 0; node.shiftEnd = 0;
        node.command = state.kind >>> 0;
        // 원문의 고정 new 결과이며 가상 생성 뒤 root 에 설치한다.
        const allocated = hooks.allocate(0x28) >>> 0;
        if (allocated) {
            const buffer = hooks.buffer(allocated);
            if (!buffer) throw new Error('TextEntry 새 버퍼 저장소 없음');
            buffer.construct(0);
        }
        node.buffer = allocated; node.ownsBuffer = 1;
        if (!allocated && typeof hooks.assertion === 'function') hooks.assertion(0x31);
        node.maxLength = 0; node.blinkPhase = 0; node.blinkAt = hooks.now() + BLINK_PERIOD;
        node.baseWidth = Math.fround(width); node.width = Math.fround(width * node.scale);
        node.textContext.set([0, font >>> 0, color >>> 0, 0, 0, 0, 1, 1]);
        node.baseHeight = Math.fround(height); node.height = Math.fround(height * node.scale);
        node.scrollLine = 0; node.scrollColumn = 0; node.flags = (node.flags | 0x400400) >>> 0;
        hooks.idle(token, true);
        const bytes = initial !== null && initial !== undefined ? terminated(initial) : new Uint8Array(0);
        this.#buffer(token).reset(bytes); this.#redraw(token);
        node.readOnly = 0; node.forwardInput = 0; node.changeNotice = 0; node.multiline = 1;
        let slider = hooks.allocate(0xd8) >>> 0;
        if (slider) hooks.constructSlider(slider, 10, 100, state.brown, state.black);
        node.verticalSlider = slider;
        slider = hooks.allocate(0xd8) >>> 0;
        if (slider) hooks.constructSlider(slider, 100, 10, state.brown, state.black);
        node.horizontalSlider = slider;
        this.#base(node.verticalSlider).flags = (this.#base(node.verticalSlider).flags & ~VISIBLE) >>> 0;
        this.#base(node.horizontalSlider).flags = (this.#base(node.horizontalSlider).flags & ~VISIBLE) >>> 0;
        hooks.attach(node.verticalSlider, Math.fround(Math.fround(width) - this.#base(node.verticalSlider).baseWidth), 0, token, 1);
        hooks.attach(node.horizontalSlider, 0, Math.fround(Math.fround(height) - this.#base(node.horizontalSlider).baseHeight), token, 1);
        this.setScroll(token, 0, 0);
        node.showVertical = 0; node.showHorizontal = 0; node.recipientHighlight = 0;
        if (initial !== null && initial !== undefined) {
            node.shiftAnchor = 0; node.shiftEnd = bytes.length - 1;
            this.#buffer(token).select(0, node.shiftEnd);
        }
        node.excludedChars = 0;
        return token;
    }

    // 0043f820: Console 확장은 기본 생성이 끝난 동일 노드에만 수행한다. 종류 BYTE 는 Console 종류(10), autocomplete/제출/단어 길이·시작 0, 단어 재설정 1, 수신 대상 강조 1 이다.
    constructConsole(token, width, height, initial, font, background, color) {
        this.#console(token);
        this.construct(token, width, height, initial, font, background, color);
        const node = this.#console(token);
        node.vtable = CONSOLE_VTABLE; node.command = this.#state.consoleKind >>> 0;
        node.autocomplete = 0; node.submit = 0; node.wordLength = 0; node.wordStart = 0; node.resetWord = 1; node.recipientHighlight = 1;
        return token;
    }

    // 004527b0: 공통 부착 뒤 두 자식을 Push 하고 크기/표시를 갱신한 다음 두 Slider 를 우하단 기준으로 재부착하고 현재 scroll 을 다시 적용한다.
    attach(token, x, y, parent, mode) {
        const node = this.#node(token); const hooks = this.#hooks;
        if (typeof hooks.baseAttach !== 'function' || typeof hooks.push !== 'function' || typeof hooks.attach !== 'function') throw new TypeError('TextEntry 실제 부착 경계 필요');
        hooks.baseAttach(token, x, y, parent, mode); hooks.push(node.verticalSlider); hooks.push(node.horizontalSlider);
        const vertical = this.#base(node.verticalSlider); vertical.baseHeight = node.baseHeight; vertical.height = Math.fround(node.baseHeight * vertical.scale);
        const horizontal = this.#base(node.horizontalSlider); horizontal.baseWidth = node.baseWidth; horizontal.width = Math.fround(node.baseWidth * horizontal.scale);
        const flip = (child, visible) => { const base = this.#base(child); base.flags = (visible ? base.flags | VISIBLE : base.flags & ~VISIBLE) >>> 0; };
        flip(node.verticalSlider, node.showVertical); flip(node.horizontalSlider, node.showHorizontal);
        hooks.attach(node.verticalSlider, Math.fround(node.baseWidth - this.#base(node.verticalSlider).baseWidth), 0, token, 1);
        hooks.attach(node.horizontalSlider, 0, Math.fround(node.baseHeight - this.#base(node.horizontalSlider).baseHeight), token, 1);
        this.setScroll(token, node.scrollLine, node.scrollColumn);
    }

    // 00451c90: 줄 clamp 후 열 clamp 는 새 line 이 아니라 쓰기 전의 현재 scrollLine 으로 조회하는 원문 순서를 보존한다. 열/줄 page 는 두 _ftol(baseHeight/M 높이, baseWidth/M 폭)이고
    // 가로 Slider 에는 현재 줄 길이로 범위(0, 길이-1)/page/position 을 주며(원문은 같은 줄 길이를 두 번 읽는다) 세로 Slider 에는 줄 수로 준 뒤 두 Slider 의 현재 position 을 되읽고 Redraw 한다.
    setScroll(token, line, column) {
        const node = this.#node(token); line |= 0; column |= 0; const buffer = () => this.#buffer(token);
        if (line < 0) line = 0; else if (buffer().lineCount() <= line) line = sub(buffer().lineCount(), 1);
        if (column < 0) column = 0; else if (buffer().lineLength(node.scrollLine) <= column) column = sub(buffer().lineLength(node.scrollLine), 1);
        node.scrollLine = line; node.scrollColumn = column;
        // 현재 실제 M 폭/높이다.
        const metric = this.#measure(token, MEASURE_LETTER);
        // 원본 두 _ftol 의 줄/열 page 다.
        const rows = cameraFtolLow(node.baseHeight / metric[1]); const columns = cameraFtolLow(node.baseWidth / metric[0]);
        const hooks = this.#hooks;
        if (typeof hooks.sliderRange !== 'function' || typeof hooks.sliderPage !== 'function' || typeof hooks.sliderPosition !== 'function') throw new TypeError('TextEntry 실제 Slider 값 처리 필요');
        buffer().lineLength(node.scrollLine); const count = buffer().lineLength(node.scrollLine);
        hooks.sliderRange(node.horizontalSlider, 0, sub(count, 1)); hooks.sliderPage(node.horizontalSlider, columns); hooks.sliderPosition(node.horizontalSlider, column);
        node.scrollColumn = this.#slider(node.horizontalSlider).position;
        const lines = buffer().lineCount();
        hooks.sliderRange(node.verticalSlider, 0, sub(lines, 1)); hooks.sliderPage(node.verticalSlider, rows); hooks.sliderPosition(node.verticalSlider, line);
        node.scrollLine = this.#slider(node.verticalSlider).position;
        this.#redraw(token);
    }

    // 004528b0: 원본 delta 는 각각 현재 signed DWORD 와 감김 덧셈한다.
    relativeScroll(token, line, column) {
        this.setScroll(token, add(this.#node(token).scrollLine, line | 0), add(this.#node(token).scrollColumn, column | 0));
    }

    // 00451e90: 원문은 현재 시각을 두 번 읽으며 Redraw 콜백 뒤 두 번째 시각으로 다음 점멸을 예약한다. dead 이거나 읽기 전용이면 아무것도 하지 않는다.
    idle(token) {
        const node = this.#node(token);
        if ((node.flags & DEAD) || node.readOnly) return;
        const now = need(this.#hooks.now, 'TextEntry 실제 점멸 시계 필요');
        if (node.blinkAt <= now()) { node.blinkPhase = (node.blinkPhase + 1) >>> 0; this.#redraw(token); node.blinkAt = now() + BLINK_PERIOD; }
    }

    // 00451e00: 초기화 뒤 Redraw 는 가상 처리기로만 요청한다.
    setText(token, text) { this.#buffer(token).reset(text); this.#redraw(token); }

    // 00451e20: 선택을 끝(length+1 을 clamp 한 length)으로 옮긴 뒤 삽입하고 요청한 경우에만 이전 선택을 복구한다.
    appendText(token, text, restoreSelection = false) {
        const buffer = () => this.#buffer(token);
        const begin = buffer().state().selectionBegin; const end = buffer().state().selectionEnd;
        const at = add(buffer().state().length, 1); buffer().select(at, at); buffer().insert(text);
        if (restoreSelection) buffer().select(begin, end);
        this.#redraw(token);
    }

    // 00451e80: 원본 출력 용량/거부 반환은 실제 문자 버퍼가 결정한다.
    copyText(token, output) { return this.#buffer(token).copyAll(output); }

    // 00451b80: 제외문자 교체는 이전 소유 주소를 먼저 반납한다(기본 소멸자의 누락 free 를 임의로 더하지 않는다). null 이면 비운다. 새 문자열의 LF(10)는 원문처럼 지운다(삭제한 자리의 다음 문자를 다시 검사).
    setExcluded(token, text) {
        const node = this.#node(token); const hooks = this.#hooks;
        if (node.excludedChars) { need(hooks.releaseBytes, 'TextEntry 제외문자 반납 필요')(node.excludedChars); node.excludedChars = 0; }
        if (text === null || text === undefined) return;
        if (typeof hooks.allocateBytes !== 'function' || typeof hooks.bytes !== 'function') throw new TypeError('TextEntry 제외문자 실제 소유 저장소 필요');
        const source = terminated(text); node.excludedChars = hooks.allocateBytes(source.length) >>> 0;
        if (!node.excludedChars && typeof hooks.assertion === 'function') hooks.assertion(0x85);
        const target = hooks.bytes(node.excludedChars);
        if (target.length < source.length) throw new Error('TextEntry 제외문자 할당/범위 실패');
        target.set(source);
        let index = 0;
        // 원문 00504b0c 는 LF 한 바이트만 포함하며 최초 문자열의 할당 폭으로 이동한다. 현재 NUL 까지 반복하고 삭제했으면 같은 위치를 다시 검사한다.
        while (target[index]) {
            if (target[index] === 10) target.copyWithin(index, index + 1, index + 1 + (source.length - index - 1));
            else index += 1;
        }
    }

    // 00451c60: 필드를 쓴 뒤 void 가 아니면 가상 Push 뒤 현재 위치로 재부착한다. 이미 void 이면 필드만 쓴다.
    setHorizontal(token, value) {
        const node = this.#node(token); node.showHorizontal = value >>> 0;
        if (!(node.flags & VOID)) {
            const hooks = this.#hooks;
            if (typeof hooks.push !== 'function' || typeof hooks.attach !== 'function') throw new TypeError('TextEntry 표시 재부착 필요');
            hooks.push(token); hooks.attach(token, node.x, node.y, 0, 0);
        }
    }

    // 00452730/0043f890: 소유한 버퍼만 반납한다(빌린 버퍼는 건드리지 않는다). 자식 Slider 는 공통 자식 우선 Destroy 가 반납한다. 마지막에 실제 기본 소멸자를 호출한다.
    destruct(token, console = false) {
        const node = this.#node(token); const hooks = this.#hooks;
        if (console) { this.#console(token); node.vtable = CONSOLE_VTABLE; }
        node.vtable = ENTRY_VTABLE;
        if (node.buffer && node.ownsBuffer) {
            const saved = node.buffer; this.#buffer(token).destroy();
            need(hooks.releaseBuffer, 'TextEntry 버퍼 객체 반납 필요')(saved); node.buffer = 0;
        }
        need(hooks.destructBase, 'TextEntry 실제 기본 소멸자 필요')(token);
    }

    // 004528e0: 선택/문자 삽입 뒤 통지·Redraw·원시 사건 전달 순서와 별도 Slider 사건 분기를 보존한다. 반환 1 은 처리됨, 0 은 인식하지 않는 문자/키다. 같은 Enter 만 전달 value 를 LF(10)로 바꾼다.
    handle(token, event, first, second) {
        const node = this.#node(token); event >>>= 0; first >>>= 0; second >>>= 0;
        let value = event; let result = 0; const code = event & CODE_MASK; const buffer = () => this.#buffer(token);
        if (!(event & COMMAND) && code !== 0x10000 && code !== 0x20000 && !node.readOnly && !(event & RELEASE)) {
            // 인식하지 않는 문자/키는 공통 Redraw 뒤 반환 0 이다.
            result = 1; let notify = true;
            // shift 선택 기준/끝을 현재 선택에서 시작하는 공통 준비다(선택이 비어 있을 때만 기준 갱신).
            const startShift = () => { const begin = buffer().state().selectionBegin; const end = buffer().state().selectionEnd; if (begin === end) { node.shiftAnchor = begin; node.shiftEnd = end; } };
            switch (code) {
            // 왼쪽: 선택이 있어도 shift 가 없으면 현재 begin-1 한 칸으로 옮긴다.
            case 0x250000: {
                const begin = buffer().state().selectionBegin;
                if (!(event & SHIFT)) buffer().select(sub(begin, 1), sub(begin, 1));
                else { startShift(); node.shiftEnd = sub(node.shiftEnd, 1); buffer().select(node.shiftAnchor, node.shiftEnd); }
                break;
            }
            // 오른쪽은 현재 end+1 을 기준으로 삼는다.
            case 0x270000: {
                const end = buffer().state().selectionEnd;
                if (!(event & SHIFT)) buffer().select(add(end, 1), add(end, 1));
                else { startShift(); node.shiftEnd = add(node.shiftEnd, 1); buffer().select(node.shiftAnchor, node.shiftEnd); }
                break;
            }
            // End 는 length+1 로 Select 한 뒤 원문처럼 +dc 기준의 열 delta 를 전달한다.
            case 0x230000: {
                if (!(event & SHIFT)) { const at = add(buffer().state().length, 1); buffer().select(at, at); this.relativeScroll(token, 0, node.shiftEnd); }
                else { startShift(); node.shiftEnd = add(buffer().state().length, 1); buffer().select(node.shiftAnchor, node.shiftEnd); this.relativeScroll(token, 0, node.shiftEnd); }
                break;
            }
            // Home 의 shift 선택과 열 0 요청은 서로 다른 상태를 사용한다.
            case 0x240000: {
                if (!(event & SHIFT)) buffer().select(0, 0);
                else { startShift(); node.shiftEnd = 0; buffer().select(node.shiftAnchor, node.shiftEnd); }
                this.relativeScroll(token, 0, sub(0, node.scrollColumn));
                break;
            }
            // Ctrl+Backspace 는 단어 삭제가 아니라 전체 Reset 이다.
            case 8: {
                if (event & CONTROL) buffer().reset(null);
                else { let begin = buffer().state().selectionBegin; const end = buffer().state().selectionEnd; if (begin === end) begin = sub(begin, 1); buffer().select(begin, end); buffer().insert(null); }
                break;
            }
            // multiline 일 때만 LF 를 삽입하나 Enter 부모 통지는 항상 보낸다.
            case 13: {
                if (node.multiline) { value = 10; buffer().insert(Uint8Array.of(10, 0)); }
                this.#bubble(token, this.#state.enterEvent, node.serial, 0);
                break;
            }
            // Tab/ESC/위/아래는 이 기본 입력기에서 처리하지 않는다.
            case 9: case 27: case 0x260000: case 0x280000: result = 0; notify = false; break;
            // Insert 는 Ctrl 복사 우선이며 shift 가 있으면 붙여넣는다.
            case 0x2d0000:
                if (event & CONTROL) buffer().copy(); else if (event & SHIFT) buffer().paste(this.#excluded(token));
                break;
            // Shift+Delete 는 Cut, 일반 Delete 는 현재 선택 또는 오른쪽 한 문자다.
            case 0x2e0000: {
                if (event & SHIFT) buffer().cut();
                else { const begin = buffer().state().selectionBegin; let end = buffer().state().selectionEnd; if (begin === end) end = add(end, 1); buffer().select(begin, end); buffer().insert(null); }
                break;
            }
            default: {
                if (code < 0x20 || code >= 0x100) { result = 0; notify = false; break; }
                if (node.maxLength < 1 || add(buffer().state().length, 1) < node.maxLength) {
                    // 문자 제한은 현재 선택 길이를 빼지 않고 전체 length+1 로 판정한다.
                    const excluded = this.#excluded(token);
                    if (excluded.length && excluded.includes(value & 0xff)) { result = 0; notify = false; break; }
                    buffer().insert(Uint8Array.of(value & 0xff, 0));
                }
                break;
            }
            }
            if (notify && node.changeNotice) this.#bubble(token, this.#state.changeEvent, 0, 0);
            this.#redraw(token);
            if (node.forwardInput) this.#bubble(token, value, first, second);
        }
        if (((value & CODE_MASK) >>> 0) === this.#state.sliderChange) {
            if (first === (this.#base(node.verticalSlider).serial >>> 0)) this.setScroll(token, this.#slider(node.verticalSlider).position, node.scrollColumn);
            else if (first === (this.#base(node.horizontalSlider).serial >>> 0)) this.setScroll(token, node.scrollLine, this.#slider(node.horizontalSlider).position);
            else if (typeof this.#hooks.assertion === 'function') this.#hooks.assertion(0x246);
        }
        if (((value & CODE_MASK) >>> 0) === this.#state.sliderStep) {
            // 원문은 serial 일치 확인 전에 M 을 측정한다.
            const metric = this.#measure(token, MEASURE_LETTER);
            if (first === (this.#base(node.horizontalSlider).serial >>> 0) && (second === 1 || second === 2 || second === 3 || second === 4)) {
                this.relativeScroll(token, 0, second === 2 || second === 4 ? sub(0, metric[0]) : metric[0]); result = 1;
            }
        }
        return result;
    }

    // 0043f8a0: Console 은 Tab 으로 새 단어는 실제 문자열(바이트 문자열)을, 반복 Tab 은 null 을 complete callback 에 보내는 원본 계약을 보존한다. Enter 는 제출 callback 에 현재 본문을 보낸다.
    // Shift 키 자체(0x100000)는 후보 순환 상태를 초기화하지 않고 기본 처리로 넘기며, 그 밖의 비 Tab 입력은 단어 재설정을 켠다. 마지막에는 항상 기본 handle 에 위임한다.
    handleConsole(token, event, first, second) {
        const node = this.#console(token); event >>>= 0; const code = event & CODE_MASK; const buffer = () => this.#buffer(token);
        if (!(event & COMMAND) && code !== 0x10000 && code !== 0x20000 && !(event & RELEASE)) {
            if (code === 9) {
                const begin = buffer().state().selectionBegin; const end = buffer().state().selectionEnd;
                if (begin === end && node.autocomplete) {
                    // null 은 이전 단어의 다음/이전 후보를 뜻한다.
                    let word = null;
                    if (node.resetWord) {
                        node.wordStart = begin >>> 0; const data = buffer().data();
                        // 공백/콜론 바로 뒤까지 단어 시작을 역방향으로 찾는다.
                        while (node.wordStart > 0) {
                            const at = node.wordStart - 1;
                            if (at >= data.length) throw new Error('Console 단어 선택 범위');
                            const letter = data[at];
                            if (isSpace(letter) || letter === 0x3a) break;
                            node.wordStart -= 1;
                        }
                        node.wordLength = ((begin >>> 0) - node.wordStart) >>> 0;
                        if (node.wordStart >= data.length) throw new Error('Console 단어 시작 범위');
                        const raw = cString(data.subarray(node.wordStart)); word = toBinary(raw.subarray(0, raw.length - 1));
                    }
                    const found = need(this.#hooks.complete, 'Console 실제 autocomplete callback 필요')(node.autocomplete, word, node.wordLength | 0, (event & SHIFT) !== 0);
                    if (found !== null && found !== undefined) {
                        buffer().select(node.wordStart | 0, (node.wordStart + node.wordLength) | 0);
                        const bytes = terminated(found); buffer().insert(bytes); node.wordLength = (bytes.length - 1) >>> 0; node.resetWord = 0;
                    }
                }
                return 1;
            }
            if (code === 13) {
                if (node.submit) need(this.#hooks.submit, 'Console 실제 제출 callback 필요')(node.submit, buffer().text());
            } else if (code === 0x100000) return this.handle(token, event, first, second);
            node.resetWord = 1;
        }
        return this.handle(token, event, first, second);
    }

    // 00451f00: 각 줄의 원시 선택 쌍과 동일 문자 바이트로 배경/일반·선택 글자/수신 대상/caret 를 출력한다. 배경을 그린 뒤 현재 M 높이로 표시 줄 수를 계산한다(단순 ceil 이 아니라 원본 정수 나머지 관문).
    draw(token) {
        const node = this.#node(token); const hooks = this.#hooks; const state = this.#state; const buffer = () => this.#buffer(token);
        if (['drawBackground', 'drawText', 'position', 'fill'].some((name) => typeof hooks[name] !== 'function')) throw new TypeError('TextEntry 실제 픽셀/글자 출력 필요');
        hooks.drawBackground(token);
        const metric = this.#measure(token, MEASURE_LETTER);
        const truncated = cameraFtolLow(node.baseHeight);
        if (!metric[1] || (truncated === -2147483648 && metric[1] === -1)) throw new Error('TextEntry 원본 줄높이 나눗셈 범위');
        const partial = truncated % metric[1] !== 0 ? 1 : 0; const limit = cameraFtolLow(node.baseHeight / metric[1] + partial);
        // 원본 한 줄 저장소와 native 두 DWORD 출력이다.
        const line = new Uint8Array(1024); const selection = [0, 0]; let y = 0;
        // 모든 출력은 같은 기본 Gump 상대 좌표를 사용한다.
        const write = (bytes, x, atY, context = null) => hooks.drawText(token, bytes, { x, y: atY }, context);
        // native 선택의 memcpy 와 명시 NUL 을 같은 바이트에서 수행한다.
        const piece = (at, count) => {
            if (at > line.length || count < 0 || count >= line.length || count > line.length - at) throw new Error('TextEntry 선택 그림 범위');
            const bytes = new Uint8Array(count + 1); bytes.set(line.subarray(at, at + count)); return bytes;
        };
        // 줄 수는 각 반복 끝의 현재 문자 버퍼에서 다시 읽는다.
        for (let index = node.scrollLine; index < buffer().lineCount(); index = add(index, 1)) {
            if (limit <= sub(index, node.scrollLine)) return;
            // 수평 잘라내기 뒤에도 원문의 선택/길이 쌍을 임의 보정하지 않는다.
            const length = buffer().copyLine(index, line, selection);
            if (length === 0xffffffff) { if (typeof hooks.assertion === 'function') hooks.assertion(0x2ae); throw new Error('TextEntry 원본 줄 복사 범위 거부'); }
            if (node.scrollColumn > 0 && node.scrollColumn < 1024) {
                // 원본 String::erase 는 요청 수가 길이를 넘으면 아무것도 바꾸지 않는다.
                const visible = cString(line).length - 1; const removed = node.scrollColumn;
                if (removed <= visible) line.copyWithin(0, removed, visible + 1);
            }
            if (length) { if (length > line.length) throw new Error('TextEntry 줄 끝 범위'); if (line[length - 1] === 10) line[length - 1] = 32; }
            else { line[0] = 32; line[1] = 0; }
            // 원문의 줄 간격은 각 실제 줄의 현재 측정 높이다.
            const dimensions = this.#measure(token, line);
            if (selection[0] === -1 || selection[1] <= selection[0]) {
                // 수신 대상 색은 선택 영역이 없을 때만 적용한다.
                const visible = cString(line); const colon = visible.indexOf(0x3a);
                if (!node.recipientHighlight || colon < 0) write(line, 0, y);
                else {
                    const at = colon; const saved = line[at + 1]; line[at + 1] = 0; line[at] = 0;
                    const valid = need(hooks.recipient, 'Console 실제 수신 대상 검사 필요')(toBinary(line.subarray(0, at))); line[at] = 0x3a;
                    if (valid) { const context = Uint32Array.from(node.textContext); context[2] = state.yellow; write(line, 0, y, context); }
                    else write(line, 0, y);
                    const prefix = this.#measure(token, line); line[at + 1] = saved; write(line.subarray(at + 1), prefix[0], y);
                }
            } else {
                if (selection[0] < 0 || selection[1] < 0) throw new Error('TextEntry 원본 선택 인덱스 범위');
                // 선택 앞 문자열의 실제 측정 폭이다.
                let x = 0;
                if (selection[0] > 0) { const before = piece(0, selection[0]); const size = this.#measure(token, before); write(before, 0, y); x = size[0]; }
                // 선택 글자는 평면 배경색으로 반전한다.
                const context = Uint32Array.from(node.textContext); context[2] = node.backgroundFlags & 0xff;
                const selected = piece(selection[0], sub(selection[1], selection[0])); const size = this.#measure(token, selected);
                // 선택 사각형은 현재 화면 위치를 다시 조회한다.
                const position = hooks.position(token); const left = add(position.x, x); const top = add(position.y, y);
                hooks.fill({ left, top, right: add(left, size[0]), bottom: add(top, size[1]) }, node.textContext[2]);
                write(selected, x, y, context);
                if (selection[1] < (length | 0)) {
                    const at = selection[1];
                    if (at >= line.length) throw new Error('TextEntry 선택 뒤 그림 범위');
                    write(line.subarray(at), add(x, size[0]), y);
                }
            }
            if (selection[0] === selection[1] && selection[0] !== -1 && (node.blinkPhase & 1) && !node.readOnly) {
                // 명시 길이는 중간 NUL 뒤 원문 바이트도 측정한다.
                const position = hooks.position(token); const caret = this.#measure(token, line, selection[0]);
                const left = add(position.x, caret[0]); const top = add(position.y, y);
                hooks.fill({ left, top, right: add(left, 1), bottom: add(top, caret[1]) }, node.textContext[2]);
            }
            y = add(y, dimensions[1]);
        }
    }
}

// 실제 Console/TextEntry 가 선택하는 벽시계·clipboard·Chat 콜백 경계 문맥이다(cpppj client/RawTextEntryContext.h, 헤더만 있는 구조체). 선택한 기능만 해당 장치를 요구한다.
// now(): 원본 460d70 벽시계 초(게임 시각과 구별), clipboardRead(Uint8Array)/clipboardWrite(Uint8Array): 원문 최대 256바이트 읽기와 명시 길이 복사,
// recipient(이름): 원문 42dc70 수신 대상 검사, complete(autocomplete, 단어|null, 길이, 역방향) → 문자열|null, submit(token, 본문): 실제 Console 제출 callback 이다.
// 사용: const context = createRawTextEntryContext({ now: () => performance.now() / 1000 }); bindTextClipboard(context, clipboard). (2026-11-14 WB04-E439)
export function createRawTextEntryContext(fields = {}) {
    return { now: null, clipboardRead: null, clipboardWrite: null, recipient: null, complete: null, submit: null, ...fields };
}
