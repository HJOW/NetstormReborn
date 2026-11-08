// Lightning.cpp 의 공유 빔 경로(00471b20/00471ca0/004720f0/00472180)와 재귀 입자 생성(00471fc0)을 원본 저장 폭으로 옮긴 모듈이다(cpppj `o/RawLightningPath.cpp/.h`, 10.78 전용).
// 원본 64바이트 경로 이미지(LightningPathFields)와 24바이트 입자 고정 저장소(LightningParticles)를 보존하며, 모든 float 는 원본 fstp 경계마다 단정도로 저장한다.
//  - initialize(fields, source, target, owner): 양끝 중심에 픽셀 배율을 곱해 snapshot 하고(source 가 반더면 y−76, 키 큰 유닛이면 y−48, 맨 오브 워면 y−48·x−5, target 은 키 큰/맨 오브 워면 y−48 아니면 y−16),
//    차 벡터·거리·각(atan2(−dy, dx))·굵기 2·미사용 +38..+3c 와 +34 등을 원문대로 쓴다. 경로는 이후 따라갈 SID 를 저장하지 않는다. initializePoints(fields, from, to, owner)는 이미 정수화된 픽셀 양끝을 직접 저장한다.
//  - seed(fields, particles): 흔들리지 않는 시작점 입자를 추가하고 step 0.15·진행 0·기본 각도를 쓴다. step(fields, particles, progress, lifeOffset): 현재 진행(≤1)까지 재귀 가지를 만들며(주점은 이전 주점에, 가지는 방금 만든 점에 연결)
//    step 난수를 두 번 뽑고, 진행이 1 이 되면 끝점을 한 번 추가하고 완료 표시를 쓴다. 입자는 죽은 것도 재사용하지 않으며 용량 초과(원본 assert)는 예외로 보고한다.
//  - state { scaleX, scaleY, vanderType, tallType, manOWarType } 는 생성 시마다 읽고, center(sid)→[x,y] 는 같은 월드의 004adea0 중심 조회, random 은 GameRandom(next(limit)) 이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E117: RawLightningPath 이식으로 추가)

// 원본 float 상수의 정확한 비트이다. 1/3 과 0.9 를 넓은 수학 상수로 다시 만들지 않는다. (2026-11-08 WB04-E117)
const bitsView = new DataView(new ArrayBuffer(4));

// 비트(uint32)를 단정도 수로 바꾼다. (2026-11-08 WB04-E117)
function bitsToFloat(bits) {
    bitsView.setUint32(0, bits >>> 0, true);
    return bitsView.getFloat32(0, true);
}

// 단정도 수를 비트(uint32)로 바꾼다. (2026-11-08 WB04-E117)
function floatToBits(value) {
    bitsView.setFloat32(0, value, true);
    return bitsView.getUint32(0, true);
}

// 최소 step 0.08, 가지 분기 임계 0.12, step 감쇠 0.9, 1/3, 기본 step 0.15 (모두 정확한 binary32)이다. (2026-11-08 WB04-E117)
const MINIMUM_STEP = bitsToFloat(0x3da3d70a);
const BRANCH_THRESHOLD = bitsToFloat(0x3df5c28f);
const STEP_DECAY = bitsToFloat(0x3f666666);
const THIRD = bitsToFloat(0x3eaaaaab);
const DEFAULT_STEP = bitsToFloat(0x3e19999a);
// 원본 005092cc 와 같은 상위 16비트 난수 단위, 판본 경로 이미지 크기이다. (2026-11-08 WB04-E117)
const RANDOM_UNIT = 1.0 / 65536.0;
const PATH_SIZE = 64;
const INT32_MAX = 0x7fffffff;
const fround = Math.fround;

// 경로 필드 위치(+0 시작점, +8 끝점, +10 차 벡터, +18 거리, +1c 각, +20 굵기, +24/+28 step 상한/현재, +2c 진행, +30 흔들림 각, +34 마지막 입자 토큰, +38 완료, +3c 소유자)이다. (2026-11-08 WB04-E117)
const OFFSET = Object.freeze({ startX: 0, startY: 4, endX: 8, endY: 12, dx: 16, dy: 20, distance: 24, angle: 28, strength: 32, maximum: 36, step: 40, progress: 44, wobble: 48, last: 52, done: 56, owner: 60 });

// 원본 24바이트 빔 입자 하나이다(이전 입자 포인터는 같은 고정 저장소의 index+1 토큰, 0 은 NULL). (2026-11-08 WB04-E117)
export class LightningParticle {
    // 생성 시 snapshot 인 픽셀 좌표, 가지의 굵기·흔들림 가중치·10배 시간 수명, 이전 입자 토큰이다.
    x = 0;
    y = 0;
    strength = 0;
    jitterWeight = 0;
    life = 0;
    previous = 0;
}

// 실제 용량 전체와 Append 꼬리를 함께 소유한다. 경로 생성은 죽은 입자를 재사용하지 않는다. 사용: new LightningParticles(120, 소유자 토큰). (2026-11-08 WB04-E117)
export class LightningParticles {
    // 생성자가 확보할 실제 용량 전체(정식 빔은 120개를 0 으로 초기화), 사용한 꼬리 수(완료 뒤에도 줄이지 않음), 경로와 같은 소유 빔 토큰이다.
    values;
    total = 0;
    owner;

    // 용량만큼 0 입자를 만든다. (2026-11-08 WB04-E117)
    constructor(capacity = 120, owner = 0) {
        this.values = Array.from({ length: capacity }, () => new LightningParticle());
        this.owner = owner;
    }
}

// 원본 64바이트 경로 이미지이다. 생성자가 쓰지 않는 +24..+34 도 원문처럼 보존한다. 사용: new LightningPathFields(). (2026-11-08 WB04-E117)
export class LightningPathFields {
    // float/DWORD 전체 저장 비트이며 +34/+3c 만 호스트 토큰이다.
    bytes = new Uint8Array(PATH_SIZE);
}

// 비정렬 경로 필드의 리틀엔디언 DWORD 를 읽는다. (2026-11-08 WB04-E117)
function word(fields, offset) {
    const bytes = fields.bytes;
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

// 나머지 미사용 바이트를 건드리지 않고 DWORD 하나만 쓴다. (2026-11-08 WB04-E117)
function setWord(fields, offset, value) {
    const wrapped = value >>> 0;
    // 원본 리틀엔디언 필드 폭을 유지한다.
    for (let index = 0; index < 4; index += 1) fields.bytes[offset + index] = (wrapped >>> (8 * index)) & 0xff;
}

// 실제 fstp float 경계에 대응하는 float 읽기/쓰기이다. (2026-11-08 WB04-E117)
function float(fields, offset) { return bitsToFloat(word(fields, offset)); }
function setFloat(fields, offset, value) { setWord(fields, offset, floatToBits(value)); }

// 같은 raw 풀·게임 난수와 현재 중심 조회를 사용하며 표적 SID 를 경로에 캐시하지 않는다. 사용: new RawLightningPath(pool, random, state, center). (2026-11-08 WB04-E117)
export class RawLightningPath {
    // 생성 시 원래 source/target 타입을 읽는 같은 저장소, 다른 표시/게임 계산과 공유하는 실제 난수 상태, 생성 시마다 읽는 배율/타입, 004adea0 중심 조회이다.
    #pool;
    #random;
    #state;
    #center;

    // 미복원 판본/잘못된 배율·중심 경계는 최초 생성 전에 거부한다. (2026-11-08 WB04-E117)
    constructor(pool, random, state, center) {
        if (pool?.edition !== 'patch1078' || typeof center !== 'function' || !random || !state
            || !Number.isFinite(state.scaleX) || !Number.isFinite(state.scaleY) || state.scaleX <= 0 || state.scaleY <= 0) throw new Error('빔 경로 판본/배율/중심 조회 오류');
        this.#pool = pool;
        this.#random = random;
        this.#state = state;
        this.#center = center;
    }

    // 00471b20: 생성 시 양끝/거리/각도를 snapshot 하고 미사용 바이트는 보존한다. source 보정과 target 보정을 구별한다. 사용: initialize(fields, source, target, owner). (2026-11-08 WB04-E117)
    initialize(fields, source, target, owner) {
        const pool = this.#pool;
        const state = this.#state;
        if (!owner || source >= pool.capacity || target >= pool.capacity) throw new RangeError('빔 경로 소유자/자산 번호 오류');
        fields.bytes.fill(0, 0, 16);
        const scaleX = fround(state.scaleX);
        const scaleY = fround(state.scaleY);
        const from = this.#center(source);
        setFloat(fields, OFFSET.startX, scaleX * from[0]);
        setFloat(fields, OFFSET.startY, scaleY * from[1]);
        const sourceType = pool.slot(source)[10];
        if (sourceType === state.vanderType) setFloat(fields, OFFSET.startY, float(fields, OFFSET.startY) - 76);
        else if (sourceType === state.tallType) setFloat(fields, OFFSET.startY, float(fields, OFFSET.startY) - 48);
        else if (sourceType === state.manOWarType) {
            setFloat(fields, OFFSET.startY, float(fields, OFFSET.startY) - 48);
            setFloat(fields, OFFSET.startX, float(fields, OFFSET.startX) - 5);
        }
        const to = this.#center(target);
        setFloat(fields, OFFSET.endX, scaleX * to[0]);
        setFloat(fields, OFFSET.endY, scaleY * to[1]);
        const targetType = pool.slot(target)[10];
        setFloat(fields, OFFSET.endY, float(fields, OFFSET.endY) - (targetType === state.tallType || targetType === state.manOWarType ? 48 : 16));
        this.#finishVector(fields, owner);
    }

    // 004720f0 이전의 공통 벡터 쓰기: 차·거리·각·굵기 2·완료 0·소유자를 원문 저장 순서로 쓴다. (2026-11-08 WB04-E117)
    #finishVector(fields, owner) {
        const dx = float(fields, OFFSET.startX) - float(fields, OFFSET.endX);
        const dy = float(fields, OFFSET.startY) - float(fields, OFFSET.endY);
        setFloat(fields, OFFSET.dx, dx);
        setFloat(fields, OFFSET.dy, dy);
        setFloat(fields, OFFSET.distance, Math.sqrt(dx * dx + dy * dy));
        setFloat(fields, OFFSET.angle, Math.atan2(-dy, dx));
        setFloat(fields, OFFSET.strength, 2);
        setWord(fields, OFFSET.done, 0);
        setWord(fields, OFFSET.owner, owner);
    }

    // 00471ca0: 이미 정수화된 픽셀 양끝을 직접 저장하고 원래 미정 바이트는 보존한다(타입/중심/SID/배율 보정을 읽지 않는다). 사용: initializePoints(fields, from, to, owner). (2026-11-08 WB04-E117)
    initializePoints(fields, from, to, owner) {
        if (!owner) throw new Error('직접 빔 경로 소유 토큰 0');
        fields.bytes.fill(0, 0, 16);
        setFloat(fields, OFFSET.startX, from[0]);
        setFloat(fields, OFFSET.startY, from[1]);
        setFloat(fields, OFFSET.endX, to[0]);
        setFloat(fields, OFFSET.endY, to[1]);
        // 실제 fstp 전의 넓은 차를 거리/각도에도 쓰므로 float 로 저장된 끝점을 다시 읽어 같은 차를 만든다.
        const dx = fround(from[0]) - fround(to[0]);
        const dy = fround(from[1]) - fround(to[1]);
        setFloat(fields, OFFSET.dx, dx);
        setWord(fields, OFFSET.owner, owner);
        setWord(fields, OFFSET.done, 0);
        setFloat(fields, OFFSET.strength, 2);
        setFloat(fields, OFFSET.dy, dy);
        setFloat(fields, OFFSET.distance, Math.sqrt(dx * dx + dy * dy));
        setFloat(fields, OFFSET.angle, Math.atan2(-dy, dx));
    }

    // owner/현재 꼬리만 확인하고 입자의 기존 수명/미사용 필드를 고쳐 주지 않는다. (2026-11-08 WB04-E117)
    static #validate(fields, particles) {
        if (!particles.owner || word(fields, OFFSET.owner) !== particles.owner || particles.total < 0 || particles.total > particles.values.length || particles.values.length > INT32_MAX) throw new Error('빔 경로/입자 소유자 또는 꼬리 오류');
    }

    // 00487b20: 빈 꼬리만 증가시킨다. 원본 용량 초과 assert 는 예외로 보고한다. 반환은 새 입자의 비영 토큰(index+1)이다. (2026-11-08 WB04-E117)
    static #append(particles) {
        if (particles.total >= particles.values.length) throw new RangeError('빔 입자 용량 원본 assert');
        particles.total += 1;
        return particles.total;
    }

    // 현재 입자 토큰을 같은 저장소의 원래 항목으로 해석한다(현재 꼬리 밖이면 가짜 연결을 만들지 않는다). (2026-11-08 WB04-E117)
    static #at(particles, token) {
        if (!token || token > particles.total) throw new RangeError('빔 이전 입자 토큰 오류');
        return particles.values[token - 1];
    }

    // 입자 하나를 float 저장 폭으로 쓴다. (2026-11-08 WB04-E117)
    static #set(particles, token, x, y, strength, jitterWeight, life, previous) {
        const particle = RawLightningPath.#at(particles, token);
        particle.x = fround(x);
        particle.y = fround(y);
        particle.strength = fround(strength);
        particle.jitterWeight = fround(jitterWeight);
        particle.life = fround(life);
        particle.previous = previous >>> 0;
    }

    // 004720f0: 첫 점의 흔들림 가중치는 0 이며 기본 진행 step 은 정확한 float 0.15 이다. 사용: seed(fields, particles). (2026-11-08 WB04-E117)
    seed(fields, particles) {
        RawLightningPath.#validate(fields, particles);
        const token = RawLightningPath.#append(particles);
        RawLightningPath.#set(particles, token, float(fields, OFFSET.startX), float(fields, OFFSET.startY), 1, 0, float(fields, OFFSET.strength), 0);
        setFloat(fields, OFFSET.maximum, DEFAULT_STEP);
        setFloat(fields, OFFSET.step, DEFAULT_STEP);
        setWord(fields, OFFSET.last, token);
        setFloat(fields, OFFSET.progress, 0);
        setWord(fields, OFFSET.wobble, word(fields, OFFSET.angle));
    }

    // 실제 00455930 의 상위 16비트/65536·절댓값 간격·넓은 반환을 유지한다(low/high 는 float 값, 반환은 double). (2026-11-08 WB04-E117)
    #randomBetween(low, high) {
        return this.#random.next(65536) * RANDOM_UNIT * Math.abs(high - low) + low;
    }

    // 00471fc0: 새 주점은 이전 주점에, 재귀 가지는 방금 만든 점에 연결한다. 재귀 결과를 주점으로 교체하지 않는다. cos/sin 점을 추가한 뒤 조건에 따라 길이/굵기를 반으로 줄여 재귀한다. (2026-11-08 WB04-E117)
    #branch(fields, particles, angle, length, previous, strength, lifeOffset) {
        const cosine = Math.cos(angle);
        const sine = Math.sin(angle);
        const before = { ...RawLightningPath.#at(particles, previous) };
        const token = RawLightningPath.#append(particles);
        RawLightningPath.#set(particles, token, before.x - cosine * length, sine * length + before.y, strength, 1, lifeOffset + float(fields, OFFSET.strength), previous);
        if (BRANCH_THRESHOLD < strength && this.#random.next(100) < 85) {
            const spread = float(fields, OFFSET.step) < MINIMUM_STEP ? 0.5 : 0.25;
            const offset = this.#randomBetween(-spread, spread);
            this.#branch(fields, particles, fround(offset * strength + angle), fround(length * 0.5), token, fround(strength * 0.5), lifeOffset);
        }
        return token;
    }

    // 00472180: 현재 진행까지 재귀 가지를 만들며 1 에 도달하면 끝점을 한 번 추가한다. step 검사와 실제 저장에 다른 난수를 쓰는 원문 분기를 한 번의 clamp 로 합치지 않는다. 사용: step(fields, particles, progress, lifeOffset). (2026-11-08 WB04-E117)
    step(fields, particles, progress, lifeOffset) {
        RawLightningPath.#validate(fields, particles);
        if (word(fields, OFFSET.done)) return;
        if (!Number.isFinite(progress) || !Number.isFinite(lifeOffset)) throw new RangeError('빔 경로 진행/수명 입력 오류');
        let limit = fround(progress);
        const offsetLife = fround(lifeOffset);
        if (limit > 1) limit = 1;
        // 누적 진행은 매회 float 저장한 값으로 다시 비교한다.
        while (float(fields, OFFSET.progress) < limit) {
            const step = float(fields, OFFSET.step);
            if (!(step > 0) || !Number.isFinite(step)) throw new Error('빔 경로 시작점 초기화 누락');
            const spread = step < MINIMUM_STEP ? 0.5 : 0.25;
            const offset = this.#randomBetween(-spread, spread);
            setFloat(fields, OFFSET.wobble, offset * step + float(fields, OFFSET.wobble));
            const current = this.#branch(fields, particles, fround(offset + float(fields, OFFSET.angle)), fround(float(fields, OFFSET.distance) * step), word(fields, OFFSET.last), 1, offsetLife);
            setWord(fields, OFFSET.last, current);
            setFloat(fields, OFFSET.progress, float(fields, OFFSET.progress) + float(fields, OFFSET.step));
            let maximum = fround(float(fields, OFFSET.step) * STEP_DECAY);
            if (maximum < MINIMUM_STEP) maximum = MINIMUM_STEP;
            setFloat(fields, OFFSET.maximum, maximum);
            const low = fround(maximum * THIRD);
            const probe = this.#randomBetween(low, maximum);
            setFloat(fields, OFFSET.step, probe < MINIMUM_STEP ? MINIMUM_STEP : this.#randomBetween(fround(float(fields, OFFSET.maximum) * THIRD), float(fields, OFFSET.maximum)));
        }
        if (limit === 1) {
            const token = RawLightningPath.#append(particles);
            RawLightningPath.#set(particles, token, float(fields, OFFSET.endX), float(fields, OFFSET.endY), 1, limit, float(fields, OFFSET.strength), word(fields, OFFSET.last));
            setWord(fields, OFFSET.done, 1);
        }
    }
}
