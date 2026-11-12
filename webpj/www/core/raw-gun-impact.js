// 투사체 충돌 효과(anim 160)의 생성·공간 Pop·Regular 예약과 프레임/삭제 처리기를 옮긴 모듈이다(cpppj `o/RawGunImpact.cpp/.h`, 10.78 전용: 0045a750 Visual·00470ac0 Ground·0045a6e0 anim Handle·0041d0d0 좌표 제한).
//  - new RawGunImpact(pool, frames, setter, random, state, hooks): frames 는 188칸 FrameCodeTable(타입별 A..P 구간), setter 는 같은 풀의 SquidFrame, random 은 공유 GameRandom, state 는 GunImpactState,
//    hooks = { processCount()→Kernel 등록 개수, create(type, flags)→SID, pop(sid, x, y, flags), regular(sid, event, payload), destroy(sid, flags), notifyDudes(x, y) } 이다. 선택하지 않은 경계는 요구하지 않고 선택된 누락은 앞선 쓰기를 남기지 않고 거부한다.
//  - GunImpactState: { suppressedTime(005c85f0 억제 double), suppressed(005c89b8 억제 DWORD), animationType(160 고정 검사), sharedSuppressedTime(`{ value }` 상자 — 실제 장기 억제 double 을 빌리면 지역 값은 읽지도 쓰지도 않는다) }.
//    `currentSuppressedTime` 접근자가 소유자를 고른다.
//  - static clamp(x, y): 0041d0d0 — 하한 1 로 올리고 상한 255 로 내리되 NaN 은 그대로 보존한다(10.78).
//  - visual(x, y, variant, interval): 0045a750 — 억제(double 이 0 이 아니거나 NaN 이거나 DWORD 비영)면 아무것도 하지 않고, Kernel 개수가 30001 이상이면 반환, 글자 구간 조회 → 좌표 제한 → anim 160 생성(타입이 160 이 아니거나 가상 표가 anim 이 아니면 거부) →
//    첫 프레임을 DWORD 로 쓴 뒤 Pop(flags 0) → Regular 예약(variant, interval) 순서이다.
//  - ground(x, y): 00470ac0 — 좌표 제한 → 주변 Dude 반응 → 공유 난수 100 소비 → variant 3·간격 0.02 의 visual.
//  - handle(sid, event, count, payload): 0045a6e0 — 현재 가상 표가 anim 이어야 하며, 현재 타입의 글자 구간에서 signed count 가 구간 안이면 SquidFrame.set(첫 번호 + count, 0) 후 payload, 아니면 Destroy(0)을 부르고 −1 을 돌려준다.
//    static handleIn 은 같은 본문이며 Native 장기 소비자가 생성/난수 상태 없이 쓴다. handles(sid)는 raw 가상 표 0x509758 로 판정(타입 번호로 추측하지 않음). handler(fallback)는 anim 만 분배하고 다른 부모는 fallback 으로 넘긴다(없으면 거부).
//  - makeGunImpactRuntimeHooks(pool, factory, kernel, host, hooks): 실제 Factory/Kernel/Host 를 생성·개수·Regular(flags 0x50) 경계에 연결(풀·Kernel 용량이 다르면 거부), makeMissileGunImpactHooks(impact, hooks): Missile 의 impact/hitEffect 를 visual/ground 로 연결,
//    makeAssetGunImpactHooks(impact, hooks): 자산 Damageable 의 damageGround 를 ground 로 연결한다(모두 새 묶음 반환, 원래 묶음 불변).
// 10.72(CD)의 BYTE 프레임과 한도(3001)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E363: 7-P2 RawGunImpact 이식으로 추가)
import { PROCESS_CAPACITY } from './kernel.js';

// 실제 anim 가상 표(10.78)와 프로세스 생성 한도, anim 프레임 DWORD·가상 표·TYPE 슬롯 위치, 지면 효과의 Regular 간격(0.02f)이다. (2026-11-13 WB04-E363)
const ANIM_TABLE = 0x509758;
const PRODUCER_LIMIT = 30001;
const FRAME_AT = 36;
const TYPE_AT = 10;
const GROUND_VARIANT = 3;
const GROUND_INTERVAL = Math.fround(0.02);
const ANIMATION_TYPE = 160;
const TYPE_COUNT = 188;
const LETTER_COUNT = 16;

// 비정렬 리틀엔디언 DWORD 를 읽는다. (2026-11-13 WB04-E363)
function readDword(raw, at) {
    let value = 0;
    // 지정된 네 바이트만 낮은 자리부터 조합한다.
    for (let index = 0; index < 4; index += 1) value += raw[at + index] * 2 ** (8 * index);
    return value;
}

// 원본 첫 프레임 쓰기와 같은 DWORD 폭으로 프레임(signed)을 저장한다(padding·상태는 보존). (2026-11-13 WB04-E363)
function writeInitialFrame(raw, frame) {
    const bits = frame >>> 0;
    // 낮은 바이트부터 네 바이트만 쓴다.
    for (let index = 0; index < 4; index += 1) raw[FRAME_AT + index] = (bits >>> (8 * index)) & 0xff;
}

// 원본 시각 효과 생성기가 읽는 두 억제 전역과 실제 anim 타입 번호이다. (2026-11-13 WB04-E363)
export class GunImpactState {
    // 005c85f0 의 현재 double 억제 값(독립 호출만 소유), 005c89b8 의 추가 억제 DWORD, 양판본 실제 anim 자산 번호이다.
    suppressedTime = 0;
    suppressed = 0;
    animationType = ANIMATION_TYPE;
    // 실제 장기 005c85f0 을 빌리는 `{ value }` 상자이다. 없으면 위의 지역 값을 쓴다.
    sharedSuppressedTime = null;

    // 지정한 값만 덮어쓴다. (2026-11-13 WB04-E363)
    constructor(init = {}) { Object.assign(this, init); }

    // 소비 위치마다 현재 소유자의 double 을 읽는다(사본을 만들지 않음). (2026-11-13 WB04-E363)
    get currentSuppressedTime() { return this.sharedSuppressedTime ? this.sharedSuppressedTime.value : this.suppressedTime; }

    // 같은 소유자에게 쓴다(지역 fallback 을 덮지 않음). (2026-11-13 WB04-E363)
    set currentSuppressedTime(value) {
        if (this.sharedSuppressedTime) this.sharedSuppressedTime.value = value;
        else this.suppressedTime = value;
    }
}

// 포구 효과와 구별하는 실제 충돌/지면 효과 생성 및 anim Handle 처리기이다. (2026-11-13 WB04-E363)
export class RawGunImpact {
    #pool;
    #frames;
    #setter;
    #random;
    #state;
    #hooks;

    // PATCH 풀·같은 풀의 지정기·188칸 프레임 표·anim 타입 160 을 확인한다. (2026-11-13 WB04-E363)
    constructor(pool, frames, setter, random, state, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('충돌 효과는 PATCH 만 복원했습니다');
        if (setter?.pool !== pool) throw new TypeError('충돌 효과/프레임 지정기 SID 풀 불일치');
        if (!Array.isArray(frames) || frames.length !== TYPE_COUNT) throw new RangeError('충돌 효과 프레임 표 크기 오류');
        if (state.animationType !== ANIMATION_TYPE) throw new RangeError('충돌 효과 anim 타입 오류');
        this.#pool = pool;
        this.#frames = frames;
        this.#setter = setter;
        this.#random = random;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 어댑터 구성 때 풀 혼용을 거부하기 위한 읽기 참조이다. (2026-11-13 WB04-E363)
    get pool() { return this.#pool; }

    // 0041d0d0: 좌표를 하한 1·상한 255 로 제한한다. 10.78 은 상한 비교가 거짓인 NaN 을 255 로 바꾸지 않고 보존한다. (2026-11-13 WB04-E363)
    static clamp(x, y) {
        let a = x < 1 ? 1 : x;
        let b = y < 1 ? 1 : y;
        if (a > 255) a = 255;
        if (b > 255) b = 255;
        return [a, b];
    }

    // 현재 타입의 'A'+event 글자 구간(첫 번호·개수)을 읽는다. (2026-11-13 WB04-E363)
    #run(type, event) {
        if (type >= this.#frames.length || event >= LETTER_COUNT) throw new RangeError('충돌 효과 실제 글자 구간 오류');
        return this.#frames[type].run(String.fromCharCode(0x41 + event));
    }

    // 0045a750: 억제와 프로세스 한도에서는 원본처럼 생성·좌표 쓰기 없이 반환한다. (2026-11-13 WB04-E363)
    visual(x, y, variant, interval) {
        const state = this.#state;
        const hooks = this.#hooks;
        if (state.currentSuppressedTime !== 0 || state.suppressed) return;
        if (!hooks.processCount) throw new Error('충돌 효과 실제 Kernel 개수 조회 누락');
        if (hooks.processCount() >= PRODUCER_LIMIT) return;
        const run = this.#run(state.animationType, variant);
        if (!hooks.create || !hooks.pop || !hooks.regular) throw new Error('충돌 효과 실제 생성/Pop/Regular 누락');
        const position = RawGunImpact.clamp(x, y);
        const sid = hooks.create(state.animationType, 2);
        if (!this.handles(sid) || this.#pool.slot(sid)[TYPE_AT] !== state.animationType) throw new Error('충돌 효과 Factory 실제 anim 생성 오류');
        writeInitialFrame(this.#pool.allocated(sid), run.first);
        hooks.pop(sid, position[0], position[1], 0);
        hooks.regular(sid, variant, interval);
    }

    // 00470ac0: Dude 의 실제 반응은 독립 하위 경계이며 누락을 묵시 성공으로 처리하지 않는다. (2026-11-13 WB04-E363)
    ground(x, y) {
        if (!this.#hooks.notifyDudes) throw new Error('충돌 효과 실제 주변 Dude 반응 미복원');
        const position = RawGunImpact.clamp(x, y);
        this.#hooks.notifyDudes(position[0], position[1]);
        this.#random.next(100);
        this.visual(position[0], position[1], GROUND_VARIANT, GROUND_INTERVAL);
    }

    // 0045a6e0: 현재 실제 anim 가상 표만 지원하며 count DWORD 를 signed int 로 해석한다. (2026-11-13 WB04-E363)
    handle(sid, event, count, payload) {
        if (!this.handles(sid)) throw new TypeError('충돌 효과 실제 anim Handle 가상 표 오류');
        return RawGunImpact.handleIn(this.#pool, this.#frames, this.#setter, this.#hooks.destroy, sid, event, count, payload);
    }

    // 현재 TYPE 의 글자 구간과 signed count 를 읽고 실제 프레임 지정 또는 현재 가상 삭제만 선택한다. 장기 Native 소비자는 생성/난수 상태 없이 이 본문만 쓴다. (2026-11-13 WB04-E363)
    static handleIn(pool, frames, setter, destroy, sid, event, count, payload) {
        const type = pool.slot(sid)[TYPE_AT];
        if (type >= frames.length || event >= LETTER_COUNT) throw new RangeError('충돌 효과 실제 글자 구간 오류');
        const run = frames[type].run(String.fromCharCode(0x41 + event));
        const index = count | 0;
        if (index >= 0 && index < run.count) {
            setter.set(sid, (run.first + count) | 0, 0);
            return payload;
        }
        if (!destroy) throw new Error('충돌 효과 실제 anim 종료 삭제 누락');
        destroy(sid, 0);
        return -1;
    }

    // 주소를 호스트에서 역참조하지 않고 현재 raw 기록값으로 anim 가상 표인지 확인한다. (2026-11-13 WB04-E363)
    handles(sid) {
        return Boolean(sid) && sid < this.#pool.capacity && readDword(this.#pool.slot(sid), 0) === ANIM_TABLE;
    }

    // 현재 anim 만 분배하고 다른 부모의 기존 처리기를 보존한다(처리기가 없으면 거부). (2026-11-13 WB04-E363)
    handler(fallback = null) {
        return (sid, event, count, payload) => {
            if (this.handles(sid)) return this.handle(sid, event, count, payload);
            if (!fallback) throw new Error('충돌 효과 외 실제 Handle 미연결');
            return fallback(sid, event, count, payload);
        };
    }
}

// 같은 실제 Factory/Kernel/Host 를 효과 생성 경계에 연결하며 공간·Dude·삭제는 호출자의 훅을 보존한다(새 묶음 반환, 풀·Kernel 용량이 다르면 거부). (2026-11-13 WB04-E363)
export function makeGunImpactRuntimeHooks(pool, factory, kernel, host, hooks) {
    if (factory.pool !== pool || host.pool !== pool || kernel.capacity !== PROCESS_CAPACITY) throw new TypeError('충돌 효과 실제 Factory/Host/Kernel 판본 오류');
    return {
        ...hooks,
        processCount: () => kernel.size,
        create: (type, flags) => factory.create(type, flags),
        regular: (sid, event, payload) => {
            if (!host.addRegular(sid, event, payload, 0x50)) throw new Error('충돌 효과 실제 Regular 부착 실패');
        },
    };
}

// barrier 와 일반 표적 충돌을 각각 실제 생산/지면 몸체에 연결한다. (2026-11-13 WB04-E363)
export function makeMissileGunImpactHooks(impact, hooks) {
    return {
        ...hooks,
        impact: (x, y, variant, interval) => { impact.visual(x, y, variant, interval); },
        hitEffect: (x, y) => { impact.ground(x, y); },
    };
}

// 기존 공통 Damageable 의 실제 지면 피해 사건도 같은 난수/생성 사슬을 소비하게 한다. (2026-11-13 WB04-E363)
export function makeAssetGunImpactHooks(impact, hooks) {
    return { ...hooks, damageGround: (x, y) => { impact.ground(x, y); } };
}
