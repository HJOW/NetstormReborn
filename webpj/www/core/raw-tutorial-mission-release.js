// Tutorial 소멸(004c3210, 예외 정리 004fb940)의 직접 전역 쓰기와 실제 AI/기본 Mission 반납 순서를 옮긴 모듈이다(cpppj `o/RawTutorialMissionRelease.cpp/.h`, 10.78 전용).
// 순서: 허용 표 갱신(setAllowAll(1), 004c23c0) → 여섯 전역을 이 순서대로 0 으로 쓰기(회수 금지 00595078·승급 금지 0059507c·미션 번호 005ca8e0·Early 분배 번호 005ca8e4·단계 005ca8e8·단계 변경 표식 005ca8ec)
//   → owner 1..8 AI 전체 반납(removeAllBrains, 00413b20) → 기본 Mission 해제(releaseBaseMission, 00483210). 기본 Mission 은 정상 꼬리에서도, 앞선 단계가 예외로 끝나는 unwind 에서도 정확히 한 번 수행한다(try/finally).
// state 는 여섯 전역을 읽고 쓰는 속성(denySalvage·denyAscend·missionNumber·tutorialNumber·stage·stageDirty)을 가진 객체로, 사본이 아닌 현재 값에 대한 대여이다. 본문 실패의 부분 쓰기를 보존하며 시계·UI·World·음악의 수명은 추가하지 않는다.
// 기본 Mission 해제가 정상 꼬리에서 실패해도 재시도하지 않고 한 번만 전파한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E15: Tutorial 소멸 이식으로 추가)

// 도달한 원문 하위 장치만 확인한다. 미연결 효과는 해당 호출 지점에서 중단하며 앞선 쓰기를 되돌리지 않는다. (2026-11-08 WB04-E15)
function need(device) {
    if (typeof device !== 'function') throw new Error('Tutorial 소멸의 실제 하위 효과 미연결');
    return device;
}

// state = { denySalvage, denyAscend, missionNumber, tutorialNumber, stage, stageDirty }, hooks = { setAllowAll(1), removeAllBrains(), releaseBaseMission() }. (2026-11-08 WB04-E15)
export function releaseTutorialMission(state, hooks) {
    try {
        need(hooks.setAllowAll)(1);
        state.denySalvage = 0;
        state.denyAscend = 0;
        state.missionNumber = 0;
        state.tutorialNumber = 0;
        state.stage = 0;
        state.stageDirty = 0;
        need(hooks.removeAllBrains)();
    } finally {
        // 원문 state0 의 unwind 와 정상 꼬리가 같은 기본 Mission 을 정확히 한 번 반납하게 한다.
        need(hooks.releaseBaseMission)();
    }
}
