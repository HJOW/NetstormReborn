# 공통 적합성 입력 v0.1.0

`core-probe.nsmod.json`을 **그대로** 두 플랫폼에서 실행한다. `core-trace.json`의 sessionSeed로 인스턴스를 만들고 엔트리 등록 뒤 cases의 event와 snapshot을 순서대로 공급한다. 등록/경계 출력의 JSON 값이 registration/expected와 같아야 한다. object key 순서 자체는 비교 대상이 아니지만 `JSON.stringify` 로그 문자열과 배열 순서는 계약 순서이므로 보존한다. 순수 API의 5경계이며 실제 게임·격리·입력 큐 관문은 별도로 실행해야 한다.

`rng-vectors.json`은 Python 정수 연산으로 독립 계산한 FNV-1a32/xorshift32 벡터다. 수학 프로필은`stdlib-fdlibm-js-v0`이며`@stdlib/math-base-special-sin/cos/atan2/pow` 각0.3.1의 고정 JS 구현/lock 전이를 양쪽에서 사용한다. `math-vectors.json`은29개 경계 입력,`math-grid.json`은 고정32비트 정수열로 만든256개 부호/지수/서브노멀/큰 인수 입력이다. args는 십진 문자열이며`Number`로 해석한다. expected는 **big endian으로 적은 IEEE754 binary64 비트** 또는`NaN` 분류다. 부호0/Infinity는 정확 비트를 비교하고 NaN payload는 비교하지 않는다. 이는 고정 JS 구현의 기준 출력이며 전체 영역의 올바른 반올림 정확도를 독립 증명한 자료는 아니다.

E1480에서 실제 Chromium/Firefox는`cos(0.1)=3fefd712f9a817c1`, 초기 Node26 native 기준은`...c0`인1ulp 차이를 재현했다. 엔진 native Math를 공통 경로로 간주했던 초기 설계를 교정해 엔진별 libm 대신 같은 JS 구현을 주입한다. 29행 중 이1행만 고정 JS 결과로 바뀌었고 초기29행 원문/SHA는`docs/webpj-e1480-evidence/native-math-original.json`에 보존했다. 엔진을 올려 불일치하면 자동 golden 갱신으로 숨기지 않는다. 구현 프로필/패키지 변경은 명세와 DN 문서를 함께 검토해야 한다.

2026-11-28 E1471에서 WB 순수 API 검사를 연결했다. DN은 사용자 지시로 구현 중지 상태이며 **DN 실행이나 양쪽 일치는 미검증**이다. DN 재개 후 MOD-3 수학/모듈 로더, MOD-4 바인딩이 이 자료를 소비한다. `core-trace.json`은 WB 기준 출력이며 기존 게임 오라클이 아니다.

E1480 WB는 실제 아군AI1-1의 자동 사건/입력/시계 장벽과 Chromium/Firefox의 공통5경계·285수학 입력을 확인했다. DN 구현/양쪽 일치는 여전히 미검증이다. 별도 동시 확대 실행의 Worker timeLimit1/2 실패는 보존하고 한도를 늘리지 않았다. 요청 phase/시간 계측을 추가한 뒤 두브라우저순차2/2이며 이전 부하/시간초과 원인의 해결로 세지 않는다.

E1483의 설치/관리 경계는 WB의 `mod-package-store.test.mjs`·`mod-manager.test.mjs`와 실제 `mod-panel.spec.js`에서 검사한다. 새 설치/명시 교체는 꺼짐, 설치 중 entry 비실행, 다른 탭/저장 오류의 원문 보존, 다음 미션 적용을 유지한다. 옛 실행의 늦은 오류는 직렬 쓰기 안에서 같은 정규화 패키지인지 확인하며 교체본/삭제를 덮지 않는다. DN MOD-5도 [같은 규칙](../../../../docs/webpj-mod-options-20261128.md)을 적용해야 하지만 DN 구현/실행은 중지 상태다.

E1484는 같은 최종 번들/공통 입력의 Linux GUI Chromium에서5경계/285수학 벡터·권한/CSP/시간 한도·관리/실제아군AI1-1/Reset All을8/8로 확인했다([환경/원문](../../../../docs/webpj-mod-linux-gui-20261128.md)). WB의 추가 플랫폼 근거이며 DN 실행이나 양쪽 적합성 완료는 아니다.
