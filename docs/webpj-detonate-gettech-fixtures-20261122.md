# Detonate·GetTechnology 입력 분배의 원본 관측 대조 (WB04-E1199~E1200)

대응표에서 "분배 계약만"으로 남아 있던 두 일반 입력 분배(`DetonateInput`·`ThemeTechnologyCommand`의 `DispatchGetTechnologyInput`)를 cpppj 가 가진 원본 10.78 x86 관측과 대조했다. 두 관측 파일은 웹 시험이 쓰지 않던 것이다.

| 분배 | 관측 | 시험 | 결과 |
| --- | --- | --- | --- |
| `dispatchDetonateInput`(원문 4d6eca..4da477) | `cpppj/tests/fixtures/detonate-input-x86.tsv` 768행(객체 칸 16×사건 코드 4×대상 y 4×변형 3) | `tests/detonate-input-native.test.mjs` | 객체 호출 흔적(`object:대상:사건코드;return:0` 288행·호출 없음 480행)과 소비 여부가 768행 모두 일치. 대상 마스크(`y & 0xffff`)를 바꾸면 시험이 실패함을 변형으로 확인 |
| `dispatchGetTechnologyInput`(원문 542970 → 492a30) | `cpppj/tests/fixtures/get-technology-input-x86.tsv` 48행(사건 코드 4×둘째 값 6×편집기/미션) | `tests/get-technology-input-native.test.mjs` | 지급 호출 흔적(`technology:둘째 값 DWORD`, 눌림 24행)·완료 열·스택 변화가 48행 모두 일치 |

두 분배는 웹 앱의 입력 프레임(Detonate)과 대본 스크립트 명령(`CheatAllTechnology`, E1189)에 연결돼 있다. 이식 추적 목록(`tests/fixtures/manifest.json`)에 두 관측 파일과 새 시험을 등록했다.
