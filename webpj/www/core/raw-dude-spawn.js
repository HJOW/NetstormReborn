// Dude 생성(00450a20)과 최초 Pop 후처리(00450cd0)의 작업 번호·population 등록·I 첫 프레임을 같은 raw 월드에서 실행하는 모듈이다(cpppj `o/RawDudeSpawn.cpp/.h`, 10.78 전용).
//  - createDudeSpawnState(population, options): 생성 한도와 최초 Pop 등록이 공유하는 현재 상태 { population(SquidPostPopList), blocked(패치 0055a4e8), dudeType(00541170, 기본 70) } 이다.
//  - new RawDudeSpawn(pool, factory, owner, state, hooks).spawn(x, y, owner): 현재 signed 개수 ≥ 용량이거나 차단이면 0(생성 안 함). 아니면 실제 SquidFactory 로 dudeType 을 flags 2 로 만들고
//    프레임 DWORD(+36)를 0 으로 쓴 뒤 SquidOwner 로 소유자를 지정하고 hooks.pop(sid, x, y, 0)(실제 가상 +90 Pop)을 부른 뒤 새 SID 를 돌려준다. 용량/차단은 이 순서로 매번 현재 값을 읽고,
//    TYPE·좌표·배열은 미리 검사하지 않는다(NaN 좌표도 Pop 소비자에 그대로 넘긴다). Pop 미연결은 생성·소유자·프레임 효과 뒤 그 위치에서 거부한다.
//  - new RawDudePostPop(pool, frames, state, hooks, bridgeType).postPop(sid, flags): 최초 flags&1 이면 물리 슬롯 +12 WORD 에 idle 작업 0x5101 을 쓰고, 현재 TYPE 이 현재 Bridge DWORD 와
//    같으면 hooks.notifySurface(sid)(004214a0)를 부르고, signed 개수 < 용량이면 그 개수 위치 배열에 SID 를 저장한 뒤 그때의 현재 개수 + 1 을 쓰고, 현재 TYPE 의 'I' 첫 프레임(-1 포함)을
//    hooks.setFrame(sid, first, 0) 로 쓴다. flags 와 무관하게 마지막에 hooks.base(sid, flags)(004b0d30 공통 후처리)를 부른다. 앞선 효과는 하위 실패에도 되돌리지 않는다.
//    frames 는 TYPE 번호로 찾는 FrameCodeTable(`assets/terrain-tiles.js`) 배열, bridgeType 은 005411a0 의 현재 DWORD getter(기본 82)이다.
//    handles(sid) 는 실제 10.78 Dude 가상 표(0x508cc8)만 보며 현재 TYPE·할당 상태를 관문으로 삼지 않는다. validate(sid, flags) 는 원문 호출에 끼우지 않는 명시 사전 검사다.
//  - makeDudePostPopHooks(pool, frame, base): setFrame/validateBase/base 를 같은 풀의 실제 SquidFrame·SquidPostPop 에 잇는다(notifySurface 는 호출자가 따로 공급).
// CD 10.72 분기(BYTE 프레임·유한 좌표 사전 검사·구성 때 공통 몸체 요구)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E206: Dude 생성/최초 Pop 이식으로 추가)

// 독립 구성의 원본 초기 Bridge 번호이다. 실제 월드는 현재 DWORD 를 따로 빌린다. (2026-11-09 WB04-E206)
export const DUDE_INITIAL_BRIDGE_TYPE = 82;

// 실제 Dude 생성자가 기록하는 10.78 가상 표와 최초 등록 idle 작업 번호이다. (2026-11-09 WB04-E206)
const DUDE_VTABLE = 0x508cc8;
const IDLE_TASK = 0x5101;

// 슬롯의 작업 WORD·TYPE·프레임 위치와 DWORD 상한, 'I' 글자이다. (2026-11-09 WB04-E206)
const TASK_AT = 12;
const TYPE_AT = 10;
const FRAME_AT = 36;
const DWORD_MAX = 0xffffffff;
const INT32_MAX = 0x7fffffff;
const LETTER_I = 0x49;

// 도달한 장치만 요구한다. (2026-11-09 WB04-E206)
function need(device) {
    if (typeof device !== 'function') throw new Error('Dude의 선택된 실제 하위 미연결');
    return device;
}

// 슬롯의 가상 표 DWORD 이다. (2026-11-09 WB04-E206)
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// 현재 용량을 원본 signed 비교에 쓸 DWORD 로 읽는다. (2026-11-09 WB04-E206)
function capacityOf(population) {
    const capacity = population.capacity();
    if (!Number.isInteger(capacity) || capacity < 0 || capacity > DWORD_MAX) throw new RangeError('Dude population DWORD 용량 물리 범위');
    return capacity >>> 0;
}

// 원본 signed count/capacity 의 정상 비음수 목록 계약을 확인한다(명시 사전 검사 전용). (2026-11-09 WB04-E206)
function validatePopulation(population) {
    if (population.capacity() > INT32_MAX) throw new RangeError('Dude population signed capacity 오류');
    population.items();
}

// 생성 한도와 최초 Pop 등록이 공유하는 현재 상태(cpppj DudeSpawnState)이다. (2026-11-09 WB04-E206)
export function createDudeSpawnState(population, { blocked = false, dudeType = 70 } = {}) {
    return { population, blocked, dudeType };
}

// 00450a20 의 Dude 생성이다. 사용: new RawDudeSpawn(pool, factory, owner, state, { pop }).spawn(x, y, owner). (2026-11-09 WB04-E206)
export class RawDudeSpawn {
    // 새 Dude 와 population SID 가 공유하는 풀, 실제 생성/소유자 몸체, 현재 상태와 Pop 연결이다.
    #pool;
    #factory;
    #owner;
    #state;
    #hooks;

    // factory/owner 와 후속 Pop 은 같은 SID 풀을 써야 한다. Pop 미연결은 원본 호출 시점에 거부한다. (2026-11-09 WB04-E206)
    constructor(pool, factory, owner, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || factory?.pool !== pool || owner?.pool !== pool) throw new TypeError('Dude spawn/factory/owner SID 풀 불일치');
        this.#pool = pool;
        this.#factory = factory;
        this.#owner = owner;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 최초 Pop 처리기와 같은 월드의 생성 연결을 검사할 때 쓰는 풀이다. (2026-11-09 WB04-E206)
    get pool() {
        return this.#pool;
    }

    // 실제 PE 인자는 x/y/owner 세 개다. 생성 조건이 거짓이면 0, 성공하면 실제 Pop 을 마친 새 Dude SID 이다. (2026-11-09 WB04-E206)
    spawn(x, y, owner) {
        const state = this.#state;
        const used = state.population.readCount();
        const capacity = capacityOf(state.population);
        // 용량 조회 효과가 바꾼 차단 전역도 그 뒤에 읽는다.
        if ((used | 0) >= (capacity | 0) || state.blocked) return 0;
        const sid = this.#factory.create(state.dudeType, 2);
        // 패치는 프레임 DWORD 전체를 0 으로 쓴다(물리 슬롯).
        this.#pool.physical(sid).fill(0, FRAME_AT, FRAME_AT + 4);
        this.#owner.set(sid, owner);
        need(this.#hooks.pop)(sid, x, y, 0);
        return sid;
    }
}

// 00450cd0 의 Dude 최초 Pop 후처리 직접 몸체이다. 공유 Pop 의 가상 분배 등록(SquidPostPop.setBody('dude', …))은 호출자가 한다. (2026-11-09 WB04-E206)
export class RawDudePostPop {
    // 최초 작업 WORD 와 population SID 가 공유하는 풀, 현재 TYPE 의 I 첫 프레임 표, 현재 상태, Bridge DWORD getter, 실제 하위 연결이다.
    #pool;
    #frames;
    #state;
    #bridgeType;
    #hooks;

    // frames·population·Bridge DWORD 는 같은 월드 자료이며 처리기보다 오래 살아야 한다. (2026-11-09 WB04-E206)
    constructor(pool, frames, state, hooks = {}, bridgeType = () => DUDE_INITIAL_BRIDGE_TYPE) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(frames) || typeof bridgeType !== 'function') throw new TypeError('Dude postPop 10.78 풀/프레임 표/Bridge 연결 오류');
        this.#pool = pool;
        this.#frames = frames;
        this.#state = state;
        this.#hooks = hooks ?? {};
        this.#bridgeType = bridgeType;
    }

    // 공유 가상 분배와 같은 SID 풀인지 검사할 때 쓰는 풀이다. (2026-11-09 WB04-E206)
    get pool() {
        return this.#pool;
    }

    // 실제 가상 표 진입에 현재 TYPE 나 할당 상태를 추가 관문으로 만들지 않는다. (2026-11-09 WB04-E206)
    handles(sid) {
        return vtableOf(this.#pool.slot(sid)) === DUDE_VTABLE;
    }

    // 현재 TYPE 의 I 구간 첫 번호이다. 표 밖 TYPE 은 물리 범위 오류다. (2026-11-09 WB04-E206)
    #firstI(type) {
        if (type >= this.#frames.length) throw new RangeError('Dude postPop 현재 Run 표 물리 범위');
        return this.#frames[type].run(LETTER_I).first;
    }

    // 최초 쓰기 전에 현재 Run/목록과 실제 공통 후처리 연결을 확인하는 명시 사전 검사다(원문 호출 앞에 끼우지 않는다). (2026-11-09 WB04-E206)
    validate(sid, flags) {
        this.#pool.allocated(sid);
        if (!this.handles(sid) || this.#pool.slot(sid)[TYPE_AT] !== this.#state.dudeType) throw new TypeError('Dude postPop 실제 타입/가상 표 오류');
        if (flags & 1) {
            validatePopulation(this.#state.population);
            const type = this.#pool.slot(sid)[TYPE_AT];
            const first = this.#firstI(type);
            if (first < 0 || first >= this.#frames[type].codes.length) throw new RangeError('Dude postPop 실제 I 프레임 미연결');
            if (typeof this.#hooks.setFrame !== 'function') throw new Error('Dude postPop 실제 프레임 쓰기 미연결');
        }
        need(this.#hooks.validateBase)(sid, flags);
    }

    // 처음만 idle/등록/I 프레임을 쓰며 가득 찬 population 에도 프레임/공통 호출은 유지한다. (2026-11-09 WB04-E206)
    postPop(sid, flags) {
        if (flags & 1) {
            const raw = this.#pool.physical(sid);
            raw[TASK_AT] = IDLE_TASK & 0xff;
            raw[TASK_AT + 1] = IDLE_TASK >>> 8;
            // QA 저장 직후 현재 TYPE BYTE 와 현재 Bridge DWORD 전체를 비교한다.
            if (this.#pool.slot(sid)[TYPE_AT] === (this.#bridgeType() >>> 0)) need(this.#hooks.notifySurface)(sid);
            const population = this.#state.population;
            // 원문 cmp 에 앞선 index 는 보존하고 배열/개수는 도달 위치에서 다시 읽는다.
            const used = population.readCount();
            const capacity = capacityOf(population);
            if ((used | 0) < (capacity | 0)) {
                if ((used | 0) < 0) throw new RangeError('Dude population 음수 저장 위치');
                const storage = population.storage();
                if (used >= storage.length) throw new RangeError('Dude population 실제 배열 저장 범위');
                storage[used] = sid >>> 0;
                population.setCount(population.readCount() + 1);
            }
            // TYPE+150 의 현재 값은 코드 개수로 사전 거부하지 않고 실제 SetFrame 에 전달한다.
            const first = this.#firstI(this.#pool.slot(sid)[TYPE_AT]);
            need(this.#hooks.setFrame)(sid, first, 0);
        }
        need(this.#hooks.base)(sid, flags);
    }
}

// 직접 후처리를 기존 실제 Frame/Base 에 연결한다. 자동 가상 표 확장은 하지 않는다. (2026-11-09 WB04-E206)
export function makeDudePostPopHooks(pool, frame, base) {
    if (frame?.pool !== pool || base?.pool !== pool) throw new TypeError('Dude postPop/Frame/Base SID 풀 불일치');
    return {
        setFrame: (sid, value, flags) => frame.set(sid, value, flags),
        validateBase: (sid, flags) => base.validateBase(sid, flags),
        base: (sid, flags) => base.postPopBase(sid, flags),
    };
}
