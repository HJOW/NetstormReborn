// 기본 Gump Whole/Map 갱신 요청의 현재 가시성·좌표/크기·지도 제한 순서를 이식한다(cpppj client/GumpRedraw.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E414)

// 원문 등록 한도 안의 조상만 허용한다.
const PARENT_LIMIT = 1000;
// CRT64비트 절삭의 하위 signed DWORD다. 비유한/범위 밖의 불확정값은 하위0이다.
function ftol(value) { if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 원문445290은 역전 범위도 max(lower,min(value,upper)) 순서로 각 좌표를 제한한다.
export function clampOriginalScreenRect(rectangle, bounds) { const clamp = (value, low, high) => Math.max(low, Math.min(value, high)); return { left: clamp(rectangle.left, bounds.left, bounds.right), top: clamp(rectangle.top, bounds.top, bounds.bottom), right: clamp(rectangle.right, bounds.left, bounds.right), bottom: clamp(rectangle.bottom, bounds.top, bounds.bottom) }; }

// 같은 위치 수집기와 현재 노드/지도/Renderer dirty 장치를 빌린다.
export class GumpRedraw {
    // 현재 위치/캐시와 요청 경계다.
    #collection; #hooks;
    // 실제 요청 장치가 없는 생성은 선행 거부한다. 사용: new GumpRedraw(collection,{node,map,invalidate}).
    constructor(collection, hooks = {}) { if (['node', 'map', 'invalidate'].some((name) => typeof hooks[name] !== 'function')) throw new TypeError('Gump 갱신 현재 장치 누락'); this.#collection = collection; this.#hooks = hooks; }
    // 원문 필드 읽기 위치마다 같은 현재 저장소를 조회한다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node) throw new RangeError('Gump 갱신 노드 부재'); return node; }
    // 숨은 조상에서는 Position/캐시/Renderer에 도달하지 않는다. dead/void는 별도 필터가 아니다.
    #visible(token) {
        const seen = new Set();
        // 현재 bit1과 현재 parent를 순서대로 읽는다.
        while (true) { if (seen.size >= PARENT_LIMIT || seen.has(token)) throw new Error('Gump 갱신 조상 순환/한도 초과'); seen.add(token); const node = this.#node(token); if (!(node.flags & 1)) return false; if (!node.parent) return true; token = node.parent; }
    }
    // Position 뒤 현재 폭/높이를 각각 절삭하고 양수일 때 XYWH 끝점을 DWORD로 더한다.
    whole(token, argument = 0) { void argument; if (!this.#visible(token)) return; const point = this.#collection.position(token); const node = this.#node(token); const width = ftol(node.width); const height = ftol(node.height); if (width > 0 && height > 0) this.#hooks.invalidate({ left: point.x, top: point.y, right: (point.x + width) | 0, bottom: (point.y + height) | 0 }, 0); }
    // Position+실수 크기를 끝점에서 절삭한 Bounds 뒤 현재 map을 읽는다.
    map(token, argument = 0) { void argument; if (!this.#visible(token)) return; const bounds = this.#collection.bounds(token); const rectangle = clampOriginalScreenRect(bounds, this.#hooks.map()); if (rectangle.right !== rectangle.left && ((rectangle.right - rectangle.left) | 0) >= 0 && rectangle.bottom !== rectangle.top && ((rectangle.bottom - rectangle.top) | 0) >= 0) this.#hooks.invalidate(rectangle, 0); }
}
