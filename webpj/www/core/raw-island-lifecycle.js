// 섬 받침의 preDestroy 재정의(00442240)와 noIsland 의 postDestroy 재정의(00442640)를 raw SID 풀 위에서 옮긴 모듈이다(cpppj `o/RawIslandLifecycle.cpp/.h`, 10.78 전용).
//  - islandPreDestroy(island, flags)(00442240, 섬 받침 vtable +0x14): 받침이 abstract/buried(extra 9)가 아니면 받침의 중심을 탐색기 생성 전에 한 번 구하고 flag 0 연결 이웃을 순회한다. flags 에 0x1000 이 없을 때,
//    죽지 않은(state 비트 2 없음) 다리 이웃(genus bridge)마다 받침의 중심 좌표로 지연 낙하를 예약한다(섬이 사라지면 닿아 있던 다리가 끝 칸으로 바뀐다). 탐색기는 0x1000 이 있어도 만들고 끝까지 순회하며
//    예약 훅이 풀을 바꿀 수 있으므로 다음 이웃은 그 뒤의 상태에서 읽는다. 그 뒤 항상 공통 preDestroy 를 부른다.
//  - surfacePostDestroy(surface, flags)(00442640, noIsland vtable +0x18): 표면 칸이 abstract/buried 가 아니면 그 칸(x,y)에 걸린 객체를 일반 탐색(해시 0단계 건너뜀)으로 훑어 walker genus 마다 가상 낙하를 부른다(낙하 훅이 바꾼 체인/타입을 매번 다시 읽음).
//    그 뒤 항상 공통 postDestroy 를 부른다.
// 지연 낙하 예약·walker 낙하·공통 pre/postDestroy 는 호출자가 연결하고 순서·인자만 원본대로 호출한다(모두 필수). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E224: 섬 받침 삭제 훅 이식으로 추가)

import { TypeFlag2 } from '../assets/type-flags.js';
import { RawSquidFinder } from './raw-squid-finder.js';
import { RawSquidNeighborWalk } from './raw-squid-neighbors.js';

// 섬 받침 preDestroy 가 둘레 다리의 지연 낙하 예약을 건너뛰는 삭제 flags 비트이다(원본 `flags & 0x1000`). (2026-11-10 WB04-E224)
export const ISLAND_DESTROY_KEEPS_BRIDGES = 0x1000;

// 공통 raw 필드(타입 +10, 상태 +11, 위치 float x +14·y +18, extra +40)와 dead/abstract-buried 마스크, 한 칸 탐색기의 해시 0단계 건너뛰기 flag 이다. (2026-11-10 WB04-E224)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const EXTRA_OFFSET = 40;
const DEAD = 2;
const ABSTRACT_OR_BURIED = 9;
const SKIP_SURFACE_LEVEL = 1;

// raw 좌표 float 의 비트를 그대로 읽는다. (2026-11-10 WB04-E224)
const floatView = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    floatView.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return floatView.getFloat32(0, true);
}

// 0041d770 으로 자른 값을 다시 _ftol 로 읽은 칸 번호이다. 비유한/64비트 밖은 원본 정수 불확정값의 하위 0 이다. (2026-11-10 WB04-E224)
function cell(value) {
    if (!(value > -9223372036854775808 && value < 9223372036854775808)) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 섬 삭제 훅이다. pool 은 10.78 SidPool, neighbors 는 같은 풀의 RawSquidNeighbors, hooks = { scheduleFall(bridge, x, y), fallWalker(walker), basePreDestroy(sid, flags), basePostDestroy(sid, flags) } 가 모두 필요하다.
// 사용: new RawIslandLifecycle(pool, neighbors, hooks).islandPreDestroy(island, 0). (2026-11-10 WB04-E224)
export class RawIslandLifecycle {
    // 풀·이웃 탐색기·외부 효과이다. 풀과 이웃 탐색기는 이 객체보다 오래 살아야 하고 같은 풀을 읽어야 한다.
    #pool;
    #neighbors;
    #hooks;

    // 외부 효과는 빠짐없이 연결되어야 하고 이웃 탐색기는 같은 풀을 읽어야 한다. (2026-11-10 WB04-E224)
    constructor(pool, neighbors, hooks = {}) {
        if (pool?.edition !== 'patch1078' || neighbors?.pool !== pool) throw new TypeError('섬 삭제 훅의 이웃 탐색기 풀이 다르다');
        for (const name of ['scheduleFall', 'fallWalker', 'basePreDestroy', 'basePostDestroy']) {
            if (typeof hooks[name] !== 'function') throw new TypeError('섬 삭제 훅이 모두 연결되어야 한다');
        }
        this.#pool = pool;
        this.#neighbors = neighbors;
        this.#hooks = hooks;
    }

    // 004ac200: 객체 타입의 genus(flags2)이다. 타입 번호가 표 밖이면 원본은 표 뒤의 메모리를 읽지만 여기서는 거부한다. (2026-11-10 WB04-E224)
    #genus(sid) {
        const types = this.#neighbors.types();
        const number = this.#pool.slot(sid)[TYPE_OFFSET];
        if (number >= types.length) throw new RangeError('섬 삭제 훅 타입 범위 오류');
        return types[number].flags2;
    }

    // 00442240: 위 설명의 섬 받침 preDestroy 이다. (2026-11-10 WB04-E224)
    islandPreDestroy(island, flags) {
        if ((this.#pool.slot(island)[EXTRA_OFFSET] & ABSTRACT_OR_BURIED) === 0) {
            // 중심은 탐색기를 만들기 전에 한 번 구한다. 탐색기는 0x1000 이 있어도 만들고 끝까지 순회한다.
            const center = this.#neighbors.center(island);
            const keepBridges = (flags & ISLAND_DESTROY_KEEPS_BRIDGES) !== 0;
            const walk = new RawSquidNeighborWalk(this.#neighbors, island, 0);
            // 예약 훅이 풀을 바꿀 수 있으므로 다음 이웃은 그 뒤의 상태에서 읽는다.
            for (let other = walk.current; other !== 0; other = walk.next()) {
                if (!keepBridges && (this.#genus(other) & TypeFlag2.bridge) !== 0 && (this.#pool.slot(other)[STATE_OFFSET] & DEAD) === 0) this.#hooks.scheduleFall(other, center[0], center[1]);
            }
        }
        this.#hooks.basePreDestroy(island, flags);
    }

    // 00442640: 위 설명의 noIsland postDestroy 이다. (2026-11-10 WB04-E224)
    surfacePostDestroy(surface, flags) {
        if ((this.#pool.slot(surface)[EXTRA_OFFSET] & ABSTRACT_OR_BURIED) === 0) {
            const raw = this.#pool.slot(surface);
            const x = cell(coordinate(raw, X_OFFSET));
            const y = cell(coordinate(raw, Y_OFFSET));
            const finder = new RawSquidFinder(this.#pool, this.#neighbors.hash, this.#neighbors.types());
            // 그 칸에 걸린 객체를 일반 탐색 순서로 훑는다. 낙하 훅이 바꾼 뒤의 체인/타입을 매번 다시 읽는다.
            for (let current = finder.begin({ left: x, top: y, right: x, bottom: y }, SKIP_SURFACE_LEVEL); current !== 0; current = finder.next()) {
                if ((this.#genus(current) & TypeFlag2.walker) !== 0) this.#hooks.fallWalker(current);
            }
        }
        this.#hooks.basePostDestroy(surface, flags);
    }
}
