// UI(Gump) 트리의 화면 위치/경계 계산과 표시 목록 수집을 옮긴 모듈이다(cpppj `client/GumpCollection.cpp/.h`, 10.78 전용. 원본 Gump.cpp 의 화면 위치·경계·트리 수집, CD 경로는 범위 밖).
// 링크/객체 포인터는 호스트 식별 토큰으로 대체하고, 노드 저장소와 월드→화면 투영은 호출자가 훅으로 공급한다(투영 자체와 SID 해석은 별도 범위).
//  - 노드 `{ flags, next, parent, child, object, depth, width, height, x, y, cached: { x, y } }`(flags 비트: visible 1·화면 밖 자식 통과 0x20·월드 부착 0x2000·삭제 대기 0x10000, 깊이/링크는 부호 없는 DWORD,
//    크기/위치는 float32 값, cached 는 일반 위치 조회만 갱신하고 월드 부착 자신은 보존한다). 훅: `node(token)` → 노드 객체(없으면 null/undefined → 거부), `project(object)` → `{ x, y }` 정수 화면 좌표.
//    상태: `{ root, overrideHead }` 를 호출 때마다 읽는다(overrideHead 가 0 이 아니면 root.child 대신 그 머리부터 수집).
//  - position(token): 월드 부착 노드는 투영점 + CRT 절삭한 로컬 위치(DWORD 덧셈)이다. 일반 노드는 부모 체인을 따라 각 단계마다 float32 로 저장하며 `부모 x + 자식 x` 를 더하고(월드 부착 부모는 투영점을 더함, 부모 자신의 캐시는 쓰지 않음)
//    마지막에 CRT 64비트 절삭의 하위 DWORD(signed)로 `cached` 를 갱신한다. bounds(token): 정수 위치에 배율 적용 크기를 더해 절삭한다(합산 직후 float 로 다시 반올림하지 않음).
//  - screenToLocal(token, point): 004650a0 의 정수 화면 좌표 → 로컬 float 좌표(각 뺄셈 뒤 float32 저장, 루트 자신의 위치 생략, 월드 부착 부모 이후 일반 부모 생략). 위치 캐시는 갱신하지 않는다. flags(token): 현재 raw 플래그.
//  - collect(view, output, limit): root.child(또는 overrideHead)의 형제를 next 순서로 훑되 다음 링크는 재귀 전에 저장하고, visible 이고 dead 가 아닌 최상위만 `collectNode` 한다. limit 은 각 최상위에 같은 값으로 전달해 전체 개수가 limit 을 넘을 수 있다.
//    collectNode(token, view, output, limit): 깊이 우선이며 현재 노드의 경계가 view 와 엄격히 교차하면 16바이트 항목(x·y 노드 위치, token, depth WORD, kind 2, tail 0)을 쓰고 남은 개수를 줄인다. 교차하지 않아도 0x20(통과) 플래그가 있으면 자식으로 내려간다.
//    자식은 visible 만 검사한다(dead 자식을 제외하는 필터는 원본에 없다). 남은 개수가 1 미만이면 0 이고, 출력 칸이 없는데 쓰려 하면 거부한다. 순환/손상된 링크·1000개를 넘는 경로는 거부한다.
//  - bindGumpCollection(hooks, collector): 공통 수집기 훅의 `ui` 를 이 수집기의 collect 로 연결한다(UI 전용 dirty 와 일반 병합이 같은 트리 수집기를 쓴다).
// 출력은 항목 n 개의 Uint8Array(16바이트×n)이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E395: 5-A UI 트리 수집기로 추가, 기준선 뒤 cpppj 변경 없음)
import { RENDER_ENTRY_BYTES } from '../core/raw-render-collection.js';

// 노드 플래그 비트: visible, 화면 밖 자식 통과, 월드 부착, 삭제 대기이다. (2026-11-13 WB04-E395)
const VISIBLE = 0x1;
const PASS_CHILDREN = 0x20;
const WORLD_ATTACHED = 0x2000;
const DEAD = 0x10000;
// 한 경로/형제 체인에서 허용하는 최대 노드 수(원본 MAX_GUMPS)이다. (2026-11-13 WB04-E395)
const MAX_GUMPS = 1000;
// 항목 안의 바이트 위치: x(0)·y(4)·token(8)·depth(12)·kind(14)·tail(15)이다. (2026-11-13 WB04-E395)
const TOKEN = 8;
const DEPTH = 12;
const KIND = 14;
const TAIL = 15;
// UI 표시 항목 종류(공통 표시 목록의 kind 2)이다. (2026-11-13 WB04-E395)
const KIND_UI = 2;

// CRT `_ftol` 의 64비트 절삭 뒤 하위 DWORD(signed)이다. 비유한/64비트 범위 밖은 원본 불확정 정수의 하위 0 이다. (2026-11-13 WB04-E395)
function ftol(number) {
    if (!Number.isFinite(number) || number < -(2 ** 63) || number >= 2 ** 63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(number))));
}

// 정수 화면 좌표 덧셈은 DWORD 넘침을 보존한다. (2026-11-13 WB04-E395)
const add = (first, second) => (first + second) | 0;

// 역전 사각형을 거부하고 맞닿은 가장자리는 제외한다. 내부의 폭/높이 0 은 원본처럼 허용한다. (2026-11-13 WB04-E395)
function intersects(a, b) {
    return a.left <= a.right && a.top <= a.bottom && b.left <= b.right && b.top <= b.bottom
        && a.left < b.right && b.left < a.right && a.top < b.bottom && b.top < a.bottom;
}

// 현재 재귀 경로/형제 체인의 손상이나 과도한 길이를 거부한다. 서로 다른 경로의 중복 토큰은 허용한다. (2026-11-13 WB04-E395)
function enter(seen, token) {
    if (seen.size >= MAX_GUMPS || seen.has(token)) throw new Error('Gump 링크 순환 또는 한도 초과');
    seen.add(token);
}

// UI 트리 수집기이다. 상태/노드 저장소는 수집기보다 오래 살아야 한다. 사용: const collector = new GumpCollection(state, { node, project }); collector.collect(view, output, limit). (2026-11-13 WB04-E395)
export class GumpCollection {
    // 루트/override 상태(호출 시 읽음)와 저장소·투영 훅이다.
    #state;
    #hooks;

    // 조회 훅이 없으면 거부한다.
    constructor(state, hooks) {
        if (!hooks || typeof hooks.node !== 'function') throw new TypeError('Gump 저장소 조회 필요');
        this.#state = state;
        this.#hooks = hooks;
    }

    // 없는 토큰/저장소는 거부한다(원본의 유효 포인터 접근에 대응하는 호스트 계약).
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node) throw new Error('Gump 토큰/저장소 없음');
        return node;
    }

    // 월드 부착은 반드시 root 직계 자식이다. 투영 이후 로컬 위치를 절삭해 DWORD 덧셈한다.
    #anchor(node) {
        if (node.parent !== this.#state.root || typeof this.#hooks.project !== 'function') throw new Error('월드 부착 Gump의 루트/투영 필요');
        const point = this.#hooks.project(node.object);
        return { x: add(point.x, ftol(node.x)), y: add(point.y, ftol(node.y)) };
    }

    // 부모별 float32 저장 후 CRT 64비트 절삭의 하위 DWORD 를 돌려주고 일반 노드 캐시를 갱신한다(월드 부착 자신은 갱신하지 않음).
    position(token) {
        const node = this.#node(token);
        if (node.flags & WORLD_ATTACHED) return this.#anchor(node);
        let x = node.x;
        let y = node.y;
        const seen = new Set();
        enter(seen, token);
        // 각 부모의 덧셈 직후 float32 로 저장하며 부모 자신의 캐시는 사용하지 않는다.
        for (let parent = node.parent; parent; parent = this.#node(parent).parent) {
            enter(seen, parent);
            const above = this.#node(parent);
            if (above.flags & WORLD_ATTACHED) {
                const point = this.#anchor(above);
                x = Math.fround(point.x + x);
                y = Math.fround(point.y + y);
            } else {
                x = Math.fround(above.x + x);
                y = Math.fround(above.y + y);
            }
        }
        node.cached = { x: ftol(x), y: ftol(y) };
        return { ...node.cached };
    }

    // 정수 위치에 배율 적용 크기를 더해 절삭한다. 합산 직후 float 로 다시 반올림하지 않는다.
    bounds(token) {
        const point = this.position(token);
        const node = this.#node(token);
        return { left: point.x, top: point.y, right: ftol(point.x + node.width), bottom: ftol(point.y + node.height) };
    }

    // 004650a0: 사건의 정수 화면 좌표를 로컬 float 좌표로 변환한다. 각 뺄셈 뒤 float32 저장, 루트 자신의 위치 생략, 월드 부착 부모 이후 일반 부모 생략. 위치 캐시는 갱신하지 않는다.
    screenToLocal(token, point) {
        const local = [Math.fround(point.x), Math.fround(point.y)];
        let anchored = false;
        const seen = new Set();
        let current = token;
        // 루트 자신의 위치는 원본처럼 생략한다. 각 단계는 현재 부모/위치를 읽고 float32 로 저장한다.
        for (;;) {
            enter(seen, current);
            const node = this.#node(current);
            if (!node.parent) return local;
            const parent = this.#node(node.parent);
            if (parent.flags & WORLD_ATTACHED) {
                const anchor = this.#anchor(parent);
                local[0] = Math.fround(local[0] - anchor.x);
                local[1] = Math.fround(local[1] - anchor.y);
                anchored = true;
            } else if (!anchored) {
                local[0] = Math.fround(local[0] - node.x);
                local[1] = Math.fround(local[1] - node.y);
            }
            current = node.parent;
        }
    }

    // Draw 직전에 같은 저장소의 현재 raw 플래그를 읽는다. visible/dead 필터는 수집기가 담당한다.
    flags(token) {
        return this.#node(token).flags;
    }

    // 지정 노드의 visible/dead 비트를 직접 검사하지 않고 깊이 우선 수집한다. 자식에는 visible 만 검사한다. 반환: 기록한 항목 수.
    collectNode(token, view, output, limit) {
        const active = new Set();
        const data = new DataView(output.buffer, output.byteOffset, output.length);
        let written = 0;
        const capacity = Math.floor(output.length / RENDER_ENTRY_BYTES);
        // 재귀 호출마다 남은 개수와 출력 위치를 이어 간다. 자식 호출이 링크를 바꾸면 현재 next 를 읽는다.
        const collect = (current, available) => {
            if (available < 1) return 0;
            enter(active, current);
            const visible = intersects(this.bounds(current), view);
            const node = this.#node(current);
            if (!visible && !(node.flags & PASS_CHILDREN)) {
                active.delete(current);
                return 0;
            }
            let count = 0;
            let remaining = available;
            if (visible) {
                if (written >= capacity) throw new RangeError('Gump 표시 출력 용량 부족');
                const base = written * RENDER_ENTRY_BYTES;
                data.setFloat32(base, node.x, true);
                data.setFloat32(base + 4, node.y, true);
                data.setUint32(base + TOKEN, current >>> 0, true);
                data.setUint16(base + DEPTH, node.depth & 0xffff, true);
                output[base + KIND] = KIND_UI;
                output[base + TAIL] = 0;
                written += 1;
                count = 1;
                remaining -= 1;
            }
            const siblings = new Set();
            // 자식은 visible 만 검사한다. dead 자식을 제외하는 필터는 원본에 없다.
            for (let child = node.child; child; child = this.#node(child).next) {
                enter(siblings, child);
                if (this.#node(child).flags & VISIBLE) {
                    const added = collect(child, remaining);
                    remaining -= added;
                    count += added;
                }
            }
            active.delete(current);
            return count;
        };
        return collect(token, limit);
    }

    // 원본의 root.child/override 순서로 수집한다. limit 은 각 최상위 노드에 같은 값으로 전달해 전체 반환 수가 limit 을 넘을 수 있다. 출력 크기를 넘기는 쓰기는 거부한다.
    collect(view, output, limit) {
        let head = this.#node(this.#state.root).child;
        if (this.#state.overrideHead) head = this.#state.overrideHead;
        let count = 0;
        const siblings = new Set();
        // 최상위 next 는 재귀 전에 저장한다. limit 은 각 루트에서 줄이지 않는 원본의 동작이다.
        while (head) {
            enter(siblings, head);
            const current = head;
            const node = this.#node(current);
            head = node.next;
            if ((node.flags & VISIBLE) && !(node.flags & DEAD)) {
                const added = this.collectNode(current, view, output.subarray(count * RENDER_ENTRY_BYTES), limit);
                if (added > 2147483647 - count) throw new RangeError('Gump 표시 개수 넘침');
                count += added;
            }
        }
        return count;
    }
}

// 공통 UI 슬롯에 실제 트리 수집을 연결한다. UI 전용 dirty 와 일반 병합이 모두 같은 트리 수집기를 쓴다. 사용: bindGumpCollection(collectionHooks, collector). (2026-11-13 WB04-E395)
export function bindGumpCollection(hooks, collector) {
    hooks.ui = (view, output, limit) => collector.collect(view, output, limit);
}
