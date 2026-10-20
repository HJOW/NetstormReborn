# 원본 월드 코어 W02 열일곱째 조각 — 그래프 소진 복구 재생(FullPool)

2026-10-21, 작업 PC **HJOW-Athlon**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 열일곱째 조각이다. 열여섯째 조각이 남긴 `graphrecovery` 재생을 수행했다. 소스 변경 없이 검사 연결만으로 원본 `GraphRecovery.FullPool` 경로를 검증한다. 근거는 커밋된 cpppj 소스와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `X86RawGraphReplayTests.cs` (확장) | `Scene` 에 `GraphRecovery` 보관, `Begin`·`Setup` 에서 같은 값으로 `RawGraph` 생성. `Replay` 에 `globalFixture`·`recovery` 인자 추가. `graphrecovery-x86.tsv` 10.78 몫 **64회**(`Add` 16·`PostPop` 16·`Pop` 16·`Detach` 16) 재생, `GlobalRebuild` 와 같은 전역 풀 재생성과 17열 표 전체 바이트 검사를 쓴다 | `RawGraphTests.cpp` 의 `ReplayRawGraph` (`RecoveryInsideAddDetachPopAndPostPop`, `recovery=FullPool`) |
| `RawGraphTests.cs` (확장) | 소진 표에서 `Add` 의 정책 분기 합성 검사 1건 (`Reject` 거부·`FullPool` 복구) | `RawGraphTests.cpp` 의 `AllocateForOperation` 소진 경로 |
| `Netstorm.Core.Tests.csproj` (확장) | `graphrecovery-x86.tsv` 링크 1건 | `cpp_sync.py` 연결 규칙 |

`RawGraph`·`SurfaceGraph` 소스는 고치지 않았다. `Add`·`Detach`·`Allocate` 의 `FullPool` 분기는 열넷째·열다섯째·열여섯째 조각에서 이미 옮겨져 있었고, 이번 조각은 `FullPool` 로 이은 재생이 원본과 같은지만 확인한다. 표시(`SquidDisplay`) 효과는 슬롯·공간을 바꾸지 않으므로 끈 경로(null)를 썼고, 표시 dirty 열(row[14]·row[15])은 비교하지 않는다.

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 상위 호출 안의 재구성·소진 할당·지도·통계 | `graphrecovery-x86.tsv` **64회** | 호출마다 7개 슬롯 50바이트, 표/스택/4단계 해시/점유 지도/풀 체크섬·TotalCost·깊이·세 Totalmade 표·표 전체 바이트 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,221개 통과**(Assets 363 + Core 858; 이번 +2).

**변이 확인:** 소스 2곳을 하나씩 망가뜨려 모두 실패로 잡혔다: `Add` 가 복구 정책을 무시하고 `Reject` 로 호출 — 재생 16행 이상과 합성 검사가 소진 예외로 실패, `DetachAt` 이 복구 정책을 무시하고 `Reject` 로 호출 — 재생의 `Detach` 16행이 소진 예외로 실패. 소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- `PostDestroy`·`Frame` 은 옮기지 않았다. `destroygraph` fixture 는 아직 연결하지 않았다.
- `graphrecovery` 의 CD 10.72·10.37 몫, `graphforms`·`graphrebuild` 의 CD 몫, 표시 모듈(`SquidDisplay`) 재생은 그대로 미연결이다. 기준 판본은 10.78 뿐이다.
- 파생 분배 허브·실제 세션 연결은 그대로 미연결이다.

## 4. 다음 조각

`PostDestroy`·`Frame`(`destroygraph`, `PostDestroy` 계산·`Frame` 번호) → 파생 분배 허브·실제 세션 연결. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
