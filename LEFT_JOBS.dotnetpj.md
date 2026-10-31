# LEFT_JOBS.dotnetpj — dotnetpj(C# + MonoGame) 작업 계획 및 인수인계

> **2026-10-29 사용자 요청으로 일시 정지 → 2026-10-30 사용자 요청으로 재개:** 0.61의 구현·검증·인계와 로컬 커밋을 마친 뒤 목표를 일시 정지했다. 2026-10-30 "dotnetpj 다음 작업 진행해 줘" 요청으로 재개해 R3k-9(스크립트 명령·Config 적용)를 0.62에서 완료했다. 남은 전체 범위와 재개 순서는 0.62에 있다.

> **2026-10-30 AGENTS.md 변경 반영(문서만, 사용자 요청):** 후순위 4~6차 재편(0.7의 'AGENTS.md 상위 요구')과 `Netstorm.exe` 실행 파일 이름 요구(0.8 5번·0.9 이름 적용 항목)를 반영했다. 공통 [LEFT_JOBS.md](LEFT_JOBS.md)도 같은 요청으로 함께 갱신했다(아래 0.7의 '수정하지 않는다'에 대한 사용자 지시 예외). 구현·빌드·실행은 하지 않았다.

> **문서 구성(2026-10-28):** [0절 현행 계획](#dotnet-current-plan-20261028)이 이 문서와 [이식 계획 문서](docs/dotnet-cpp-port-plan-20261016.md)의 순서·구조 결정보다 우선한다. 1절은 지금까지의 완료 범위 색인, 2·3절은 계속 유효한 사용자 결정, 4·5절은 검증 명령과 참고 문서다. 더 이상 현행 의미가 없어진 옛 계획·날짜별 기록은 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)로 옮겼다(6절). 이 문서는 [LEFT_JOBS.md](LEFT_JOBS.md) 끝의 분할 지침에 따라 dotnetpj의 진행과 인수인계만 담는다.

<a id="dotnet-current-plan-20261028"></a>

## 0. 2026-10-28 현행 계획 — cpppj 그래픽 복원분을 기준으로 dotnetpj 재구성

- [x] **이번 요청(문서):** 사용자 요청은 다음과 같다. "cpppj 진척이 특히 그래픽 쪽은 거의 다 된 상황이라 dotnetpj에도 이를 반영해도 될 수준이 왔다. 문제는 dotnetpj에 이미 구현된 내용들이 기존 게임과 다르게 구현된 게 많아, 이미 구현된 dotnetpj 소스도 다시 수정해야 된다." 이에 따라 계획을 다시 세웠다. AGENTS.md·LEFT_JOBS.md·이 문서·이식 계획 문서를 읽고 cpppj/dotnetpj 소스 구성을 확인했다. **이번에는 이 문서를 고치고, 더 이상 의미가 없어진 내용을 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)로 옮기고(6절), 이식 계획 문서 머리에 안내 1줄을 넣었다.** 게임 코드·검사·동기화 목록·cpppj·원본/CD·AGENTS.md는 바꾸지 않았다. 빌드·검사·게임/창 실행·디컴파일도 하지 않았다.

### 0.1 확인한 현재 상태 (2026-10-28, VM-W11-Codex)

| 항목 | cpppj (HEAD `adc5774a`, 단위527) | dotnetpj (마지막 DN 커밋 `aa182874`, 2026-10-23) |
| --- | --- | --- |
| 규모 | `src/o` 921파일 약 6.3만 줄, `src/client` 565파일 약 5.3만 줄이다. 한 줄이 긴 밀집 코드이며 `RawCampaignSession.cpp` 하나가 약 540KB다. 등록 검사 3,358개, x86 fixture 669개 | Assets 45파일 7.1k줄, Core 147파일 30.2k줄(그중 `Original` 55파일 15.5k줄), Game 40파일 10.8k줄, Net 0. 검사 170파일·1,349개(10-23 기록) |
| 화면 | 원본의 8비트 논리 화면·256색 팔레트·Renderer dirty 갱신·RLE 셰이프·소유자/생산/미리보기 색표를 그대로 두고 **마지막 출력만** 현대 borderless DIB로 바꿨다(단위495). 4:3/16:9/16:10 해상도 메뉴, 가장자리 스크롤을 갖췄다. Gump UI 전체·HUD·미니맵·덱·QWASZX·ESC·커서·픽셀 선택(524)·우클릭 메뉴까지 실제 연결했다 | MonoGame `SpriteBatch`가 RGBA 텍스처를 합성한다. 그림자는 실수 알파, 확대는 `RenderTarget2D`다. 영어 글자는 `.chfnt` 아틀라스, 그 밖은 FontStashSharp로 그린다. UI는 `OriginalUiSkin`의 근사 배치다. 같은 입력에서 cpppj와 화면을 비교하는 수단이 없다 |
| 규칙·세션 | `RawCampaignSession`이 Fort 적재·월드·Player·SP·Kernel/Process·건설·전투·의식·공통 전략 AI(`RawAi*` 33개 모듈)·승패·Edit를 소유한다. 34경로 짧은 진입을 통과했다(506) | `BattleSession`(엔티티 목록·`FixedTimestep`·정수 SP·관찰/팬게임 기반 근사)이 실제 플레이를 소유한다. `Original` 코어(W02 18조각·W03 17조각·W05 선행)는 L1 검증만 마쳤고 세션에 연결하지 않았다 |
| 플레이 | Early6·Training9·캠페인16·데모3·Edit 진입. 자연 완주 검증은 사용자 결정으로 보류 | 1-1·1-2 근사 플레이만 가능하고 나머지는 잠겨 있다 |
| 동기화 | — | 반영 커밋 `f95a8db2` 이후 cpppj 관련 커밋 **265개**(`cpp_sync.py` 기준: cpppj·docs/exe·LEFT_JOBS.md를 바꾼 커밋). 연결 fixture는 669개 중 61개(10-23 기록) |
| 디컴파일 | 공통 기록의 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**(LEFT_JOBS.md 0절)이며 현재 PC VM-W11-Codex와 다르다. 10-22 계획의 "현재 PC와 마지막 PC가 모두 VM-W11-Codex"는 공통 기록과 어긋나므로 공통 기록을 따른다 | 이식 근거는 cpppj 소스·fixture·문서이므로 디컴파일이 필요 없다. 산출물을 직접 읽어야 할 때만 AGENTS.md대로 재디컴파일한 뒤 진행한다 |

### 0.2 진단 — 기존 구현이 원본과 달라진 구조적 원인

1. **화면 모델이 다르다.** 원본과 cpppj는 팔레트 인덱스 버퍼에 RLE 셰이프를 색표로 찍는다. 그림자·반투명·번쩍임·날씨도 팔레트와 색표 연산으로 만든다. dotnetpj는 RGBA 텍스처를 합성하고 그림자를 실수 알파로 그린다. 이 구조로는 색 섞기·디더·팔레트 번쩍임·가림 순서를 픽셀 단위로 맞출 수 없다. 9-1의 "지금 구조로 같게 만들 수 없으면 구조를 바꾼다"에 해당한다.
2. **UI가 원본 Gump 트리가 아니다.** `MainMenuView`·`OriginalUiSkin`·`HelpWindow`·`FortMapViewer.*`의 메뉴와 창은 치수와 그림만 맞춘 자체 배치다. 따라서 Gump 생성·사건·입력·수명·Draw 순서가 다르다. 메뉴 항목의 조건(소유자·모드·CHEAT 관문 등)도 다르다.
3. **규칙이 근사 세션에 있다.** `BattleSession`의 24Hz 고정 틱·정수 SP·일률 건설 시간·범용 공격 처리는 cpppj가 복원한 Kernel/Regular·SP 난수 소비·건설 Process·타입별 처리와 다르다. 옮겨 둔 `Original` 모듈은 세션에 연결되지 않아 플레이를 바꾸지 못했다.
4. **이식 방식이 cpppj 진척을 따라가지 못한다.** 모듈마다 fixture를 하나씩 붙이는 L1 조각 이식(10-16~10-23, 35조각)은 cpppj의 265커밋 진척보다 느리다. 세션을 조립하기 전에는 플레이에도 반영되지 않는다.
5. **판정이 단위 통과 중심이다.** 같은 입력에서 cpppj와 화면·상태를 직접 비교하는 수단이 없다. 그래서 "검사는 통과하지만 다르게 보이는" 상태를 잡지 못했다.

### 0.3 구조 결정 (이번 재수립의 기본값)

- **결정 1 — 화면은 8비트 논리 화면으로 옮긴다.** cpppj의 `Screen`·`Renderer`·팔레트·셰이프·색표·Gump Draw를 C#으로 옮긴다. 그리기 대상은 팔레트 인덱스 버퍼(`byte[]`, 행 간격 포함)와 256색 팔레트다. MonoGame은 창·입력 장치·오디오 장치·**최종 출력**만 맡는다. 최종 출력은 인덱스를 RGBA로 바꿔 텍스처를 갱신하고, cpppj `ScaleScreenPresentation`과 같은 정수 픽셀 중심 최근접 확대와 검정 여백을 쓴다. 이 방식은 9-1의 "MonoGame 사용은 바꾸지 않는다"와 양립한다. 9-1 E(부분 다시 그리기)도 cpppj Renderer의 dirty 경로로 해소한다. 게임 화면 경로에서는 SpriteBatch 합성·실수 알파·`RenderTarget2D` 확대를 없앤다.
- **결정 2 — cpppj 구조를 보존해 직역한다.** cpppj의 파일·클래스·함수 경계와 이름을 따른다. 부작용과 호출 순서, 정수 폭(BYTE/WORD 감김), float32 저장 지점, 목록 순서, 난수 소비를 그대로 옮긴다. cpppj가 원본 메모리 배치(슬롯·전역 DWORD)를 쓰는 곳은 C#도 같은 배치를 쓴다. 이는 이식 계획 3절 3번의 "세션·화면 쪽은 의미만 옮긴다"를 대체한다. Win32·DirectDraw·GDI·WINMM·파일 장치처럼 cpppj에서도 대체 경계인 부분만 .NET/MonoGame으로 바꾸고, 차이와 이유를 한국어 주석에 적는다. Windows 전용 동작(커서 제한 등)은 호스트의 OS 분기에만 두어 Linux 후속 작업을 막지 않는다.
- **결정 3 — 프로젝트를 cpppj 디렉터리와 짝지어 배치한다.**

  | 프로젝트 | 대응하는 cpppj | 내용 |
  | --- | --- | --- |
  | `Netstorm.Core.Original` | `src/o` | 기존 55파일을 유지하고 계속 확장한다 |
  | **새 `Netstorm.Client`** | `src/client` 중 플랫폼 독립 부분 | 화면 버퍼·Gump·Renderer·세션·HUD·소리 논리. MonoGame에 의존하지 않는다 |
  | `Netstorm.Game` | `src/platform`, Win32 대체 부분 | MonoGame 호스트 |
  | `Netstorm.Assets` | `GameAssets`·`RiftTypeTable`·`Template`·`Config*` 등 | 자료 판독. cpppj HEAD와 재대조한다 |
  | `Netstorm.Net` | `src/zacket`, 연결 계열 | 멀티플레이. 후순위 |

  대응표 `dotnetpj/PORT_MAP.md`(cpppj 파일 → C# 파일 → 이식 기준 cpppj 커밋 → 상태)를 새로 만들어 `cpppj/SOURCE_MAP.md`와 짝을 맞춘다.
- **결정 4 — cpppj 실행 파일을 판정 기준(오라클)으로 쓴다.** cpppj는 원본 기계어와 대조된 결과이므로 dotnetpj는 같은 입력에서 cpppj와 같아야 한다.
  - 화면은 cpppj `--run … --ui-script <steps.tsv>`의 `snapshot` 논리 캡처를 기준으로 쓴다. 스크립트는 `프레임⇥동작⇥인자` 형식이며 click/right/key/esc/pointer/snapshot 등을 지원한다. 캡처는 `Client::Capture`가 쓰는 팔레트 변환 BMP다. dotnetpj에도 같은 문법의 실행기와 같은 형식의 논리 캡처를 만든다. 기존 `--ui-script-file`(`UiAutomation`)은 이것으로 대체한다.
  - 상태는 cpppj 헤드리스 출력(`--dump-world`·`--dump-types`·`--inspect-*`)과 이식 계획 8절 요청 1·2(Session 덤프·고정 입력 재생)로 비교한다.
  - 비교 도구는 `dotnetpj/tools/`에 둔다.
- **결정 5 — 기존 경로는 동결한 뒤 교체·삭제한다.** 기존 `BattleSession`·`FortMapViewer`·`MainMenuView` 등에는 새 기능 추가와 부분 수선을 멈춘다(실행 오류 수정만 예외). 새 경로가 같은 범위를 대체하면 기본 실행을 새 경로로 바꾸고, 기존 경로는 `--legacy` 실행 인자로만 남긴다. 1-1·1-2가 새 경로에서 같은 수준 이상으로 진행되면 기존 경로와 그 검사를 삭제한다. 근사를 기대하는 기존 검사는 기대값을 고치지 않는다. 대체되는 코드와 함께 은퇴시키고 은퇴 목록을 단위 기록에 남긴다. 원본 fixture를 쓰는 검사는 대상 모듈을 옮긴 뒤에도 유지한다. 이 결정은 이전의 "교체 도중에도 1-1·1-2를 계속 플레이할 수 있어야 한다"(이식 계획 3절 5번)를 대체한다.
- **결정 6 — cpppj가 미완·보류로 둔 범위는 추측으로 채우지 않는다.** cpppj가 명시 경계·미복원·보류로 둔 동작(0.7)은 dotnetpj 새 경로에서도 같은 경계로 둔다. cpppj가 명시 오류로 멈추는 미연결 경로는 같은 지점의 명시 예외·진단으로 옮긴다. 이전의 "미복원 동작은 근사를 유지한다"(이식 계획 3절 6번, 3절 9번)는 기존 경로에만 남는다.

### 0.4 기존 dotnetpj 소스의 재수정 판정

"교체"는 cpppj 대응 모듈을 새로 옮긴 뒤 기존 파일을 지운다는 뜻이다. "재대조"는 cpppj HEAD와 같은 입력에서 결과·오류 처리를 비교해, 같으면 유지하고 다르면 cpppj 구조로 바꾼다는 뜻이다.

| 기존 dotnetpj | 판정 | 기준이 되는 cpppj | 단계 |
| --- | --- | --- | --- |
| `Game/NetstormGame`·`DisplayManager`·`PerfMeter`·`FrameWallClock`, `Core/Display/ScreenLayout`·`WideScreenMode`·`FramePacing`·`DisplaySettings` | 교체. 호스트에는 창·입력·오디오 장치·출력만 남긴다. 와이드 시야 확장은 cpppj의 논리 해상도 방식으로 바꾼다. `settings.json`은 9-1 C 판정 때 `options.cfg`로 옮긴다 | `ClientMain`, `Screen`, `ScreenPresentation`, `ScreenResolutions`, `ScreenPaletteApply`, 단위495·501 | R1(설정은 R7) |
| `Game/UiAutomation` | 교체 | `app/main.cpp`의 `--ui-script`·`snapshot` | R1 |
| `Core/Display/OriginalGameClock`·`FrameLoopClock`, `Simulation/BattleSessionFrameClock` | 유지 후 재대조(L1 검증 있음). `ClientMain` 루프에 맞춰 새 호스트에 다시 연결 | `ClientMain`, `o/GameClock`, [main-loop.md](docs/exe/main-loop.md) | R1 |
| `Game/SpriteAnimation`·`FortMapViewer.Sprites`, `Assets/ObjectColorRemap`·`IsleColorRemap`·`ProductionTintRemap`·`MapSpriteFrames`·`StructureAnimation`·`BitmapFontAtlas`, `Core/Display/DrawOrder`·`DisplayBounds` | 교체. `DrawOrder`·`DisplayBounds`의 L1 대조는 새 모듈의 검사로 흡수 | `VFXDraw`·`VFXMappedDraw`·`VFXLine`·`VFXTransform`, `RawShapeToBuffer`, `SpriteDraw`, `*ColorMaps`, `Palette*`, `DrawSort`, `o/SquidDisplay` | R2·R5 |
| `Game/MainMenuView`·`OriginalUiSkin`·`HelpWindow`·`OriginalFonts`·`OriginalCursor`, `Core/Rules/ButtonGump`, `Assets/CampaignMenu`·`StartupTips`·`HelpDocument`·`HelpTopics`·`InlinePicture` | 교체 | `Gump*`, `ButtonGump`·`StoneButtonGump`, `MenuGump*`, `ScrollGump*`, `TextGump`, `HtmlWindow*`·`HtmlBodyGump`·`Markup`·`StyleText*`, `QuickHelp*`, `TellTip`, `UberGump`·`UberRootGump`, `NativeMainMenuState`, `BitmapFont`, `Cursor` | R3 |
| `Assets/BitmapFont`(`.chfnt` 판독)·`Palette`·`GifImage`·`ShapeDatabase`·`CursorBitmap`·`TaffArchive`·`XlatTable`·`XorCipher`·`WaveFile`·`PatternDecoder`·`Config*`·`Type*`·`OriginalText`·`GameFileSystem`·`GameDataLocator`·`GameResources`·`GameLanguage`·`GameUnitNames`·`TrueTypeCollection` | 재대조. L1/정적 대조 기록은 있으나 기준 시점이 10-07~10-10이다 | `GameAssets`, `RiftTypeTable`, `Template`, `Config*`, `Xlat`, `GifImage`, `BitmapFont`, `RawImage` | R0·R2 |
| `Assets/FortFile`·`FortMap` / `FortTerrainPreview`·`FortTerrainFringe`·`FortEdgeFarmPreview`·`FortIslandSupports` | Fort 판독은 재대조, 지형 미리보기 4종은 교체 | `RawFort*Load`, `RawCampaignWorldLoad`, `TerrainBuilder`, `RawTerrain*`, `Islandbuilder`·`RawIsland*`, `RawFarmCluster` | R4 |
| `Assets/MissionScript`·`MissionConditions`·`TutorialDialogScript` | 교체 | `TellSection`·`TellParameters`·`DialogScript`·`Html*`·`ScriptCommand`·`RawMissionGuide` | R3·R6 |
| `Core/Simulation/BattleSession.*`(15파일)·`BattleSessionFactory`·`GameEntity`·`GameCommands`·`SessionEvents`·`PlayerState`·`UnitProduction`·`*Animation`·`CombatImpact`·`FlyerFlight`·`MovementRate`·`UnitHeading`·`ConstructionTimes`·`TutorialStages`·`TutorialGeysers`·`FixedTimestep` | 교체 후 삭제 | `RawCampaignSession*`, `RawCampaign*Bindings`, `RawSurfaceWorld`, `State`, `Mission*`, 대응 `Raw*` | R4·R6 |
| `Core/Rules/*`(25파일: `BattleMap`·`SpotMap`·`Footprint`·`IslandOwnership`·`Energy*`·`StormPower`·`ProductionDeck`·`KnowledgeCatalog`·`PlacementCursor`·`RangeIndicator`·`SelectionMarks`·`GameCursor`·`MiniMapLayout`·`PlayerColors`·`MissionStart` 등) | 교체 후 삭제. `SpotRules`의 L1 대조는 새 해시 모듈 검사로 흡수 | `SquidHash`·`SquidSpatial`, `CanonDecoder`·`RawCanonPlacement*`, `RawEnergySupply`, `SpStore`, `RawProductionDeck`, `RawPlayerTechnology`, `RawCursor*`, `MinimapGeometry`, `RawPlayerColor`, `RawMissionStart` 등 | R4~R6 |
| `Core/Bridges/*`(13파일) | L1 대조된 `BridgeDecayRules`·`SurfaceFinder`·`BridgeSurfaceRules`·`SurfaceGraph`는 `Original` 쪽 대응 모듈로 합친다. 나머지는 교체 후 삭제 | `Bridge`, `Graph`·`RawGraph`, `SquidFinder`, `RawBridge*`, `CanonDecoder` | R4 |
| `Simulation/MissionFrameGate`·`PlayerParticipation`·`MissionParticipation`·`TotalMade`·`TutorialOne`·`NetstormRandom`·`MsvcRandom`·`SidPool` | 유지 후 재대조(L1 검증 있음). 세션 조립 때 cpppj와 같은 소유 위치로 재배선. `SidPool`은 `RawSidPool`과 하나로 정리 | `RawMissionFrameGate`, `RawPlayerParticipation`, `RawTotalMade`, `RawTutorialOne`, `CrtRandom`, `SidPool` | R0·R4 |
| `Core/Original/*`(55파일) | 유지. 265커밋의 영향을 재대조한 뒤 세션 조립에 쓴다 | 대응 `o/*` | R0·R4 |
| `Core/Audio/*`, `Game/AudioPlayer`, `FortMapViewer.Audio` | 교체. 장치 계층만 MonoGame | `Sound`·`SoundProcess`·`SoundMusic*`·`SoundSceneMusic`·`SoundDevice`·`ClientAudio` | R6 |
| `Game/FortMapViewer*.cs`(26파일) | 교체 후 삭제 | `Renderer`·HUD·커서·선택·메뉴, `RawCampaignSession`의 입력 | R5·R6 |
| `Game/SpriteBrowser` | 개발 도구로 유지하되 게임 경로와 분리 | — | 필요 시 |
| `package-dotnetpj.ps1` | 유지. 프로젝트·자산 구성이 바뀔 때마다 갱신하고 압축 해제 실행을 확인 | — | 매 전환 |

### 0.5 실행 순서 (R0~R8)

각 단계의 괄호는 [이식 계획](docs/dotnet-cpp-port-plan-20261016.md) 5절의 W 번호다. W 번호와 그 fixture 목록은 근거 목록으로 계속 쓰고, 순서와 완료 기준은 이 절을 따른다.

**R0 — 기준선과 동기화 재정비 (W01 확장)**

- 현재 PC에서 cpppj HEAD를 Release로 빌드해 오라클 실행 파일의 SHA를 기록한다. 로컬 `cpppj/build/bin/Release/NetstormCpp.exe`가 HEAD 빌드라고 가정하지 않는다. dotnetpj도 Release 빌드·전체 검사로 기준선을 다시 잡는다.
- `cpp_sync.py`로 `f95a8db2..HEAD` 265커밋의 영향을 본다. 이미 옮긴 `Original` 55파일과 0.4의 "유지 후 재대조" 대상에 대해, 대응 cpppj 소스·fixture가 바뀌었는지 확인하고 수정 필요 / 영향 없음 / 교체 예정으로 나눈다. 연결 fixture가 바뀌었으면 해당 검사를 다시 대조한다.
- `cpp_sync.py`에 **이식한 cpppj 소스 파일의 해시 추적**을 더한다. `PORT_MAP.md`의 기준 커밋과 HEAD를 비교해 fixture뿐 아니라 소스 변경도 경고하게 한다.
- `PORT_MAP.md` 초판을 만든다.
- **완료 기준:** 기준선 수치·오라클 SHA·영향 목록·`PORT_MAP.md`가 커밋된다. 반영 커밋은 영향 검토를 마친 범위까지만 올린다.

**R1 — 8비트 화면 호스트·입력 큐·오라클 도구 (W14 일부, 9-1 B·E)**

- `Netstorm.Client`를 만든다. `Screen`(논리 크기·행 간격·Lock/Update·팔레트), `ScreenPaletteApply`, `ScreenResolutions`, `InputEvent`와 입력 큐(40칸, 넘치면 새 사건을 버림 — 9-1 B), `ClientMain` 프레임 루프 골격을 옮긴다.
- MonoGame 호스트를 만든다. 인덱스→RGBA 변환과 텍스처 갱신, 정수 픽셀 중심 최근접 확대와 검정 여백(4:3/16:9/16:10), borderless 전체화면과 창 전환, 마우스 좌표 역변환, 활성 전체화면에서의 커서 제한과 반납(cpppj 495 동작), 키·글자 사건 변환을 맡는다.
- cpppj와 같은 문법의 `--ui-script`, 같은 BMP 형식의 `snapshot` 논리 캡처, 필요한 실행 인자(`--window`·`--frames`·`--set`·`--mission`)를 만든다.
- 비교 도구 `dotnetpj/tools/cpp_frame_compare.py`를 만든다. 두 캡처의 크기·픽셀 차이 수·차이 영역을 출력한다.
- **완료 기준:** 행 간격 1368 같은 경계를 포함한 cpppj 출력 변환 검사를 이식해 통과한다. 4:3/16:9/16:10의 확대·여백·좌표 역변환 검사도 통과한다.

**R2 — 그리기 원시·팔레트·글꼴 (W14 일부)**

- 옮길 것: `VFXDraw`·`VFXMappedDraw`·`VFXLine`·`VFXTransform`, `RawShapeToBuffer`, `SpriteDraw`, `SpriteColorMaps`·`OwnerColorMaps`·`ProductionColorMaps`·`PreviewColorMaps`, `PaletteColors`·`PaletteShade`·`PaletteFlash`·`ParticlePaletteMemory`, `RawColorSort*`·`RawColorMapCache`·`ClientColorSortInput`, `BitmapFont`과 `GumpText`·`GumpTextDraw`의 글리프, `GifImage`·`RawImage`.
- 연결할 fixture: spritedraw, shape-to-buffer, graphics, typehotspot, palettecolor, palettecolors, paletteflash, gumpglyph, gumptext, colorsort-* 등. 해당 cpppj 검사도 xUnit으로 옮긴다.
- **완료 기준:** 위 fixture가 L1 일치하고, 대표 셰이프·글자·팔레트 효과의 캡처가 cpppj와 차이 0이다.

**R3 — Gump UI와 메인 메뉴 계열 (W15) → 이정표 D1: 메뉴 화면 픽셀 일치**

- 옮길 것: Gump 프레임워크 전체(`Gump`·`GumpTree`·`GumpCreate`·`GumpLifetime`·`GumpEvents`·`GumpInput`·`GumpFrame`·`GumpDraw`·`GumpCollection`·`GumpVisual`·`GumpBackground*`·`GumpDecoration`·`GumpHelp`·`GumpPointerBoundary`·`GumpFormBinding`), 버튼·돌 버튼·메뉴·스크롤·텍스트·텍스트 입력·HTML 창·메시지·이미지·배경 GIF 위젯, `UberGump`·`UberRootGump`·`RawUberRootControls`·`NativeMainMenuState`, 로딩 화면(`MissionLoadingMessage`·`RawMenuLoading`), Tell·Dialog·StyleText·Markup·QuickHelp, 기본 커서(`Cursor`).
- 범위: 메인 메뉴 8버튼, Campaign/Demo/Help/Options(해상도·음량·Window Mode)/Credits/Tips/Quit, 캠페인 목록, 브리핑 창.
- **완료 기준:** 같은 ui-script로 각 화면·하위 메뉴·Help 쪽·Credits 쪽을 캡처해 cpppj와 차이 0이다. 이때부터 메뉴는 새 경로가 기본이 되고, 메뉴에서 미션에 들어가는 경로는 R5 전까지 기존 경로를 쓴다(0.8 1번).

**R4 — 월드 세션 조립 (W02·W03 잔여, W04~W07) → 이정표 M1: 시작 상태 L2**

- 옮길 것: 시간·Kernel·Process·Regular, SP 저장소·보상·난수의 실제 연결, Fort 적재(객체·월드·메타·돈·소유자 마감·TypeNames·Deck), 지형·섬·농지·영역, 다리 연결, Player·기술·색, 미션 Start·로그인·타이머·Guide, AI 생성과 초기 갱신, 생산 Deck.
- `RawSurfaceWorld`/`RawWorldCore`와 `RawCampaignSession`의 소유 구조를 그대로 옮기고, 기존 `Original` 모듈을 그 안에 끼운다.
- **완료 기준:** 34경로 각각의 Load/Start 뒤 상태가 cpppj와 같다. cpppj에 Session 덤프 명령이 없으면 사용자에게 cpppj 쪽 추가를 요청하고, 그동안은 `--dump-world` 범위로 제한해 기록한다.

**R5 — 월드 표시·카메라·HUD (W14 나머지, W15의 HUD) → 이정표 D2: 진입 화면 픽셀 일치**

- 월드 표시: `Renderer`·`DrawSort`·`DrawSubmission`·`RawRenderCollection`·`ObjectCollection`·`TileGroupCollection`·`PreviewCollection`·`PreviewDraw`·`SquidRenderer`, 지형 바인딩, 타입별 Draw(신전·받침·Factory·다리·레인 볼텍스·일반 자산·덱·연소), 시각 시스템(Burning·Construction·Dissolve·FieryTail·Lightning·Shrapnel·Smoke·RenderProcess), KenCloud, 파티클, 번개 화면, Fort 팔레트 전환.
- 카메라: `RawCameraView`·`RawCameraCommit`·`RawCameraPresent`·`CameraHome`·`UberGumpCamera`·`RawEdgeScroll`.
- HUD: `UberHud`·`HudDock`·`HudNsInfo`·채팅 표시, 미니맵(`MinimapGeometry`·`RawMinimapRaster`·`RawMinimapControls`), `FloatingSpGump`·`ProductionHintGump`, 덱(`InventoryGump`).
- **완료 기준:** 34경로의 진입 직후와 N프레임 뒤 캡처가 cpppj와 차이 0이다. 애니메이션 장면은 같은 `--set`(maxFPS·sleepPerLoop 등)과 고정 시계 조건에서 비교하고, 조건을 맞출 수 없는 장면은 비교 범위를 기록한다. 이 시점에 미션 실행의 기본도 새 경로로 바꾼다.

**R6 — 플레이 입력·규칙·소리 교체 (W08~W13, W16, W19의 AI) → 이정표 M2·M3**

- 입력·선택·커서: `UserInput`·`GameWorldInput`·`RawUserPick`·`RawObjectHit`·`RawUserSelection`·`RawUserHighlight`·`RawCampaignCursor*`·`RawCursor*`.
- 게임 메뉴: QWASZX(`RawProductionControls`·`ProductionShortcuts`), ESC 가로·하위 메뉴(`RawGameMenuBar`·`RawGameMenus`·`GameMenuActions`), Player List·F9, 우클릭 메뉴 전체(`RawObjectMenu*`·`RawKnowledgeMenu`·`RawFactoryMenu`·`RawPriestMenu`·`RawPlayerMenu`·`RawCarrierMenu`), Warning·MessageBoard.
- 규칙: 사제·경로·운반·수확, 배치·건설·생산, 전투·방어·파생 건물, 의식·신전·제단, 미션 진행·안내·승패, 공통 전략 AI(`RawAi*`). AI는 cpppj에서 실제 세션 연결까지 끝났으므로 W19 "대기"에서 이식 대상으로 바꾼다. 단 cpppj도 정상 간격의 AI 행동을 자연 플레이로 대조하지는 않았다.
- 소리: 소리 Process, 음악 선택·흐름, 장면 음악. 장치는 MonoGame.
- **완료 기준:** 고정 입력열의 L3 상태 해시와 대표 장면 캡처가 일치한다. 1-1의 건설 파이프라인이 원본 경로로 동작한다(M2). 1-1·1-2가 새 경로에서 진행되면 기존 경로와 그 검사를 삭제한다.

**R7 — 미션 전체·Edit·설정·배포 (W17·W18, 9-1 C·D·F, Edit) → 이정표 M4**

- 34경로 전체, 데모 3개의 반복, 진행 저장과 다음 미션 해제, Retry/Next/Abort를 옮긴다.
- Edit(편집 메뉴·섬 추가·저장·Test Battle·SaveGoMain, `.english` 등 언어 파일)를 옮긴다. AGENTS.md상 Edit 제외는 webpj에만 해당한다.
- `options.cfg`를 원본의 시점·형식으로 읽고 쓴다(9-1 C). 단 전체화면 전환 뒤 재실행 오류는 허용하지 않는다. 9-1 D·F를 판정하고 Windows 10/11 실기에서 확인한다. 이 단계의 저장 위치는 원본과 같은 게임 폴더 `d/`이며, `%USERPROFILE%\Saved Games\Netstorm\Saves\`로의 이동은 AGENTS.md 5차 요구로 R8에서 한다. Edit 맵 목록의 원문 50개 상한도 5차까지 원본대로 둔다.
- 배포 스크립트를 갱신하고(실행 파일 이름은 `Netstorm.exe`, 0.9), 압축 해제한 사본에서 실행되는지 확인한다.
- **완료 기준:** 34경로 짧은 진입과 핵심 장면이 cpppj와 일치한다. 자연 완주 L5는 보류가 풀린 뒤 진행한다.

**R8 — 요구사항 확장(① 완료 뒤, W20)**

- 한국어: D2Coding 글리프를 8비트 화면에 그리는 방식(단색 또는 팔레트 단계)을 정해 구현한다. 기존 한국어 명칭 규칙은 그대로 따른다.
- 60/120프레임, Linux(DesktopGL), 그 뒤 TCP/IP 로컬 멀티플레이·Outpost(최후순위)를 진행한다.
- AGENTS.md 4~6차(2026-10-30 반영): 4차 MCP(해석은 0.8 6번), 5차 설정·캠페인 진행 저장을 `%USERPROFILE%\Saved Games\Netstorm\Saves\`로 옮기고 커스텀 맵을 기존 폴더와 `...\CustomMaps\` 양쪽에서 불러오며 Edit 신규 저장은 후자로 하고 Edit 맵 목록에 스크롤과 개수 무제한을 넣는다, 6차 한국어(위 첫 항목)다. 현재 기존 경로는 자체 `settings.json`(`DisplaySettings`)을 쓰고 새 경로는 R7에서 `options.cfg`로 옮기므로, 5차는 R7의 저장 구조 위에서 위치만 바꾸는 순서로 진행한다.

### 0.6 검증 단계와 기록 규칙

- **L1:** 원본 x86 fixture(기존 방식). **L2:** 같은 미션 Load/Start 상태가 cpppj와 같다. **L3:** 같은 고정 시계·입력열로 N프레임 진행한 상태 해시가 같다. **L4a(신규):** 같은 ui-script의 논리 캡처가 픽셀 차이 0이다. **L4b:** 소리 시점과 곡 선택이 같다. **L5:** 자연 완주·저장·재실행. 현재 공통 보류 결정에 따라 L5는 보류한다.
- cpppj 캡처는 오라클 SHA·스크립트·`--set` 값·프레임과 함께 기록한다. cpppj가 논리 캡처(`snapshot`)와 실제 출력 캡처(`snapshot-output`)를 구별하듯 dotnetpj도 둘을 구별한다. 실행에는 `originals/`가 아니라 `extracted/` 아래의 데이터 사본을 쓴다.
- 한 단위를 마칠 때 기록할 것: 옮긴 cpppj 파일과 기준 커밋, 연결 fixture와 행 수, 이식한 cpppj 검사 수, 은퇴시킨 기존 코드와 검사, 비교한 캡처 수와 차이, 남은 경계. L1 통과를 L2 이상으로 합산하지 않는다. 기존 근사 검사의 은퇴로 실패를 감추지 않으며, 원본 fixture 기대값을 C# 결과로 바꾸지 않는다.
- 클론·cpppj 창 실행은 AGENTS.md의 확인·예외 규칙을 따른다(VM-W11-Codex·HJOW-Athlon·vm-debian-codex는 예외 PC). 이 계획은 원본 게임을 실행할 필요가 없다.

### 0.7 계속 유효한 결정·제약

- **목표 순서:** ① 원본 10.78과 동일한 동작 → ② 요구사항(화면비·60/120프레임·다국어·재실행 오류 방지) → ③ Linux. 이미 들어간 요구사항 기능(한국어 선택·와이드·전체화면)은 버리지 않는다. 새 경로로 옮길 때 cpppj 방식(논리 해상도·호환 전체화면)으로 다시 만들고, 한국어는 R8에서 다시 만든다(0.8 2번).
- **AGENTS.md 상위 요구(2026-10-30 갱신 반영):** ① `bfccadf5`·`24959e75`: cpppj의 후순위 4~6차(4차 MCP, 5차 설정·캠페인 진행 저장을 `%USERPROFILE%\Saved Games\Netstorm\Saves\`로 이동하고 커스텀 맵을 기존 폴더와 `%USERPROFILE%\Saved Games\Netstorm\CustomMaps\` 양쪽에서 불러오되 Edit 신규 저장은 후자로 하며 Edit 맵 목록에 스크롤을 넣고 출력 개수 제한을 없앤다, 6차 한국어 지원)가 **dotnetpj 후순위 요구에도 포함**된다. 원본과 같은 동작(목표 ①) 뒤에 R8에서 다룬다. ② `ef0c6b91`: cpppj·dotnetpj 모두 **Windows 용 게임 실행 파일 이름이 `Netstorm.exe`**여야 한다. 후순위가 아니라 배포물에 직접 보이는 요구이므로 0.8 5번과 0.9의 이름 적용 항목을 따른다. 현재 기존 게임의 실행 파일은 `NetstormClone.exe`다.
- **9-1 의도적 차이(2026-10-10 사용자 결정)의 적용:** A(시간)는 R4의 Kernel/Regular, B(입력 큐)는 R1, C(`options.cfg`)는 R7, D(바쁜 대기)·F(창 동작)는 R7에서 판정, E(부분 다시 그리기)는 R1·R5에서 처리한다.
- **글꼴·명칭·주석:** 영어는 원본 `.chfnt`, 한국어는 D2Coding(3절 2번). D2Coding은 1.3.2를 유지한다(3절 7번). 기존 주석은 대상을 고칠 때 정비한다(3절 8번). AGENTS.md의 한국어 명칭 규칙을 따른다.
- **cpppj 공통 제약을 준용한다.**
  - CHEAT 항목은 Edit 편집에서만 나타난다.
  - 공격형 공중 3종(훨리기그·맨오워·더스트 데빌)은 cpppj 코드를 옮기되, 이 타입을 위한 실제 생산·생성·우클릭 재현과 추가 실행 관측은 하지 않는다.
  - 연녹색 직사각형 분석은 보류한다(원본에서도 관찰됨).
  - 대응 `.english`가 없을 때 Test Battle이 끝나는 것은 원본 동작이며 개선은 보류한다.
  - 자연 완주·사용자 개입 검증은 보류하고, 멀티플레이는 후순위다.
- **cpppj의 그래픽/UI 미완 범위는 cpppj를 따라간다:** View/zoom 전체, 도킹·미니맵 독립 토글, DD 조정·확인 UI 잔여, Credits의 nsstart 배경·20초 자동롤, Chat 입력, Parallax(FourPage0 관문으로 비활성), 메타/효과 추가 Draw, 전체 화면·음향 동일성 대조(LEFT_JOBS.md 3절 5번). cpppj에 반영되기 전에는 dotnetpj에서 추정으로 만들지 않는다.
- **근거 우선순위:** 원본 fixture → cpppj 보고서의 계약·경계 → cpppj 소스 → 녹화·스크린샷. cpppj는 HEAD만 쓰고 미커밋 변경은 쓰지 않는다. `cpppj/`·`LEFT_JOBS.md`는 수정하지 않고, 필요한 요청은 이 문서와 이식 계획 8절에 적어 사용자에게 전달한다.
- **보호·커밋:** 원본/CD·AGENTS.md는 수정하지 않는다. push는 하지 않는다. 커밋 메시지는 `YYYYMMDD-NN DN 요약`이다. 새 문서·코드는 UTF-8과 한국어 주석 규칙을 따른다.

### 0.8 사용자 확인이 필요한 것 (확인 전까지 에이전트 기본값으로 진행)

1. **기존 경로 처리(결정 5):** R3부터는 메뉴, R5부터는 미션의 기본 실행이 새 경로로 바뀐다. 기존 1-1·1-2 근사 플레이는 `--legacy`로만 남기고 R6 뒤 삭제한다. 그동안 새 경로에서 플레이할 수 있는 범위가 일시적으로 줄 수 있다.
2. **한국어(0.7):** 새 경로는 R8 전까지 영어 원본 화면만 제공하고, 한국어 선택은 `--legacy`에서만 쓸 수 있다. 한국어가 계속 필요하면 D2Coding 단색 글리프 출력을 R3으로 앞당긴다.
3. **cpppj 미복원 경로(결정 6):** 새 경로는 cpppj와 같은 명시 오류·경계를 쓴다. 이전 3절 9번의 "근사 유지" 기본값은 기존 경로에만 남는다.
4. **프로젝트 배치(결정 3):** `Netstorm.Client`를 새로 만든다. 다른 이름이나 기존 Core 안의 이름공간을 원하면 R1 착수 전에 정한다.
5. **`Netstorm.exe`를 어느 실행 파일이 갖는가(AGENTS.md ef0c6b91):** dotnetpj에는 실행 파일이 둘이다. 기존 게임 `Netstorm.Game`(`NetstormClone.exe`, 배포 대상)과 새 경로의 개발 호스트 `Netstorm.Client.Host`(`Netstorm.Client.Host.exe`, 아직 배포하지 않음)다. 한 배포 폴더에 `Netstorm.exe`는 하나뿐이다. **기본값:** 지금은 배포되는 `Netstorm.Game`의 `AssemblyName`만 `Netstorm`으로 바꾼다. 결정 5의 기본 실행 교체 때(R3 메뉴·R5 미션) `Netstorm.exe`가 새 경로로 진입하게 하는 방법은 둘이다. A) `Netstorm.Client.Host`가 `Netstorm.exe`를 이어받고 기존 Game은 `--legacy` 전용 이름(예: `NetstormLegacy`)으로 내리거나 새 호스트에 흡수한다. B) `Netstorm.Game`이 계속 `Netstorm.exe`이고 새 경로를 라이브러리로 호출한다. 어느 쪽이든 사용자가 보는 이름은 `Netstorm.exe` 하나다. 원하는 방식이 있으면 R3 착수 전에 알려 달라.
6. **4차 MCP의 dotnetpj 해석(AGENTS.md bfccadf5):** AGENTS.md는 'cpppj의 4~6차 목표 내용이 dotnetpj 후순위 요구사항에도 포함된다'고만 적었다. 4차 MCP는 dotnetpj 개발을 위해 게임을 분석하는 용도라서, dotnetpj 클론에도 같은 MCP를 붙이라는 뜻인지 cpppj 쪽 MCP를 개발에 활용하라는 뜻인지 문장만으로는 정해지지 않는다. **기본값:** R8 전에는 구현하지 않고 cpppj의 MCP 결과가 나온 뒤 사용자에게 확인한다.

### 0.9 바로 다음 작업

- [x] **R0-1 (2026-10-28 완료):** cpppj를 HEAD `0cc1d33d`로 증분 재빌드해 오라클을 확정했다. 기존 `NetstormCpp.exe`(09:00 빌드)는 09:15 병합 소스보다 오래돼 쓰지 않았다. 새 SHA-256은 `63DF9A10DD9569FDF7DBE8B6D977D8287BFE898632E0CCC7499B0A0BAD27B8FE`(09:30:52 링크)이고 `--dump-types originals` 188행·`--dump-world originals thewarbegins` 3,509행·종료 0을 확인했다. dotnetpj는 Release 경고·오류 0, 검사 **1,349개(Assets 363 + Core 986) 통과**로 10-23 기록과 같다.
- [x] **R0-2 (2026-10-28 완료):** [재대조 영향표](docs/dotnet-cpp-resync-20261028.md). 연결 fixture 61개는 모두 일치(반영 이후 fixture 추가 215·수정 0·삭제 0)하지만 근거 소스가 바뀐 C# 파일이 30개이고, 그중 6개 묶음은 현재 C#이 이전 cpppj 동작에 머물러 있음을 소스로 확인했다.
- [x] **R0-3 (2026-10-28 완료):** `cpp_sync.py --ports`·`--ports-init`·`--ports-reflect`와 [PORT_MAP.tsv](dotnetpj/PORT_MAP.tsv)(68행)·[PORT_MAP.md](dotnetpj/PORT_MAP.md), 형식·경로 검사 `CppPortMapTests`(2개, 오류 행을 넣으면 실패함을 확인)를 만들었다. 검사 수는 1,351개가 됐다.
- [x] **R0-4 — 새 fixture로 뒤처진 C#을 고친다 (2026-10-28 완료):** 영향표 A의 여섯 묶음을 `sp-add-bits`·`sp-initialization`·`fort-sp-write` → `squid-retype`·`object-conversion` → `squid-pool-reset`·`state-sid-lists` → `priestregen`·`priest-panic`·`priest-prayer` → `RawGraph` 비표면 좌표 → `RawAssetPostPop` 4종 순으로 처리한다. 묶음마다 fixture `Link`+`cpp_sync.py --update --add`, C# 수정, `--ports-reflect`를 하고 단위 기록을 남긴다. 기대값을 C# 결과로 바꾸지 않는다. `sp-add-bits`·`sp-initialization`은 [0.14](#014-단위-기록--r0-4-sp-묶음-가산-비트와-초기화-명시-할당-2026-10-28-hjow-athlon)에서, `fort-sp-write`는 [0.15](#015-단위-기록--r0-4-sp-묶음-fort-sp-writer-신규-이식-2026-10-28-hjow-athlon)에서, `squid-retype`·`object-conversion`은 [0.16](#016-단위-기록--r0-4-두-번째-묶음-타입-변경과-비석주문-전환-2026-10-28-hjow-athlon)에서, `squid-pool-reset`·`state-sid-lists`는 [0.17](#017-단위-기록--r0-4-세-번째-묶음-풀-정리와-sid-목록-2026-10-28-hjow-athlon)에서, `priest-panic`·`priest-prayer`와 회복 허용 공유 판정은 [0.18](#018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon)에서, `priest-regen` 재생과 기록 정정은 [0.19](#019-단위-기록--r0-4-네-번째-묶음-후속-사제-회복-재생과-기록-정정-2026-10-28-hjow-athlon)에서, `RawGraph` 비표면 좌표는 [0.20](#020-단위-기록--r0-4-다섯-번째-묶음-그래프-비표면-좌표-2026-10-28-hjow-athlon)에서, `RawAssetPostPop` 4종은 [0.21](#021-단위-기록--r0-4-마지막-묶음-자산-후처리-4종-2026-10-28-hjow-athlon)에서 완료했다. R0-4는 끝났다.
- [x] **R1-1 (2026-10-28 완료, 라이브러리 부분):** `Netstorm.Client` 프로젝트와 `Netstorm.Client.Tests`를 만들고 `Screen`(8비트 버퍼·팔레트·잠금·자르기)·`GamePalette`·`PaletteSearch`·`ScreenGeometry`·`ScreenPresentation`·`ScreenResolutions`·`ScreenPaletteApply`·`InputQueue`를 이식했다. 원본 fixture 3개(786·357·166행)가 L1로 일치하고 전체 1,384개 통과·경고 0. [문서](docs/dotnet-client-screen-20261028.md). 이 단위는 규칙의 이식이며 실제 화면 픽셀 비교가 아니다.
- [x] **R1-2a (2026-10-28 완료):** MonoGame 출력 호스트(`Netstorm.Client.Host` 실행 + `Netstorm.Client/Host` 순수 부분): `IScreenOutput` 구현(변경 사각형만 RGBA 부분 업로드), 정수 최근접 확대·검정 여백, 전체화면 borderless 창과 F11 전환, 마우스 역변환·전체화면 커서 제한, 키·글자·버튼 → `InputQueue`, 프레임 루프 골격(`--frames`·`--ui-script`·`--ui-report`·`--set`). 기존 `NetstormGame`/`DisplayManager`는 동결하고 별도 진입으로 둔다(결정 5). [문서 6절](docs/dotnet-client-screen-20261028.md#6-r1-2a-monogame-출력-호스트-같은-날-hjow-athlon). **남은 것:** 그리기 내용·세션·소리(R2·R3·R4 이후), 라벨 클릭·report·kernel-tell(R3 이후).
- [x] **R1-2b (2026-10-28 완료, 호스트 없이 가능한 부분):** `ScreenCapture`(cpppj와 byte 동일한 BMP)·`UiScript`/`UiScriptRunner`(esc·key·keyup·pointer·좌표/라벨 클릭·snapshot)·`dotnetpj/tools/cpp_frame_compare.py`(파일·폴더 비교, 종료 코드 0/1/2). 오라클 캡처(메인 메뉴 프레임 30)가 이 PC에서 결정적으로 만들어짐을 확인했다. [문서 5절](docs/dotnet-client-screen-20261028.md#5-r1-2-부분-캡처스크립트-도구와-오라클-확인-같은-날). **남은 것:** `--ui-script`/`--mission`/`--frames`/`--set` 실행 인자를 받는 dotnetpj 실행 진입과 그 안에서 두 도구를 부르는 프레임 루프는 R1-2a의 호스트가 있어야 하며, 라벨 클릭·report·kernel-tell 등은 R3 이후다.
- [ ] **R2 — 그리기 원시·팔레트·글꼴 (진행 중: R2d-3 일반 객체 그리기 완료):** `VFXDraw`·`VFXMappedDraw`·`VFXLine`·`VFXTransform`, `RawShapeToBuffer`, `SpriteDraw`, `*ColorMaps`, `Palette*`, `RawColorSort*`, `BitmapFont`, `GifImage`, `RawImage`를 `Netstorm.Client/Display`로 옮긴다. 이때 `Screen.LoadPalette`의 색 표 계산(`BuildPaletteColorTable`·`BuildGumpShadeMaps`)을 연결한다(R2a에서 완료). 묶음마다 fixture 연결·C# 수정·`--ports-reflect`·단위 기록을 남긴다. R2a는 [0.23](#023-단위-기록--r2a-팔레트-3종과-loadpalette-연결-2026-10-28-hjow-athlon)에서, R2b는 [0.24](#024-단위-기록--r2b-vfx-그리기-원시-2026-10-28-hjow-athlon)에서, R2c는 [0.25](#025-단위-기록--r2c-색-표-2026-10-28-hjow-athlon)에서, R2d-1은 [0.26](#026-단위-기록--r2d-1-변환합성이미지시작-2026-10-29-hjow-athlon)에서, R2d-2는 [0.27](#027-단위-기록--r2d-2-판본표시자료캡처-2026-10-29-hjow-athlon)에서 완료했다. R2d-3 본체와 원본 명령열 1,389행 대조는 0.28에서 완료했다. 실자산/세션 연결은 R4·R5에 남는다. `GifImage` 적응·`BitmapFont` 적응은 L1에 UI 인프라(메뉴·Gump·글꼴 저장소)가 필요해 R3 Gump 이식과 함께 다룬다.
- [ ] **현재 R3k(2026-10-29):** 한 줄/다중 줄 조각·버퍼/그림/실제 글꼴·SHP 픽셀/스크롤 표시는0.38~0.39, 대괄호/다중 줄 파서와 버퍼 수명은0.40, 실제 Slider/자식은0.41, 다중 줄 Widget은0.42에서 연결했다. 메뉴 기본/항목 소유/레이아웃은0.43, 제어부는0.44, 장식 배경은0.45, 실제 메뉴 생성/수명·공용 SID Form은0.46에서 연결했다. 출력은0.47, 실제 사건/캡처·선택/명령·하위 메뉴 입력은0.48에서 연결했다. 문자 편집 버퍼/실제 소유 저장소는0.49, 문자 입력창/Console과 실제 공통 트리·Slider/글꼴/수명 연결은0.50에서 작성했다. 이미지 UI의 SHP/색표/16.16/투명 적중은0.51에서 연결했다. 동적MetaImage의 생성/기하/실제출력은0.52에서 연결했다. 실제MetaImage 목록의 배치/선택/삭제는0.53에서 연결했다. 배경 GIF/실제 원시 버퍼와 자산은0.54에서 연결했다. HTML 본문/조건/그림/배경은0.55에서 연결했다. HTML 링크/QuickHelp 파생 본문은0.56에서 연결했다. HTML 부모/내장 Tell과 실제 스크롤 자식/링크 수명은0.57에서 연결했다. 도움말 열기/요청·이력/입력 제어는0.58에서 연결했다. 전용 도움말 창 생성/배치/본문·그림은0.59에서 연결했다. 일반 HTML 창의 수명/본문/전체 배치는0.60에서 연결했다. HTML 메타/전체 생성기·실제 링크/모달과 하위 UI는0.61에서 연결했다. 명령/Config 적용은0.62에서, 설정 객체 층은0.63에서, ConfigInterface 본체는0.64에서 연결했다. 재개 후 MissionScript 상당·대화창 분리·목록 owner·메시지/대화창·상위 메뉴/전체 자산/호스트를 조립한다. R3 전체 완료로 세지 않는다.
- [ ] 이후 R3(D1) → R4(M1) → R5(D2) → R6 → R7 → R8 순서로 진행하고, 단위마다 이 절의 체크와 [0.4](#04-기존-dotnetpj-소스의-재수정-판정) 판정표를 갱신한다.
- [ ] **`Netstorm.exe` 이름 적용(사용자 AGENTS.md ef0c6b91, 2026-10-30 신규·미착수. 0.8 5번 확인 전에는 기본값으로 진행 가능):** 재개 뒤 다음 배포본을 만들기 전에 처리하는 것을 권장한다. 문서 작업 때 확인한 영향은 다음과 같다. (a) 정의: `dotnetpj/src/Netstorm.Game/Netstorm.Game.csproj` 6행 `<AssemblyName>NetstormClone</AssemblyName>`(WinExe)을 `Netstorm`으로 바꾸면 `Netstorm.exe`·`Netstorm.dll`·`Netstorm.deps.json`·`Netstorm.runtimeconfig.json`이 나온다. 같은 어셈블리 이름이 Linux 산출물(`NetstormClone` → `Netstorm`)에도 적용되나 이 이름 요구는 Windows 용에 대한 것이다. `src/*.cs`에서 어셈블리 이름 문자열을 쓰는 코드는 찾지 못했고 `AssemblyTitle`은 빌드가 자동 생성한다. 다른 어셈블리(`Netstorm.Core.dll` 등)와 파일 이름이 겹치지 않는다. (b) 배포: `dotnetpj/package-dotnetpj.ps1`의 `$ExeName`(80행)·필수 파일 목록(96행의 `NetstormClone.exe/.dll/.deps.json/.runtimeconfig.json`)·머리 설명(7행)·압축에 들어가는 안내문(388~404행의 실행 예시)을 새 이름으로 바꾸고, 새 ZIP 생성과 압축 해제본 실행으로 다시 검증한다. (c) 문서·도구: README.md/README.en.md의 빌드·실행 절(`NetstormClone.exe`·`dist/win-x64/` 폴더 구조·Linux `NetstormClone`), `docs/`의 4개 문서(`cpp-build.md` 93행 비교표 포함), `tools/`의 클론 스모크 스크립트 약 16개(`clone_*_smoke.ps1/py`)가 기존 이름을 참조한다. 과거 기록은 당시 이름으로 보존하고 계속 쓰는 도구만 바꾼다. (d) `Netstorm.Client.Host`는 `Netstorm.Client.Host.exe`(AssemblyName 미지정)라 이름이 달라 지금은 충돌하지 않는다. 기본 실행을 새 경로로 바꾸는 시점의 정리는 0.8 5번을 따른다. (e) 원본 `originals/Netstorm.exe`와 이름이 같다. 배포 폴더에는 원본 실행 파일을 넣지 않고, 프로세스 이름이 같아지므로 도구는 전체 경로로 구분하며 실행 확인 규칙에서도 어떤 실행 파일인지 경로로 밝힌다. (f) 검증: 이름 변경은 동작 변경이 아니므로 Release 빌드·전체 검사(`dotnet test`)와 압축 해제본의 짧은 실행이면 되고 게임 실행은 AGENTS.md의 실행 확인·예외 규칙을 따른다.

### 0.10 이번 문서 작업 기록

- 확인한 것: AGENTS.md, LEFT_JOBS.md 전체, 이 문서의 현행 계획·9~13절·구 개요, [이식 계획](docs/dotnet-cpp-port-plan-20261016.md), [현대 전체화면](docs/exe/cpp-modern-fullscreen-20261027.md), cpppj `src/o`·`src/client` 모듈 목록과 영역별 줄 수, `app/main.cpp`의 `--run`·`--ui-script`·`snapshot` 처리, `Client::Capture`, dotnetpj 프로젝트·파일·줄 수·MonoGame 그리기 경로, `cpp-fixture-manifest.tsv`의 반영 커밋, cpppj fixture 목록. 현재 호스트는 `VM-W11-Codex`다.
- 실행하지 않은 것: 빌드·검사·cpppj/클론/원본 게임·창·디컴파일. 위 수치 중 검사 수 1,349는 10-23 기록이고, 243커밋·669 fixture는 이번에 `git`과 파일 목록으로 센 값이다.
- 사용자 추가 요청("더 이상 의미가 없어진 내용은 LEFT_JOBS.dotnetpj.old.md를 만들어 옮기고, 최하단에 링크와 이관 사실을 명시")에 따라 문서를 나눴다. 원문 3,721줄의 비어 있지 않은 줄이 모두 두 파일 중 하나에 있는지 대조했다. 원문과 다른 줄은 의도적으로 바꾼 제목 4개와 문서 구성 안내 1줄뿐이다. 파일을 넘나드는 링크 12개의 대상 파일을 고쳤다. 세 변경 파일의 로컬 링크 690개를 검사했으며, 남은 오류는 HEAD 원문에 이미 있던 1개(`docs/map-viewer.md`의 바뀐 옛 제목 앵커)뿐이다. 원문 보존을 위해 이관 파일에서도 그대로 두었다.
- 변경 파일은 이 문서, 새 `LEFT_JOBS.dotnetpj.old.md`, 이식 계획 문서(머리 안내 1줄)다. `20261028-19 DN cpppj 그래픽 기준 dotnetpj 재구성 계획과 인계 문서 이관`으로 로컬 커밋했다(`11774e6f`). push는 하지 않는다.

### 0.11 단위 기록 — R0 기준선·영향 목록·포트 맵 (2026-10-28, VM-W11-Codex)

- **착수 상태:** 사용자 요청 "dotnetpj 작업 진행해 줘". 시작 HEAD는 `0cc1d33d`(원격 병합으로 `a3ad9431 20261028-19 CP`가 들어와 있었다). 작업 트리는 깨끗했고 게임·창은 실행하지 않았다. 빌드·검사·헤드리스 오라클 명령만 실행했다.
- **변경 파일:** `dotnetpj/tools/cpp_sync.py`(포트 맵 3개 명령), `dotnetpj/PORT_MAP.tsv`·`PORT_MAP.md`(신규), `dotnetpj/tests/Netstorm.Core.Tests/CppPortMapTests.cs`(신규), `docs/dotnet-cpp-resync-20261028.md`(신규), 이 문서. cpppj·원본·CD·AGENTS.md·`cpp-fixture-manifest.tsv`의 reflected-commit(`f95a8db2`)은 바꾸지 않았다. 영향 검토가 끝나지 않았으므로 올리지 않는다.
- **기록해 둘 것:**
  - Git Bash에서 `cmake --build … -- /p:…`를 실행하면 `/p:`가 경로로 바뀌어 MSBuild가 즉시 실패한다(소스·산출물 변화 없음). 이 PC에서는 PowerShell로 실행한다. 첫 시도 로그는 덮어썼다.
  - `cpp_sync.py`의 보고는 이 PC 콘솔에서 cp949로 출력되어 한글이 깨진다. `PYTHONUTF8=1`을 설정하거나 `python -X utf8`로 실행한다.
  - cpppj 전체 빌드에는 `netstorm_tests`(3,358개 등록)가 포함되어 오래 걸린다. 오라클은 `NetstormCpp.exe`만 필요하며 이번에 cpppj 검사를 실행하지 않았다. cpppj 쪽 검사 결과를 이 단위의 근거로 합산하지 않는다.
  - 처음 커밋에서 `-c core.autocrlf=false`를 써서 LF 저장 문서가 CRLF로 들어갔던 일을 `git add --renormalize`로 정정했다. 이 저장소의 `dotnetpj`·`docs`는 LF로 저장된다. 기본 `autocrlf`로 add 한다.
- **원시 자료(Git 제외, 이 PC):** `extracted/dotnet-r0-baseline/` — `dotnet-test.log`, `cpp-sync.txt`, `cpp-build.log`, `impact.md`, `dump-types.tsv`, `dump-world-thewarbegins.tsv`, `new-fixtures.txt`, `cpp-changed-files.txt`, `PORT_MAP.backup.tsv`.

### 0.12 단위 기록 — R1-1 Netstorm.Client 화면·입력 계층 (2026-10-28, VM-W11-Codex)

- **변경:** 새 프로젝트 `dotnetpj/src/Netstorm.Client`(9개 C# 파일)와 `dotnetpj/tests/Netstorm.Client.Tests`(3개 파일), `Netstorm.sln` 갱신, `cpp_sync.py`(`Client` 사용 프로젝트 허용, `--ports-add`), `CppFixtureManifestTests`(Client 허용), `cpp-fixture-manifest.tsv`(3개 등록), `PORT_MAP.tsv`(11행 추가), `PORT_MAP.md`·`docs/dotnet-client-screen-20261028.md`·`docs/dotnet-cpp-resync-20261028.md`. cpppj·원본·AGENTS.md·reflected-commit은 바꾸지 않았다.
- **검증:** [문서 3절](docs/dotnet-client-screen-20261028.md#3-검증). 전체 1,384개 통과(R1-1 시점), Release 경고·오류 0. fixture 검사의 판별력을 변이 2건으로 확인했고 최초 실패 1건은 테스트의 잘못된 기대값이었다.
- **동시 작업 주의:** 다른 PC·세션이 같은 저장소에 cpppj 커밋을 계속 병합하고 있다(`0cc1d33d`→`ac9ea916`: CP `20261028-20`·`-21`). 그 결과 커밋 번호 `20261028-19`·`-20`은 CP와 DN이 각각 하나씩 썼다. 번호는 구분용이므로 그대로 두고, 다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다. 병합 때문에 R0의 오라클 실행 파일(`0cc1d33d` 빌드)은 이미 HEAD보다 오래됐다. 화면 캡처 비교(R1-2b) 전에 다시 빌드한다. 변경 쌍은 35개로 늘었다([영향표 2.1](docs/dotnet-cpp-resync-20261028.md)).
- **다음:** R1-2a(MonoGame 출력 호스트) → R2. R0-4(뒤처진 C# 수정)는 `Original` 모듈이 세션에 연결되는 R4 전에 끝내면 되므로 R1~R2와 번갈아 진행할 수 있다.

### 0.13 단위 기록 — R1-2 캡처·스크립트 도구와 오라클 확인, 작업 중지 인계 (2026-10-28, VM-W11-Codex)

- **중지 사유:** 사용자가 "문서 갱신 및 커밋하고 그 이후 일단 작업 중지"를 요청했다. 이 단위까지 커밋하고 멈춘다. 중지 시점에 진행 중인 빌드·게임·검사 프로세스는 없다(`NetstormCpp.exe` 없음을 확인했고 마지막 cpppj 빌드는 종료 0으로 끝났다).
- **변경:** `Netstorm.Client`에 `Display/ScreenCapture.cs`, `Automation/UiScript.cs`, 검사 `CaptureAndScriptTests`(12개), `dotnetpj/tools/cpp_frame_compare.py`, `PORT_MAP.tsv`(3행 추가, 82행), `docs/dotnet-client-screen-20261028.md` 5절. cpppj·원본·AGENTS.md·`cpp-fixture-manifest.tsv`의 reflected-commit은 바꾸지 않았다.
- **검증:** Client 45개를 포함한 전체 1,396개 통과. 캡처 BMP의 머리 122바이트가 cpppj의 실제 캡처와 byte 동일함을 확인했다(화면 픽셀 일치는 아님). 도구는 일치·차이·입력 오류·폴더 모드를 직접 실행해 확인했다.
- **오라클 사용 환경(재개 때 그대로 쓸 수 있다):** 데이터 사본 `extracted/dotnet-oracle-data/`(`originals/` 1,394파일 복사, 실행 중 설정은 사본에만 쓴다), 캡처·로그 `extracted/dotnet-oracle-captures/`, 기준선·영향 원시 `extracted/dotnet-r0-baseline/`. 모두 Git 제외이며 다른 PC에는 없다. 오라클 `NetstormCpp.exe`는 HEAD `0cc1d33d`로 빌드(SHA-256 `63DF9A10…`)했다. 그 뒤 병합된 cpppj 변경은 화면 계층과 무관하나 다음 캡처 비교 전에 `cmake --build`로 다시 빌드한다(Git Bash가 아니라 PowerShell에서, 26분 걸린 전체 빌드였고 증분은 더 짧다). 실행 전후 `originals/` 해시가 같음을 확인했다. 원본 해시 기준선은 `extracted/dotnet-r0-baseline/originals-hash-before.txt`다.
- **재개 순서(권장):**
  1. `git pull`로 새 cpppj 커밋을 받은 뒤 `python dotnetpj/tools/cpp_sync.py`·`--ports`를 실행해 변경 쌍 수(현재 35개)와 새 fixture를 확인한다.
  2. **R1-2a:** `Netstorm.Game`에 새 MonoGame 호스트(기존 `NetstormGame`은 동결)를 만든다. `IScreenOutput` 구현(인덱스→RGBA, `ScreenPresentation.ScaleScreenPresentation`로 CPU 정수 확대 후 업로드), 마우스·키 → `InputQueue`, `UiScriptRunner`를 프레임 루프에 연결, `--run/--window/--frames/--set/--ui-script/--ui-report` 진입. 시험 패턴을 그려 cpppj 출력 변환 검사와 같은 경계(행 간격 1368, 4:3/16:9/16:10)를 실제 창에서 확인한다.
  3. **R2:** 그리기 원시·팔레트·글꼴(`PORT_MAP.md` 4절). 이때 `Screen.LoadPalette`의 색 표 계산(`BuildPaletteColorTable`·`BuildGumpShadeMaps`)을 연결한다.
  4. **R0-4:** 뒤처진 `Original` 모듈 수정은 R4 전에 끝내면 되며 R1~R2와 번갈아 할 수 있다. 영향표 A 목록을 따른다.
- **알아 둘 것:** 이 저장소에는 다른 PC·세션이 cpppj 커밋을 계속 병합한다(커밋 번호 `-19`·`-20`은 CP와 DN이 각각 썼다). 다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인한다. dotnetpj·docs 문서는 저장소에 LF로 저장되므로 `git -c core.autocrlf=false`를 쓰지 않는다.

### 0.14 단위 기록 — R0-4 SP 묶음: 가산 비트와 초기화 명시 할당 (2026-10-28, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘". 시작 HEAD는 `3d038e51`(DN `20261028-23` 이후 CP `24`~`27`·WB `28` 병합)이며
  작업 트리는 깨끗했다(병렬 cpppj 세션의 변경은 이 단위 착수 전에 정리됐다). 게임·창 실행과 디컴파일은 하지 않았다.
- **변경:** `RawSquidReward.cs`(`SpMirrorFtol`·`BindPatchWallet`·저장값 유한 검사 제거),
  `RawSpStore.cs`(명시 할당 공급자 생성자·행 단위 발행·중단 유지·첫 행 뒤 `FirstPool`),
  `X86RawSquidRewardTests.cs`(`SpAddBitsMatchNative` 504행·기존 비유한 저장값 기대 수정),
  `X86RawSpStoreTests.cs`(`SpInitializationMatchNative` 48행),
  `Netstorm.Core.Tests.csproj`(두 fixture `Link`), `cpp-fixture-manifest.tsv`(66개, reflected-commit 유지),
  `PORT_MAP.tsv`(두 파일 base를 HEAD로, 바뀐 쌍 35→33개), `PORT_MAP.md`(5절 수치 갱신),
  `docs/dotnet-r04-sp-bits-20261028.md`(신규), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다.
- **검증:** Release 경고·오류 0, 전체 **1,398개(Assets 363 + Core 990 + Client 45) 통과**.
  변별력은 [단위 문서](docs/dotnet-r04-sp-bits-20261028.md#3-검증)에 기록했다.
- **남은 것:** 같은 묶음의 `fort-sp-write`(`RawFortSpWrite` 신규 이식 + `PORT_MAP.tsv` 행 추가 + 198행 L1)가 다음 순서다.
  공유 전역 소유(`Percent`·`GeyserPool`·`PriestQuarterHp`, `TrackingOffsetCurrent` 쓰기 참조)는 R4에서 연결한다.
  다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다(작업 중 `20261028-29`를 CP가 썼으므로 0.14 단위는 30번으로 커밋했다).

### 0.15 단위 기록 — R0-4 SP 묶음: Fort SP writer 신규 이식 (2026-10-28, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘". 시작 HEAD는 `91a62e79`(직전 DN `20261028-30`)이며
  작업 트리는 깨끗했다(병렬 cpppj 세션의 536 변경은 이 단위 착수 전에 정리됐다). 게임·창 실행과 디컴파일은 하지 않았다.
- **변경:** `Netstorm.Core/Original/RawFortSpWrite.cs`(신규: `Write`·`MemStreamAssertion`·`MemStreamHooks`·`FortSpWriteHooks`),
  `X86RawFortSpWriteTests.cs`(신규: 198행 재생 + 계약 3개),
  `Netstorm.Core.Tests.csproj`(fixture `Link`), `cpp-fixture-manifest.tsv`(67개, reflected-commit 유지),
  `PORT_MAP.tsv`(83번째 행 추가), `PORT_MAP.md`(5절 수치 갱신),
  `docs/dotnet-r04-fort-sp-write-20261028.md`(신규), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다. 포괄 참조(`MemStreamHooks` 할당·반납, 공유 전역 소유)는 MemStream 본체·R4 세션 조립 때 연결한다.
- **검증:** Release 경고·오류 0, 전체 **1,402개(Assets 363 + Core 994 + Client 45) 통과**.
  변별력은 [단위 문서](docs/dotnet-r04-fort-sp-write-20261028.md#3-검증)에 기록했다.
- **다음:** 영향표 A의 다음 묶음 `squid-retype`·`object-conversion`(`RawSquidFactory` Retype)이다.
  다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다(작업 중 `20261028-31`을 CP가 썼으므로 이 단위는 32번으로 커밋한다).

### 0.16 단위 기록 — R0-4 두 번째 묶음: 타입 변경과 비석·주문 전환 (2026-10-28, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘". 시작 HEAD는 `b126e900`(직전 DN `20261028-32`)이며
  작업 트리는 깨끗했다(병렬 cpppj 세션의 변경은 이 단위 착수 전에 정리됐다). 게임·창 실행과 디컴파일은 하지 않았다.
- **변경:** `RawSquidFactory.cs`(`Retype` 신설·`ConstructorVtable` 명시 오류),
  `Netstorm.Core/Original/RawObjectConversion.cs`(신규: `ObjectConversionHooks`·`RawObjectConversion`),
  `X86RawObjectConversionTests.cs`(신규: 128·36행 재생 + 계약 1개),
  `Netstorm.Core.Tests.csproj`(두 fixture `Link`), `cpp-fixture-manifest.tsv`(69개, reflected-commit 유지),
  `PORT_MAP.tsv`(84번째 행 추가·`RawSquidFactory.cs` base 갱신), `PORT_MAP.md`(5절 수치 갱신),
  `docs/dotnet-r04-retype-conversion-20261028.md`(신규), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다. `SupportsConstructedVtable`·약화 전역 공유는 R4·R6에서 연결한다.
- **검증:** Release 경고·오류 0, 전체 **1,405개(Assets 363 + Core 997 + Client 45) 통과**.
  변별력은 [단위 문서](docs/dotnet-r04-retype-conversion-20261028.md#3-검증)에 기록했다.
- **다음:** 영향표 A의 다음 묶음 `squid-pool-reset`·`state-sid-lists`(`RawSidPool`·`SidPool` 공유 판정·삭제 기록)이다.
  다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다(33번이 비어 있어 이 단위는 33번으로 커밋한다).

### 0.17 단위 기록 — R0-4 세 번째 묶음: 풀 정리와 SID 목록 (2026-10-28, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘". 시작 HEAD는 `1ddc4ba0`(CP `20261028-39`까지 병합)이며
  작업 트리는 병렬 cpppj 세션의 진행 중 변경이 있었으나 이 단위 파일과 겹치지 않았다. 게임·창 실행과 디컴파일은 하지 않았다.
- **변경:** `RawSidPool.cs`·`Simulation/SidPool.cs`(`ClearDeletionHistory`·공유 서버 판정),
  `Netstorm.Core/Original/RawSquidPoolReset.cs`(신규: `RawPathPoint`·`Finish`·`Reset`),
  `Netstorm.Core/Original/RawStateSidLists.cs`(신규: 등록 3함수·8목록·`ResetCount`·`PrepareGeneralWorld`),
  `X86RawSquidPoolResetTests.cs`·`X86RawStateSidListsTests.cs`(신규: 재생 6·66행 + 계약 3·3개),
  `Netstorm.Core.Tests.csproj`(두 fixture `Link`), `cpp-fixture-manifest.tsv`(71개, reflected-commit 유지),
  `PORT_MAP.tsv`(85·86번째 행 추가·두 풀 파일 base 갱신), `PORT_MAP.md`(5절 수치 갱신),
  `docs/dotnet-r04-pool-sidlists-20261028.md`(신규), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다. Client·CD·공유 전역 연결은 R4 이후에서 잇는다.
- **검증:** Release 경고·오류 0, 전체 **1,413개(Assets 363 + Core 1005 + Client 45) 통과**.
  변별력은 [단위 문서](docs/dotnet-r04-pool-sidlists-20261028.md#3-검증)에 기록했다.
- **다음:** 영향표 A의 다음 묶음 `priestregen`·`priest-panic`·`priest-prayer`
  (`RawPriestState` 사제 HP 회복 허용의 실제 Player 기록 연결)이다.
  다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다(작업 중 `20261028-34`·`-35`를 CP가 썼으므로 이 단위는 36번으로 커밋한다).

### 0.18 단위 기록 — R0-4 네 번째 묶음: 사제 panic·기도와 회복 허용 (2026-10-28, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘". 시작 HEAD는 `55520f78`(CP `20261028-40`까지 병합)이며
  작업 트리는 깨끗했다. 게임·창 실행과 디컴파일은 하지 않았다.
- **변경:** `RawPriestState.cs`(`GetAuthority`·`GetRecoveryAllowed`+`SetHitPoints` 3곳),
  `RawPriestDamage.cs`(`GetBoss`), `RawPriestPreDestroy.cs`(`GetPriests`),
  `Netstorm.Core/Original/RawPriestPanic.cs`·`RawPriestPrayer.cs`(신규),
  `X86RawPriestPanicTests.cs`·`X86RawPriestPrayerTests.cs`(신규: 600·1126행 재생),
  `X86RawPriestStateTests.cs`(신규: 계약 3개),
  `Netstorm.Core.Tests.csproj`(두 fixture `Link`), `cpp-fixture-manifest.tsv`(73개, reflected-commit 유지),
  `PORT_MAP.tsv`(87·88번째 행 추가·세 파일 base 갱신), `PORT_MAP.md`(5절 수치 갱신),
  `docs/dotnet-r04-priest-actions-20261028.md`(신규), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다. 분배기·저장소 읽기·세션·CD 연결과 Regen 재생은 R4 이후·0.19에서 잇는다.
- **검증:** Release 경고·오류 0, 전체 **1,418개 통과 + 1개 건너뜀**(Assets 363 + Core 1010 + skip 1 + Client 45).
  변별력은 [단위 문서](docs/dotnet-r04-priest-actions-20261028.md#3-검증)에 기록했다. 검사 장치 오류 2건(신전 분기 반전·DWORD 오타)은 원본과 대조해 정정했다.
- **알아 둘 것:** 이 기록의 초판에는 "regen fixture가 병렬 세션 작업 중 삭제됐다"고 적었으나 재점검 결과 사실이 아니었다.
  파일명은 하이픈 없는 `priestregen-x86.tsv`이며 git 추적 상태로 처음부터 디스크에 있었고, 제 link·로드명이 잘못된 하이픈형을 가리킨 것이었다.
  병렬 세션을 오해한 대목은 여기서 정정하며, 해당 세션에 삭제·복원 요청을 하지 않는다. Regen 재생은 0.19에서 마저 잇는다.
- **다음:** Regen 재생(0.19) 뒤 `RawGraph` 비표면 좌표 묶음을 잇는다. 다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다(작업 중 `20261028-40`을 CP가 썼으므로 이 단위는 37번으로 커밋했다).

### 0.19 단위 기록 — R0-4 네 번째 묶음 후속: 사제 회복 재생과 기록 정정 (2026-10-28, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "방금의 작업 사항 점검하고 필요한 경우 작업해서 복원해 줘".
  시작 HEAD는 `98106330`(직전 DN `20261028-37`)이며 작업 트리에 0.18 미커밋분(Regen 모듈·검사 2개)이 남아 있었다.
  게임·창 실행과 디컴파일은 하지 않았다.
- **점검 결과(정정):** `priestregen-x86.tsv` 삭제는 사실이 아니었다.
  파일명은 하이픈 없는 `priestregen-x86.tsv`이며 git 추적 상태로 처음부터 디스크에 있었고,
  0.18 작업 중 제 link·로드명이 잘못된 하이픈형(`priest-regen-x86.tsv`)을 가리켜 "없음"으로 오판했다.
  병렬 세션은 무관하며, 0.18의 해당 대목을 위에서 정정했다.
- **변경:** `Netstorm.Core/Original/RawPriestRegen.cs`(신규: `PriestRegenState`·`PriestRegenHooks`·`Handle`),
  `X86RawPriestRegenTests.cs`(신규: 1513행 재생), `Netstorm.Core.Tests.csproj`(fixture `Link`),
  `cpp-fixture-manifest.tsv`(74개, reflected-commit 유지), `PORT_MAP.tsv`(89번째 행 추가),
  `docs/dotnet-r04-priest-actions-20261028.md`(Regen 절 보강), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다. 분배기·저장소 읽기·세션·CD 연결은 R4에서 잇는다.
- **검증:** Release 경고·오류 0, 전체 **1,419개(Assets 363 + Core 1011 + Client 45) 통과**, 건너뜀 0.
  9행 불일치는 검사 후크의 오독(`sample`→`sequence`, 생성 시점 원본·기대값·통과 중인 cpp 검사로 확정)을 정정한 것이다.
  불일치 주입 변이(불일치 분기 제거)에 286행 실패를 확인하고 원복했다.
- **다음:** `RawGraph` 비표면 좌표 묶음이다. 다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다(작업 중 `20261028-42`를 CP가 썼으므로 이 단위는 38번으로 커밋했다).

### 0.20 단위 기록 — R0-4 다섯 번째 묶음: 그래프 비표면 좌표 (2026-10-28, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘". 시작 HEAD는 `73506bcb`(CP `20261028-45`까지 병합)이며
  작업 트리는 깨끗했다. 게임·창 실행과 디컴파일은 하지 않았다.
- **변경:** `Netstorm.Core/Original/RawGraph.cs`(`RegionCoordinate` 신설·`Coordinate` 위임·`Region`·`Calculate` Pop 구간·`CalculatePostDestroy` 셀 선택·삭제 연결 진단),
  `RawGraphTests.cs`(신규 계약 5개), `docs/dotnet-r04-rawgraph-region-20261028.md`(신규), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다. `RawGraphStorage` 공유·`island-population` 섬 모듈·CD·세션 연결은 R4 이후에서 잇는다.
- **검증:** Release 경고·오류 0, 전체 **1,425개(Assets 363 + Core 1017 + Client 45) 통과**.
  기존 L1(rawgraph 1796행·island-graph-frame 24행·파괴 전 Suite)이 그대로 통과해 회귀 없음을 확인했다.
  셀 선택을 구형으로 되돌리는 변이에 신규 검사 2종 실패를 확인하고 원복했다.
- **알아 둘 것:** `island-population-x86.tsv`는 `RawIslandPopulation` 일가의 것으로 `RawGraph`와 무관함을 확인했다(해당 테스트에 RawGraph 참조 0개).
  별도 섬 모듈 단위에서 다룬다.
- **다음:** `RawAssetPostPop` 4종 묶음이다. 다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다(작업 중 `20261028-45`까지 병합됐으므로 이 단위는 46번으로 커밋한다).

### 0.21 단위 기록 — R0-4 마지막 묶음: 자산 후처리 4종 (2026-10-28, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘". 시작 HEAD는 `9ed8bc63`(DN `20261028-46`)이며
  dotnetpj 작업 트리는 깨끗했다(병렬 세션의 cpppj 변경은 건드리지 않았다). 게임·창 실행과 디컴파일은 하지 않았다.
- **변경:** `Netstorm.Core/Original/RawAssetPostPop.cs`(분배표 24개·신규 Validate 4분기·분배·`Type(sid)` 호출 시점 조회),
  `RawArcSpire.cs`·`RawAcidFence.cs`(신규, 최초 Regular 예약+Damageable 연결만),
  `OriginalTypeRecord.cs`(`TechBitIndex` 기본 -1·복사 보존),
  `RawAssetPostPopTests.cs`(신규 계약 5개), `PORT_MAP.tsv`(91행: 새 행 2·base 갱신 2),
  `PORT_MAP.md`(수치), `docs/dotnet-r04-assetpostpop-20261028.md`(신규), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다. 공유 저장소 접근자·span 전환·CD·세션 연결은 R4에서 잇는다.
- **검증:** Release 경고·오류 0, 전체 **1,430개(Assets 363 + Core 1022 + Client 45) 통과**.
  기존 L1(assetpostpop 10.78 몫 포함 Core 전 Suite)이 그대로 통과해 회귀 없음을 확인했다.
  분배표에서 새 4행을 지우는 변이에 신규 검사 3종 실패를 확인하고 원복했다.
- **알아 둘 것:** `arc-spire`·`acid-fence`·`ice-tower` fixture는 전체 몸체 클래스 대조용이라 연결하지 않았다.
  R4 세션 조립에서 새 모듈 단위로 이식한다. `island-population`과 같은 판정(0.20)이다.
- **다음:** R0-4 완료. 다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다.

### 0.22 단위 기록 — R1-2a MonoGame 출력 호스트 (2026-10-28, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘". 시작 HEAD는 `a73d43d2`(DN `20261028-47`)이며
  dotnetpj 작업 트리는 깨끗했다(병렬 세션의 cpppj 변경·신규 파일은 건드리지 않았다). 원본 게임·디컴파일은 실행하지 않았다.
  새 호스트 창의 실행은 이 PC(HJOW-Athlon, AGENTS.md 확인 면제 시스템)에서만 --frames 제한으로 짧게 했다.
- **변경:** `Netstorm.Client/Host/`(신규 5개: `HostOptions`·`ScreenTextureConverter`·`HostPresentation`·`HostInputMapper`·`HostFrameDriver`),
  `Netstorm.Client.Host/`(신규 프로젝트: `ClientHostGame`·`MonoGameScreenOutput`·`Program`), `Netstorm.sln`(등록),
  `Netstorm.Client.Tests/ClientHostTests.cs`(신규 11개), `PORT_MAP.tsv`(99행: `port` 8개 추가),
  `PORT_MAP.md`(수치), `docs/dotnet-client-screen-20261028.md`(6절), 이 문서.
  cpppj·원본·AGENTS.md·`Netstorm.Game`(동결)은 바꾸지 않았다.
- **검증:** Release 경고·오류 0, 전체 **1,441개(Assets 363 + Core 1022 + Client 56) 통과**.
  변환 R/B 뒤바꿈 변이에 기대값 실패를 확인하고 원복했다.
  실행: `--window 320×200 --frames 5` + 3프레임 snapshot 스크립트 → 종료 0, BMP 256,122바이트, 빈 보고서. 인자 오류·없는 스크립트는 종료 2.
  실행 중 찾은 결함(따라잡기 Update 뒤 중간 프레임 관측 건너뜀)은 `EndFrame` draining 으로 고치고 검사로 고정했다.
- **알아 둘 것:** `port` 8행은 fixture 대조가 없는 이식이라 L1이 아니다. 그리기 내용·세션·소리·라벨 클릭은 R2·R3·R4 몫이다.
  `Screen.ToLogicalPoint`(비율 맞춤)와 호스트 정수 맞춤은 다른 사각형을 쓰므로 호스트는 자체 사각표로 마우스를 되돌린다.
- **다음:** R2(그리기 원시·팔레트·글꼴). 다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다.

### 0.23 단위 기록 — R2a 팔레트 3종과 LoadPalette 연결 (2026-10-28, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘". 시작 HEAD는 `58622e15`(DN `20261028-49`)이며
  dotnetpj 작업 트리는 깨끗했다(병렬 세션의 cpppj 변경은 건드리지 않았다). 게임·창 실행과 디컴파일은 하지 않았다.
- **변경:** `Netstorm.Client/Display/`(신규 `PaletteColors`·`PaletteShade`·`PaletteFlash`),
  `Screen.cs`(`LoadPalette` 판본 인자·색 표/명암 표 계산·`Colors()`/`ShadeMaps()`),
  `Netstorm.Client.Tests/`(신규 `X86PaletteColorsTests`·`PaletteColorsTests`·`X86PaletteFlashTests`·`PaletteFlashTests`·`ShadeMapsTests`),
  `Netstorm.Client.Tests.csproj`(fixture Link 2개), `cpp-fixture-manifest.tsv`(2개 등록),
  `PORT_MAP.tsv`(105행: `L1` 6개 추가·`Screen.cs` base 갱신), `PORT_MAP.md`(수치),
  `docs/dotnet-client-screen-20261028.md`(7절), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다.
- **검증:** Release 경고·오류 0, 전체 **1,451개(Assets 363 + Core 1022 + Client 66) 통과**.
  `palettecolors` 24행·`paletteflash` 768행을 원본 기대값과 바이트·비트 단위로 대조해 통과했다.
  검사 작성 중 실패 3건은 모두 테스트의 오류였고(부분 팔레트 +start 채우기·열 번호·리틀 엔디언 double), 소스 수정 없이 고쳤다.
  명암 표는 대조 fixture가 없어 별도 Python 참조 구현의 세 무작위 팔레트(768값)와 회색 경우로 고정했다(L1 아님, R3에서 끝까지 대조).
  밝음 제외 시작 229→230 변이에 참조 기대값 실패를 확인하고 원복했다.
- **알아 둘 것:** `Screen.LoadPalette`는 판본 인자(`patch1078`, 기본 참)를 받는다. 일시 효과(SetPalette·번개)는 표를 바꾸지 않는다.
- **다음:** R2b(그리기 원시 묶음). 다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다.

### 0.24 단위 기록 — R2b VFX 그리기 원시 (2026-10-28, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘". 시작 HEAD는 `cb56907c`(DN `20261028-50`)이며
  dotnetpj 작업 트리는 깨끗했다(병렬 세션의 cpppj 변경은 건드리지 않았다). 게임·창 실행과 디컴파일은 하지 않았다.
- **변경:** `Netstorm.Client/Display/`(신규 `VfxDraw`·`VfxMappedDraw`·`VfxLine`),
  `Netstorm.Client.Tests/`(신규 `X86VfxDrawTests`·`VfxLineTests`·`VfxMappedDrawTests`),
  `Netstorm.Client.Tests.csproj`(fixture Link 1개), `cpp-fixture-manifest.tsv`(1개 등록),
  `PORT_MAP.tsv`(111행: `L1` 2개·`port` 4개 추가), `PORT_MAP.md`(수치),
  `docs/dotnet-client-screen-20261028.md`(8절), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다.
- **검증:** Release 경고·오류 0, 전체 **1,457개(Assets 363 + Core 1022 + Client 72) 통과**.
  `graphics` 709행을 원본 기대값과 바이트 단위로 대조해 통과했다.
  검사 작성 중 실패 1건은 테스트의 오류였고(C++ 조건 `ax > INT32_MAX`에 맞게 수정) 소스 수정은 없었다.
  선·mapped는 대조 fixture가 없어 별도 Python 참조 구현(10가지 경우)·기본 그리기와의 관계로 고정했다(L1 아님).
  투명 마스크 무시 변이에 L1 기대값 실패를 확인하고 원복했다.
- **알아 둘 것:** `VFXTransform`은 `Renderer`·`ColorMap`을 빌려야 하므로 색 표 묶음(R2c)에서 잇는다.
  `fence-line` fixture는 `o/RawFenceLine` 월드 모듈 대조용이라 연결하지 않았다.
- **다음:** R2c(스프라이트 그리기·색 표: `SpriteDraw`·`*ColorMaps`·`VFXTransform`).
  다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다.

### 0.25 단위 기록 — R2c 색 표 (2026-10-28, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘". 시작 HEAD는 `e4b739b9`(DN `20261028-51`)이며
  dotnetpj 작업 트리는 깨끗했다(병렬 세션의 cpppj 변경은 건드리지 않았다). 게임·창 실행과 디컴파일은 하지 않았다.
- **변경:** `Netstorm.Core/Original/`(신규 `RawColorSortPrimary`),
  `Netstorm.Client/Display/`(신규 `SpriteColorMaps`·`PreviewColorMaps`·`ProductionColorMaps`·`OwnerColorMaps`),
  `Netstorm.Client.Tests/`(신규 `X86SpriteColorMapsTests`·`X86PreviewColorMapsTests`·`X86ProductionColorMapsTests`·`X86OwnerHslColorMapsTests`·`X86OwnerColorMapsTests`·`ColorMapFixture`),
  `Netstorm.Core.Tests/`(신규 `RawColorSortPrimaryTests`),
  `Netstorm.Client.Tests.csproj`(fixture Link 5개), `cpp-fixture-manifest.tsv`(5개 등록),
  `PORT_MAP.tsv`(121행: `L1` 10개 추가), `PORT_MAP.md`(수치),
  `docs/dotnet-client-screen-20261028.md`(9절), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다.
- **검증:** Release 경고·오류 0, 전체 **1,465개(Assets 363 + Core 1025 + Client 77) 통과**.
  5개 fixture를 원본 기대값 그대로 대조해 통과했다.
  그림자 어두움 곱 변이에 L1 기대값 실패를 확인하고 원복했다.
- **알아 둘 것:** `owner-capture` C행 480개와 `SpriteDraw` 클래스·그림자 alias 세션 연결은 R4·R5에서 잇는다.
- **다음:** R2d(변환·나머지: `VFXTransform`·`RendererDraw`·`RawShapeToBuffer`·`RawImage`·`GifImage` 적응·`BitmapFont` 적응·`RawColorSortStartup`).
  다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다.

### 0.26 단위 기록 — R2d-1 변환·합성·이미지·시작 (2026-10-29, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘"(cpppj 파일 수정 금지 준수). 시작 HEAD는 `ca0c01eb`(DN `20261028-52`·CP `-53` 병합)이며
  dotnetpj 작업 트리는 깨끗했다(병렬 세션의 cpppj·LEFT_JOBS.md 변경은 건드리지 않았다). 게임·창 실행과 디컴파일은 하지 않았다.
  R2d 묶음 중 세션 자산이 필요 없는 4종(`VFXTransform`·`DrawIndexedImage`·`RawImage`·`RawColorSortStartup`)을 먼저 옮겼다.
  `RawShapeToBuffer`·`SpriteDraw` 본체와 `GifImage`·`BitmapFont` 적응은 R2d-2로 남겼다.
- **변경:** `Netstorm.Client/Display/`(신규 `RendererDraw`·`VfxTransform`·`RawImage`+`TerrainBitmapInput`),
  `Netstorm.Core/Original/RawColorSortStartup.cs`(신규: 시작 전역·후크·두 부모),
  `Netstorm.Client.Tests/`(신규 `RendererDrawTests` 5개·`VfxTransformTests` 5개·`RawImageTests` 7개),
  `Netstorm.Core.Tests/`(신규 `X86RawColorSortStartupTests`: 116행 재생 + 계약 1개),
  `Netstorm.Core.Tests.csproj`(fixture `Link`), `cpp-fixture-manifest.tsv`(1개 등록, reflected-commit 유지),
  `PORT_MAP.tsv`(129행: `L1` 2개·`port` 6개 추가), `PORT_MAP.md`(수치),
  `docs/dotnet-client-screen-20261028.md`(10절), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다.
- **검증:** Release 경고·오류 0, 전체 **1,484개(Assets 363 + Core 1027 + Client 94) 통과**.
  `colorsort-startup` 116행(정상 100·부분중단 16)을 원본 기대값 그대로 대조해 통과했다.
  검사 작성 중 실패 3건은 모두 테스트의 오류였고(하네스 경계 이름·이중 변환 거부 오기·축소 프레임 높이, 소스 수정 없음) 원본과 대조해 고쳤다.
  변이 2건(시작 개수 10→11에 L1 실패, 변환 시작점 반전에 계약 실패)에 실패를 확인하고 원복했다.
  편향 상수 1 증가 변이는 작은 계약 경우에 걸리지 않아 판별력 근거로 쓰지 않았다.
- **알아 둘 것:** `Renderer.h` 행은 `RendererDraw.cs`에 함께 대응시켰고 전체 `Renderer`는 R5 몫이다.
  `RawImage`의 `TerrainBitmapInput`은 R4에서 공용 지형 비트맵으로 재배선할 때까지 이 파일이 소유한다.
  `Set-Content`로 파일을 직접 쓰면 인코딩이 깨져 빌드가 깨진다. 파일 쓰기는 `write`·`edit` 도구로만 한다(이번 단위에서 변이 주입 때 확인하고 원복했다).
- **다음:** R2d-2(`RawShapeToBuffer`·`SpriteDraw` 본체 L1, `GifImage` 적응·`BitmapFont` 적응).
  다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다.

### 0.27 단위 기록 — R2d-2 판본·표시자료·캡처 (2026-10-29, HJOW-Athlon)

- **착수 상태:** 사용자 요청 "dotnetpj 다음 작업 진행해 줘"(cpppj 파일 수정 금지 준수). 시작 HEAD는 `4fab249c`(DN `20261029-01`)이며
  dotnetpj 작업 트리는 깨끗했다(병렬 세션의 LEFT_JOBS.md 변경은 건드리지 않았다). 게임·창 실행과 디컴파일은 하지 않았다.
  R2d-2 계획(`RawShapeToBuffer`·`SpriteDraw` 본체 L1, `GifImage`·`BitmapFont` 적응)을 점검한 결과 L1에 실자료·UI 인프라가 필요해
  `RawShapeToBuffer` 본체(port, 합성 데이터 계약)와 그 전제(판본·표시자료)만 이 단위로 옮겼다.
  `SpriteDraw` 본체는 R2d-3, `GifImage`·`BitmapFont` 적응은 L1에 메뉴·Gump 인프라가 필요해 R3와 함께 다룬다.
- **변경:** `Netstorm.Core/Original/`(신규 `OriginalEdition`·`SquidDisplayShape`·`ConstructionVisualImage`),
  `Netstorm.Client/Display/RawShapeToBuffer.cs`(신규),
  `RawSidPool.cs`(판본 인자·`Edition`), `OriginalTypeRecord.cs`(`HotspotX`·`HotspotY`·복사),
  `Netstorm.Client.Tests/RawShapeToBufferTests.cs`(신규 13개), `Netstorm.Core.Tests/RawSidPoolEditionTests.cs`(신규 3개),
  `PORT_MAP.tsv`(134행: `port` 5개 추가·2개 base 갱신), `PORT_MAP.md`(수치),
  `docs/dotnet-client-screen-20261028.md`(11절), 이 문서.
  cpppj·원본·AGENTS.md는 바꾸지 않았다.
- **검증:** Release 경고·오류 0, 전체 **1,500개(Assets 363 + Core 1030 + Client 107) 통과**.
  기존 L1(const sid-x86 전체 포함)가 판본 기본값에서 그대로 통과해 판본 인자 추가의 회귀 없음을 확인했다.
  검사 작성 중 실패 4건은 모두 테스트의 오류였고(0 풀·0 vtable·짧은 표 인덱스·풀 최소 용량, 소스 수정 없음) 원본과 대조해 고쳤다.
  변이 2건(해제 표식 상수 변경에 계약 실패, 판본 기본값 변경에 판본 검사 실패)에 실패를 확인하고 원복했다.
- **알아 둘 것:** `RawSidPool` 최소 용량은 예측 머리보다 2 이상 커야 하며(합성 하네스는 32768 사용),
  `Allocate()` 기본값은 서버 영역이므로 합성 슬롯은 반환 번호를 그대로 쓴다.
  `RawShapeToBuffer` 실자료 L1은 `SquidRenderer::Shapes` 파이프라인과 함께 R4에서 잇는다.
- **다음:** R2d-3(`SpriteDraw` 본체 port: 규칙 분기는 합성 데이터 계약, 실자료 L1·`GameAssets` 연결·그림자 alias 세션 연결은 R4·R5).
  다음 DN 커밋은 `git log`로 당일 마지막 번호를 확인해 이어 쓴다(당일 `-01`은 DN·CP가 각각 썼다).

### 0.28 단위 기록 — R2d-3 일반 객체 그리기 (2026-10-29, HJOW-Athlon)

- **현재 요청:** Early6·Training9·캠페인16·데모3의 사용자 플레이, Edit/커스텀 맵·시나리오, Multiplayer 외 메인 메뉴·덱·미니맵·QWASZX·ESC를 원본과 같은 동작까지 진행한다. 실제 게임 검사에서 사제 포획/희생·특정 적 공격·승리 재현은 건너뛰고, 멀티플레이는 cpppj/디컴파일 근거로 신규 검사 없이 사전 구현한다. 단위별 문서 갱신/커밋(실패해도 계속)과 push 금지를 지킨다. 기존 중지 요청은 이번 재개 요청으로 해제됐고 목표는 계속 진행 중이다.
- **착수:** AGENTS.md·LEFT_JOBS.md·이 문서를 다시 읽었다. 시작 HEAD `8ca081a26b4b`, dotnetpj 작업 트리는 깨끗했다. 현재/마지막 정밀 디컴파일 호스트가 모두 HJOW-Athlon임을 확인했다. cpppj 소스/기존 fixture만 읽었으며 cpppj·공통 인계·원본/CD·AGENTS.md는 변경하지 않았다. 도중 외부에서 변경된 TODO.md는 커밋 대상에서 제외한다.
- **변경:** `SpriteDraw.cs`·`SpritePixelTarget.cs`·`RenderCollectionEntry.cs`, `SpriteDrawTests.cs`(13개), Client 검사 프로젝트의 `spritedraw` Link, fixture 목록(84개), PORT_MAP(138행), [단위 문서](docs/dotnet-spritedraw-20261029.md). 이식 기준 cpppj 커밋은 `8ca081a26b4b`다. 기존 근사 코드/검사 은퇴는 없다.
- **검증:** 전체 **1,513개 통과(Assets363·Core1030·Client120), 실패/건너뜀0**. 원본 명령열 1,389행(3판본×463)·선택 SID·alias·raw 불변성과 실제 픽셀/현재 전역 계약을 대조했다. 그림자 프레임+1 변이에 세 판본 모두 실패했고 원복했다. CD 입력도 Draw 필드만 50바이트 저장소에 적재하며 CD36바이트 풀 복원으로 세지 않는다. 원시 로그/세부 경계는 단위 문서를 본다. 게임·창 실행과 금지된 실제 플레이 재현·멀티플레이 검사는 하지 않았다.
- **다음:** R2d-3 본체/명령열 L1은 완료다. 실자산 어댑터·수집/정렬/분배·alias 공유 세션 연결은 R4·R5에 남는다. R3의 Gump 기반/위젯/메뉴와 글꼴·GIF 적응을 진행한다. 전체 미션/메뉴/Edit 동일성 목표는 아직 미완료다.
- **최종 확인:** Release 전체 빌드 경고/오류0. 소유자 색 문맥의 값 복사와 SID WORD 변환 재대조 후 Client120개도 재통과했다. 이번 단위는 `20261029-25 DN 일반 객체 그리기와 원본 명령열 대조 이식`으로 로컬 커밋한다.

### 0.29 단위 기록 — R3a Gump 생성·표시 수집 기반 (2026-10-29, HJOW-Athlon)

- **변경:** `Netstorm.Client/Gump/GumpCreate.cs`·`GumpCollection.cs`, 검사 `GumpCreateTests.cs`·`GumpCollectionTests.cs`·`GumpRawSupport.cs`, fixture Link2개와 목록86개, PORT_MAP142행, [단위 문서](docs/dotnet-gump-base-20261029.md). 기준 cpppj는 `2155b0f0855c`의 같은 이름4파일이다. 생성/표시 노드는 같은 객체를 공유한다. 이전 근사 코드·검사 은퇴는 없다.
- **대조:** 생성 전체160바이트·등록 표·일련번호·루트 할당 순서945행, 표시 위치/경계/수집·전체512바이트 출력/guard·캐시·투영 순서1167행을 기대값 그대로 대조했다. 집중14개 통과. 기본 생성이 쓰지 않는Form을0으로 만드는 변이에 세 판본이 모두 실패했고 원복했다. 로컬 좌표 변환·순환/출력 용량·같은 저장소 연결은 별도 계약이며 메뉴 화면 일치로 세지 않는다.
- **다음:** R3b `GumpTree`의 부착/밀어내기·정렬·깊이·이동을 같은 저장소에 연결한다. 이어서 수명/입력/프레임/그리기·글꼴/GIF·메뉴를 조립한다. 실제 메뉴·전체 미션/Edit/멀티플레이 사전 구현은 아직 남는다. 게임·창 실행/금지된 실제 재현/멀티플레이 검사는 하지 않았다. 외부 TODO.md 변경은 이번 커밋에서도 제외한다.
- **최종 확인:** 전체 **1,527개 통과(Assets363·Core1030·Client134), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. `20261029-26 DN Gump 생성 등록과 표시 수집 기반 이식`으로 로컬 커밋한다. 목표는 계속 진행 중이며 다음 단위는 R3b다.

### 0.30 단위 기록 — R3b Gump 트리·정렬·깊이·이동 (2026-10-29, HJOW-Athlon)

- **변경:** `Netstorm.Client/Gump/GumpTree.cs`, 검사 `GumpTreeTests.cs`8개와 공유 `GumpRawSupport.ApplyDword`, fixture Link1개/목록87개, PORT_MAP144행, [단위 문서](docs/dotnet-gump-tree-20261029.md). cpppj 기준은 `a84c02c69ef0`의 같은 이름2파일이다. stable 노드를 교체하지 않고 같은 객체의 필드에 콜백 응답을 쓴다.
- **대조:** 원본1,722행(3판본×574)의 모든 노드160바이트·전역·정렬 반환 비트·가상 호출/당시 전체 상태를 그대로 대조했고 집중8개가 통과했다. 부모 기본 깊이 -3→-4 변이에 세 판본 모두 실패 후 원복했다. 생성→부착/분리/재부착→수집은 실제 연결이며 정렬/Draw/전체 픽셀 통합은 R5에 남는다.
- **다음:** R3c `GumpLifetime`의 지연 삭제/소멸자·idle/overlay/dialog/큐를 같은 저장소에 연결한다. 이어서 사건/입력/프레임/그리기·위젯·글꼴/GIF·메뉴다. 실제 메뉴/미션/Edit/전체 목표는 계속 진행 중이다. 게임/창·금지된 실제 재현·멀티플레이 검사는 하지 않았고 cpppj/공통 인계/원본/AGENTS.md도 바꾸지 않았다. 외부 TODO.md 변경은 커밋 대상에서 제외한다.
- **최종 확인:** 전체 **1,535개 통과(Assets363·Core1030·Client142), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. `20261029-27 DN Gump 트리 부착 정렬 깊이와 이동 이식`으로 로컬 커밋한다. 다음 재개 지점은 R3c이며 목표 완료/일시정지로 처리하지 않는다.

### 0.31 단위 기록 — R3c Gump 수명·루트 교체 (2026-10-29, HJOW-Athlon)

- **착수:** AGENTS.md와 이 문서 및 공통 인계의 유효 제약을 다시 읽었다. 직전 목표 턴은3단위 구현/검증/커밋이 완료된 진행이다. 현재/마지막 정밀 디컴파일 호스트는 모두 HJOW-Athlon이다. 시작 HEAD는 `6f2b5101bae0`이며 외부 TODO.md만 수정돼 있었다.
- **변경:** `GumpLifetime.cs`, `GumpLifetimeTests.cs`·`GumpResetTests.cs`·공유 `GumpLifetimeFixture.cs`, fixture Link2개/목록89개, PORT_MAP146행, [단위 문서](docs/dotnet-gump-lifetime-20261029.md). cpppj 기준은 시작 HEAD의 같은 이름2파일이다. 자식 우선 지연/즉시 삭제·등록 해제·기본 소멸자·큐·idle/overlay/dialog·제외 타입 순회·루트 교체를 같은 생성/트리 저장소에 연결했다.
- **대조:** 원본761행(수명519·reset242)의 전체 노드160바이트·5표·스칼라·연속 반환·콜백 당시 상태를 그대로 재생했고 집중13개가 통과했다. 실제 기본 소멸자/저장소 해제·생존 자식 이동·루트 교체·등록 칸 재사용도 연결했다. 제외 signed BYTE0확장 변이에 두 판본이 실패한 뒤 원복했다. 기존 근사 코드/검사 은퇴·전체 메뉴 캡처 비교는 없다.
- **다음:** R3d `GumpEvents`·`GumpInput`·`GumpFrame`의 사건 선택/재귀 전달/캡처/dialog/overlay/통지/idle/재그리기를 연결한다. 이후 Draw·위젯·글꼴/GIF·메뉴다. 실제 전체 미션/Edit/멀티플레이 사전 구현은 남아 있고 목표는 계속 진행 중이다. 원본/AGENTS.md/cpppj/공통 인계는 변경하지 않았으며 게임/창·금지된 실제 재현·멀티플레이 검사는 하지 않았다.
- **최종 확인:** 전체 **1,548개 통과(Assets363·Core1030·Client155), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. `20261029-28 DN Gump 지연 삭제 소멸자와 루트 교체 이식`으로 로컬 커밋한다. 다음은 R3d 사건/입력이며 목표는 활성 상태로 유지한다.

### 0.32 단위 기록 — R3d-1 Gump 사건·입력 (2026-10-29, HJOW-Athlon)

- **변경:** `GumpEvents.cs`·`GumpInput.cs`, 검사 `GumpEventsTests.cs`·`GumpInputTests.cs`·공유 `GumpEventFixture.cs`, fixture Link2개/목록91개, PORT_MAP150행, [단위 문서](docs/dotnet-gump-events-input-20261029.md). 기준 cpppj는 `d9ddcc929d42`의 같은 이름4파일이다. 현재 생성/트리/위치/수명에 커서 선택·기본/가상 필터·자식 우선/부모 전달·캡처/dialog/overlay를 연결했다.
- **대조:** 원본1,467행(사건771·입력696)의 전체 노드160바이트·선택/입력 전역·2표·연속 반환·가상 호출 당시 상태를 재생했고 집중17개가 통과했다. 실제 생성→부착→선택/전달→지연 삭제와 캡처/dialog/overlay 통합도 포함한다. 기존 근사 코드/검사 은퇴나 실제 전체 화면 캡처 비교는 없다.
- **다음:** R3d-2 `GumpFrame`의 실제 등록 통지·idle·재그리기를 연결한다. 이후 Draw·위젯·글꼴/GIF·메뉴/호스트 입력 큐다. 전체 미션/Edit/멀티플레이 사전 구현은 계속 남으며 목표는 진행 중이다. 게임/창·금지된 실제 재현·멀티플레이 검사는 하지 않았고 보호 디렉터리/cpppj/AGENTS.md/공통 인계도 변경하지 않았다.
- **최종 확인:** Dispatch raw 반환 변이에 세 판본 모두 실패 후 원복했다. 전체 **1,565개 통과(Assets363·Core1030·Client172), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. `20261029-29 DN Gump 사건 선택과 캡처 대화상자 입력 이식`으로 로컬 커밋한다. 외부 TODO.md는 제외하며 다음은 R3d-2다.

### 0.33 단위 기록 — R3d-2 Gump 프레임 통지·idle·재그리기 (2026-10-29, HJOW-Athlon)

- **변경:** `GumpFrame.cs`, 검사 `GumpFrameTests.cs`9개, fixture Link1개/목록92개, PORT_MAP152행, [단위 문서](docs/dotnet-gump-frame-20261029.md). cpppj 기준은 `9fcc08d7153f`의 같은 이름2파일이다. 실제 등록/수명/트리 표를 이용해 통지·idle·재그리기 요청을 연결했다.
- **대조:** 원본810행(3판본×270)의 전체 노드160바이트·등록 표/idle·개수·콜백 당시 상태를 재생했고 집중9개가 통과했다. 실제 idle 중 Kill/정리/해제, CD/패치 길이, 비영 칸 수와 현재 signed count, 최상위 next 보관/하위 next 재조회·예외 전 쓰기까지 포함한다. 기존 근사 검사 은퇴나 실제 전체 화면 비교는 없다.
- **다음:** R3e `GumpDraw`의 임시 클립·현재 픽셀 표면과 `GumpVisual`을 옮긴다. 이어서 배경/위젯·글꼴/GIF·메뉴·호스트 프레임/입력 큐다. 실제 전체 미션/Edit/멀티플레이 사전 구현은 남으며 목표는 진행 중이다. 원본/AGENTS.md/cpppj/공통 인계는 변경하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사는 하지 않았다.
- **최종 확인:** notify 칸만 개수에 세는 변이에 세 판본 모두 추가 통지 호출을 검출했고 원복했다. 전체 **1,574개 통과(Assets363·Core1030·Client181), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. `20261029-30 DN Gump 프레임 통지 idle과 재그리기 이식`으로 로컬 커밋한다. 외부 TODO.md는 제외한다. 다음 재개 지점은 R3e이며 목표는 활성 상태로 유지한다.

### 0.34 단위 기록 — R3e Gump 클립·버튼·현재 픽셀 출력 (2026-10-29, HJOW-Athlon)

- **완료:** 기준 `39bc55e914fa`의 `GumpDraw`·`GumpVisual`·`GumpPixelTarget`을 새 경로로 옮겼다. 현재 flags/투영 뒤 클립 저장, 중첩/예외 복구, 버튼 세 문맥·선/글자 배치, 현재 inclusive VFX 클립의 선/그림/원시 비트맵/확대/fill을 연결했다. 상세는 [단위 문서](docs/dotnet-gump-draw-20261029.md)다.
- **대조:** 원본 gumpvisual192·gumpdraw1428행을 Link로 연결했다(목록94개, 맵158행). 집중17개 및 VFX right 끝점 -1 누락 변이의 세 판본 실패를 확인했다. 원복 후 전체 **1,591개 통과(Assets363·Core1030·Client198), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 실제 캡처 비교0이며 헤드리스 픽셀 통합/L1을 메뉴 완료로 세지 않는다.
- **다음:** R3f `GumpBackground*`의 생성·질감/단색/체크·픽셀 위젯을 연결한다. 공통 정렬/kind2 분배는 R5이며 글꼴/GIF·버튼/메뉴·호스트 프레임/입력 큐도 남는다. 기존 코드/검사 은퇴0, 전체 목표는 계속 진행 중이다. 보호 디렉터리/cpppj/공통 인계/AGENTS.md는 변경하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사도 하지 않았다. 외부 TODO.md는 커밋에서 제외한다.

### 0.35 단위 기록 — R3f 배경 생성·질감·현재 자산 출력 (2026-10-29, HJOW-Athlon)

- **완료:** 기준 `436deaf700d8`의 `GumpBackground*`를 같은 기본 생성/수집/클립/픽셀 경로에 옮겼다. signed 색/헤더·판본별 shape 저장·type0 미작성 필드 보존, 화면 기준 타일 반복/2픽셀 명암 테두리, 체크/단색 우선순위, 현재 자산 재조회와 클립 복구, 실제 SHP/소스 명암표 출력이다. [범위와 남은 연결](docs/dotnet-gump-background-20261029.md).
- **대조:** 원본804행(명암표9·질감168·전체 Draw627)을 Link로 연결했다(목록96·맵164행). 집중16개가 통과했고 밝음 우선순위 반전 변이를 원본 기록이 검출했다. 변이 제거 후 전체 **1,607개 통과(Assets363·Core1030·Client214), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이며 기대값 수정은 없다. GameAssets 전용 생성자/실자산 Inspector·실제 메뉴 조립은 아직 남는다.
- **다음:** R3g `BitmapFont`/`GumpText`/`GumpTextDraw`의 글리프 출력과 버튼/돌 버튼 배경 자식·가상 Draw를 연결한다. 이후 GIF·텍스트/메뉴·실제 호스트다. 실제 캡처 비교0, 기존 코드/검사 은퇴0이며 전체 미션/Edit/멀티플레이 사전 구현 목표는 계속 진행 중이다. 보호 디렉터리/cpppj/공통 인계/AGENTS.md는 변경하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사를 하지 않았다. 외부 TODO.md는 커밋에서 제외한다.

### 0.36 단위 기록 — R3g-1 비트맵 글꼴·측정·글리프 출력 (2026-10-29, HJOW-Athlon)

- **착수:** AGENTS.md·이 문서·공통 인계의 유효 제약을 다시 읽었다. 직전 목표 턴은2단위 구현/검증/커밋이 완료된 진행이다. 현재/마지막 정밀 디컴파일 호스트는 모두 HJOW-Athlon이며 시작 HEAD는 `e074cad7f385`다. 기존 외부 TODO.md만 수정돼 있었다.
- **완료:** `Netstorm.Client`의 원본 BitmapFont/FontStore·GumpText·GumpTextDraw와 호스트 Windows GDI 메모리 생성 경계를 연결했다. 세 폭/포인터/특수 빈 프레임, 판본별 alias 쓰기·스타일/슬롯/메트릭, 그림자/외곽선/본문/현재 색표, 투영 뒤 기본/명시 참조 문맥을 보존했다. [검증과 남은 연결](docs/dotnet-gump-font-20261029.md).
- **대조:** 두 fixture1,719행(Font36·원본Case1683)을 Link로 연결했다(목록98·맵172행). 집중17개·실제 캐시18개/4,608글리프/302,480픽셀 합·창 없는 실제GDI→글리프 출력이 통과했다. 첫 폭을 출력 폭으로 쓰는 변이는 세 판본 모두 실패했고 원복했다. 이후 전체 **1,624개 통과(Assets363·Core1030·Client231), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 기대값 수정은 없다.
- **다음:** R3g-2 `ButtonGump`/`StoneButtonGump`/`BindStoneButtonText`의 라벨·배경 자식·입력·가상 Draw·파괴를 같은 저장소에 연결한다. Renderer 장면/GIF/메뉴/전체 자산·호스트 조립은 남는다. 실제 캡처 비교0, 기존 코드/검사 은퇴0이며 전체 목표는 계속 진행 중이다. cpppj/공통 인계/원본/AGENTS.md는 변경하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사도 하지 않았다. 외부 TODO.md는 커밋에서 제외한다.

### 0.37 단위 기록 — R3g-2 버튼·돌 버튼·도움말 연결 (2026-10-29, HJOW-Athlon)

- **착수:** AGENTS.md·이 문서·공통 인계의 유효 제약을 다시 읽었다. 직전 턴은 글꼴의 구현/검증/커밋이 완료된 진행이다. 현재/마지막 정밀 디컴파일 호스트는 모두 HJOW-Athlon이며 시작 HEAD는 `0a1a709127ad`다. 외부 TODO.md만 수정돼 있었다.
- **완료:** 실제 `ButtonGump`의 값/일반·토글·즉시 입력/캡처/뗌/반복·점멸·부모 통지·팝업 Kill, `StoneButtonGump`의 부분 문맥·라벨·측정·배경 자식·가상Attach/Draw·파괴, `BindStoneButtonText`와 기본 우클릭 `GumpHelp`를 같은 저장소에 연결했다. [범위/검증/남은 연결](docs/dotnet-gump-button-20261029.md).
- **대조:** 원본 버튼742행의 전체 노드/표/콜백 상태/시계QWORD가 일치한다(목록99·맵178행). 집중21개가 통과했고 시계 조회를1회로 합친 변이를 세 판본 모두 검출한 뒤 원복했다. 이후 전체 **1,645개 통과(Assets363·Core1030·Client252), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 실제 입력과 배경/글리프 픽셀·자식 우선Cleanup도 연결했으며 기대값 수정은 없다.
- **다음:** R3h 텍스트/메뉴/스크롤 위젯과 실제 메뉴 조립이다. GumpHelp의 실제 메뉴/소리·GIF/전체 자산·호스트 프레임/입력 큐, 공통 CRT 정렬/kind 분배(R5)는 남는다. 이번 픽셀 입력의 명시 순서는 배경→부모이며 정렬을 대신 구현하지 않았다. 실제 캡처0/기존 코드·검사 은퇴0, 전체 목표는 계속 진행 중이다. 보호 디렉터리/cpppj/공통 인계/AGENTS.md는 변경하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사를 하지 않았다. 외부 TODO.md는 커밋에서 제외한다.

### 0.38 단위 기록 — R3h-1 StyleText 조각·한 줄 텍스트 (2026-10-29, HJOW-Athlon)

- **착수:** AGENTS.md·이 문서·공통 인계의 유효 제약을 다시 읽었다. 직전 턴은 버튼 구현/검증/커밋이 완료된 진행이며 시작 HEAD는 `702917dcf7d9`다. 현재/마지막 정밀 디컴파일 호스트는 모두 HJOW-Athlon이고 외부 TODO.md만 수정돼 있었다.
- **완료:** StyleText의 값복사/실제 소유 체인·반납·두 번 측정/출력·판본별alias/head/next 조회·짧은 명령과 한 줄 TextGump의 실제 생성/라벨/맞춤/내부클립/수명을 같은 글꼴/팔레트/픽셀에 연결했다. [범위·근거·남은 연결](docs/dotnet-styletext-single-20261029.md).
- **검증:** 집중19개가 통과했다. C++ styletext.json은 정적 보고서이며 독립 원본 실행 기대값이 없어 이 단위를 L1로 세지 않는다. 기존 fixture99/reflected-commit은 유지하고 포트 맵만182행으로 갱신했다. 최초 장면6개 실패는 필수Kill 훅 누락을 검사 배선에서 고쳐 통과했다. 두 번째head 스냅숏 변이는 고정 계약에서 실패한 뒤 원복했으며 실제 원본 대조를 대신하지 않는다. 이후 전체 **1,664개 통과(Assets363·Core1030·Client271), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다.
- **다음:** R3h-2 StyleTextBuffer/다중 줄 ScrollingTextGump·Markup/StyleTextMarkup, 이후 MenuGump 레이아웃/컨트롤/위젯/입력/Draw와 실제 메뉴 조립이다. BindMenuLayoutStyleText와 다중 줄 포함Draw 분류는 해당 조립 때 연결한다. 실제 캡처0/기존 코드·검사 은퇴0, 전체 목표는 계속 진행 중이다. 보호 디렉터리/cpppj/공통 인계/AGENTS.md는 변경하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사도 하지 않았다. 외부 TODO.md는 커밋에서 제외한다.

### 0.39 단위 기록 — R3h-2 버퍼·그림·다중 줄 표시 (2026-10-29, HJOW-Athlon)

- **착수:** AGENTS.md·이 문서·공통 인계 제약을 다시 읽었다. 직전 턴은 스타일 조각/한 줄 텍스트의 구현·검증·커밋이 완료된 진행이며 시작 HEAD는 `b9be2a82a0f9`다. 현재/마지막 정밀 디컴파일 호스트는 모두 HJOW-Athlon이고 외부 TODO.md만 수정돼 있었다.
- **완료:** 실제 StyleTextBuffer의 완료/미완료 입력과 표시 복제 소유·줄바꿈·반납, Markup 그림의 현재 자산/배율/수명/실제 SHP·색표 출력, ScrollingTextGump의 배치/맞춤/스크롤/선택/내부클립/가시 줄 출력과3인자Draw 분배를 같은 저장소에 연결했다. [상세 범위·검증·남은 연결](docs/dotnet-stylebuffer-scroll-20261029.md).
- **검증 완료:** 집중31개와 전체 **1,695개 통과(Assets363·Core1030·Client302), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 독립 원본 실행 기대값이 없어 L1로 세지 않으며 fixture99/reflected-commit을 유지하고 포트 맵만188행으로 갱신했다(갱신 뒤 형식/경로2개도 통과). 글자·그림 혼합 체인의 실제 픽셀과 C++/C# 대입 평가 차이의 현재 노드 재조회도 검사했다. 변경12파일의 UTF-8·한국어와 보호 경로 변경0을 확인했다.
- **다음:** R3h-3 StyleTextMarkup·ScrollingTextGumpWidgets/Markup의 파서/생성/버퍼 소유 수명/slider/입력, 이후 MenuGump 전체와 실제 메뉴/전체 GameAssets/GIF/호스트 조립이다. 실제 캡처0/기존 코드·검사 은퇴0이고 전체 목표는 계속 진행 중이다. 보호 경로/cpppj/공통 인계를 수정하지 않았으며 게임/창·금지된 실제 재현·멀티플레이 검사를 하지 않았다. 외부 TODO.md는 커밋에서 제외한다.

### 0.40 단위 기록 — R3h-3a 스타일 파서·버퍼 소유 수명 (2026-10-29, HJOW-Athlon)

- **착수:** AGENTS.md·이 문서·공통 인계 제약을 다시 읽었다. 직전 턴은 버퍼/그림/다중 줄 표시의 구현·검증·커밋이 완료된 진행이며 시작 HEAD는 `f7e1748e8c9a`다. 현재/마지막 정밀 디컴파일 호스트는 모두 HJOW-Athlon이고 외부 TODO.md만 수정돼 있었다.
- **완료:** StyleTextMarkup의 부분 검색/greedy/우선순위/판본별 그림·옵션, ScrollingTextGumpMarkup의 구간별 실제 추가/태그/현재 문맥·제한, StyleTextBufferStore의 소유/외부 상태 생성·실제 조회·소멸/free를 같은 저장소에 연결했다. [범위·검증·후속](docs/dotnet-stylemarkup-parser-20261029.md).
- **검증 완료:** 집중50개와 전체 **1,745개 통과(Assets363·Core1030·Client352), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 실제 한 줄/다중 줄 파서와 두 판본의 실제 글꼴/SHP 혼합 픽셀·버퍼 수명을 확인했다. 최초 검사 컴파일 오류1건은 잘못된 표 인덱싱 경로를 검사에서 수정했다. 독립 원본 실행 기대값이 없어 L1로 세지 않으며 fixture99/reflected-commit을 유지하고 포트 맵만194행으로 갱신했다. 변경12파일의 UTF-8·한국어와 보호 경로 변경0을 확인했다.
- **다음:** 공통 Slider 계층이 선행 미구현이므로 R3h-3b ScrollGump/ScrollGumpWidgets→ScrollingTextGumpWidgets의 실제 생성/입력/수명·Draw를 잇는다. 이후 MenuGump 전체와 실제 메뉴/전체 자산/ConfigInterface/음향/GIF/호스트 조립이다. 실제 캡처0/기존 코드·검사 은퇴0, 전체 목표는 계속 진행 중이다. 보호 경로/cpppj/공통 인계를 수정하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사를 하지 않았다. 외부 TODO.md는 커밋에서 제외한다.

### 0.41 단위 기록 — R3h-3b-1 공통 Slider·자식 Widget (2026-10-29, HJOW-Athlon)

- **착수:** AGENTS.md·이 문서의 현행 계획/최신 인계·공통 제약을 다시 읽었다. 직전 턴은 파서/버퍼 수명의 구현·검증·커밋이 완료된 진행이며 시작 HEAD는 `075b0c1424db`다. 현재/마지막 정밀 디컴파일 호스트는 모두 HJOW-Athlon이고 외부 TODO.md만 수정돼 있었다.
- **완료:** ScrollGump 값/범위/실제 부착/포함 선택/폴링·드래그/부모 통지와 ScrollGumpWidgets의 실제 돌 화살표/질감 손잡이 생성·Draw·자식 우선 수명을 같은 기본 저장소에 연결했다. 실제 다중 줄 표시→같은 저장소 Slider의 네 경계도 연결했다. [범위·검증·다음](docs/dotnet-slider-widgets-20261029.md).
- **검증 완료:** 집중21개와 전체 **1,766개 통과(Assets363·Core1030·Client373), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 최초3실패는 C++ 감김 비교/안쪽·바깥 Bubble을 수동 기대값에서 빠뜨린 것으로 소스를 재확인해 정정했고 제품 코드는 바꾸지 않았다. 실제 픽셀·원본 new 크기·부분 상태·캡처/반납도 확인했다. 독립 원본 실행 기대값이 없어 L1로 세지 않으며 fixture99/reflected-commit을 유지하고 포트 맵만198행으로 갱신했다. 변경9파일의 UTF-8·한국어와 보호 경로 변경0을 확인했다.
- **다음:** R3h-3b-2 ScrollingTextGumpWidgets의 생성/128칸 버퍼 소유·교체/Slider 자식/부착/커서·태그·캡처·드래그/수명, 이후 MenuGump 전체/실제 메뉴 조립이다. 실제 캡처0/기존 코드·검사 은퇴0, 전체 목표는 계속 진행 중이다. 보호 경로/cpppj/공통 인계를 수정하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사를 하지 않았다. 외부 TODO.md는 커밋에서 제외한다.

### 0.42 단위 기록 — R3h-3b-2 다중 줄 Widget 생성·입력·수명 (2026-10-29, HJOW-Athlon)

- **착수:** 같은 목표 턴에서0.41의 구현/검증/인계/커밋(`5765d120253d`, DN38)을 완료한 뒤 진행했다. 현재/마지막 정밀 디컴파일 호스트는 모두 HJOW-Athlon이다. 외부 TODO.md는 계속 제외한다.
- **구현:** 실제128칸 소유 버퍼/overlay/숨긴 Slider·자식 생성·부착, 현재 폭/요청/설정 비트, 태그·커서·캡처·드래그/부모 통지, 소유·빌린 버퍼 교체/실제 자식 우선 반납과 가상 분배를 같은 저장소에 연결했다. [범위·검증·다음](docs/dotnet-scrolling-widget-20261029.md).
- **검증 완료:** 신규17개/기존Slider21개 집중38개와 최종 전체 **1,783개 통과(Assets363·Core1030·Client390), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 최초NUL 입력 오류6건은 검사 도우미의 명시 길이로 보완했다. 추가NaN 검사/첫전체 CD1건은 원본 GumpTree의 재부착 전 Push 계약을 빠뜨린 검사 전제로, 검사에 Push를 추가한 뒤 최종 통과했다. 제품 코드/기대값은 바꾸지 않았고 실패 로그를 보존했다. 독립 원본 실행 대조/L1이 아니며 fixture99/reflected-commit을 유지하고 포트 맵만200행으로 갱신했다. 변경8파일의 UTF-8·한국어와 보호 경로 변경0을 확인했다.
- **다음:** R3h-4 MenuGump 전체 레이아웃/컨트롤/위젯/입력/Draw·항목 소유 StyleText 바인딩, 실제 메뉴/전체 자산/음향/GIF/호스트를 조립한다. 실제 캡처0/기존 코드·검사 은퇴0이며 전체 목표는 계속 진행 중이다. 보호 경로/cpppj/공통 인계를 수정하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사를 하지 않았다.

### 0.43 단위 기록 — R3h-4a 메뉴 기본·항목 소유·레이아웃 (2026-10-29, HJOW-Athlon)

- **착수:** AGENTS.md·이 문서의 현행 계획/최신 인계·공통 제약을 다시 읽었다. 직전 턴은 Slider/다중 줄 Widget의2단위 구현·검증·커밋이 완료된 진행이며 시작 HEAD는 `6c0a1865fb35`다. 현재/마지막 정밀 디컴파일 호스트는 모두 HJOW-Athlon이고 외부 TODO.md만 수정돼 있었다.
- **구현:** 실제 MenuGump의 크기/장식/확장/controlled 스택과 MenuGumpLayout의 조건부 항목 추가·strncpy40·스타일 소유/실제 글꼴/두 방향 전체 배치, BindMenuLayoutStyleText의 실제 항목 ref 필드를 연결했다. [근거·검증·다음](docs/dotnet-menu-layout-20261029.md).
- **검증:** 원본 메뉴308행/레이아웃260행의 전체 바이트/상태·콜백/CRC·반환이 일치했고 집중14개가 통과했다. 기존 최상위 원문도 사본 없이 Link하려고 동기화 도구/목록에 읽기 전용 cpppj-root 출처를 추가했다(기존 보고서 SHA와의 차이는 CRLF/LF임을 확인). fixture101/reflected-commit 유지·포트204행(L1 101/port103)이며 원본 기대값 수정은 없다. 최초 검사 컴파일 네임스페이스1건을 검사에서 고쳤다. 전체 **1,797개(Assets363+Core1030+Client404) 통과**, 실패/건너뜀0이며 Release 전체 빌드는 경고/오류0이다.
- **다음:** MenuGumpControls/Widgets/Input/Draw와 실제 메뉴 전체 자산/음향/GIF/호스트 조립이다. 실제 캡처0/기존 코드·검사 은퇴0이며 전체 목표는 계속 진행 중이다. 보호 경로/cpppj/공통 인계를 수정하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사를 하지 않았다. 외부 TODO.md는 커밋에서 제외한다.

### 0.44 단위 기록 — R3h-4b 메뉴 제어부 (2026-10-29, HJOW-Athlon)

- **착수/근거:** 앞 단위를 `5f1ee0b7`로 커밋했다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon이다. 같은 cpppj MenuGumpControls/기존 보고서의 정적 근거를 읽고 실제 항목/제목·설정 읽기 경계로 옮겼다. 이 모듈의 독립 PE 관찰은 없어 L1로 세지 않는다.
- **구현/검증:** MenuGumpControls의 판본별 색/서식·선검사/현재 칸 재조회·CD 자식 문맥/선택 부분 갱신을 작성했고 Client Release 컴파일은 경고/오류0이다. 같은 실제 트리/소유 조각/다중 줄 버퍼/팔레트/픽셀에 연결한 집중17개가 통과했다. 새 검사4개의 문맥 필드/명령 순서 가정을 고쳤으며 제어부/원본 기대값 수정은 없다. 검사 분석기 경고5개는 Assert.Single 조건 인자로 정리했다. 포트2행을 port로 추가해206행(L1 101/port105)이고 fixture101/reflected-commit은 유지했다. 전체 **1,814개(Assets363+Core1030+Client421) 통과**, 실패/건너뜀0이며 Release 전체 빌드 경고/오류0이다. [구현과 한계](docs/dotnet-menu-controls-20261029.md). 원본 기대값과 보호 경로는 수정하지 않았다.
- **다음:** MenuGumpWidgets의 실제 메뉴 생성/장식·제목 자식/controlled 시작/부착/생성 개수·역순 소멸, 이어 MenuGumpInput/Draw다. ConfigStore 읽기/치환은 연결했으나 ConfigInterface 시작/저장·전체 자산/음향/GIF/호스트는 남는다. 실제 메뉴/게임 캡처0·기존 코드/검사 은퇴0이며 전체 목표는 계속 진행 중이다. 보호 경로/cpppj/공통 인계를 수정하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사를 하지 않았다. 외부 TODO.md는 커밋에서 제외한다.

### 0.45 단위 기록 — R3h-4c 장식 배경 (2026-10-29, HJOW-Athlon)

- **착수:** AGENTS.md·현행 계획/최신 인계·공통 제약을 다시 읽었다. 직전 목표 턴은 메뉴 기본/제어부2단위의 구현·검증·커밋이 완료된 진행이며 시작 HEAD는 `d5d05597`이다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon이다. 외부 TODO.md만 수정돼 있었다.
- **구현/검증:** 메뉴 생성자의 선행 의존성인 GumpDecoration의 실제 배경/현재 모서리/변경 영역/수명·SHP 픽셀을 작성했다. 첫 컴파일의 Assets/Display 이름 충돌은 불필요한 using 제거로 고쳤다. 첫 집중 검사 컴파일의 RedrawNode 인자1건과 기본 루트 포함 개수 가정을 검사에서 고쳤다. 집중9개·전체 **1,823개(Assets363+Core1030+Client430) 통과**, 실패/건너뜀0·Release 전체 경고/오류0이다. 포트208행(L1 101/port107), fixture101/reflected-commit 유지이며 독립 PE 관찰/L1은 아니다. 도중 외부 TODO.md만의 커밋53c597b8이 들어왔고 해당 내용은 수정하지 않았다. [근거와 다음](docs/dotnet-decoration-20261029.md). 다음은 MenuGumpWidgets다. 원본 게임/창/PE 실행과 보호 경로 변경은 없다.

### 0.46 단위 기록 — R3h-4d 메뉴 생성/수명 (2026-10-29, HJOW-Athlon)

- **착수/구현/검증:** 장식 단위를 `acc8f3dc`로 커밋한 뒤 같은 cpppj MenuGumpWidgets.h/.cpp를 옮겼다. 실제 메뉴 생성/장식·제목/controlled 시작·부착·역순 항목 소멸과 가상 분배/제목 Reflow를 작성했고 Client Release 컴파일 경고/오류0이다. 같은 실제 기본 트리/장식/제목/버퍼/소유 조각·픽셀/수명과 공용 SID Form에 연결했다. 집중21개(메뉴17+실제 Form4)가 통과했다. 새 검사4개에서 그림자 여백/판본별 제목 문맥 가정을 고쳤고 제품/원본 기대값 수정은 없다. 포트212행(L1 101/port111)이며 fixture101/reflected-commit 유지다. 전체 **1,844개(Assets363+Core1030+Client451) 통과**, 실패/건너뜀0·Release 전체 경고/오류0이다. [구현과 범위](docs/dotnet-menu-widgets-20261029.md). 원본 기대값/보호 경로/cpppj/공통 인계 변경과 게임/창 실행은 없다.
- **다음:** MenuGumpInput/Draw를 같은 실제 사건/트리/표면에 연결한다. 실제 메뉴/전체 자산/ConfigInterface/음향/GIF/호스트·R4 이후 세션/월드/HUD/전체 미션/Edit는 남는다. 독립 PE 관찰/실제 메뉴 캡처/L1 추가0·기존 코드/검사 은퇴0이며 전체 목표는 계속 진행 중이다. Form 실제 연결은 기존10.78 범위를 사용했고 CD Form/서버 전송 경계는 확장하지 않았다. 실제 금지 재현/멀티플레이 검사와 외부 TODO.md 수정은 없다.

### 0.47 단위 기록 — R3h-4e 메뉴 출력 (2026-10-29, HJOW-Athlon)

- **착수/구현/검증:** AGENTS.md·현행 인계/공통 제약을 다시 읽었다. 직전 목표 턴은 장식/메뉴 생성·수명2단위의 구현·검증·커밋을 완료한 진행이며 시작 HEAD는 `5c3306f9`다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon이다. MenuGumpDraw의 실제 배경/강조/구분선/체크/글자/자식 표시와 J 첫 프레임+2 바인딩을 작성했고 Client Release 컴파일은 경고/오류0이다. 집중16개가 통과했다. 새 검사3개의 Append 계약/스타일 필드 입력을 고쳤고 원본 기대값/실패 대응 제품 변경은 없다. 두 보호 SHP 고정J02=60의 전체 픽셀/clip/padding과 실제 큰 정수Y의 패치/CD 좁힘 차이를 확인했다. 포트214행(L1 101/port113), fixture101/reflected-commit 유지다. 전체 **1,860개(Assets363+Core1030+Client467) 통과**, 실패/건너뜀0·Release 전체 경고/오류0이다. [구현과 한계](docs/dotnet-menu-draw-20261029.md). cpppj·보호 경로·공통 인계 변경과 게임/창 실행은 없다.
- **다음:** MenuGumpInput의 선택 통지/하위 메뉴 열기·닫기/명령·부모 통지/팝업 삭제/변경 영역을 같은 실제 사건/캡처/트리에 연결한다. 전체 자산/ConfigInterface/음향/GIF/실제 메뉴·호스트와 R4 이후 전체 미션/세션/HUD/Edit는 남는다. 독립 PE Draw 관찰/실제 메뉴 캡처/L1 추가0·기존 코드/검사 은퇴0이며 전체 목표는 계속 진행 중이다.

### 0.48 단위 기록 — R3h-4f 메뉴 입력 (2026-10-29, HJOW-Athlon)

- **착수/구현/검증:** 출력 단위를 `f4f2c483`로 커밋한 뒤 같은 cpppj MenuGumpInput.h/.cpp를 읽고 선택/자식 가림·열기/닫기·명령/부모 통지·팝업 삭제·변경 영역과 현재 가상 분배/음향 인자를 작성했다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon이다. Client Release 컴파일 경고/오류0이며 실제 사건/캡처/트리/수명/출력에 연결한 집중19개가 통과했다. 최초 새 검사2개의 잘못된 자식 입력/캡처 중 Kill 후 현재 반환값 가정을 고쳤으며 제품/원본 기대값 변경은 없다. 포트216행(L1 101/port115), fixture101/reflected-commit 유지다. 전체 **1,879개(Assets363+Core1030+Client486) 통과**, 실패/건너뜀0·Release 전체 경고/오류0이다. [구현과 범위](docs/dotnet-menu-input-20261029.md). cpppj·보호 경로·공통 인계 변경과 게임/창 실행은 없다.
- **다음:** 메뉴 본체0.43~0.48은 연결했으며 R3의 남은 텍스트 입력/이미지/HTML·대화창·메뉴 상위 소유/분배와 전체 자산/ConfigInterface/음향/GIF/실제 호스트를 cpppj 의존성 순서로 조립한다. R4 이후 전체 미션/세션/월드/HUD/Edit·후순위 멀티플레이 사전 구현은 남는다. 독립 PE 입력 관찰/실제 메뉴 캡처/L1 추가0·기존 코드/검사 은퇴0이며 전체 목표는 계속 진행 중이다. 실제 금지 재현/멀티플레이 검사와 외부 TODO.md 수정은 없다.

### 0.49 단위 기록 — R3i-1 문자 편집 버퍼/소유 저장소 (2026-10-29, HJOW-Athlon)

- **착수/구현/검증:** AGENTS.md·현행 인계/공통 제약을 다시 읽었다. 직전 목표 턴은 메뉴 입력/출력2단위의 구현·검증·커밋을 완료한 진행이며 시작 HEAD는 `a4b04553`이다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon이다. TextEntryBuffer의 실제10DWORD·주소/선택/줄 표/클립보드와 TextEntryStorage의 안정된 객체/본문/줄 표 소유를 작성했고 Client Release 컴파일 경고/오류0이다. 원본570행의 전체10DWORD/메모리·가드·반납/출력/효과/의미 반환·명시 중단10건이 일치했고 실제 소유 저장소8개를 더한 집중9개가 최초부터 통과했다. 포트220행(L1 103/port117)의 편집 버퍼2행만 L1이며 저장소2행은 port다. fixture102개/새 원문571행·reflected-commit 유지다. 전체 **1,888개(Assets363+Core1030+Client495) 통과**, 실패/건너뜀0·Release 전체 경고/오류0이다. [구현과 범위](docs/dotnet-text-entry-buffer-20261029.md). cpppj/보호 경로/공통 인계 변경과 게임/창 실행은 없다.
- **다음:** TextEntryGump의 생성/Draw/캐럿·선택/키보드·마우스/클립보드·소유 수명에 이 버퍼를 연결한다. 이미지/HTML·대화창·실제 메뉴 상위 소유/분배/자산/음향/GIF/호스트 및 R4 이후 전체 미션/세션/월드/HUD/Edit·후순위 멀티플레이 사전 구현은 남는다. 실제 게임/메뉴 캡처0·기존 코드/검사 은퇴0이며 전체 목표는 계속 진행 중이다. 금지된 실제 플레이 재현/멀티플레이 검사와 외부 TODO.md 수정은 없다.

### 0.50 단위 기록 — R3i-2 문자 입력창/Console (2026-10-29, HJOW-Athlon)

- **착수/구현/검증:** R3i-1을 `39ec3a5a`로 커밋한 뒤 cpppj TextEntryGump.h/.cpp·TextEntryGumpTests.cpp·RawTextEntryControls.h/.cpp·TextEntryClientTests.cpp를 읽었다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon이다. 같은 기본/배경·편집 버퍼/두 Slider를 쓰는 생성/부착·스크롤·원시 입력/Shift 선택·클립보드·Console 자동완성/제출·선택/캐럿 출력·점멸/소멸을 작성했다. 원본2292행의 전체48DWORD/두 자식/10DWORD·가드 포함 메모리/효과/의미 반환이 일치한다. 최초 검사1건은 검사 입력의 nullable ReadOnlyMemory 변환을 명시해 수정했으며 제품/원본 기대값 변경은 없다. Client Release 컴파일 오류/경고0이다. 실제 공통 트리/Slider/글꼴·픽셀/수명 연결을 작성했고 통합6개+원본전수1개의 집중7개가 통과했다. 조립의 선택 인자 메서드 그룹/검사 속성명 컴파일 오류와 실제Slider 페이지 제한/클립 출력의 검사 가정2건을 고쳤다. 전체 **1,895개(Assets363+Core1030+Client502) 통과**, 실패/건너뜀0이다. Release 전체 빌드는 경고/오류0으로 종료했다. [구현과 검증](docs/dotnet-text-entry-gump-20261029.md). fixture103개(새 원문2293행), 포트222행(L1 105/port117)·reflected-commit 유지다. 실제 게임/창/시스템 클립보드 실행과 보호 경로/cpppj/공통 인계 변경은 없다.
- **다음:** R3j ImageGump의 SHP/색표/16.16 배율·투명 적중과 기존 inventory-image 원본221행을 연결한다. 이어 이미지/GIF·HTML/대화창·실제 메뉴 상위 소유/분배·전체 자산/ConfigInterface/음향/호스트를 조립한다. RawTextEntryControls의 전체 소유·부분 생성 정리/시스템 클립보드와 Save as·숫자 창/ChatWindow도 해당 조립에서 처리한다. R4 이후 전체 미션/월드/HUD/Edit·후순위 멀티플레이 사전 구현은 남는다. 실제 전체 메뉴/게임 캡처0·기존 코드/검사 은퇴0이며 전체 목표는 계속 진행 중이다. 외부 TODO.md 수정은 없다.

### 0.51 단위 기록 — R3j-1 이미지 UI/SHP·색표·투명 적중 (2026-10-29, HJOW-Athlon)

- **착수/구현/검증:** AGENTS.md·현행 인계/공통 제약을 다시 읽었다. 직전 목표 턴은 문자 편집 버퍼/입력창2단위의 구현·검증·커밋을 완료한 진행이며 시작 HEAD는 `9813ecd5`다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이다. cpppj ImageGump.h/.cpp·ImageGumpTests.cpp·inventory-image 원본 자료를 읽고 같은 기본/파생 생성·현재TYPE/SHP 기준점·단위 비트/16.16·그림자/선택/비활성/현재owner색·현재 필드 재조회·200×200 원시선형index/255 적중을 작성했다. 실제TYPE/SHP 명시 연결과같은VFX/공통트리·출력도 연결했다. 원본221행의 전체0xb4바이트/효과순서/반환 전수가 최초부터 일치했으며 Client Release 컴파일 경고/오류0이다. 실제 썬 캐논/골렘 자료의배율별6272점출력/적중·클립/색표/guard와 실제공통수명 등 집중7개가 최초부터 통과했다. 포트224행(L1 107/port117), fixture104개(새원문221행)/reflected-commit 유지다. [구현과 검증](docs/dotnet-image-gump-20261029.md). 전체 **1,902개(Assets363+Core1030+Client509) 통과**, 실패/건너뜀0이다. Release 전체 빌드는 경고/오류0으로 종료했다. 게임/창/금지된 실제 플레이 재현/멀티플레이 검사와 보호 경로/cpppj/공통 인계 변경은 없다.
- **다음:** MetaImageGump의 현재 동적Image/생성·기하·가상출력을 잇고 이미지/GIF·HTML/대화창·실제메뉴상위 소유/분배·전체자산/ConfigInterface/음향/호스트를 조립한다. R4 이후 전체미션/월드/HUD/Edit·후순위멀티플레이 사전구현은 남는다. 실제전체화면/게임캡처0·기존코드/검사은퇴0이며 전체 목표는 계속 진행 중이다. 외부TODO.md 수정은 없다.

### 0.52 단위 기록 — R3j-2 동적 MetaImage 생성/기하/출력 (2026-10-29, HJOW-Athlon)

- **착수/구현/검증:** 이미지UI 단위를 `8eece422`로 커밋한 뒤 cpppj MetaImageGump.h/.cpp·MetaImageGumpTests.cpp와 기존 RawImage를 읽었다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon이다. 같은 RawImage를 빌리는 생성/Resize/기본부착 뒤 기하·두float비율/reciprocal·현재Image/단위/배율/색표출력을 작성했다. 매출력 실제동적shape를읽어 같은VFX/클립/공유색표에연결하고 공통가상Attach/Draw와빌린Image를반납하지않는기본수명을연결했다. 독립원본326행의기하/등록/효과가일치하고 실제조립을포함한집중6개가통과했다. 검사코드var다중선언/nullable경고와실제출력의+1좌표·표면초기화가정을고쳤다. 제품/원본기대값변경없음·최종Client컴파일경고/오류0이다. 포트226행(L1 109/port117), fixture105개(새원문327행)/reflected-commit 유지다. 전체 **1,908개(Assets363+Core1030+Client515) 통과**, 실패/건너뜀0이다. Release 전체 빌드는 경고/오류0으로 종료했다. [구현과검증](docs/dotnet-meta-image-gump-20261029.md). 게임/창/보호경로/cpppj/공통인계 변경과금지된실제플레이재현/멀티플레이검사는없다.
- **다음:** MetaImageList의21칸 실제목록/자식소유·기하/선택/적중/삭제와원본514행을연결한다. 이어GIF/HTML·대화창·실제메뉴상위소유/분배·전체자산/ConfigInterface/음향/호스트와R4이후 전체미션/월드/HUD/Edit·후순위멀티플레이사전구현을잇는다. 실제전체메뉴/게임캡처0·기존코드/검사은퇴0이며 전체목표는 계속 진행 중이다. 외부TODO.md수정은없다.

### 0.53 단위 기록 — R3j-3 실제 MetaImage 목록/선택/수명 (2026-10-29, HJOW-Athlon)

- **착수/구현/검증:** AGENTS.md·현행/공통 인계를 다시 읽었다. 직전 목표 턴은 ImageGump/MetaImageGump2단위의 구현·검증·커밋을 완료한 진행이며 시작 HEAD는 `27075f9b`다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이다. cpppj MetaImageList.h/.cpp·MetaImageListTests.cpp를 읽고 같은 실제21칸/ID·자식생성·가로/grid배치·현재count/셀 재조회·원시적중/선택/통지·체크무늬·접두부삭제/끝슬롯보존/수명을 작성했다. 독립514행의 전체tail/실제MetaImage22개 기하/기본등록/효과순서/반환이 최초부터 일치했다. Client Release 컴파일 경고/오류0이다. 실제 공통트리/21개 자식생성·픽셀/입력/지연수명 연결을 포함한 집중8개가 최초부터 통과했다. 포트228행(L1 111/port117), fixture106개(새원문515행)/reflected-commit 유지다. 전체 **1,916개(Assets363+Core1030+Client523) 통과**, 실패/건너뜀0이다. Release 전체 빌드는 경고/오류0으로 종료했다. [구현과 검증](docs/dotnet-meta-image-list-20261029.md). 게임/창/금지된플레이재현/멀티플레이검사와 cpppj/보호경로/공통인계 변경은 없다.
- **다음:** BackgroundGifGump의 원시GIF버퍼/소유·현재치수/입력/복사와GIF판독기·실제자산을 연결한다. 이어HTML/대화창·실제메뉴 상위소유/분배·전체자산/ConfigInterface/음향/호스트와R4 이후 전체미션/월드/HUD/Edit·후순위멀티플레이 사전구현은 남는다. 실제전체메뉴/게임캡처0·기존코드/검사은퇴0이며 전체 목표는 계속 진행 중이다. 외부TODO.md 수정은 없다.

### 0.54 단위 기록 — R3k-1 배경 GIF 창/판독/원시 버퍼 (2026-10-29, HJOW-Athlon)

- **진행:** AGENTS.md·현행/공통 인계와 현재/마지막 디컴파일 PC(HJOW-Athlon)를 확인했다. 시작 HEAD는 `a1680825`다. BackgroundGifGump.h/.cpp와 RawBackgroundGifControls의 저장소 부분, GifImage.cpp 및 대응 검사를 읽고 배경 전용 descriptor 정규화·현재 공유 노드/외부 효과 순서·raw 입력/복사·독립 버퍼 소유를 작성했다. 독립 원본308행(명시 중단16행 포함)이 일치했다. 중단 중 ESP는 함수 복귀가 아니므로 완료 행에만 RET 스택 복귀량을 비교하도록 검사 범위를 고쳤으며 제품/fixture 기대값은 바꾸지 않았다. 현재 실제 자산34개/변형4개와 공통 UI/수명 연결을 검증 중이다.
- **검증 완료:** 보호 GIF34개/변형4개의 소스·픽셀 SHA-256/FNV와 치수가 독립 원본과 일치했다. 실제 공통 트리/입력/원시 큐·삭제 재진입·raw 픽셀/클립/padding/stride·동시 소유/교체·부분 생성 예외와 지연 해제까지 집중8개가 통과했다. 검사 입력의 FromPairs 중괄호 이스케이프를 실제 ConfigText로 고쳤으며 제품 경로 대체값은 만들지 않았다. 포트233행(L1 115/port118), fixture109개, reflected-commit 유지다. 전체 **1,924개(Assets363+Core1030+Client531) 통과**, 실패/건너뜀0·Release 전체 빌드 경고/오류0이다. [구현과 검증](docs/dotnet-background-gif-gump-20261029.md).
- **다음:** HTML/대화창·실제 메뉴 상위 소유/분배·전체 자산/ConfigInterface/음향/호스트와 R4 이후 전체 미션/월드/HUD/Edit·후순위 멀티플레이 사전 구현을 잇는다. 전체 RawBackgroundGifControls 노드/창 소유자와 실제 호스트/메뉴 캡처는 후속이며 R3 전체 완료로 세지 않는다. 실제 전체 메뉴/게임 캡처0·기존 코드/검사 은퇴0이다. 게임/창·금지된 플레이 재현·멀티플레이 검사와 cpppj/보호 경로/공통 인계/외부 TODO.md 변경은 없다. 전체 목표는 계속 진행 중이다.

### 0.55 단위 기록 — R3k-2 HTML 본문/조건/그림/배경 (2026-10-29, HJOW-Athlon)

- **착수:** 배경 GIF 단위를 `fb0f7378`(DN51)로 커밋한 뒤 같은 목표를 이어간다. HtmlTextParser/HtmlConditionEffect·공통 RawTextToken과 대응 cpppj 검사를 읽었다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이다. 기본 HTML→StyleText 바이트·현재 커서/가림 상태·그림 제어문자·80BYTE 배경·조건/토큰/현재 Config 치환 순서를 이식하고 독립816/648/276/754행을 연결한다.
- **구현/검증 완료:** 기본/그림/배경3개 전수는 최초 통과했다. 조건 검사의 동작 이름 `Parse`를 실제 입력 `Html`로 고친 뒤 제품/기대값 수정 없이754행도 통과했다. 총2,494행과 실제 Config/현재값·StyleText 색/굵기·그림 생성/해제·글리프 픽셀/배경 GIF를 포함한 집중12개가 통과했다. 포트239행(L1 121/port118)·fixture113개, reflected-commit 유지다. 전체 **1,936개(Assets363+Core1030+Client543) 통과**, 실패/건너뜀0·전체 Release 빌드 경고/오류0이다. [구현과 검증](docs/dotnet-html-text-parser-20261029.md).
- **다음:** HtmlLinkText/QuickHelpHtml·HtmlBodyGump/HtmlWindowGump와 창 생성/레이아웃·실제 메뉴 상위 소유/분배·전체 자산/ConfigInterface/음향/호스트를 잇는다. 명령 큐/링크 저장소·파생 TextHtml과 전체 ConfigInterface 임시local 객체 소유도 남으며 미연결 효과를 대체 성공으로 만들지 않는다. R4 이후 전체 미션/월드/HUD/Edit·후순위 멀티플레이 사전 구현은 남는다. 게임/창·금지된 플레이·멀티플레이 검사와 cpppj/보호 경로/공통 인계/외부 TODO.md 변경은 없다. 실제 전체 메뉴/게임 캡처0·기존 코드/검사 은퇴0이며 전체 목표는 계속 진행 중이다.

### 0.56 단위 기록 — R3k-3 HTML 링크/전용 도움말 (2026-10-29, HJOW-Athlon)

- **착수:** 직전 목표 턴은 GIF/HTML2단위의 구현·검증·커밋을 마친 진행이다. AGENTS.md·현행/공통 인계를 다시 읽었고 시작 HEAD `c58ddc1f`·깨끗한 작업 트리·현재/마지막 전체 정밀 디컴파일 PC(HJOW-Athlon)를 확인했다. HtmlLinkText/QuickHelpHtml와 대응83행/중첩3행 검사, 실제 도움말/기존 StyleText·다중 줄을 대조하고 링크의 연속 NUL 문자열·마나 그림·주제 prefix/현재 TYPE·중첩 본문/스트림/정보 pane 순서를 옮긴다.
- **구현/검증 완료:** 원본83/중첩3행은 최초 일치했다. 실제 링크/다중 줄/적중/글리프·현재 TYPE·중첩 모드와 버퍼 경계를 더한 집중10개도 통과했다. 검사 입력의 비활성 링크 helper 연결과 그림자/strict 위 경계 적중 좌표만 고쳤고 제품/원본 기대값은 바꾸지 않았다. 포트244행(L1 125/port119)·fixture115개이며 reflected-commit은 유지한다. 전체 **1,946개(Assets363+Core1030+Client553) 통과**, 실패/건너뜀0·전체 Release 빌드 경고/오류0이다. [구현과 검증](docs/dotnet-quick-help-html-20261029.md).
- **다음:** HtmlBodyGump/내장 Tell·QuickHelpWindow/HtmlWindowGump 생성/배치와 현재 표시 자식/링크 수명·실제 메뉴 상위 소유/분배·전체 자산/ConfigInterface/음향/호스트를 잇는다. ReadAssetQuickHelpType의 전체 GameAssets TYPE/AST·requiredEnergy 후처리와 R4 이후 전체 미션/월드/HUD/Edit·후순위 멀티플레이 사전 구현은 남는다. 게임/창·금지된 플레이·멀티플레이 검사와 cpppj/보호 경로/공통 인계/외부 TODO.md 변경은 없다. 실제 전체 메뉴/게임 캡처0·기존 코드/검사 은퇴0이며 전체 목표는 계속 진행 중이다.

### 0.57 단위 기록 — R3k-4 HTML 부모/내장 Tell (2026-10-29, HJOW-Athlon)

- **착수:** 같은 목표에서0.56을 `ecb9802a`(DN53)로 커밋했다. HtmlBodyGump/EmbeddedTell·원본82행·RawMenuControls의 실제 RenderEmbeddedTell과 기존 다중 줄 생성/버퍼/입력/수명을 읽었다. 두8192BYTE 소유/현재 자식 교체·가상 부착/배율 치수·현재 Mission→매번 새 Tell 조회→생성/현재serial, 파서 효과 전후/예외 때 실제 본문 접두부를 게시하는 경로를 옮긴다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이다.
- **구현/검증 완료:** 독립82행의 현재 serial/부모·전체 두8192BYTE 지문/효과 순서가 최초부터 일치했다. 실제 기본 HTML/링크/스크롤 자식/보호 글꼴/픽셀·입력/지연 수명/현재 Config·부분 예외와 기하를 더한 집중9개도 최초 통과했다. 포트249행(L1 129/port120)·fixture116개, reflected-commit 유지다. 전체 **1,955개(Assets363+Core1030+Client562) 통과**, 실패/건너뜀0·전체 Release 빌드 경고/오류0이다. [구현과 검증](docs/dotnet-html-body-embedded-tell-20261029.md).
- **다음:** QuickHelpWindow의 전체 생성/배치/요청·이력/입력과 HtmlWindowGump/Factory/Metadata/Fit·실제 메뉴 상위 소유/분배·전체 자산/ConfigInterface/음향/호스트를 잇는다. ReadAssetQuickHelpType/요구 에너지와 ConfigInterface 전체 등록부/임시 객체 수명, R4 이후 전체 미션/월드/HUD/Edit·후순위 멀티플레이 사전 구현은 남는다. 게임/창·금지된 플레이·멀티플레이 검사와 cpppj/보호 경로/공통 인계/외부 TODO.md 변경은 없다. 실제 전체 메뉴/게임 캡처0·기존 코드/검사 은퇴0이며 전체 목표는 계속 진행 중이다.

### 0.58 단위 기록 — R3k-5 도움말 열기/요청/이력/입력 (2026-10-29, HJOW-Athlon)

- **착수:** 직전 목표 턴은 전용 도움말 본문/내장 Tell2단위의 구현·검증·커밋을 완료한 진행이다. AGENTS.md·현행/공통 인계를 다시 읽었고 시작 HEAD `2aef224e`·깨끗한 작업 트리·현재/마지막 전체 정밀 디컴파일 PC(HJOW-Athlon)를 확인했다. QuickHelpControl/QuickHelpRequest와 대응150/985행 검사·전용 창 생성/배치 의존성을 읽고 동일 창 재사용/모달 관문·요청 접미/현재 타입 조회·연속 중복 이력/공유80BYTE 팝업·현재 버튼/페이지/링크/Back 입력을 옮긴다.
- **구현/검증 완료:** 원본150/985행은 최초부터 일치했다. 실제 공통 등록/트리/입력/dialog 전달·요청/이력/Back·현재 popup·같은 BYTE 인자/원시 큐·자식 우선 지연 수명과 현재 모달/버퍼 경계를 더한 집중8개도 최초 통과했다. 포트253행(L1 133/port120)·fixture118개, reflected-commit 유지다. 전체 **1,963개(Assets363+Core1030+Client570) 통과**, 실패/건너뜀0·전체 Release 빌드 경고/오류0이다. [구현과 검증](docs/dotnet-quick-help-control-20261029.md).
- **다음:** QuickHelpWindow의 전체 자식 생성/배치·본문/그림 갱신과 HtmlWindowGump/Factory/Metadata/Fit·실제 메뉴 상위 소유/분배·전체 자산/ConfigInterface/음향/호스트를 잇는다. RawMenuControls 전체 모달/등록 소유자·링크/명령 소비자와 R4 이후 전체 미션/월드/HUD/Edit·후순위 멀티플레이 사전 구현도 남는다. 게임/창·금지된 플레이·멀티플레이 검사와 cpppj/보호 경로/공통 인계/외부 TODO.md 변경은 없다. 실제 전체 메뉴/게임 캡처0·기존 코드/검사 은퇴0이며 전체 목표는 계속 진행 중이다.

### 0.59 단위 기록 — R3k-6 전용 도움말 창 생성/배치/본문 (2026-10-29, HJOW-Athlon)

- **착수:** 같은 목표에서0.58을 `c6b19afd`(DN55)로 커밋했다. QuickHelpWindow.h/.cpp·원본84/추가14경로와 대응 전체 노드/효과 검사, 기존 HTML 부모/전용 파서·실제 장식/배경/돌 버튼/이미지/다중 줄 장치를 읽었다. 세 배경·버튼/HTML 부모의 현재 생성, 캐시/표식/기본 주제·본문 치환/그림 선택·현재 자식 배치를 옮긴다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이다.
- **구현/검증 완료:** 원본84/추가14경로의12개 전체 기본 노드/창12DWORD·캐시/표식/효과가 최초부터 일치했다. 현재 자식/배경/프레임·치환 전 배열 유지·부분 실패와 실제 전용 HTML/부모/스크롤/글꼴/수명을 더한 집중10개도 최초 통과했다. 포트255행(L1 135/port120)·fixture120개, reflected-commit 유지다. 전체 **1,973개(Assets363+Core1030+Client580) 통과**, 실패/건너뜀0·전체 Release 빌드 경고/오류0이다. [구현과 검증](docs/dotnet-quick-help-window-20261029.md).
- **다음:** HtmlWindowGump/Factory/Metadata/Fit·MessageGump/대화창·실제 메뉴 상위 소유/분배를 잇는다. 전체 위젯/자산 동시 조립·TYPE/요구 에너지·ConfigInterface/RawMenuControls의 등록/모달/링크 소비자·음향/호스트와 R4 이후 전체 미션/월드/HUD/Edit·후순위 멀티플레이 사전 구현은 남는다. 게임/창·금지된 플레이·멀티플레이 검사와 cpppj/보호 경로/공통 인계/외부 TODO.md 변경은 없다. 실제 전체 메뉴/게임 캡처0·기존 코드/검사 은퇴0이며 전체 목표는 계속 진행 중이다.

### 0.60 단위 기록 — R3k-7 일반 HTML 창 수명/본문/전체 배치 (2026-10-29, HJOW-Athlon)

- **착수/구현:** AGENTS.md·현행/공통 인계를 다시 읽었다. 직전 목표 턴은 도움말 제어/전용 창2단위의 구현·검증·커밋을 완료한 진행이다. 시작 HEAD는 `5509df94`이고 작업 트리는 깨끗했다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이다. HtmlWindowGump/Fit·대응 원본250/88/160행과 RawMenuHtml의 실제 장치 연결을 읽고 같은 장식 파생/8192BYTE·20행/기한·생성/본문/명령/소멸·현재 크기/메뉴/버튼/화면 배치를 이식했다. 같은 장식 노드의 상속만 열고 실제 위젯/트리/수명·메뉴 어댑터를 작성했다. 도중 외부CP `f9a8cfa5`가 들어왔고 공통 인계/데모 보고서만 변경했다. 제품 근거 cpppj 소스는 그대로다.
- **검증 완료:** 독립498행의 전체필드/행/8192BYTE·자식/메뉴스택/현재전역/효과/반환이 일치했다. 최초 검사의 TSV 머리행 처리·조립 속성명/var 선언 컴파일 오류와 실제 전체Fit의 중앙정렬을 놓친 픽셀 검사 입력을 고쳤으며 제품/원본 기대값 변경은 없다. 동적본문/돌버튼/장식/보호글꼴·원시큐/OnExit/현재보조창/Idle·두열메뉴/공통수명 포함 집중10개 통과다. 전체 **1,983개(Assets363+Core1030+Client590) 통과**, 실패/건너뜀0·전체Release 경고/오류0·종료0이다. 포트259행(L1 139/port120)·fixture123개, reflected-commit 유지다. [구현과 검증](docs/dotnet-html-window-20261029.md).
- **다음:** HtmlWindowMetadata/Factory·MessageGump/대화창·실제 상위 메뉴/전체 자산 소유·호스트 및 R4 이후 전체 미션/월드/HUD/Edit·후순위 멀티플레이 사전 구현은 남는다. 게임/창·금지된 플레이·멀티플레이 검사와 이 단위의 cpppj/보호 경로/공통 인계 변경은 없다. 실제 전체메뉴/게임캡처0·기존코드/검사은퇴0이며 전체 목표는 계속 진행 중이다.

### 0.61 단위 기록 — R3k-8 HTML 메타/전체 생성/링크·모달 연결 (2026-10-29, HJOW-Athlon)

- **착수/구현:** AGENTS.md·현행/공통 인계를 다시 읽었다. 직전 목표 턴은 일반 HTML 창 수명/전체 배치의 구현·검증·커밋을 완료한 진행이다. 시작 HEAD는 `9ad3ff97`이고 작업 트리는 깨끗했다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이다. HtmlWindowMetadata/Factory·DialogExitCommand/ScriptStringRing·RawMenuHtml의 파서/모달 및 대응 Native/Client 검사를 읽었다. 고정9종 검색/현재8192BYTE·20버튼/열수ref·기한/OnExit·두번파서/기본버튼/빈본문/현재serial·즉시링크게시/모달최소깊이를 이식하고 같은 실제 UI/Config/메뉴/본문/고리/원시큐/수명을 연결했다.
- **검증 완료:** 원본메타124행/전체생성106행/명령해석204행은 최초부터 일치했다. 사용자 무시험 방침에 따라 Zsend 입력4행은 실행 전 제외했고 전송은 호출 경계만 작성했다. 새 검사19개와 기존HTML10개가 통과했다. 실제 조립 검사의 Route 서명 컴파일 오류를 고쳤고, 두 실제 입력 검사에서 누락된 기본버튼 BaseEvent를 기존GumpHelp에 연결했다. 제품/원본 기대값을 약화하지 않았다. 전체 **2,002개(Assets363+Core1030+Client609) 통과**, xUnit 실패/건너뜀0·전체Release 경고/오류0·종료0이다. 모든 검사/빌드 핸들은 정상 종료했고 이 작업의 살아 있는 게임/검사/빌드는 없다. 포트269행(L1 145/port124)·fixture126개, reflected-commit 유지다. [상세 구현/제외/검증](docs/dotnet-html-factory-20261029.md).
- **사용자 요청에 따른 중지:** “문서 작성 및 커밋 후 목표 일시 정지”를 받았다. 이 단위 인계와 로컬 커밋을 마친 뒤 목표를 일시 정지하며 다음 구현 단위는 시작하지 않는다. 재개 후 실제 QueueScriptCommand/ApplyScriptConfig·전체 ConfigInterface·맵/파일/기술 목록 owner, 메시지/대화창·상위 메뉴/전체 자산/음향/호스트를 잇는다. R3/D1 전체 메뉴 캡처와 R4 이후 전체 미션/월드/HUD/Edit/설정/배포·후순위 멀티플레이 사전 구현은 남는다. 전체 목표를 완료로 세지 않는다. 게임/창·금지된 플레이·멀티플레이 검사와 cpppj/보호 경로/공통 인계/외부TODO.md 변경은 없다. 전체메뉴/게임캡처0·기존코드/검사은퇴0이다.

### 0.62 단위 기록 — R3k-9 스크립트 명령·Config 적용 (2026-10-30, HJOW-Athlon)

- **착수/구현:** AGENTS.md·현행/공통 인계를 다시 읽었다. 사용자 요청("dotnetpj 다음 작업 진행해 줘")으로 0.61의 일시 정지를 풀고
  R3k 다음 순서(명령/ConfigInterface 조립)를 재개했다. 시작 HEAD는 `d2300fea`이고 작업 트리는 깨끗했다(병렬 CP·WB 세션의
  cpppj·webpj·LEFT_JOBS.md 변경은 건드리지 않았다). 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이다.
  cpppj `ScriptCommand.h/.cpp`와 `ScriptCommandTests.cpp`를 읽고 `QueueScriptCommand`·`ApplyScriptConfig`(쓰기 주입 overload)를
  `Netstorm.Client/Scripting/ScriptCommand.cs`로 이식했다. `InputQueue.PushCommand`·`ScriptStringRing`·`ConfigNumbers.ParseLong`·
  `RawTextToken.Read`는 기존 C# 구현을 그대로 연결했다.
- **검증 완료:** `script-command-x86.tsv` 262행(Token 본문·다음 위치, Config 쓰기·없음 알림·반환, Command 큐 사건·고리 위치·진단 수,
  Ring 저장·해석·위치·80칸 무효화)이 최초 실행부터 일치했다. 변이 1건(`0xffffffff`→`0xfffffffe`)에 L1 기대값 실패를 확인하고
  원복했다. 숫자 판별 하한 변이는 fixture에 `0` 시작 인자가 없어 걸리지 않았으므로 판별력 근거로 쓰지 않는다.
  전체 **2,003개(Assets 363 + Core 1030 + Client 610) 통과**, xUnit 실패/건너뜀 0, 전체 Release 경고/오류 0이다.
  포트 271행(`L1` 147/`port` 124), fixture 127개, reflected-commit 유지다. [단위 문서](docs/dotnet-script-command-20261030.md).
- **다음:** `ApplyScriptConfig`의 `ConfigInterface` 직접 연결 overload와 두 번째 검사 상당(`ConfigOverwriteAndQueueLifetime`),
  전체 `ConfigInterface` 이식, 목록 owner·메시지/대화창·상위 메뉴/전체 자산/호스트 조립. R3 전체 완료로 세지 않는다.
  게임/창 실행과 디컴파일은 하지 않았다. cpppj·원본·CD·AGENTS.md·공통 인계·webpj 변경은 없다.

### 0.63 단위 기록 — R3k-10 설정 객체 층 (2026-10-30, HJOW-Athlon)

- **착수/구현:** AGENTS.md·현행/공통 인계를 다시 읽었다. 사용자 요청("AGENTS.md, LEFT_JOBS.dotnetpj.md 를 읽고 dotnetpj 다음 작업 진행해 줘")으로
  0.62의 다음 순서(전체 ConfigInterface 조립)를 재개했다. 시작 HEAD는 `b1c2261d`이고 작업 트리는 깨끗했다(병렬 CP·WB 세션의
  cpppj·webpj·LEFT_JOBS.md 변경은 건드리지 않았다). 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이다.
  cpppj `o/Config.h/.cpp`와 `ConfigOracleTests.cpp`를 읽고 설정 객체 층(`ConfigText`·`ConfigRegistry`·`Config`·자유 함수)을
  `Netstorm.Core/Original/Config.cs`로 이식했다. `DecodeOriginalText` 자리는 바이트 단위가 같은 `OriginalText.DecodeWindows1252`로 두고,
  XOR 키는 기존 `XorCipher.Apply`를 썼다. `FindSection`의 원본 nullptr 분기는 이름 없는 overload 분리로 재현했다.
- **검증 완료:** `config-x86.tsv` 2440행(치환 495·조회 77·삭제 272·쓰기 143·섹션머리 1045·섹션 408, Def 2행 제외)이 일치했다.
  검사 작성 중 실패 4건은 모두 테스트의 오류였고(조회 오프셋·쓰기 오프셋·섹션 위치 계산·`out` 변수 확정, 소스 수정 없음) 원본과 대조해 고쳤다.
  섹션 1건은 제품 오류처럼 보였으나 원본 nullptr 분기의 재현이었고 overload 분리로 원본과 일치시켰다.
  변이 1건(`Set`의 변경 표시 제거)에 L1 기대값 실패를 확인하고 원복했다.
  전체 **2,004개(Assets 364 + Core 1030 + Client 610) 통과**, xUnit 실패/건너뜀 0, 전체 Release 경고/오류 0이다.
  포트 273행(`L1` 149/`port` 124), fixture 127개(새 등록 없음 — 같은 fixture를 쓰는 Assets.Tests에 검사를 두었다), reflected-commit 유지다.
  [단위 문서](docs/dotnet-config-object-20261030.md).
- **다음:** `ConfigInterface`(Startup·GetInt·PathSpec·SubstituteLocal·LoadLanguage·SaveText·SaveFile)와 `ConfigInterfaceTests` 4종,
  `ApplyScriptConfig`의 `ConfigInterface` 직접 연결과 두 번째 검사 상당. R3 전체 완료로 세지 않는다.
  게임/창 실행과 디컴파일은 하지 않았다. cpppj·원본·CD·AGENTS.md·공통 인계·webpj 변경은 없다.
- **알아 둘 것:** `ConfigText` 이름이 `Assets.ConfigText`와 겹친다. 기존 `HtmlTextParserTests`·`HtmlTextParserNativeTests` 2개 파일에
  기존 뜻 유지용 using 별칭 1줄씩만 추가했고 동작 변경은 없다. fixture 소비자 규칙(한 fixture 한 소비자·프로젝트별 출력 검사) 때문에
  새 L1 검사는 `config-x86.tsv`의 소비자인 Assets.Tests에 두었으며, 이를 위해 Assets.Tests에 Core 프로젝트 참조를 추가했다(테스트→제품 참조라 순환 없음).
  `PORT_MAP.tsv`·`cpp-fixture-manifest.tsv`의 `config-x86` 행은 그대로다.

### 0.64 단위 기록 — R3k-11 ConfigInterface 본체 (2026-10-31, HJOW-Athlon)

- **착수/구현:** AGENTS.md·현행/공통 인계를 다시 읽었다. 사용자 요청("AGENTS.md, LEFT_JOBS.dotnetpj.md 를 읽고 dotnetpj 다음 작업 진행해 줘",
  cpppj 쪽 파일 수정 금지 준수)으로 0.63의 다음 순서(ConfigInterface 본체)를 재개했다. 시작 HEAD는 `760e5ec3`이고 작업 트리는
  깨끗했다(병렬 CP·WB 세션의 변경은 건드리지 않았다). 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이다.
  cpppj `o/ConfigInterface.h/.cpp`와 `ConfigInterfaceTests.cpp`를 읽고 `ConfigInterface`(Startup 순서·Load 인자·GetInt·PathSpec·
  ReadInt/ReadString·IntEquals·Set/SetInt·SubstituteLocal·LoadLanguage·SaveText/SaveFile·onChanged·갈고리)를
  `Netstorm.Core/Original/ConfigInterface.cs`로 이식했다. `ConfigParseLong`·`ConfigListItem`은 기존 `Assets/ConfigNumbers`를
  그대로 쓰고, `EncodeOriginalText`만 `Assets/OriginalText`에 추가했다. `ApplyScriptConfig`의 ConfigInterface 직접 연결 overload는
  `Netstorm.Client/Scripting/ScriptCommand.cs`에 이었다.
- **검증 완료:** 계약 검사 11종(`ConfigInterfaceTests` 10종: 시작 순서·타입 접근·미션 조회·저장 왕복·인코딩·파일 쓰기 2·인자/파일 읽기·
  정적 소유·한정 쓰기·임시 범위 + `ScriptCommandConfigTests` 1종: Config 덮어쓰기·큐 수명)이 최초 실행부터 통과했다.
  `ParseLong/ListItem` 1종은 기존 `ConfigNumbersTests`가, `MissionScript` 1종과 대화창 분리(`ParseDialogPage`) 부분은 해당 단위로 미뤘다.
  변이 1건(`Set`의 변경 알림 제거)에 계약 검사 2종의 실패를 확인하고 원복했다.
  전체 **2,015개(Assets 364 + Core 1040 + Client 611) 통과**, xUnit 실패/건너뜀 0, 전체 Release 경고/오류 0이다.
  포트 276행(`L1` 149/`port` 127), fixture 127개(새 등록 없음), reflected-commit 유지다.
  [단위 문서](docs/dotnet-config-interface-20261031.md).
- **다음:** `MissionScript` 상당(미션 객체·`fortSpec`·`missionType`·`tellAtList`)은 미션 객체 단위에서,
  `ConfigOverwriteAndQueueLifetime`의 대화창 분리(`ParseDialogPage`) 부분은 대화창 조립 단위에서 잇는다.
  목록 owner·메시지/대화창·상위 메뉴/전체 자산/호스트 조립과 R3 전체 완료는 남는다. 전체 목표를 완료로 세지 않는다.
  게임/창 실행과 디컴파일은 하지 않았다. cpppj·원본·CD·AGENTS.md·공통 인계·webpj 변경은 없다.
- **알아 둘 것:** `EncodeOriginalText` 추가분만 `o/OriginalText.h`→`Assets/OriginalText.cs` 행(port)으로 포트 맵에 올렸다.
  `ScriptCommand.cs`는 overload 추가로 바뀌어 base를 HEAD로 올렸다(같은 R3k-9 L1·신규 검사로 재대조).

## 1. 지금까지의 완료 범위 — 이관 기록 색인

단위별 상세는 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)와 아래 문서에 있다. 마지막 판정 열은 0절 계획에서 그 결과를 어떻게 다루는지다([0.4](#04-기존-dotnetpj-소스의-재수정-판정)).

| 기간·단위 | 완료 범위 | 근거 문서 | 0절에서의 처리 |
| --- | --- | --- | --- |
| 2026-09-27~10-05 | MonoGame 기반 초기 C# 구현(메인 메뉴·캠페인 1-1/1-2·다리·사제·전투·희생·덱 등 녹화·팬게임 기반), `dotnetpj/` 이동 | [이관 파일 14절](LEFT_JOBS.dotnetpj.old.md#legacy-dotnet-history) | 대부분 교체 |
| 2026-10-07~10-10 | 자료 계층 정정(설정·타입 표·요새·SHP 헤더), 다리·표면·그래프·SID·점유·표시 경계 계산, `.chfnt` 판독·커서 표·배경 GIF 색·미션 시작 값 | [10-07](docs/dotnet-reconstruction-20261007.md), [10-09](docs/dotnet-reconstruction-20261009.md), [10-10](docs/dotnet-reconstruction-20261010.md) | 판독기는 재대조, 계산은 `Original` 대응 모듈로 흡수 |
| 2026-10-11~10-13 | 9-1 A 첫 단계(프레임 명령·붕괴 스캔), 화면 루프·숫자 시계·세션 시계, 도움말 검증, 안내창·단계 프레임, 한국어 명칭 연결 | [frame-parity](docs/dotnet-frame-parity-20261011.md), [loop-parity](docs/dotnet-loop-parity-20261012.md), [clock-kernel](docs/dotnet-clock-kernel-20261012.md), [session-clock](docs/dotnet-session-clock-20261012.md), [dialog-frames](docs/dotnet-dialog-frames-20261013.md), [stage-frames](docs/dotnet-stage-frames-20261013.md) | 시계 계산은 유지 후 재대조, 나머지는 교체 |
| 2026-10-15 | 감사 D01(시간 소비 목록)·D02(미션 프레임 게이트)·D03(참여·동맹)·D05(튜토리얼 1 판정)·D14(Totalmade) | [감사](docs/dotnet-cpp-audit-20261015.md), [미션 프레임](docs/dotnet-mission-frame-gate-20261015.md), [참여·동맹](docs/dotnet-player-relations-20261015.md), [장부·튜토리얼 1](docs/dotnet-ledger-tutorial-one-20261015.md) | 유지 후 재대조 |
| W01 (10-16) | `cpp_sync.py`·`cpp-fixture-manifest.tsv`·`X86Fixture` 확장 | [이식 계획 7절](docs/dotnet-cpp-port-plan-20261016.md) | 유지. R0에서 소스 해시 추적 추가 |
| W02 1~18조각 (10-16~10-21) | `RawSidPool`·생성자 표·`RawSquidFactory`·`RawSquidHash`·`RawUnpop`·`RawPop`·`RawPostPop`·`RawCarrierPostPop`·`RawAssetPostPop`·소유자·프레임·건물 공간·탐색·이웃·다리 사건·미니맵 큐·`RawGraph`(표·영역·Detach·Rebuild·FullPool·PostDestroy) | [W02 첫 조각](docs/dotnet-original-core-20261016.md) 외 `docs/dotnet-original-core-*.md` | 유지. R0 재대조, R4 세션 조립 |
| W03 1~17조각 (10-21~10-23) | 공통 삭제·다리 수명·삭제 수명주기·삭제 그래프·Damageable 사전/해방/사후/피해·사제 피격·종속 조회·Form 수명·Form 패킷·내용물 수명·contained 생성·신전 삭제·Form 파생 분배·GumpForm 호스트·가상 삭제 재정의 | `docs/dotnet-original-core-*.md`(destroy·destroylifecycle·destroygraph·damageable*·priestdamage·containedlookup·formlifecycle·formpacket·contentlifetime·vortexpre·formdispatch·destroydispatch·assetpredestroy) | 유지. R0 재대조, R4 세션 조립 |
| W05 선행 (10-23) | `RawSpStore`·`RawSquidReward`·AI 지갑·난수 상태 | [SP 저장소·보상](docs/dotnet-original-core-spstore-reward-20261023.md) | 유지. R0 재대조, R4 세션 조립 |
| 배포 (10-23) | `package-dotnetpj.ps1` | 이관 파일의 2026-10-23 배포 절 | 유지. 구성이 바뀔 때마다 갱신 |
| R0 (10-28) | cpppj 오라클 확정, 재대조 영향표, 포트 맵(`PORT_MAP.tsv`)·`cpp_sync.py --ports*` | [영향표](docs/dotnet-cpp-resync-20261028.md), [PORT_MAP.md](dotnetpj/PORT_MAP.md) | 유지. R0-4 완료(0.14~0.21) |
| R1-1 (10-28) | `Netstorm.Client`: 8비트 `Screen`·팔레트·색 검색·화면 스크롤·전체화면 출력 계산·입력 큐 | [R1-1 문서](docs/dotnet-client-screen-20261028.md) | 새 경로(`port`) |

최신 전체 빌드·검사 결과는 0절의 마지막 단위 기록을 따른다. Assets·Core의 기존 경로 검사 수를 새 경로의 진행도로 세지 않는다. 새 경로의 진행도는 Client 검사와 0.5절의 실제 단계 완료 기준을 함께 본다.

---

<a id="original-parity-plan"></a>

## 2. 9-1 결정 — 의도적 차이는 원본처럼 동작하도록 바꾼다 (2026-10-10 사용자 결정)

> **2026-10-28 정리:** 옛 9절 표(dotnetpj가 일부러 다르게 둔 것)는 이관 파일로 옮겼다. 아래 결정문의 "위 표"는 그 표를 말하며, 같은 항목은 아래 A~F 표에 모두 있다. 아래 "이 결정으로 바꾸지 않는 것"의 MonoGame·슬롯 문단은 0절 결정 1·2로 갱신됐다. MonoGame은 창·입력·오디오 장치·최종 출력 계층으로 유지하고 게임 화면은 8비트 논리 화면에 그린다. cpppj가 원본 배치를 쓰는 곳은 C#도 같은 배치를 쓴다. A~F의 남은 처리 시점은 [0.7](#07-계속-유효한-결정제약)을 따르며, 옛 순서 제안(A → C → B → D·E·F)은 이관했다.

**결정:** 위 표의 차이는 원본처럼 동작하도록 바꾼다. **지금 구조를 둔 채 원본 게임의 경험을 거의 동일하게 만들 수 있으면 구조는 유지해도 되지만, 그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다.** 위 표의 "이유" 칸(요구사항·편의)은 더 이상 차이를 그대로 두는 근거가 아니다. 이 작업은 목표 순서의 ①(원본과 완전히 동일한 동작, [0.7](#07-계속-유효한-결정제약))에 속한다.

**항목마다 밟는 순서:** ① 그 차이 때문에 원본과 달라지는 동작을 구체적으로 적는다 → ② 원본 근거(cpppj 의 x86 기대값·`--dump-*` 출력·분석 문서·녹화)와 같은 입력으로 비교한다 → ③ 차이가 없거나 플레이에서 구별할 수 없으면 구조를 유지하고 그 근거를 이 문서에 남긴다 → ④ 구별되면 먼저 지금 구조 안에서 고치고, 그것으로 안 되면 구조를 원본 방식으로 바꾼다. **비교 근거 없이 "같다"고 적지 않는다.**

**2026-10-11 A 판정·첫 단계(`HJOW-Athlon`):** 24Hz 구조를 그대로 유지해도 같다는 판단은 폐기했다. 명령의 틱 대기·정지 중 1틱 진행·붕괴 스캔 33개/틱은 구별되는 차이이며 프레임 처리로 바꿨다. 명령과 다리 스캔은 같은 화면 갱신에서 처리하고 정지 입력은 시각을 늘리지 않는다. 실제 세션에 원본 10.78 Scan **1,948행**·Init **4행**을 연결해 시각·커서·다음 시각 double 비트가 일치했다. [범위·검증·제한](docs/dotnet-frame-parity-20261011.md).

- [x] **A 첫 단계:** 명령·실제 프레임 붕괴 스캔·정지 시각 보존과 지도 입력 호출 순서 수정.
- [x] **A 화면 루프 후속(2026-10-12):** MonoGame 고정 화면 갱신·합성 따라잡기를 제거하고 WINMM 실제 간격·직전 그리기 기준 제한·정지/늦은 프레임 대기 생략을 연결했다. 새 시계 경계 15개·전체 단위 983개·GUI 20회/454개 단언 통과. [범위·검증](docs/dotnet-loop-parity-20261012.md).
- [x] **A 숫자 시계 후속(2026-10-12):** 원본 DWORD 원점/epoch/보정·몫/나머지 double·중첩 정지 누계를 Core API로 복원하고 벽시계 double 차이를 실제 그리기 제한에 연결했다. 기본 x87 **4,464개** 비트 대조·전체 단위 **990개** 통과. [범위·검증](docs/dotnet-clock-kernel-20261012.md).
- [x] **A 세션 시계 연결 구현(2026-10-12 `VM-W11-CODEX`):** 숫자 시계·중첩 정지 소유권을 실제 세션과 화면 제한에 연결하고 원본 F6 직접 지식창의 실행 상태를 유지했다. [기록](docs/dotnet-session-clock-20261012.md). Release 컴파일까지 완료했으며 새 동작 대조는 후속이다.
- [ ] **A 나머지:** 이동·전투·건설·생산·수확·의식·AI·튜토리얼·애니메이션의 정수 틱을 실제 시각/원본 타이머로 이행한다. 과도 단계의 `FrameSeconds`와 `Seconds`, 8틱 상한, 정지 중 새 경로의 기존 보간 잔여량과 새 세션 시계 연결의 동작 검증을 정리한다. 안내창 재개/브리핑 지연·FPS 표본/Capture·초 경계 초기화도 후속이다. 이벤트 Tick과 RunTicks 기반 검사도 함께 옮긴다. **A 전체 완료로 처리하지 않는다. C 이후 항목은 아직 판정하지 않았다.**

| # | 판정 시작 시 차이 (원본) | 원본과 달라질 수 있는 동작 | 판정에 쓸 근거 | 구조를 바꾸게 되면 |
|---|---|---|---|---|
| A | 24Hz 고정 틱 + 화면 보간 (원본은 고정 틱 없이 프레임마다 한 번 돌고 "지금 + 간격" 타이머를 쓴다) | 사건이 틱 경계(41.7ms)에 맞춰 일어난다. 명령이 다음 틱 처음에 실행된다. 타이머 간격이 41.7·83.3ms 다(원본 14ms 루프에서는 42·84ms). 붕괴 스캔이 틱당 33개다(원본은 프레임당 11개) | [main-loop.md](docs/exe/main-loop.md) 7절 — 2026-10-05 에 "이동·애니메이션 간격·프레임당 동작은 이미 같다"고 판단했으나 **다시 판정한다**. [cpp-process-reconstruction.md](docs/exe/cpp-process-reconstruction.md) 와 `process-x86.tsv`, `bridgedecay-x86.tsv` 의 Scan 행 | 세션을 프레임 단위 갱신(가변 경과 시간)과 원본 타이머 방식으로 바꾼다. `RunTicks` 기반 테스트·결정론 검사합·화면 보간이 함께 바뀐다 |
| B | 입력을 MonoGame 으로 읽는다 (원본은 사건 큐 40칸, 넘치면 새 사건을 버리고 한 바퀴에 한 번 처리한다) | 빠른 연속 클릭·끌기·키 반복의 처리 순서와 누락 | cpppj 의 입력 큐 복원([cpp-screen-reconstruction.md](docs/exe/cpp-screen-reconstruction.md) 6절). 입력 처리 전체(UserInput `004d62b0`)는 2026-10 초 기록 당시 cpppj 미복원이었다. 현재 cpppj에는 `client/UserInput`·`InputEvent`가 있으므로 0절 R1·R6에서 그 범위를 옮긴다 | 원본과 같은 사건 큐를 두고 한 바퀴에 한 번 비운다 |
| C | 자체 `settings.json` 을 쓰고 `options.cfg` 는 읽기만 한다 (원본은 `<게임 폴더>/d/options.cfg` 를 시작·변경 시 저장하고 `fullscreenStateFile.dat` 를 만들고 지운다) | 옵션이 저장되는 시점과 위치, 원본 게임과 설정을 주고받는지 | [cpp-options-reconstruction.md](docs/exe/cpp-options-reconstruction.md), 이미 C# 테스트에 연결한 options x86 기대값 16개 | `options.cfg` 를 원본의 시점·형식으로 읽고 쓴다. **단 AGENTS.md 의 "전체화면 전환 뒤 재실행 오류는 클론에서 발생하지 않아야 한다"는 그대로 지킨다** |
| D | 그리기 직전 제한의 대기는 Sleep/Yield로 CPU를 양보한다(2026-10-12; 원본은 바쁜 대기) | 실제 간격과 흔들림·초과 대기가 달라질 수 있다. 모든 환경에서 14ms라고 단정하지 않는다 | main-loop.md 4·7절. 원본 전체 루프의 실측 대조는 후속이며 현재 D 미판정 | 바쁜 대기로 바꾼다(CPU 한 코어를 계속 쓴다) |
| E | 매 프레임 전체를 다시 그린다 (원본은 변경 사각형 100개 표로 부분만 다시 그린다) | 화면 결과가 같아야 한다 | [cpp-renderer-reconstruction.md](docs/exe/cpp-renderer-reconstruction.md), 화면 캡처 비교 | 변경 영역 표를 옮긴다 |
| F | 뮤텍스·레지스트리·CD 찾기·창 크기 고정이 없다 | 창 크기 조절·중복 실행처럼 사용자가 보는 동작 | [cpp-screen-reconstruction.md](docs/exe/cpp-screen-reconstruction.md) | 사용자가 보는 동작만 원본대로 맞춘다 |

**이 결정으로 바꾸지 않는 것:** MonoGame 을 쓰는 것과 raw 슬롯·vtable 을 옮기지 않는 것은 플랫폼(Linux 목표)과 내부 표현의 문제다(2026-10-16: 원본 월드 코어 저장소에 한해 슬롯 바이트를 옮기도록 바꿨다, 3절 11번). 화면·소리·규칙의 결과가 원본과 같아야 한다는 기준만 적용한다. CD 10.72·10.82 값을 쓰지 않는 것은 기준 판본(10.78)을 따르는 것이라 원본과의 차이가 아니다.

---

## 3. 사용자 결정·확인 기록 (옛 10절)

> **2026-10-28:** 번호는 옛 10절 번호를 그대로 쓴다. 해결·구현이 끝난 1·3·4번은 이관 파일로 옮겼다. 이번 재수립에서 새로 생긴 확인 사항은 [0.8](#08-사용자-확인이-필요한-것-확인-전까지-에이전트-기본값으로-진행)에 있다.

2. **영어 UI 글꼴(5-3) — 해결(2026-10-10 사용자 결정): 언어가 영어일 때의 글꼴은 기존 게임의 글꼴을 따른다.** 판단 기준은 **선택한 언어**다. 언어가 영어이면 UI 글자를 원본 비트맵 글꼴(`.chfnt`)로 그리고 폭도 그 글꼴로 잰다. 언어가 한국어이면 지금처럼 D2Coding 을 쓴다. 영어 화면에 원본 글꼴에 없는 글자(한글 등)가 섞인 문자열은 D2Coding 으로 대신 그린다(에이전트 해석 — 개발용 표시와 아직 번역 분기가 없는 문구가 깨지지 않게 하려는 것이다). 옛 구현 상태(5-3)는 이관 파일에 있다. **2026-10-28:** 새 경로에서는 R2·R3의 `BitmapFont`·Gump 글리프로 영어를 그린다. 한국어 D2Coding은 R8(0.8 2번)에서 8비트 화면용으로 다시 만든다.
5. **"원본과 완전히 동일한 동작"과 9절의 의도적 차이 — 해결(2026-10-10 사용자 결정): 원본처럼 동작하도록 바꾼다. 지금 구조를 둔 채 원본 게임의 경험을 거의 동일하게 만들 수 있으면 구조는 유지해도 되지만, 그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다.** 항목별 판정과 작업은 [9-1](#original-parity-plan)에 있다. 같은 날 먼저 적었던 에이전트 기본값("구조를 두고 cpppj 와 대조")은 폐기했다.
6. **이미 들어간 요구사항 기능의 처리(2026-10-10 추가, 사용자 확인 전).** 한국어 선택·와이드 화면 시야 확장·테두리 없는 전체화면 창은 이미 구현돼 있다(자체 `settings.json` 은 [9-1](#original-parity-plan) C 의 판정 대상으로 옮겼다). 기본값(에이전트 해석): 되돌리지 않고 유지한다. 기본 실행(영어·4:3)에서 원본과 같은 동작을 먼저 맞추고, 새 요구사항 기능은 ① 뒤로 미룬다. **2026-10-28:** 0.7과 0.8 2번에 따라, 새 경로로 옮길 때 cpppj 방식(논리 해상도·호환 전체화면)으로 다시 만들고 한국어는 R8에서 다시 만든다.
7. **D2Coding 1.4.0 교체 — 해결(2026-10-10 사용자 결정): 중요도가 낮다.** 당분간 1.3.2 를 그대로 쓴다. 다른 작업을 미루면서 하지 않으며, 글꼴 코드를 고칠 일이 있거나 여유가 있을 때 한다. 교체할 때 확인할 것: 코드 3곳의 파일명(`Netstorm.Game.csproj`·`NetstormGame.cs`·`TrueTypeCollectionTests.cs`), TTC face 순서·글자 폭·줄 높이, `TrueTypeCollectionTests` 기대값, 한국어 화면 캡처, README 두 개의 파일명.
8. **기존 코드 주석 정비 — 해결(2026-10-10 사용자 결정): 그 주석이 설명하는 대상이 수정될 때 한다.** 상수·함수·클래스·멤버함수·반복문을 고치면 그 대상의 주석을 새 규칙(설명·사용법 먼저, 추가/수정 이력은 뒤)에 맞춘다. 고치지 않는 대상의 주석을 찾아다니며 일괄 정리하지 않는다.
9. **cpppj 미복원 동작의 처리(2026-10-16 추가, 에이전트 기본값·사용자 확인 전).** cpppj는 미복원 타입의 후처리를 만나면 명시 오류로 멈춘다. dotnetpj는 플레이 가능 상태 유지를 우선한다. 기존 근사를 유지하고 [core-rules.md](docs/core-rules.md)의 근사 목록에 "원본 근거 없음"으로 등록한 뒤, cpppj가 복원하면 교체한다. 대신 해당 미션을 잠그기를 원하면 이 기본값을 바꾼다. **2026-10-28:** 새 경로에서는 0절 결정 6으로 대체했다. 이 기본값은 동결된 기존 경로에만 남는다.
10. **원본 월드 코어 도입(2026-10-16 추가, 에이전트 판단).** 5번 결정의 "그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다"에 해당한다고 판단했다. 근거와 범위는 [이식 계획 4절](docs/dotnet-cpp-port-plan-20261016.md#4-구조-결정-c-원본-월드-코어-도입)에 있다. 규모가 크므로(cpppj 원본 대응 소스 약 6.2만 줄·fixture 약 68만 행) 영역별 교체형으로 진행하고 1-1·1-2 플레이를 유지한다. **2026-10-28:** "영역별 교체형·1-1·1-2 플레이 유지"는 0절 결정 5로 대체했다. 원본 월드 코어는 R4에서 세션에 조립한다.
11. **원본 월드 코어의 저장 방식(2026-10-16 추가, 에이전트 판단·사용자 확인 전).** 코어 저장소는 슬롯 50바이트를 그대로 둔다(이식 계획 3절 3번의 "raw 슬롯 미이식"을 코어 안쪽에 한해 변경). 근거: W02~W03 fixture가 슬롯 전체 바이트를 대조하고, 파생 생성자 쓰기가 필드 이름 없는 오프셋이라 의미 모델로는 검증할 수 없다. 가상 표 주소는 클래스 식별 값으로만 쓰고 세션·`GameEntity`는 코어 위의 보기로 만든다. 상세는 [원본 월드 코어 W02 첫 조각](docs/dotnet-original-core-20261016.md) 1절. 의미 모델을 원하면 이 기본값을 바꾼다(아직 세션에 연결하지 않아 되돌리는 비용이 작다). **2026-10-28:** 0절 결정 2로 범위를 넓혔다. cpppj가 원본 배치를 쓰는 곳은 세션·화면 쪽도 같은 배치를 쓰며, "세션·`GameEntity`는 코어 위의 보기로 만든다"는 문장은 대체됐다.

---

## 4. 검증 명령과 기록 규칙 (옛 12절)

```powershell
# 저장소 루트에서 기준선과 단계별 검증 (헤드리스)
dotnet build dotnetpj/Netstorm.sln -c Release
dotnet test dotnetpj/Netstorm.sln -c Release

# cpppj 콘솔 출력 대조 (헤드리스 명령만 사용. --run 등 게임/창 실행은 해당 PC의 규칙을 별도로 확인)
cpppj/build/bin/Release/NetstormCpp.exe --dump-types originals
cpppj/build/bin/Release/NetstormCpp.exe --dump-world originals thewarbegins
cpppj/build/bin/Release/NetstormCpp.exe --inspect-bridges originals

# cpppj 반영 범위 확인 (W01, 2026-10-16). 저장소 루트에서 실행. 마지막 반영 커밋은 cpp-fixture-manifest.tsv 의 reflected-commit
python dotnetpj/tools/cpp_sync.py                                    # 보고 (읽기 전용, 연결 fixture 가 어긋나면 종료 코드 1)
python dotnetpj/tools/cpp_sync.py --update --reflect HEAD            # HEAD까지 영향 검토·반영을 모두 마친 경우에만 갱신
python dotnetpj/tools/cpp_sync.py --update --add 이름:Core           # 새 fixture 를 연결할 때 목록에 등록 (Assets 도 가능)
```

```powershell
# cpppj 오라클 캡처 (2026-10-28 추가, 0절 결정 4). 창이 뜨므로 AGENTS.md 확인·예외 규칙을 따른다.
# 데이터는 originals/ 대신 extracted/ 아래 사본을 쓴다. steps.tsv 한 줄은 "프레임<TAB>동작<TAB>인자"다
# (click·right·key·esc·pointer·snapshot 등. snapshot 인자는 저장할 BMP 경로).
cpppj/build/bin/Release/NetstormCpp.exe --run <데이터 사본> --window --no-audio --frames 120 `
  --set "SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0" `
  --ui-script steps.tsv --ui-report states.tsv            # 미션 직행은 --mission thewarbegins 등을 더한다
# 캡처 비교 도구: python dotnetpj/tools/cpp_frame_compare.py A.bmp B.bmp [--diff diff.png]  /  --dir 폴더A 폴더B (종료 코드 0 일치·1 차이·2 입력 오류)
# dotnetpj 쪽의 같은 실행 인자(--run/--ui-script/--frames/--set)를 받는 진입은 R1-2a(MonoGame 호스트)에서 만든다(아직 없음).
# PowerShell 5.1의 Start-Process -PassThru 는 종료 코드를 비우므로 `& exe 인자; $LASTEXITCODE` 로 읽는다.
```

- 클론 창을 띄우는 검사(`tools/clone_*_smoke.ps1`, `dotnet run --project dotnetpj/src/Netstorm.Game`)는 `DESKTOP-HJOW` 가 아닌 PC 에서 한다.
- 원본 x86 기대값 대조만 돌릴 때: `dotnet test -c Release --filter "FullyQualifiedName~X86"`(Core 39개 + Assets). 새 기대값 파일은 `tests/Netstorm.Core.Tests/Netstorm.Core.Tests.csproj`에 링크로 추가하고(사본 금지) 위 `cpp_sync.py --update --add`로 SHA-256 목록에 등록한다(빠뜨리면 `EveryLinkedFixtureIsRegistered`가 실패).
- 새 코드의 **상수·함수·구조체·클래스·멤버함수·멤버변수·반복문**에 한국어 주석을 단다(AGENTS.md 2026-10-13 확장). **주 내용은 설명과 사용법**이고, 언제·왜 추가/수정했는지는 그 뒤에 적는다. C# 에서는 `/// <summary>` 에 무엇을 하는지와 쓰는 방법(인자·반환·호출 조건)을 쓰고, 원본 주소·근거 문서와 추가/수정 이력은 summary 의 뒤쪽이나 `<remarks>` 에 둔다. 반복문은 바로 위 `//` 주석으로 무엇을 도는지 적는다. 기존 주석은 그 주석이 설명하는 대상을 수정할 때 이 규칙에 맞춘다(2026-10-10 사용자 결정). 새 파일은 UTF-8 로 쓴다.
- 원본 주소를 주석에 적을 때는 10.78 주소를 쓰고, 근거 문서(`docs/exe/cpp-*.md`)를 함께 적는다.
- 작업이 끝난 항목은 이 문서에서 체크하거나 지운다. 중단하면 변경 파일·검사 결과·다음 항목을 0절의 해당 체크리스트와 단위 기록에 남긴다. 끝난 단위의 상세 기록이 길어지면 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)로 옮기고 1절 색인만 갱신한다(2026-10-28).
- AGENTS.md 와 원본 디렉터리(`originals/`, `originalCD/` 등)는 수정하지 않는다.

---

## 5. 참고 문서 (옛 13절)

| 주제 | 문서 | cpppj 기준 구현 |
|---|---|---|
| **현행 계획·재수정·다음 작업(2026-10-28)** | [이 문서 0절](#dotnet-current-plan-20261028) | cpppj `src/o`·`src/client` 전체, [cpppj 소스 맵](cpppj/SOURCE_MAP.md) |
| 8비트 화면 출력·전체화면 호환 | [cpp-modern-fullscreen-20261027.md](docs/exe/cpp-modern-fullscreen-20261027.md), [cpp-screen-resolution-menu-20261027.md](docs/exe/cpp-screen-resolution-menu-20261027.md) | `client/Screen.cpp`, `client/ScreenPresentation.cpp`, `client/ClientMain.cpp` |
| 이관 기록(2026-10-22 이전 계획·날짜별 완료 절·옛 0~11·14절) | [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md) | — |
| **cpppj → dotnetpj 이식 계획(W01~W20 상세, 2026-10-16)** | [dotnet-cpp-port-plan-20261016.md](docs/dotnet-cpp-port-plan-20261016.md) | `client/RawCampaignSession`, `client/RawSurfaceWorld`의 `RawWorldCore` |
| **원본 월드 코어 W02 첫 조각(슬롯 풀·생성자·Create/Take, 2026-10-16)** | [dotnet-original-core-20261016.md](docs/dotnet-original-core-20261016.md) | `o/SidPool.cpp`, `o/SquidFactory.cpp`, `o/TypeConstructors.inc`, `o/DerivedConstructors.inc` |
| 반영 점검 D01~D16 | [dotnet-cpp-audit-20261015.md](docs/dotnet-cpp-audit-20261015.md) | — |
| 표시 이름·내부 타입명 대응 | [unit-source-name-map.md](docs/unit-source-name-map.md) | `o/RiftTypeTable.cpp` |
| 데모·캠페인 1-6/2-4 대응 | [demo-campaign-map.md](docs/gameplay/demo-campaign-map.md) | `o/RawMissionStart` |
| 설정 조회·치환·저장 | [cpp-config-reconstruction.md](docs/exe/cpp-config-reconstruction.md), [cpp-options-reconstruction.md](docs/exe/cpp-options-reconstruction.md) | `o/Config.cpp`, `o/ConfigInterface.cpp` |
| 타입 표·요새 파일 | [cpp-fort-reconstruction.md](docs/exe/cpp-fort-reconstruction.md), [cpp-assets-reconstruction.md](docs/exe/cpp-assets-reconstruction.md) | `o/RiftTypeTable.cpp`, `o/Template.cpp` |
| 다리 표·추첨·열린 방향·수명 | [cpp-bridge-reconstruction.md](docs/exe/cpp-bridge-reconstruction.md), [bridge-pieces.md](docs/exe/bridge-pieces.md) | `o/Bridge.cpp`, `o/CanonDecoder.cpp` |
| 표면 이웃·연결·붕괴 방문 | [cpp-surface-reconstruction.md](docs/exe/cpp-surface-reconstruction.md) | `o/SquidFinder.cpp` |
| 표면 그래프 | [cpp-graph-reconstruction.md](docs/exe/cpp-graph-reconstruction.md) 외 graph* 5개 | `o/Graph.cpp`, `o/RawGraph.cpp` |
| 점유 비트·해시 단계·등록/해제 | [cpp-hash-reconstruction.md](docs/exe/cpp-hash-reconstruction.md), [cpp-spatial-reconstruction.md](docs/exe/cpp-spatial-reconstruction.md) | `o/SquidHash.cpp`, `o/SquidSpatial.cpp` |
| 번호(SID) 할당 | [cpp-sid-reconstruction.md](docs/exe/cpp-sid-reconstruction.md) | `o/SidPool.cpp` |
| 첫 등록 통계·비용·목록 | [cpp-postpop-reconstruction.md](docs/exe/cpp-postpop-reconstruction.md) | `o/SquidPostPop.cpp` |
| 그리기 순서·글꼴·커서 | [cpp-renderer-reconstruction.md](docs/exe/cpp-renderer-reconstruction.md), [cpp-display-reconstruction.md](docs/exe/cpp-display-reconstruction.md) | `client/Renderer.cpp`, `o/SquidDisplay.cpp` |
| 메뉴·브리핑 | [cpp-menu-reconstruction.md](docs/exe/cpp-menu-reconstruction.md) | `client/UberGump.cpp`, `client/Gump.cpp` |
| 지형·시작 값·조작 | [cpp-world-reconstruction.md](docs/exe/cpp-world-reconstruction.md) | `o/TerrainBuilder.cpp`, `client/GameWorld.h` |
| 메인 루프·프레임 | [main-loop.md](docs/exe/main-loop.md) | `client/ClientMain.cpp` |
| 판본 비교(10.37·10.62·10.82·V12) | [cpp-reference-versions.md](docs/exe/cpp-reference-versions.md) | — |
| C# 규칙 코어의 현재 범위와 근사 목록 | [core-rules.md](docs/core-rules.md) | — |
| dotnetpj 복원 기록(날짜별) | [2026-10-07 자료 계층](docs/dotnet-reconstruction-20261007.md), [2026-10-09 다리·표면·그래프·번호·점유](docs/dotnet-reconstruction-20261009.md), [2026-10-10 커서 표·배경 GIF 색·미션 시작 값](docs/dotnet-reconstruction-20261010.md) | — |

---

## 6. 이관 기록 — LEFT_JOBS.dotnetpj.old.md

**2026-10-28 이관(VM-W11-Codex):** 0절 재수립으로 더 이상 현행 의미가 없어진 내용을 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)로 옮겼다. 내용은 고치지 않고 그대로 옮겼으며, 그 파일 안의 "현재·다음·미구현" 표현은 당시 상태다.

- **옮긴 것:** 2026-10-22 이전 계획, 2026-10-09~10-23 날짜별 완료·인계 절, 구 머리말과 0~8절(2026-10-07~10 당시 1~6단계·8절 이식/대기 구분), 옛 9절 표와 9-1의 옛 순서 제안, 옛 10절 1·3·4번(해결·구현이 끝난 확인 항목), 11절(과거 미확인 항목 재분류), 14절(LEFT_JOBS.md에서 이관한 2026-09-27~10-05 이력).
- **본문에 남긴 것:** 0절 현행 계획, 1절 완료 범위 색인, 2절 9-1 결정, 3절 옛 10절의 유효한 결정, 4절 검증 명령(옛 12절), 5절 참고 문서(옛 13절).
- **앵커:** 옮긴 절의 `<a id>`(`dotnet-current-plan-20261022`·`sid-session-plan`·`legacy-dotnet-history`·`legacy-dotnet-031`~`107` 등)는 이관 파일에 같은 이름으로 남겼다. LEFT_JOBS.old.md·LEFT_JOBS.webpj.md·`docs/`의 기존 링크가 이 안내로 오도록 같은 이름의 빈 앵커를 바로 아래에 두었다. 그런 링크로 들어왔다면 이관 파일의 같은 앵커를 본다.

<a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
