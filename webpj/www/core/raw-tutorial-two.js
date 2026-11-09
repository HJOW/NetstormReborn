// Secret Workshop(튜토리얼 2)의 실제 A~H 단계 조건을 공유 단계/타이머와 같은 제작 장부에 연결하는 모듈이다(cpppj `o/RawTutorialTwo.cpp/.h`, PATCH 004c3bb0, 10.78 전용).
//  - handle(event): switch 진입 때 현재 단계(공유 미션 게이트 상태의 stage)를 선택하며 callback 이 다음 단계로 바꿔도 같은 호출에서 다른 분기로 넘어가지 않는다. 반환값은 항상 0(I 는 성공 섹션 자체가 처리).
//    'A': 옵션 BYTE 4 를 0, 2 를 2 로 쓰고 에너지 공급 반지름 갱신, 신전 부류(0x200) 제작 수 > 0 이면 Advance. 'B': 썬 워크샾 타입을 허용하고 워크샾 부류(0x4000) 제작 수 > 0 이면 Advance 뒤 선택 해제.
//    'C': 신전이 선택돼 있으면(선택 조회를 두 번 호출) 타이머가 0 일 때 지금 + 2초, 2초가 지났으면 `NotVortex` 섹션 Tell(열렸으면 임시 Gump 정리 뒤 이 미션의 정지) 뒤 선택 해제, 이어서 현재 생산 목록에 썬 디스크 발사기가 있으면 Advance.
//    'D': 발사기 제작 수 > 0 이면 Advance. 'E': > 1 이면 Advance 뒤 선택 해제. 'F': 타이머가 0 이면 신전이 선택됐을 때 지금 + 4초, 아니면 시각이 지났을 때 Advance 뒤 선택 해제.
//    'G': 발사기 제작 수 > 3 이면 Advance, 선택 해제, 타이머 0. 'H': 회수 금지 DWORD 를 0 으로 쓰고, 타이머가 0 이면 회수 완료 명령(`event & 0x03ffffff`)이 왔을 때 지금 + 2초, 아니면 시각이 지났을 때 Advance.
//  - 선택 해제는 resetSelection 뒤 clearSelection 을 항상 둘 다 부르며(Advance 가 거부돼도 유지), 허용/제작 집계는 E237 `RawTotalMade`, 단계 예약/정지는 E231 `RawMissionFrameGate` 를 그대로 쓴다.
// state = { battleOptions: Uint8Array(0052f7e0, 5 BYTE 이상), denySalvage(00595078), workshopType(005411ac), archerType(00541174), salvageCommand(0054286c) }(현재 전역을 읽고 쓰는 속성),
// hooks = { gameSeconds() → double, refreshEnergyRange(), selected() → SID, productionContains(type) → boolean, resetSelection(), clearSelection(), tell(섹션) → boolean, closeTransientGumps() } (모두 필수).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E238: Secret Workshop 단계 판정기 이식으로 추가)

// 원본 사건 마스크와 제작/선택 부류, ROM 에서 읽는 지연(보정 2초·에너지 4초)이다. (2026-11-10 WB04-E238)
const COMMAND_MASK = 0x03ffffff;
const TEMPLE_GENUS = 0x200;
const WORKSHOP_GENUS = 0x4000;
const CORRECTION_SECONDS = 2.0;
const ENERGY_SECONDS = 4.0;
// 필수 하위 경계 이름이다. 누락을 빈 성공으로 대체하지 않는다. (2026-11-10 WB04-E238)
const REQUIRED_HOOKS = Object.freeze(['gameSeconds', 'refreshEnergyRange', 'selected', 'productionContains', 'resetSelection', 'clearSelection', 'tell', 'closeTransientGumps']);

// PATCH 004c3bb0 전체이다. (2026-11-10 WB04-E238)
export class RawTutorialTwo {
    // 같은 풀/TYPE/TotalMade/게이트의 저장소와 현재 옵션·금지·타입·회수명령 전역, 실제 UI/생산/옵션 하위이다.
    #pool;
    #types;
    #made;
    #gate;
    #state;
    #hooks;

    // 원본 판본과 물리 저장소/필수 하위 경계를 효과 실행 전에 확인한다. (2026-11-10 WB04-E238)
    constructor(pool, types, made, gate, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || types?.length !== 188 || !(state?.battleOptions?.length >= 5) || REQUIRED_HOOKS.some((name) => typeof hooks[name] !== 'function')) {
            throw new TypeError('튜토리얼2 현재 원본 저장소/하위 연결 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#made = made;
        this.#gate = gate;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 원본 004d5430 을 실제로 두 번 호출한다. 첫 조회가 0 이면 두 번째 조회를 생략한다. 두 조회 사이 상태 변화도 두 번째 raw 슬롯에 반영한다. (2026-11-10 WB04-E238)
    #templeSelected() {
        if (!this.#hooks.selected()) return false;
        const selected = this.#hooks.selected(); // 첫 조회의 번호를 재사용하지 않는 현재 SID 이다.
        if (selected >= this.#pool.capacity || selected > 0xffff) throw new RangeError('튜토리얼2 현재 선택 SID 물리 범위');
        const type = this.#pool.slot(selected)[10]; // 원본 004ac200 이 읽는 실제 타입 BYTE 이다.
        if (type >= this.#types.length) throw new RangeError('튜토리얼2 현재 선택 TYPE 물리 범위');
        return (this.#types[type].flags2 & TEMPLE_GENUS) !== 0;
    }

    // 어느 한 단계 예약이 거부되어도 원본 선택 초기화/해제의 후속 호출은 생략하지 않는다. (2026-11-10 WB04-E238)
    #clearSelection() {
        this.#hooks.resetSelection();
        this.#hooks.clearSelection();
    }

    // 단계 한 분기만 실행한다. 반환값은 항상 0 이다. (2026-11-10 WB04-E238)
    handle(event) {
        const mission = this.#gate.state; // 단계/임시 타이머의 유일한 실제 저장소이다.
        const made = this.#made;
        const hooks = this.#hooks;
        const state = this.#state;
        const archer = () => state.archerType | 0;
        switch (mission.stage) {
        case 0x41: // 'A'
            state.battleOptions[4] = 0;
            state.battleOptions[2] = 2;
            hooks.refreshEnergyRange();
            if (made.builtForGenus(TEMPLE_GENUS) > 0) this.#gate.advance();
            break;
        case 0x42: // 'B'
            made.allow(state.workshopType | 0, 1);
            if (made.builtForGenus(WORKSHOP_GENUS) > 0) { this.#gate.advance(); this.#clearSelection(); }
            break;
        case 0x43: // 'C'
            if (this.#templeSelected()) {
                if (mission.timerSeconds === 0) mission.timerSeconds = hooks.gameSeconds() + CORRECTION_SECONDS;
                else if (mission.timerSeconds <= hooks.gameSeconds()) {
                    if (hooks.tell('NotVortex')) { hooks.closeTransientGumps(); this.#gate.pauseForDialog(); }
                    this.#clearSelection();
                }
            }
            if (hooks.productionContains(state.archerType)) this.#gate.advance();
            break;
        case 0x44: // 'D'
            if ((made.built(archer()) | 0) > 0) this.#gate.advance();
            break;
        case 0x45: // 'E'
            if ((made.built(archer()) | 0) > 1) { this.#gate.advance(); this.#clearSelection(); }
            break;
        case 0x46: // 'F'
            if (mission.timerSeconds === 0) {
                if (this.#templeSelected()) mission.timerSeconds = hooks.gameSeconds() + ENERGY_SECONDS;
            } else if (mission.timerSeconds <= hooks.gameSeconds()) { this.#gate.advance(); this.#clearSelection(); }
            break;
        case 0x47: // 'G'
            if ((made.built(archer()) | 0) > 3) { this.#gate.advance(); this.#clearSelection(); mission.timerSeconds = 0; }
            break;
        case 0x48: // 'H'
            state.denySalvage = 0;
            if (mission.timerSeconds === 0) {
                if (((event & COMMAND_MASK) >>> 0) === (state.salvageCommand >>> 0)) mission.timerSeconds = hooks.gameSeconds() + CORRECTION_SECONDS;
            } else if (mission.timerSeconds <= hooks.gameSeconds()) this.#gate.advance();
            break;
        default: break;
        }
        return 0;
    }
}
