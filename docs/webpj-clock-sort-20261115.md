# 시간 보정의 Windows CRT 표본 정렬 (WB04-E516)

제품 1313의 `RawClockSynchronization::Poll`은 왕복 시간이 빠른 절반을
선택하기 전에 C++ `qsort`를 호출한다. 동률 표본이 서로 다른 offset을 가지면
정렬 결과에 따라 최종 시계 보정값이 달라진다. JavaScript 안정 정렬은 동률 두
표본의 입력 순서를 유지하지만 이 PC의 Windows UCRT는 두 표본을 교환한다.

`webpj/www/core/ucrt-clock-sort.js`는 0~70개 offset·왕복 시간 쌍을 제자리에서
정렬한다. 설치된 Windows SDK의 CRT 정렬 구현과 DLL 관측을 참고해 작은 구간의
최댓값 선택과 큰 구간의 중간값 분할·동률 경계를 구현했다. `Poll`이 제거하는 NaN
왕복 시간은 입력 범위가 아니다. 기존 원본 10.78의 CRT 정렬은 아직 별도 검증하지
않았으므로 이 결과를 원본 기계어 관측으로 주장하지 않는다.

Windows `ucrtbase.dll` 버전 `10.0.19041.7802`의 실제 `qsort`에 합성 double
쌍을 전달해 길이 1~70에 대해 동률·오름·내림·중복 난수 다섯 종류씩 총
350건의 순열을 `webpj/tests/fixtures/ucrt-clock-sort.json`에 기록했다. 재생
도구는 `webpj/tools/capture-ucrt-clock-sort.py`이며 게임 실행은 하지 않는다.
추가 독립 난수 입력 3,500건(길이당 50건, seed `0xdeadbeef`, 값 −3~3)도 같은
DLL과 웹 정렬기의 순열이 일치했다. 추가 자료는 Git 제외 경로
`extracted/webpj-oracle/ucrt-clock-sort-differential.json`에 있다.
제품 1313 오라클 실행 파일의 import 표에는 `api-ms-win-crt-utility-l1-1-0.dll`
`qsort`가 있다. 수집된 fixture는 추후 실행 환경이 Windows가 아니어도 검사한다.

이 정렬기는 아직 시간 동기화의 `Poll`에 연결되지 않았다. 표본 만료·빠른 절반
평균·시계 보정·송신과 고정 수신기 baseline·Session 연결은 계속 남아 있으며
`RawClockSynchronization` 상태는 partial이다. Node 22·26 전체 회귀 검사를
수행했다.
