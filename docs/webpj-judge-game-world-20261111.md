# webpj GameWorld·GameWorldInput 이식 판정 — WB04-E300 (2026-11-11, 4-B)

3.1절 4-B 의 **GameWorld·GameWorldInput**(client, 기준선 `.cpp` 931 + `.h` 222 + `.inc` 42 = 1,195줄; 3.1절 표기 1,155+42)을 이식할 것인지 판정한다. 코드는 옮기지 않고, 어느 부분을 웹이 옮기고 어느 부분을 옮기지 않는지와 옮길 단계를 정한다. E246(`o/Squid` 옛 struct 필드)·E289(`TerrainBuilder.h` 옛 지상 격자)가 이 판정에 맡긴 항목을 함께 닫는다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 결론

대응표 메모의 "옛 GameWorld 어댑터(--dump-world)"는 **절반만 맞다.** `GameWorld` 는 두 얼굴을 가진 클래스이다.

- **옛 값형(typed) 어댑터** — `FortTemplate` 에서 지면 타일·옛 `o::Squid` 객체·보행 격자(`GroundGrid`)를 직접 만들고 관찰 기반으로 이동시키는 경로. production 소비자가 `--dump-world` CLI(`app/main.cpp` 의 `GameWorld world(assets,fort,players)` 한 곳) 뿐이다(나머지 `GameWorld` 생성은 모두 Session 을 받는 두 번째 생성자 — `UberGump`·`UberEditor`·`UberLan`·시험). 웹은 raw 풀·`RawSurfaceWorld`·터레인 모델로 같은 일을 하므로 **옮기지 않는다**(제외).
- **실제 캠페인 호스트** — `RawCampaignSession` 을 소유하며 카메라·팔레트·표시 수집·선택/강조/명령 입력·미니맵을 한 곳에서 이어 주는 클라이언트 어댑터. `ClientRendererFrame`(friend)·`CampaignClientServices`·`CampaignObjectMenuClient`·`CampaignUserInputServices`·`ClientCampaignTransport` 가 `GameWorld&` 를 직접 받으므로 이 경로는 **버리지 않고** 웹의 같은 역할 모듈로 옮긴다. 다만 한 파일로 옮기지 않고 아래 3절의 구역별 단계에서 웹 구조(DOM 렌더러·Canvas·입력 핸들러)에 맞게 나눠 옮긴다.

따라서 `GameWorld.cpp`·`GameWorldInput.inc` 행은 `port`(todo)로 두고, 제외한 옛 구역과 옮길 구역·단계를 대응표 메모에 적는다. 새 제외 분류(`exclude-legacy` 규칙)는 만들지 않는다(파일 단위 제외가 아니라 구역 단위 제외이기 때문).

## 2. 제외하는 옛 값형 경로 (기준선 `GameWorld.cpp` 약 415줄)

| 구역(기준선 줄) | 내용 | 제외 이유 |
| --- | --- | --- |
| 생성자 1 (62~84)·`Place` (174~203)·`BuildTerrain` (251~305)·`TerrainFrame` (223~250)·`BuildSurfaces` (306~333)·`RefreshSurfaceGround` (334~348) | Fort 청크 → 지면 타일·`Tile`·`coreVariants_`·`regionOwners_/Themes_`·`ground_`(GroundGrid) | 웹은 raw 지형(E290~E296 `RawTerrain*`)과 `RawSurfaceWorld.load`(E299)로 대체. `TerrainFrame` 의 "변형 선택은 고정 시드의 어댑터"라는 설명대로 원본 동작이 아님 |
| `Step`·`AdvanceObjects` (368~381)·`Frame`·`Bounds` (439~479)·`Object` (585~592)·`objects_` | 옛 `o::Squid` 객체의 관찰 기반 이동·프레임·그림 사각형 | 실제 이동은 raw Kernel 프로세스(6-P/7), 그림 사각형은 `SquidDisplay` 가 소유 |
| `Point(label)` (729~830) | 자동 검사용 그림 지점 라벨 | 검사 보조(웹 시험은 raw 슬롯을 직접 읽음) |
| `Report` (849~920)·`--dump-world` | 초기 상태 TSV | 웹은 `RawSurfaceWorld.core()/objects()`·Session 덤프(4-D, 3.5절)로 같은 정보를 읽음. E299 가 `Report()` 를 옮기지 않은 이유와 같음 |

함께 닫는 항목:
- **E246 `o/Squid` 옛 struct 필드**: `struct Squid` 저장 필드·`Face`/`Advance`(GroundGrid/CellPoint/FortContentList 의존)의 production 소비자는 위 옛 경로(`objects_`)와 `UberGump` 일부뿐이다. 웹은 raw 슬롯을 쓰므로 제외한다 → 대응표 `o/Squid.cpp` 행을 partial 에서 **port**(EffectiveGenus 이식으로 완료, 나머지는 제외)로 올린다.
- **E289 `TerrainBuilder.h` 옛 지상 격자** `GroundCell`·`GroundGrid`·`CellPoint`·`Path`: 소비자가 위 옛 경로와 옛 `Squid::Advance` 뿐이다 → 제외. `TerrainBuilder.cpp` 의 본섬 마스크 모델(E289)은 이미 완료.

## 3. 옮길 실제 호스트 구역과 단계

| 구역(기준선 줄) | 내용 | 웹 단계 |
| --- | --- | --- |
| 생성자 2 (85~116)·`~GameWorld` (133)·`BeginLocalConnection`/`StartLocalConnection`/`PrepareLocalConnectionForState`/`CompleteFortInstall`/`CompleteLocalConnection` (117~132)·`Session()`/`Surfaces()`/`Players()`/`CurrentSubscriber` (833~843) | Session 소유·로그인 접합·Fort 설치 대여 | **4-D** Session 조립(이때 `RawSurfaceWorld`·`RawSurfaceStorage`·`RawProcessHostStorage` 를 한 수명으로 묶음) |
| `RunFrame`/`PrepareKernelFrame`/`FinishKernelFrame` (349~367)·`CheckMissionOutcome` (844~848)·`TakeChanged` (831) | 프레임 준비·Kernel 한 번·변경 표시 수집·미션 판정 | 4-D(프레임 루프)·8(미션 판정) |
| `Resize`/`Scroll`/`Home`/`CenterOnObject`/`ViewPriest`/`CenterCamera`/`MoveFromMinimap`/`CameraApplied`/`ResetCampaignCamera`/`MinimapView`/`MapView`/`BindMapView` (135~150·389~434·921~931, `ToScreen`/`ToCell` 435~438) | 카메라·초기 시야·홈 보기·미니맵 | **5-B** (카메라·HUD·미니맵) |
| `SetPalette`/`BindHideBuildings` (204~222)·`Sprites`/`SelectionImage`/`Remap` (593~728)·`RefreshCampaignObjects` (152~173)·`surfaceDisplay`/`soundViewChanged` | 표시 팔레트·스프라이트 수집·선택 괄호·소유자 색 변환 | **5-A** (렌더러·Draw) |
| `Pick`/`Select`/`SelectFromPointer`/`ExtendSelection`/`SelectNextUnit`/`SelectPriest`/`PrepareEditorObjectMenu`/`MoveSelected`/`SetMousePoint`/`Highlight*`/`OpenSelectionMenu`/`UpdateMovementCursor`/`ResetSelection`/`ClearSelection`/`BindUserInput` (480~584·`GameWorldInput.inc` 42줄 전부) | 선택·강조·이동 명령·객체 메뉴 부모 | **6** (입력·선택) — 웹 입력은 DOM 이벤트가 소유하고 이 모듈은 그 뒤의 선택 분배를 맡음 |

`GameWorldInput.inc`(42줄)는 `GameWorld::Input` 구조체와 선택/강조 메서드이며 위 마지막 구역(6)에 속한다. 기준선 뒤 변경이 없다(3.6절 ①).

## 4. 3.6절 판정 (기준선 뒤 cpppj 변경)

기준선 뒤 `GameWorld.cpp` 를 바꾼 커밋은 하나(`0817fa87`, 6줄)이다.

| 커밋 | 바뀐 함수 | 판정 |
| --- | --- | --- |
| `0817fa87` `20261111-154 CP NB20` | `GameWorld::SelectPriest` — 건설 조각이 커서에 붙은 채 P/R 로 사제를 선택하면 `hooks.pieceHeld`(잠금 깊이 0·미리보기 있음)로 조각을 먼저 취소(`campaign_->Cursor().Cancel()`)한 뒤 `SelectPriestByKey` 를 실행. 원문 결함 수정(AGENTS 10차 NB20) | **③ 6** — `SelectPriest` 를 옮길 때(입력·선택) 같은 두 훅과 `PriestSelectionShortcut` 의 새 필드를 함께 반영한다(webpj 10차 대상) |

나머지 구역은 기준선 판과 같다(①).

## 5. 후속

- 3.1절 4-B 는 **31/33** 이다(GameWorld·GameWorldInput 판정 완료 반영). 남은 항목: client RawFortWorldLoadBinding, 4-B 적재 통합 시험.
- 대응표 변경: `client/GameWorld.cpp`(base `0817fa87`, todo 유지, 구역 판정 메모), `client/GameWorldInput.inc`(base `1f00a25a`, todo 유지), `o/Squid.cpp`(partial → port, 옛 struct 제외 메모).
- 3.6절 ③ 표 6행에 NB20 항목을 추가한다.
