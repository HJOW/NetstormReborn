// 실제 Player152 의 첫 생성(00490250)·파생 postPop(004905d0)·Technology SID 조회(0048fc60/0048fc90)를 같은 월드 기록에 연결하는 모듈이다(cpppj `o/RawPlayerWorld.cpp/.h`, 10.78 전용).
// 역할:
//  - create(owner): PlayerKnown 진단 → 좌표 (owner+10, special ? 10 : 1) 의 같은 타입 검색(있으면 진단) → Factory Create(현재 playerType, flags 2) → 가상 Owner → 물리 슬롯 +36 DWORD 0 → 가상 Pop(flags 0).
//    진단이 돌아온 뒤 현재 playerType/special 을 다시 읽는다(진단 효과가 바꾼 값을 쓴다).
//  - postPop(sid, flags): 10.78 은 사전 검사 없이 물리 슬롯 owner(+34)의 Player 9기록 Technology DWORD(+0x9c)에 SID 를 먼저 쓰고,
//    최초(flags 비트 1)·owner BYTE == 현재 battleOwner DWORD·fort 아님이면 Regular 예약(new 0x28) 성공 시 event 1/payload 5.0 Regular 를 만든다.
//    그 뒤 최초이고 현재 special 이 0 이 아니면 물리 슬롯 +36 DWORD 1 을 쓰고, 마지막에 공통 몸체(postPopBase)를 부른다. 하위 실패 전의 쓰기는 되돌리지 않는다.
//  - currentTechnology()/technology(owner): PlayerKnown 이 0 이면 현재 getter 는 0 을, 명시 getter 는 진단 뒤 실제 DWORD 를 읽는다.
// 상태(PlayerWorldState)는 owner 두 개와 모드를 호출마다 다시 읽는 공유 객체이고, 9기록(Uint8Array 9×0xb4)은 Money/Meta/AI 와 같은 저장소를 빌린다.
// 하위 장치(hooks)는 도달한 위치에서만 요구한다: assertion(kind), find(x, y, type) → SID(0=없음), create(type, flags) → SID, setOwner(sid, owner), pop(sid, x, y, flags),
//  reserveRegular() → boolean, addRegular(sid, event, payload). 미연결 장치는 기본 성공으로 바꾸지 않고 그 위치에서 오류다.
// CD 10.72 판본 분기(BYTE frame +34·0xac 기록·사전 Validate)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E55: Player 월드 연결 이식으로 추가)

// 실제 Player152 생성자가 쓰는 10.78 가상 표 주소이다. (2026-11-08 WB04-E55)
export const PLAYER_VTABLE = 0x0050ee08;

// Player 한 기록의 크기(0xb4)와 기록 안 Technology DWORD 위치(+0x9c), 기록 수(9)이다. (2026-11-08 WB04-E55)
export const PLAYER_RECORD_STRIDE = 0xb4;
export const PLAYER_TECHNOLOGY_OFFSET = 0x9c;
export const PLAYER_RECORD_COUNT = 9;

// 원본 assert 위치 이름이다: PlayerKnown 이 0(10.78 Player.cpp 0x351/0x29d), 같은 좌표에 Player 가 이미 있음(0x354). 진단 뒤 계속한다. (2026-11-08 WB04-E55)
export const PLAYER_WORLD_ASSERTION = Object.freeze({ playerIdKnown: 'PlayerIdKnown', playerAlreadyExists: 'PlayerAlreadyExists' });

// 슬롯 필드 위치: 소유자 BYTE(+34)와 직접 frame DWORD(+36)이다. (2026-11-08 WB04-E55)
const OWNER_OFFSET = 34;
const FRAME_OFFSET = 36;

// Player 생성 flags(서버 풀의 client 영역 할당)·Regular 사건 번호·첫 payload 이다. (2026-11-08 WB04-E55)
const CREATE_FLAGS = 2;
const FIRST_EVENT = 1;
const FIRST_PAYLOAD = 5.0;

// 두 owner 와 모드를 같은 현재 전역으로 공유하는 상태이다(10.78 주소). 호출마다 다시 읽으며 사본을 만들지 않는다.
// 사용: const state = new PlayerWorldState(); state.playerKnown = 1; state.battleOwner = 2. (2026-11-08 WB04-E55)
export class PlayerWorldState {
    // 00595344: 현재 local Player 번호가 알려졌는가이다.
    playerKnown = 0;
    // 00540c70: CurrentTechnology 와 최초 local 예약의 현재 owner(DWORD)이다.
    battleOwner = 1;
    // 00540cac: 저장 Template 선택용 owner 이다. Player 월드 네 몸체는 쓰지 않고 Regular 몸체가 읽는다.
    templateOwner = 1;
    // 005412b8: Create/Find 가 매 호출 읽는 playerType 전역이다.
    playerType = 152;
    // 00594fb8: local 최초 Regular 1 을 생략하는 fort(편집) 모드이다.
    fortMode = false;
    // 0055a480: 생성 y=10 과 최초 직접 frame 1 을 고르는 원시 DWORD 이다.
    special = 0;
}

// 선택된 하위 장치를 꺼낸다. 없으면 도달한 위치에서 오류이다. (2026-11-08 WB04-E55)
function need(device, name) {
    if (typeof device !== 'function') throw new Error(`Player 의 선택된 실제 하위 미연결: ${name}`);
    return device;
}

// 비정렬 little endian DWORD 를 지정 폭만큼 쓴다(주변 바이트 보존). (2026-11-08 WB04-E55)
function putDword(bytes, offset, value) {
    // 낮은 바이트부터 네 바이트만 쓴다.
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 비정렬 little endian DWORD 를 읽는다. (2026-11-08 WB04-E55)
function getDword(bytes, offset) {
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

// Player 월드 연결 몸체이다. 같은 풀·공통 후처리(SquidPostPop)·9기록·상태를 빌리며 이들이 이 객체보다 오래 살아야 한다.
// 사용: const player = new RawPlayerWorld(pool, post, records, state, hooks); post.setBody('player', player); player.create(1). (2026-11-08 WB04-E55)
export class RawPlayerWorld {
    // 실제 Factory/Pop/호스트와 같은 살아 있는 raw SID 풀이다.
    #pool;
    // 가상 재분배 없이 마지막 공통 장부 몸체(postPopBase)를 실행하는 SquidPostPop 이다.
    #base;
    // Money/Meta/AI 와 공유하는 실제 현재 9개 Player BYTE 저장소(9×0xb4)이다.
    #players;
    // PlayerKnown·두 owner·fort/special 을 호출마다 다시 읽는 공유 상태이다.
    #state;
    // 원본 필수 하위 생성/공간/예약/진단의 실제 연결이다(도달 위치에서 요구).
    #hooks;

    // 같은 풀의 공통 후처리와 실제 9기록 크기를 확인한다. 10.78 은 하위 장치를 생성 때 요구하지 않는다(도달 위치에서 요구).
    constructor(pool, base, playerRecords, state, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Player 월드는 10.78 SID 풀만 받습니다.');
        if (base?.pool !== pool) throw new TypeError('Player 공통 후처리와 SID 풀이 다릅니다.');
        if (!(playerRecords instanceof Uint8Array) || playerRecords.length !== PLAYER_RECORD_COUNT * PLAYER_RECORD_STRIDE) throw new RangeError('Player 실제 9기록 크기 오류');
        if (!(state instanceof PlayerWorldState)) throw new TypeError('Player 월드 상태가 필요합니다.');
        this.#pool = pool;
        this.#base = base;
        this.#players = playerRecords;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 월드/분배기가 다른 풀을 연결하지 않도록 같은 풀을 제공한다.
    get pool() { return this.#pool; }

    // 상위 캠페인 부모가 이름/지역 초기화에 직접 쓰는 같은 9기록 저장소이다(사본이 아니다).
    get records() { return this.#players; }

    // 상위 부모가 매 하위 호출 직전 다시 읽는 같은 현재 상태이다.
    get state() { return this.#state; }

    // 기록 안 Technology DWORD 위치이다. 원본 wrapper 에 owner 범위 진단이 없으므로 9기록 밖은 명시 계약 오류다.
    #technologyOffset(owner) {
        const value = owner >>> 0;
        if (value >= PLAYER_RECORD_COUNT) throw new RangeError('Player Technology owner 의 실제 9기록 범위 오류');
        return value * PLAYER_RECORD_STRIDE + PLAYER_TECHNOLOGY_OFFSET;
    }

    // 00490250: 진단 → 검색 → Create(type, 2) → Owner → 직접 frame 0 → Pop(0) 순서로 Player 를 만든다. 반환은 SID 이다.
    create(owner) {
        const state = this.#state;
        const hooks = this.#hooks;
        if (!state.playerKnown) need(hooks.assertion, 'assertion')(PLAYER_WORLD_ASSERTION.playerIdKnown);
        // 원본 signed32 좌표의 감김 덧셈과 float 변환을 보존한다. 진단 뒤의 현재 special 을 읽는다.
        const x = Math.fround((owner + 10) | 0);
        const y = state.special ? 10.0 : 1.0;
        if (need(hooks.find, 'find')(x, y, state.playerType)) need(hooks.assertion, 'assertion')(PLAYER_WORLD_ASSERTION.playerAlreadyExists);
        const sid = need(hooks.create, 'create')(state.playerType, CREATE_FLAGS);
        need(hooks.setOwner, 'setOwner')(sid, owner >>> 0);
        // 물리 슬롯의 직접 frame DWORD 0 쓰기이다(현재 free/TYPE/VT 와 무관).
        putDword(this.#pool.physical(sid), FRAME_OFFSET, 0);
        need(hooks.pop, 'pop')(sid, x, y, 0);
        return sid;
    }

    // 실제 Player 생성자 VT 만 분배 대상으로 인정한다. 10.78 은 물리 슬롯을 읽는다.
    handles(sid) { return getDword(this.#pool.slot(sid), 0) === PLAYER_VTABLE; }

    // 독립 명시 사전 검사이다: Player VT·할당 슬롯·owner 의 9기록 범위·공통 사전 검사(공간 효과 전). 10.78 가상 postPop 은 이 검사를 먼저 부르지 않는다.
    validate(sid, flags, pop = null) {
        if (!this.handles(sid)) throw new Error('미복원 Player 가상 postPop 표');
        this.#technologyOffset(this.#pool.allocated(sid)[OWNER_OFFSET]);
        this.#base.validateBase(sid, flags, pop);
    }

    // 004905d0: 현재 owner 의 Technology SID 쓰기 → 조건부 예약 → special 직접 frame 1 → 비가상 공통 postPop 이다.
    postPop(sid, flags) {
        const state = this.#state;
        const hooks = this.#hooks;
        // 같은 물리 슬롯의 살아 있는 보기이며 하위 효과 뒤에도 현재 BYTE 를 다시 읽는다.
        const raw = this.#pool.physical(sid);
        putDword(this.#players, this.#technologyOffset(raw[OWNER_OFFSET]), sid);
        // owner BYTE 와 현재 battleOwner DWORD 전체를 비교한다(257 과 BYTE 1 은 다르다).
        if ((flags & 1) && raw[OWNER_OFFSET] === state.battleOwner && !state.fortMode) {
            if (need(hooks.reserveRegular, 'reserveRegular')()) need(hooks.addRegular, 'addRegular')(sid, FIRST_EVENT, FIRST_PAYLOAD);
        }
        // 예약 효과 뒤 바뀐 special 을 다시 읽는다.
        if ((flags & 1) && state.special) putDword(raw, FRAME_OFFSET, 1);
        this.#base.postPopBase(sid, flags);
    }

    // 0048fc60: PlayerKnown 이 0 이면 기록/owner 를 읽지 않고 0, 아니면 현재 battleOwner 의 실제 DWORD 이다.
    currentTechnology() {
        const state = this.#state;
        return state.playerKnown ? getDword(this.#players, this.#technologyOffset(state.battleOwner)) : 0;
    }

    // 0048fc90: PlayerKnown 진단 뒤 명시 owner 의 실제 DWORD 를 읽는다(진단 효과가 쓴 현재 기록을 읽는다).
    technology(owner) {
        if (!this.#state.playerKnown) need(this.#hooks.assertion, 'assertion')(PLAYER_WORLD_ASSERTION.playerIdKnown);
        return getDword(this.#players, this.#technologyOffset(owner));
    }
}
