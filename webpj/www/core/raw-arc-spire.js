// 아크 스파이어(149)의 원본 이웃 검색·연쇄 빔·Regular 와 최초 Regular 예약 후처리를 옮긴 모듈이다(cpppj `o/RawArcSpire.cpp/.h`, 10.78 전용).
//  - postPopArcSpire(pool, parent, flags, reserve, regular, damageable) [48c080]: 상태 비트 9 가 없고 flags 비트 1 이 있으며 reserve() 가 참일 때만 regular(parent, 0, 1.0), 이어 항상 공통 damageable(parent, flags).
//  - RawArcSpire.handle(parent, event, count, period) [48c6e0]: 상태 비트 0x20 이 없을 때 event0 이면서 count≠0 이면 attack, event1 이면 count 를 DWORD 로 올려 프레임 수보다 크면 0 을 돌려주고 아니면 (총 프레임 − count) 프레임으로 setFrame 한 뒤, 공통 damageable 의 float 반환을 돌려준다.
//  - RawArcSpire.attack(parent, period) [48c110]: 피해 = ftol(period × damageRate), 부모 좌표·범위로 +X/+Y 두 축만 검사한다. 표면 비트 1 이 있는 칸마다 collect 하고 첫 양방향 동맹을 찾으면 그 축을 끝낸 뒤, 각 축마다 별도로 strike 한다.
//  - RawArcSpire.collect(parent, buffer, count, x, y, type) [48c300]: find 가 돌려준 같은 위치의 이웃이 free/dead 가 아니고 양방향 동맹이며 현재 좌표가 정확히 (x, y) 일 때만 참(버퍼가 가득 차도 참)이다. {found, count} 를 돌려준다.
//  - RawArcSpire.strike(parent, neighbors, damage) [48c410]: 이웃마다 부모~이웃 사각형(−2, +0.99999 보정)을 iterate 로 훑어 적 후보 최대 40개를 모으고, 살아남은 후보를 이어 beam/damage 하며 마지막에 이웃으로 beam 한다.
// 하위 공간 검색(find/iterate)·메타(range/damageRate)·빔·피해·프레임·공통 Regular 는 훅으로 받으며, 도달한 지점에서만 요구한다(없으면 TypeError). 편집 모드 전역은 state.editor 로 호출마다 다시 읽는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E49: RawArcSpire 이식으로 추가)

// 슬롯 필드 위치(타입·상태·소유자·x·y)와 개수 상수이다. (2026-11-08 WB04-E49)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const FLAG_OFFSET = 40;
const OWNER_OFFSET = 34;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const TYPE_COUNT = 188;
const SPOT_COUNT = 65536;
const ALLIANCE_COUNT = 81;
const OWNER_COUNT = 9;
const MAP_SIDE = 256;
const BUFFER_CAPACITY = 20;
const VICTIM_CAPACITY = 40;

// 부모 상태 비트와 후보 타입 genus/메타 비트이다. (2026-11-08 WB04-E49)
const POPPED_MASK = 9;
const DEAD_FREE_MASK = 0x21;
const FREE_BIT = 1;
const REGULAR_STOP_BIT = 0x20;
const CARRY_MASK = 7;
const DAMAGEABLE_FLAG = 0x10;
const FORM_DAMAGE_GENUS = 0x400000;
const IMMUNE_GENUS = 0x120000;
const SURFACE_BIT = 1;

// 41d7a0 이 두 float 에 더하는 원본 502310 의 정확한 float 값(0.99999 에 가장 가까운 float)이다. (2026-11-08 WB04-E49)
const CEIL_BIAS = Math.fround(0.9999899864196777);

// 정렬되지 않은 float 4바이트를 읽는다(주변 필드는 포함하지 않는다). (2026-11-08 WB04-E49)
const floatView = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    floatView.setUint8(0, raw[offset]);
    floatView.setUint8(1, raw[offset + 1]);
    floatView.setUint8(2, raw[offset + 2]);
    floatView.setUint8(3, raw[offset + 3]);
    return floatView.getFloat32(0, true);
}

// 원본 CRT _ftol: 0 방향 절삭한 64비트 정수의 하위 32비트이며 비유한/범위 밖은 0 이다. (2026-11-08 WB04-E49)
function truncate(value) {
    if (!(value > -9223372036854775808 && value < 9223372036854775808)) return 0;
    return Math.trunc(value) | 0;
}

// 호출한 효과만 필수로 요구하며 빈 성공으로 대체하지 않는다. (2026-11-08 WB04-E49)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('아크 스파이어 실제 하위 미연결');
    return hook;
}

// 48c080: 최초 상태 비트 9 관문은 공통 후처리 전에 한 번만 읽는다. reserve/regular/damageable 은 도달한 지점에서만 요구한다. (2026-11-08 WB04-E49)
export function postPopArcSpire(pool, parent, flags, reserve, regular, damageable) {
    if (pool?.edition !== 'patch1078') throw new TypeError('아크 스파이어 후처리 판본');
    if (!(pool.slot(parent)[FLAG_OFFSET] & POPPED_MASK) && (flags & 1) && need(reserve)()) need(regular)(parent, 0, 1.0);
    need(damageable)(parent, flags);
}

// 아크 스파이어 실행기이다. 사용: new RawArcSpire(pool, types, frames, spots, alliances, state, hooks).
// pool 은 10.78 풀, types 는 188개 {flags1, flags2}, frames 는 188개 프레임 코드 배열(길이가 numFrames), spots 는 256×256 표면 바이트, alliances 는 방향 있는 81개 값, state.editor 는 편집 모드 전역(매번 읽음)이다.
// hooks = { range, damageRate, find, iterate, beam, damage, setFrame, damageable }. (2026-11-08 WB04-E49)
export class RawArcSpire {
    #pool;
    #types;
    #frames;
    #spots;
    #alliances;
    #state;
    #hooks;

    // 다른 판본/크기의 자료를 섞지 않도록 생성 때 확인한다(참조는 세션 저장소 그대로 유지). (2026-11-08 WB04-E49)
    constructor(pool, types, frames, spots, alliances, state, hooks) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || frames?.length !== TYPE_COUNT ||
            spots?.length !== SPOT_COUNT || alliances?.length !== ALLIANCE_COUNT || typeof state !== 'object' || state === null) {
            throw new TypeError('아크 스파이어 같은 원본 자료 크기/판본 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#spots = spots;
        this.#alliances = alliances;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 하위 효과 뒤 현재 부모 좌표를 다시 읽는다. (2026-11-08 WB04-E49)
    #position(sid) {
        const raw = this.#pool.slot(sid);
        return [coordinate(raw, X_OFFSET), coordinate(raw, Y_OFFSET)];
    }

    // 행 first, 열 second 의 방향을 보존한다(반대 방향 값으로 대체하지 않는다). (2026-11-08 WB04-E49)
    #allied(first, second) {
        if (first > OWNER_COUNT - 1 || second > OWNER_COUNT - 1) throw new RangeError('아크 스파이어 동맹 owner 범위');
        return this.#alliances[first * OWNER_COUNT + second] !== 0;
    }

    // 4ab7c0: 각 축을 1..255 로 제한한 포함 사각형의 표면 바이트 OR 이다(원본 y→x 순서). (2026-11-08 WB04-E49)
    #spotUnion(area) {
        const left = Math.min(Math.max(area.left, 1), 255);
        const right = Math.min(Math.max(area.right, 1), 255);
        const top = Math.min(Math.max(area.top, 1), 255);
        const bottom = Math.min(Math.max(area.bottom, 1), 255);
        let result = 0;
        // 위에서 아래로 한 줄씩 OR 한다(경계 두 칸도 포함).
        for (let y = top; y <= bottom; y += 1) {
            // 왼쪽에서 오른쪽으로 OR 한다.
            for (let x = left; x <= right; x += 1) result |= this.#spots[y * MAP_SIDE + x];
        }
        return result & 0xff;
    }

    // 48c300: buffer 가 가득 찼어도 유효한 첫 이웃이면 참이다. buffer 는 제자리 갱신하고 새 개수를 {found, count} 로 돌려준다. (2026-11-08 WB04-E49)
    collect(parent, buffer, count, x, y, type) {
        const pool = this.#pool;
        const other = need(this.#hooks.find)(Math.fround(x), Math.fround(y), type);
        if (!other) return { found: false, count };
        const raw = pool.slot(other);
        if ((raw[FLAG_OFFSET] & DEAD_FREE_MASK) || !this.#allied(pool.slot(parent)[OWNER_OFFSET], raw[OWNER_OFFSET]) ||
            !this.#allied(raw[OWNER_OFFSET], pool.slot(parent)[OWNER_OFFSET])) return { found: false, count };
        const point = this.#position(other);
        if (point[0] !== Math.fround(x) || point[1] !== Math.fround(y)) return { found: false, count };
        let next = count;
        if (next < buffer.length) {
            buffer[next] = other;
            next += 1;
        }
        return { found: true, count: next };
    }

    // 48c410: 적 후보는 먼저 모두 모으며(최대 40), 각 빔/피해 뒤 현재 상태·owner 를 다시 읽는다. neighbors 는 동맹 이웃 SID 목록이다. (2026-11-08 WB04-E49)
    strike(parent, neighbors, damage) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        // 각 동맹 이웃은 독립적인 40항목 적 목록을 갖는다.
        for (const other of neighbors) {
            const source = this.#position(parent);
            const target = this.#position(other);
            const lowX = Math.fround(source[0] - 2.0);
            const lowY = Math.fround(source[1] - 2.0);
            const area = { left: truncate(lowX), top: truncate(lowY), right: truncate(target[0] + CEIL_BIAS), bottom: truncate(target[1] + CEIL_BIAS) };
            const victims = [];
            // 4b16d0/4b1810 의 검색 순서대로 후보를 하나씩 받는다.
            need(hooks.iterate)(area, (candidate) => {
                const raw = pool.slot(candidate);
                if ((raw[FLAG_OFFSET] & 1) || candidate === parent || candidate === other || this.#allied(pool.slot(parent)[OWNER_OFFSET], raw[OWNER_OFFSET])) return;
                const type = this.#types[raw[TYPE_OFFSET]];
                if (type === undefined) throw new RangeError('아크 스파이어 후보 TYPE 범위 오류');
                const damageable = (type.flags2 & FORM_DAMAGE_GENUS) ? (raw[FLAG_OFFSET] & 1) !== 0 : (type.flags1 & DAMAGEABLE_FLAG) !== 0;
                if (damageable && !(this.#types[pool.slot(candidate)[TYPE_OFFSET]].flags2 & IMMUNE_GENUS) && victims.length < VICTIM_CAPACITY) victims.push(candidate);
            });
            let cursor = parent;
            // 살아남은 후보만 다음 빔의 출발점으로 사용한다.
            for (const candidate of victims) {
                need(hooks.beam)(cursor, candidate);
                if (!this.#state.editor) need(hooks.damage)(candidate, damage, pool.slot(parent)[OWNER_OFFSET], parent);
                if (!(pool.slot(candidate)[STATE_OFFSET] & CARRY_MASK)) cursor = candidate;
            }
            need(hooks.beam)(cursor, other);
        }
    }

    // 48c110: 원본은 양의 두 축만 검사하고 첫 양방향 동맹을 찾으면 해당 축을 끝낸다. period 는 float 이다. (2026-11-08 WB04-E49)
    attack(parent, period) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const damage = truncate(Math.fround(period) * Math.fround(need(hooks.damageRate)(parent)));
        const source = this.#position(parent);
        const x = truncate(source[0]);
        const y = truncate(source[1]);
        const range = truncate(Math.fround(need(hooks.range)(parent)));
        const endX = truncate(Math.min(source[0] + range, 254.0));
        const endY = truncate(Math.min(source[1] + range, 254.0));
        const buffer = new Array(BUFFER_CAPACITY).fill(0);
        let count = 0;
        // 가로 축: 표면 비트 1 이 있는 칸만 위치 검색을 실행한다.
        if (this.#spotUnion({ left: x, top: y, right: (endX - 1) | 0, bottom: y }) & SURFACE_BIT) {
            // 부모 오른쪽 칸부터 끝 직전까지 순서대로 훑는다.
            for (let at = x + 1; at < endX; at += 1) {
                if (at < 0 || at >= MAP_SIDE || y < 0 || y >= MAP_SIDE) throw new RangeError('아크 스파이어 가로 지도 범위');
                if (this.#spots[y * MAP_SIDE + at] & SURFACE_BIT) {
                    const found = this.collect(parent, buffer, count, at, y, pool.slot(parent)[TYPE_OFFSET]);
                    count = found.count;
                    if (found.found) break;
                }
            }
        }
        this.strike(parent, buffer.slice(0, count), damage);
        count = 0;
        // 세로 축: 앞선 가로 효과 뒤에도 현재 같은 지도/부모를 읽는다.
        if (this.#spotUnion({ left: x, top: y, right: x, bottom: (endY - 1) | 0 }) & SURFACE_BIT) {
            // 부모 아래 칸부터 끝 직전까지 순서대로 훑는다.
            for (let at = y + 1; at < endY; at += 1) {
                if (at < 0 || at >= MAP_SIDE || x < 0 || x >= MAP_SIDE) throw new RangeError('아크 스파이어 세로 지도 범위');
                if (this.#spots[at * MAP_SIDE + x] & SURFACE_BIT) {
                    const found = this.collect(parent, buffer, count, x, at, pool.slot(parent)[TYPE_OFFSET]);
                    count = found.count;
                    if (found.found) break;
                }
            }
        }
        this.strike(parent, buffer.slice(0, count), damage);
    }

    // 48c6e0: signed count 증가는 DWORD 감김 뒤 비교하며, event0 의 count0 은 공격하지 않는다. 반환은 공통 damageable 의 float 이다. (2026-11-08 WB04-E49)
    handle(parent, event, count, period) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        let current = count >>> 0;
        if (!(pool.slot(parent)[FLAG_OFFSET] & REGULAR_STOP_BIT)) {
            if (!event && current) {
                this.attack(parent, period);
            } else if (event === 1) {
                current = (current + 1) >>> 0;
                const codes = this.#frames[pool.slot(parent)[TYPE_OFFSET]];
                if (codes === undefined) throw new RangeError('아크 스파이어 프레임 TYPE 범위 오류');
                const total = codes.length | 0;
                if ((current | 0) > total) return 0;
                need(hooks.setFrame)(parent, (total - current) | 0, 0);
            }
        }
        return Math.fround(need(hooks.damageable)(parent, event, current, period));
    }
}
