// 원본 10.78 의 State 39/40/41/42(인트로 준비·영상 대기·복귀·제목 스플래시)를 현재 Native 블록과 실제 영상 장치 경계로 실행하는 모듈이다(cpppj `client/NativeIntroState.cpp/.h`, 원문 004b6e62..004b6fa9).
// 사용: const result = installNativeIntroState({ state: { old, phase, immediate, current, next }, fort, server, boss, pendingName }, hooks) → LocalMissionParentResult.Commit
//   globals.state 는 `RawLocalMissionState` 의 다섯 36DWORD 블록(current+8c 의 DWORD 35 번째 칸이 인트로 준비 여부), fort/server/boss 는 같은 실제 모드(00594fb8/00540bc0/00540bc4)를 읽고 쓰는 `{ value }` 상자(또는 속성 접근자),
//   pendingName 은 0055b494 가 가리키는 이름 물리(Uint8Array)이며 State 41 은 첫 BYTE 만 지운다. 별도 상태 사본을 만들지 않는다.
//   hooks = { cleanup(), hideChatInput(), hideChat(), core(), renderer(), hud(), moviePath() → 문자열, markIntro(), openMovie(경로), movieActive() → boolean, request(상태 번호), titleSplash(), restoreTitlePalette(), titleCursor(보임) }.
//   - State 42: 커서 숨김 → 제목 스플래시 → State 41 예약 → immediate+35 0 → 제목 팔레트 복구 → 커서 표시. 예외 앞서 숨긴 커서/비트맵 효과는 되돌리지 않는다(원문 그대로).
//   - State 39: current[35] 가 있으면 cleanup → 채팅 입력 숨김 → 채팅 숨김 → fort/server/boss 모두 참 → Core → Renderer → HUD → 영상 경로 조회, 이름이 있으면 인트로 표식 → 영상 열기(성공 여부를 쓰지 않음) → State 40 예약, 없으면 State 41 예약.
//   - State 40: 영상이 있으면 next[0] = 40 으로 유지하고 없으면 State 41 예약. State 41: pending 이름 첫 BYTE 를 지우고 State 3 예약.
//   그 밖의 상태는 장치 호출 전에 거부한다. 선택된 효과만 원문 위치에서 요구하며 공통 commit 은 `RawLocalMissionState` 에 맡긴다.
// 원본 관측: `native-intro-state-x86.tsv` 17행(다섯 144BYTE 전체·모드·pending 이름·효과 순서)을 tests/native-intro-state.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E532: 5-C 메인 메뉴·타이틀 상태 NativeIntroState 이식)
import { LocalMissionParentResult } from './raw-local-mission-state.js';

// 도달하지 않은 영상/준비 장치를 빈 성공으로 채우지 않는다. 현재 선택된 하위가 없으면 앞선 부분 효과를 보존하고 명시 중단한다. (2026-11-15 WB04-E532)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Native 인트로의 선택된 실제 장치 미연결');
    return hook;
}

// 원문 004b6e62..004b6fa9 의 분기/현재 전역 쓰기/하위 호출 순서를 보존한다. State39 준비 뒤의 원문 bool 쓰기는 cleanup 이 끝난 현재 같은 물리에 적용한다. (2026-11-15 WB04-E532)
export function installNativeIntroState(globals, hooks) {
    const { state, fort, server, boss, pendingName } = globals;
    switch (state.current[0]) {
    case 42:
        need(hooks.titleCursor)(false);
        need(hooks.titleSplash)();
        need(hooks.request)(41);
        state.immediate[35] = 0;
        need(hooks.restoreTitlePalette)();
        need(hooks.titleCursor)(true);
        break;
    case 39:
        if (state.current[35]) {
            need(hooks.cleanup)();
            need(hooks.hideChatInput)();
            need(hooks.hideChat)();
            fort.value = true;
            server.value = true;
            boss.value = true;
            need(hooks.core)();
            need(hooks.renderer)();
            need(hooks.hud)();
            // 원문의 초기 NUL 버퍼에 ConfigPath 가 작성한 실제 이름이다.
            const path = need(hooks.moviePath)();
            if (path !== '') {
                need(hooks.markIntro)();
                need(hooks.openMovie)(path);
                need(hooks.request)(40);
                break;
            }
        }
        need(hooks.request)(41);
        break;
    case 40:
        if (need(hooks.movieActive)()) state.next[0] = 40;
        else need(hooks.request)(41);
        break;
    case 41:
        if (!pendingName || pendingName.length === 0) throw new RangeError('Native 인트로 복귀의 pending 이름 물리 없음');
        pendingName[0] = 0;
        need(hooks.request)(3);
        break;
    default:
        throw new Error('Native 인트로 State39/40/41/42 계약');
    }
    return LocalMissionParentResult.Commit;
}
