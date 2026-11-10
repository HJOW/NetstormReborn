// Damageable 삭제 준비에서 권한(boss) 확인 뒤 수행하는 종속 해방 구간을 raw SID 풀 위에서 옮긴 모듈이다(cpppj `o/RawDamageableRelease.cpp/.h`, 10.78 전용; 원본 0044b626~0044b80c).
// 순서: 부모 TYPE 의 발자국 X 가 1 이면 현재 raw 좌표, 아니면 발자국 중심(좌표 − 발자국×0.5 를 float 로 저장)을 구해 각 축에 단정도 0.99999 를 더해 절삭한 정수를 float 로 고정한다(이 점은 순회 내내 유지).
//   내용물 순회(dead 부모 허용, 반환 전에 저장한 next 사용)에서 후보마다 [mask 하위 BYTE 의 0x20(해방 가능) → 종속 kind WORD → 전투가 아니면 kind 의 genus(flags2)와 현재 허용 genus DWORD 교집합]을 확인한다.
//   - 사제 genus(0x200000): 부모의 현재 좌표(부모 TYPE genus 가 0x80400000 비트를 가지면 발자국 중심 + 0.5f)와 kind·후보 WORD 전체를 사제 생성(spawnPriest)에 넘긴다.
//   - 그 밖: 고정한 점의 spot(단정도 0.9999 를 더해 절삭한 칸)이 0x10 을 갖거나 6 비트가 없으면 건너뛴다. 아니면 생성(kind, 0) → 생성 **뒤** 후보의 현재 WORD 를 새 슬롯에 복사 → 새 슬롯의 현재 TYPE BYTE 가
//     현재 다리 TYPE DWORD 와 같으면 표면 알림 → 소유자 지정(new, 0) → Pop(new, 점X, 점Y, 0).
//   하위 효과가 raw·전역·공간을 바꿀 수 있으므로 battle·허용 genus·다리 TYPE·부모 좌표/TYPE·spot·후보 WORD 는 각 소비 위치에서 현재값을 다시 읽는다.
//  - RawDamageableRelease(pool, types, spots, finderState, state, hooks): types 는 188칸 TYPE 표(참조; footX·footY·flags2 를 현재값으로 읽음), spots 는 256×256 Uint8Array(y*256+x),
//    hooks = { spawnPriest(x, y, kind, word), create(kind, flags)→SID, setOwner(sid, owner), pop(sid, x, y, flags), notifySurface(sid) } 가 모두 필수(makePriestSpawnHooks 로 spawnPriest 를 실제 나선 생성에 연결).
//    release(parent): 독립 typed 경로. 할당 슬롯·상태(free|contained 거부)·자산 TYPE 하한(70)을 먼저 확인하고 발자국(1..256)·좌표(−1 < 값 < 256) 범위를 거부한다. battle·다리 TYPE 은 state 필드의 현재값이다.
//    releaseNative(parent, battle, bridgeType): 실제 장기 owner 진입(cpppj 기준선 뒤 230528e3). battle·bridgeType 은 현재 전역을 돌려주는 getter 함수이며 부모 자격·발자국·축별 보드 범위 선검사를 하지 않는다.
//      좌표는 signed64 CRT 절삭 뒤 하위 DWORD(스냅은 signed 로 다시 해석해 float 저장), spot 은 감긴 Y*256+X 의 현재 물리 한 BYTE 만 확인한다.
//    state = DamageableReleaseState: battle(00594fbc)·allowedGenus(00542640, 초기 0x40480002)·bridgeType(005411a0, 82). bindAllowedGenus({ value }) 로 실제 Client 의 현재 DWORD 를 빌리면 후보마다 다시 읽는다.
//  - makeDamageableReleaseHooks(pool, release, hooks): Damageable preDestroy 훅 묶음의 releaseContained 만 같은 풀의 이 몸체(typed)로 바꾼다. 다른 풀이면 거부한다.
// 이 모듈은 DOM 을 쓰지 않는다. CD 10.72 분기(타입 171칸·별도 중심/공간 조회)는 옮기지 않았다. (2026-11-10 WB04-E252: 3-C RawDamageableRelease 이식으로 추가)
import { RawContainedFinder } from './raw-contained-finder.js';

// 공통 raw 필드 위치(TYPE·상태·원래 소유자 WORD·좌표·종속 kind·mask)와 해방 후보(0x20)/사제 genus/부모 중심 보정 genus 마스크, 부모 거부 상태(free|contained), 첫 자산 TYPE, TYPE 표 길이, 공간 한 변이다. (2026-11-10 WB04-E252)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const WORD_OFFSET = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const KIND_OFFSET = 18;
const MASK_OFFSET = 20;
const RELEASABLE = 0x20;
const PRIEST_GENUS = 0x200000;
const CENTERED_PARENT = 0x80400000;
const UNSUPPORTED = 9;
const FIRST_ASSET_TYPE = 70;
const TYPE_COUNT = 188;
const SIDE = 256;
const CELLS = SIDE * SIDE;

// 좌표 스냅은 단정도 0.99999, 공간 조회는 단정도 0.9999 이다. 둘 다 double 로 더해 x87 덧셈을 보존하고 중간 float 반올림을 넣지 않는다. (2026-11-10 WB04-E252)
const SNAP_BIAS = Math.fround(0.99999);
const SPOT_BIAS = Math.fround(0.9999);

// signed64 CRT 절삭의 표현 범위(±2^63)이다. (2026-11-10 WB04-E252)
const CRT_LIMIT = 9223372036854775808;

const REQUIRED_HOOKS = Object.freeze(['spawnPriest', 'create', 'setOwner', 'pop', 'notifySurface']);

// 정렬되지 않은 raw 리틀엔디언 값을 읽는다(부호 없음). (2026-11-10 WB04-E252)
function read(raw, offset, width) {
    let value = 0;
    // 원본 필드 폭만큼 낮은 바이트부터 합친다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}

// raw 좌표 float 를 비트 그대로 읽는다. (2026-11-10 WB04-E252)
const floatView = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    floatView.setUint32(0, read(raw, offset, 4), true);
    return floatView.getFloat32(0, true);
}

// typed: x87 덧셈 뒤 CRT 절삭 결과를 유효한 공간 인덱스(−1 < 값 < 256)로 제한한다. (2026-11-10 WB04-E252)
function cell(value, bias) {
    const wide = value + bias;
    if (!Number.isFinite(wide) || wide <= -1 || wide >= SIDE) throw new RangeError('Damageable 해방 좌표 범위 오류');
    return Math.trunc(wide) | 0;
}

// Native: 실제 signed64 CRT 절삭 뒤 하위 DWORD 를 다시 signed 로 해석한다. 축별 보드 범위 선검사를 추가하지 않는다. (2026-11-10 WB04-E252)
function nativeCell(value, bias) {
    const wide = value + bias;
    if (!Number.isFinite(wide) || wide < -CRT_LIMIT || wide >= CRT_LIMIT) throw new RangeError('Damageable Native 해방의 실제 CRT 절삭 표현 범위');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(wide))));
}

// 해방 처리 중 조회하는 전역이다: 전투(00594fbc; 켜지면 허용 genus 검사를 건너뜀), 허용 genus 마스크(00542640 초기 0x40480002), 다리 TYPE(005411a0; WORD 쓰기 뒤 표면 알림 대상).
// 독립 입력은 필드를 직접 쓰고, 실제 Client 는 bindAllowedGenus 로 현재 DWORD 의 { value } 소유자를 빌린다. (2026-11-10 WB04-E252)
export class DamageableReleaseState {
    battle = false;
    allowedGenus = 0x40480002;
    bridgeType = 82;
    #shared = null;

    // 실제 Client 의 00542640 을 빌린다(null 이면 해제). (2026-11-10 WB04-E252)
    bindAllowedGenus(owner) {
        if (owner !== null && (typeof owner !== 'object' || !('value' in owner))) throw new TypeError('Damageable 해방 허용 genus 공유 소유자는 { value } 여야 합니다.');
        this.#shared = owner;
    }

    // 가상 효과 뒤 후보마다 다시 읽는 현재 전체 DWORD 이다.
    get currentAllowedGenus() { return (this.#shared ? this.#shared.value : this.allowedGenus) >>> 0; }
}

export class RawDamageableRelease {
    // 객체 슬롯 풀, TYPE 표, spot 지도, 내용물 순회 전역, 해방 전역, 하위 효과이다.
    #pool;
    #types;
    #spots;
    #finderState;
    #state;
    #hooks;

    // 풀·TYPE 표·공간·현재 전역은 이 객체보다 오래 살아야 한다. 표 길이·공간 크기·모든 하위 효과를 구성 때 확인한다. (2026-11-10 WB04-E252)
    constructor(pool, types, spots, finderState, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Damageable 해방은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || spots?.length !== CELLS || REQUIRED_HOOKS.some((name) => typeof hooks?.[name] !== 'function')) {
            throw new TypeError('Damageable 해방 자료/하위 효과 누락');
        }
        this.#pool = pool;
        this.#types = types;
        this.#spots = spots;
        this.#finderState = finderState;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 같은 풀의 Damageable 훅 연결을 검사할 때 쓰는 참조이다. (2026-11-10 WB04-E252)
    get pool() { return this.#pool; }

    // 발자국·genus 를 현재 표에서 조회하며 TYPE 번호 범위를 검사한다(종속 kind 는 WORD 전체로 조회). (2026-11-10 WB04-E252)
    #type(number) {
        if (number >= this.#types.length) throw new RangeError('Damageable 해방 타입 범위 오류');
        return this.#types[number];
    }

    // 부모의 현재 raw 좌표 두 개를 비트 그대로 읽는다. (2026-11-10 WB04-E252)
    #position(parent) {
        const raw = this.#pool.slot(parent);
        return [coordinate(raw, X_OFFSET), coordinate(raw, Y_OFFSET)];
    }

    // 실제 중심 함수: 현재 TYPE 발자국의 절반을 빼고 각 좌표를 float 로 저장한다. typed 만 발자국 1..256 을 확인한다. (2026-11-10 WB04-E252)
    #center(parent, native) {
        const type = this.#type(this.#pool.slot(parent)[TYPE_OFFSET]);
        if (!native && (type.footX < 1 || type.footY < 1 || type.footX > SIDE || type.footY > SIDE)) throw new RangeError('Damageable 해방 발자국 범위 오류');
        const point = this.#position(parent);
        return [Math.fround(point[0] - type.footX * 0.5), Math.fround(point[1] - type.footY * 0.5)];
    }

    // 고정한 점의 현재 공간 BYTE 이다. typed 는 유효 칸의 y*256+x, Native 는 감긴 Y*256+X 가 도달한 물리 한 BYTE 만 확인한다. (2026-11-10 WB04-E252)
    #spot(point, native) {
        if (native) {
            const y = nativeCell(point[1], SPOT_BIAS) >>> 0;
            const x = nativeCell(point[0], SPOT_BIAS) >>> 0;
            const index = (Math.imul(y, SIDE) + x) >>> 0;
            if (index >= this.#spots.length) throw new RangeError('Damageable Native 해방의 현재 flat BYTE 물리 부족');
            return this.#spots[index];
        }
        return this.#spots[cell(point[1], SPOT_BIAS) * SIDE + cell(point[0], SPOT_BIAS)];
    }

    // 0044b626~0044b80c: 상위에서 확인한 boss/extra/flags 를 다시 평가하지 않는다. typed 는 부모 자격을 먼저 확인한다. (2026-11-10 WB04-E252)
    release(parent) {
        const raw = this.#pool.allocated(parent);
        if ((raw[STATE_OFFSET] & UNSUPPORTED) || raw[TYPE_OFFSET] < FIRST_ASSET_TYPE) throw new Error('Damageable 해방 부모 상태 오류');
        const state = this.#state;
        this.#run(parent, () => state.battle, () => state.bridgeType, false);
    }

    // 실제 44b626 은 진입한 뒤 현재 TYPE 을 선택하며 이미 죽은 부모의 자격을 새로 판정하지 않는다. battle·bridgeType 은 현재 전역 getter 이다. (2026-11-10 WB04-E252)
    releaseNative(parent, battle, bridgeType) {
        if (typeof battle !== 'function' || typeof bridgeType !== 'function') throw new TypeError('Damageable Native 해방의 현재 battle/다리 TYPE getter 미연결');
        this.#run(parent, battle, bridgeType, true);
    }

    // 두 진입이 공유하는 후행 몸체이다. 현재 부모/후보/전역을 원문 소비 위치에서 읽는다. (2026-11-10 WB04-E252)
    #run(parent, battle, bridgeType, native) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const first = this.#type(pool.slot(parent)[TYPE_OFFSET]).footX === 1 ? this.#position(parent) : this.#center(parent, native);
        // 스냅한 정수는 float 로 저장되며 이후 일반 자산의 위치로 고정된다.
        const point = first.map((value) => Math.fround(native ? nativeCell(value, SNAP_BIAS) : cell(value, SNAP_BIAS)));
        const finder = new RawContainedFinder(pool, this.#finderState);
        // 반환 전 저장된 next 를 쓰므로 현재 후보의 연결이 바뀌어도 원래 다음 후보로 간다.
        for (let current = finder.begin(parent, true); current; current = finder.next()) {
            if (!(pool.slot(current)[MASK_OFFSET] & RELEASABLE)) continue;
            const kind = read(pool.slot(current), KIND_OFFSET, 2);
            if (!battle() && !(this.#type(kind).flags2 & this.#state.currentAllowedGenus)) continue;
            if (this.#type(kind).flags2 & PRIEST_GENUS) {
                let position = this.#position(parent);
                if (this.#type(pool.slot(parent)[TYPE_OFFSET]).flags2 & CENTERED_PARENT) {
                    const center = this.#center(parent, native);
                    position = [Math.fround(center[0] + 0.5), Math.fround(center[1] + 0.5)];
                }
                hooks.spawnPriest(position[0], position[1], kind, read(pool.slot(current), WORD_OFFSET, 2));
            } else {
                const spot = this.#spot(point, native);
                if ((spot & 0x10) || !(spot & 6)) continue;
                const created = hooks.create(kind, 0);
                // 생성 효과 뒤 후보의 현재 WORD 전체를 새 슬롯에 쓴다.
                const word = read(pool.slot(current), WORD_OFFSET, 2);
                const target = pool.allocated(created);
                target[WORD_OFFSET] = word & 0xff;
                target[WORD_OFFSET + 1] = word >>> 8;
                if (target[TYPE_OFFSET] === (bridgeType() >>> 0)) hooks.notifySurface(created);
                hooks.setOwner(created, 0);
                hooks.pop(created, point[0], point[1], 0);
            }
        }
    }
}

// 상위에서 이미 결정한 해방 진입을 같은 풀의 실제 몸체로 전달한다. 다른 Damageable 효과는 유지한다. (2026-11-10 WB04-E252)
export function makeDamageableReleaseHooks(pool, release, hooks) {
    if (pool !== release?.pool) throw new TypeError('Damageable 해방의 SID 풀이 다릅니다');
    return { ...hooks, releaseContained: (sid) => release.release(sid) };
}
