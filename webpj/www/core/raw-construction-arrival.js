// 건설 운반 객체(teleportEffect)의 도착 판정을 옮긴 모듈이다(cpppj `o/RawConstructionArrival.cpp/.h`, 10.78 00444080).
// 도착한 운반 객체의 현재 좌표를 정수 칸으로 바꿔 실제 점 finder 를 끝까지 방문하고, 서버가 확정한 건설 대상에서 실제 건설을 시작한다.
//  - arrive(builder): 도착 객체의 x/y 를 ftol → float32 저장 → ftol 한 칸으로 만들고(두 번의 절삭을 모두 보존), 그 칸 하나짜리 사각형으로 finder 를 시작한다.
//    후보마다 현재 raw 를 다시 읽는다: extra(+40) 비트 0(abstract)이 없으면 건너뛰고, 후보 TYPE 이 타입 표 밖이면 거부하며, 타입 flags1 에 needsBuilder(0x8000)가 없으면 건너뛴다.
//    남은 후보는 "찾음"이 되고, 후보 번호가 5 미만이거나 서버 영역(15000 이상)이면 `start(후보, duration)` 으로 건설을 시작한다("시작함"). 시작 callback 이 바꾼 현재 raw 와 해시는
//    finder 가 미리 저장한 next 이후의 후보에 그대로 반영된다. 사본·정렬·별도 후보 목록으로 대신하지 않는다.
//    순회가 끝난 뒤: 시작하지 못했는데 후보만 찾았다면(예측 대상만 보임) checking 이 꺼져 있을 때 도착 객체의 첫 streak contained 를 지우고(`removeContained(builder, 현재 streak TYPE)`)
//    1002(대기)를 반환한다. 그 밖에는(서버 대상에 시작했거나 대상이 없음) 자기 삭제(`destroy(builder, 0)`)를 하고 1001(종료)을 반환한다. 삭제 뒤에는 이 객체를 다시 읽지 않는다.
//  - makeConstructionArrivalHooks(pool, process, hooks): start 경계를 같은 풀의 실제 `RawConstructionProcess.start` 로 잇는다. 풀이 다르면 거부한다. 나머지(contained 삭제·가상 삭제)는 그대로 둔다.
// 상위 PathProcess 의 도착 호출(parent 가상 +0x3c)·경로 탐색/이동/재시도 분배는 이 모듈 밖이다. 10.72(CD) 경로(extra +35, 타입 171)는 범위 밖이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E133: 건설 도착 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 경로 반환 코드이다: 예측 대상만 있어 대기(1002), 시작했거나 대상이 없어 종료(1001). 상위 경로 효과는 이 몸체 밖이다. (2026-11-08 WB04-E133)
const WAITING = 1002;
const FINISHED = 1001;
// 타입 flags1 의 needsBuilder 비트이다. (2026-11-08 WB04-E133)
const NEEDS_BUILDER = 0x8000;
// 10.78 슬롯의 TYPE(+10)·x(+14)·y(+18)·abstract 가 있는 extra(+40) 위치와 타입 표 크기, 서버 영역의 첫 번호, 예측 영역의 마지막 보호 번호(5 미만)이다. (2026-11-08 WB04-E133)
const TYPE = 10;
const X_AT = 14;
const Y_AT = 18;
const EXTRA = 40;
const TYPE_COUNT = 188;
const SERVER_FIRST = 15000;
const LOW_LIMIT = 5;
// 도착 판정이 쓰는 필수 하위 효과 이름이다(생성 때 모두 확인한다). (2026-11-08 WB04-E133)
const HOOK_NAMES = Object.freeze(['start', 'removeContained', 'destroy']);

// 도착 판정이 호출 뒤에도 다시 읽는 원본 전역이다. checking(005e4794: 현재 DWORD 비영 뷰[숫자·bool·현재 값을 돌려주는 함수])·streakProcessType(005410a0: 기본 12)·
// duration(005325c0: float32, 기본 0.6)·sharedStreakProcessType(실제 Session 이 표시 생성/부착과 같은 전역을 빌려주는 함수)이다.
// 사용: 월드마다 하나를 두고 RawConstructionArrival 에 넘긴다. 도착 판정보다 오래 살아야 하며 값은 사용 위치마다 다시 읽는다. (2026-11-08 WB04-E133)
export class ConstructionArrivalState {
    checking = 0;
    streakProcessType = 12;
    duration = Math.fround(0.6);
    sharedStreakProcessType = null;

    // checking 을 현재 DWORD 로 읽는다(함수면 호출, bool 은 0/1). 소비 위치마다 부른다. (2026-11-08 WB04-E133)
    readChecking() {
        const value = typeof this.checking === 'function' ? this.checking() : this.checking;
        return typeof value === 'boolean' ? (value ? 1 : 0) : value >>> 0;
    }

    // 삭제가 선택된 위치에서만 현재 streak TYPE 을 읽는다. 빌린 전역이 없으면 독립 입력의 값을 쓴다. (2026-11-08 WB04-E133)
    currentStreakProcessType() {
        return this.sharedStreakProcessType ? this.sharedStreakProcessType() >>> 0 : this.streakProcessType >>> 0;
    }
}

// 정렬과 무관한 리틀엔디언 DWORD 를 단정도 float 로 읽는다. (2026-11-08 WB04-E133)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 원본 __ftol 의 64비트 절삭 결과 하위 signed DWORD 이다. 비유한 값과 64비트 범위 밖은 불확정값의 하위 0 으로 둔다. (2026-11-08 WB04-E133)
function truncate(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    const whole = Math.trunc(value);
    if (Math.abs(whole) <= 2147483647) return whole | 0;
    return Number(BigInt.asIntN(32, BigInt(whole)));
}

// 칸 하나: 좌표 → ftol → float32 저장 → ftol 이다(저장 때 24비트 가수로 반올림되는 큰 값도 원본과 같게 한다). (2026-11-08 WB04-E133)
function cellOf(raw, offset) {
    return truncate(Math.fround(truncate(coordinate(raw, offset))));
}

// 건설 운반 객체의 도착 판정 몸체이다. pool(10.78 SidPool)·hash(SquidHash)·types(현재 타입 표 188칸)·state(ConstructionArrivalState)는 이 객체보다 오래 살아야 하며 복사하지 않는다.
// hooks 의 start(sid, duration)·removeContained(sid, type)·destroy(sid, flags)는 필수이고 생성 때 하나라도 없으면 첫 효과 전에 거부한다.
// 사용: const arrival = new RawConstructionArrival(pool, hash, types, state, hooks); const code = arrival.arrive(builderSid); // 1001 또는 1002. (2026-11-08 WB04-E133)
export class RawConstructionArrival {
    #pool;
    #hash;
    #types;
    #state;
    #hooks;

    // 판본·타입 표 크기·필수 하위 효과를 확인한다. (2026-11-08 WB04-E133)
    constructor(pool, hash, types, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('건설 도착은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !state) throw new TypeError('건설 도착 자료/효과 연결 오류');
        for (const name of HOOK_NAMES) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError('건설 도착 자료/효과 연결 오류');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 실제 연결에서 같은 풀인지 확인하는 읽기용 참조이다. (2026-11-08 WB04-E133)
    get pool() { return this.#pool; }

    // 원본 00444080: 현재 좌표의 정수 칸 → 실제 finder → abstract/needsBuilder → 서버 대상 건설 시작 순서이다. 반환은 1002(예측만 있어 대기) 또는 1001(종료, 자기 삭제 뒤). (2026-11-08 WB04-E133)
    arrive(builder) {
        const raw = this.#pool.slot(builder);
        const x = cellOf(raw, X_AT);
        const y = cellOf(raw, Y_AT);
        let found = false;
        let started = false;
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 이전 반환의 next 는 finder 가 저장한다. 시작 callback 이 바꾼 현재 raw 도 이후 후보에 반영된다.
        for (let candidate = finder.begin({ left: x, top: y, right: x, bottom: y }); candidate !== 0; candidate = finder.next()) {
            const current = this.#pool.slot(candidate);
            if ((current[EXTRA] & 1) === 0) continue;
            if (current[TYPE] >= this.#types.length) throw new RangeError('건설 도착 후보 타입 오류');
            if ((this.#types[current[TYPE]].flags1 & NEEDS_BUILDER) === 0) continue;
            found = true;
            if (candidate < LOW_LIMIT || candidate >= SERVER_FIRST) {
                started = true;
                this.#hooks.start(candidate, this.#state.duration);
            }
        }
        if (!started && found) {
            if (this.#state.readChecking() === 0) this.#hooks.removeContained(builder, this.#state.currentStreakProcessType());
            return WAITING;
        }
        this.#hooks.destroy(builder, 0);
        return FINISHED;
    }
}

// 시작 경계를 같은 풀의 실제 RawConstructionProcess 로 잇는다. contained/가상 삭제 경계는 그대로 유지한다. 풀이 다르면 거부한다. (2026-11-08 WB04-E133)
export function makeConstructionArrivalHooks(pool, process, hooks) {
    if (pool !== process?.pool) throw new TypeError('건설 도착/진행 SID 풀 불일치');
    return { ...hooks, start: (sid, duration) => { process.start(sid, duration); } };
}
