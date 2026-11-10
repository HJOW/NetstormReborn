# webpj RawGraph 조합 대조 마감 — WB04-E257

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-D의 첫째 항목이다. cpppj 기준선 `1f00a25a`의 `o/RawGraph`(+`RawGraphLive`: 실제 SID 풀·4단계 해시·spot 위의 정수 Graph — 영역 통지·Add·삭제 연결·재구성·복구)는 [E40](webpj-raw-graph-20261108.md)에서 사전 계산과 실제 Flood/Rebuild/Allocate/Add/PostDestroy/Detach 를 옮겼고, Pop/PostPop/Unpop·SquidDisplay 가 함께 도는 조합 대조(rawgraph·regiongraph·graphremove·graphrecovery)는 SquidPostPop 처리기 단위(E41)로 넘겼다. 이 단위는 그 넘긴 조합 대조가 cpppj `RawGraphTests.cpp` 의 모든 시험을 덮는지 확인하고, 덮이지 않은 두 시험을 웹으로 옮겨 `RawGraph`를 port 로 올린다. 게임·브라우저 실행과 디컴파일은 하지 않았다(Node 단위 검사만).

## 1. 기준선 뒤 변경 판정 (3.6절)

`git diff --numstat 1f00a25a HEAD`는 `RawGraph.cpp`·`RawGraph.h`·`RawGraphLive.cpp`·`RawGraphTests.cpp` 와 네 fixture(`rawgraph`·`regiongraph`·`graphremove`·`graphrecovery`) 모두 변경 0 이다(저장소 루트에서 확인). 행 base 는 기준선 `1f00a25a` 그대로 둔다.

## 2. 시험 대응표 (cpppj `RawGraphTests.cpp` 18개)

| cpppj 시험 | 웹 시험 |
| --- | --- |
| `X86_AllocatedSidPostPopAndProjectedPop` | `squid-postpop.test.mjs` rawgraph |
| `X86_DetachGeneralIslandBridgeUnpopAndRelease` | 같은 파일 graphremove |
| `X86_DetachUsesPositionHeadGraphAndSourceFrame` | `raw-graph.test.mjs` graphlookup 128행 |
| `X86_GlobalRebuildAndExhaustedAllocation` | 같은 파일 graphrebuild 96행 |
| `X86_RecoveryInsideAddDetachPopAndPostPop` | `squid-postpop.test.mjs` graphrecovery |
| `RecoveryPreviewRejectsOtherVoidAndOutOfRangeTypeBeforePop` | `raw-graph.test.mjs` 복구 사전 계산 |
| `RebuildRejectsDamageAndFastAllocateDoesNotReadPool` | 같은 파일 여유 번호 경로 |
| `PositionHeadEarlyReturnAndPreflightPreserveSource` | 같은 파일 조회 머리의 무효/미사용 번호 |
| `RegionX86_ThreeBinariesAndNormalBridgeIslandPop` | `squid-postpop.test.mjs` regiongraph |
| `GraphReferenceChecking_ActualRegionReplaysExistingNativeGeometry` | 같은 파일 regiongraph(checking) |
| `ProjectedPreviewAndActualPopKeepDifferentFailureEffects` | `raw-graph.test.mjs` 사전 계산(앞 절반) + **신규** `raw-graph-pop.test.mjs`(실제 Pop 후반) |
| `RegionThenAddFailureRetainsEarlierPopMutation` | **신규** `raw-graph-pop.test.mjs` |
| `RefreshesFramesMapAndRawMembershipBetweenCalls` | CD 10.72 전용(`Cd1072`) — 웹 범위 밖 |
| `DetachPreflightRejectsBeforePoolSpaceOrGraphMutation` | `raw-graph.test.mjs` 삭제 계획의 소진/손상/활성 원천 |
| `NonSurfaceFractionalRegionPreservesPositionAndNeighbor` | 같은 파일 비표면 그림의 소수 좌표 |
| `CorruptCandidatesAndMismatchedConnectionsRejectBeforeMutation` | 같은 파일 공간/타입 연결 검증 |
| `SharedStorageAndExplicitSnapshotHaveSeparateOwnership` | 같은 파일 대여 실행기 |
| `SharedPostDestroyValidationKeepsCloneAndCommitSeparate` | 같은 파일 postDestroy 임시 사본 |

`RawGraph::Commit`은 실제 실행이 쓰지 않아 웹에서 제외한다(E40 결정 유지). fixture 줄 수: rawgraph 1,797·regiongraph 1,927·graphremove 1,549·graphrecovery 391(CD/10.37 행 포함 전체 줄, 10.78 `originals` 행만 사용).

## 3. 옮긴 것 (`tests/raw-graph-pop.test.mjs`, 2건)

- **사전 계산과 실제 Pop 의 실패 효과가 다르다:** 정상 251행이 모두 사용 중인 입력에서 `validatePostPop(…, projected)`(x=20, 20.25)는 거부하되 풀·해시·spot·기록·장부·깊이를 바꾸지 않는다. 실제 `pop.pop`은 후처리가 실패하기 전에 공간(해시 머리 = SID, void 해제, firstPop extra 0x80)과 깊이 1 을 이미 썼고 그래프 표는 그대로이며, 소수 좌표(20.25)로 다시 시도하면 변화 없음(`unchanged`)이고 깊이도 늘지 않는다.
- **영역 통지 뒤 Add 실패:** 3×3 표면 root 의 Pop(flags 0x201)에서 영역이 후보의 그래프 BYTE 를 254 로 지우고 표 한 행의 surface WORD 를 하나 줄인 뒤 Add 가 실패해도, 앞선 3×3 spot 점유(비트 8)·root 의 next 체인(옛 머리 = 후보)·같은 버킷의 root 머리가 그대로 남는다. 후보 슬롯은 그래프 BYTE 하나만 바뀌고 나머지 바이트가 같다.
- 두 시험은 처음부터 통과했다(첫 실행 2/2). 변형 시험 5개(영역 BYTE 무효화 제거·영역 표 감소 제거·영역 spot 비트·영역 왼쪽 경계·영역 비표면 건너뜀 제거) 중 3개를 탐지했고, 남은 2개는 이 장면의 입력(후보 x 가 경계와 같지 않음·후보가 모두 표면)이 구별하지 못하지만 기존 `regiongraph` 512행·`graph-region-free` 4행 등 영역 대조가 계속 덮는다.

## 4. 경계와 다음 시작점

- 이 단위로 `RawGraph`는 PORT_MAP `port`(base `1f00a25a`)다. 앱 수명 연결(실제 World 의 Graph 소유자)은 4-B/4-D 에서 한다.
- 3-D 의 다음(마지막) 항목은 `SquidPostPop`(partial 677줄, +36)이다: 표시 dirty 열, 3-A~3-C 몸체의 `setBody`/`set*Prefix` 연결, 섬/표면/Teleport 접두.
