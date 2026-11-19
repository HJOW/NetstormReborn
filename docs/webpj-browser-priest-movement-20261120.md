# 브라우저 사제 이동 검증 (WB04-E992)

실제 Chromium에서 Campaign → Early Missions → Tutorial1을 시작하고 첫 안내의 전진 버튼을 모두 눌러 시계를 재개했다. 사제 그림을 클릭해 같은 Session의 사제를 선택한 뒤 섬 위 목적지를 클릭해 로컬 전송의 이동 요청 코드 `0x40`이 추가되고 실제 SID 풀의 사제 좌표가 바뀌는 것을 확인했다. 처리되지 않은 브라우저 오류는 없었다.

`npx playwright test e2e/tutorial-menu-start.spec.js --project=chromium --workers=1` 1건이 통과했다. 이 검증은 비활성 음향, 한 목적지와 초기 튜토리얼만 사용했으며 전체 경로 탐색·HUD·건설·승리나 나머지 미션 완료를 보증하지 않는다.

E982 고정 Node 26과 Node 22 전체 검사는 각각 6,556건 통과·실패와 건너뜀 0이다. E986 고정 Node 26 전체 검사가 실행 중이며 E990 고정 Node 22 검사는 그 뒤 순차 실행한다.
