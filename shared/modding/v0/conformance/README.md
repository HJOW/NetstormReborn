# 공통 적합성 입력 v0.1.0

`core-probe.nsmod.json`을 **그대로** 두 플랫폼에서 실행한다. `core-trace.json`의 sessionSeed로 인스턴스를 만들고 엔트리 등록 뒤 cases의 event와 snapshot을 순서대로 공급한다. 등록/경계 출력의 JSON 값이 registration/expected와 같아야 한다. object key 순서 자체는 비교 대상이 아니지만 `JSON.stringify` 로그 문자열과 배열 순서는 계약 순서이므로 보존한다. 순수 API의 5경계이며 실제 게임·격리·입력 큐 관문은 별도로 실행해야 한다.

`rng-vectors.json`은 Python 정수 연산으로 독립 계산한 FNV-1a32/xorshift32 벡터다. `math-vectors.json`은 Node26 V8의 sin/cos/atan2/pow 29입력을 기록한 기준이다. args는 십진 문자열이며 `Number`로 해석한다. expected는 **big endian으로 적은 IEEE754 binary64 비트** 또는 `NaN` 분류다. 부호0/Infinity는 정확 비트를 비교하고 NaN payload는 비교하지 않는다. 수학 golden은 Node/V8 버전도 기록한다. 엔진을 올려 불일치하면 자동으로 golden을 덮어쓰지 말고 원인과 버전 정책을 검토한다.

2026-11-28 E1471에서 WB 순수 API 검사를 연결했다. DN은 사용자 지시로 구현 중지 상태이며 **DN 실행이나 양쪽 일치는 미검증**이다. DN 재개 후 MOD-3 수학/모듈 로더, MOD-4 바인딩이 이 자료를 소비한다. `core-trace.json`은 WB 기준 출력이며 기존 게임 오라클이 아니다.
