// Factory 의 가상 입력(004547d0)을 기존 연구/공통 객체 입력과 같은 월드에서 합성하는 모듈이다(cpppj `o/RawFactoryInput.cpp/.h`, 10.78 전용).
// 입력 명령은 상위 6개 수정 비트를 제외한(0x03ffffff) 값으로 현재 명령 전역과 비교한다. 순서: 순환(cycleCommand 0x03000063) → 업그레이드(upgradeCommand 0x03000032) → 연구(researchCommand 0x03000080) → 그 밖은 공통 부모(004ae010).
//  - 순환: 현재 저장 레벨(level) + 1 을 signed 3 으로 나눈 나머지를 setLevel 로 저장하고 adjustFrame 으로 현재 상태 프레임을 보정하며 1 을 반환한다.
//  - 업그레이드: 비용(cost, 정수 복호화 단위)을 float 로 줄이고 현재 로컬 SP(money(localOwner))를 읽는다. 편집기가 아니고 !(비용 <= SP)(NaN 은 unordered 라 자금 부족)이면 안내 'upgradeNoBucks' 후 1.
//    그렇지 않고 이미 진행 중(upgrading)이면 'alreadyUpgrading' 후 1, 아니면 request 로 실제 요청/예약/송신 후 1. 편집기에서도 SP 를 먼저 읽고 진행 중 검사는 자금 관문을 통과한 뒤에만 한다.
//  - 연구: 같은 code/packedArgument 를 기존 연구 입력(research)에 전달, 나머지는 parent 에 수정키 포함 원문을 위임한다.
// 임의 소유자/상한 관문을 추가하지 않으며 생성은 장치/게임 효과를 일으키지 않는다. 선택하지 않은 하위는 요구하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E18: Factory 입력 이식으로 추가)

// 원본 입력의 상위 수정 비트를 제외하는 마스크이다. (2026-11-08 WB04-E18)
const COMMAND_MASK = 0x03ffffff;

// 선택되지 않은 하위는 요구하지 않으며 누락된 효과를 빈 성공으로 바꾸지 않는다. (2026-11-08 WB04-E18)
function need(device) {
    if (typeof device !== 'function') throw new Error('Factory 실제 입력 하위 미연결');
    return device;
}

// 현재 모드/로컬소유자는 getter 로 읽고 원본 명령 전역의 기본값을 유지하는 상태이다. 사용: createFactoryInputState({ editor: () => 0, localOwner: () => 1 }). (2026-11-08 WB04-E18)
export function createFactoryInputState(sources) {
    return { editor: sources.editor, localOwner: sources.localOwner, cycleCommand: 0x03000063, upgradeCommand: 0x03000032, researchCommand: 0x03000080 };
}

// pool 은 현재 Factory TYPE 을 읽는 10.78 풀, state 는 위 상태(호출마다 현재 값을 읽음), hooks = { level, setLevel, adjustFrame, cost, money, upgrading, request, tell, research, parent }. (2026-11-08 WB04-E18)
export class RawFactoryInput {
    #pool;
    #state;
    #hooks;

    // 가상 입력은 호출할 때 실행하며 생성 시 장치/게임 효과를 일으키지 않는다.
    constructor(pool, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Factory 입력은 같은 PATCH 풀만 지원합니다');
        this.#pool = pool;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 원본 순서 cycle → upgrade → research → 부모를 실행한다. 반환은 DWORD 이다.
    input(parent, code, packedArgument) {
        const hooks = this.#hooks;
        const state = this.#state;
        const command = (code & COMMAND_MASK) >>> 0;
        if (command === (state.cycleCommand >>> 0)) {
            // 원본 signed 레벨 나머지를 보존한다.
            const next = ((need(hooks.level)(parent) + 1) | 0) % 3;
            need(hooks.setLevel)(parent, next >>> 0);
            need(hooks.adjustFrame)(parent);
            return 1;
        }
        if (command === (state.upgradeCommand >>> 0)) {
            // 비용은 비교 전에 float 로 축소되고 NaN 은 unordered 라 자금 부족이다.
            const cost = Math.fround(need(hooks.cost)(this.#pool.slot(parent)[10]));
            const money = need(hooks.money)(state.localOwner() >>> 0);
            if (!state.editor() && !(cost <= money)) { need(hooks.tell)('upgradeNoBucks'); return 1; }
            if (need(hooks.upgrading)(parent)) { need(hooks.tell)('alreadyUpgrading'); return 1; }
            need(hooks.request)(parent);
            return 1;
        }
        if (command === (state.researchCommand >>> 0)) return need(hooks.research)(parent, code, packedArgument);
        return need(hooks.parent)(parent, code, packedArgument);
    }

    // 호출자가 같은 실제 raw 풀을 사용하는지 확인하는 풀이다.
    get pool() { return this.#pool; }
}
