# 앱 HUD 생성과 World 도킹 분리

- 단위: WB04-E1382, 2026-11-27, HJOW-Athlon.
- 근거: E1380 보존 초안과 E1378 공유 생산 메뉴. 기존 이식 기준선 `225553df`와 오라클은 유지한다.

실제 앱의 `_createCampaignProductionHud`는 Client의 표면·Player·SP·옵션으로 `ProductionControlContext`를 만들고 HUD·생산 패널·Chat을 생성한다. 객체 효과와 미니맵 입력은 사용 시점의 현재 World를 조회한다. 게임 메뉴의 SP 검산·Player 조회·모드와 경고 이름도 같은 Client 장기 저장소를 사용한다.

`_dockCampaignProductionHud`는 Ready가 된 같은 Session만 연결하고 최초 도킹·미니맵 연결·웹 게임 메뉴 버튼 설치를 수행한다. `_prepareCampaignProductionUi`는 두 단계를 순서대로 호출하는 기존 진입점이다. 이번 단위는 호출 시점을 이동하지 않는다. 실제 State7 부모와 Runtime 반납 연결은 후속 E1383에서 수행한다.

초안의 Form 호스트 중복 연결은 집중 검사에서 발견해 제거했다. 현재 입력 준비가 같은 호스트를 이미 연결하므로 도킹에서 다시 연결하지 않는다. Chat·Console 객체 및 `blinkAt`은 생성부터 도킹까지 보존하며 기한 보정이나 정렬 치환을 사용하지 않는다.

## 검증

Node26 집중 11/11 통과. 실제 앱 Tutorial1 로그인 검사에 Chat·Console 정체·점멸 기한 보존, 실제 도킹 완료, 중복 도킹 거부를 추가했다. 번들 재현 1/1·대응표 1,160행 오류0이다. 운영 번들은 3,464,851바이트다.

Chromium 12/12 통과(4.0분): Chat 10경로, 캠페인1-1 브리핑·시작·대기 L3, 실제 서비스 결선이다. 캠페인3화면0픽셀·덤프3곳허용밖0을 유지했다. 공격·포획·희생·승리는 실행하지 않았다. 원문 로그는 Git 제외 `webpj/extracted/webpj-e1382-{focused,identity,build,browser}.log`에 있다. 전체 회귀 주기는 E1380 뒤 완료1단위(1/4·1/8)다.
