// Evilray.cpp 의 EvilRay(process 10) 생성·중단 통지와 공통 Particle 소멸 순서를 옮긴 모듈이다(cpppj `o/RawEvilRayLifetime.cpp/.h`, 10.78 전용: 004530b0 생성자·00453240 Stop·공통 소멸).
//  - createEvilRayFields(): 공통 SystemProcess 와 EvilRay 파생 필드의 의미 저장소 — registration(+4..+20 표시 등록/부모/Form), particles(+24/+2c 원문 50개 24바이트 입자, 불꽃 꼬리 입자와 같은 `{ x, y, velocityX, velocityY, life, spare }`),
//    total(+28 사용 입자 수), lastTime/elapsed(+30/+38), delta/scaledDelta(+40/+44, 생성자가 쓰지 않음), first(+48 최초 시계 준비), nextEmission(+50 다음 방출 기한 float), source/stopping(+54/+58 전역에서 읽은 source DWORD 와 중단 DWORD)이다.
//  - buildEvilRayPalette(palette): PE 의 실제 RGB double 상수(0x00508e60/68/58 → 0x00508e78/70/68)로 만든 단일 3색 띠이다(끝색 성분이 1 을 넘어도 원문대로 유지). palette 는 256 DWORD RGBX 이다.
//  - EvilRayRelease: 공통 소멸자가 구별하는 입자 배열·객체 자체이다(공유 색 객체는 개별 소멸 때 반납하지 않음).
//  - new RawEvilRayLifetime(pool, registry, state, hooks): registry 는 같은 풀의 `RawRenderRegistry`, state = { palette, processType(10), source(0), sharedProcessType?, sharedSource? } 는
//    공유 색 객체 토큰(0055a6f0)과 독립 기본 TYPE/source(00541098/00595128)이다. sharedProcessType/sharedSource 는 현재 장기 전역을 돌려주는 함수이며, 있으면 원문 인자 위치(TYPE 는 등록 전, source 는 등록/부착 뒤)에서 그 값을 읽는다.
//    hooks = { reserve(바이트)→성공 여부(입자 1200·첫 공유 객체 8), createPalette()→비영 토큰, buildPalette(token), assertion(식, 파일, 줄) } 은 **도달한 위치에서만** 요구한다(미연결이면 그때 오류).
//  - initialize(fields, token, parent): TYPE 인자 → 공통 등록/부착 → 필드 초기화와 현재 source → 50입자 확보/zero → (공유 색이 없을 때만) 8바이트 확보 → 생성 → 단일 3색 계산 순서이다.
//  - stop(fields, mode): mode 3 만 중단(stopping=1)하고, 다른 mode 나 이미 중단된 경우는 진단(정상 복귀 가능) 뒤 다른 mode 면 그대로 돌아간다.
//  - destroy(fields, flags, release): 입자(있을 때) → 공통 등록 제거/base → flags bit 0 이면 객체 순서이다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E190: EvilRay 수명 이식으로 추가)
import { createRenderProcessFields } from './raw-construction-lifetime.js';
import { createFieryTailParticle } from './raw-fiery-tail-frame.js';
import { buildParticleGradient, mergeParticleGradients } from './particle-gradient.js';

// 실제 입자 수·24바이트 stride·공유 색 객체 크기이다(00453103). (2026-11-09 WB04-E190)
const PARTICLE_COUNT = 50;
const PARTICLE_BYTES = 24;
const PALETTE_BYTES = 8;
// 진단 정상 복귀 경계가 받는 원문 파일 이름이다. (2026-11-09 WB04-E190)
const SOURCE_FILE = '.\\Evilray.cpp';

// binary64 비트를 double 로 바꾼다(PE 상수를 계산 없이 옮길 때 쓴다). (2026-11-09 WB04-E190)
function bitsDouble(bits) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, bits, true);
    return view.getFloat64(0, true);
}

// 00508e60/68/58(시작)과 00508e78/70/68(끝)의 binary64 를 원문 RGB 인자 순서로 쓴다. (2026-11-09 WB04-E190)
const EVIL_FROM = Object.freeze([bitsDouble(0x3fe0303030303030n), bitsDouble(0x3fe3535353535353n), bitsDouble(0x3fd4545454545454n)]);
const EVIL_TO = Object.freeze([bitsDouble(0x3febfbfbfbfbfbfcn), bitsDouble(0x3ff1414141414141n), bitsDouble(0x3fe3535353535353n)]);

// 공통 원문 정렬/선택/병합 몸체로 단일 3색 띠를 만든다. 사용: buildEvilRayPalette(palette256) → 3개 색 번호. (2026-11-09 WB04-E190)
export function buildEvilRayPalette(palette) {
    return mergeParticleGradients([buildParticleGradient(palette, EVIL_FROM, EVIL_TO, 3)]);
}

// 공통 소멸자가 구별하는 두 실제 반납 대상이다. (2026-11-09 WB04-E190)
export const EvilRayRelease = Object.freeze({ Particles: 'Particles', Object: 'Object' });

// 새 EvilRay 효과의 의미 필드를 만든다(입자 배열은 비어 있다). (2026-11-09 WB04-E190)
export function createEvilRayFields() {
    return {
        registration: createRenderProcessFields(), particles: [], total: 0, lastTime: 0, elapsed: 0, delta: 0, scaledDelta: 0, first: false,
        nextEmission: 0, source: 0, stopping: 0
    };
}

// 도달한 위치에서 현재 선택 장치를 고른다. 미연결이면 빈 성공으로 바꾸지 않고 오류이다. (2026-11-09 WB04-E190)
function need(fn, message) {
    if (typeof fn !== 'function') throw new Error(message);
    return fn;
}

// EvilRay 생성/중단/소멸 몸체이다. 실제 Kernel 부착/프레임/출력 연결과 구별한다. (2026-11-09 WB04-E190)
export class RawEvilRayLifetime {
    #registry;
    #state;
    #hooks;

    // 같은 PATCH 풀/등록 owner 를 요구하고 다른 판본/월드는 생성 전에 거부한다(장치는 도달 시 확인). (2026-11-09 WB04-E190)
    constructor(pool, registry, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || registry?.pool !== pool || !state || !hooks) throw new TypeError('EvilRay 실제 생성 월드/판본 오류');
        this.#registry = registry;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 현재 TYPE 전역(공유 함수가 있으면 그 값, 없으면 독립 기본값)이다. (2026-11-09 WB04-E190)
    #processType() {
        const state = this.#state;
        return (typeof state.sharedProcessType === 'function' ? state.sharedProcessType() : state.processType) >>> 0;
    }

    // 현재 source 전역의 전체 DWORD 이다(SID 축소/범위 검사 없음). (2026-11-09 WB04-E190)
    #source() {
        const state = this.#state;
        return (typeof state.sharedSource === 'function' ? state.sharedSource() : state.source) >>> 0;
    }

    // TYPE 인자 → 공통 등록 → source → 50입자 → 첫 공유 3색 순서이다. 등록 콜백이 바꾼 source 는 뒤에서 현재 값으로 읽는다. 원본: 004530b0. (2026-11-09 WB04-E190)
    initialize(fields, token, parent) {
        const state = this.#state;
        const type = this.#processType();
        this.#registry.construct(fields.registration, token, parent, type);
        fields.particles = [];
        fields.total = 0;
        fields.lastTime = 0;
        fields.elapsed = 0;
        fields.first = false;
        fields.nextEmission = 0;
        fields.source = this.#source();
        fields.stopping = 0;
        // 확보가 자기 연결을 바꾸어도 이번 선택 장치는 호출 종료까지 유지한다.
        const reserve = need(this.#hooks.reserve, 'EvilRay 입자 확보 장치 미연결');
        if (!reserve(PARTICLE_COUNT * PARTICLE_BYTES)) throw new Error('EvilRay 입자 메모리 확보 실패');
        fields.particles = Array.from({ length: PARTICLE_COUNT }, createFieryTailParticle);
        if (!state.palette) {
            // 첫 공유 객체의 원문 확보 위치에서 현재 장치를 다시 고른다.
            const allocate = need(this.#hooks.reserve, 'EvilRay 공유 객체 확보 장치 미연결');
            if (!allocate(PALETTE_BYTES)) throw new Error('EvilRay 공유 객체 메모리 확보 실패');
            const create = need(this.#hooks.createPalette, 'EvilRay 공유 객체 생성 장치 미연결');
            state.palette = create() >>> 0;
            if (!state.palette) throw new Error('EvilRay 공유 객체 생성 실패');
            const build = need(this.#hooks.buildPalette, 'EvilRay 공유 색 계산 장치 미연결');
            build(state.palette);
        }
    }

    // mode 3 만 중단한다. 다른 mode 는 진단 뒤 현재 stopping 을 쓰지 않고 복귀하며, 이미 중단된 mode 3 은 진단 뒤 다시 1 을 쓴다. 원본: 00453240. (2026-11-09 WB04-E190)
    stop(fields, mode) {
        if (mode !== 3 || fields.stopping) {
            // 진단의 재진입/연결 반납에도 이번 callable 은 살아 있다.
            const assertion = need(this.#hooks.assertion, 'EvilRay 중단 진단 장치 미연결');
            assertion(mode !== 3 ? 'false' : 'stopping == false', SOURCE_FILE, mode !== 3 ? 0x4e : 0x48);
            if (mode !== 3) return;
        }
        fields.stopping = 1;
    }

    // 개별 소멸은 같은 공유 색 객체를 지우지 않는다. 입자 → 등록/base → bit 0 객체 순서이다. (2026-11-09 WB04-E190)
    destroy(fields, flags, release) {
        if (typeof release !== 'function') throw new TypeError('EvilRay 반납 장치 미연결');
        if (fields.particles.length) {
            release(EvilRayRelease.Particles);
            fields.particles = [];
        }
        this.#registry.destroy(fields.registration);
        if (flags & 1) release(EvilRayRelease.Object);
    }
}
