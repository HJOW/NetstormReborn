// 애시드(145)의 전용 주기·양축 이웃·13개 높이 영역 포획을 옮긴 모듈이다(cpppj `o/RawAcidFence.cpp/.h`, 10.78 전용).
//  - postPopAcidFence(pool, parent, flags, reserve, regular, damageable) [48c7a0]: 상태 비트 9 가 없고 flags 비트 1 이며 reserve() 가 참일 때만 regular(parent, 0, 0.3) 뒤 항상 공통 damageable(parent, flags).
//  - RawAcidFence.handle(parent, event, count, period) [48ea30]: 중지 비트(0x20)가 없을 때 event0 은 count0 이어도 공격하고, event1 은 count 를 DWORD 로 올려 마지막 프레임(총 프레임 − 1)보다 크면 0 을 돌려주며 아니면 (마지막 − count) 프레임으로 setFrame 한다. 공통 Regular 로 위임하지 않고 period 를 돌려준다.
//  - RawAcidFence.attack(parent) [48c830]: +X/+Y 첫 양방향 동맹(RawArcSpire.collect)을 찾아 축마다 strike(가로 xOriented=1, 세로 0) 하고, 어느 축이든 이웃이 있었으면 예약이 성공할 때 본체 애니메이션 regular(parent, 1, 0.2)를 건다.
//  - RawAcidFence.strike(parent, neighbors, xOriented) [48ca70]: 이웃마다 애니메이션 예약 → 첫 영역(0x50100) → 비행체 6영역(0x100000) → 높이 연결 6영역(비행체 3·운반체 3)을 이 순서로 훑어 최대 40개 후보를 모은 뒤, 살아 있는 후보만 사제는 피해 1000, 나머지는 11바이트 포획 명령을 보내고 로컬 수신한다.
// 하위 공간 검색/예약/피해/전송/프레임은 훅이며 도달한 지점에서만 요구한다(없으면 TypeError). 월드 전역(boss/editor/localOwner)은 state 객체를 매번 읽고, 친군 사격은 friendlyFire() 함수의 비영 값을 매번 읽는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E50: RawAcidFence 이식으로 추가)
import { RawArcSpire } from './raw-arc-spire.js';

// 슬롯 필드 위치와 개수 상수이다. (2026-11-08 WB04-E50)
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

// 상태 비트와 후보 genus 비트, 사제 피해량이다. (2026-11-08 WB04-E50)
const POPPED_MASK = 9;
const REGULAR_STOP_BIT = 0x20;
const DEAD_MASK = 3;
const FREE_BIT = 1;
const SURFACE_BIT = 1;
const FIRST_GENUS = 0x50100;
const FLYER_GENUS = 0x100000;
const BALLOON_GENUS = 0x20000;
const EXCLUDED_GENUS = 0x200;
const PRIEST_GENUS = 0x200000;
const PRIEST_DAMAGE = 1000;

// 41d7a0 이 더하는 원본 502310 의 float 값과 50e870 의 binary64 상수(float 5.3 으로 먼저 좁히지 않음)이다. (2026-11-08 WB04-E50)
const CEIL_BIAS = Math.fround(0.9999899864196777);
const HIGH_FLYER_OFFSET = 5.3;

// 예약 뒤 본체/이웃 애니메이션 지연(float 0.3, 0.2)이다. (2026-11-08 WB04-E50)
const FIRST_DELAY = Math.fround(0.3);
const ANIMATION_DELAY = Math.fround(0.2);

// 정렬되지 않은 float 4바이트를 읽는다. (2026-11-08 WB04-E50)
const floatView = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    floatView.setUint8(0, raw[offset]);
    floatView.setUint8(1, raw[offset + 1]);
    floatView.setUint8(2, raw[offset + 2]);
    floatView.setUint8(3, raw[offset + 3]);
    return floatView.getFloat32(0, true);
}

// 원본 CRT _ftol: 0 방향 절삭한 64비트 정수의 하위 32비트이며 비유한/범위 밖은 0 이다. (2026-11-08 WB04-E50)
function truncate(value) {
    if (!(value > -9223372036854775808 && value < 9223372036854775808)) return 0;
    return Math.trunc(value) | 0;
}

// 중간 float 저장은 명시한 지점에서만 적용한다. (2026-11-08 WB04-E50)
const stored = Math.fround;

// 선택된 실제 하위 경계가 없으면 빈 성공으로 대신하지 않는다. (2026-11-08 WB04-E50)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('애시드 실제 하위 경계 미연결');
    return hook;
}

// 현재 패킷의 리틀엔디언 WORD 하나이다. (2026-11-08 WB04-E50)
const word = (packet, at) => packet[at] | (packet[at + 1] << 8);

// 48c7a0: 최초 관문은 예약/공통 후처리 전에 한 번만 읽는다. (2026-11-08 WB04-E50)
export function postPopAcidFence(pool, parent, flags, reserve, regular, damageable) {
    if (pool?.edition !== 'patch1078') throw new TypeError('애시드 후처리 판본 오류');
    if (!(pool.slot(parent)[FLAG_OFFSET] & POPPED_MASK) && (flags & 1) && need(reserve)()) need(regular)(parent, 0, FIRST_DELAY);
    need(damageable)(parent, flags);
}

// 애시드 실행기이다. 사용: new RawAcidFence(pool, types, frames, spots, alliances, state, friendlyFire, special, zap, hooks).
// state = { boss, editor, localOwner }(월드 전역, 매번 읽음), friendlyFire() 는 친군 사격 DWORD 의 현재 값, special 은 { rainFlyer, sunFlyer, windFlyer, rainBalloon, windBalloon, sunBalloon } 현재 TYPE 전역(후보마다 읽음),
// zap 은 RawAcidZap(build/receive), hooks = { range, find, iterate, reserveRegular, regular, damage, broadcast, assertion, setFrame }. (2026-11-08 WB04-E50)
export class RawAcidFence {
    #pool;
    #types;
    #frames;
    #spots;
    #alliances;
    #state;
    #friendlyFire;
    #special;
    #zap;
    #hooks;
    #neighbors;

    // 다른 월드/판본/부분 지도를 섞지 않도록 생성 때 확인한다. 이웃 수집은 같은 48c300 몸체(RawArcSpire.collect)에 find 만 연결한다. (2026-11-08 WB04-E50)
    constructor(pool, types, frames, spots, alliances, state, friendlyFire, special, zap, hooks) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || frames?.length !== TYPE_COUNT || spots?.length !== SPOT_COUNT ||
            alliances?.length !== ALLIANCE_COUNT || typeof state !== 'object' || state === null || typeof friendlyFire !== 'function' ||
            typeof special !== 'object' || special === null || typeof zap?.build !== 'function' || typeof zap.receive !== 'function') {
            throw new TypeError('애시드 같은 원본 자료 크기/판본 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#spots = spots;
        this.#alliances = alliances;
        this.#state = state;
        this.#friendlyFire = friendlyFire;
        this.#special = special;
        this.#zap = zap;
        this.#hooks = hooks ?? {};
        this.#neighbors = new RawArcSpire(pool, types, frames, spots, alliances, state, { find: (...args) => need(this.#hooks.find)(...args) });
    }

    // 각 영역과 하위 효과 뒤 현재 좌표를 다시 읽는다. (2026-11-08 WB04-E50)
    #position(sid) {
        const raw = this.#pool.slot(sid);
        return [coordinate(raw, X_OFFSET), coordinate(raw, Y_OFFSET)];
    }

    // 원본의 정방향 동맹만 비교한다(역방향은 이 피해 후보 필터에 추가하지 않는다). (2026-11-08 WB04-E50)
    #allied(first, second) {
        if (first > OWNER_COUNT - 1 || second > OWNER_COUNT - 1) throw new RangeError('애시드 현재 동맹 owner 오류');
        return this.#alliances[first * OWNER_COUNT + second] !== 0;
    }

    // 각 축을 1..255 로 제한한 포함 사각형의 표면 바이트 OR 이다(원본 y→x 순서). (2026-11-08 WB04-E50)
    #spotUnion(area) {
        const left = Math.min(Math.max(area.left, 1), 255);
        const right = Math.min(Math.max(area.right, 1), 255);
        const top = Math.min(Math.max(area.top, 1), 255);
        const bottom = Math.min(Math.max(area.bottom, 1), 255);
        let result = 0;
        // 위에서 아래로 한 줄씩 OR 한다.
        for (let y = top; y <= bottom; y += 1) {
            // 왼쪽에서 오른쪽으로 OR 한다.
            for (let x = left; x <= right; x += 1) result |= this.#spots[y * MAP_SIDE + x];
        }
        return result & 0xff;
    }

    // 한 영역을 검색해 후보를 모은다. 수집 시 free/dead 를 걸러내지 않고 최종 실행 직전에 따로 확인한다. typeKey 가 null 이면 타입 제한이 없고, 아니면 후보마다 현재 special[typeKey] 와 비교한다. (2026-11-08 WB04-E50)
    #scan(parent, other, bounds, genus, typeKey, victims) {
        const pool = this.#pool;
        const area = { left: truncate(bounds[0]), top: truncate(bounds[1]), right: truncate(bounds[2] + CEIL_BIAS), bottom: truncate(bounds[3] + CEIL_BIAS) };
        need(this.#hooks.iterate)(area, (candidate) => {
            const raw = pool.slot(candidate);
            if ((raw[FLAG_OFFSET] & FREE_BIT) || candidate === parent || candidate === other ||
                (this.#allied(pool.slot(parent)[OWNER_OFFSET], raw[OWNER_OFFSET]) && !this.#friendlyFire())) return;
            const type = this.#types[raw[TYPE_OFFSET]];
            if (type === undefined) throw new RangeError('애시드 후보 TYPE 오류');
            const flags = type.flags2;
            if (!(flags & genus) || (flags & EXCLUDED_GENUS) || !this.#state.boss || this.#state.editor ||
                (typeKey !== null && raw[TYPE_OFFSET] !== this.#special[typeKey]) || victims.length >= VICTIM_CAPACITY) return;
            victims.push(candidate);
        });
    }

    // 48ca70: 이웃마다 독립 40개 버퍼이며 13영역의 중복과 순서를 그대로 유지한다. xOriented 는 가로 축(1) 또는 세로 축(0) 방향이다. (2026-11-08 WB04-E50)
    strike(parent, neighbors, xOriented) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        // 이웃의 실제 애니메이션 예약을 모든 영역 검색보다 먼저 수행한다.
        for (const other of neighbors) {
            if (need(hooks.reserveRegular)()) need(hooks.regular)(other, 1, ANIMATION_DELAY);
            const victims = [];
            let source = this.#position(parent);
            let target = this.#position(other);
            let ly = stored(source[1] - 1);
            let hy = stored(target[1] - 1);
            if (xOriented) {
                ly = stored(ly + 0.5);
                hy = stored(hy - 0.5);
            }
            this.#scan(parent, other, [stored(source[0] - 1), ly, stored(target[0] - 1), hy], FIRST_GENUS, null, victims);
            // 144/73/138 각각의 부모와 이웃 발자국 영역이다.
            for (let region = 0; region < 6; region += 1) {
                const point = this.#position(region % 2 ? other : parent);
                const record = this.#types[pool.slot(parent)[TYPE_OFFSET]];
                if (record === undefined) throw new RangeError('애시드 부모 TYPE 오류');
                let lowX = stored(point[0] - stored(record.footX));
                let lowY = stored(point[1] - stored(record.footY));
                let highY = point[1];
                const typeKey = region < 2 ? 'rainFlyer' : region < 4 ? 'sunFlyer' : 'windFlyer';
                if (region < 2) {
                    lowX = stored(lowX + 1);
                } else if (region < 4) {
                    lowX = stored(lowX + 2);
                    lowY = stored(lowY - 4);
                    if (xOriented) highY = stored(highY - 2);
                }
                this.#scan(parent, other, [lowX, lowY, point[0], highY], FLYER_GENUS, typeKey, victims);
            }
            // 비행체 세 종과 운반체 세 종의 서로 다른 높이 연결 영역이다.
            for (let region = 0; region < 6; region += 1) {
                source = this.#position(parent);
                target = this.#position(other);
                let lowX = 0;
                let lowY = 0;
                let highX = 0;
                let highY = 0;
                let typeKey;
                const genus = region < 3 ? FLYER_GENUS : BALLOON_GENUS;
                if (region === 0) {
                    lowX = stored(source[0] - 2);
                    lowY = stored(source[1] - 3);
                    if (xOriented) lowY = stored(lowY + 1);
                    else lowX = stored(lowX + 0.5);
                    highX = xOriented ? target[0] : stored(target[0] - 0.5);
                    highY = stored(target[1] - 1);
                    if (xOriented) highY = stored(highY - 1.5);
                    typeKey = 'rainFlyer';
                } else if (region === 1) {
                    lowX = stored(source[0] - 2.5);
                    lowY = stored(source[1] - 1);
                    highX = stored(target[0] - 1);
                    highY = stored(target[1] - 1);
                    if (xOriented) {
                        lowX = stored(lowX + 2.5);
                        lowY = stored((lowY - 3) + 0.5);
                        highX = stored(highX - 1);
                        highY = stored(highY - 1.5);
                    }
                    typeKey = 'windFlyer';
                } else if (region === 2) {
                    lowX = stored(source[0] - 1);
                    lowY = stored(source[1] - 1);
                    highX = stored(target[0] + 1);
                    highY = stored(target[1] - 1);
                    if (xOriented) {
                        lowX = stored(lowX + 1);
                        lowY = stored((lowY - 7) + 1.5);
                        highX = stored(highX - 3);
                        highY = stored((highY - HIGH_FLYER_OFFSET) - 0.5);
                    }
                    typeKey = 'sunFlyer';
                } else if (region === 3) {
                    lowY = stored(source[1] - 1);
                    if (xOriented) lowY = stored((lowY - 2) + 0.5);
                    lowX = stored((source[0] - 2.5) + (xOriented ? 2.5 : 1));
                    highX = stored(target[0] - 1);
                    if (xOriented) highX = stored(highX - 1);
                    highY = stored((target[1] - 1) - (xOriented ? 1 : 5));
                    typeKey = 'rainBalloon';
                } else if (region === 4) {
                    lowY = stored(source[1] - 1);
                    if (xOriented) lowY = stored(lowY + 0.5);
                    lowX = stored((source[0] - 2.5) + (xOriented ? 2.5 : 2));
                    highX = stored(target[0] - 1);
                    if (xOriented) highX = stored(highX - 1);
                    highY = stored((target[1] - 1) - (xOriented ? 1 : 6));
                    typeKey = 'windBalloon';
                } else {
                    lowX = stored(source[0] - 2.5);
                    lowY = stored(source[1] - 1);
                    highX = stored(target[0] - 1);
                    highY = stored(target[1] - 1);
                    if (xOriented) {
                        lowX = stored(lowX + 2.5);
                        lowY = stored((lowY - 2) + 0.5);
                        highX = stored(highX - 1);
                    } else {
                        highY = stored(highY - 5);
                    }
                    typeKey = 'sunBalloon';
                }
                this.#scan(parent, other, [lowX, lowY, highX, highY], genus, typeKey, victims);
            }
            // 송신/피해 뒤 앞선 중복 대상이 죽으면 다음 실행을 건너뛴다.
            for (const victim of victims) {
                if (!victim || victim >= pool.capacity || (pool.slot(victim)[STATE_OFFSET] & DEAD_MASK)) continue;
                const raw = pool.slot(victim);
                const record = this.#types[raw[TYPE_OFFSET]];
                if (record === undefined) throw new RangeError('애시드 대상 TYPE 오류');
                if (record.flags2 & PRIEST_GENUS) {
                    need(hooks.damage)(victim, PRIEST_DAMAGE, pool.slot(parent)[OWNER_OFFSET], parent);
                    continue;
                }
                const packet = this.#zap.build(victim, parent, other, (xOriented << 24) >> 24);
                need(hooks.broadcast)(packet);
                if (word(packet, 4) !== victim) need(hooks.assertion)(0x2fa);
                if (word(packet, 6) !== parent) need(hooks.assertion)(0x2fb);
                if (word(packet, 8) !== other) need(hooks.assertion)(0x2fc);
                if (((packet[10] << 24) >> 24) !== xOriented) need(hooks.assertion)(0x2fd);
                this.#zap.receive(this.#state.localOwner, 0, packet);
            }
        }
    }

    // 48c830: 양의 두 축에서 첫 동맹까지만 연결하고 표면 비트가 없는 곳은 find 를 호출하지 않는다. (2026-11-08 WB04-E50)
    attack(parent) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const source = this.#position(parent);
        const x = truncate(source[0]);
        const y = truncate(source[1]);
        const range = truncate(Math.fround(need(hooks.range)(parent)));
        const endX = truncate(Math.min(source[0] + range, 254.0));
        const endY = truncate(Math.min(source[1] + range, 254.0));
        const buffer = new Array(BUFFER_CAPACITY).fill(0);
        let count = 0;
        // 가로 축: 첫 양방향 동맹까지 훑는다.
        if (this.#spotUnion({ left: x, top: y, right: (endX - 1) | 0, bottom: y }) & SURFACE_BIT) {
            // 부모 오른쪽 칸부터 끝 직전까지 순서대로 훑는다.
            for (let at = x + 1; at < endX; at += 1) {
                if (at < 0 || at >= MAP_SIDE || y < 0 || y >= MAP_SIDE) throw new RangeError('애시드 가로 지도 범위');
                if (this.#spots[y * MAP_SIDE + at] & SURFACE_BIT) {
                    const found = this.#neighbors.collect(parent, buffer, count, at, y, pool.slot(parent)[TYPE_OFFSET]);
                    count = found.count;
                    if (found.found) break;
                }
            }
        }
        let connected = count !== 0;
        this.strike(parent, buffer.slice(0, count), 1);
        count = 0;
        // 세로 축: 앞선 가로 효과 뒤에도 현재 같은 지도/부모를 읽는다.
        if (this.#spotUnion({ left: x, top: y, right: x, bottom: (endY - 1) | 0 }) & SURFACE_BIT) {
            // 부모 아래 칸부터 끝 직전까지 순서대로 훑는다.
            for (let at = y + 1; at < endY; at += 1) {
                if (at < 0 || at >= MAP_SIDE || x < 0 || x >= MAP_SIDE) throw new RangeError('애시드 세로 지도 범위');
                if (this.#spots[at * MAP_SIDE + x] & SURFACE_BIT) {
                    const found = this.#neighbors.collect(parent, buffer, count, x, at, pool.slot(parent)[TYPE_OFFSET]);
                    count = found.count;
                    if (found.found) break;
                }
            }
        }
        connected = connected || count !== 0;
        this.strike(parent, buffer.slice(0, count), 0);
        if (connected && need(hooks.reserveRegular)()) need(hooks.regular)(parent, 1, ANIMATION_DELAY);
    }

    // 48ea30: event1 의 마지막 프레임은 총 프레임 − 1 을 기준으로 하며 DWORD 감김 뒤 signed 비교이다. 공통 Regular 로 위임하지 않고 period 를 돌려준다. (2026-11-08 WB04-E50)
    handle(parent, event, count, period) {
        const pool = this.#pool;
        let current = count >>> 0;
        if (!(pool.slot(parent)[FLAG_OFFSET] & REGULAR_STOP_BIT)) {
            if (!event) {
                this.attack(parent);
            } else if (event === 1) {
                current = (current + 1) >>> 0;
                const codes = this.#frames[pool.slot(parent)[TYPE_OFFSET]];
                if (codes === undefined) throw new RangeError('애시드 프레임 TYPE 범위 오류');
                const last = ((codes.length | 0) - 1) | 0;
                if (last < (current | 0)) return 0;
                need(this.#hooks.setFrame)(parent, (last - current) | 0, 0);
            }
        }
        return Math.fround(period);
    }
}
