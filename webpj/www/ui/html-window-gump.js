// 원문 HTML 부모의8192BYTE/20행·생성·본문·버튼·입력·수명을 같은 실제 Gump 저장소에 연결한다(cpppj client/HtmlWindowGump.cpp/.h, 기준선1f00a25a).
// 하위 장치는 원문 위치에서만 요구한다. 전체 가상 Fit/메타/공장은 별도 단위다. 사용: new HtmlWindowGump(sharedState,{node,base,...}). (2026-11-14 WB04-E456)
import { GumpDecorationNode } from './gump-decoration.js';

// 보호10.78 HTML 부모 가상표 식별값과 고정 제어행 수다(호스트 함수 포인터가 아니다).
const VTABLE = 0x515e7c; const CONTROL_COUNT = 20;
// 선택된 실제 장치가 없으면 앞선 부분 효과를 보존한 채 중단한다.
function need(hook) { if (typeof hook !== 'function') throw new Error('HTML 부모 실제 장치 미연결'); return hook; }
// 물리20행 밖을 임의 빈 행으로 채우지 않는다. signed 반복 한도와 별개로 실제 접근에서 확인한다.
function slot(index) { if ((index >>> 0) >= CONTROL_COUNT) throw new RangeError('HTML 부모 실제20행 밖 접근'); return index >>> 0; }

// 기본/장식 배경과 같은 주소의0x21e8 HTML 파생 저장소다. JS 새 할당은0이지만 생성자는 미작성 꼬리를 다시 지우지 않는다.
export class HtmlWindowGumpNode extends GumpDecorationNode {
    // +c4의 원시HTML/링크와+20c4의20개 실제세DWORD 행이다.
    html = new Uint8Array(8192); controls = Array.from({ length: CONTROL_COUNT }, () => ({ child: 0, command: 0, parameter: 0 }));
    // +21b4..21cc의 현재 제어수/본문/갱신 깊이/요청 크기/메뉴 열수/보조창serial이다.
    controlCount = 0; body = 0; updateDepth = 0; requestedWidth = 0; requestedHeight = 0; menuColumns = 0; auxiliarySerial = 0;
    // +21d0..21e4의 벽시각 기한과 원시 명령/인자다. 생성자는 두 parameter를 덮지 않는다.
    timeoutSeconds = 0; timeoutCommand = 0; timeoutParameter = 0; onExitCommand = 0; onExitParameter = 0;
}

// Client 수명의 같은 HTML 인스턴스 수/serial/닫힘 시각과 원문 PE 사건 초기값이다.
export class HtmlWindowGumpState {
    // 5cab2c/38/34의 현재DWORD와5cad30/34의 벽시각이다.
    instances = 0; createdSerial = 0; transientSerial = 0; lastCloseTime = 0;
    // 540436의 signed 종류BYTE와5427d8/a8/8c·5429ec의 실제 사건이다.
    kind = 27; enterEvent = 0x02000038; clickEvent = 0x0200002c; tagEvent = 0x02000025; closedEvent = 0x020000be;
}

// 같은 실제 노드/기본 주소를 검증하고 수명과 입력 효과를 공통 처리기로 전달한다.
export class HtmlWindowGump {
    // 호출보다 오래 유지되는 같은 Client 상태와 값 캡처한 장치 목록이다.
    #state; #hooks;
    // 같은 node/base만 선행 요구하며 미사용 하위는 선택 경계에서 확인한다.
    constructor(state, hooks) {
        if (typeof hooks?.node !== 'function' || typeof hooks?.base !== 'function') throw new TypeError('HTML 부모 같은 노드 필요');
        this.#state = state; this.#hooks = { ...hooks };
    }
    // 같은 토큰의 기본/파생 주소가 동일해야 하며 표시용 사본을 허용하지 않는다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node || node !== this.#hooks.base(token)) throw new Error('HTML 부모 같은 실제 주소 없음'); return node; }
    // 현재 실제 본문/버튼 기본 주소만 읽는다.
    #base(token) { const node = token ? this.#hooks.base(token) : null; if (!node) throw new Error('HTML 실제 자식 없음'); return node; }
    // 실제 장식 생성 뒤 선택 필드와20행만 초기화하고 공유 인스턴스 DWORD를 증가시킨다.
    construct(token, width, height) {
        this.#node(token); need(this.#hooks.constructDecoration)(token); const node = this.#node(token); node.vtable = VTABLE;
        // 기존20행 객체를 유지하면서 실제 세 필드만0으로 쓴다.
        for (const entry of node.controls) { entry.child = 0; entry.command = 0; entry.parameter = 0; }
        node.timeoutSeconds = 0; node.command = ((this.#state.kind << 24) >> 24) >>> 0; node.controlCount = 0; node.body = 0; node.auxiliarySerial = 0; node.html[0] = 0;
        node.updateDepth = 0; node.menuColumns = 0; node.timeoutCommand = 0; node.onExitCommand = 0; node.requestedWidth = width >>> 0; node.requestedHeight = height >>> 0;
        this.#state.instances = (this.#state.instances + 1) >>> 0; return token;
    }
    // count감소→OnExit큐→현재 보조창Kill→벽시각→기본 소멸 순서이며 OnExit를 미리 지우지 않는다.
    destruct(token) {
        this.#node(token).vtable = VTABLE; this.#state.instances = (this.#state.instances - 1) >>> 0;
        if (this.#node(token).onExitCommand) { const command = this.#node(token).onExitCommand; const parameter = this.#node(token).onExitParameter; need(this.#hooks.queue)(command, parameter, parameter); }
        const auxiliary = need(this.#hooks.reference)(this.#node(token).auxiliarySerial); if (auxiliary) need(this.#hooks.kill)(auxiliary);
        this.#state.lastCloseTime = need(this.#hooks.wallSeconds)(); need(this.#hooks.destructBase)(token);
    }
    // Kill뒤 현재count를 다시 읽고 각 선택 행의 세DWORD와 마지막count만 지운다.
    clearButtons(token) {
        // signed count가 음수면 원문처럼 반복하지 않는다.
        for (let index = 0; (index | 0) < (this.#node(token).controlCount | 0); index += 1) {
            const entry = this.#node(token).controls[slot(index)]; if (entry.child) { need(this.#hooks.kill)(entry.child); entry.child = 0; } entry.command = 0; entry.parameter = 0;
        }
        this.#node(token).controlCount = 0;
    }
    // 실제0x170돌버튼을 만들고 현재행 저장→가상부착→명령/인자→Instant bit→count 증가 순서다.
    addButton(token, label, command, parameter, instant) {
        if ((this.#node(token).controlCount | 0) >= CONTROL_COUNT) return; slot(this.#node(token).controlCount);
        const child = need(this.#hooks.allocate)(0x170); if (child) need(this.#hooks.constructButton)(child, label);
        const entry = this.#node(token).controls[slot(this.#node(token).controlCount)]; entry.child = child;
        need(this.#hooks.attach)(child, 0, 0, token, 0); entry.parameter = parameter >>> 0; entry.command = command >>> 0;
        if (instant) { const button = need(this.#hooks.button)(child); if (!button) throw new Error('HTML 실제 돌 버튼 없음'); button.buttonFlags = (button.buttonFlags | 4) >>> 0; }
        this.#node(token).controlCount = (this.#node(token).controlCount + 1) >>> 0;
    }
    // 첫NUL까지의 본문에 필요한LF만 보충하고 실제 기존본문/보조창을Kill한다. 비void 부모 재부착은 전체Fit을 선택한다.
    setBody(token, source) {
        let value = source.split('\0', 1)[0];
        if (!value.length || !value.endsWith('\n')) { if (value.length > 1006) throw new RangeError('HTML 원문 지역 LF 버퍼 범위 초과'); value += '\n'; }
        if (this.#node(token).body) { need(this.#hooks.kill)(this.#node(token).body); this.#node(token).body = 0; }
        const child = need(this.#hooks.allocate)(0xd8); if (child) need(this.#hooks.constructBody)(child); this.#node(token).body = child; this.#base(child).secondaryDepth = 0xffffffff;
        need(this.#hooks.scrollable)(this.#node(token).body, 0); need(this.#hooks.parseBody)(this.#node(token).body, value); need(this.#hooks.attach)(this.#node(token).body, 0, 0, token, 0);
        const auxiliary = need(this.#hooks.reference)(this.#node(token).auxiliarySerial); if (auxiliary) need(this.#hooks.kill)(auxiliary);
        if (!(this.#node(token).flags & 8)) { need(this.#hooks.push)(token); need(this.#hooks.attach)(token, this.#node(token).x, this.#node(token).y, 0, 0); }
    }
    // 20행의 현재 serial/첫행Enter·태그·메뉴닫힘을 분배한다. 기본위임에는 원래 전체DWORD를 보존한다.
    handle(token, event, first, second) {
        event >>>= 0; first >>>= 0; second >>>= 0;
        const code = event & 0x03ffffff; const state = this.#state;
        // 명령/인자를 큐 전에 캡처하고 큐 뒤 부모를Kill한다.
        const fire = (entry) => { const command = entry.command; const parameter = entry.parameter; need(this.#hooks.queue)(command, parameter, parameter); need(this.#hooks.kill)(token); return 1; };
        if (code === state.enterEvent && this.#node(token).controls[0].child) return fire(this.#node(token).controls[0]);
        if (code === state.clickEvent) {
            // 원문은 controlCount와 무관하게 실제20행을 전부 검사한다.
            for (const entry of this.#node(token).controls) if (entry.child && first === this.#base(entry.child).serial) return fire(entry);
        } else if (code === state.tagEvent) { if (second) { need(this.#hooks.kill)(token); need(this.#hooks.link)(second); } return 1; }
        else if (code === state.closedEvent) { need(this.#hooks.kill)(token); return 1; }
        return need(this.#hooks.baseHandle)(token, event, first, second);
    }
    // 기한0에서는 시계를 읽지 않으며 벽시각 callback뒤 현재기한/명령을 다시 읽는다. NaN의 ordered 비교는 거짓이다.
    idle(token) {
        if (this.#node(token).timeoutSeconds === 0) return; const now = need(this.#hooks.wallSeconds)();
        if (now >= this.#node(token).timeoutSeconds) { const command = this.#node(token).timeoutCommand; const parameter = this.#node(token).timeoutParameter; need(this.#hooks.queue)(command, parameter, parameter); need(this.#hooks.kill)(token); }
    }
}
