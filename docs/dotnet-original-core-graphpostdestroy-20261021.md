# 원본 월드 코어 W02 열여덟째 조각 — 그래프 삭제 후처리와 방향 코드 조회(PostDestroy·Frame)

2026-10-21, 작업 PC **HJOW-Athlon**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 열여덟째 조각이다. 열일곱째 조각이 남긴 `RawGraph` 의 마지막 두 메서드를 옮겼다. 근거는 커밋된 cpppj 소스다. 원본 fixture 재생은 `destroygraph` 계열이 수명주기 전체(장부·비용·Unpop·Release·가상 분배)를 요구하므로 W03 에서 하며, 이 조각은 합성 검사로 검증한다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawGraph.cs` (확장) | `PostDestroy(sid)`·`ValidatePostDestroy(sid)` — 발자국 영역 계산·범위 고정, 풀 사본 위에서 일반 탐색(`RawSquidFinder.Begin`·`Next`) 순서대로 표면만 `Add`, graph byte 변경분만 계획에 넣어 반영. `Frame(sid)` — 판본 폭(10.78 4바이트)으로 프레임 번호를 읽고 타입·프레임 표 범위를 확인한 뒤 방향 코드를 돌려준다 | `RawGraph.cpp` 의 `CalculatePostDestroy`·`PostDestroy`·`ValidatePostDestroy`·`Frame`, `RawGraph.h` |
| `RawGraphTests.cs` (확장) | 합성 검사 3건. 순차 등록(먼저 묶인 무리를 다음 `Add` 가 읽는다)·쓰기 전 거부·방향 코드 조회 | `DestroyGraphTests.cpp` 의 `FreeGuards`·`RejectsMismatched` 경계와 `RawGraph` 합성 검사 관례 |

`PostDestroy` 는 원본처럼 풀 사본(`RawSidPool` 복사본) 위에서 순차 `Add` 를 수행한다. 사본 슬롯 바이트를 현재 풀과 같게 채우고, 같은 해시·점유 지도·타입·프레임 표·표·스택·복구 정책을 공유하는 시험용 `RawGraph` 로 탐색 순서대로 등록한 뒤, graph byte 가 달라진 번호만 실제 풀에 반영한다. 성공 전 실패(손상 후보·소진·범위 밖 발자국)는 실제 풀·표·스택을 바꾸지 않는다. `Add` 를 이미 묶인 표면에 다시 부르면 표면 수가 또 느는 원본 동작도 그대로 따른다(첫 `Add` 의 flood 가 이웃까지 묶고, 다음 `Add` 가 같은 표면을 또 셈한다).

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,224개 통과**(Assets 363 + Core 861; 이번 +3).

| 검사 | 내용 |
| --- | --- |
| 순차 등록 | 3×1 원천 발자국 안의 1×1 표면 2개가 같은 무리에 묶이고(Surfaces 3 — flood분 + 직접분), 발자국 겹침으로 이웃에서 제외된 원천은 새 무리를 이룬다. graph byte 외 슬롯 필드는 그대로다 |
| 쓰기 전 거부 | 할당되지 않은 번호·빈 발자국은 예외, 비표면 원천은 등록할 표면이 없어 그대로 돌아간다. 풀·표·스택은 그대로다 |
| 방향 코드 조회 | 0·1번 프레임의 방향 코드를 읽고, 범위 밖 프레임·비자산 타입은 거부하며 풀을 바꾸지 않는다 |

**변이 확인:** 구현 2곳을 하나씩 망가뜨려 모두 실패로 잡혔다(프레임 번호 무시하고 0번만 반환 — 1번 조회가 불일치로 실패, `PostDestroy` 의 반영 누락 — 슬롯이 무효 번호 그대로여서 실패). 소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- `destroygraph-x86.tsv`(Pre·Post·Destroy 각 192행)·`island-graph-frame-x86.tsv`(24행) 재생은 하지 않았다. 이 행들은 삭제 수명주기 전체(선택·장부·비용·깊이·Unpop·Release·가상 분배 효과)를 대조하므로 W03(원본 월드 코어 ② 삭제·수명)의 `SquidDestroyLifecycle` 이식 뒤에 한다. `destroygraph` fixture 도 연결하지 않았다.
- `PostDestroy` 의 `NeedsPostGraph` 분기(수명주기가 그래프 효과를 건너뛰는 조건)는 수명주기 쪽 조건이라 이 조각에서 옮기지 않았다. `RawGraph.PostDestroy` 자체는 항상 계산한다.
- CD 10.72·10.37, 파생 분배 허브·실제 세션 연결은 그대로 미연결이다. 기준 판본은 10.78 뿐이다.

## 4. 다음 조각

W03 원본 월드 코어 ② 삭제·수명 — `SquidDestroyLifecycle`(Pre·Post·Destroy)·`RawSquidDestroy`·장부·`destroygraph` 전체 재생(`Pre`·`Post`·`Destroy` 각 192행과 섬 H 치환 24행) → 파생 분배 허브·실제 세션 연결. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
