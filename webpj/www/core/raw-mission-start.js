// PATCH 004c2b20 의 공통 미션 시작 순서를 같은 미션 상태/현재 전역과 실제 효과 경계로 옮긴 모듈이다(cpppj `o/RawMissionStart.cpp/.h`, 10.78 전용).
//  - start()(004c2b20): 보조 필드 두 개를 0 으로 쓰고 → 설정 `denySalvage`·`denyAscend`·`allowAnyCapture`(atol, 전체 DWORD)와 `moreGeysers`의 논리 부정(`ordinaryGeysers`)을 현재 전역에 쓰고
//    → `demoRestartTimer` 가 비영이면 데모 종료 기한 = 그 값 + 현재 게임 시각(double) → 섹션 이름 목록(`@`)을 `tellAtList` 에 쓰고 첫 tellAt 을 예약하고 sequence 증가·flags 의 비트 0 지움
//    → `myStartMoney`(편집 모드면 194162) → 플레이어가 알려져 있으면 로컬 owner 에 SP 지정 → SP 표시 갱신 → 현재 로컬 owner 의 기술 제거 → `myTech` 기술 추가
//    → owner 1..8 중 (로컬이 아니거나 데모) 이면서 사제가 있으면 AI 생성 → 명령 Gump 갱신 → `randGeysers` 가 비영이면 랜덤 간헐천 → 카메라 초기화 → 시작 시각 기록·started = 1.
//    실제 효과가 실패하면 이후 상태/started 쓰기를 실행하지 않는다. 도중의 효과가 현재 owner/시각을 바꾸면 그 이후 원본 위치의 조회에 반영한다.
//  - refreshTellAt()(004c3100): 현재 tellAtIndex 번째 항목의 첫 글자 뒤를 atol 로 읽는다('@' 여부는 확인하지 않는다). 항목이 없으면 0.
//  - bindCurrentInputs(reads): 현재 로컬 owner/PlayerKnown/editor getter 셋을 모두 연결한다(부분 연결 거부). 연결하면 독립 입력(`globals`)의 값형 fallback 은 읽지 않는다.
// 기존 `RawMissionFrameGate` 상태(gate)의 start/demo/tellAt/started 를 직접 참조한다. 설정 조회·세계·UI 효과는 호출자가 연결한다(모두 필수). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E234: 미션 시작 이식으로 추가)

import { configParseLong } from '../assets/config.js';
import { configListItem } from './raw-mission-ai-settings.js';

// 기존 frameGate 와 중복되지 않는 원본 Tutorial/base Mission 필드(+54/+58/+a0/+c0/+c4)를 만든다. Start 는 tellAtIndex 를 초기화하지 않는다. (2026-11-10 WB04-E234)
export function createMissionStartState() {
    return { flags: 0, sequence: 0, tellAtIndex: 0, fieldC0: 0, fieldC4: 0 };
}

// Start 가 읽고 쓰는 현재 전역(독립 입력)을 만든다: localOwner(00540c70)·playerKnown(00595344)·editor(005c85a4)와 출력 denySalvage(00595078)·denyAscend(0059507c)·allowAnyCapture(005453e4)·ordinaryGeysers(005950ac).
// `shared` 에 같은 이름의 `{ value }` 상자(실제 owner 의 접근자)를 넣으면 그 칸은 값형 fallback 대신 장기 owner 의 현재 값을 읽고 쓴다. (2026-11-10 WB04-E234)
export function createMissionStartGlobals() {
    return { localOwner: 1, playerKnown: 0, editor: 0, denySalvage: 0, denyAscend: 0, allowAnyCapture: 0, ordinaryGeysers: 0, shared: {} };
}

// 선택된 실제 효과만 필수로 요구하며 빈 callback 을 성공으로 바꾸지 않는다. (2026-11-10 WB04-E234)
function need(fn, name) {
    if (typeof fn !== 'function') throw new TypeError(name);
    return fn;
}

// 출력 칸에 쓴다: shared 상자가 있으면 그 owner(실제 장기 저장소)에, 없으면 값형 fallback 에 쓴다. 폭 검사/감김은 owner 의 접근자가 맡는다. (2026-11-10 WB04-E234)
function writeField(globals, name, value) {
    if (globals.shared?.[name]) globals.shared[name].value = value;
    else globals[name] = value;
}

// 004c2b20 의 공통 시작 순서이다.
//  gate: createMissionFrameGateState() 상태, state: createMissionStartState(), globals: createMissionStartGlobals(),
//  hooks = { get(키)(현재 mission.<키> 조회), sectionNames(접두)(00440640), set(키, 값)(004404e0), gameSeconds() → double, setSp(owner, float)(0040edb0), refreshSp()(0043dad0), clearTechnology(owner)(004903b0),
//            addTechnology(owner, 문자열)(004915c0), hasPriest(owner) → boolean(004921f0), createAi(owner)(004c2d40), refreshCommandGump()(0042e440), randomGeysers()(00486440), resetCamera()(004340d0) }. (2026-11-10 WB04-E234)
export class RawMissionStart {
    // 같은 장기 상태와 현재 전역·효과이다. 실제 Session 의 현재 getter(`reads`)는 연결 뒤에만 쓴다.
    #gate;
    #state;
    #globals;
    #hooks;
    #reads = null;

    // 현재 설정 조회(get)가 있어야 생성한다. 모든 참조 대상은 처리기보다 오래 살아야 한다. (2026-11-10 WB04-E234)
    constructor(gate, state, globals, hooks = {}) {
        if (typeof hooks.get !== 'function') throw new TypeError('MissionStart 는 PATCH 현재 설정 조회가 필요합니다');
        this.#gate = gate;
        this.#state = state;
        this.#globals = globals;
        this.#hooks = hooks;
    }

    // 세 현재 getter 를 모두 연결한다. 값형 fallback 을 갱신하거나 getter 반환을 저장하지 않는다. 하나라도 없으면 이전 연결을 유지한 채 거부한다. (2026-11-10 WB04-E234)
    bindCurrentInputs(reads) {
        if (typeof reads?.localOwner !== 'function' || typeof reads.playerKnown !== 'function' || typeof reads.editor !== 'function') throw new TypeError('MissionStart 현재 owner/PlayerKnown/editor 조회 누락');
        this.#reads = reads;
    }

    // 현재 로컬 owner 는 SP/기술/AI 순회의 서로 다른 원본 읽기 위치마다 조회한다. (2026-11-10 WB04-E234)
    #localOwner() { return (this.#reads ? this.#reads.localOwner() : this.#globals.localOwner) >>> 0; }

    // 현재 PlayerKnown 이 0 이면 SP 쓰기와 해당 owner 조회를 모두 생략한다. (2026-11-10 WB04-E234)
    #playerKnown() { return (this.#reads ? this.#reads.playerKnown() : this.#globals.playerKnown) >>> 0; }

    // 설정 조회 뒤 현재 editor 를 읽어 원본의 고정 시작 금액 선택에 쓴다. (2026-11-10 WB04-E234)
    #editor() { return (this.#reads ? this.#reads.editor() : this.#globals.editor) >>> 0; }

    // 004c3100: 첫 항목 예약을 위해 기존 index 를 쓴다. 항목의 첫 글자 뒤를 atol 로 읽으며 '@' 여부는 추가로 검사하지 않는다. (2026-11-10 WB04-E234)
    refreshTellAt() {
        const value = configListItem(this.#hooks.get('tellAtList'), this.#state.tellAtIndex | 0);
        this.#gate.tellAtSeconds = value === '' ? 0 : configParseLong(value.slice(1));
    }

    // 004c2b20 전체 순서이다. 도중의 실제 효과가 현재 owner/시각을 변경하면 그 이후 원본 위치의 조회에 반영한다. (2026-11-10 WB04-E234)
    start() {
        const state = this.#state;
        const globals = this.#globals;
        const hooks = this.#hooks;
        const gate = this.#gate;
        state.fieldC0 = 0;
        state.fieldC4 = 0;
        writeField(globals, 'denySalvage', configParseLong(hooks.get('denySalvage')));
        writeField(globals, 'denyAscend', configParseLong(hooks.get('denyAscend')));
        writeField(globals, 'allowAnyCapture', configParseLong(hooks.get('allowAnyCapture')));
        writeField(globals, 'ordinaryGeysers', configParseLong(hooks.get('moreGeysers')) === 0 ? 1 : 0);
        const demoTimer = configParseLong(hooks.get('demoRestartTimer'));
        if (demoTimer !== 0) gate.demoEndSeconds = demoTimer + need(hooks.gameSeconds, 'MissionStart 실제 시계 누락')();
        const names = need(hooks.sectionNames, 'MissionStart 실제 섹션 목록 누락')('@');
        need(hooks.set, 'MissionStart 실제 설정 쓰기 누락')('tellAtList', names);
        this.refreshTellAt();
        state.sequence = (state.sequence + 1) >>> 0;
        state.flags = (state.flags & ~1) >>> 0;
        let money = Math.fround(configParseLong(hooks.get('myStartMoney')));
        if (this.#editor() !== 0) money = 194162;
        if (this.#playerKnown() !== 0) need(hooks.setSp, 'MissionStart 실제 SP 지정 누락')(this.#localOwner(), money);
        need(hooks.refreshSp, 'MissionStart 실제 SP 화면 갱신 누락')();
        need(hooks.clearTechnology, 'MissionStart 실제 기술 제거 누락')(this.#localOwner());
        const techOwner = this.#localOwner();
        const tech = hooks.get('myTech');
        need(hooks.addTechnology, 'MissionStart 실제 기술 추가 누락')(techOwner, tech);
        // 실제 1..8 사제 존재를 각 owner 위치에서 검사하며 aiName 유무로 참가자를 대신 정하지 않는다.
        for (let owner = 1; owner < 9; owner += 1) {
            if (owner !== this.#localOwner() || gate.demo !== 0) {
                if (need(hooks.hasPriest, 'MissionStart 실제 사제 조회 누락')(owner)) need(hooks.createAi, 'MissionStart 실제 AI 생성 누락')(owner);
            }
        }
        need(hooks.refreshCommandGump, 'MissionStart 실제 명령 Gump 갱신 누락')();
        if (configParseLong(hooks.get('randGeysers')) !== 0) need(hooks.randomGeysers, 'MissionStart 실제 랜덤 간헐천 생성 누락')();
        need(hooks.resetCamera, 'MissionStart 실제 카메라 초기화 누락')();
        gate.startSeconds = need(hooks.gameSeconds, 'MissionStart 실제 시계 누락')();
        gate.started = 1;
    }
}
