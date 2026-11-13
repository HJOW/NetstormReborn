// 같은 생성/수명 저장소의 통지·idle·재그리기 요청을 이식한다(cpppj client/GumpFrame.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E413)
// 사용: new GumpFrame(create,life,{node,idle?,redraw?,broadcast?}). 실제 픽셀 Draw/OS 프레임은 별도 상위 연결이다.
import { GUMP_REGISTRY_CAPACITY } from './gump-create.js';

// 원문 통지/루트 필터와 자식 전파/재그리기 생략 비트다.
const VISIBLE = 1; const DEAD = 0x10000; const NOTIFY = 0x100; const CHILDREN = 0x20; const SKIP_REDRAW = 0x42000;
// 같은 형제 체인의 순환/원본 한도만 검사한다.
function enter(seen, token) { if (seen.size >= GUMP_REGISTRY_CAPACITY || seen.has(token)) throw new Error('Gump 프레임 형제 순환/한도 초과'); seen.add(token); }

// 현재 등록표·idle와 실제 가상 요청 장치를 빌린다.
export class GumpFrame {
    // 같은 장기 생성·수명 상태와 현재 가상 경계다.
    #create; #life; #hooks;
    // 노드만 선행 필수다. 가상 함수는 도달한 때에만 요구한다.
    constructor(create, lifetime, hooks = {}) { if (typeof hooks.node !== 'function') throw new TypeError('Gump 프레임 저장소 필요'); this.#create = create; this.#life = lifetime; this.#hooks = hooks; }
    // 실제 필드를 읽을 토큰만 검사한다.0은 노드로 조회하지 않는다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node) throw new Error('Gump 프레임 토큰/저장소 없음'); return node; }
    // 루트 밖 등록표의 현재 signed count에 도달할 때 종료한다. 필터 실패도 비0칸 수에 포함한다.
    broadcast(event, first, second) {
        let encountered = 0;
        // 콜백 뒤 현재 count와 다음 등록 칸을 다시 읽는다.
        for (let i = 1; i < this.#create.entries.length; i += 1) { if (encountered >= (this.#create.count | 0)) return; const token = this.#create.entries[i]; if (!token) continue; const flags = this.#node(token).flags; encountered += 1;
            if ((flags & (VISIBLE | NOTIFY)) === (VISIBLE | NOTIFY) && !(flags & DEAD)) { if (typeof this.#hooks.broadcast !== 'function') throw new TypeError('Gump 가상 통지 경계 필요'); this.#hooks.broadcast(token, event, first, second); }
        }
    }
    // 실제 패치500칸을 현재 순서로 호출한다. flags/부모 visible을 재검사하지 않는다.
    idle() {
        // 이미 방문한 칸을 콜백이 채워도 뒤로 돌아가지 않는다.
        for (let i = 0; i < this.#life.idle.length; i += 1) { const token = this.#life.idle[i]; if (!token) continue; this.#node(token); if (typeof this.#hooks.idle !== 'function') throw new TypeError('Gump 가상 idle 경계 필요'); this.#hooks.idle(token); }
    }
    // 각 공개 재귀 진입은 독립 경로를 쓴다.
    redrawNode(token) { this.#redraw(token, []); }
    // redraw 콜백 뒤 현재 자식 전파 비트와 재귀 뒤 현재 next를 읽는다.
    #redraw(token, active) {
        const node = this.#node(token); if (active.length >= GUMP_REGISTRY_CAPACITY || active.includes(token)) throw new Error('Gump 프레임 자식 순환/한도 초과'); active.push(token);
        if (!(node.flags & SKIP_REDRAW)) { if (typeof this.#hooks.redraw !== 'function') throw new TypeError('Gump 가상 재그리기 경계 필요'); this.#hooks.redraw(token, false); }
        if (node.flags & CHILDREN) { const seen = new Set();
            // 하위는 visible만 검사한다. dead/void 자식도 visible이면 전달한다.
            for (let child = node.child; child; child = this.#node(child).next) { enter(seen, child); if (this.#node(child).flags & VISIBLE) this.#redraw(child, active); }
        }
        active.pop();
    }
    // 실제 루트 자식의 next는 콜백 전에 저장한다. override/루트 visible은 쓰지 않는다.
    redrawRoots() {
        const seen = new Set();
        // 최상위 visible/비dead 필터와 하위의 다른 필터를 구별한다.
        for (let token = this.#node(this.#create.entries[0]).child; token;) { enter(seen, token); const node = this.#node(token); const flags = node.flags; const next = node.next; if ((flags & VISIBLE) && !(flags & DEAD)) this.redrawNode(token); token = next; }
    }
}
