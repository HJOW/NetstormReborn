// 메뉴의 선택 통지·하위 메뉴 열기/닫기·명령과 부모 통지를 같은 트리/수명 저장소에 연결한다.
// (cpppj client/MenuGumpInput.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 의 unsigned float 비트 하한(-0 허용)은 옮기지 않는다)
// 콜백(Redraw/Kill/명령 큐/소리/가상 입력) 뒤에는 현재 선택/항목 플래그/링크/등록표를 다시 읽고 숨김과 지연 삭제의 원본 순서를 유지한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E431: RawMenuControls 선행 MenuGumpInput 이식)
import { MENU_ITEM_CAPACITY } from './menu-gump.js';

// 메뉴 항목의 명령/유지/강조/자식/명시 인자/비활성/동적 자식 비트다.
const COMMAND = 1; const KEEP = 2; const SELECTED = 8; const CHILD = 0x20; const PARAMETER = 0x80; const DISABLED = 0x200; const DYNAMIC = 0x800;
// 메뉴 방향/루트 닫기/명령 뒤 삭제/compact/외부 누름 삭제와 기본 표시/팝업/dead 비트다.
const HORIZONTAL = 0x40000; const VERTICAL = 0x80000; const ROOT_MENU = 0x100000; const DELETE_COMMAND = 0x200000;
const COMPACT = 0x1000000; const DELETE_OUTSIDE = 0x4000000; const VISIBLE = 1; const POPUP = 0x4000; const DEAD = 0x10000;
// 마우스 사건의 종류 마스크·왼쪽/오른쪽 누름·release·선택 해제(화면 밖) 비트다.
const EVENT_MASK = 0x03ffffff; const LEFT = 0x10000; const RIGHT = 0x20000; const RELEASE = 0x80000000; const OUTSIDE = 0x4000000;

// CRT _ftol 의 64비트 절삭에서 signed 하위 DWORD 만 취한다. 무효 정수의 하위 DWORD 는 0 이다.
function ftol(value) { if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 화면 끝점 합산의 signed DWORD 감김이다.
const add = (first, second) => (first + second) | 0;
// 선택된 장치가 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 거부한다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }
// 한 재귀 경로의 순환만 거부한다. 유효 원본 트리에서는 추가 동작을 하지 않는다.
function enter(active, token) { if (active.includes(token)) throw new Error('메뉴 재귀 링크 순환'); active.push(token); }

// +20 command 의 signed 메뉴 식별 바이트와 동적 하위 메뉴 요청/닫힘 사건 전역이다. 실제 초기화 값을 공급해야 한다.
export class MenuGumpInputState {
    // 메뉴 식별 바이트(signed, 두 원본 PE 의 초기값 26)다.
    alignment = 26;
    // 동적 하위 메뉴 요청 사건 번호와 메뉴 닫힘 통지 사건 번호다.
    requestEvent = 0; closedEvent = 0;
}

// 복원 메뉴의 실제 가상 입력 몸체다. OS 입력 큐와 메뉴 객체를 만드는 상위 Client 는 별도 연결 대상이다.
// 사용: new MenuGumpInput(createState, inputState, menu, collection, events, lifetime, tree, { node, base, broadcast, handle, push, attach, redraw, command, sound, invalidate }).handle(token, 0x10000, x, y)
export class MenuGumpInput {
    // 같은 생성/전역/메뉴 크기·수집기·사건·수명·트리 처리기와 현재 가상 경계다.
    #create; #state; #menu; #collection; #events; #lifetime; #tree; #hooks;
    // 생성/사건/수명/트리/메뉴 상태와 모든 저장소는 처리기 및 분배 훅보다 오래 살아야 한다. 실제 가상 호출 경계는 도달 시에만 요구한다.
    constructor(create, state, menu, collection, events, lifetime, tree, hooks = {}) {
        if (typeof hooks.node !== 'function' || typeof hooks.base !== 'function') throw new TypeError('메뉴 입력 공유 저장소 필요');
        this.#create = create; this.#state = state; this.#menu = menu; this.#collection = collection; this.#events = events; this.#lifetime = lifetime; this.#tree = tree; this.#hooks = hooks;
    }

    // 기본 조회는 메뉴 이외의 부모/팝업도 허용한다. 토큰 0 은 조회하지 않는다.
    #base(token) {
        const node = token ? this.#hooks.base(token) : null; if (!node) throw new Error('메뉴 입력 기본 노드 없음'); return node;
    }

    // 같은 객체의 현재 선택/항목 상태를 반환한다.
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node || node !== this.#base(token)) throw new Error('메뉴 입력 공유 노드 없음');
        return node;
    }

    // 원본은 itemCount 와 별개로 현재 선택의 40칸 주소에 접근한다. 사용하지 않는 유효 칸은 허용한다.
    #item(token, index) {
        if (index < 0 || index >= MENU_ITEM_CAPACITY) throw new RangeError('메뉴 입력 항목 범위 오류');
        return this.#node(token).items[index];
    }

    // 항목의 childId 는 Gump 토큰이 아니라 현재 등록표의 ID 다.
    #child(id) {
        if (id >= this.#create.entries.length || !this.#create.entries[id]) throw new Error('메뉴 자식 등록 ID 없음');
        return this.#create.entries[id];
    }

    // 패치의 ordered 하한(NaN 은 밖)과 두 판본의 strict 상한이다. 앞 조건이 실패하면 기본 노드의 크기를 읽지 않는다.
    #contains(token, local) {
        const x = local[0] >= 0; const y = local[1] >= 0;
        return x && local[0] < this.#base(token).width && y && local[1] < this.#base(token).height;
    }

    // 메뉴 식별인 자식만 재귀 검사한다. 메뉴 아닌 자식은 하위 탐색도 생략하며 표시 여부는 자신의 경계 검사에만 적용한다.
    // 원본은 재귀 뒤 현재 next 를 읽으므로 ScreenToLocal 의 투영 콜백이 바꾼 링크도 반영한다.
    #covered(token, point, active) {
        enter(active, token); const siblings = new Set(); const identity = ((this.#state.alignment << 24) >> 24) >>> 0;
        // 자식 형제 순환만 거부하며 메뉴 식별이 다르면 건너뛴다.
        for (let child = this.#base(token).child; child; child = this.#base(child).next) {
            if (siblings.has(child)) throw new Error('메뉴 가림 형제 순환'); siblings.add(child);
            if (this.#base(child).command !== identity) continue;
            const local = this.#collection.screenToLocal(child, point);
            if (((this.#base(child).flags & VISIBLE) && this.#contains(child, local)) || this.#covered(child, point, active)) { active.pop(); return true; }
        }
        active.pop(); return false;
    }

    // 가상 재그리기 요청은 픽셀 출력과 별개이며 원본 인자는 항상 false 다.
    #redraw(token) { need(this.#hooks.redraw, '메뉴 입력 Redraw 경계 필요')(token, false); }

    // 명시 인자 비트가 없으면 원본처럼 flags 전체 DWORD 를 인자로 전달한다.
    #parameter(token, index, flags) { return (flags & PARAMETER) ? this.#item(token, index).parameter : flags >>> 0; }

    // 현재 음향 경계가 없으면 해당 원본 동작을 생략하지 않고 도달 시 거부한다.
    #sound(name) { need(this.#hooks.sound, '메뉴 음향 경계 필요')(name); }

    // 표시 중 자식을 재귀 닫고 숨긴다(00476670). 숨김 자식의 동적 삭제도 수행하며 redraw 와 Kill 뒤 항목의 현재 flags 를 다시 읽는다.
    #closeChildren(token, clearSelection, active) {
        enter(active, token);
        // signed 항목 수가 0 이하이면 생략한다. 매 반복 끝에 콜백이 바꾼 현재 개수를 반영한다.
        for (let index = 0; index < (this.#node(token).itemCount | 0); index += 1) {
            const item = this.#item(token, index); if (clearSelection) item.flags = (item.flags & ~SELECTED) >>> 0;
            if (!(item.flags & CHILD)) continue;
            const child = this.#child(item.childId);
            if (this.#base(child).flags & VISIBLE) {
                this.#closeChildren(child, clearSelection, active); this.#redraw(child); this.#base(child).flags = (this.#base(child).flags & ~VISIBLE) >>> 0;
            }
            if (item.flags & DYNAMIC) { this.#lifetime.kill(child); item.childId = 0; item.flags = (item.flags & ~CHILD) >>> 0; }
        }
        active.pop();
    }

    // 순환 검사 상태를 호출 단위로 새로 만들며 정상 재귀에서 선택 인덱스는 보존한다. clearSelection 은 강조 비트만 지운다.
    close(token, clearSelection) { this.#closeChildren(token, clearSelection, []); }

    // 실제 등록표 루트의 sticky 팝업 중 비 dead 인 직계 자식에 Kill 을 요청한다(00466710). 루트 next 는 Kill 전에 저장하며 collection override 를 쓰지 않는다.
    closePopups() {
        const siblings = new Set();
        // 등록표 0번이 실제 루트이며 삭제 요청이 형제 링크를 바꾸어도 같은 방문 순서를 유지한다.
        for (let token = this.#base(this.#create.entries[0]).child; token;) {
            if (siblings.has(token)) throw new Error('메뉴 팝업 형제 순환'); siblings.add(token);
            const node = this.#base(token); const next = node.next;
            if ((node.flags & POPUP) && !(node.flags & DEAD)) this.#lifetime.kill(token);
            token = next;
        }
    }

    // visible 부모 확인 뒤 compact 전체/일반 행의 양수 변경 영역을 요청한다(00476e00). 높이/폭은 이미 배율 적용된 값이다.
    redrawItem(token, index) {
        if (!(this.#base(token).flags & VISIBLE)) return;
        const parents = new Set();
        // 현재 모든 부모의 visible 만 확인하며 dead/void 는 추가로 제외하지 않는다.
        for (let parent = this.#base(token).parent; parent; parent = this.#base(parent).parent) {
            if (parents.has(parent)) throw new Error('메뉴 행 부모 순환'); parents.add(parent); if (!(this.#base(parent).flags & VISIBLE)) return;
        }
        const point = this.#collection.position(token); let height = 0;
        if (this.#node(token).menuFlags & COMPACT) height = ftol(this.#node(token).height);
        else { this.#item(token, index); point.y = ftol(point.y + this.#node(token).offsets[index]); height = ftol(this.#node(token).heights[index]); }
        const width = ftol(this.#node(token).width); if (width <= 0 || height <= 0) return;
        need(this.#hooks.invalidate, '메뉴 행 변경 영역 경계 필요')({ left: point.x, top: point.y, right: add(point.x, width), bottom: add(point.y, height) });
    }

    // 마우스 이동의 선택 통지다(004772d0). 로컬 좌표/자식 메뉴 가림→행 선택→기존/새 강조와 행 또는 전체 Redraw 이며 선택 변경은 old/new 행 요청 사이에 현재 선택/항목을 다시 읽는다.
    broadcast(token, event, first, second) {
        const point = { x: first | 0, y: second | 0 };
        const local = this.#collection.screenToLocal(token, point); let selected = -1; const active = [];
        if (!(event & OUTSIDE) && this.#contains(token, local) && !this.#covered(token, point, active)) {
            if (!(this.#node(token).menuFlags & VERTICAL)) selected = ftol(local[0] / this.#node(token).itemWidth);
            else {
                selected = 0;
                if (!(this.#node(token).menuFlags & COMPACT)) {
                    // 원본은 마지막 항목까지 찾지 못하면 마지막 인덱스를 그대로 선택한다.
                    while (selected < add(this.#node(token).itemCount | 0, -1)) {
                        this.#item(token, selected);
                        if (local[1] >= this.#node(token).offsets[selected] && local[1] < this.#node(token).heights[selected] + this.#node(token).offsets[selected]) break;
                        selected += 1;
                    }
                }
            }
        }
        const old = this.#node(token).selection; if (selected === old) return;
        if (old >= 0) {
            if (this.#node(token).menuFlags & VERTICAL) this.redrawItem(token, old);
            const item = this.#item(token, this.#node(token).selection); item.flags = (item.flags & ~SELECTED) >>> 0; this.#node(token).selection = -1;
        }
        if (selected >= 0 && !(this.#item(token, selected).flags & DISABLED)) {
            if (this.#node(token).menuFlags & VERTICAL) this.redrawItem(token, selected);
            this.#node(token).selection = selected; const item = this.#item(token, selected); item.flags = (item.flags | SELECTED) >>> 0;
        }
        if (!(this.#node(token).menuFlags & VERTICAL)) this.#redraw(token);
    }

    // 클릭 입력이다(00476820). 확장 중/비누름 제외→가상 통지→명령/닫기→동적 자식/열기→부모 통지이며 명령 큐·소리·Close 가 바꾼 현재 선택은 후속 단계마다 다시 조회한다. 처리했으면 1 이다.
    handle(token, event, first, second) {
        event >>>= 0; const kind = (event & EVENT_MASK) >>> 0;
        if (this.#node(token).expanding || (kind !== LEFT && kind !== RIGHT) || (event & RELEASE)) return 0;
        need(this.#hooks.broadcast, '메뉴 가상 통지 경계 필요')(token, event, first, second);
        const selected = this.#node(token).selection;
        if (selected < 0) { if (this.#node(token).menuFlags & DELETE_OUTSIDE) this.#lifetime.kill(token); return 1; }
        let flags = this.#item(token, selected).flags;
        if (flags & COMMAND) {
            const parameter = this.#parameter(token, selected, flags); const command = this.#item(token, selected).command; const id = this.#base(token).id;
            need(this.#hooks.command, '메뉴 명령 큐 경계 필요')(command, id, parameter); this.#sound('openSubGump.wav');
        }
        this.close(token, (flags & (CHILD | KEEP)) === 0); flags = this.#item(token, this.#node(token).selection).flags;
        if ((flags & DYNAMIC) && !(flags & CHILD)) {
            const parameter = this.#parameter(token, this.#node(token).selection, flags); const serial = this.#base(token).serial; const eventCode = this.#state.requestEvent;
            const child = need(this.#hooks.handle, '메뉴 동적 자식 가상 입력 경계 필요')(token, eventCode, serial, parameter);
            if (child) {
                const childId = this.#base(child).id; this.#item(token, this.#node(token).selection).childId = childId; this.#base(child).flags = (this.#base(child).flags & ~VISIBLE) >>> 0;
                need(this.#hooks.attach, '메뉴 동적 자식 Attach 경계 필요')(child, 0, 0, token, 0);
                const item = this.#item(token, this.#node(token).selection); item.flags = (item.flags | CHILD) >>> 0; flags = item.flags;
            }
        }
        if (flags & CHILD) {
            const child = this.#child(this.#item(token, this.#node(token).selection).childId);
            need(this.#hooks.push, '메뉴 자식 Push 경계 필요')(child); this.#base(child).flags = (this.#base(child).flags | VISIBLE) >>> 0;
            const position = this.#menu.itemPosition(token, this.#node(token).selection);
            need(this.#hooks.attach, '메뉴 자식 Attach 경계 필요')(child, position[0], position[1], 0, 0);
            this.#tree.addDepth(child, 0xffffffff); this.#menu.startExpand(child, (this.#node(token).menuFlags & HORIZONTAL) ? VERTICAL : HORIZONTAL);
            this.#sound('openGump.wav'); this.#redraw(token);
        } else if (!(flags & KEEP)) {
            const parents = new Set(); let current = token; const identity = ((this.#state.alignment << 24) >> 24) >>> 0;
            // 메뉴 부모를 따라가며 루트 메뉴 경계에서 해당 가지의 자식들을 닫는다.
            while (current) {
                if (parents.has(current)) throw new Error('메뉴 닫기 부모 순환'); parents.add(current);
                const parent = this.#base(current).parent;
                if (this.#base(parent).command !== identity || (this.#node(current).menuFlags & ROOT_MENU)) { this.close(current, true); break; }
                current = parent;
            }
        }
        if (this.#node(token).menuFlags & DELETE_COMMAND) {
            if (!(flags & COMMAND)) return 1;
            this.#lifetime.kill(token); const serial = this.#base(token).serial; this.#events.bubble(token, this.#state.closedEvent, serial, serial);
        }
        if (flags & COMMAND) this.closePopups();
        return 1;
    }
}

// 두 입력 계층(사건/캡처)의 기존 반환 DWORD 를 보존하며 현재 메뉴에만 실제 handle 을 사용하고, 프레임 통지(broadcast)는 현재 메뉴만 교체한다. 등록표/visible 필터는 GumpFrame 에 맡긴다.
// 사용: bindMenuGumpInput(eventHooks, inputHooks, frameHooks, (token) => menus.get(token) ?? null)
export function bindMenuGumpInput(events, input, frame, resolve) {
    if (typeof resolve !== 'function') throw new TypeError('메뉴 입력 분배 조회 필요');
    // 기존 이벤트/캡처 처리기가 서로 다를 수 있으므로 각각 감싼다. 동적 하위 메뉴 요청도 같은 가상 경계로 상위 파생 처리기에 전달할 수 있다.
    const bind = (previous) => (token, event, first, second) => {
        const menu = resolve(token); if (menu) return menu.handle(token, event, first, second);
        if (typeof previous !== 'function') throw new TypeError('메뉴 외 입력 경계 필요'); return previous(token, event, first, second);
    };
    events.handle = bind(events.handle); input.handle = bind(input.handle); const previous = frame.broadcast;
    frame.broadcast = (token, event, first, second) => {
        const menu = resolve(token);
        if (menu) menu.broadcast(token, event, first, second);
        else { if (typeof previous !== 'function') throw new TypeError('메뉴 외 통지 경계 필요'); previous(token, event, first, second); }
    };
}

// 음향 파일 이름만 메뉴가 결정하며 채널/거리/우선순위는 원본처럼 모두 0 이다(openSubGump.wav/openGump.wav 를 기존 SoundPlayer 로 재생한다).
// 사용: bindMenuGumpInputSound(inputHooks, soundPlayer)
export function bindMenuGumpInputSound(hooks, sound) { hooks.sound = (name) => sound.playByName(name, 0, 0, 0, 0, 0); }
