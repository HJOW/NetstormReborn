// 사제 가상 +38 Halt(지면 모드·저 HP 보정)와 공통 Carrier Halt(0042a5f0)의 지면·검색·반환 순서를 옮긴 모듈이다(cpppj `o/RawPriestHalt.cpp/.h`, 10.78 전용).
//  - halt(sid): 최초 좌표를 보관한 뒤, 전투 모드이면 좌표(0.9999 를 더해 절삭)의 spot & 6(지면)이 없을 때 권한자는 Unpop(0)→최초 좌표를 0.99998999 로 절삭한 칸으로 Pop(0)하고 11 을 반환하며,
//    지면이 있고 현재 HP 가 최대 HP 의 절반 미만이며 권한자이면 같은 보정 뒤 11 을 반환한다. 그 밖에는 공통 Halt 로 위임한다. 이후 raw 위치는 다시 읽지 않는다.
//  - commonHalt(sid): Carrier 가상 +0xcc 조회가 참이면 12 → 현재 좌표를 단순 절삭한 한 칸의 일반 finder 를 돌며 자신이 아니고 특정 운반체 타입(156)이면 포획 몸체(10.78 은 반환 12 일 때만 12), 너겟 운반체 타입(187)이면 전달 몸체(반환 12 일 때 12), 그 밖에는 10 을 반환한다.
// 큰 포획/전달/공간 몸체는 빈 효과로 완료 처리하지 않는 필수 하위 경계이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E66: 사제 Halt 이식으로 추가)

import { RawSquidFinder } from './raw-squid-finder.js';

// Halt 가 반환하는 부모 Halt 코드(이동 중/정지/변경)와 지면 조회·공간 복귀의 서로 다른 단정도 상수, 지도 크기이다. (2026-11-08 WB04-E66)
const MOVING = 10;
const STOPPED = 11;
const CHANGED = 12;
const GROUND_BIAS = Math.fround(0.9999);
const SNAP_BIAS = Math.fround(0.9999899864196777);
const GROUND = 6;
const WORLD_CELLS = 256;
const TYPE_COUNT = 188;
const TYPE_OFFSET = 10;
const X_OFFSET = 14;
const Y_OFFSET = 18;

// 실제 전역을 각 하위 효과 뒤 다시 읽을 수 있도록 값을 직접 보관한다: battle(00594fbc), boss(공간 권한 00540bc4), 두 특정 운반체 TYPE 번호(변경 가능)이다. (2026-11-08 WB04-E66)
export class PriestHaltState {
    battle = false;
    boss = true;
    carrierType = 156;
    nuggetCarrierType = 187;
}

// 비트 보존을 위해 정렬되지 않은 좌표를 단정도로 읽는다. (2026-11-08 WB04-E66)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    // 네 바이트를 리틀엔디언으로 옮긴다.
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 단정도 좌표와 단정도 bias 의 합을 double 에 유지하고 0 방향으로 절삭한다. 정상 지도 범위 밖은 조회 전에 거부한다. (2026-11-08 WB04-E66)
function cell(coordinate, bias) {
    const value = coordinate + bias;
    if (!Number.isFinite(value) || value < 0 || value >= WORLD_CELLS) throw new RangeError('사제 Halt 정상 지도 좌표');
    return Math.trunc(value);
}

// pool 은 10.78 SidPool, hash 는 같은 공간 해시, types 는 188칸 TYPE 표(참조), spots 는 65536칸 spot 지도, check 는 RawCarrierCheck, priest 는 RawPriestState, hitPoints 는 SquidReward, state 는 PriestHaltState 이고
//   hooks = { unpop(sid, flags), pop(sid, x, y, flags), carrierPickup(sid)→수, nuggetTransfer(sid)→수 } 는 모두 필요하다. (2026-11-08 WB04-E66)
export class RawPriestHalt {
    // 모든 현재 raw 입력과 SID 의 풀, 공간 해시, 타입 표, 지도, 지면 조회, 사제 상태, HP, 전역, 하위 효과이다.
    #pool;
    #hash;
    #types;
    #spots;
    #check;
    #priest;
    #hitPoints;
    #state;
    #hooks;

    // 첫 몸체 실행 전에 같은 풀/타입/지도와 필수 외부 효과를 모두 확인한다. (2026-11-08 WB04-E66)
    constructor(pool, hash, types, spots, check, priest, hitPoints, state, hooks) {
        if (pool?.edition !== 'patch1078' || check?.pool !== pool || priest?.pool !== pool || hitPoints?.pool !== pool
            || !Array.isArray(types) || types.length !== TYPE_COUNT || !(spots instanceof Uint8Array) || spots.length !== WORLD_CELLS * WORLD_CELLS
            || ['unpop', 'pop', 'carrierPickup', 'nuggetTransfer'].some((name) => typeof hooks?.[name] !== 'function')) {
            throw new TypeError('사제 Halt 실제 월드/하위 효과 연결 누락');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#spots = spots;
        this.#check = check;
        this.#priest = priest;
        this.#hitPoints = hitPoints;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 실제 Path 부모/공간 처리기와 다른 풀을 섞는 연결을 거부할 때 쓰는 풀이다. (2026-11-08 WB04-E66)
    get pool() {
        return this.#pool;
    }

    // 현재 raw 좌표 두 개를 읽는다. (2026-11-08 WB04-E66)
    #position(sid) {
        const raw = this.#pool.slot(sid);
        return [floatAt(raw, X_OFFSET), floatAt(raw, Y_OFFSET)];
    }

    // 최초 좌표를 지도 칸으로 돌려놓는 Unpop/Pop 보정이다. 이후 raw 위치는 다시 읽지 않는다. (2026-11-08 WB04-E66)
    #snap(sid, initial) {
        this.#hooks.unpop(sid, 0);
        this.#hooks.pop(sid, Math.fround(cell(initial[0], SNAP_BIAS)), Math.fround(cell(initial[1], SNAP_BIAS)), 0);
    }

    // 실제 사제 가상 +38 이다. 지면 모드/저 HP 보정 다음 공통 Carrier Halt 로 위임한다. (2026-11-08 WB04-E66)
    halt(sid) {
        const initial = this.#position(sid);
        if (this.#state.battle) {
            const x = cell(initial[0], GROUND_BIAS);
            const y = cell(initial[1], GROUND_BIAS);
            const ground = (this.#spots[y * WORLD_CELLS + x] & GROUND) !== 0;
            if (!ground) {
                if (this.#state.boss) this.#snap(sid, initial);
                return STOPPED;
            }
            if (this.#priest.currentHitPoints(sid) < Math.trunc(this.#hitPoints.maxHitPoints(sid) / 2) && this.#state.boss) {
                this.#snap(sid, initial);
                return STOPPED;
            }
        }
        return this.commonHalt(sid);
    }

    // 가상 +0xcc → 단순 절삭 점 finder → 각 판본 특정 운반체 타입 몸체 → 기본 10 의 순서이다(0042a5f0). (2026-11-08 WB04-E66)
    commonHalt(sid) {
        if (this.#check.check(sid)) return CHANGED;
        const point = this.#position(sid);
        const x = cell(point[0], 0);
        const y = cell(point[1], 0);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 기본 finder 의 0단계/버킷 순서와 buried 제외를 그대로 재사용한다.
        for (let candidate = finder.begin({ left: x, top: y, right: x, bottom: y }); candidate; candidate = finder.next()) {
            const type = this.#pool.slot(candidate)[TYPE_OFFSET];
            if (candidate !== sid && type === this.#state.carrierType) {
                if (this.#hooks.carrierPickup(candidate) === CHANGED) return CHANGED;
            } else if (candidate !== sid && type === this.#state.nuggetCarrierType) {
                if (this.#hooks.nuggetTransfer(candidate) === CHANGED) return CHANGED;
            }
        }
        return MOVING;
    }
}
