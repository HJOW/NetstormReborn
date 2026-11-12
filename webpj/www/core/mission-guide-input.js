// 원본 활성 미션의 F8 예약·Guide First 사건과 현재 안내 재표시를 옮기는 모듈의 **첫 부분**이다(cpppj `client/MissionGuideInput.cpp/.h`, 10.78).
//  - missionFirst(state, tellStage)(004c2a70): 순번이 0 이면 Tell 을 부르지 않고 flags 값 1(비트 0)만 요청한다(다른 flags 와 상태 필드는 보존). 그 밖에는 현재 안내를 다시 표시하는 `tellStage()` 를 부르며 연결이 없으면 거부한다.
//    MissionLifecycle 단계 0 이 이 함수를 쓰므로 먼저 옮겼다(E341).
//  - 아직 옮기지 않은 부분(PORT_MAP partial): 미션 제어 명령 코드 일곱 개 상수와 `missionControlScriptCode`·`queueMissionControlAction`(ScriptCommand), `dispatchMissionAbort`·`captureMissionReload`·`applyMissionDemo`·
//    `tellMissionStage`·`dispatchMissionFirst`·`queueMissionFirstKey`(입력 사건/큐). 이어서 같은 파일에 더한다(4-D B 구획 MissionGuideInput 단위).
//  state 는 `createMissionStartState()` 모양(`flags`/`sequence`/…)이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E341: MissionLifecycle 의 선행으로 일부 이식)

// 004c2a70: 순번 0 에서는 flags 값 1(비트 0)만 요청하고 그 외에는 현재 안내를 다시 표시한다. (2026-11-12 WB04-E341)
export function missionFirst(state, tellStage) {
    if (state.sequence === 0) {
        state.flags = (state.flags | 1) >>> 0;
        return;
    }
    if (typeof tellStage !== 'function') throw new Error('현재 미션 First의 실제 안내 연결 누락');
    tellStage();
}
