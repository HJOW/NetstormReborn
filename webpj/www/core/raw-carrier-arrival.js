// 실제 Carrier(운반체) 도착(00429800)의 현재 경로/내용물/다이스/운반 분기와 끝의 회피 검색 순서를 옮긴 모듈이다(cpppj `o/RawCarrierArrival.cpp/.h`, 10.78).
//  - createCarrierArrivalState(): 도착 권한(boss)·패치 Robust 검사(debug)·친구 사제 pickup 허용(allowAlliedPickup)·원래 mutable 타입 getter 값(priestType 158·nuggetType 154·daisType 165·geyserType 122·outpostType 126·isleStalagType 84)·
//    방향을 보존하는 9×9 동맹표(alliances[운반체 소유자×9+후보 소유자])·독립 재생용 Player AI 존재(controlledOwners[9])와 실제 세션의 공유 getter(sharedAllowAlliedPickup·sharedGeyserType·sharedNuggetType)를 만든다.
//    공유 getter 는 선행 효과 뒤에도 현재 값을 다시 읽는다. allowAlliedPickupNow/geyserTypeNow/nuggetTypeNow(state)가 현재 값을 돌려준다.
//  - new RawCarrierArrival(pool, hash, types, carrier, cleanup, priest, anchor, create, footprint, state, hooks): 모든 풀/해시/타입/하위 처리기 참조와 mutable 상태는 이 객체보다 오래 살아야 한다.
//    hooks = { path(sid)→RawPathRuntime|null, genusAt(x, y, genus)→SID, pickup(sid), escape(sid, delay, event)→반환, chooseReturn(sid)→SID, chooseGeyser(sid)→SID, goReturn(sid, target), goGeyser(sid, target),
//    minedPacket(sid), destroy(sid, flags), robust(sid)→bool, (선택) currentControlled(owner)→bool, currentAlliance(first, second)→DWORD } 이며 도달한 큰 운반/포획/경제/프로토콜/회피 몸체는 필수 실제 효과를 공급해야 한다.
//    CD(10.72) 전용 경제 경계(cdAddSp·cdOwnerPacket·playAt·cdPriestPickupPriest)는 옮기지 않았다.
//  - arrival(sid, targetA, targetB): 00429800 전체 부모 몸체이다. 운반체 genus 확인(아니면 assert 로 거부) → 사제 부모의 현재 경로 helper → 소유자 0 이면 끝 → 현재 보조(너겟 검색·지면/2000 genus·귀환 장소·포획 사제·다이스 위치 검증·pickup 후보)를 읽고
//    경로의 깊이가 0 이면 pickup/다이스/귀환/간헐천 분기(중립/제어 소유자는 새 이동 요청을 하지 않음)를 수행한 뒤, 권한이 있으면 끝의 회피 검색(다른 운반체·섬 종유석·위험 지면)을 실제 finder 순서로 한다. 반환은 1000(종료)/1001(대체)/회피 반환이다.
//  - pickupCandidate(sid, x, y): 00426900 현재 거의올림 지점의 첫 허용 pickup 후보와 genus 2000 여부({ sid, flag }). validGeyser(sid, target)/validReturn(sid, target): 00427580/004274b0 현재 대상 검증이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E160: 운반체 도착 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 운반체/사제/포획 후보 genus, 너겟 contained mask, 부모 도착 상태(종료 1000·대체 1001)와 회피 후속 사건, 거의올림 상수, 다이스·낮은 위치 오프셋이다. (2026-11-09 WB04-E160)
const CARRIER = 0x30000;
const PRIEST = 0x200000;
const PICKUP = 0x20a000;
const NUGGET_MASK = 32;
const DONE = 1000;
const REPLACED = 1001;
const OBSTACLE_EVENT = 0x1da1a;
const ALMOST_CEIL = Math.fround(0.9999899864196777);
const DAIS_OFFSET = 3;
const LOWER_OFFSET = 2;
const TYPE = 10;
const STATE = 11;
const X = 14;
const Y = 18;
const OWNER = 34;
const EXTRA = 40;
const TYPE_COUNT = 188;
const REQUIRED_HOOKS = ['path', 'genusAt', 'pickup', 'escape', 'chooseReturn', 'chooseGeyser', 'goReturn', 'goGeyser', 'minedPacket', 'destroy', 'robust'];

// 도착 처리가 읽는 전역을 원본 초기값으로 만든다. (2026-11-09 WB04-E160)
export function createCarrierArrivalState() {
    return {
        boss: true, debug: 0, allowAlliedPickup: false,
        priestType: 158, nuggetType: 154, daisType: 165, geyserType: 122, outpostType: 126, isleStalagType: 84,
        alliances: new Uint32Array(81), controlledOwners: Array(9).fill(false),
        sharedAllowAlliedPickup: null, sharedGeyserType: null, sharedNuggetType: null
    };
}

// 실제 capture DWORD 의 0/비0 를 bool 로 alias 하지 않고 현재 비0 를 판정한다. (2026-11-09 WB04-E160)
export function allowAlliedPickupNow(state) {
    return typeof state.sharedAllowAlliedPickup === 'function' ? (state.sharedAllowAlliedPickup() | 0) !== 0 : !!state.allowAlliedPickup;
}

// 선행 효과 뒤 현재 간헐천 TYPE 를 다시 읽는다. (2026-11-09 WB04-E160)
export function geyserTypeNow(state) {
    return (typeof state.sharedGeyserType === 'function' ? state.sharedGeyserType() : state.geyserType) >>> 0;
}

// 원문 위치마다 현재 너겟 번호를 읽는다(같은 장기 owner 를 내용물 검색/너겟 생성이 공유). (2026-11-09 WB04-E160)
export function nuggetTypeNow(state) {
    return (typeof state.sharedNuggetType === 'function' ? state.sharedNuggetType() : state.nuggetType) >>> 0;
}

// 정수 좌표와 현재 biased float 합을 float 으로 한 번 좁히지 않는다. 비유한/지도 밖은 거부한다. (2026-11-09 WB04-E160)
function cell(value, biased = false) {
    const adjusted = Math.fround(value) + (biased ? ALMOST_CEIL : 0);
    if (!Number.isFinite(adjusted) || adjusted < 0 || adjusted >= 256) throw new RangeError('Carrier 도착 정상 지도 좌표');
    return Math.trunc(adjusted);
}

// 정렬되지 않은 float 좌표/필드를 읽는다. (2026-11-09 WB04-E160)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return view.getFloat32(0, true);
}

// 원본 WORD 옆의 stale 필드를 섞지 않고 지정 폭만 읽는다. (2026-11-09 WB04-E160)
function word(raw, offset) { return raw[offset] | (raw[offset + 1] << 8); }

// 실제 Carrier 부모 도착 처리기이다. 큰 하위 효과 경계와 순수 조회를 분리하며 기존 완료 몸체를 다시 작성하지 않는다. (2026-11-09 WB04-E160)
export class RawCarrierArrival {
    #pool;
    #hash;
    #types;
    #carrier;
    #cleanup;
    #priest;
    #anchor;
    #create;
    #footprint;
    #state;
    #hooks;

    // 첫 원본 조회 전에 실제 같은 월드와 모든 선택 가능한 큰 하위 경계를 확인한다. (2026-11-09 WB04-E160)
    constructor(pool, hash, types, carrier, cleanup, priest, anchor, create, footprint, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Carrier 도착은 10.78 SidPool 만 받습니다.');
        if (carrier?.pool !== pool || cleanup?.pool !== pool || anchor?.pool !== pool || footprint?.pool !== pool || !priest || !create || !hash || !state
            || !Array.isArray(types) || types.length !== TYPE_COUNT || REQUIRED_HOOKS.some((name) => typeof hooks?.[name] !== 'function')) {
            throw new TypeError('Carrier 도착 실제 풀/필수 하위 효과 연결 누락');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#carrier = carrier;
        this.#cleanup = cleanup;
        this.#priest = priest;
        this.#anchor = anchor;
        this.#create = create;
        this.#footprint = footprint;
        this.#state = state;
        this.#hooks = { ...hooks };
    }

    // 실제 사제+3c/걷기 어댑터가 다른 월드의 도착 처리기를 연결하는 일을 거부할 수 있도록 원래 풀을 제공한다. (2026-11-09 WB04-E160)
    get pool() { return this.#pool; }

    // 현재 좌표는 같은 슬롯에서 float 비트를 그대로 읽는다. 현재 raw/type/owner/extra 는 하위 효과 뒤 같은 풀에서 다시 읽는다. (2026-11-09 WB04-E160)
    #position(sid) { const raw = this.#pool.slot(sid); return { x: floatAt(raw, X), y: floatAt(raw, Y) }; }
    #genus(sid) {
        const type = this.#pool.slot(sid)[TYPE];
        if (type >= this.#types.length) throw new RangeError('Carrier 도착 현재 타입 범위');
        return this.#types[type].flags2;
    }
    #owner(sid) { return this.#pool.slot(sid)[OWNER]; }
    #extra(sid) { return this.#pool.slot(sid)[EXTRA]; }

    // 실제 owner0 은 Player AI 배열을 조회하지 않고 거짓이다. (2026-11-09 WB04-E160)
    #controlled(sid) {
        const owner = this.#owner(sid);
        if (owner >= this.#state.controlledOwners.length) throw new RangeError('Carrier 도착 Player 범위');
        return owner !== 0 && !!(typeof this.#hooks.currentControlled === 'function' ? this.#hooks.currentControlled(owner) : this.#state.controlledOwners[owner]);
    }

    // 동맹표의 두 순서를 같다고 가정하지 않는다. (2026-11-09 WB04-E160)
    #allied(first, second) {
        if (first >= 9 || second >= 9) throw new RangeError('Carrier 도착 동맹표 범위');
        return (typeof this.#hooks.currentAlliance === 'function' ? this.#hooks.currentAlliance(first, second) : this.#state.alliances[first * 9 + second]) !== 0;
    }

    // 원본 공통 정상 반환 접두를 goto 분기에서도 빠뜨리지 않는다(패치의 Robust 검사). (2026-11-09 WB04-E160)
    #checkedDone(sid) {
        if (this.#state.debug && !this.#hooks.robust(sid)) throw new Error('Carrier 도착 isRobust 원본 assert');
        return DONE;
    }

    // 004261d0: 실제 현재 Path 가 없으면 원본 helper 처럼 대상 필드 쓰기도 없다. (2026-11-09 WB04-E160)
    #setTargets(sid, first, second) {
        const path = this.#hooks.path(sid);
        if (path) path.setTargets(first, second);
    }

    // 00426900: 거의올림→단순 절삭 점 finder 와 현재 사제 불가/owner/설정 조건을 원본 순서로 읽는다. buried/공간 순서는 일반 finder 이며 추가 dead/free/owner 필터를 넣지 않는다. (2026-11-09 WB04-E160)
    pickupCandidate(sid, x, y) {
        const cx = cell(x, true);
        const cy = cell(y, true);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        for (let candidate = finder.begin({ left: cx, top: cy, right: cx, bottom: cy }); candidate !== 0; candidate = finder.next()) {
            const genus = this.#genus(candidate);
            if ((genus & PICKUP) === 0) continue;
            let accept = true;
            if ((genus & PRIEST) !== 0) {
                const immobile = this.#priest.groundImmobile(candidate);
                accept = this.#owner(sid) !== this.#owner(candidate) && (this.#genus(sid) & PRIEST) === 0
                    && (immobile || (allowAlliedPickupNow(this.#state) && this.#allied(this.#owner(sid), this.#owner(candidate))));
            }
            if (accept) return { sid: candidate, flag: (genus & 0x2000) !== 0 };
        }
        return { sid: 0, flag: false };
    }

    // 00427580: 현재 지면/상태/타입 다음 fly 부류가 아니면 실제 Graph 를 비교한다. (2026-11-09 WB04-E160)
    validGeyser(sid, target) {
        const raw = this.#pool.slot(target);
        if ((this.#extra(target) & 9) !== 0 || (raw[STATE] & 2) !== 0 || raw[TYPE] !== geyserTypeNow(this.#state)) return false;
        return (this.#genus(sid) & 0x20000) !== 0 || this.#anchor.graphFor(sid) === this.#anchor.graphFor(target);
    }

    // 004274b0: 패치의 현재 canProcessStormpower assert 를 보존한다. (2026-11-09 WB04-E160)
    validReturn(sid, target) {
        const state = this.#state;
        const raw = this.#pool.slot(target);
        if (state.debug && (this.#genus(target) & 0x200) === 0 && raw[TYPE] !== state.outpostType) throw new Error('Carrier return 대상 원본 assert');
        if (this.#owner(target) !== this.#owner(sid) || (this.#extra(target) & 9) !== 0 || (raw[STATE] & 2) !== 0) return false;
        return (this.#genus(sid) & 0x20000) !== 0 || this.#anchor.graphFor(sid) === this.#anchor.graphFor(target);
    }

    // 현재 보조/포획/대상/깊이 뒤 회피와 마지막 Robust 를 실제 부모 순서로 실행한다. (2026-11-09 WB04-E160)
    arrival(sid, targetA, targetB) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const state = this.#state;
        if ((this.#genus(sid) & CARRIER) === 0) throw new Error('Carrier 도착 isCarrier 원본 assert');
        if ((this.#genus(sid) & PRIEST) !== 0) {
            const current = hooks.path(sid);
            if (current && current.helper) return DONE;
        }
        if (!this.#owner(sid)) return DONE;
        let nugget = this.#carrier.findContained(sid, NUGGET_MASK, nuggetTypeNow(state), 0);
        let point = this.#position(sid);
        let ground = hooks.genusAt(point.x, point.y, 0x10000000);
        point = this.#position(sid);
        hooks.genusAt(point.x, point.y, 0x2000);
        point = this.#position(sid);
        let place = this.#cleanup.returnPlaceAt(point.x, point.y, 1);
        const captured = this.#carrier.findContained(sid, NUGGET_MASK, state.priestType, 0);
        if (captured) { place = 0; ground = 0; nugget = 0; }
        point = this.#position(sid);
        let dais = this.#cleanup.nearbyType(point.x, point.y, 1, state.daisType);
        if (dais) {
            const daisPoint = this.#position(dais);
            if ((this.#extra(dais) & 1) !== 0 || point.x !== daisPoint.x - DAIS_OFFSET || point.y !== daisPoint.y - DAIS_OFFSET) dais = 0;
        }
        point = this.#position(sid);
        const pickup = this.pickupCandidate(sid, point.x, point.y);
        const currentContent = this.#carrier.findContained(sid, NUGGET_MASK, 0, 0);
        this.#carrier.findContained(sid, 1, 0, 0);
        const currentPath = hooks.path(sid);
        if (!currentPath || !currentPath.moveAwayDepth) {
            if ((this.#types[pool.slot(sid)[TYPE]].flags1 & 0x1000000) === 0 && pickup.sid !== 0 && (pickup.flag || !currentContent)) hooks.pickup(sid);
            else if (dais) {
                if (state.boss) {
                    if (captured && this.#allied(this.#owner(sid), this.#owner(dais)) && !this.#carrier.findContained(dais, NUGGET_MASK, state.priestType, 0)) {
                        this.#create.create(dais, state.priestType, word(pool.slot(captured), 12), NUGGET_MASK, 0, 0);
                        hooks.destroy(captured, 0);
                    }
                    hooks.escape(sid, 5, 0);
                    return REPLACED;
                }
                return this.#checkedDone(sid);
            } else if (!place || !nugget) {
                if (ground && this.#anchor.graphFor(ground) === this.#anchor.graphFor(sid)) {
                    if (!state.boss) return this.#checkedDone(sid);
                    let replaced = false;
                    if (state.boss && !this.#controlled(sid)) {
                        if (!targetB || !this.validReturn(sid, targetB)) targetB = hooks.chooseReturn(sid);
                        this.#setTargets(sid, ground, targetB);
                        if (targetB) { replaced = true; hooks.goReturn(sid, targetB); }
                    }
                    if (state.boss && !nugget && word(pool.slot(ground), 12)) {
                        this.#create.create(sid, nuggetTypeNow(state), 0, NUGGET_MASK, 0, 0);
                        hooks.minedPacket(ground);
                    }
                    if (replaced) return REPLACED;
                }
            } else if (this.#owner(place) === this.#owner(sid) && this.#anchor.graphFor(place) === this.#anchor.graphFor(sid)) {
                if (!state.boss) return this.#checkedDone(sid);
                if (state.boss && !this.#controlled(sid)) {
                    if (!targetA || !this.validGeyser(sid, targetA)) {
                        targetA = hooks.chooseGeyser(sid);
                        this.#setTargets(sid, targetA, 0);
                    }
                    if (targetA) { hooks.goGeyser(sid, targetA); return REPLACED; }
                }
            }
        }
        if (state.boss) {
            point = this.#position(sid);
            if ((this.#genus(sid) & PRIEST) === 0 || !this.#cleanup.nearbyType(point.x, point.y, 1, state.daisType)) {
                const x = cell(point.x);
                const y = cell(point.y);
                const finder = new RawSquidFinder(pool, this.#hash, this.#types);
                // 현재 점의 기본 비표면 finder 는 다른 운반체/특정 지면 자산을 원본 순서로 검사한다.
                for (let candidate = finder.begin({ left: x, top: y, right: x, bottom: y }, 1); candidate !== 0; candidate = finder.next()) {
                    if ((this.#genus(candidate) & CARRIER) !== 0 && candidate !== sid && candidate !== pickup.sid) return hooks.escape(sid, 1, 0xffffffff);
                    if (pool.slot(candidate)[TYPE] === state.isleStalagType && !this.#controlled(sid)) return hooks.escape(sid, 2, OBSTACLE_EVENT);
                }
                if (!this.#controlled(sid) && (this.#genus(sid) & 0x20000) === 0) {
                    point = this.#position(sid);
                    const positions = [point, { x: point.x, y: Math.fround(point.y + LOWER_OFFSET) }];
                    const genus = [0x4200, 0x200];
                    const delays = [1, 2];
                    // 마지막 두 점은 현재 위치/y+2 이며 각 점의 plain 절삭 비표면 finder 다.
                    for (let index = 0; index < positions.length; index += 1) {
                        const px = cell(positions[index].x);
                        const py = cell(positions[index].y);
                        const obstacles = new RawSquidFinder(pool, this.#hash, this.#types);
                        for (let candidate = obstacles.begin({ left: px, top: py, right: px, bottom: py }, 1); candidate !== 0; candidate = obstacles.next()) {
                            if ((this.#genus(candidate) & genus[index]) !== 0) return hooks.escape(sid, delays[index], OBSTACLE_EVENT);
                        }
                    }
                }
            }
        }
        return this.#checkedDone(sid);
    }
}
