# cpppj 반영 재검토 — `f95a8db2` → HEAD (R0-2)

2026-10-28, 작업 PC `VM-W11-Codex`. [LEFT_JOBS.dotnetpj.md 0절](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261028) R0의 영향 목록이다. 마지막 반영 커밋 `f95a8db2`(20261023) 이후 cpppj 관련 커밋 **265개**가 쌓였다. 이미 옮긴 C# 모듈의 근거 소스가 바뀌었는지, 연결 fixture가 바뀌었는지를 확인했다. 게임·창·디컴파일은 실행하지 않았고 `cpppj/`·원본·AGENTS.md는 수정하지 않았다.

## 1. 요약

| 항목 | 결과 |
| --- | --- |
| 연결 fixture 61개의 SHA·행 수 | **모두 일치.** 반영 이후 cpppj fixture 변경은 추가 215 / 수정 0 / 삭제 0이다. 기존 L1 검사의 기대값은 하나도 다시 만들어지지 않았다 |
| 생성 자료 3개 | 일치 |
| cpppj `src` 변경 | 769파일, +30,916/−2,146줄 (`src/o` 신규 252·수정 171 / `src/client` 신규 223·수정 119, 삭제 0) |
| 근거 소스를 주석에 적은 C# 65개 | **근거 소스가 바뀐 파일 30개**, 바뀌지 않은 파일 35개 (`PORT_MAP.tsv` 기준으로는 68행 중 32행) |
| dotnetpj 기준선 | Release 경고·오류 0, 검사 1,349개(Assets 363 + Core 986) 통과 (이번 단위에서 +2) |

**판정:** 연결 fixture가 같으므로 기존 L1 일치는 유지된다. 그러나 바뀐 소스에는 (A) 현재 C#과 **동작이 달라진 경계 조건**이 있고, (B) 새 fixture가 마련된 **미이식 동작**이 있고, (C) 세션 조립(R4)에서 반드시 따라야 할 **소유 구조 변경**이 있다. 이 변경들은 fixture 기대값이 아니라 cpppj 소스의 diff와 새 fixture로 확인했다.

## 2. 변경 성격별 분류

### A. 수정 필요 — 현재 C#이 이전 cpppj 동작에 머물러 있음

C# 소스를 직접 확인했다. 해당 cpppj 검사와 새 fixture를 연결해 L1로 다시 대조한 뒤 고친다(R0-4).

| C# 파일 | 바뀐 cpppj | 현재 C# | 대조할 새 fixture |
| --- | --- | --- | --- |
| `RawSquidReward`·`RawSpStore`·`RawHitPoints` | `SquidReward.cpp`(3커밋): `SpMirrorFtol` 신설. 비유한 SP·64비트 범위 밖에서 예외 대신 **하위 DWORD 0**을 돌려준다. SP 저장 시 유한 검사 제거. `SpStore.cpp`: 할당 입력을 한 행씩 발행하는 `allocation` 공급자와 `TrackingOffsetCurrent` | `__ftol`이 비유한·범위 밖을 거부(`RawSquidReward.cs:230`), 시작 SP가 비유한이면 예외(`:480`) | `sp-add-bits`, `sp-initialization`, `fort-sp-write` |
| `RawGraph` | `RawGraph.cpp`(2커밋): 비표면 객체의 영역 통지에 `RegionCoordinate`(소수 좌표를 0쪽으로 절삭, 범위 밖만 예외)를 쓴다. 표면은 정수 칸을 유지 | `Coordinate`가 정수가 아닌 좌표를 거부(`RawGraph.cs` 다수) | `island-graph-frame`(연결됨), `island-population`, `rawgraph`(연결됨) 재실행 + 비표면 소수 위치 경계 |
| `RawPriestState`·`RawPriestDamage` | `RawPriestState.cpp`(2커밋): 사제 HP 회복 허용을 `bool[]`이 아니라 **실제 Player 기록의 신전 DWORD**(10.78 stride 0xb4/offset 0x7c, CD 0xac/0x74)에서 읽는다 | `bool[] RecoveryAllowed`(`RawPriestState.cs:12`) | `priestregen`(연결 안 됨), `priest-panic`, `priest-prayer` |
| `RawAssetPostPop`·`OriginalTypeRecord` | `RawAssetPostPop.cpp`(7커밋): 분배표 20→24개. 데빌메이커·아크 스파이어·애시드 바리케이드·성장형 아이스 타워 종류 추가, 호출 시점의 TYPE을 조회(`Type(sid)`). `RiftType.h` 1줄 | 새 종류 4개와 호출 시점 TYPE 조회 없음 | `assetpostpop`(연결됨) 재실행, `arc-spire`, `acid-fence` 등 |
| `RawSquidFactory` | `SquidFactory.cpp`(3커밋): 동일 SID 타입 변경 `Retype`(4b1620, 10.78 전용, 진단 3종), 최종 VT 판별 `SupportsConstructedVtable`, 미등록 생성자 오류 | `Retype` 없음 | `squid-retype`, `object-conversion` |
| `RawSidPool`·`SidPool` | `SidPool.cpp`(2커밋): `ClearDeletionHistory`, 서버 판정을 공유 포인터(`sharedServer`)로 | 둘 다 없음 | `squid-pool-reset`, `state-sid-lists` |

### B. 새 fixture가 있는 미이식 동작

R4(세션 조립)에서 필요하고 fixture 연결만 하면 L1을 마칠 수 있다. 이번에 연결하지 않았다.

- SP·SID·객체 수명 11개: `object-conversion`, `object-owner-command`, `sp-add-bits`, `sp-initialization`, `squid-pool-reset`, `squid-retype`, `state-bridge-reset`, `state-flat-reset`, `state-sid-lists`, `state-world-cleanup`, `state-zone-reset`
- Editor·Fort 저장/적재 48개 (`editor-*`, `fort-*`, `island-*`, `new-fort` 등) — Edit 이식(R7)과 R4의 Fort 적재가 쓴다.
- 카메라·화면·팔레트·그리기 17개 (`camera-commit`·`camera-present`·`camera-view`, `colorsort-*`, `kencloud*`, `minimap-*`, `object-hit`) — R1·R2·R5가 쓴다.
- Gump·메뉴·HUD·UI 37개 (`gump-pointer-boundary`, `game-menu*`, `html-*`, `chat-*`, `dialog-*` 등) — R3·R6.
- AI 17개 (`ai-bridge-*`, `acid-*`) — R6. 그 밖의 유닛·전투·건설·경로·배경 GIF 등 85개.

### C. 세션 조립(R4)에서 반영할 소유 구조 변경 — 지금 C#을 고치지 않음

이 변경은 단독 L1 결과를 바꾸지 않는다. 실제 Client 전역을 빌리는 방식으로 cpppj 내부 구조가 바뀐 것이다.

- `SquidPostPopList`: 독립 vector 대신 **현재 목록을 빌리는 `CurrentBinding`**(저장소·용량·개수 읽기/쓰기). `Append`/`RemoveAll`을 목록이 소유한다. `SquidOwner.cpp`·`SquidPostPop.cpp`·`SquidDestroyLifecycle.cpp`가 같은 `list.Append`/`RemoveAll`을 쓰도록 정리됐다. 동작은 같고 소유권이 바뀌었다.
- 장부 상태 `state_.X` → `state_.X()`(접근자): `totalCost`·`providers`·`factories`·`localCounts`·`globalCounts`·`lostX/Y`·`geyserPool`·`percent`·`priestQuarterHp`. 각 값이 실제 전역 하나를 가리킨다는 뜻이다. `RawSquidDestroyLifecycle`·`RawAssetPostPop`·`RawSquidReward`의 상태 객체를 R4에서 세션 전역에 연결한다.
- `SquidFactory`의 약화된 마나(`weakenedMana`)와 `SidPool`의 서버 판정은 **같은 전역을 빌린다**(주소 중복 연결은 거부).
- `RawGraph`의 표·스택은 `RawGraphStorage`에 두어 **장기 물리와 미션 교체 수명을 분리**한다(생성자가 공유 표를 다시 초기화하지 않는다).
- `SquidHash.MutableEntries`(Editor의 직접 WORD 쓰기용 버킷 대여), `SquidPop.ReleaseMetadisplayDirty`(교체 중 이전 처리기 소멸이 새 관측 대여를 덮지 않음)가 추가됐다.

### D. 근거 소스가 바뀌었으나 영향이 작은 것

`OriginalPopDisplay`·`OriginalUnpopDisplay`·`RawPop`·`RawUnpop`(각 +2줄, 미니맵 대여 해제), `RawOwner`(리스트 위임), `RawSquidHash`(+2줄 접근자), `RawContainedFinder`·`RawContentLifetime`·`RawCarrierPostPop`·`RawCarrierPreDestroy`·`RawDamageable{Pre,Post}Destroy`·`RawDamageableRelease`·`RawMetadisplayDirty`·`RawPriestPreDestroy`·`RawPostPop`·`RawSquidDestroyLifecycle`(위 C의 접근자/대여 변경), `CampaignMenu`(`DialogScript.cpp` 4커밋, R3에서 교체), `GifImage`(`GifImage.cpp` 1커밋, R2에서 재대조). 이 파일들의 diff는 줄 수가 작고 fixture가 같으므로 L1 결과가 바뀐다는 증거는 없다. 단 각 파일을 `--ports-reflect`로 올리기 전에 해당 diff를 한 번 읽는다.

## 3. 바뀌지 않은 모듈 (35개)

`BridgeFrameSearch`, `RawBombPreDestroy`, `RawBridgeEvent`, `RawBridgeLifecycle`, `RawBuildingSpace`, `RawContainedCreate`, `RawContainedLookup` 등 `PORT_MAP.tsv`에 있고 `--ports` 보고에 나오지 않는 파일이다. `cpp_sync.py --ports`로 언제든 다시 확인한다.

## 4. 다음 처리

- **R0-4:** A의 여섯 묶음을 우선순위(`sp-add-bits`·`squid-retype`·`squid-pool-reset`·`priestregen` → `RawGraph` → `RawAssetPostPop`)로 나눠 새 fixture를 `Link`+`--add`로 연결하고 C#을 고친다. 고친 파일은 `--ports-reflect`로 기준 커밋을 올리고 단위 기록을 남긴다.
- **R1부터:** 이 표의 B 중 화면·카메라·팔레트 fixture를 R1~R2에서 쓴다.
- 이 단위는 영향 파악이며 C#은 수정하지 않았다. 바뀐 cpppj가 모두 현재 C#과 일치한다는 뜻으로 읽지 않는다.
