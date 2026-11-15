// Tutorial 형식의 캠페인/데모와 Early1~6 공통 프레임·안내를 같은 Session/Start/UI 에 연결하는 client 소비자이다(cpppj `client/RawCampaignTutorialOneBindings.cpp/.h`, 10.78 전용).
// 사용:
//   const tutorial = new RawCampaignTutorialOneBindings(session, script, start, gateState, startState, { frame: { gameSeconds, paused, dialogsClear, knowledgeOpen, guideOpen, blockingGumpOpen, endDemo, pause, resume },
//       tell(섹션) → boolean, closeTransient(), selected()?, resetSelection()?, clearSelection()? });
//   tutorial.frame() 은 4c4770: 사건 0 으로 공통 프레임을 한 번 실행한다. tutorial.event(event) 는 Guide First → 4c33a0 의 단계/시간 안내 명령 → 현재 사건 순서로 소비한다.
//   tutorial.tellStage()(4c2a90: 현재 단계 문자열 → Tell → 팝업 Kill → 자기 정지) · tutorial.tellAt()(4c3470: 사제 생존 → 현재 목록 Tell → 인덱스 증가 → 다음 예약) · tutorial.gate() · tutorial.stageShown().
// 공통 프레임은 모든 Tutorial 형식에 적용하고 비데모 1~6 에서만 전용 Early 판정기를 만든다. 판정기는 gate 생성 뒤 같은 주소에 연결한다(gate 생성 중에는 callback 을 실행하지 않는다).
// session 계약(실제 `RawCampaignSession` 이식 전의 최소 덕타입, 4-D 구획 D 에서 같은 이름으로 구현한다): { checkMissionOutcome(), localOwner(), missionPriestDead(owner) → boolean,
//   makeTutorialOne(gate) → { handle(event) }, makeTutorialTwo(gate, hooks) → { handle(event) }, makeTutorialLater(gate, startState, hooks) → { handle(number, [event, x, y]) } }.
// start 는 `RawCampaignMissionStartBindings`(같은 Start 의 refreshTellAt 사용)이고, script 는 `MissionScript`(`tellAtList` 조회)이다. event 는 `{ code, x, y }`(x/y 는 정수 좌표 또는 SID·TYPE 비트)이다.
// 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E520: 4-D C 구획 RawCampaignTutorialOneBindings 이식)
import { configListItem } from './raw-mission-ai-settings.js';
import { dispatchMissionFirst, missionFirst, tellMissionStage } from './mission-guide-input.js';
import { RawMissionFrameGate } from './raw-mission-frame-gate.js';

// 보호 10.78 PE 의 542a40/542a3c 초기 DWORD 이다. 입력 modifier 를 제거한 뒤 비교한다. (2026-11-15 WB04-E520)
const NEXT_STAGE_COMMAND = 0x020000d3;
const NEXT_TELL_AT_COMMAND = 0x020000d2;
// 명령 비교에 쓰는 modifier 제거 마스크(하위 26비트)이다. (2026-11-15 WB04-E520)
const COMMAND_MASK = 0x03ffffff;

// 모든 Tutorial 형식 미션의 공통 프레임/입력과 선택된 Early1~6 판정에 실제 소비자만 묶는다. 같은 장기 Session/Script/Start/gate/state 를 빌린다. (2026-11-15 WB04-E520)
export class RawCampaignTutorialOneBindings {
    // 현재 객체/Player/장부/간헐천의 유일한 저장소, 현재 미션의 tellAtList/섹션 Script, 같은 Start, 단계/잠금/정지/시각의 단일 저장소와 같은 현재 tellAtIndex 의 Start 상태이다.
    #session;
    #script;
    #start;
    #state;
    #startState;
    // 실제 화면/시계/팝업의 외부 소비자와 독립 원본 입력으로 검증된 공통 gate 몸체, 전용 판정기 셋(비데모 1~6 중 하나만 채워진다)이다.
    #effects;
    #gate;
    #tutorialOne = null;
    #tutorialTwo = null;
    #tutorialLater = null;
    // 공통 gate 호출 동안만 빌리는 현재 사건(코드·x·y 의 DWORD 세 개)이고, TellStage 가 마지막에 기록하는 원본 +b4 결과 상자이다.
    #event = [0, 0, 0];
    #stageShown = { value: 0 };

    // 판정기를 만들기 전에 실제 안내/UI 연결을 확인한다. 생성만으로 프레임을 실행하지 않는다. (2026-11-15 WB04-E520)
    constructor(session, script, start, gateState, startState, effects) {
        if (typeof effects?.tell !== 'function' || typeof effects.closeTransient !== 'function') throw new TypeError('공통 Tutorial 형식 미션의 실제 안내/UI 연결 계약 오류');
        this.#session = session;
        this.#script = script;
        this.#start = start;
        this.#state = gateState;
        this.#startState = startState;
        this.#effects = effects;
        this.#gate = new RawMissionFrameGate(gateState, this.#hooks());
        // 원본 공통 프레임은 데모이거나 번호가 1..6 밖이면 전용 Early 판정기를 호출하지 않는다.
        if (gateState.demo || gateState.tutorialNumber < 1 || gateState.tutorialNumber > 6) return;
        const frame = effects.frame;
        if (gateState.tutorialNumber === 1) {
            this.#tutorialOne = session.makeTutorialOne(this.#gate);
        } else if (gateState.tutorialNumber === 2) {
            // 게임 조회/옵션 갱신은 같은 Session 이 연결하며 UI 경계만 빌린다.
            this.#tutorialTwo = session.makeTutorialTwo(this.#gate, {
                gameSeconds: frame.gameSeconds, selected: effects.selected, resetSelection: effects.resetSelection, clearSelection: effects.clearSelection,
                tell: effects.tell, closeTransientGumps: effects.closeTransient,
            });
        } else {
            // 실제 월드 조회는 Session 이 공급하며 UI/시계는 같은 현재 창을 빌린다.
            this.#tutorialLater = session.makeTutorialLater(this.#gate, startState, {
                gameSeconds: frame.gameSeconds, selected: effects.selected, resetSelection: effects.resetSelection, clearSelection: effects.clearSelection,
                tell: effects.tell, closeTransientGumps: effects.closeTransient,
            });
        }
    }

    // 전체 게임 판정은 같은 Session 에 맡기고 기존 프레임 몸체의 실행 순서를 변경하지 않는다. UI/시계의 현재 조회와 효과는 그대로 보존한다. (2026-11-15 WB04-E520)
    #hooks() {
        return {
            ...this.#effects.frame,
            checkOutcome: () => { this.#session.checkMissionOutcome(); },
            tellStage: () => { this.tellStage(); },
            tellAt: () => { this.tellAt(); },
            tutorial: (number, event) => {
                if (number === 1 && this.#tutorialOne) return this.#tutorialOne.handle(event);
                if (number === 2 && this.#tutorialTwo) return this.#tutorialTwo.handle(event);
                if (number >= 3 && number <= 6 && this.#tutorialLater) {
                    // gate 가 전달한 현재 명령과 같은 호출의 두 인자를 함께 사용한다.
                    const current = [...this.#event];
                    current[0] = event;
                    return this.#tutorialLater.handle(number, current);
                }
                throw new Error('현재 Early1~6 전용 판정 연결 누락');
            },
        };
    }

    // 원본 화면 프레임은 입력이 없어도 공통 안내/단계 판정을 한 번 호출한다. (2026-11-15 WB04-E520)
    frame() {
        this.#run({ code: 0, x: 0, y: 0 });
    }

    // 중첩 Tell/조회가 입력을 다시 분배하거나 예외를 내도 이전 호출의 인자를 남기지 않는다. (2026-11-15 WB04-E520)
    #run(event) {
        const saved = this.#event;
        this.#event = [event.code >>> 0, event.x >>> 0, event.y >>> 0];
        try {
            // 실제 공통 gate 와 선택된 원본 판정기의 반환값이다.
            return this.#gate.runFrame(event.code >>> 0);
        } finally {
            this.#event = saved;
        }
    }

    // 원본 특수 명령은 공통 프레임을 추가 실행하지 않고 즉시 1 을 반환한다. (2026-11-15 WB04-E520)
    event(event) {
        if (dispatchMissionFirst(event, () => { missionFirst(this.#startState, () => { this.tellStage(); }); })) return 1;
        // 원본 입력의 modifier 만 제거한다.
        const code = (event.code & COMMAND_MASK) >>> 0;
        if (code === NEXT_STAGE_COMMAND) {
            this.#gate.advance();
            return 1;
        }
        if (code === NEXT_TELL_AT_COMMAND) {
            this.tellAt();
            return 1;
        }
        return this.#run(event);
    }

    // Tell 이 없는 단계도 마지막 결과 0 을 기록한다. 창이 생긴 경우에만 원본 후속 정리(팝업 Kill → 자기 정지)를 실행한다. (2026-11-15 WB04-E520)
    tellStage() {
        tellMissionStage(this.#state, this.#stageShown, {
            tell: (section) => (this.#effects.tell(section) ? 1 : 0),
            closeTransient: this.#effects.closeTransient,
            pause: () => { this.#gate.pauseForDialog(); },
        });
    }

    // 원본처럼 사제가 죽었어도 목록 인덱스 증가와 다음 예약은 실행한다. (2026-11-15 WB04-E520)
    tellAt() {
        if (!this.#session.missionPriestDead(this.#session.localOwner())) {
            // 현재 목록/현재 인덱스이다.
            const item = configListItem(this.#script.get('tellAtList') ?? '', this.#startState.tellAtIndex | 0);
            if (this.#effects.tell(item)) {
                this.#effects.closeTransient();
                this.#gate.pauseForDialog();
            }
        }
        this.#startState.tellAtIndex = (this.#startState.tellAtIndex + 1) >>> 0;
        this.#start.startBody().refreshTellAt();
    }

    // UI 와 공통 프레임이 같은 정지 소유권을 쓰도록 같은 gate 를 돌려준다. (2026-11-15 WB04-E520)
    gate() { return this.#gate; }

    // 원본 +b4 의 마지막 TellStage 반환값을 별도 진행 결과로 바꾸지 않고 그대로 보고한다. (2026-11-15 WB04-E520)
    stageShown() { return this.#stageShown.value >>> 0; }
}
