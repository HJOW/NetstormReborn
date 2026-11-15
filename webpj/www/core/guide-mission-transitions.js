// 원본 Guide 의 Begin/Fork/Restart/End 교체 순서와 제어 사건을 현재 미션 수명에 연결하는 모듈이다(cpppj `client/GuideMissionTransitions.cpp/.h`, 10.78 전용).
// 사용:
//   const hooks = { current() → boolean, finish(), release(), create(이름), get(키) → 문자열, mode(값), demo(값) };
//   beginGuideMission(name, hooks)  (00463b70) 현재가 있으면 finish/release 한 번 뒤 이름을 NUL 앞까지 복사해 공장(create)을 호출한다.
//   closeGuideMission(hooks)        (00463b20) 현재가 있을 때만 release 한다(가상 +1c 종료는 호출하지 않으며 해제 재진입도 한 번만 처리).
//   forkGuideMission(parameter, hooks) (00463da0) `fork%d`(signed) 값을 먼저 복사한 뒤 finish/release 하고 Begin 으로 넘긴다.
//   restartGuideMission(request, hooks) (00463cf0) 캡처한 이름/정규화 필드(`captureMissionReload`)로 release 부터 시작하고 공장 뒤 원래 mode/demo 를 복원한다.
//   endGuideMission(hooks)          (00463e40 End) finish 와 release 를 한 번씩 수행하며 공장은 호출하지 않는다.
//   dispatchGuideMissionCommand(event, { begin(세 번째 DWORD), fork(DWORD), restart(), end(), invalidBegin(DWORD) }) → boolean: Begin/Reload/Fork/End 명령만 상위 플래그를 지운 코드로 분배하고 그 밖은 false 이다.
//     Begin 의 세 번째 값(event.y, signed)이 100 이하이면 원본의 잘못된 문자열 주소 진단(invalidBegin)을 먼저 요청하고 반환 뒤 같은 인자로 계속한다.
// 큰 수명 효과 뒤의 현재 재조회·문자열 소유 복사·마지막 필드 복원을 원본 순서대로 실행한다. 선택되지 않은 효과는 빈 성공으로 대신하지 않고 도달한 위치에서 거부한다.
// 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E523: 4-D 구획 GuideMissionTransitions 이식)
import { MISSION_BEGIN_COMMAND, MISSION_END_COMMAND, MISSION_FORK_COMMAND, MISSION_RELOAD_COMMAND } from './mission-guide-input.js';

// 명령 사건 코드에서 상위 플래그를 뺀 하위 26비트 마스크이다. (2026-11-15 WB04-E523)
const COMMAND_BITS = 0x03ffffff;

// 실제 도달한 효과만 필수로 요구한다. 선택되지 않은 효과를 빈 성공으로 채우지 않는다. (2026-11-15 WB04-E523)
function transitionEffect(effect) {
    if (typeof effect !== 'function') throw new Error('Guide 교체의 실제 수명 효과 연결 누락');
    return effect;
}

// Begin 은 기존 해제 중 새 현재가 생겨도 재확인 루프 없이 한 번의 공장으로 덮어쓴다. 이름은 해제 뒤 NUL 까지만 소유 복사한다(고정 버퍼를 빌린 경우 해제 중 바뀐 길이도 이 위치에서 읽는다). (2026-11-15 WB04-E523)
export function beginGuideMission(name, hooks) {
    if (transitionEffect(hooks.current)()) {
        transitionEffect(hooks.finish)();
        transitionEffect(hooks.release)();
    }
    const text = String(name);
    const nul = text.indexOf('\0');
    transitionEffect(hooks.create)(nul < 0 ? text : text.slice(0, nul));
}

// Close 는 현재가 없으면 해제를 요구하지 않고, 있으면 finish 없이 같은 현재를 한 번 반납한다. (2026-11-15 WB04-E523)
export function closeGuideMission(hooks) {
    if (transitionEffect(hooks.current)()) transitionEffect(hooks.release)();
}

// Fork 의 설정 결과는 첫 해제 전에 반드시 소유 복사한다. 키의 `%d` 는 signed DWORD 서식이다. (2026-11-15 WB04-E523)
export function forkGuideMission(parameter, hooks) {
    const key = `fork${parameter | 0}`;
    const target = transitionEffect(hooks.get)(key);
    transitionEffect(hooks.finish)();
    transitionEffect(hooks.release)();
    beginGuideMission(target, hooks);
}

// Restart 의 첫 해제에는 finish 가 없다. 재진입 현재만 Begin 이 끝내며 mode 는 공장 뒤 복원하고 demo 는 새 현재가 있을 때만 복원한다. (2026-11-15 WB04-E523)
export function restartGuideMission(request, hooks) {
    transitionEffect(hooks.release)();
    beginGuideMission(request.name, hooks);
    transitionEffect(hooks.mode)(request.mode);
    if (transitionEffect(hooks.current)()) transitionEffect(hooks.demo)(request.demo);
}

// End 는 해제 재진입 현재를 추가로 제거하지 않는 원본 한 번의 동작이다. (2026-11-15 WB04-E523)
export function endGuideMission(hooks) {
    transitionEffect(hooks.finish)();
    transitionEffect(hooks.release)();
}

// 잘못된 Begin 주소의 진단도 원본처럼 별도 효과이며 반환 후 같은 인자로 계속한다. 세 번째 DWORD 는 x 와 무관한 event.y 의 비트이다. (2026-11-15 WB04-E523)
export function dispatchGuideMissionCommand(event, hooks) {
    const command = (event.code & COMMAND_BITS) >>> 0;
    const parameter = event.y >>> 0;
    if (command === MISSION_BEGIN_COMMAND) {
        if ((event.y | 0) <= 100) transitionEffect(hooks.invalidBegin)(parameter);
        transitionEffect(hooks.begin)(parameter);
        return true;
    }
    if (command === MISSION_RELOAD_COMMAND) {
        transitionEffect(hooks.restart)();
        return true;
    }
    if (command === MISSION_FORK_COMMAND) {
        transitionEffect(hooks.fork)(parameter);
        return true;
    }
    if (command === MISSION_END_COMMAND) {
        transitionEffect(hooks.end)();
        return true;
    }
    return false;
}
