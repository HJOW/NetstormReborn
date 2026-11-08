// 사제(Priest)의 소유자 재정의(00491790)를 raw SID 에 옮긴 모듈이다(cpppj `o/RawPriestOwner.cpp/.h`, 10.78 전용).
// 순서: 요새(편집)/전투/로딩 모드로 원래 소유자 WORD(+12)의 하위 7비트를 바꿀지 정한다 — 비전투이거나 로딩 중이면 요청 번호로 바꾸고(0 이면 진단 `newPlayerId != INVALID_PLAYER_ID`, .\Priest.cpp,
//   전투/비전투에 따라 0x5b/0x68), 로딩이 끝난 전투에서는 기존 원래 소유자를 다시 쓴다. 바뀐 경우(요청 ≠ 0) WORD 를 쓰고 무상태 알림(004214a0)을 한 번 부른다. 유효 소유자(≤ 8)이면 공통 소유자 지정(base)을 부른다.
//   로딩이 아니거나 전투이면 마지막으로 현재 WORD 의 로컬 표식 비트 7 을 (요새 모드이고 소유자가 요새 플레이어이거나 전투이고 소유자가 전투 플레이어일 때) 켜고 알림을 한 번 더 부른다.
// 실제 가상 진입 뒤에는 TYPE/free/dead 를 선행 거부하지 않고 가상 표만 확인한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E62: 사제 소유자 이식으로 추가)

// 사제 TYPE 번호와 10.78 사제 가상 표, 원래 소유자 WORD 위치와 마스크(하위 7비트·로컬 표식 비트 7), 플레이어 수이다. (2026-11-08 WB04-E62)
export const PRIEST_TYPE = 158;
export const PRIEST_VTABLE = 0x0050f210;
const WORD_OFFSET = 12;
const ORIGINAL_PLAYER = 0x7f;
const LOCAL_FLAG = 0x80;
const PLAYER_COUNT = 8;

// 사제 소유자 처리 중 조회·갱신하는 상태이다: 로딩 깊이(005c89b8), 요새 모드에서 비트 7 을 켜는 플레이어(00540cac), 전투 모드에서 비트 7 을 켜는 플레이어(00540c70). (2026-11-08 WB04-E62)
export class PriestOwnerState {
    loadingDepth = 0;
    fortPlayer = 0;
    battlePlayer = 0;

    // 초기값 객체로 만든다(생략한 칸은 0). (2026-11-08 WB04-E62)
    constructor({ loadingDepth = 0, fortPlayer = 0, battlePlayer = 0 } = {}) {
        this.loadingDepth = loadingDepth;
        this.fortPlayer = fortPlayer;
        this.battlePlayer = battlePlayer;
    }
}

// pool 은 10.78 SidPool, mode 는 SquidOwnerMode(fort·battle), state 는 PriestOwnerState, hooks = { base(sid, 소유자), notify(sid), assertion?(식, 파일, 줄) }.
//   base 와 notify 는 필수이고 assertion 은 요청 0 진단에 도달할 때만 요구한다. 풀/모드/상태는 이 객체보다 오래 살아야 한다. (2026-11-08 WB04-E62)
export class RawPriestOwner {
    // 객체 슬롯 풀, 소유자 모드 전역, 로딩/플레이어 상태, 외부 효과 묶음이다.
    #pool;
    #mode;
    #state;
    #hooks;

    // 원본 알림도 호출 순서를 검증할 수 있게 생략하지 않는다. (2026-11-08 WB04-E62)
    constructor(pool, mode, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('사제 소유자는 10.78 풀만 받습니다.');
        if (typeof hooks?.base !== 'function' || typeof hooks.notify !== 'function') throw new TypeError('사제 소유자 효과 누락');
        this.#pool = pool;
        this.#mode = mode;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 다른 타입의 가상 표를 사제로 바꾸어 처리하지 않는다. (2026-11-08 WB04-E62)
    handles(sid) {
        const raw = this.#pool.slot(sid);
        return ((raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0) === PRIEST_VTABLE;
    }

    // 사제 몸체의 공통 소유자 호출 여부와 이후 비트 변경 순서를 그대로 적용한다. player 는 부호 없는 요청 번호이다. (2026-11-08 WB04-E62)
    set(sid, player) {
        if (!this.handles(sid)) throw new TypeError('사제 소유자 가상 표 불일치');
        const raw = this.#pool.physical(sid);
        const requested = player >>> 0;
        const mode = this.#mode;
        const state = this.#state;
        // 요청 0 진단의 줄 번호는 최초에 선택한 전투/비전투 분기를 따른다.
        const initialBattle = mode.battle;
        const changesOriginal = !initialBattle || state.loadingDepth !== 0;
        const effective = changesOriginal ? requested : (raw[WORD_OFFSET] & ORIGINAL_PLAYER);
        // 패치의 범위 밖 인자는 공통 지정만 생략한다.
        const callsBase = (!changesOriginal || effective !== 0) && effective <= PLAYER_COUNT;
        if (changesOriginal && requested === 0) {
            const assertion = this.#hooks.assertion;
            if (typeof assertion !== 'function') throw new Error('사제 원래 소유자0의 실제 진단 장치 미연결');
            assertion('newPlayerId != INVALID_PLAYER_ID', '.\\Priest.cpp', initialBattle ? 0x5b : 0x68);
        }
        if (changesOriginal && requested !== 0) {
            raw[WORD_OFFSET] = (raw[WORD_OFFSET] & LOCAL_FLAG) | (requested & ORIGINAL_PLAYER);
            this.#hooks.notify(sid);
        }
        if (callsBase) this.#hooks.base(sid, effective);
        if (state.loadingDepth === 0 || mode.battle) {
            // 공통 소유자 호출 뒤의 현재 raw 단어를 고친다. 상위 바이트(+13)는 보존한다.
            const local = (mode.fort && effective === state.fortPlayer) || (mode.battle && effective === state.battlePlayer);
            raw[WORD_OFFSET] = (raw[WORD_OFFSET] & ORIGINAL_PLAYER) | (local ? LOCAL_FLAG : 0);
            this.#hooks.notify(sid);
        }
    }
}

// 섬/종유석 소유자 분배기와 합성할 수 있는 사제 전용 가상 분배기이다: 사제만 재정의하고 나머지는 base 로 넘긴다. priest 와 base 가 참조하는 월드는 반환 함수보다 오래 살아야 한다. (2026-11-08 WB04-E62)
export function makePriestOwnerDispatch(priest, base) {
    if (typeof base !== 'function') throw new TypeError('사제 소유자 분배의 base 누락');
    return (sid, player) => {
        if (priest.handles(sid)) priest.set(sid, player);
        else base(sid, player);
    };
}
