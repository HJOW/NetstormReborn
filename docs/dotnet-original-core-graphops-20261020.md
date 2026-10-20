# 원본 월드 코어 W02 열넷째 조각: 그래프 영역·추가·flood·등록 후처리

2026-10-20, 작업 PC **HJOW-Athlon**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 열넷째 조각이다. [열셋째 조각](dotnet-original-core-graph-20261020.md)이 남긴 `RawGraph` 의 영역 무효화·추가/flood·등록 후처리를 옮겼다. 근거는 커밋된 cpppj 소스와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawGraph.cs` (확장) | 원본 `Calculate`/`Commit` 계획 계산 — 슬롯·해시·점유 지도에서 표면 스냅샷(`SurfaceObject`/`GraphMembership`)을 만들고 `SurfaceFinder`+`SurfaceGraph` 로 `Region`(`00462d40`)·`Add`(`004633c0`)·`Flood`(`00462f10`)·`Allocate` 를 수행. `PostPop`/`ValidateAdd`/`ValidatePostPop` 와 `IRawPostPopGraph` 구현. 모든 검사는 사본에서 하고 성공한 계획만 반영한다 | `RawGraph.cpp` 의 `Calculate`·`Region`·`Commit`·공개 연산 |
| `X86RawGraphReplayTests.cs` | `rawgraph-x86.tsv` 10.78 몫 **768회**(Pop 64·PostPop 320·Add 256·Flood 128)와 `regiongraph-x86.tsv` 10.78 몫 **512회**(Pop 96·PostPop 160·Add 96·Region 160, 두 fixture 합계 1,280회) 재생 | `RawGraphTests.cpp` 의 `ReplayRawGraph` |

장면은 cpppj 재생과 같다: 실제 슬롯 풀(32,768)·해시·점유 지도 위에서 클라이언트 슬롯 7개를 확보하고, Setup 의 노드 13필드·프레임 코드·해시 체인·내부 점유·표 1,530바이트·스택 4,096(영역 fixture 는 시드+증가)·Totalmade stale 값(31337+i·2654435761)을 그대로 넣는다. 표시(`SquidDisplay`)는 슬롯·공간을 바꾸지 않으므로 끈 경로(null)를 썼고, 공통 Pop·PostPop 은 기존 `RawPop`·`RawPostPop` 을 연결했다.

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 사후처리·예측 Pop·추가/flood | `rawgraph-x86.tsv` **768회** | 호출마다 7개 슬롯 50바이트, 표/스택/4단계 해시/점유 지도/풀 체크섬, TotalCost·깊이·세 Totalmade 표 |
| 영역 무효화·추가 | `regiongraph-x86.tsv` **512회** | 같은 열 전체 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,210개 통과**(Assets 363 + Core 847; 이번 +6).

**변이 확인:** 구현 14곳을 하나씩 망가뜨려 **9곳이 실패로 잡혔다**: 그래프 byte 위치(+30→+31), 영역 끝 버킷 확장 제거, 영역 점유 비트 8→4, 영역 레코드 감소의 `>0` 조건 제거, commit 번호 쓰기 생략, 0단계 머리를 빈 배열로, 표면 타입 검사 제거, 스택 복사 생략, Flood 목적 번호 0 고정. 나머지 5곳(투영 단계 비교, 영역에 투영 전달, 추가 조기 반환 조건, `RegionAdd` 구분, 영역 단계 순서·번호 병합 조건·비표면 후보 필터)은 이 재생 장면에서 결과가 같아 잡히지 않았다 — 쓰기 없는 사전 검사에만 쓰이거나(투영), Graph 쪽 이중 방어(추가 조기 반환)이거나, 장면에 그 분기가 없다(`RegionAdd`·비표면 후보·단계 간 중복). 원본 소스대로 유지했고 소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- `Detach`(삭제 준비, 원본 `004637b0`/`00463110` 계열 조회·연결)·`Rebuild`(전체 재구성)·`PostDestroy`·`Frame` 은 옮기지 않았다. `Calculate` 는 이 두 연산을 `NotSupportedException` 으로 거부하며, 해당 fixture(`graphremove`·`graphlookup`·`graphrebuild`·`graphrecovery`·`graphforms`)는 아직 연결하지 않았다.
- `graphrecovery` 의 `GraphRecovery.FullPool` 정책은 `SurfaceGraph` 에 넘기도록 자리만 만들었고 재생으로 검증하지 않았다.
- 표시 모듈(`SquidDisplay`)은 C# 에 아직 없어 재생에서 끈 경로를 썼다. 표시 효과의 실제 대조는 W14 에서 한다.
- CD 10.72·10.37, 실제 세션 연결, 파생 분배 허브는 그대로 미연결이다.

## 4. 다음 조각

`Detach`(위치 조회·연결 목록·분할 정책) → `Allocate` 소진 복구 검증·`Rebuild` 구현(`graphrebuild`) → `PostDestroy`/`Frame` → `graphremove`·`graphlookup`·`graphrecovery`·`graphforms` 재생 → 파생 분배 허브·실제 세션 연결. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
