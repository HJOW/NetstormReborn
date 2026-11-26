# 일본어 캠페인2 2-4~2-5·데모와 기본 AI 이름 — 단위1345

2026-11-26, HJOW-Athlon. 마지막 디컴파일 PC도 HJOW-Athlon이며 디컴파일 변경은 없다.

`guardmyback`·`surrounded`·`demostormrages`의 제목·안내·성공/실패·선택 버튼을 번역했다. UTF-8 BOM/CRLF이며 공식36/43파일의 한국어/원본 영어 독립 구조 감사가 통과했다. 실행 설정·원본의 닫는 따옴표가 없는 ai3Name·AI/SP·시간 조건·진행 키도 보존했다. 새 세 파일에는 그림 참조가 없다.

일반 캠페인2경로의 실제 일본어 시작·비정지·실패/동맹 메시지 표시가 종료0이었다. 최초 일본어 DemoStormRages 메뉴 진입은 `Player 참여 이름의 NUL 종료 없음`으로 종료1이었다. 기본 이름 `イービル・デューク`9문자가 표시 토큰36BYTE로 늘어나, RawAiBrainInitialUpdate의 원문 strcpy 경로에서 Player+0xc의 이름32BYTE를 넘어 subscriber+0x2c를 덮는 원인을 확인했다. 최초 원시 로그·보고·실행 결과는 그대로 보존했다.

배포 기본 번역을 Evil Duke의 뜻을 살린 `悪の公爵`(16BYTE)으로 바꾸고, Client 기본 이름 공급자에서 확장 표시 문자를31BYTE 이내의 완전한 토큰으로 제한했다. 원본 몸체/fixture는 변경하지 않았다. 회귀 검사는 원래36BYTE 입력, 정확히31BYTE인 혼합 이름, 32BYTE로 넘어가는 혼합 이름, 새 배포 기본값과 현재 Config 치환을 확인한다.

제품/Root/집중 대상 Release 빌드 종료0, 채팅/AI 이름6/6와 언어/표시51/51검사가 통과했다. 큰 전체 `netstorm_tests` 재컴파일는 중단하고 관련 집중 대상을 만들었다. 전수 검사 완료로 세지 않는다. 새 개발용 `netstorm_chat_channels_tests.exe`는 `cmake --build cpppj/build --config Release --target netstorm_chat_channels_tests`로 빌드하고 `cpppj/build/bin/Release/netstorm_chat_channels_tests.exe`로 실행한다. `--list` 및 `--filter <이름 일부>`도 지원하며 게임 창/배포 대상이 아니다.

수정 후 실제 메인 메뉴의 데모→목록 클릭으로 DemoStormRages·GuardMyBack2경로를 확인했다. 원래 오류를 낸 긴 이름을 명시 설정한 DemoStormRages도 별도 실행했다. 세 실행 모두 종료0이며 일본어 Demo 안내의 phase3/정지에서 원래15초 timeout 뒤 phase4/비정지로 전환됐다. 긴 이름 실행은 제품 종료0·보고 저장 뒤 검사 도구의 CP949 콘솔 print만 실패했다. 제품을 반복 실행하지 않고 저장된 결과와 보호 지문을 별도 확인했다.

[2-4](../screens/cpp-japanese-campaign-1345-20261126/guardmyback.png) · [2-5](../screens/cpp-japanese-campaign-1345-20261126/surrounded.png) · [폭풍 데모](../screens/cpp-japanese-campaign-1345-20261126/demo-demostormrages-fixed.png) · [등을 맞댄 데모](../screens/cpp-japanese-campaign-1345-20261126/demo-guardmyback-fixed.png). 제목/본문/버튼 잘림이나 겹침은 없었다. 일반2회는 수정 전 EXE, 수정 후 데모3회는 아래 새 EXE임을 기록에서 구분한다.

새 EXE SHA-256 `458f23e09021c5c8d7230459c1499034d8fddfb69a471da60af5c39e4741b918`. 보호 원본/CD·AGENTS/README와 설치 사본3,212파일은 최초 실패 전과 같다. [검증 JSON](../../cpppj/japanese-campaign-1345-20261126.json). 원시 자료·빌드/집중 검사·최초 실패·도구 설명은 `extracted/cpp-followups-1328/*1345*`에 있다.

직접 공격·포획·희생·승리·성공 안내/완료 키는 실행하지 않았다. 동맹 상태 안내는 본문만 요청했고 실제 피해/구조 결과로 세지 않는다. 데모는 원래 자동 안내와 짧은 대기만 확인했다. 다음1346은 캠페인3 3-1~3-3이다.
