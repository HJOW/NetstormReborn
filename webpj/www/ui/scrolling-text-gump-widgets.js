// 다중 줄 TextGump 의 정상 생성·버퍼 소유·Slider 재부착·태그/스크롤/커서 입력과 프레임 드래그를 결합한다.
// (cpppj client/ScrollingTextGumpWidgets.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 의 vtable·NaN 폭 비교는 옮기지 않는다)
// 기준선 뒤 변경(ScrollingTextGumpWidgetState 의 sliderType/sliderFrame 주석)은 설명 변경뿐이라 3.6절 ①로 판정했다: 이름과 달리 두 값은 TYPE/프레임이 아니라 현재 팔레트 색 번호다(갈색/검정).
// 원본의 소유 여부·현재 자식/요청/태그 재조회와 overlay·캡처·프레임 입력 순서를 유지한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다).
// (2026-11-14 WB04-E427: RawMenuControls 선행 ScrollingTextGumpWidgets·StyleTextBufferStore 이식)
import { StyleTextBuffer, StyleTextBufferState } from './style-text-buffer.js';
import { ScrollingTextGumpNode } from './text-gump.js';
import { InputCode } from '../core/input-queue.js';

// 10.78 다중 줄 가상 표와 생성 요청 크기·기본 버퍼 줄 수/Slider 크기다(주소는 실행 포인터가 아닌 근거 식별자).
const VTABLE = 0x51454c; const BUFFER_BYTES = 0x38; const SLIDER_BYTES = 0xd8; const LINES = 128; const SLIDER_WIDTH = 9; const SLIDER_HEIGHT = 100; const STEP = 5;
// 원본 생성/입력 플래그·textFlags 비트(1 dirty·2 slider·4 scroll·8 왼쪽 태그·16 오른쪽 태그)·세 가지 커서 번호다.
const CREATE_FLAGS = 0x400a00; const VISIBLE = 1; const NOTIFY = 0x100; const KEYBOARD_EXCLUDE = 0x200; const VOID = 8; const DIRTY = 1; const USE_SLIDER = 2; const SCROLL = 4; const LEFT_TAG = 8; const RIGHT_TAG = 16; const EVENT_MASK = 0x03ffffff;
const NORMAL_CURSOR = 1; const SCROLL_CURSOR = 15; const TAG_CURSOR = 16;
// DWORD 뺄셈을 signed 로 해석한다.
const subtract = (first, second) => (first - second) | 0;
// CRT _ftol 의 signed 64비트 절삭 결과의 하위 DWORD 다. 무효 변환의 하위 DWORD 는 0 이다.
function ftol(value) { if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 산술 결과는 새 float 저장이다. 원문 직접 복사의 신호 NaN 비트를 재사용하지 않는다.
function setDimension(node, field, value) { node[field] = Math.fround(value); if (node.floatBits) delete node.floatBits[field]; }
// 선택된 장치가 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 거부한다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }

// 버퍼는 Gump 등록 표에 넣지 않는 별도 소유 객체다. 동일 조각/글꼴/그림 처리기와 안정된 상태를 유지한다.
// release 는 원본 버퍼 free 에 해당하는 호스트 토큰 반납이며 필수다. 상태/문맥/반납 경계는 저장소보다 오래 살아야 한다.
export class StyleTextBufferStore {
    // 같은 조각 표·StyleText·버퍼 훅·반납 경계와 토큰 → 레코드 표다(레코드 주소는 이동하지 않는다).
    #fragments; #style; #hooks; #release; #records = new Map();
    constructor(fragments, style, hooks, release) {
        if (typeof release !== 'function') throw new TypeError('StyleTextBuffer 토큰 반납 경계 필요');
        this.#fragments = fragments; this.#style = style; this.#hooks = hooks; this.#release = release;
    }
    // 원본 버퍼 생성자를 실제 새 상태에 호출한다. token 은 외부 new(0x38)가 성공한 별도 식별자이며 Gump 등록 수/serial 을 늘리지 않는다.
    construct(token, maxLines) {
        token >>>= 0;
        if (!token || this.#records.has(token)) throw new TypeError('StyleTextBuffer 새 토큰 필요');
        const state = new StyleTextBufferState(); const handler = new StyleTextBuffer(state, this.#fragments, this.#style, maxLines, this.#hooks);
        this.#records.set(token, { state, handler }); return token;
    }
    // Client 전역 로그 자체에 생성한다. 외부 상태는 destroy 까지 살아 있어야 하며 주소를 복사하지 않는다. 같은 상태의 이중 생성은 거부한다.
    constructBorrowed(token, state, maxLines) {
        token >>>= 0;
        if (!token || this.#records.has(token)) throw new TypeError('StyleTextBuffer 새 외부 토큰 필요');
        // 같은 물리 상태의 이중 생성을 거부한다(동일 상태를 두 처리기가 쓰는 것을 막는다).
        for (const record of this.#records.values()) if (record.state === state) throw new TypeError('StyleTextBuffer 외부 상태 중복 생성');
        const handler = new StyleTextBuffer(state, this.#fragments, this.#style, maxLines, this.#hooks);
        this.#records.set(token, { state, handler }); return token;
    }
    // 안정된 현재 레코드를 실제 도달 시 읽는다. 없는 토큰은 거부한다.
    #entry(token) { const record = token ? this.#records.get(token >>> 0) : null; if (!record) throw new Error('StyleTextBuffer 등록 토큰 없음'); return record; }
    // 기존 TextGump 의 표시/입력에 같은 상태와 실제 버퍼 처리기를 공급한다(표시 복사본을 만들지 않는다). 없는 토큰은 거부한다.
    binding(token) { const entry = this.#entry(token); return { state: entry.state, handler: entry.handler }; }
    // 완료/표시 조각 소멸 → 호스트 free → 기록 반납 순서다. 원본의 미완료 끝 칸 누락도 유지한다.
    destroy(token) { this.#entry(token).handler.destroy(); this.#release(token >>> 0); this.#records.delete(token >>> 0); }
}

// 현재 +20 종류 BYTE 와 Slider 생성 인자·실제 문자열 사건 ID 전역이다. +28 위치 정렬과 구별한다. 사건 ID 는 원본 초기화 결과를 공급해야 한다(PE 의 미초기화 0 으로 대체 금지).
export class ScrollingTextGumpWidgetState {
    // +20 의 종류 BYTE(초기 46)다.
    alignment = 46;
    // Slider 생성자의 넷째/다섯째 인자다. 이름과 달리 TYPE/프레임이 아니라 현재 팔레트 색 번호(갈색 005acd24/검정 005a4058)다.
    sliderType = 0; sliderFrame = 0;
    // Slider 변경/스크롤 이동/갱신/커서 선택/왼쪽·오른쪽 태그 사건 ID 다.
    changeEvent = 0; stepEvent = 0; refreshEvent = 0; cursorEvent = 0; leftTagEvent = 0; rightTagEvent = 0;
}

// 기존 다중 줄 표시/스크롤 처리기에 실제 생성/수명/입력과 추가 설정을 연결한다. 자동 Client 조립은 후속이다.
// hooks: { node, slider, base, allocate(bytes), push(token), attach(token,x,y,parent,mode), redraw(token,erase), cursor(number) } — 노드는 모두 같은 안정된 저장소를 돌려줘야 한다.
// 사용: new ScrollingTextGumpWidgets(create, state, factory, tree, lifetime, events, input, fragments, buffers, scrollingTextGump, scrollGumpWidgets, hooks).construct(token, 120, 80, font, color).
export class ScrollingTextGumpWidgets {
    // 같은 생성/전역/트리/수명/통지/캡처/조각/버퍼 저장소·표시·Slider 생성 처리기와 노드 조회·가상 경계다.
    #create; #state; #factory; #tree; #lifetime; #events; #input; #fragments; #buffers; #text; #sliderWidgets; #hooks;
    // 공유 노드 조회만 필수다. 가상/커서/할당 경계는 실제 원본 분기에 도달할 때 검사한다.
    constructor(create, state, factory, tree, lifetime, events, input, fragments, buffers, text, sliderWidgets, hooks = {}) {
        if (typeof hooks.node !== 'function' || typeof hooks.slider !== 'function' || typeof hooks.base !== 'function') throw new TypeError('다중 줄 Widget 공유 저장소 필요');
        this.#create = create; this.#state = state; this.#factory = factory; this.#tree = tree; this.#lifetime = lifetime; this.#events = events; this.#input = input; this.#fragments = fragments; this.#buffers = buffers; this.#text = text; this.#sliderWidgets = sliderWidgets; this.#hooks = hooks;
    }

    // 표시/입력/수명과 생성이 같은 기본·파생 객체를 사용해야 한다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node || node !== this.#hooks.base(token)) throw new Error('다중 줄 Widget 공유 노드 없음'); return node; }
    // Slider 값/serial/기본 크기는 실제 생성한 같은 Slider 저장소에서 읽는다.
    #slider(token) { const node = token ? this.#hooks.slider(token) : null; if (!node || node !== this.#hooks.base(token)) throw new Error('다중 줄 Widget Slider 노드 없음'); return node; }
    // 가상 Push/Attach 의 토큰도 실제 기본 저장소가 존재해야 한다.
    #base(token) { const node = token ? this.#hooks.base(token) : null; if (!node) throw new Error('다중 줄 Widget 기본 노드 없음'); return node; }
    // 원본 가상 +8 인자는 erase=false 다. 콜백이 변경한 현재 요청/플래그를 되돌리지 않는다.
    #redraw(token) { need(this.#hooks.redraw, '다중 줄 Widget Redraw 경계 필요')(token, false); }
    // Slider 의 실제 가상 Push 를 호출하고 이후 현재 Slider 필드를 다시 읽는다.
    #push(token) { this.#base(token); need(this.#hooks.push, '다중 줄 Widget Push 경계 필요')(token); }
    // 원본 우측 Slider 는 mode 1 로 화면 제한을 생략하며 y 는 항상 0 이다.
    #attachSlider(token, x, parent) { this.#base(token); need(this.#hooks.attach, '다중 줄 Widget Attach 경계 필요')(token, x, 0, parent, 1); }

    // 비0 은 원본 입력 제외 비트(0x200)를 해제한다. 값의 최하위 비트가 아닌 전체 DWORD 의 0/비0 을 쓴다.
    setSelectable(token, value) { const node = this.#node(token); if (value) node.flags = (node.flags & ~KEYBOARD_EXCLUDE) >>> 0; else node.flags = (node.flags | KEYBOARD_EXCLUDE) >>> 0; }

    // 값의 최하위 비트로 slider 비트(2)를 바꾸고 slider 만 켜고 scroll 이 꺼져 있으면 scroll 비트(4)도 켠다. 기본 void 비트(8)가 없으면 현재 Push 뒤의 현재 x/y 로 자신 Attach(parent 0, mode 0)를 호출한다.
    setSlider(token, value) {
        const node = this.#node(token); node.textFlags = ((node.textFlags & ~USE_SLIDER) | ((value & 1) << 1)) >>> 0;
        if ((node.textFlags & (USE_SLIDER | SCROLL)) === USE_SLIDER) node.textFlags = (node.textFlags | SCROLL) >>> 0;
        if (!(this.#node(token).flags & VOID)) {
            this.#push(token); const x = this.#node(token).x; const y = this.#node(token).y;
            need(this.#hooks.attach, '다중 줄 Widget 자기 Attach 경계 필요')(token, x, y, 0, 0);
        }
    }

    // 현재 버퍼의 noWrap=(value==0)만 저장한다(변경 플래그/Redraw 는 호출하지 않는 원본 누락 유지).
    setWrap(token, value) { this.#buffers.binding(this.#node(token).buffer).state.noWrap = value === 0 ? 1 : 0; }

    // 최하위 비트로 scroll 비트(4)를 바꾸며 끄면서 slider 만 켜져 있으면 SetSlider(0)로 현재 slider 도 끈다(가상 재부착까지 이어짐).
    setScrollable(token, value) {
        const node = this.#node(token); node.textFlags = ((node.textFlags & ~SCROLL) | ((value & 1) << 2)) >>> 0;
        if ((node.textFlags & (USE_SLIDER | SCROLL)) === USE_SLIDER) this.setSlider(token, 0);
    }

    // 새 버퍼는 null 만 거부한다. 현재 소유 버퍼를 Destroy/free/포인터 0 처리한 뒤 새 포인터/소유 DWORD 만 쓴다(dirty/Redraw/스크롤/태그는 갱신하지 않으며 같은 소유 토큰 교체는 소멸된 참조를 남긴다).
    replaceBuffer(token, buffer, owns) {
        if (!buffer) throw new TypeError('다중 줄 Widget 교체 버퍼는 null일 수 없음');
        const old = this.#node(token).buffer;
        if (old && this.#node(token).ownsBuffer) { this.#buffers.destroy(old); this.#node(token).buffer = 0; }
        this.#node(token).buffer = buffer >>> 0; this.#node(token).ownsBuffer = owns >>> 0;
    }

    // 기본 등록 → 파생 가상 표/보조 칸/signed 종류 → 128칸 버퍼(0x38 할당·소유=1) → float32 크기/문맥/overlay/요청 초기화 → 숨긴 Slider(9×100, 0xd8 할당) 생성/우측 mode 1 부착 → 맨 아래 요청/기본 플래그다.
    // +ac/dragY 와 기존 textFlags 의 다른 비트는 보존한다(전체 생성 EH 정리는 별도 후속). C++17 대입 순서(우변 먼저)로 현재 노드에 토큰을 쓴다.
    construct(token, width, height, font, color) {
        this.#node(token); this.#factory.construct(token, 0, 0, 0, 1); const node = this.#node(token);
        node.vtable = VTABLE; node.reserved[0] = 0; node.reserved[1] = 0; node.command = ((this.#state.alignment << 24) >> 24) >>> 0;
        const allocate = need(this.#hooks.allocate, '다중 줄 Widget 할당 경계 필요');
        const bufferToken = allocate(BUFFER_BYTES) >>> 0; const buffer = bufferToken ? this.#buffers.construct(bufferToken, LINES) : 0;
        this.#node(token).buffer = buffer; this.#node(token).ownsBuffer = 1;
        if (!this.#node(token).buffer) throw new Error('다중 줄 Widget 버퍼 할당 실패');
        width |= 0; height |= 0; const savedWidth = Math.fround(width);
        setDimension(node, 'baseWidth', width); setDimension(node, 'width', width * node.scale); setDimension(node, 'baseHeight', height); setDimension(node, 'height', height * node.scale);
        node.textContext.set([0, font >>> 0, color >>> 0, 0, 0, 0, 1, 1]); node.layoutWidth = 0; node.scroll = 0; node.callback = 0; node.flags = (node.flags | CREATE_FLAGS) >>> 0;
        this.#lifetime.setOverlay(token, true); node.scrollRequest = -1; node.bottomPadding = 0;
        let slider = allocate(SLIDER_BYTES) >>> 0;
        if (slider) slider = this.#sliderWidgets.construct(slider, SLIDER_WIDTH, SLIDER_HEIGHT, this.#state.sliderType, this.#state.sliderFrame);
        this.#node(token).slider = slider; const sliderNode = this.#slider(slider); sliderNode.flags = (sliderNode.flags & ~VISIBLE) >>> 0;
        this.#attachSlider(this.#node(token).slider, Math.fround(savedWidth - this.#slider(this.#node(token).slider).baseWidth), token);
        this.#text.requestBottom(token); const current = this.#node(token); current.textFlags = (((current.textFlags & ~(USE_SLIDER | SCROLL)) | LEFT_TAG | RIGHT_TAG)) >>> 0;
        current.metadata = 0;
        return token;
    }

    // 기본 부착 중에도 파생 자식 분배를 사용한다. 그 뒤 현재 Slider 의 Push/높이/표시(slider+scroll 비트가 모두 켜졌을 때만 visible)와 우측 부착, 대기 요청이 없으면 현재 scroll 의 상대 요청, 부모 폭 변경(NaN 포함)을 처리한다.
    attach(token, x, y, parent, mode) {
        this.#node(token); this.#tree.attach(token, x, y, parent, mode); this.#push(this.#node(token).slider);
        const slider = this.#slider(this.#node(token).slider); const height = this.#node(token).baseHeight;
        setDimension(slider, 'baseHeight', height); setDimension(slider, 'height', height * slider.scale);
        if ((this.#node(token).textFlags & (USE_SLIDER | SCROLL)) === (USE_SLIDER | SCROLL)) slider.flags = (slider.flags | VISIBLE) >>> 0; else slider.flags = (slider.flags & ~VISIBLE) >>> 0;
        this.#attachSlider(this.#node(token).slider, Math.fround(this.#node(token).baseWidth - this.#slider(this.#node(token).slider).baseWidth), token);
        if (this.#node(token).scrollRequest === -1) this.#text.requestScroll(token, this.#node(token).scroll);
        const previousWidth = this.#node(token).layoutWidth; const currentWidth = this.#node(token).baseWidth;
        // 패치는 unordered(NaN)도 폭 변경으로 처리한다.
        if (previousWidth !== currentWidth) { const node = this.#node(token); node.textFlags = (node.textFlags | DIRTY) >>> 0; this.#node(token).layoutWidth = this.#node(token).baseWidth; }
    }

    // 실제 조각의 현재 T 태그는 문자열/그림을 구분하지 않는다.
    #tag(fragment) { const found = fragment ? this.#fragments.fragments.get(fragment) : null; if (!found) throw new Error('다중 줄 Widget 태그 조각 없음'); return found.metadata; }
    // 화면 좌표를 원본 signed DWORD 로 전달한다. hitTest 자체는 layout/대기 스크롤을 적용하지 않는다.
    #hit(token, x, y) { return this.#text.hitTest(token, { x: x | 0, y: y | 0 }); }

    // Slider 사건을 소비한 뒤에도 원본의 후속 refresh/마우스/커서 분류를 계속 검사한다. 반환 1 은 처리됨이다.
    // 변경 사건은 발신 Slider serial 이 맞아야 하고 현재 Slider 위치로 스크롤을 요청한다. step 사건(1 위·2 아래 ±5, 3/4 페이지 ±기본 높이)은 상대 요청이다.
    // 왼쪽 버튼: 태그 위가 아니면 release 에서 통지 해제·누름에서 y 저장/통지 설정/캡처, 태그 위의 누름이면 왼쪽 태그 사건을 부모에 보낸다. 오른쪽 버튼: 태그 위의 누름만 오른쪽 태그 사건을 보낸다.
    // 커서 사건: 태그 위 16·scroll 가능하고 page 가 범위보다 작으면 15·그 밖에는 1 을 선택 경계로 보낸다.
    handle(token, event, first, second) {
        event >>>= 0; first >>>= 0; second >>>= 0; const code = (event & EVENT_MASK) >>> 0; let result = 0;
        if (code === this.#state.changeEvent) {
            if (first !== this.#slider(this.#node(token).slider).serial) throw new TypeError('다중 줄 Widget 변화 사건 발신 Slider 불일치');
            this.#text.requestScroll(token, this.#slider(this.#node(token).slider).position);
        }
        if (code === this.#state.stepEvent && first === this.#slider(this.#node(token).slider).serial) {
            let delta = 0; let step = true;
            switch (second) {
                case 1: delta = STEP; break;
                case 2: delta = -STEP; break;
                case 3: delta = ftol(this.#node(token).baseHeight); break;
                case 4: delta = ftol(-this.#node(token).baseHeight); break;
                default: step = false;
            }
            if (step) { this.#text.requestRelativeScroll(token, delta); result = 1; }
        }
        if (code === this.#state.refreshEvent) { const node = this.#node(token); node.textFlags = (node.textFlags | DIRTY) >>> 0; this.#redraw(token); }
        if (code !== this.#state.cursorEvent) {
            if (code === InputCode.leftButton) {
                const fragment = this.#node(token).textFlags & LEFT_TAG ? this.#hit(token, first, second) : 0;
                if (!fragment || !this.#tag(fragment)) {
                    if (event & InputCode.release) { const node = this.#node(token); node.flags = (node.flags & ~NOTIFY) >>> 0; }
                    else { this.#node(token).dragY = second | 0; const node = this.#node(token); node.flags = (node.flags | NOTIFY) >>> 0; this.#input.capture(token); }
                } else if (!(event & InputCode.release)) this.#events.bubble(token, this.#state.leftTagEvent, this.#node(token).serial, this.#tag(fragment));
                result = 1;
            }
            if (code === InputCode.rightButton && (this.#node(token).textFlags & RIGHT_TAG)) {
                if (!(event & InputCode.release)) {
                    const fragment = this.#hit(token, first, second);
                    if (fragment) { this.#events.bubble(token, this.#state.rightTagEvent, this.#node(token).serial, this.#tag(fragment)); return 1; }
                }
                result = 0;
            }
            return result;
        }
        let cursor = NORMAL_CURSOR;
        const fragment = this.#node(token).textFlags & LEFT_TAG ? this.#hit(token, first, second) : 0;
        if (fragment && this.#tag(fragment)) cursor = TAG_CURSOR;
        else if (this.#node(token).textFlags & SCROLL) {
            const slider = this.#slider(this.#node(token).slider);
            if (slider.page < subtract(slider.maximum, slider.minimum)) cursor = SCROLL_CURSOR;
        }
        need(this.#hooks.cursor, '다중 줄 Widget 커서 선택 경계 필요')(cursor); return 1;
    }

    // 폴링/사건 종류/x 를 읽지 않는다. scroll 비트(4)가 없으면 이전 y 도 갱신하지 않는다. 전달 y 와 저장 y 의 차이만큼 반대 방향의 상대 요청을 한다.
    broadcast(token, event, first, second) {
        void event; void first; if (!(this.#node(token).textFlags & SCROLL)) return;
        const y = second | 0; const delta = subtract(y, this.#node(token).dragY);
        if (delta) { this.#node(token).dragY = y; this.#text.requestRelativeScroll(token, subtract(0, delta)); }
    }

    // 파생 가상 표 → 현재 소유 버퍼 소멸/free/포인터 0 → 기본 scalar 소멸이다. 버퍼 포인터는 소멸 전 저장하고 소유 여부는 현재 값을 검사한다. 빌린 버퍼는 포인터도 그대로 보존한다.
    deleteScalar(token, flags) {
        this.#node(token).vtable = VTABLE; const buffer = this.#node(token).buffer;
        if (buffer && this.#node(token).ownsBuffer) { this.#buffers.destroy(buffer); this.#node(token).buffer = 0; }
        return this.#lifetime.deleteScalar(token, flags);
    }
}

// 원본 버퍼 생성/표시/소멸이 하나의 상태/handler 레코드를 공유하도록 연결한다(각 표시 호출에서 현재 token 으로 같은 실제 버퍼를 지연 조회).
export function bindScrollingTextBuffers(hooks, buffers) { hooks.buffer = (token) => buffers.binding(token); }

// 가상 Attach/재귀·부모 통지/직접 캡처/프레임/scalar 를 실제 다중 줄 Widget 에 연결하고 이전 분배를 보존한다. resolve 는 호출 때마다 평가한다(처리기 생성 뒤 연결 가능).
export function bindScrollingTextGumpWidgets(tree, events, input, frame, lifetime, resolve) {
    if (typeof resolve !== 'function') throw new TypeError('다중 줄 Widget 분류 경계 필요');
    const attach = tree.attach;
    // 기본 부모 부착은 Widgets 에서 실제 tree 몸체로 직접 위임해 가상 재귀를 피한다.
    tree.attach = (token, x, y, parent, mode, bits) => {
        const handler = resolve(token);
        if (handler) handler.attach(token, x, y, parent, mode);
        else { if (!attach) throw new TypeError('다중 줄 Widget 외 Attach 필요'); attach(token, x, y, parent, mode, bits); }
    };
    const eventHandle = events.handle; const inputHandle = input.handle;
    // 부모 Bubble/재귀 선택과 직접 캡처 입력은 같은 실제 Widget Handle 에 도달한다.
    const route = (previous, token, code, first, second) => {
        const handler = resolve(token); if (handler) return handler.handle(token, code, first, second);
        if (!previous) throw new TypeError('다중 줄 Widget 외 입력 필요');
        return previous(token, code, first, second);
    };
    events.handle = (token, code, first, second) => route(eventHandle, token, code, first, second);
    input.handle = (token, code, first, second) => route(inputHandle, token, code, first, second);
    const broadcast = frame.broadcast;
    // 기존 프레임 통지 필터 이후 실제 원본 y 드래그 몸체를 실행한다.
    frame.broadcast = (token, code, first, second) => {
        const handler = resolve(token);
        if (handler) handler.broadcast(token, code, first, second);
        else { if (!broadcast) throw new TypeError('다중 줄 Widget 외 Broadcast 필요'); broadcast(token, code, first, second); }
    };
    const deleting = lifetime.deletingDestructor;
    // 자식 우선 파괴/등록 해제 뒤 버퍼 소유와 실제 기본 scalar 소멸을 수행한다.
    lifetime.deletingDestructor = (token, flags) => {
        const handler = resolve(token);
        if (handler) handler.deleteScalar(token, flags);
        else { if (!deleting) throw new TypeError('다중 줄 Widget 외 scalar 필요'); deleting(token, flags); }
    };
}

// 다중 줄 노드 클래스를 재노출해 상위 조립이 한 곳에서 가져올 수 있게 한다(E423 의 노드가 곧 이 처리기의 파생 노드다).
export { ScrollingTextGumpNode };
