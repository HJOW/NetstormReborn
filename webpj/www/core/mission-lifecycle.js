// 원본 Guide 의 Tutorial 가상 단계 0..6 을 같은 현재 미션 상태/세계 소비자에 분배하는 모듈이다(cpppj `client/MissionLifecycle.cpp/.h`, 10.78).
//  - runTutorialMissionStage(state, sequence, hooks): `RawMissionGuide` 가 현재 단계 번호로 부르는 가상 표의 0..6 이다. 큰 초기화/Start/프레임은 실제 몸체(hooks)에 위임하고 단계 0/2/5/빈 6 을 직접 옮겼다.
//      0 → `missionFirst(state, hooks.showStage)`(순번 0 이면 flags 값 1 만 요청, 아니면 현재 안내 재표시) · 1 → `hooks.initialize()`(004c3290 상당의 설정/월드 설치와 flags 에 1 쓰기)
//      · 2 → `hooks.tellGlobal('init', 1)`(004cf970) 뒤 flags |= 1 · 3 → `hooks.start()`(실제 RawMissionStart; Start 가 같은 순번을 증가시킴) · 4 → `hooks.frame()`(공통 프레임)
//      · 5 → `hooks.tellMission(flags 값 4 비트가 있으면 'Success' 아니면 'Failure')`(현재 미션의 결과 섹션 Tell)가 비영을 돌려주면 `hooks.closeTransient()` 와 `hooks.pause()`(자기 정지) 뒤, 항상 flags |= 1
//      · 6 → 원본 004437f0 의 실제 무동작 · 그 밖의 번호는 `RangeError`. 선택된 단계가 요구하는 효과가 없으면 도달한 위치에서 `Error` 로 거부하며 빈 성공으로 바꾸지 않는다.
//  - pauseMissionWorldEntry(current, pause): 00463a40 — 현재 Guide 미션 조회를 한 번 하고, 있을 때에만 가상+28 의 Pause 를 부른다(Start 가상+10 과 구별). 조회가 없으면 거부하고, 있는데 pause 가 없으면 거부한다.
//  hooks = { initialize(), tellGlobal(이름, 1), start(), frame(), showStage(), tellMission(이름) → 정수, closeTransient(), pause() }. 원본 순번을 별도로 복제하지 않고 `state`(createMissionStartState() 모양)를 직접 쓴다.
//  이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E341: 4-D B 구획 MissionLifecycle 이식)
import { missionFirst } from './mission-guide-input.js';

// 선택된 단계가 실제로 요구하는 효과의 누락은 도달한 위치에서 드러낸다. (2026-11-12 WB04-E341)
function need(effect, message) {
    if (typeof effect !== 'function') throw new Error(message);
    return effect;
}

// 가상 표 0..6 의 분배이다. flags 쓰기는 부호 없는 32비트로 유지한다. (2026-11-12 WB04-E341)
export function runTutorialMissionStage(state, sequence, hooks) {
    switch (sequence) {
    case 0:
        missionFirst(state, hooks?.showStage);
        return;
    case 1:
        need(hooks?.initialize, 'Guide 초기화의 실제 월드 설치 연결 누락')();
        return;
    case 2:
        need(hooks?.tellGlobal, 'Guide init Tell 연결 누락')('init', 1);
        state.flags = (state.flags | 1) >>> 0;
        return;
    case 3:
        need(hooks?.start, 'Guide 단계3 실제 Start 연결 누락')();
        return;
    case 4:
        need(hooks?.frame, 'Guide 단계4 실제 공통 프레임 연결 누락')();
        return;
    case 5: {
        // 원본 BYTE 의 성공 비트(값 4)만 읽는다.
        const shown = need(hooks?.tellMission, 'Guide 종료 안내의 현재 미션 조회 연결 누락')((state.flags & 4) ? 'Success' : 'Failure');
        if (shown) {
            // 두 효과를 모두 확인한 뒤 팝업 닫기 → 자기 정지 순서로 부른다.
            const closeTransient = need(hooks.closeTransient, 'Guide 종료 안내의 실제 팝업/정지 연결 누락');
            const pause = need(hooks.pause, 'Guide 종료 안내의 실제 팝업/정지 연결 누락');
            closeTransient();
            pause();
        }
        state.flags = (state.flags | 1) >>> 0;
        return;
    }
    case 6:
        return;
    default:
        throw new RangeError('Guide 가상 단계0..6 범위 위반');
    }
}

// 00463a40: 원본의 현재 미션 포인터 조회는 한 번이며 없음이면 가상 함수를 읽지 않는다. (2026-11-12 WB04-E341)
export function pauseMissionWorldEntry(current, pause) {
    if (typeof current !== 'function') throw new Error('월드 진입의 현재 Guide 조회 누락');
    if (current()) need(pause, '월드 진입의 실제 미션 Pause 연결 누락')();
}
