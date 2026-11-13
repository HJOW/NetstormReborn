// 같은 생성/수명/표시 저장소의 커서 선택·재귀 이벤트·부모 전파를 이식한다(cpppj client/GumpEvents.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E413)
// 사용: new GumpEvents(create,life,state,collection,{node,filter?,handle?}). 표시 정렬과 이벤트 선택 순서는 서로 다르다.

// 원본 선택/전달 플래그와 사건 마스크다.
const VISIBLE = 1; const PASS_CHILDREN = 0x20; const VOID = 8; const DEAD = 0x10000; const HELP = 0x200; const KEYBOARD = 0x400; const MOUSE = 0x800; const EVENT_MASK = 0x03ffffff; const MOUSE_EVENT = 0x02000000;
// 사각형의 오른쪽/아래 끝은 포함하지 않는다.
const contains = (rect, point) => rect.left <= point.x && point.x < rect.right && rect.top <= point.y && point.y < rect.bottom;
// 현재 경로 안의 순환만 거부하며 정상 반복 방문을 제거하지 않는다.
function enter(active, token) { if (active.includes(token)) throw new Error('Gump 이벤트 링크 순환'); active.push(token); }

// 전체 트리 선택의 같은 현재 좌표/후보 마스크/최소 signed 깊이/선택 토큰이다.
export class GumpEventState {
    // point는 장기 주소를 유지한다. pick이 필드만 초기화한다.
    point = { x: 0, y: 0 };
    // 전체DWORD 후보 마스크·깊이 비트·선택 토큰이다.
    mask = 0; bestDepth = 0; selected = 0;
}

// 현재 노드/전역을 참조하며 가상 필터/처리기는 전달 때만 선택한다.
export class GumpEvents {
    // 같은 생성·수명·선택·위치와 현재 가상 경계다.
    #create; #life; #state; #collection; #hooks;
    // 노드 조회만 생성 시 필수다. 기본 필터는 선택적 filter를 대신한다.
    constructor(create, lifetime, state, collection, hooks = {}) { if (typeof hooks.node !== 'function') throw new TypeError('Gump 이벤트 저장소 필요'); this.#create = create; this.#life = lifetime; this.#state = state; this.#collection = collection; this.#hooks = hooks; }
    // 처리 동안 안정된 같은 노드를 읽는다. 삭제는 지연 수명이 맡는다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node) throw new Error('Gump 이벤트 토큰 없음'); return node; }
    // 현재 dialog는 모든 사건을 받는다. help 경계 실패를 다른 마우스/키보드 플래그로 통과시키지 않는다.
    baseFilter(token, event, first, second) {
        if (token === this.#life.currentDialog) return true; const flags = this.#node(token).flags; const kind = event & EVENT_MASK;
        if ((flags & HELP) && (kind === 0x10000 || kind === 0x20000)) return contains(this.#collection.bounds(token), { x: first | 0, y: second | 0 });
        return !!(((flags & KEYBOARD) && !(event & MOUSE_EVENT) && kind !== 0x10000 && kind !== 0x20000) || ((flags & MOUSE) && (event & MOUSE_EVENT)));
    }
    // 루트 직계만 고른다. Bounds를 flags보다 먼저 읽고 같은 깊이는 첫 방문을 보존한다.
    pickTopLevel(point) {
        point = { x: point.x | 0, y: point.y | 0 }; let selected = 0; let depth = 1000; const siblings = [];
        // 현재 next/깊이를 매 방문에서 읽으며 void는 제외하지 않는다.
        for (let child = this.#node(this.#create.entries[0]).child; child; child = this.#node(child).next) { enter(siblings, child); const bounds = this.#collection.bounds(child); const node = this.#node(child); if (contains(bounds, point) && (node.depth | 0) < depth && (node.flags & VISIBLE) && !(node.flags & DEAD)) { depth = node.depth | 0; selected = child; } }
        return selected;
    }
    // 모든 선택 전역을 초기화하고 실제 위치 수집기를 사용해 전체 자식을 탐색한다.
    pick(point, mask) { const x = point.x | 0; const y = point.y | 0; Object.assign(this.#state.point, { x, y }); this.#state.mask = mask >>> 0; this.#state.bestDepth = 10000; this.#state.selected = 0; this.#pickChildren(this.#create.entries[0], []); return this.#state.selected; }
    // 재귀 뒤 현재 next와 현재 선택 전역을 읽는다. mask는 재귀 진입을 막지 않는다.
    #pickChildren(token, active) {
        enter(active, token); const siblings = [];
        // 화면 밖 통과도 visible·void/dead 검사를 생략하지 않는다.
        for (let child = this.#node(token).child; child; child = this.#node(child).next) { enter(siblings, child); const bounds = this.#collection.bounds(child); const inside = contains(bounds, this.#state.point); const node = this.#node(child);
            if (((node.flags & PASS_CHILDREN) || inside) && (node.flags & VISIBLE) && !(node.flags & (DEAD | VOID))) { if (inside && (node.depth | 0) < (this.#state.bestDepth | 0) && (!this.#state.mask || (node.flags & this.#state.mask))) { this.#state.bestDepth = node.depth; this.#state.selected = child; } this.#pickChildren(child, active); }
        }
        active.pop();
    }
    // 전달할 때만 처리기를 요구하며 성공한 처리기 값 대신 대상 토큰을 반환한다.
    dispatch(token, event, first, second, includeHidden = false) { if (typeof this.#hooks.handle !== 'function') throw new TypeError('Gump 이벤트 처리기 필요'); return this.#deliver(token, event, first, second, includeHidden, []); }
    // 자식 우선 전달은 visible만 제한한다. dead/void를 추가로 걸러내지 않는다.
    #deliver(token, event, first, second, includeHidden, active) {
        enter(active, token); const siblings = [];
        // 자식 성공은 즉시 반환하고 실패한 자식 뒤의 현재 next로 이동한다.
        for (let child = this.#node(token).child; child; child = this.#node(child).next) { enter(siblings, child); if ((this.#node(child).flags & VISIBLE) || includeHidden) { const result = this.#deliver(child, event, first, second, includeHidden, active); if (result) { active.pop(); return result; } } }
        const accepts = this.#hooks.filter ? this.#hooks.filter(token, event, first, second) : this.baseFilter(token, event, first, second); const result = accepts && this.#hooks.handle(token, event, first, second) ? token : 0; active.pop(); return result;
    }
    // 자신을 제외한 현재 부모 체인에 직접 전달하며 첫 비0반환의 전체DWORD를 보존한다.
    bubble(token, event, first, second) {
        if (typeof this.#hooks.handle !== 'function') throw new TypeError('Gump 이벤트 처리기 필요'); const seen = [];
        // 기본 필터는 호출하지 않는다. 실패한 부모가 바꾼 현재 부모 링크를 다음에 읽는다.
        for (let parent = this.#node(token).parent; parent; parent = this.#node(parent).parent) { enter(seen, parent); const result = this.#hooks.handle(parent, event, first, second) >>> 0; if (result) return result; }
        return 0;
    }
}
