// 커서의 현재 Gump 조회·수신·부모와 팝업 종료 장치를 같은 컨트롤에 연결한다.
// 사용: bindCampaignCursorGumps(effects, controls). 두 객체는 커서 효과보다 오래 살아야 한다.

// 원본 Gump의 조각 허용 조회와 조각 수신 명령이다.
const CURSOR_GUI_FLAGS = 0x0200005f;
const CURSOR_GUI_RECEIVE = 0x0200005e;

// 동일 컨트롤의 실제 트리와 가상 명령에 기존 커서 효과를 연결한다.
export function bindCampaignCursorGumps(effects, controls) {
    // 현재 화면점의 최상위 Gump 토큰을 조회한다.
    effects.placement.gumpAt = (x, y) => controls.pickGump({ x, y });
    // 가상 수신 반환과 재귀 분배 토큰을 구분해 원본 인자를 전달한다.
    effects.placement.accept = (token, event, first, second, hidden) => controls.dispatchGump(token, event, first, second, hidden !== 0);
    // 배치와 내려놓기가 같은 현재 Gump 부모 SID를 조회한다.
    const parent = (token) => controls.gumpParentSid(token);
    effects.placement.parent = parent; effects.drop.guiParent = parent;
    // Gump의 원본 조각 허용 플래그를 조회한다.
    effects.drop.guiFlags = (token) => controls.handleGump(token, CURSOR_GUI_FLAGS, 0, 0);
    // 선택된 조각의 원본 Gump 수신 명령을 실행한다.
    effects.drop.guiReceive = (token, form) => { controls.handleGump(token, CURSOR_GUI_RECEIVE, form, 0); };
    // 기존 종료 훅이 없을 때만 팝업의 지연 Kill을 요청한다.
    if (!effects.lifecycle.closePopups) effects.lifecycle.closePopups = () => { controls.requestClosePopups(); };
}
