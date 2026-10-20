# 원본 월드 코어 W02 열다섯째 조각 — 그래프 삭제 준비(Detach)

2026-10-20, 작업 PC **HJOW-Athlon**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 다섯째 조각이다. 열넷째 조각이 남긴 `RawGraph` 의 삭제 준비를 옮겼다. 근거는 커밋된 cpppj 소스와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawGraph.cs` (확장) | 원본 `Calculate` 의 `Detach` 경로 — 죽음·정책 검사→위치 조회(0단계 머리 SID 읽기·무효/미사용 자연 반환)→`SurfaceFinder`+`SurfaceGraph` 로 `DetachConnections`·`DetachAt` 수행. `ValidateDetach`·`Detach` 공개. `FullPool` 일 때 삭제 준비도 전체 풀에서 읽는다. 치환 타입(`sourceType`)은 위치 정보만 바꾼다 | `RawGraph.cpp` 의 `Calculate`·`DetachConnections`·`Detach`·`ValidateDetach`, `Graph.cpp` 의 `Detach`·`DetachAt` |
| `X86RawGraphReplayTests.cs` (확장) | `graphremove-x86.tsv` 10.78 몫 **386회**(Detach 128·Unpop 256·Release 2)와 `graphlookup-x86.tsv` 10.78 몫 **128회**(Detach 128) 재생. `RawUnpop`·`Release` 연결 추가 | `RawGraphTests.cpp` 의 `ReplayRawGraph` |
| `RawGraphTests.cs` (확장) | 무효·미사용 조회의 자연 반환과 죽음·감소 수·위치 불일치의 쓰기 전 거부 2건 | `RawGraphTests.cpp` 의 `PositionHeadEarlyReturn`·`DetachPreflight` |

장면은 cpppj 재생과 같다: 실제 슬롯 풀(32,768)·해시·점유 지도 위에서 클라이언트 슬롯 7개를 확보하고, Setup 의 노드 13필드·프레임 코드·해시 체인·내부 점유·표 1,530바이트·시드 스택을 그대로 넣는다. 표시(`SquidDisplay`)는 슬롯·공간을 바꾸지 않으므로 끈 경로(null)를 썼고, 공통 Pop·PostPop·Unpop 은 기존 모듈을 연결했다.

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 이웃 분리·다리/섬 해제·반납 | `graphremove-x86.tsv` **386회** | 호출마다 7개 슬롯 50바이트, 표/스택/4단계 해시/점유 지도/풀 체크섬·TotalCost·깊이·세 Totalmade 표 |
| 위치 조회·원천 프레임 유지 | `graphlookup-x86.tsv` **128회** | 같은 열 전체 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,215개 통과**(Assets 363 + Core 852; 이번 +4).

**변이 확인:** 구현 3곳을 하나씩 망가뜨려 모두 실패로 잡혔다: 조회 번호 대신 원천 번호 사용(슬롯·표 불일치 66행), 가로·세로 접촉 비교 반전(슬롯 불일치 60행), 빈 머리 조기 반환 제거는 이 장면에 빈 머리가 없어 잡히지 않았다(원본대로 유지). 소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- `Rebuild`(전체 재구성)·`PostDestroy`·`Frame` 은 옮기지 않았다. `Calculate` 는 이 연산을 `NotSupportedException` 으로 거부하며, 해당 fixture(`graphrebuild`·`graphrecovery`·`graphforms`·`destroygraph`)는 아직 연결하지 않았다.
- `graphrecovery` 의 `FullPool` 정책은 삭제 준비 경로에만 넘기고 재생으로 검증하지 않는다.
- 표시 모듈(`SquidDisplay`)은 C# 에 아직 없어 재생에서 끈 경로를 썼다. 표시 효과의 실제 대조는 W14 에서 한다.
- CD 10.72·10.37, 실제 세션 연결, 파생 분배 허브는 그대로 미연결이다.

## 4. 다음 조각

`Rebuild`(전체 재구성, `graphrebuild`·`graphforms`) → `Allocate` 소진 복구 재생(`graphrecovery` 의 `FullPool` 경로) → `PostDestroy`·`Frame`(`destroygraph`) → 파생 분배 허브·실제 세션 연결. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
