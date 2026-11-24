# 시간 동기화의 플랫폼 적응 판정 — WB04-E1317

2026-11-24, HJOW-Athlon. LEFT_JOBS 0.2의 RawClockSynchronization 닫는 조건 중 ‘적응 판정’을 적용한다. 제품 알고리즘을 바꾸지 않고 대응표 상태를 `port`로 승격하며 플랫폼 적응 사유를 메모에 남긴다. 분류는 기존 `rebaseline-port` 규칙을 유지한다.

비교 기준은 제품 1313 `225553df`다. 이 C++ 실행 파일은 `api-ms-win-crt-utility-l1-1-0.dll`의 `qsort`를 사용한다. 웹은 E516에서 관측한 Windows UCRT 10.0.19041.7802의 동률 순서를 플랫폼 독립적으로 고정한다. 안정 정렬로 대체하면 RTT가 같은 서로 다른 offset의 빠른 절반 선택이 달라지므로 현행 구현을 보존한다. 길이 1~70의 실제 DLL 순열 350건과 E517의 C++ Poll 합성 장면 24건이 근거다. [정렬 근거](webpj-clock-sort-20261115.md)

원본 10.78 EXE의 구형 CRT 동률 순서는 별도로 관측하지 않았다. 따라서 이 판정은 원본 CRT 일치 주장이 아니며, 복원된 C++ 기준선의 플랫폼 동작을 브라우저에서 고정한 적응이다. 향후 구형 CRT 증거가 생기면 C++와 웹을 함께 재대조한다.

실제 Session의 등록·초기화·Attach·Poll은 `raw-campaign-session-login-front.js`가 같은 ClientLocalStateGlobals의 전역·70표본을 빌린다. E706·E947의 앱 결선, E1268의 이탈 초기화와 E1274의 단독 기본 수신기 범위 판정을 함께 적용한다. C++ `dataOwner_`의 canonical 장기 수신기/반납 baseline은 C++ 장치 수명 표현이며, 웹은 같은 내부 연결의 일반 등록과 JS 클로저/GC로 처리한다. LAN·소켓 주소 및 별도 Native 기본 수신기 발생원은 웹 범위에 포함하지 않는다. [앱 결선](webpj-app-clock-synchronization-20261119.md) · [이탈 정리](webpj-mission-leave-state-reset-20261123.md) · [단독 수신기](webpj-client-default-receivers-scope-20261123.md)

검증: Node 26.13.0에서 `ucrt-clock-sort.test.mjs`, `raw-clock-synchronization.test.mjs`, `client-local-state-globals.test.mjs` 46/46 통과(`webpj/extracted/webpj-e1317-clock.log`). 새 제품 코드나 fixture는 없다. E1310에서 이미 port로 승격한 CampaignObjectMenuClient의 낡은 인계 집계도 함께 바로잡는다. 대응표는 총 1,160행, 대상 1,007 중 port 272·session 732·partial 3·todo 0이다. 남은 partial은 RawCampaignSession·RawButtonControls·RawMenuChat 세 행이다.
