// 사제/Carrier 의 HP·지면·extra 기반 이동 불가 상태 조회와 사제 HP setter(004942b0)를 옮긴 모듈이다(cpppj `o/RawPriestState.cpp/.h`, 10.78 전용).
//  - currentHitPoints: 10.78 은 HP DWORD(+26)를 부호 있는 수로 읽는다(최대 HP 모드와 별개인 raw 값).
//  - groundImmobile(00492090): genus 0x200000(사제 부류)이 아니면 거짓, HP 가 최대 HP/2(0 방향 절삭) 미만이면 지면 좌표를 읽지 않고 참, 아니면 거의 올림(+0.9999)한 지면 spot & 6 이 없거나 extra 0x20 이 있으면 참이다.
//  - immobile(00427030): extra 0x20 이면 HP/좌표를 조회하지 않고 바로 참이다.
//  - setHitPoints(004942b0): 절반 HP 전환의 권한/소유자 허용을 보고 Unpop → (새 HP 가 저HP 면 좌표를 0.99999 배율로 정수 보정) → Repop 한다. 전체 사제 Pop 을 대신하지 않으며 두 가상 공간 효과는 호출자가 공급한다.
// 현재 TYPE 표는 실제 PATCH World 의 주소 고정 표를 빌릴 수 있다(bindCurrentTypes). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E62: 사제 상태 이식으로 추가)

import { PRIEST_TYPE, PRIEST_VTABLE } from './raw-priest-owner.js';

// 사제 TYPE/상태/좌표/HP 슬롯 위치와 사제 부류 비트, 이동 불가 비트, 지면 지지 비트, 무효 상태 비트, 지면 조회/HP 전환 좌표 배율(float32)이다. (2026-11-08 WB04-E62)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const HP_OFFSET = 26;
const OWNER_OFFSET = 34;
const EXTRA_OFFSET = 40;
const PRIEST_GENUS = 0x200000;
const FORCED_IMMOBILE = 0x20;
const SUPPORTED_GROUND = 6;
const INVALID_STATE = 9;
const COORDINATE_BIAS = Math.fround(0.9999);
const SNAP_BIAS = Math.fround(0.99999);
const WORLD_CELLS = 256;
const FIRST_ASSET_TYPE = 70;
const PLAYER_COUNT = 8;
const TYPE_COUNT = 188;
const PLAYER_STRIDE = 0xb4;
const PLAYER_TEMPLE_FIELD = 0x7c;

// 정렬되지 않은 DWORD 를 리틀엔디언으로 읽는다. (2026-11-08 WB04-E62)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 단정도 float 비트 변환이다. (2026-11-08 WB04-E62)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// float 값의 단정도 비트이다. (2026-11-08 WB04-E62)
function bitsOf(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// x87 의 float+float 중간 합을 float 으로 좁히지 않고 0 방향 절삭한다. 지도 밖/비유한 좌표는 접근 전에 거부한다. (2026-11-08 WB04-E62)
function cell(coordinate) {
    const shifted = coordinate + COORDINATE_BIAS;
    if (!Number.isFinite(coordinate) || shifted < 0 || shifted >= WORLD_CELLS) throw new RangeError('사제 지면 조회 좌표 범위 오류');
    return Math.trunc(shifted);
}

// 사제 체력 좌표 모드이다: authority(00540bc4) 권한, 회복 허용 배열(Player+0x7c 의 0 아님), 실제 Client boss 를 빌리는 getter(sharedAuthority), 실제 Player 9 기록(playerRecords)이다. (2026-11-08 WB04-E62)
export class PriestHitPointMode {
    authority = true;
    recoveryAllowed = new Array(PLAYER_COUNT + 1).fill(false);
    sharedAuthority = null;
    playerRecords = new Uint8Array(0);

    // HP·중립 회복의 원문 조회 위치에서 현재 권한을 읽는다. (2026-11-08 WB04-E62)
    isAuthority() {
        return this.sharedAuthority ? Boolean(this.sharedAuthority()) : this.authority;
    }

    // 원문 회복 분기에서 현재 신전 SID DWORD 의 0 아님을 읽는다(Player 9 기록이 있으면 owner*0xb4+0x7c, 없으면 독립 배열). SID 유효성·하위 WORD 는 따로 검사하지 않는다. (2026-11-08 WB04-E62)
    isRecoveryAllowed(owner) {
        if (owner > PLAYER_COUNT) throw new RangeError('사제 HP 회복 Player 번호 오류');
        if (this.playerRecords.length === 0) return this.recoveryAllowed[owner];
        if (this.playerRecords.length !== (PLAYER_COUNT + 1) * PLAYER_STRIDE) throw new RangeError('사제 HP 회복 Player9 기록 폭 오류');
        return readDword(this.playerRecords, owner * PLAYER_STRIDE + PLAYER_TEMPLE_FIELD) !== 0;
    }
}

// pool 은 10.78 SidPool, hp 는 같은 풀의 SquidReward(maxHitPoints(sid)), types 는 188칸 TYPE 표(복사; flags2), spots 는 65536칸 spot 지도(읽기)이다. (2026-11-08 WB04-E62)
export class RawPriestState {
    // 객체 슬롯 풀, 최대 HP 처리기, 복사한 타입 표, 실제 World 의 현재 TYPE 표(없으면 null), spot 지도이다.
    #pool;
    #hp;
    #types;
    #currentTypes = null;
    #spots;

    // 풀 불일치와 타입/지도 크기 오류는 조회 전에 확인한다. (2026-11-08 WB04-E62)
    constructor(pool, hp, types, spots) {
        if (pool?.edition !== 'patch1078' || hp?.pool !== pool || !Array.isArray(types) || types.length !== TYPE_COUNT
            || !(spots instanceof Uint8Array) || spots.length !== WORLD_CELLS * WORLD_CELLS) {
            throw new TypeError('사제 상태 조회 풀/타입/지도 연결 오류');
        }
        this.#pool = pool;
        this.#hp = hp;
        this.#types = types.map((record) => ({ ...record }));
        this.#spots = spots;
    }

    // 후속 회복 처리기의 같은 풀 연결을 검사하는 풀이다. (2026-11-08 WB04-E62)
    get pool() {
        return this.#pool;
    }

    // 실제 PATCH 의 주소 고정 TYPE 표(188칸)를 빌린다. 빈 표(또는 null)는 생성 시 사본으로 복귀한다. 잘못된 대여는 기존 연결을 바꾸기 전에 거부한다. (2026-11-08 WB04-E62)
    bindCurrentTypes(types) {
        if (types && types.length > 0 && types.length !== TYPE_COUNT) throw new RangeError('사제 상태 현재 TYPE 대여의 판본/길이 오류');
        this.#currentTypes = types && types.length > 0 ? types : null;
    }

    // 지면 조회의 genus 판정 시 읽는 현재 표이다(빈 대여는 소유 사본). (2026-11-08 WB04-E62)
    #currentTypesOrCopy() {
        return this.#currentTypes ?? this.#types;
    }

    // 정상 자산 SID/타입을 검증하고 슬롯 사본을 돌려준다: 예약/풀 밖 번호, free/contained 비트(9), 자산이 아닌 TYPE 은 거부하되 dead/void/abstract 는 추가로 제외하지 않는다. (2026-11-08 WB04-E62)
    #raw(sid) {
        if (sid < 5 || sid >= this.#pool.capacity) throw new RangeError('사제 상태 조회 SID 오류');
        const raw = this.#pool.slot(sid);
        if ((raw[STATE_OFFSET] & INVALID_STATE) || raw[TYPE_OFFSET] < FIRST_ASSET_TYPE || raw[TYPE_OFFSET] >= this.#currentTypesOrCopy().length) {
            throw new TypeError('사제 상태 조회 raw 자산 오류');
        }
        return raw;
    }

    // HP DWORD(+26)를 부호 있는 수로 읽는다(최대 HP 모드와 별개인 raw 값). (2026-11-08 WB04-E62)
    currentHitPoints(sid) {
        return readDword(this.#raw(sid), HP_OFFSET) | 0;
    }

    // HP 경계는 signed 정수 최대 HP/2 의 0 방향 절삭이다. 저HP 일 때에는 지면 좌표를 읽지 않는다. (2026-11-08 WB04-E62)
    groundImmobile(sid) {
        const raw = this.#raw(sid);
        if ((this.#currentTypesOrCopy()[raw[TYPE_OFFSET]].flags2 & PRIEST_GENUS) === 0) return false;
        if (this.currentHitPoints(sid) < Math.trunc(this.#hp.maxHitPoints(sid) / 2)) return true;
        const y = cell(floatOf(readDword(raw, Y_OFFSET)));
        const x = cell(floatOf(readDword(raw, X_OFFSET)));
        return (this.#spots[y * WORLD_CELLS + x] & SUPPORTED_GROUND) === 0 || (raw[EXTRA_OFFSET] & FORCED_IMMOBILE) !== 0;
    }

    // 이 외부 wrapper 는 genus 가 사제가 아니어도 강제 비트를 먼저 검사한다. (2026-11-08 WB04-E62)
    immobile(sid) {
        const raw = this.#raw(sid);
        return (raw[EXTRA_OFFSET] & FORCED_IMMOBILE) !== 0 || this.groundImmobile(sid);
    }

    // 004942b0: 절반 HP 전환의 권한/소유자 허용, Unpop → 좌표 거의 올림 → Repop 을 복원한다. mode 는 PriestHitPointMode, hooks = { unpop(sid, flags), repop(sid, flags) }(전환이 공간에 반영될 때만 필요). (2026-11-08 WB04-E62)
    setHitPoints(sid, value, mode, hooks) {
        const raw = this.#raw(sid);
        if (raw[TYPE_OFFSET] !== PRIEST_TYPE || readDword(raw, 0) !== PRIEST_VTABLE) throw new TypeError('사제 HP 변경 타입/가상 표 오류');
        const oldHp = this.currentHitPoints(sid);
        const half = Math.trunc(this.#hp.maxHitPoints(sid) / 2);
        const stored = value | 0;
        const wasLow = oldHp < half;
        const transition = (stored < half) !== wasLow;
        const owner = raw[OWNER_OFFSET];
        // 실제로 참조하는 소유자 칸과 필요한 가상 효과만 쓰기 전에 검사한다.
        if (transition && mode.isAuthority() && wasLow && owner > PLAYER_COUNT) throw new RangeError('사제 HP 회복 소유자 오류');
        const space = transition && mode.isAuthority() && (!wasLow || mode.isRecoveryAllowed(owner));
        if (space && (typeof hooks?.unpop !== 'function' || typeof hooks.repop !== 'function')) throw new TypeError('사제 HP 변경 공간 효과 누락');
        // 원본 setter 폭 밖의 stale 바이트는 그대로 보존한다.
        const write = (hp) => {
            const target = this.#pool.allocated(sid);
            for (let index = 0; index < 4; index += 1) target[HP_OFFSET + index] = (hp >>> (8 * index)) & 0xff;
        };
        write(value);
        if (!transition) return;
        if (!mode.isAuthority()) {
            write(half - (wasLow ? 1 : 0));
            return;
        }
        if (wasLow && !mode.isRecoveryAllowed(owner)) {
            write(oldHp);
            return;
        }
        hooks.unpop(sid, 0);
        // 가상 효과 이후 HP/좌표를 다시 읽는다. 원본도 새 HP 가 저HP 일 때만 현재 위치를 거의 올림한다.
        if (this.currentHitPoints(sid) < Math.trunc(this.#hp.maxHitPoints(sid) / 2)) {
            const target = this.#pool.allocated(sid);
            // x/y 에 HP 전환 전용 배율을 더하고 원본 _ftol → float 저장으로 바꾼다.
            for (const offset of [X_OFFSET, Y_OFFSET]) {
                const coordinate = floatOf(readDword(target, offset));
                if (!Number.isFinite(coordinate) || coordinate < 0 || coordinate >= WORLD_CELLS) throw new RangeError('사제 HP 전환 좌표 범위 오류');
                const bits = bitsOf(Math.trunc(coordinate + SNAP_BIAS));
                for (let index = 0; index < 4; index += 1) target[offset + index] = (bits >>> (8 * index)) & 0xff;
            }
        }
        hooks.repop(sid, 0);
    }
}
