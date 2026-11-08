// Burning 모드 0/1 생성자의 표시 등록·150입자 초기화와 실제 입자 추가를 옮긴 모듈이다(cpppj `o/RawBurningVisual.cpp/.h`, 10.78 전용: 생성자 상위 몸체 + 00423e80 공통 Particle.New 입자 추가).
//  - createBurningVisualFields(): 원본 24바이트 입자와 의미 필드를 모은 객체이다 — registration(공통 표시 등록 필드, `createRenderProcessFields()`), capacity/total(+0x24/+0x28 실제 입자 용량 150·현재 입자 수), particles(`{ x, y, velocityX, velocityY, life, source }` 배열),
//    lastTime/elapsed(+0x30/+0x38 시각 helper 초기 0), palette/sound(+0x48/+0x50), stage(+0x54), image(`{ width, height, hotspotX, hotspotY, pixels }` 후속 현재 프레임 캡처, 생성 때 비어 있음), victim(+0x6c 생성 진입 피해 SID), mode(+0x70 DudeNotice 0 또는 일반 저HP 효과 1의 DWORD 인수),
//    rows(+0x78 후속 고정 연소점 선택표, 생성 때 비어 있음), selected(+0x7c 현재 선택된 연소점 수), off(+0x80). 미초기화 delta 40/44 와 DWORD 74 의 값을 만들지 않는다.
//  - new RawBurningVisual(pool, registry, state, hooks): pool 은 같은 PATCH SidPool, registry 는 같은 풀의 `RawRenderRegistry`(생성자가 재사용하는 표시 목록 몸체), state = { processType: 14(burningSystemProcessType), palette: 0(005453d0 현재 공유 색 표) } 는 월드가 소유하며,
//    hooks = { reserve(바이트)→성공 여부(생성자 150*24 또는 공유 색 표 8바이트 확보), createPalette()→토큰(실제 8바이트 색 표 생성자), gradient(첫 RGB, 끝 RGB, 색수 8, 보정 2.4)(원본 주황색 띠), buildPalette(token)(실제 색띠를 같은 공유표에 넣음) } 로 색 표 생성은 도달할 때만 요구한다.
//  - initialize(fields, token, victim, mode): 이미 확보한 객체의 ctor — 공통 표시 등록 → 의미 필드 초기화 → 150입자 zero(24바이트 확보) → 공유 색 표가 아직 없으면 첫 색 표를 만든다(미복원이면 이름 없는 빈 색 표로 완료 처리하지 않고 거부하며 앞선 효과는 보존).
//  - append(fields, particle): 00423e80 — 여유 칸이 있으면 개수를 늘려 거기에, 가득 찼으면 처음 만료된(ordered float life ≤ 0) 칸에 6필드를 쓰고 성공 여부를 돌려준다(없으면 false).
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E183: Burning 표시 초기화·입자 추가 이식으로 추가)
import { createRenderProcessFields } from './raw-construction-lifetime.js';

// 생성자 고정 150입자와 실제 32비트 메모리의 확보량(입자 24바이트·색 표 8바이트)이다. (2026-11-09 WB04-E183)
const PARTICLES = 150;
const PARTICLE_BYTES = 24;
const PALETTE_BYTES = 8;
// 보호 PE 가 공급하는 주황색 두 끝점(시작 주황 → 끝 밝은 색)과 공통 색띠 보정, 색 수이다. (2026-11-09 WB04-E183)
const FIRST_COLOR = Object.freeze([1.0, 115.0 / 255.0, 7.0 / 255.0]);
const LAST_COLOR = Object.freeze([1.0, 245.0 / 255.0, 117.0 / 255.0]);
const GRADIENT_NOISE = 2.4;
const GRADIENT_COLORS = 8;

// 새 영(0) 입자 하나를 만든다. (2026-11-09 WB04-E183)
export function createBurningParticle() {
    return { x: 0, y: 0, velocityX: 0, velocityY: 0, life: 0, source: false };
}

// 후속 현재 프레임 캡처의 빈 이미지이다(생성 때 비어 있음). (2026-11-09 WB04-E183)
function emptyImage() {
    return { width: 0, height: 0, hotspotX: 0, hotspotY: 0, pixels: [] };
}

// 생성 전 상태의 Burning 의미 필드를 만든다. (2026-11-09 WB04-E183)
export function createBurningVisualFields() {
    return {
        registration: createRenderProcessFields(), capacity: 0, total: 0, particles: [], lastTime: 0, elapsed: 0, palette: 0, sound: 0, stage: 0,
        image: emptyImage(), victim: 0, mode: 0, rows: [], selected: 0, off: false
    };
}

// Burning 표시 생성/입자 추가 몸체이다. registry/pool 은 같은 월드여야 하며 현재 state/hooks 는 이 처리기보다 오래 살아야 한다. (2026-11-09 WB04-E183)
export class RawBurningVisual {
    #pool;
    #registry;
    #state;
    #hooks;

    // 공통 표시 등록은 현재 같은 풀을 사용하며 정상 메모리 경계가 반드시 필요하다. (2026-11-09 WB04-E183)
    constructor(pool, registry, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Burning 표시는 10.78 SidPool 만 받습니다.');
        if (registry?.pool !== pool || typeof hooks?.reserve !== 'function') throw new TypeError('Burning 표시 등록/메모리 실제월드 오류');
        this.#pool = pool;
        this.#registry = registry;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 같은 피해/주변 통지/Host 소비자에서 실제 풀을 확인한다. (2026-11-09 WB04-E183)
    get pool() { return this.#pool; }

    // 원본 ctor 의 모드 인수는 0/1 이외도 DWORD 로 보존한다(현재 두 wrapper 관찰은 0/1). (2026-11-09 WB04-E183)
    initialize(fields, token, victim, mode) {
        const state = this.#state;
        const hooks = this.#hooks;
        this.#registry.construct(fields.registration, token, victim, state.processType);
        fields.capacity = 0;
        fields.total = 0;
        fields.particles = [];
        fields.lastTime = 0;
        fields.elapsed = 0;
        fields.palette = 0;
        fields.victim = victim;
        fields.stage = 0;
        fields.image = emptyImage();
        fields.rows = [];
        fields.selected = 0;
        fields.off = false;
        fields.sound = 0;
        if (!hooks.reserve(PARTICLES * PARTICLE_BYTES)) throw new Error('Burning 입자 메모리 확보 실패');
        fields.capacity = PARTICLES;
        fields.particles = Array.from({ length: PARTICLES }, createBurningParticle);
        fields.total = 0;
        fields.mode = mode >>> 0;
        if (!state.palette) {
            if (typeof hooks.createPalette !== 'function' || typeof hooks.gradient !== 'function' || typeof hooks.buildPalette !== 'function') throw new Error('Burning 실제첫팔레트 생성 미복원');
            if (!hooks.reserve(PALETTE_BYTES)) throw new Error('Burning 색 표 메모리 확보 실패');
            state.palette = hooks.createPalette();
            if (!state.palette) throw new Error('Burning 색 표 생성 실패');
            hooks.gradient(FIRST_COLOR, LAST_COLOR, GRADIENT_COLORS, GRADIENT_NOISE);
            hooks.buildPalette(state.palette);
        }
    }

    // 00423e80: 원본 공통 Particle.New 로 아직 쓰지 않은 앞 칸을 늘리고, 가득 찼을 때 처음 만료된 칸을 재사용한다. (2026-11-09 WB04-E183)
    append(fields, particle) {
        if (fields.particles.length !== fields.capacity || fields.total > fields.capacity) throw new RangeError('Burning 현재입자 저장소/개수 오류');
        let index = fields.total;
        if (index < fields.capacity) fields.total += 1;
        else {
            index = 0;
            // 패치는 ordered float <= 0 에서만 재사용한다(NaN 수명은 재사용하지 않음).
            while (index < fields.total && !(fields.particles[index].life <= 0)) index += 1;
            if (index === fields.total) return false;
        }
        fields.particles[index] = { x: particle.x, y: particle.y, velocityX: particle.velocityX, velocityY: particle.velocityY, life: particle.life, source: Boolean(particle.source) };
        return true;
    }
}
