# 메뉴60/120 선택·시계·메모리 실측

WB04-E1403, 2026-11-27, HJOW-Athlon. 실제 메뉴에서60/120 FPS를 두 번씩 선택하고 같은 호스트의 실제 rAF·게임 시계·renderer/V8 메모리를 읽었다. **제품 게임 코드는 바꾸지 않았다.** 원래 시각 공급원과 프레임 루프를 유지했으며 미션에는 들어가지 않았다.

서버를 켠 뒤 webpj에서 `node tools/menu-frame-profile.cjs`로 재현한다. 도구는 독립 저장소의 새 Chromium에서 실제 메뉴 좌표를 클릭하고, 네4초 구간의 호스트 경과와 원래 게임 시계를 비교한다. 표시 상한 설정은 실제 UI로 바꾸며 시각/프레임을 주입하지 않는다. CDP의 [Runtime.getHeapUsage](https://chromedevtools.github.io/devtools-protocol/tot/Runtime/#method-getHeapUsage)와 [Performance.getMetrics](https://chromedevtools.github.io/devtools-protocol/tot/Performance/#method-getMetrics)를 사용한다. GC 직후 표본과 일반 표본을 구별해 원문에 남겼다.

| 설정 | 처리FPS | 실제 구간 초 | 게임 시계 차이 ms | GC 뒤 JS사용 바이트 |
| ---: | ---: | ---: | ---: | ---: |
|60|60.193|4.0038|0.2|9,585,820|
|120|60.133|4.0078|0.2|9,679,256|
|60|60.238|4.0008|0.2|9,713,320|
|120|60.151|4.0066|0.4|9,721,992|

동일 구간의 rAF 횟수와 처리 프레임 수가 같았다. 현재 headless Chromium의 rAF가 약60Hz였으므로120 설정을 **실제120FPS 출력 확인으로 올리지 않는다**. 네 구간에서 비고정 게임 시계/벽 시각이 실제 경과를 유지했고, 설정 전환 사이에도 게임 시계가 뒤로 가지 않았다. 마지막 실제 저장값도120이다.

네 GC 뒤 backing storage는39,579,834바이트로 같았다. 첫 구간 GC 뒤 대비 마지막은 JS사용량136,172바이트 증가다. 짧은 메뉴 표본이며 캐시/코드 준비와 누수를 구분한 장시간 검사로 해석하지 않는다. Runtime의 JS/외부 저장소 수치이며 OS 전체 프로세스/GPU 메모리를 측정했다고 주장하지 않는다. renderer TaskDuration은 별도의 metric Timestamp 구간 대비1.28~1.55%였고, 페이지의4초 표본 구간과 완전히 같은 측정창은 아니다.

Node26.13.0·Chromium156.0.8078.4·Windows에서1-6 AI 관찰/webpack 빌드/다른 DN 작업과 호스트를 공유했다. 부하 없는 기준 성능이나 물리 모니터 인증이 아니다. 미션 진입/종료 반복과 장시간 게임 메모리, 실제120Hz 화면은 남겨 두었다.

첫 임시 수집기는 닫힌 popup이 null인 것을 목록으로 잘못 읽어 실패했다. 수집기만 수정하고 최종 네 표본/오류0·종료0을 다시 확인했다. [첫 실패 원문](webpj-e1403-evidence/first-collector-failure.log), [최종 원문](webpj-e1403-evidence/menu-profile.json), [로그](webpj-e1403-evidence/menu-profile.log), [계산·조건·해시](webpj-e1403-evidence/provenance.json)를 보존했다.

새 도구의 브라우저 콜백 환경도 ESLint 설정에 명시했다. 전체 선행 검사/webpack 빌드가 통과했고 운영 번들은 E1400과 같은3,500,786바이트·SHA-256 e24f9de3…0b다([빌드](webpj-e1403-evidence/build.log)). 제품 동작을 바꾸지 않은 M5 부분 검증이며 전체 M5는 미완료다. E1397 정기 회귀 뒤3/4·3/8이고 E1401 관찰은 아직 진행 중이다.
