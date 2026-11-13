// 파일 팔레트를 읽은 뒤 원본 004a4923..004a49a2 의 지우기(clear) 관문과 하위 호출 순서를 옮긴 모듈이다(cpppj `client/ScreenPaletteApply.h`, 10.78 전용).
//  - applyScreenPaletteClear(clear, flags, loaded, hooks): `loaded.value` 는 파일을 읽은 뒤 항상 1 이다. clear 가 0 이 아니고 현재 화면 flags 도 0 이 아닐 때만 필수 하위 장치를 원문 순서로 부른다 —
//    lock(004a14e0, 중첩 깊이와 같은 현재 draw surface) → fillBlack(004a2060, lock 뒤의 현재 크기 (0,0,width,height)를 색 0 으로) → unlock(깊이 감소, 0 이면 표면 Unlock 과 buffer/pitch 0 은 실제 Screen 소유) →
//    waitVerticalBlank(004a1fb0, GDI 는 비트 1 관문에서 자연 반환·DirectDraw 는 필수) → presentPreservingClip(004a46d0, viewport Update 전후 기존 clip 보존) → flip(004a16d0, flipping 0 은 자연 반환·실제 flip 은 필수).
//  - 선택하지 않은 경로(clear 0 또는 flags 0)는 하위 장치를 하나도 요구하지 않는다. 도달한 장치가 없으면 빈 성공으로 넘기지 않고 부분 효과와 함께 오류를 낸다.
// hooks = { lock(), fillBlack(), unlock(), waitVerticalBlank(), presentPreservingClip(), flip() }. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E376: 5-A 팔레트 계열 이식으로 추가, 기준선 뒤 cpppj 변경 없음)

// 도달한 장치만 요구하며 없으면 빈 성공 대신 오류를 낸다(원문 std::function 미연결 호출과 같은 시점). (2026-11-13 WB04-E376)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('화면 팔레트 적용의 필수 하위 장치 미연결');
    return hook;
}

// 파일 팔레트를 읽은 뒤의 clear 관문이다. loaded 는 { value } 상자(원문 DWORD 전역), flags 는 현재 화면 flags 숫자이다. 사용: applyScreenPaletteClear(1, screen.flags, loadedBox, hooks). (2026-11-13 WB04-E376)
export function applyScreenPaletteClear(clear, flags, loaded, hooks) {
    loaded.value = 1;
    if (clear !== 0 && flags !== 0) {
        need(hooks.lock)();
        need(hooks.fillBlack)();
        need(hooks.unlock)();
        need(hooks.waitVerticalBlank)();
        need(hooks.presentPreservingClip)();
        need(hooks.flip)();
    }
}
