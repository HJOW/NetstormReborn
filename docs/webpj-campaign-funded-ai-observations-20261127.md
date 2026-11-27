# 캠페인1-3/1-4 시작 자금 조건의 실제 AI 관찰

WB04-E1424, 2026-11-27, HJOW-Athlon. 같은 수집기/시작 자금 조건을 두 미션에 적용한 한 관찰 단위다. `E1425`는1-4 실행 로그의 하위 case ID이며 별도 완료 단위로 중복 가산하지 않는다. [C++ 자동 플레이 안내](testing/cpp-autoplay-test-guide.md)의 자금 조건처럼 아군 시작20000 SP·적 시작1000 SP를 지정하고 실제 아군 어려움/적 쉬움 AI로 진행했다.

| 미션 | 실제 종료 표본 | 결과 | 게임 오류 | 도구 종료 |
| --- | --- | --- | --- | --- |
|1-3 SaveTheIsland|45,300프레임·1140.925초|아군 사제 포획994.5초·희생1140.9초·사망1140.925초, 실제 Failure/Continue|0|1, 자연 승리 단언 실패|
|1-4 FragileFortune|60,000프레임·1509.175초|포획/사망/승패 창 없이 상한 종료|0|1, 자연 승리 단언 실패|

두 종료 화면을 직접 확인했다.1-3에는 실제 Failure 안내가 있고1-4는 건물/다리가 남은 진행 화면이다.1-3 종료 시 아군 사제 SID0·적15001,1-4는 아군15000·적15067이었다. 양쪽 AI의 초기화/스캔은1·중지는0이며 현재 상대 소유자를 가리켰다.1-4 아군 SP는22600으로 남았다. 자금 조건만으로 두 미션의 승리가 확인되지는 않았다. **두 미션의 목표/승리·신규 저장·Next 검사는 미완료로 유지한다.**

## 실제 입력과 수집 경계

`MissionScript.get`에서 선택한 미션의 `myStartMoney`를20000, 요청된 적 `ai2StartMoney`를1000으로 읽게 했다. 원래 `myStartMoney` 텍스트는2000이었고 `ai2StartMoney` 명시 키는null이었다(공통/기본 AI 설정과 별개다). 실제 아군 생성 직전 SP20000을 읽어 확인했다. 이름·동맹·기술은 그대로였고 플레이 중 SP/HP/유닛/목표/완료값에는 쓰지 않았다. 제품·원본 언어 파일은 수정하지 않았다.

모든 조작은 실제 아군 AI가 수행했으며 일반 Kernel/Renderer/Screen 프레임을 모두 실행하고 최종 Canvas 제출만 미뤘다.1-3/1-4만 시작하도록 별도 인자 제한을 뒀고1-6은 실행하지 않았다. 자연 승리 단언 전에 전체 AI 본문·오류 배열·Session 덤프·화면을 저장했다. 그 뒤의 완료값/저장/Next 검사는 실행하지 않았다. 이 조건의 관찰을 기본 자금 플레이나 원본/C++와의 World/RNG 동일성으로 세지 않는다.

명령은 각각 `node extracted/e1424-funded-campaign-flow.cjs savetheisland 60000`, `node extracted/e1424-funded-campaign-flow.cjs fragilefortune 60000`이다. [공용 수집기](webpj-e1424-evidence/funded-collector.cjs), [1-3 결과](webpj-e1424-evidence/campaign1-3/result.json)·[로그](webpj-e1424-evidence/campaign1-3/flow.log)·[화면](webpj-e1424-evidence/campaign1-3/terminal.png), [1-4 결과](webpj-e1424-evidence/campaign1-4/result.json)·[로그](webpj-e1424-evidence/campaign1-4/flow.log)·[화면](webpj-e1424-evidence/campaign1-4/terminal.png)과 [환경/해시](webpj-e1424-evidence/provenance.json)를 보존했다. 앞선 기본 자금1-4 상한은 [E1417](webpj-campaign1-4-ai-120000-20261127.md),1-3 패배는 [E1408](webpj-campaign1-3-ai-defeat-20261127.md)과 구분한다.

완료 주기는 v26=3/4·v22=8/8이다. 다음 작업으로 Node22 전체 회귀를 수행하며 다른 버전 전체와 동시에 실행하지 않는다.
