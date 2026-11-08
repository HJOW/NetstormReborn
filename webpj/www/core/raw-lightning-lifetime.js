// Lightning.cpp 의 빔 생성/경로 소유와 공통 표시 반납 순서를 옮긴 모듈이다(cpppj `o/RawLightningLifetime.cpp/.h`, 10.78 전용: 00472700/00472690 wrapper·00472770 화면 wrapper·생성자 00471b20/00471e70·소멸).
//  - createLightning(pool, parent, target, sound, create): 표적이 dead/void(상태 비트 3)이면 `{ status: 'TargetUnavailable', skippedTarget, process: null }`(원본이 반환한 raw 주소에 해당하는 SID 를 0 실패와 합치지 않음), 아니면 생존 검사 뒤에만 `create(parent, target, sound)` 를 호출해 `Created`/`AllocationFailed` 를 돌려준다.
//  - createLightningAtScreen(parent, from, to, life, cameraX, cameraY, create): 00472770 — 현재 카메라를 signed DWORD 감김으로 더한 정수 양끝으로 `create(parent, from, to, life, 'shortZap.wav')` 를 요청한다(확보 뒤에만 float 변환은 create 가 담당).
//  - new RawLightningLifetime(pool, registry, path, state, hooks): registry 는 같은 월드의 `RawRenderRegistry`, path 는 `RawLightningPath`(생성 시 현재 부모/표적 snapshot), state = { palette }(0055a758 월드 공유 색 객체, 첫 생성 때 채우며 개별 빔 소멸로 반납하지 않음),
//    hooks = { reserve(바이트)→성공 여부(입자 2880·공유 색 객체 8·경로 64바이트 요청), createPalette()→비영 토큰, buildPalette(token)(실제 8+3색 구성) }.
//  - initialize(fields, token, parent, target, sound): 공통 등록/부착 → 120입자 zero → 공유 색 → 깊이 −25 → 경로 64바이트 확보 뒤 부착이 바꿀 수 있는 현재 부모의 경로(`path.initialize`) → 흔들림(거리 × float 0.025) → 파일명 순서이며 delta/scaledDelta 와 경로 생성자가 쓰지 않는 바이트를 보존한다.
//    initializePoints(fields, token, parent, from, to, life, sound)(00471e70): 직접 좌표 경로이며 부모 head/type/가짜 표적을 추가로 읽지 않고 life 가 0 일 때만 기본 2 를 보존한다.
//  - destroy(fields, flags, release): 경로 → 입자 → 공통 등록 제거/base → flags bit 0 이면 객체 순서로 `release(LightningRelease.*)` 하며 필드의 포인터/개수 자체는 지우지 않고 입자 저장소만 비운다. 공유 팔레트는 반납하지 않는다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E186: 번개 수명 이식으로 추가)

// 소멸자가 구별해 반납하는 세 대상이다(공유 팔레트는 포함하지 않음). (2026-11-09 WB04-E186)
export const LightningRelease = Object.freeze({ Path: 'Path', Particles: 'Particles', Object: 'Object' });
// 원본 wrapper 의 raw 주소 반환/NULL/생성 성공을 호스트 포인터 혼용 없이 구별한다. (2026-11-09 WB04-E186)
export const LightningCreateStatus = Object.freeze({ TargetUnavailable: 'TargetUnavailable', AllocationFailed: 'AllocationFailed', Created: 'Created' });

// 보호 10.78 의 실제 process 19·120입자·stride 24·공유 색 객체/경로 확보량이다. (2026-11-09 WB04-E186)
const PROCESS_TYPE = 19;
const PARTICLE_COUNT = 120;
const PARTICLE_BYTES = 24;
const PALETTE_BYTES = 8;
const PATH_BYTES = 64;
// 원본 생성자가 쓰는 깊이와 흔들림 배율(0050c264 의 float 0.025)이다. (2026-11-09 WB04-E186)
const DEPTH = -25;
const JITTER_SCALE = (() => {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, 0x3ccccccd, true);
    return view.getFloat32(0, true);
})();

// 00472700/00472690 wrapper 이다. (2026-11-09 WB04-E186)
export function createLightning(pool, parent, target, sound, create) {
    if (pool?.edition !== 'patch1078' || target >= pool.capacity) throw new TypeError('빔 wrapper 판본/표적 오류');
    if (pool.slot(target)[11] & 3) return { status: LightningCreateStatus.TargetUnavailable, skippedTarget: target, process: null };
    if (typeof create !== 'function') throw new TypeError('빔 wrapper 실제 생성 경계 누락');
    const process = create(parent, target, sound ?? null);
    return { status: process ? LightningCreateStatus.Created : LightningCreateStatus.AllocationFailed, skippedTarget: 0, process: process ?? null };
}

// 카메라는 호출 시점 값이며 확보 효과가 바꿔도 이미 계산한 정수 양끝을 다시 계산하지 않는다(원본 add 의 32비트 감김). (2026-11-09 WB04-E186)
export function createLightningAtScreen(parent, from, to, life, cameraX, cameraY, create) {
    if (typeof create !== 'function') throw new TypeError('화면빔 실제확보/생성 경계누락');
    const add = (value, camera) => (value + camera) | 0;
    return create(parent, [add(from[0], cameraX), add(from[1], cameraY)], [add(to[0], cameraX), add(to[1], cameraY)], life, 'shortZap.wav');
}

// 실제 경로 거리(+0x18)의 비정렬 little endian float 만 읽는다. (2026-11-09 WB04-E186)
function pathDistance(fields) {
    const bytes = fields.bytes;
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, (bytes[24] | (bytes[25] << 8) | (bytes[26] << 16) | (bytes[27] << 24)) >>> 0, true);
    return view.getFloat32(0, true);
}

// 빔 생성/소멸 몸체이다. 현재 판본/같은 풀/필수 확보 경계를 등록 전에 확인한다. (2026-11-09 WB04-E186)
export class RawLightningLifetime {
    #pool;
    #registry;
    #path;
    #state;
    #hooks;

    // 생성 때 같은 PATCH 월드와 확보 경계를 확인한다. (2026-11-09 WB04-E186)
    constructor(pool, registry, path, state, hooks) {
        if (pool?.edition !== 'patch1078' || registry?.pool !== pool || typeof hooks?.reserve !== 'function' || !path || !state) throw new TypeError('빔 생성 월드/확보 경계 오류');
        this.#pool = pool;
        this.#registry = registry;
        this.#path = path;
        this.#state = state;
        this.#hooks = hooks;
    }

    // target 형과 직접 좌표형이 같은 원본 생성 접두(등록/부착/입자/팔레트/경로 확보 순서)를 공유한다. (2026-11-09 WB04-E186)
    #initializeBase(fields, token, parent) {
        const hooks = this.#hooks;
        const state = this.#state;
        this.#registry.construct(fields.registration, token, parent, PROCESS_TYPE);
        fields.particles.values = [];
        fields.particles.total = 0;
        fields.particles.owner = token;
        fields.lastTime = 0;
        fields.elapsed = 0;
        fields.first = false;
        if (!hooks.reserve(PARTICLE_COUNT * PARTICLE_BYTES)) throw new Error('빔 입자 메모리 확보 실패');
        fields.particles.values = Array.from({ length: PARTICLE_COUNT }, () => ({ x: 0, y: 0, strength: 0, jitterWeight: 0, life: 0, previous: 0 }));
        fields.firstLive = 0;
        if (!state.palette) {
            if (typeof hooks.createPalette !== 'function' || typeof hooks.buildPalette !== 'function') throw new Error('빔 첫 공유 색 경계 누락');
            if (!hooks.reserve(PALETTE_BYTES)) throw new Error('빔 색 객체 메모리 확보 실패');
            state.palette = hooks.createPalette();
            if (!state.palette) throw new Error('빔 색 객체 생성 실패');
            hooks.buildPalette(state.palette);
        }
        fields.palette = state.palette;
        fields.registration.zOrder = DEPTH;
        if (!hooks.reserve(PATH_BYTES)) throw new Error('빔 경로 메모리 확보 실패');
        fields.pathAllocated = true;
    }

    // 공통 등록/부착이 끝나야 파생 생성 필드를 설정한다(초기화 중 Run/Draw 는 상위 ready 가 막음). (2026-11-09 WB04-E186)
    initialize(fields, token, parent, target, sound) {
        this.#initializeBase(fields, token, parent);
        if (fields.registration.parent < 0 || fields.registration.parent > 65535) throw new RangeError('빔 생성 현재 부모 SID 오류');
        const current = fields.registration.parent;
        const raw = this.#pool.slot(current);
        if (!raw[6] && !raw[7]) throw new Error('빔 생성 현재 부모 head 원본 assert');
        this.#path.initialize(fields.path, current, target, token);
        fields.jitter = Math.fround(pathDistance(fields.path) * JITTER_SCALE);
        fields.sound = sound ?? null;
    }

    // 00471e70: 직접 좌표 경로에는 부모 head/type/가짜 target 을 추가로 읽는 절차가 없다. (2026-11-09 WB04-E186)
    initializePoints(fields, token, parent, from, to, life, sound) {
        this.#initializeBase(fields, token, parent);
        this.#path.initializePoints(fields.path, from, to, token);
        fields.jitter = Math.fround(pathDistance(fields.path) * JITTER_SCALE);
        if (life !== 0) {
            const view = new DataView(new ArrayBuffer(4));
            view.setFloat32(0, life, true);
            // NaN 도 원본 비교처럼 수명 필드에 그대로 쓰고 다른 경로 필드는 보존한다.
            for (let index = 0; index < 4; index += 1) fields.path.bytes[32 + index] = view.getUint8(index);
        }
        fields.sound = sound ?? null;
    }

    // native free 는 필드의 포인터/개수 자체를 지우지 않는다. 호스트 입자 저장소만 반납한다. (2026-11-09 WB04-E186)
    destroy(fields, flags, release) {
        if (typeof release !== 'function') throw new TypeError('빔 반납 경계 누락');
        if (fields.pathAllocated) release(LightningRelease.Path);
        if (fields.particles.values.length) {
            release(LightningRelease.Particles);
            fields.particles.values = [];
        }
        this.#registry.destroy(fields.registration);
        if (flags & 1) release(LightningRelease.Object);
    }
}
