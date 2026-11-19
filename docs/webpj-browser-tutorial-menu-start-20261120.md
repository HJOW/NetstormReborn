# 브라우저 첫 Tutorial 시작 검증 (WB04-E986)

독립 Chromium 저장소에서 시작 안내를 마친 설정과 비활성 음향을 사용해 실제 Canvas를 클릭했다. Campaign → Early Missions → Tutorial1의 화면 좌표를 실제 마우스로 선택하고, 같은 앱의 로컬 로그인 완료·Guide Start·월드 Renderer 출력·첫 안내창과 게임 시계 정지를 확인했다. 1280×720 브라우저에서 논리 화면은 1366×768이었으며 처리되지 않은 페이지 오류가 없었다.

`npx playwright test e2e/tutorial-menu-start.spec.js --project=chromium --workers=1` 1건이 통과했다. 테스트는 비활성 음향 경로를 사용하고 캔버스에 색 번호 픽셀이 출력됨을 확인한다. 실제 음향, HUD·게임 조작, 나머지 미션과 플레이 완료 검증을 대신하지 않는다. E982 고정 Node 22 전체 검사는 별도 검증 트리에서 진행 중이다.
