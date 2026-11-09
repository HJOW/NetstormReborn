// Damageable 의 붕괴/폭발·해방 소리 접두와 공통 삭제 호출을 raw SID 풀 위에서 옮긴 모듈이다(cpppj `o/RawDamageablePreDestroy.cpp/.h`, 10.78 전용; 원본 0044b4b0).
// 순서: 객체의 extra(abstract|buried=9)가 없으면 [붕괴 효과 → 소리(그 시점 좌표) → 폭발 효과 → 소리 → 현재 좌표의 spot 이 6 비트를 가지면 내용물 순회(dead 부모 허용)에서 사제 종류마다 해방 소리
// → 권한(boss)이 있고 해방 억제 flags(0x800)가 아니면 내용물 해방] 을 하고, extra 와 무관하게 마지막에 공통 preDestroy(base)를 부른다. 효과가 raw 좌표/전역을 바꿀 수 있으므로 소리·spot·boss·사제 TYPE 은
// 효과 뒤 현재값을 다시 읽는다. extra 진입 조건은 효과 중 extra 가 바뀌어도 다시 평가하지 않는다.
//  - RawDamageablePreDestroy(pool, spots, finderState, state, hooks): spots 는 256×256 바이트(y*256+x), hooks = { collapse(sid), explosion(sid), sound(event), releaseContained(sid), validateBase(sid, flags), base(sid, flags) } 가 모두 필수.
//    sound 의 event = { sound: 'collapse' | 'explosion' | 'priestFree', x, y }(붕괴/폭발은 현재 raw 좌표, priestFree 는 전역 소리라 x=y=0).
//    validate(sid, flags): 할당 슬롯/자산 TYPE(70..187)/지원 상태 사전 검사 + (extra 없으면) 좌표 유효성 + 공통 사전 검사. preDestroy(sid, flags): 독립 typed 경로(사전 검사 뒤 실행).
//    enableNativeBody(): 실제 장기 owner 만 쓰는 원문 몸체 선택(사전 검사 생략, spot 은 감긴 Y*256+X 로 현재 물리 한 BYTE 만 확인). preDestroyNative(sid, flags, releaseContained): 해방 장치를 호출마다 받는 Native 진입(사전 검사 없음).
//    state = DamageablePreDestroyState: 사제 종속 kind(WORD)와 비교하는 DWORD(기본 158). bindPriestType({ value }) 로 실제 Client 의 현재 DWORD 를 빌리면 각 내용물 비교에서 다시 읽는다.
//  - makeDamageablePreDestroyHooks(pool, damageable, hooks): Carrier(RawCarrierPreDestroy)의 validateDamageable/damageablePre 두 경계만 같은 풀의 이 몸체로 바꾼다. 다른 풀이면 거부한다.
// 이 모듈은 DOM 을 쓰지 않는다. CD 10.72 분기는 옮기지 않았다. (2026-11-10 WB04-E251: 3-C RawDamageablePreDestroy 이식으로 추가)
import { RawContainedFinder } from './raw-contained-finder.js';

// 공통 raw 타입·상태·좌표·종속 kind 위치와 효과/해방 억제 flags, 지원 상태 마스크(free|contained, abstract|buried)이다. (2026-11-10 WB04-E251)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const KIND_OFFSET = 18;
const EXTRA_OFFSET = 40;
const COLLAPSE_FLAG = 0x200000;
const EXPLOSION_FLAG = 0x100000;
const NO_RELEASE_FLAG = 0x800;
const UNSUPPORTED = 9;
const FIRST_ASSET_TYPE = 70;
const TYPE_COUNT = 188;
const SIDE = 256;
const SPOT_BYTES = SIDE * SIDE;

// 소리 종류 이름이다. (2026-11-10 WB04-E251)
export const DAMAGEABLE_SOUND = Object.freeze({ collapse: 'collapse', explosion: 'explosion', priestFree: 'priestFree' });

// spot 좌표 fadd 상수(단정도 0.9999)이다. 두 계산 모두 double 로 더해 x87 덧셈을 보존한다. (2026-11-10 WB04-E251)
const SPOT_BIAS = Math.fround(0.9999);

// 정렬되지 않은 raw 리틀엔디언 값을 읽는다(부호 없음). (2026-11-10 WB04-E251)
function read(raw, offset, width) {
    let value = 0;
    // 원본 필드 폭만큼 낮은 바이트부터 합친다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}

// raw 좌표 float 를 비트 그대로 읽는다. (2026-11-10 WB04-E251)
const floatView = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    floatView.setUint32(0, read(raw, offset, 4), true);
    return floatView.getFloat32(0, true);
}

// 유효한 보드 좌표만 절삭한다(typed): 더하기 뒤 별도 float 반올림을 넣지 않는다. (2026-11-10 WB04-E251)
function cell(value) {
    const wide = value + SPOT_BIAS;
    if (!Number.isFinite(wide) || wide <= -1 || wide >= SIDE) throw new RangeError('Damageable spot 좌표 범위 오류');
    return Math.trunc(wide) | 0;
}

// Native _ftol 의 signed64 절삭 뒤 EAX 하위 DWORD 이다. 좌표별 0..255 자격 관문은 넣지 않는다. (2026-11-10 WB04-E251)
function nativeCell(value) {
    const wide = value + SPOT_BIAS;
    if (!Number.isFinite(wide) || wide < -9223372036854775808 || wide >= 9223372036854775808) throw new RangeError('Damageable Native spot 의 실제 CRT 절삭 표현 범위');
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(wide))));
}

// 사제 종속 kind 비교 DWORD 상태이다. 독립 입력은 priestType, 실제 Client 는 bindPriestType 으로 현재 DWORD 의 { value } 소유자를 빌린다. (2026-11-10 WB04-E251)
export class DamageablePreDestroyState {
    // 005412d0 / CD 0051cbbc: 종속 kind WORD 와 비교하는 DWORD 이다.
    priestType = 158;
    #shared = null;

    bindPriestType(owner) {
        if (owner !== null && (typeof owner !== 'object' || !('value' in owner))) throw new TypeError('Damageable 사제 TYPE 공유 소유자는 { value } 여야 합니다.');
        this.#shared = owner;
    }

    // 각 현재 내용물 비교에서 다시 읽는 사제 TYPE 이다.
    get currentPriestType() { return this.#shared ? this.#shared.value >>> 0 : this.priestType >>> 0; }
}

const REQUIRED_HOOKS = Object.freeze(['collapse', 'explosion', 'sound', 'releaseContained', 'validateBase', 'base']);

export class RawDamageablePreDestroy {
    #pool;
    #spots;
    #finderState;
    #state;
    #hooks;
    // 명시 실제 Native owner 의 선택이며 독립 typed 의 기존 계약은 기본 false 이다.
    #nativeBody = false;

    // 풀·전역·공간·훅 대상은 더 오래 살아야 한다. 공간 크기와 모든 하위 효과를 구성 때 확인한다. (2026-11-10 WB04-E251)
    constructor(pool, spots, finderState, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Damageable preDestroy 는 10.78 SidPool 만 받습니다.');
        if (spots?.length !== SPOT_BYTES || REQUIRED_HOOKS.some((name) => typeof hooks?.[name] !== 'function')) throw new TypeError('Damageable preDestroy 공간/하위 효과 누락');
        this.#pool = pool;
        this.#spots = spots;
        this.#finderState = finderState;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 같은 풀의 Carrier 연결에 쓰는 참조이다. (2026-11-10 WB04-E251)
    get pool() { return this.#pool; }

    // 실제 PATCH 장기 owner 만 원문 위치의 검사/효과를 선택하며 typed 선검사를 추가하지 않는다. (2026-11-10 WB04-E251)
    enableNativeBody() { this.#nativeBody = true; }

    // 자산 상태/타입과 연결된 공통 경계를 효과 전에 검사한다. Native 몸체는 원문에 없는 선검사를 하지 않는다. (2026-11-10 WB04-E251)
    validate(sid, flags) {
        if (this.#nativeBody) return;
        const raw = this.#pool.allocated(sid);
        if ((raw[STATE_OFFSET] & UNSUPPORTED) || raw[TYPE_OFFSET] < FIRST_ASSET_TYPE || raw[TYPE_OFFSET] >= TYPE_COUNT) throw new Error('Damageable preDestroy raw 자산 상태/타입 오류');
        if (!(raw[EXTRA_OFFSET] & UNSUPPORTED)) this.#spot(sid);
        this.#hooks.validateBase(sid, flags);
    }

    // 실제 map byte 는 효과 뒤 현재 raw 좌표에서 y*256+x 순서로 읽는다. (2026-11-10 WB04-E251)
    #spot(sid) {
        const raw = this.#pool.slot(sid);
        const x = cell(coordinate(raw, X_OFFSET));
        const y = cell(coordinate(raw, Y_OFFSET));
        return this.#spots[y * SIDE + x];
    }

    // 효과 뒤 현재 좌표를 선택하고 원문의 감긴 Y*256+X 가 도달한 공간 한 BYTE 만 확인한다. (2026-11-10 WB04-E251)
    #spotNative(sid) {
        const raw = this.#pool.slot(sid);
        const y = nativeCell(coordinate(raw, Y_OFFSET));
        const x = nativeCell(coordinate(raw, X_OFFSET));
        const index = (Math.imul(y, SIDE) + x) >>> 0;
        if (index >= this.#spots.length) throw new RangeError('Damageable Native spot 의 현재 flat BYTE 물리 부족');
        return this.#spots[index];
    }

    // 붕괴/폭발 효과가 raw 좌표를 바꿀 수 있어 소리 직전에 다시 읽는다. (2026-11-10 WB04-E251)
    #soundAt(sid, sound) {
        const raw = this.#pool.slot(sid);
        this.#hooks.sound({ sound, x: coordinate(raw, X_OFFSET), y: coordinate(raw, Y_OFFSET) });
    }

    // 0044b4b0: extra 의 진입 조건은 효과 중 extra 가 바뀌어도 다시 평가하지 않는다. typed 는 선검사 뒤 실행하고 Native 는 곧바로 실행한다. (2026-11-10 WB04-E251)
    preDestroy(sid, flags) {
        if (!this.#nativeBody) this.validate(sid, flags);
        this.#run(sid, flags, this.#hooks.releaseContained, this.#nativeBody);
    }

    // 선택된 실제 부모는 파편/소리보다 앞에서 spot 이나 공통 장부를 미리 읽지 않는다. 해방 장치는 호출마다 받는다. (2026-11-10 WB04-E251)
    preDestroyNative(sid, flags, releaseContained) {
        this.#run(sid, flags, releaseContained, true);
    }

    // 두 진입은 현재 extra 의 한 번 분기와 효과 뒤 현재 좌표/전역/종속 순서를 공유한다. (2026-11-10 WB04-E251)
    #run(sid, flags, releaseContained, native) {
        const hooks = this.#hooks;
        flags >>>= 0;
        if (!(this.#pool.slot(sid)[EXTRA_OFFSET] & UNSUPPORTED)) {
            if (flags & COLLAPSE_FLAG) { hooks.collapse(sid); this.#soundAt(sid, DAMAGEABLE_SOUND.collapse); }
            if (flags & EXPLOSION_FLAG) { hooks.explosion(sid); this.#soundAt(sid, DAMAGEABLE_SOUND.explosion); }
            if ((native ? this.#spotNative(sid) : this.#spot(sid)) & 6) {
                const finder = new RawContainedFinder(this.#pool, this.#finderState);
                // yesIKnow=1 로 dead 부모도 허용한다. 소리 콜백 뒤 현재 kind/타입 전역을 다시 읽는다.
                for (let current = finder.begin(sid, true); current; current = finder.next()) {
                    if (read(this.#pool.slot(current), KIND_OFFSET, 2) === this.#state.currentPriestType) hooks.sound({ sound: DAMAGEABLE_SOUND.priestFree, x: 0, y: 0 });
                }
            }
            if (this.#finderState.currentBoss && !(flags & NO_RELEASE_FLAG)) {
                if (typeof releaseContained !== 'function') throw new Error('Damageable Native 삭제의 선택된 실제 해방 장치 미연결');
                releaseContained(sid);
            }
        }
        hooks.base(sid, flags);
    }
}

// 전역 후처리 훅은 Carrier 에 남기고 Damageable 두 경계(사전 검사·삭제 준비)만 같은 풀의 이 몸체로 바꾼다. (2026-11-10 WB04-E251)
export function makeDamageablePreDestroyHooks(pool, damageable, hooks) {
    if (pool !== damageable?.pool) throw new TypeError('Carrier/Damageable preDestroy 의 SID 풀이 다릅니다');
    return {
        ...hooks,
        validateDamageable: (sid, flags) => damageable.validate(sid, flags),
        damageablePre: (sid, flags) => damageable.preDestroy(sid, flags),
    };
}
