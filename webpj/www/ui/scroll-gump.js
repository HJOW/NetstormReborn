// 원본 Slider 의 값/범위/페이지·자식 부착과 손잡이 선택/드래그/부모 통지를 같은 Gump 저장소에 연결한다.
// (cpppj client/ScrollGump.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 의 float32 중간 저장·SetPage 위임·가로 손잡이 무동작은 옮기지 않는다)
// 동일 노드의 콜백 후 현재 필드와 원본의 DWORD 산술·float 저장 전 배율·가로/세로 입력 차이를 유지한다. 생성/파생 Draw 는 ScrollGumpWidgets 단위의 몫이다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E425: RawMenuControls 선행 ScrollGump 이식)
import { GumpCreateNode } from './gump-create.js';
import { GumpBackgroundNode } from './gump-background-create.js';
import { InputCode } from '../core/input-queue.js';

// 원본 flags 의 void 비트·손잡이의 프레임 통지 비트·scrollFlags 의 가로 비트·사건 코드의 하위 분류 마스크다.
const VOID = 8; const BROADCAST = 0x100; const HORIZONTAL = 1; const EVENT_MASK = 0x03ffffff;
// 원본 정수 ADD/SUB 의 하위 DWORD 를 signed 로 해석한다.
const add = (first, second) => (first + second) | 0;
const subtract = (first, second) => (first - second) | 0;
// CRT _ftol 의 signed 64비트 절삭 결과의 하위 DWORD 다. 무효 변환의 하위 DWORD 는 0 이다.
function ftol(value) { if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 선택된 장치가 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 거부한다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }

// 원본 +a0 min/+a4 max/+a8 position/+ac page/+b4 방향/+b8 step/+c8..d0 자식 토큰이다. 기본 생성과 화살표/손잡이 할당은 ScrollGumpWidgets 가 수행한다.
export class ScrollGumpNode extends GumpCreateNode {
    // 스크롤 가능한 최소/최대 위치·현재 위치·페이지 크기·한 칸 이동량(signed 32비트)이다.
    minimum = 0; maximum = 0; position = 0; page = 0; step = 0;
    // 방향/입력 동작 플래그(bit0 가로)와 위치 감소/증가 버튼·손잡이의 Gump 토큰이다.
    scrollFlags = 0; decrement = 0; increment = 0; thumb = 0;
    // +b0 은 생성 시 0, +bc 는 미작성, +c0/+c4 는 생성자가 0 으로 쓰는 보조 필드다.
    reservedB0 = 0; reservedBC = 0; reserved = [0, 0];
}

// 손잡이 +c0..cc 의 양 끝 포함 사각형과 +d0 이전 y/+d4 이전 x 다(+d4 는 패치에만 존재). 기본 배경 필드/트리/캐시를 같은 객체에서 공유한다.
export class ScrollGumpThumbNode extends GumpBackgroundNode {
    // +b8/+bc 의 생성 인자 사본이다(원본 손잡이 Draw 는 이 값 대신 기본 배경의 type/frame 을 쓴다).
    detailType = 0; detailFrame = 0;
    // 양 끝 포함 손잡이 사각형(손잡이 로컬 좌표)이다.
    handle = { left: 0, top: 0, right: 0, bottom: 0 };
    // 드래그 중 이전 y/x 다.
    previousY = 0; previousX = 0;
}

// 초기 문자열 ID 의 현재 전역이다. click/repeat 는 ButtonGumpState 와 같은 실제 ID 를 공급한다.
// 패치 click/repeat/step/change: 005427a8/005427b4/005427b0/005427d0.
export class ScrollGumpState {
    // 왼쪽 클릭·누름 유지 반복·스크롤 이동 요청·값 변경 통지의 사건 ID 다.
    clickEvent = 0; repeatEvent = 0; stepEvent = 0; changeEvent = 0;
}

// ScrollGump.cpp 의 Slider 값/부착/입력 처리기다. 생성/방향/사건 전역과 실제 트리/좌표/부모 통지/캡처가 이 처리기보다 오래 살아야 한다.
// hooks: { node, thumbNode, baseNode, push(token), attach(token,x,y,parent,mode), redraw(token,erase), poll(code)→{code,x,y} }.
// 사용: const slider = new ScrollGump(create, state, tree, collection, events, input, hooks); slider.setRange(token, 0, 30).
export class ScrollGump {
    // 같은 생성 상태·사건 전역·트리/수집기/사건/입력 처리기와 노드 조회·가상 경계다.
    #create; #state; #tree; #collection; #events; #input; #hooks;
    // 안정된 기본/두 파생 조회만 생성 시 요구한다. 가상 호출과 폴링은 실제 도달 시 검사한다.
    constructor(create, state, tree, collection, events, input, hooks = {}) {
        if (typeof hooks.node !== 'function' || typeof hooks.thumbNode !== 'function' || typeof hooks.baseNode !== 'function') throw new TypeError('ScrollGump 공유 저장소 필요');
        this.#create = create; this.#state = state; this.#tree = tree; this.#collection = collection; this.#events = events; this.#input = input; this.#hooks = hooks;
    }

    // Slider 의 값/방향/링크와 기본 트리가 같은 노드를 쓰도록 확인한다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node || node !== this.#hooks.baseNode(token)) throw new Error('ScrollGump 공유 노드 없음'); return node; }
    // 손잡이 rect/이전 좌표/부모 링크를 복사본이 아닌 같은 객체에서 읽는다.
    #thumb(token) { const node = token ? this.#hooks.thumbNode(token) : null; if (!node || node !== this.#hooks.baseNode(token)) throw new Error('ScrollGump 손잡이 공유 노드 없음'); return node; }
    // 화살표 버튼은 StoneButtonGump 등 다른 파생 노드도 같은 기본 저장소로 허용한다.
    #base(token) { const node = token ? this.#hooks.baseNode(token) : null; if (!node) throw new Error('ScrollGump 기본 노드 없음'); return node; }
    // 현재 자식 토큰의 가상 Push(+18)를 호출한다. 기본 부모 부착은 tree.attach 로 직접 위임한다.
    #push(token) { need(this.#hooks.push, 'ScrollGump 가상 Push 경계 필요')(token); }
    // 자식은 mode 0 으로 부착한다. 부모 serial/token 이나 위치 캐시를 임의 생성하지 않는다.
    #attachChild(token, x, y, parent) { need(this.#hooks.attach, 'ScrollGump 자식 Attach 경계 필요')(token, x, y, parent, 0); }
    // 기본 크기를 float32 에 쓰지만 배율 결과는 저장 전 size 로 계산한다. 폭/높이 중 지정 축만 바꾼다.
    #setSize(token, size, width) {
        const child = this.#base(token);
        const field = width ? 'baseWidth' : 'baseHeight'; const scaled = width ? 'width' : 'height';
        child[field] = Math.fround(size); child[scaled] = Math.fround(size * child.scale);
        if (child.floatBits) { delete child.floatBits[field]; delete child.floatBits[scaled]; }
    }

    // page 계약 → 최솟값/(최댓값-page) 제한 → position 저장 → (void 가 아니면) 현재 Push 뒤의 현재 x/y 로 가상 Attach(parent 0, mode 0), void 이면 redraw(false) 다.
    // 범위가 page 보다 작으면 거부한다(원본 assert). 사용: slider.setPosition(token, 12).
    setPosition(token, position) {
        position |= 0; const node = this.#node(token);
        if (subtract(node.maximum, node.minimum) < node.page) throw new TypeError('ScrollGump page가 범위보다 큼');
        if (position < node.minimum) position = node.minimum;
        if (node.maximum < add(node.page, position)) position = subtract(node.maximum, node.page);
        node.position = position;
        if (!(node.flags & VOID)) {
            this.#push(token);
            const x = this.#node(token).x; const y = this.#node(token).y;
            need(this.#hooks.attach, 'ScrollGump 가상 Attach 경계 필요')(token, x, y, 0, 0);
        } else need(this.#hooks.redraw, 'ScrollGump Redraw 경계 필요')(token, false);
    }

    // page 를 signed 범위 길이 이하로 제한한 뒤 현재 position 을 다시 적용한다(음수 page 는 원본처럼 하한 제한하지 않는다).
    setPage(token, page) {
        page |= 0; const node = this.#node(token); const length = subtract(node.maximum, node.minimum);
        if (length < page) page = length;
        node.page = page; this.setPosition(token, this.#node(token).position);
    }

    // 두 끝점을 정렬하고 같으면 최대+1 로 한 뒤 최대/최소를 저장하고 page 를 범위 길이 이하로 제한한 다음 현재 position 을 두 번 다시 적용한다(패치 순서).
    setRange(token, first, last) {
        first |= 0; last |= 0;
        const minimum = first < last ? first : last; let maximum = first < last ? last : first;
        if (first === last) maximum = add(minimum, 1);
        const node = this.#node(token);
        node.maximum = maximum; node.minimum = minimum;
        const length = subtract(maximum, minimum); if (length < node.page) node.page = length;
        this.setPosition(token, this.#node(token).position);
        this.setPosition(token, this.#node(token).position);
    }

    // 현재 +a8 을 그대로 돌려준다(bool 변환/범위 보정 없음). TextGump 의 마지막 slider 재조회가 같은 저장소를 읽게 한다.
    position(token) { return this.#node(token).position; }

    // 기본 부착 → 세 자식 Push → 방향별 크기/위치 부착 → 현재 값/페이지의 손잡이 사각형 계산이다. 방향 bit 1 은 가로이며 손잡이 위치 분자는 position-minimum 이 아닌 원본의 position 이다.
    attach(token, x, y, parent, mode) {
        this.#node(token); this.#tree.attach(token, x, y, parent, mode);
        const horizontal = (this.#node(token).scrollFlags & HORIZONTAL) !== 0;
        const thickness = ftol(horizontal ? this.#node(token).baseHeight : this.#node(token).baseWidth);
        const roundedThickness = Math.fround(thickness); const secondThickness = thickness; const doubleThickness = Math.fround(add(thickness, thickness));
        this.#push(this.#node(token).decrement); this.#push(this.#node(token).increment); this.#push(this.#node(token).thumb);
        if (horizontal) {
            this.#setSize(this.#node(token).decrement, this.#node(token).baseHeight, false); this.#setSize(this.#node(token).decrement, thickness, true);
            this.#setSize(this.#node(token).increment, this.#node(token).baseHeight, false); this.#setSize(this.#node(token).increment, secondThickness, true);
            this.#setSize(this.#node(token).thumb, this.#node(token).baseHeight, false); this.#setSize(this.#node(token).thumb, this.#node(token).baseWidth - doubleThickness, true);
            this.#attachChild(this.#node(token).decrement, 0, 0, token);
            this.#attachChild(this.#node(token).increment, Math.fround(this.#node(token).baseWidth - roundedThickness), 0, token);
            this.#attachChild(this.#node(token).thumb, roundedThickness, 0, token);
        } else {
            this.#setSize(this.#node(token).decrement, thickness, false); this.#setSize(this.#node(token).decrement, this.#node(token).baseWidth, true);
            this.#setSize(this.#node(token).increment, secondThickness, false); this.#setSize(this.#node(token).increment, this.#node(token).baseWidth, true);
            this.#setSize(this.#node(token).thumb, this.#node(token).baseHeight - doubleThickness, false); this.#setSize(this.#node(token).thumb, this.#node(token).baseWidth, true);
            this.#attachChild(this.#node(token).decrement, 0, 0, token);
            this.#attachChild(this.#node(token).increment, 0, Math.fround(this.#node(token).baseHeight - roundedThickness), token);
            this.#attachChild(this.#node(token).thumb, 0, roundedThickness, token);
        }
        const node = this.#node(token); const thumb = this.#thumb(node.thumb);
        const available = (horizontal ? node.baseWidth : node.baseHeight) - doubleThickness;
        const range = subtract(node.maximum, node.minimum);
        const page = ftol(node.page * available / range); const position = ftol(node.position * available / range);
        const cross = subtract(ftol(horizontal ? node.baseHeight : node.baseWidth), 1);
        const first = add(position, 1); const last = subtract(add(position, page), 1);
        if (horizontal) Object.assign(thumb.handle, { bottom: cross, left: first, top: 1, right: last });
        else Object.assign(thumb.handle, { right: cross, left: 1, top: first, bottom: last });
    }

    // 세로에서만 DWORD 곱/CRT 절삭으로 y delta 를 position 으로 바꾼다. 원본 세로 몸체는 range*(delta+현재 손잡이 top)의 DWORD 곱을 height-2*width 로 나눈다.
    dragVertical(token, delta) {
        const node = this.#node(token); if (node.scrollFlags & HORIZONTAL) return;
        const numerator = Math.imul(subtract(node.maximum, node.minimum), add(delta | 0, this.#thumb(node.thumb).handle.top));
        const denominator = node.baseHeight - node.baseWidth * 2;
        this.setPosition(token, ftol(numerator / denominator));
    }

    // 변화 통지는 현재 사건 전역/serial 을 부모 체인에 보낸다. 값은 항상 0 이다.
    #notifyChange(token) { this.#events.bubble(token, this.#state.changeEvent, this.#base(token).serial, 0); }

    // 화살표 click/repeat·손잡이 step 을 부모 체인에 통지하고 현재 값 변경을 전달한다. 원본은 명령을 소비해도 0 을 반환해 더 위 부모 전달을 계속 허용한다.
    // 화살표는 부모가 step 을 처리하지 않을 때만(Bubble 이 0) 직접 position 을 한 칸 옮긴다. 사용: bindScrollGump 가 사건/입력 분배에 연결한다.
    handle(token, event, first, second) {
        event = (event & EVENT_MASK) >>> 0; first >>>= 0; second >>>= 0;
        if ((event === this.#state.clickEvent || event === this.#state.repeatEvent) && first !== this.#base(this.#node(token).thumb).serial) {
            const previous = this.#node(token).position; let arrow = false;
            if (first === this.#base(this.#node(token).decrement).serial) {
                arrow = true;
                if (!this.#events.bubble(token, this.#state.stepEvent, this.#node(token).serial, 2)) this.setPosition(token, subtract(this.#node(token).position, this.#node(token).step));
            } else if (first === this.#base(this.#node(token).increment).serial) {
                arrow = true;
                if (!this.#events.bubble(token, this.#state.stepEvent, this.#node(token).serial, 1)) this.setPosition(token, add(this.#node(token).step, this.#node(token).position));
            }
            if (arrow && previous !== this.#node(token).position) this.#notifyChange(token);
        }
        if (event === this.#state.stepEvent && first === this.#base(this.#node(token).thumb).serial) {
            const previous = this.#node(token).position;
            this.#events.bubble(token, this.#state.stepEvent, this.#node(token).serial, second);
            if (previous !== this.#node(token).position) this.#notifyChange(token);
        }
        return 0;
    }

    // 왼쪽 누름에서 손잡이 안이면 선택/캡처, 손잡이 밖이면 page 방향 3/4 를 부모에 통지한다. 범위 비교는 실제 양 끝 포함 손잡이 rect 다.
    // 세로만 release 에서 플래그(BROADCAST)를 해제하고 가로 release 는 플래그를 건드리지 않는다. 항상 1 을 반환한다(왼쪽 버튼 이외는 0).
    thumbHandle(token, event, first, second) {
        if (((event & EVENT_MASK) >>> 0) !== InputCode.leftButton) return 0;
        const thumb = this.#thumb(token); const vertical = !(thumb.baseWidth > thumb.baseHeight);
        if ((event & InputCode.release) !== 0) { if (vertical) thumb.flags = (thumb.flags & ~BROADCAST) >>> 0; return 1; }
        const bounds = this.#collection.bounds(token);
        const point = (vertical ? second : first) | 0;
        const local = subtract(point, vertical ? bounds.top : bounds.left);
        const low = vertical ? this.#thumb(token).handle.top : this.#thumb(token).handle.left; const high = vertical ? this.#thumb(token).handle.bottom : this.#thumb(token).handle.right;
        if (local < low) this.#events.bubble(token, this.#state.stepEvent, this.#thumb(token).serial, 4);
        else if (high < local) this.#events.bubble(token, this.#state.stepEvent, this.#thumb(token).serial, 3);
        else {
            if (vertical) this.#thumb(token).previousY = point; else this.#thumb(token).previousX = point;
            const current = this.#thumb(token); current.flags = (current.flags | BROADCAST) >>> 0; this.#input.capture(token);
        }
        return 1;
    }

    // 폴링한 왼쪽 버튼이 release 이면 플래그만 해제한다. 그 밖에는 사건/x 를 무시하고 전달 y 와 이전 y 의 차이로 부모 Slider 를 갱신하고 변경을 부모 체인에 통지한다(폴링의 좌표는 읽지 않는다).
    thumbBroadcast(token, event, first, second) {
        void event; void first;
        const poll = need(this.#hooks.poll, 'ScrollGump 손잡이 폴링 경계 필요');
        if ((poll(InputCode.leftButton).code & InputCode.release) !== 0) { const thumb = this.#thumb(token); thumb.flags = (thumb.flags & ~BROADCAST) >>> 0; return; }
        const y = second | 0; const delta = subtract(y, this.#thumb(token).previousY);
        if (delta) {
            this.#thumb(token).previousY = y; this.dragVertical(this.#thumb(token).parent, delta);
            this.#events.bubble(token, this.#state.changeEvent, this.#base(this.#thumb(token).parent).serial, 0);
        }
    }
}

// Slider 가상 Attach/Handle 과 손잡이 Handle/Broadcast 를 같은 트리/입력/프레임에 연결한다. 각 resolve 는 해당 파생 토큰일 때만 처리기를 돌려주고 다른 기존 분배기는 보존한다.
// resolve 는 호출 때마다 평가한다(순환 의존 처리기를 만든 뒤 연결할 수 있다).
export function bindScrollGump(tree, events, input, frame, slider, thumb) {
    if (typeof slider !== 'function' || typeof thumb !== 'function') throw new TypeError('ScrollGump 분류 경계 필요');
    const attach = tree.attach;
    // Slider 자체만 파생 Attach 를 호출하고 나머지 돌 버튼/기본 부착 분배를 보존한다.
    tree.attach = (token, x, y, parent, mode, bits) => {
        const handler = slider(token);
        if (handler) handler.attach(token, x, y, parent, mode);
        else { if (!attach) throw new TypeError('ScrollGump 외 Attach 처리기 필요'); attach(token, x, y, parent, mode, bits); }
    };
    const eventHandle = events.handle; const inputHandle = input.handle;
    // 재귀/부모 Bubble 과 직접 캡처 입력은 같은 실제 Slider/손잡이 Handle 로 분배한다.
    const route = (previous, token, code, first, second) => {
        const sliderHandler = slider(token); if (sliderHandler) return sliderHandler.handle(token, code, first, second);
        const thumbHandler = thumb(token); if (thumbHandler) return thumbHandler.thumbHandle(token, code, first, second);
        if (!previous) throw new TypeError('ScrollGump 외 입력 처리기 필요');
        return previous(token, code, first, second);
    };
    events.handle = (token, code, first, second) => route(eventHandle, token, code, first, second);
    input.handle = (token, code, first, second) => route(inputHandle, token, code, first, second);
    const broadcast = frame.broadcast;
    // 원본 손잡이의 가상 +2c 만 복원한 폴링/드래그 몸체로 연결한다.
    frame.broadcast = (token, code, first, second) => {
        const handler = thumb(token);
        if (handler) handler.thumbBroadcast(token, code, first, second);
        else { if (!broadcast) throw new TypeError('ScrollGump 외 Broadcast 처리기 필요'); broadcast(token, code, first, second); }
    };
}
// 기존 다중 줄 TextGump 의 네 Slider 경계를 실제 범위/페이지/위치/현재 값 구현에 연결한다(position 조회까지 실제 Slider 와 같은 저장소).
export function bindScrollingTextSlider(hooks, slider) {
    hooks.sliderRange = (token, first, last) => slider.setRange(token, first, last);
    hooks.sliderPage = (token, page) => slider.setPage(token, page);
    hooks.sliderSetPosition = (token, position) => slider.setPosition(token, position);
    hooks.sliderPosition = (token) => slider.position(token);
}
