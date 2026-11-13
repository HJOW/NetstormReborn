// 같은 생성/수명/이벤트 저장소의 캡처·dialog 우선 전달과 overlay 선택을 이식한다(cpppj client/GumpInput.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E413)
// 사용: new GumpInput(create,life,state,collection,events,{node,handle?,overflow?}). OS 입력 큐는 별도 상위 연결이다.

// 후보 visible/dead와 dialog 한도 로그의 원문 코드다.
const VISIBLE = 1; const DEAD = 0x10000; const DIALOG_OVERFLOW = 0x13;

// overlay의 별도 원문 사건 전역이며 호출자 event 인자와 구별한다.
export class GumpInputState {
    // 가상 overlay 처리기에 전달할 현재 전체DWORD 사건이다.
    overlayEvent = 0;
}

// 같은 실제 선택/재귀 전달기를 호출하며 별도 UI 트리나 경계 사본을 만들지 않는다.
export class GumpInput {
    // 장기 생성/수명/입력/위치/이벤트와 현재 가상 장치다.
    #create; #life; #state; #collection; #events; #hooks;
    // 노드 조회만 필수이며 직접 처리기/한도 로그는 도달할 때 요구한다.
    constructor(create, lifetime, state, collection, events, hooks = {}) { if (typeof hooks.node !== 'function') throw new TypeError('Gump 입력 저장소 필요'); this.#create = create; this.#life = lifetime; this.#state = state; this.#collection = collection; this.#events = events; this.#hooks = hooks; }
    // 유효한 같은 현재 노드만 조회한다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node) throw new Error('Gump 입력 토큰 없음'); return node; }
    // 직접 캡처/overlay 전달에는 기본 필터를 넣지 않는다.
    #handle(token, event, first, second) { this.#node(token); if (typeof this.#hooks.handle !== 'function') throw new TypeError('Gump 입력 처리기 필요'); return this.#hooks.handle(token, event, first, second) >>> 0; }
    // 첫 캡처만 유지하며0도 원문처럼 대입할 수 있다.
    capture(token) { if (!this.#life.focus) this.#life.focus = token >>> 0; }
    // enabled와 빈 override가 모두 맞을 때만 설정하고 나머지는 현재 값을 해제한다.
    setOverride(token, enabled) { this.#create.collection.overrideHead = enabled && !this.#create.collection.overrideHead ? token >>> 0 : 0; }
    // 가득 찬 경우 중복 검색보다 먼저 원문 한도 로그를 호출한다.
    addDialog(token) {
        if ((this.#life.dialogCount | 0) < 0) throw new RangeError('Gump dialog 개수 음수'); if (this.#life.dialogCount >= this.#life.dialogs.length) { if (typeof this.#hooks.overflow !== 'function') throw new TypeError('Gump dialog 한도 로그 필요'); this.#hooks.overflow(DIALOG_OVERFLOW); return false; }
        // 활성 구간의 중복만 검사하고 미사용 뒤 칸의 같은 값은 무시한다.
        for (let i = 0; i < this.#life.dialogCount; i += 1) if (this.#life.dialogs[i] === token) return false;
        this.#life.dialogs[this.#life.dialogCount] = token; this.#life.dialogCount = (this.#life.dialogCount + 1) >>> 0; this.#life.currentDialog = token; return true;
    }
    // 캡처 실패는 재전달하지 않는다. 성공하면 콜백 뒤 현재 캡처를 읽고 지운다.
    route(event, first, second, includeHidden = false) { if (this.#life.focus) { if (this.#handle(this.#life.focus, event, first, second)) { const result = this.#life.focus; this.#life.focus = 0; return result; } return 0; } const target = this.#life.currentDialog || this.#create.entries[0]; return target ? this.#events.dispatch(target, event, first, second, includeHidden) : 0; }
    // 자신과 모든 부모의 visible만 검사한다. dead/void를 여기서 제외하지 않는다.
    #parentsVisible(token) {
        const seen = new Set();
        // 현재 부모 링크를 읽으며 순환만 거부한다.
        for (let current = token; current; current = this.#node(current).parent) { if (seen.has(current)) throw new Error('Gump overlay 부모 순환'); seen.add(current); if (!(this.#node(current).flags & VISIBLE)) return false; }
        return true;
    }
    // 실제 첫 후보의 처리기 반환이0이어도 종료한다. 원문처럼 별도 overlayEvent 전역을 쓴다.
    overlay(event, point) {
        void event; point = { x: point.x | 0, y: point.y | 0 };
        // 앞 대상의 위치/선택 콜백 뒤 현재 뒤 칸을 읽는다.
        for (const token of this.#life.overlay) { if (!token) continue; const flags = this.#node(token).flags; if (!(flags & VISIBLE) || (flags & DEAD) || !this.#parentsVisible(token)) continue; const rect = this.#collection.bounds(token);
            if (rect.left <= point.x && point.x < rect.right && rect.top <= point.y && point.y < rect.bottom && this.#events.pick(point, 0) === token) return this.#handle(token, this.#state.overlayEvent, point.x >>> 0, point.y >>> 0);
        }
        return 0;
    }
}
