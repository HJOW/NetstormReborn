// 피해 후처리의 Dude 드롭 두 부모(00450b30 난수 위치·00450c20 진입점)를 같은 생성/task/진입점 모듈에 연결하는 모듈이다(cpppj `o/RawDudeDrop.cpp/.h`, 10.78 전용).
//  - new RawDudeDrop(pool, types, spawn, task, notice, random): spawn 은 RawDudeSpawn(`raw-dude-spawn.js`), task 는 RawDudeTask(`raw-dude-task.js`), notice 는 RawDudeNotice(`raw-dude-notice.js`,
//    표적 진입점 residencePoint 만 씀), random 은 공유 GameRandom(`random.js`)이다. 모두 같은 풀이어야 하며 이 객체보다 오래 살아야 한다.
//  - drop(source, index, random):
//    · source SID 0 이면 원본 assert(DudeDropSourceAssertError)다. source 가 해제(상태 비트 1)면 아무것도 하지 않는다(dead 부모의 정상 PostDestroy 드롭은 계속한다).
//    · random=true 면 현재 TYPE 발자국 안의 두 난수 좌표(x 먼저, [0.1, foot−0.1] 구간의 53비트 중간값을 x/y 에서 빼고 float 로 저장)를, false 면 실제 표적 진입점을 쓴다.
//    · 그 좌표와 source 의 현재 소유자로 Dude 를 생성한다. 생성되지 않았거나 새 Dude 가 해제/dead 면 그대로 반환한다.
//    · 아니면 task 를 random 이면 0x5103·index×0.1f, 아니면 0x5104·index×0.7f(float 지연)로 설정한다.
// CD 10.72 분기(SID 0 진단 없이 반환·생성 실패에도 task 호출)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E208: Dude 드롭 부모 이식으로 추가)

// 실제 PE 의 두 지연 상수(00501784 의 3333333f 는 0.7f)와 드롭 작업 번호, 원본 float 난수 분모이다. (2026-11-09 WB04-E208)
const RANDOM_DELAY = Math.fround(0.1);
const POINT_DELAY = Math.fround(0.7);
const RANDOM_TASK = 0x5103;
const POINT_TASK = 0x5104;
const RANDOM_DIVISOR = 65536;

// 슬롯의 상태·TYPE·좌표·소유자 위치와 발자국 상한이다. (2026-11-09 WB04-E208)
const STATE_AT = 11;
const TYPE_AT = 10;
const X_AT = 14;
const Y_AT = 18;
const OWNER_AT = 34;
const FOOT_LIMIT = 256;

// float 비트 변환용 공용 보기이다. (2026-11-09 WB04-E208)
const FLOAT_VIEW = new DataView(new ArrayBuffer(4));

// 원본 비정렬 현재 좌표 DWORD 를 IEEE float 로 읽는다. (2026-11-09 WB04-E208)
function coordinate(raw, offset) {
    FLOAT_VIEW.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return FLOAT_VIEW.getFloat32(0, true);
}

// source SID 0 의 원본 assert(패치 452/469)를 가리키는 오류이다. 정상 반환과 구별한다. (2026-11-09 WB04-E208)
export class DudeDropSourceAssertError extends Error {
    constructor() {
        super('Dude drop sourceSID 원본 assert');
        this.name = 'DudeDropSourceAssertError';
    }
}

// Dude 드롭 두 부모이다. 사용: new RawDudeDrop(pool, types, spawn, task, notice, random).drop(source, index, true). (2026-11-09 WB04-E208)
export class RawDudeDrop {
    // 현재 source/새 Dude 의 풀, 발자국 TYPE 표, 생성/task/진입점 몸체와 공유 난수이다.
    #pool;
    #types;
    #spawn;
    #task;
    #notice;
    #random;

    // 같은 풀을 쓴 실제 하위 몸체만 연결한다. (2026-11-09 WB04-E208)
    constructor(pool, types, spawn, task, notice, random) {
        if (pool?.edition !== 'patch1078' || spawn?.pool !== pool || task?.pool !== pool || notice?.pool !== pool) throw new TypeError('Dude drop/Spawn/Task/Point SID 풀 불일치');
        this.#pool = pool;
        this.#types = types;
        this.#spawn = spawn;
        this.#task = task;
        this.#notice = notice;
        this.#random = random;
    }

    // 실제 Damageable PostDestroy 의 같은 월드 연결을 확인할 때 쓰는 풀이다. (2026-11-09 WB04-E208)
    get pool() {
        return this.#pool;
    }

    // float 최솟값/최댓값의 차이를 53비트로 계산하며 최종 좌표 저장 전까지 float 로 절삭하지 않는다. (2026-11-09 WB04-E208)
    #randomFloat(minimum, maximum) {
        const factor = this.#random.next(RANDOM_DIVISOR) / RANDOM_DIVISOR;
        return factor * Math.abs(maximum - minimum) + minimum;
    }

    // 원본은 source free 만 검사하므로 죽은 부모의 정상 PostDestroy 드롭도 이 호출을 쓴다. (2026-11-09 WB04-E208)
    drop(source, index, random) {
        if (!source) throw new DudeDropSourceAssertError();
        const raw = this.#pool.slot(source);
        if (raw[STATE_AT] & 1) return;
        const type = raw[TYPE_AT];
        if (type >= this.#types.length) throw new RangeError('Dude drop 현재 타입 오류');
        let point;
        if (random) {
            const record = this.#types[type];
            if (record.footX < 0 || record.footY < 0 || record.footX > FOOT_LIMIT || record.footY > FOOT_LIMIT) throw new RangeError('Dude drop 원본 발자국 범위 오류');
            const x = coordinate(raw, X_AT);
            const y = coordinate(raw, Y_AT);
            if (!Number.isFinite(x) || !Number.isFinite(y)) throw new RangeError('Dude drop 유한 source 좌표 오류');
            // x/y 난수 소비와 각 최종 float 저장 순서를 원본대로 유지한다.
            const px = Math.fround(x - this.#randomFloat(RANDOM_DELAY, Math.fround(Math.fround(record.footX) - RANDOM_DELAY)));
            const py = Math.fround(y - this.#randomFloat(RANDOM_DELAY, Math.fround(Math.fround(record.footY) - RANDOM_DELAY)));
            point = [px, py];
        } else point = this.#notice.residencePoint(source);
        const born = this.#spawn.spawn(point[0], point[1], this.#pool.slot(source)[OWNER_AT]);
        if (!born || (this.#pool.slot(born)[STATE_AT] & 3)) return;
        const delay = Math.fround((index | 0) * (random ? RANDOM_DELAY : POINT_DELAY));
        this.#task.set(born, random ? RANDOM_TASK : POINT_TASK, delay);
    }
}
