# 원본 월드 코어 W02 열여섯째 조각 — 그래프 전체 재구성(Rebuild)

2026-10-21, 작업 PC **HJOW-Athlon**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 열여섯째 조각이다. 열다섯째 조각이 남긴 `RawGraph` 의 전체 재구성을 옮겼다. 근거는 커밋된 cpppj 소스와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawGraph.cs` (확장) | 원본 `Calculate` 의 `Rebuild` 경로 — 전역 스냅샷(전체 풀 읽기) 위에서 `SurfaceGraph.Rebuild(resetAll)` 수행. `Rebuild(bool)`·`ValidateRebuild(bool)` 공개. `Allocate` 소진 경로와 같은 전역 읽기(`Free`만 보고 `Void` 포함)를 쓴다 | `RawGraph.cpp` 의 `Calculate`·`Rebuild`, `Graph.cpp` 의 `Rebuild`·`AllocateForOperation` |
| `X86RawGraphReplayTests.cs` (확장) | `graphrebuild-x86.tsv` 10.78 몫 **96회**(`Rebuild` 64·`Allocate` 32) 재생. 전역 fixture 라서 `Setup` 마다 풀을 새로 확보해 같은 번호(5~11)를 확인한다. 17열 행은 표 255개 전체 바이트도 직접 비교한다 | `RawGraphTests.cpp` 의 `ReplayRawGraph` (`GlobalRebuildAndExhaustedAllocation`) |
| `X86RawGraphFormsTests.cs` (새 파일) | `graphforms-x86.tsv` 10.78 몫 **32행** 재생. 내부 Form 슬롯 6개를 포함한 전체 풀 재구성 뒤 슬롯·표·스택 전체를 비교한다 | `GraphFormsTests.cpp` 의 `ActualNativeFullPoolSkipsNonSpatialForms` |
| `RawGraphTests.cs` (확장) | 초기화 재구성/무효 수집 구분과 해시 머리 내부 Form 거부의 합성 검사 2건 | `RawGraphTests.cpp` 의 `RebuildRejectsDamageAndFastAllocateDoesNotReadPool`·`GraphForms_HashFormRejects` |

장면은 cpppj 재생과 같다: 실제 슬롯 풀(32,768)·해시·점유 지도 위에서 클라이언트 슬롯 7개를 확보하고, Setup 의 노드 13필드·프레임 코드·해시 체인·내부 점유·표 1,530바이트·시드 스택을 그대로 넣는다. 표시(`SquidDisplay`) 효과는 슬롯·공간을 바꾸지 않으므로 끈 경로(null)를 썼다. 전역 Setup 은 빈 슬롯 입력(상태 바이트 free)까지 다시 준비하므로 cpppj 처럼 풀을 매번 새로 만든다(재사용하면 두 번째 Setup 부터 `AllocatedBytes` 가 free 비트 때문에 거부된다).

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 전역 무효 수집·초기화 재구성과 소진 할당 | `graphrebuild-x86.tsv` **96회** | 호출마다 7개 슬롯 50바이트, 표/스택/4단계 해시/점유 지도/풀 체크섬·TotalCost·깊이·세 Totalmade 표·표 전체 바이트 |
| 내부 Form 생략 전체 재구성 | `graphforms-x86.tsv` **32행** | 슬롯 7개 전체·표 255개 전체·스택 4,096개 전체·해시 머리·점유 지도 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,219개 통과**(Assets 363 + Core 856; 이번 +4).

**변이 확인:** 구현 2곳을 하나씩 망가뜨려 모두 실패로 잡혔다: 재구성 여부 반전(`target != 0` → `== 0`, 두 재생이 슬롯 불일치로 실패), 재구성 대신 소진 할당 호출(두 재생이 슬롯 불일치로 실패). 소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- `Allocate` 소진 복구의 `graphrecovery` 재생(`Add`·`PostPop`·`Pop`·`Detach` 의 `FullPool` 경로 10.78 몫 64회)은 옮기지 않았다. 합성 검사(`AllocateRecoversExhaustedTableViaRebuild`)만 있고 fixture 재생은 다음 조각이다.
- `PostDestroy`·`Frame` 은 옮기지 않았다. `destroygraph` fixture 는 아직 연결하지 않았다.
- `graphforms` 의 CD 10.72 몫 32행, `graphrebuild`·`graphrecovery` 의 CD 10.72·10.37 몫, 표시 모듈(`SquidDisplay`) 재생은 그대로 미연결이다. 기준 판본은 10.78 뿐이다.
- 파생 분배 허브·실제 세션 연결은 그대로 미연결이다.

## 4. 다음 조각

`Allocate` 소진 복구 재생(`graphrecovery` 의 `FullPool` 경로) → `PostDestroy`·`Frame`(`destroygraph`) → 파생 분배 허브·실제 세션 연결. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
