# 스톰 파워 표시의 실제 우클릭 — WB04-E1324

2026-11-24, HJOW-Athlon. 원본10.78의 Tutorial1 안내 창을 MORE/OK로 닫고 숫자 칸 `(16,12)`·`(40,12)`·`(64,12)`을 실제 우클릭했다. 모두 `Storm Power Available → About` 팝업이 열렸다. `(40,180)` 생산 창의 `Production Window → About`도 양성 대조군으로 확인했다. E1247의 ‘원본도 열리지 않을 가능성’은 실제 실행으로 기각했다.

analyzeManager 세션 `20261124T024452986Z-21e51050b597`은 보호 원본을 별도 작업 사본으로 실행했다. 원본 EXE SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. 자동 녹화는 명시적으로 껐으며 음향 검증은 아니다. 시작/입력/PNG 기록은 `extracted/webpj-e1324-native/`와 해당 analyzeManager 세션에 있다. 검사 뒤 end_session 성공과 phase=ended를 확인하고 드라이버를 종료했다. 원본/CD 파일은 변경하지 않았다.

같은 호스트의 마지막 정밀 디컴파일 `extracted/refined/originals/Netstorm.c`를 확인했다. `00464ed0`은 비어 있지 않은 LOOK 문구를 +78에 넣고 flags의1000을 켜며 빈 값은 문구를 지우고1000을 끈다. `00464f80`은 비어 있지 않은 HELP 주제를 +7c에 넣고200을 켜며 빈 값은 포인터만 지우고200은 유지한다. 원본 `00464dd0` 필터는 HELP200이 있어야 버튼 사건의 좌표를 검사한다. 웹 `_setNodeHelp`는 문자열만 저장해 이 두 플래그 쓰기를 빠뜨렸다. 문자열 소유권/교체 정책은 기존 앱 정책을 유지하고 원본의 플래그 및 빈 값 처리를 복원했다.

실제 원본 화면 두 장과 관찰 JSON을 `tests/fixtures/original-budget-help/`에 보존했다. e2e는 이 관찰의 실제 클릭 좌표·주제와 About 항목을 사용해 웹 입력으로 팝업이 열리는지 확인한다. 화면 자체의 전 픽셀 일치를 주장하는 검사가 아니며 PNG 크기/해시는 자료 추적용이다. 스타일 메뉴의 고정 label 칸은 비어 있으므로 실제 표시 조각 체인의 문자열을 읽는다. 첫 시험의 `moneyGump` 기대는 실제 명령 주제인 `moneyGumpHelp`로 교정했다.

C++ `RawMenuControls::SetNodeHelp`도 같은 플래그 쓰기가 없다. 이는 원본보다 빠진 공통 동작이며 C++ 후속 수정 후보로 기록한다. 이 단위는 웹 입력을 수정했다. 예산 표시 재부착 `storm-power-refresh`의 앱 연결은 별도 잔여다.

Chromium 보완 후 우클릭1/1(39.9초)·기존 고정 시각4/4 통과다. 처음에는 명령 주제의 Help 접미사와 스타일 메뉴의 빈 label 칸을 잘못 기대해 실패했으며, 실제 명령 인자/표시 조각을 읽도록 검사만 교정했다. 실제 빈 HELP 뒤200 유지·빈 LOOK 뒤1000 해제·다시 설정도 확인했다. 번들은3,427,877바이트로 한도 통과. 정기 Node22.23.5 전체6,845/6,845 통과(760.8초), 실패·취소·미실행0이다. manifest/번들 신선도도 전체에 포함돼 통과했고 새3항목 포함966항목이다. 로그는 extracted/webpj-e1324-{build,browser,budget-verified,node22,manifest}.log다. 원본 EXE 해시 재확인도 실행 전과 같다. 회귀 주기는 Node26 E1321 전체 뒤3/4, Node22 E1324 전체 뒤0/8이다.
