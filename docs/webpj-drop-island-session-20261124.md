# 섬 투하의 실제 받침·입자 대조 — WB04-E1316

2026-11-24, HJOW-Athlon. `cpppj/tests/CampaignSessionTests.cpp`의 `CampaignSession_ActualDropIslandSupportAndParticles`를 실제 웹 Session에서 재현했다. 비교 기준은 `225553df`의 Session·건설·삭제·파편 구현이다.

`campaign-drop-island-session.test.mjs`는 TheWarBegins를 로드하고 현재 spot의 하위 6비트로 빈 3×3 영역을 찾는다. 실제 noIsland 받침을 확정한 45바이트 패킷의 9개 SID에서 오른쪽 아래 좌표를 읽어 소유자를 전파한다. 이때 TYPE 157 표면 9개·94 섬 1개·95 종유석 1개가 있어야 한다. 같은 위치에 실제 서버 제네레이터를 놓고 삭제하면 이 11개 부품이 반납되고 실제 Host·표시 등록표에 파편 파생 프로세스 10개가 남는다. SP 보존·unitLost 통지·삭제 깊이 0도 C++와 같다. 전투/AI Kernel·자연 승패는 실행하지 않았다.

웹 외곽 Session에는 C++의 공개 `ConfirmSupport`·`Construction` 조회가 없어 같은 하위 몸체를 감싸는 `confirmSupport(x,y)`·`construction()`을 추가했다. 설치 전 확정과 반납 뒤 대여를 거부하며, noIsland 현재 TYPE·Player0·나머지 0 인자를 그대로 보낸다. 기존 앱의 생성·프레임 순서는 바꾸지 않는다. 검사 준비 장치에는 unitLost·건설 장치를 명시적으로 빌릴 수 있게 했다.

집중 Node 26.13.0 검사 1/1 통과(`webpj/extracted/webpj-e1316-focused.log`). 독립 자식 프로세스에서 섬 파편 `fall` 훅을 끄면 파편 0 대 기대 10으로 실패해 변형을 검출한다(`webpj/extracted/webpj-e1316-mutation.log`). 변형은 모듈 로드 훅에만 적용했고 제품 소스는 변형하지 않았다.

webpack 운영 번들 3,423,251바이트로 한도를 통과했다(`webpj/extracted/webpj-e1316-build.log`). 관련 Chromium Tutorial2 시작 상태와 메타데이터 2/2 통과(`webpj/extracted/webpj-e1316-browser.log`). 정기 Node 26 전체는 6,834건 중 6,833 통과·1 실패(483초, `webpj/extracted/webpj-e1316-node26.log`)였다. 유일한 실패는 E1317에서 닫은 시간 동기화를 여전히 partial로 고정한 `port-map.test.mjs`의 낡은 기대값이다. 판정 근거에 맞춰 port로 고친 뒤 해당 집중 검사 1/1 통과(`webpj/extracted/webpj-e1317-status-regression.log`). 이 교정 후 전체 재실행은 하지 않았으며 제품/번들·나머지 검사는 전체 실행에서 통과했다.

E197 캡처·E198 EvilRay·E201 Burning·이번 받침/입자까지 5-A 재대조는 완료다. 7단계의 ‘전용 시험이 없었던 시스템’도 같은 근거로 닫는다.
