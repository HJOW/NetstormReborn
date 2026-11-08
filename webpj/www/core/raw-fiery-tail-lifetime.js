// Fierytail.cpp 의 생성자와 Particle.cpp 의 공유 소멸 순서를 옮긴 모듈이다(cpppj `o/RawFieryTailLifetime.cpp/.h`, 10.78 전용: 00457ea0 불꽃 생성자(process 11)·00458680 건설 마나 생성자(process 12)·004882c0 소멸).
//  - FieryTailVariant: 같은 프레임/Draw 를 공유하는 두 생성자의 차이이다. Fiery 는 수명 4·offset 0·깊이 −25, Streak 는 수명 2·offset −8·깊이 −20 이다.
//  - FieryTailRelease: scalar deleting destructor 가 구별해 반납하는 두 대상(입자 배열·객체 자체)이다. 공유 색 표는 반납하지 않는다.
//  - new RawFieryTailLifetime(pool, registry, state, hooks, variant = Fiery): registry 는 같은 월드의 `RawRenderRegistry`, state = { palette, sharedPalette?, sharedProcessType? } 는 월드 공유 저장소이다.
//    palette 는 0055a750 의 첫 생성 때 채우고 후속 효과가 재사용하는 색 표 토큰, sharedPalette 는 `{ value }` 상자로 건설 마나가 빔과 같은 0055a758 을 빌릴 때 그 실제 owner 를 직접 읽고 쓴다(없으면 자체 palette 사용),
//    sharedProcessType 은 등록 전에 읽는 현재 0054109c/a0 값을 돌려주는 함수(없으면 판본 기본값 11/12)이다. hooks = { reserve(바이트)→성공 여부(입자 2880·공유 색 표 8바이트), createPalette()→비영 토큰, buildPalette(token)(실제 8+3색 구성) } 이다.
//  - initialize(fields, token, parent): 공통 등록/부착(TYPE 는 등록/확보 콜백보다 먼저 읽은 값) → 120입자 zero → 첫 공유 색 표 → 파생 필드(수명/palette/offset/깊이) 순서이며, 원본이 쓰지 않는 delta/scaledDelta 는 보존한다. 실패한 등록의 정리는 호출자 책임이다.
//  - destroy(fields, flags, release): 입자 반납 → 공통 등록 제거/base → flags bit 0 이면 객체 반납 순서이며 공유 색 표는 보존한다. release 는 예외를 던지거나 등록/수명을 바꾸지 않아야 하며 같은 객체에 두 번 호출하지 않는다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E187: 불꽃 꼬리 수명 이식으로 추가)
import { createFieryTailParticle } from './raw-fiery-tail-frame.js';

// 같은 프레임/Draw 를 공유하는 11 불꽃과 12 건설 마나의 생성자 구별이다. (2026-11-09 WB04-E187)
export const FieryTailVariant = Object.freeze({ Fiery: 'Fiery', Streak: 'Streak' });
// scalar deleting destructor 에서 구별하는 두 실제 반납 대상이다. (2026-11-09 WB04-E187)
export const FieryTailRelease = Object.freeze({ Particles: 'Particles', Object: 'Object' });

// 보호 10.78 PE 의 process 11/12·120입자·24바이트 stride·공유 객체 8바이트이다. (2026-11-09 WB04-E187)
const FIERY_PROCESS_TYPE = 11;
const STREAK_PROCESS_TYPE = 12;
const PARTICLE_COUNT = 120;
const PARTICLE_BYTES = 24;
const PALETTE_BYTES = 8;
// 파생 생성자가 마지막에 쓰는 깊이·최대 수명·offset 이다. (2026-11-09 WB04-E187)
const FIERY_DEPTH = -25;
const STREAK_DEPTH = -20;
const FIERY_LIFE = 4;
const STREAK_LIFE = 2;
const STREAK_OFFSET = -8;

// 불꽃 꼬리/건설 마나 생성·소멸 몸체이다. 다른 월드/판본과 메모리 경계 누락은 등록 전에 거부한다. (2026-11-09 WB04-E187)
export class RawFieryTailLifetime {
    #registry;
    #state;
    #hooks;
    #variant;

    // 같은 PATCH 월드와 확보 경계를 확인한다. (2026-11-09 WB04-E187)
    constructor(pool, registry, state, hooks, variant = FieryTailVariant.Fiery) {
        if (pool?.edition !== 'patch1078' || registry?.pool !== pool || typeof hooks?.reserve !== 'function' || !state
            || (variant !== FieryTailVariant.Fiery && variant !== FieryTailVariant.Streak)) throw new TypeError('Fiery Tail 실제 생성 월드/확보 경계 오류');
        this.#registry = registry;
        this.#state = state;
        this.#hooks = hooks;
        this.#variant = variant;
    }

    // 현재 공유 색 표 토큰을 읽는다(공유 상자가 있으면 그 실제 owner 를 읽고 복사본을 남기지 않음). (2026-11-09 WB04-E187)
    #currentPalette() {
        const state = this.#state;
        return state.sharedPalette ? state.sharedPalette.value : state.palette;
    }

    // 현재 공유 색 표 토큰을 쓴다(공유 상자가 있으면 그 owner 에 직접 쓴다). (2026-11-09 WB04-E187)
    #storePalette(token) {
        const state = this.#state;
        if (state.sharedPalette) state.sharedPalette.value = token;
        else state.palette = token;
    }

    // 공통 등록이 끝나야 System 필드를 초기화한다. 생성 중 프레임 재진입은 상위 호스트가 차단한다. (2026-11-09 WB04-E187)
    initialize(fields, token, parent) {
        const hooks = this.#hooks;
        const state = this.#state;
        const streak = this.#variant === FieryTailVariant.Streak;
        // 등록/확보 콜백 전에 원문 인자의 현재 DWORD 를 한 번 읽는다.
        const type = state.sharedProcessType ? state.sharedProcessType() >>> 0 : streak ? STREAK_PROCESS_TYPE : FIERY_PROCESS_TYPE;
        this.#registry.construct(fields.registration, token, parent, type);
        fields.particles = [];
        fields.total = 0;
        fields.lastTime = 0;
        fields.elapsed = 0;
        fields.first = false;
        fields.previousX = 0;
        fields.previousY = 0;
        if (!hooks.reserve(PARTICLE_COUNT * PARTICLE_BYTES)) throw new Error('Fiery Tail 입자 메모리 확보 실패');
        fields.particles = Array.from({ length: PARTICLE_COUNT }, createFieryTailParticle);
        if (!this.#currentPalette()) {
            if (typeof hooks.createPalette !== 'function' || typeof hooks.buildPalette !== 'function') throw new Error('Fiery Tail 첫 공유 색 표 경계 누락');
            if (!hooks.reserve(PALETTE_BYTES)) throw new Error('Fiery Tail 색 표 메모리 확보 실패');
            const created = hooks.createPalette();
            if (!created) throw new Error('Fiery Tail 색 표 생성 실패');
            this.#storePalette(created);
            hooks.buildPalette(created);
        }
        fields.maximumLife = streak ? STREAK_LIFE : FIERY_LIFE;
        fields.palette = this.#currentPalette();
        fields.offset = streak ? STREAK_OFFSET : 0;
        fields.registration.zOrder = streak ? STREAK_DEPTH : FIERY_DEPTH;
    }

    // 실제 free 는 개수/포인터 필드를 다시 쓰지 않는다. 호스트 컨테이너의 저장소만 반납한다. (2026-11-09 WB04-E187)
    destroy(fields, flags, release) {
        if (typeof release !== 'function') throw new TypeError('Fiery Tail 반납 경계 누락');
        if (fields.particles.length) {
            release(FieryTailRelease.Particles);
            fields.particles = [];
        }
        this.#registry.destroy(fields.registration);
        if (flags & 1) release(FieryTailRelease.Object);
    }
}
