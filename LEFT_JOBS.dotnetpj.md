# LEFT_JOBS.dotnetpj — dotnetpj(C# + MonoGame) 작업 계획 및 인수인계

이 문서는 dotnetpj의 **현재 상태, 계획, 남은 작업, 유효한 사용자 결정**을 담는다. cpppj와 공통 사항은 [LEFT_JOBS.md](LEFT_JOBS.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md), 과거 기록은 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)에 있다.

[0절 현행 계획](#dotnet-current-plan-20261028)이 이 문서와 [이식 계획 문서](docs/dotnet-cpp-port-plan-20261016.md)의 순서·구조 결정보다 우선한다. 1절은 완료 범위 색인, 2·3절은 계속 유효한 사용자 결정, 4·5절은 검증 명령과 참고 문서다.

단위를 마치면 0.1절의 값과 0.9절의 체크 항목을 갱신한다. 상세는 `docs/dotnet-*.md` 단위 문서에 남기고, 문서가 없는 작은 단위만 0.127 이후의 단위 기록에 짧게 적는다(0.126 이하는 이관 파일).

**2026-11-02 정리:** 이 문서를 현재 상태·계획·남은 작업 중심으로 정리했다(6절 첫 항목). AGENTS.md 최근 변경(4차 로컬 MCP·와이드 해상도, 7차 2560×1440·3840×2160 FSR, 11.01, 외부 접속 문서화)은 0.7·0.8·0.9에 반영했고 공통 현황은 [LEFT_JOBS.md 1.3절](LEFT_JOBS.md#agents-reflection-20261102)에 있다. 정리 직전 전체 원문은 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)에 그대로 보관했다.

<a id="dotnet-current-plan-20261028"></a>

## 0. 2026-10-28 현행 계획 — cpppj 그래픽 복원분을 기준으로 dotnetpj 재구성

### 0.1 현재 값 (2026-11-02, 단위 기록 0.136 기준)

| 항목 | 값 |
| --- | --- |
| 진행 상태 | **작업 진행 중.** 전체 목표는 미완료다. |
| 현재 단계 | R0·R1 완료, R2 본체 완료, R3 진행 중, R4 일부 이식. R5 이후는 미착수 |
| 실행·플레이 가능 여부 | 플레이 가능한 기본 실행은 기존 경로 `Netstorm.Game`(`Netstorm.exe`, 0.96)의 1-1·1-2 근사 플레이다. 새 경로 호스트 `Netstorm.Client.Host`는 타이틀·메인 메뉴 등 메뉴 계열 화면 일부까지이고 미션 월드는 없어 플레이할 수 없다. 기존 코드·검사 은퇴는 0건이다. |
| 원본과의 동일성 | 실제 메뉴·게임 캡처 비교는 아직 0건이다. 오라클 메인 메뉴 캡처의 결정성만 확인했다([문서 5절](docs/dotnet-client-screen-20261028.md)). L1(원본 x86 fixture) 통과를 L2 이상으로 합산하지 않는다(0.6). |
| 마지막 구현 커밋 | `f1815c83`(0.135 Template 저장소 TS5). 이번 0.136(TS6)은 이 문서와 함께 커밋하므로 다음 단위가 해시를 적는다. |
| 검사 | Release 전체 통과 수의 프로젝트별 마지막 확인값: **Client 841개**(0.136), **Core 1,125개**(0.136), **Assets 366개**(0.128). 서로 다른 단위에서 실행한 값이라 한 번의 `sln` 전체 실행으로 합산하지 않으며 실패·건너뜀은 모두 0이다. 0.136은 PORT_MAP 무결성 2개와 fixture 목록(164개) 검사도 통과했다. Assets·Core의 기존 경로 검사 수를 새 경로의 진행도로 세지 않는다. 단위별 집중 검사 수는 0.127~0.136 기록과 이관 파일의 0.74~0.126을 본다. |
| 포트 맵 | `PORT_MAP.tsv` 385행 (L1 199, port 186) |
| fixture | 연결 164개(0.128에서 DataManager·Template 14개, 0.136에서 새 맵 4개 추가). 반영 커밋(`reflected-commit`)은 영향 검토 전이라 `f95a8db2`에 그대로 둔다. |
| cpppj 기준 | 단위695 제품(표시 11.01), cpppj 커밋 `b61b2007`([LEFT_JOBS.md](LEFT_JOBS.md) 0.1절). 소스는 HEAD만 쓰고 미커밋 변경은 쓰지 않는다. 위 fixture의 `reflected-commit`(`f95a8db2`)은 영향 검토 전의 기준이라 올리지 않았다. |
| 디컴파일 | 공통 기록의 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**(LEFT_JOBS.md 0절)이다. 이식 근거는 cpppj 소스·fixture·문서이므로 디컴파일이 필요 없다. 산출물을 직접 읽어야 할 때만 AGENTS.md대로 재디컴파일한다. |

### 0.2 진단 — 기존 구현이 원본과 달라진 구조적 원인

기존 구현은 화면이 RGBA 합성이고, UI가 원본 Gump 트리가 아닌 자체 배치이며, 규칙이 근사 세션(`BattleSession`)에 있다. 옮겨 둔 `Original` 모듈은 세션에 연결되지 않아 플레이를 바꾸지 못했고, 같은 입력에서 cpppj와 비교할 수단도 없었다. 그래서 0.3의 구조로 다시 세웠다.

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

  대응표는 [PORT_MAP.md](dotnetpj/PORT_MAP.md)·`PORT_MAP.tsv`(cpppj 파일 → C# 파일 → 이식 기준 cpppj 커밋 → 상태)이며 `cpppj/SOURCE_MAP.md`와 짝을 이룬다.
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

**단계 전환 기준 (2026-10-31 사용자 결정, cpppj와 같음):**

- **자연 완주·전체 동일성:** 모든 시나리오(Early Missions, Priest Training, 캠페인, 데모)의 자연 완주와 전체 동일성은 사용자가 직접 실행·플레이하며 테스트한다. AI 작업 대상이 아니며, 단계에 남은 작업이 이것뿐이면 다음 단계로 넘어간다. 각 단계 완료 기준의 L1~L4 대조(원본 fixture, cpppj 오라클과의 상태·캡처 비교)는 계속 AI가 한다.
- **Windows 11:** Windows 10에서 오류 없이 구동되면 Windows 11에서도 문제가 없을 가능성이 높다고 본다. Windows 10 확인을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다.
- **제보 처리:** 사용자가 플레이 중 오류나 원본과의 차이를 제보하면 해당 항목을 AI 작업으로 올린다.

**R0 — 기준선과 동기화 재정비 (W01 확장) — 완료.** 오라클 확정, 재대조 영향표, `PORT_MAP`, `cpp_sync.py --ports*`를 마쳤고 뒤처진 C# 여섯 묶음도 고쳤다(1절).

**R1 — 8비트 화면 호스트·입력 큐·오라클 도구 (W14 일부, 9-1 B·E) — 완료.** `Netstorm.Client`, `Netstorm.Client.Host`, `--ui-script`, `snapshot`, `cpp_frame_compare.py`를 만들었다. 라벨 클릭·report·kernel-tell은 R3에 남는다.

**R2 — 그리기 원시·팔레트·글꼴 (W14 일부) — 본체 완료.** VFX, 색 표, 팔레트, `RawShapeToBuffer`, `SpriteDraw`, 글꼴, GIF의 본체와 L1 대조를 마쳤다. 실자산 연결과 대표 장면의 캡처 비교는 R3~R5에서 잇는다.

**R3 — Gump UI와 메인 메뉴 계열 (W15) → 이정표 D1: 메뉴 화면 픽셀 일치 — 진행 중**

- 옮길 것: Gump 프레임워크 전체(`Gump`·`GumpTree`·`GumpCreate`·`GumpLifetime`·`GumpEvents`·`GumpInput`·`GumpFrame`·`GumpDraw`·`GumpCollection`·`GumpVisual`·`GumpBackground*`·`GumpDecoration`·`GumpHelp`·`GumpPointerBoundary`·`GumpFormBinding`), 버튼·돌 버튼·메뉴·스크롤·텍스트·텍스트 입력·HTML 창·메시지·이미지·배경 GIF 위젯, `UberGump`·`UberRootGump`·`RawUberRootControls`·`NativeMainMenuState`, 로딩 화면(`MissionLoadingMessage`·`RawMenuLoading`), Tell·Dialog·StyleText·Markup·QuickHelp, 기본 커서(`Cursor`).
- 범위: 메인 메뉴 8버튼, Campaign/Demo/Help/Options(해상도·음량·Window Mode)/Credits/Tips/Quit, 캠페인 목록, 브리핑 창.
- **완료 기준:** 같은 ui-script로 각 화면·하위 메뉴·Help 쪽·Credits 쪽을 캡처해 cpppj와 차이 0이다. 이때부터 메뉴는 새 경로가 기본이 되고, 메뉴에서 미션에 들어가는 경로는 R5 전까지 기존 경로를 쓴다(0.8 1번).

**R4 — 월드 세션 조립 (W02·W03 잔여, W04~W07) → 이정표 M1: 시작 상태 L2 — 일부 이식**

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
- 규칙: 사제·경로·운반·수확, 배치·건설·생산, 전투·방어·파생 건물, 의식·신전·제단, 미션 진행·안내·승패, 공통 전략 AI(`RawAi*`). AI는 cpppj에서 실제 세션 연결까지 끝났으므로 W19 "대기"에서 이식 대상으로 바꾼다. 단 cpppj도 정상 간격의 AI 행동을 자연 플레이로 대조하지는 않았다. `arc-spire`·`acid-fence`·`ice-tower` fixture는 전체 몸체 클래스 대조용이라 해당 클래스를 옮길 때 연결한다.
- 소리: 소리 Process, 음악 선택·흐름, 장면 음악. 장치는 MonoGame.
- **완료 기준:** 고정 입력열의 L3 상태 해시와 대표 장면 캡처가 일치한다. 1-1의 건설 파이프라인이 원본 경로로 동작한다(M2). 1-1·1-2가 새 경로에서 진행되면 기존 경로와 그 검사를 삭제한다.

**R7 — 미션 전체·Edit·설정·배포 (W17·W18, 9-1 C·D·F, Edit) → 이정표 M4**

- 34경로 전체, 데모 3개의 반복, 진행 저장과 다음 미션 해제, Retry/Next/Abort를 옮긴다.
- Edit(편집 메뉴·섬 추가·저장·Test Battle·SaveGoMain, `.english` 등 언어 파일)를 옮긴다. AGENTS.md상 Edit 제외는 webpj에만 해당한다.
- `options.cfg`를 원본의 시점·형식으로 읽고 쓴다(9-1 C). 단 전체화면 전환 뒤 재실행 오류는 허용하지 않는다. 9-1 D·F를 판정하고 Windows 10에서 확인한다. Windows 11 실기 검증은 단계 전환 조건이 아니다. 이 단계의 저장 위치는 원본과 같은 게임 폴더 `d/`이며, `%USERPROFILE%\Saved Games\Netstorm\Saves\`로의 이동은 AGENTS.md 5차 요구로 R8에서 한다. Edit 맵 목록의 원문 50개 상한도 5차까지 원본대로 둔다.
- 배포 스크립트를 갱신하고(실행 파일 이름은 `Netstorm.exe`, 0.9), 압축 해제한 사본에서 실행되는지 확인한다.
- **완료 기준:** 34경로 짧은 진입과 핵심 장면이 cpppj와 일치한다. 자연 완주(L5)는 사용자 확인 대상이라 완료 기준에 넣지 않는다.

**R8 — 요구사항 확장(① 완료 뒤, W20)**

- 한국어: D2Coding 글리프를 8비트 화면에 그리는 방식(단색 또는 팔레트 단계)을 정해 구현한다. 기존 한국어 명칭 규칙은 그대로 따른다.
- 60/120프레임, Linux(DesktopGL), 그 뒤 TCP/IP 로컬 멀티플레이·Outpost(최후순위)를 진행한다.
- AGENTS.md 4~7차: 4차 로컬 MCP(해석은 0.8 6번)와 와이드 해상도(16:9, 최대 1920×1080, 해상도 지원만이며 그래픽 크기 스케일 조정은 없음, 2026-11-02 AGENTS.md `5867f35e`), 5차 설정·캠페인 진행 저장을 `%USERPROFILE%\Saved Games\Netstorm\Saves\`로 옮기고 커스텀 맵을 기존 폴더와 `...\CustomMaps\` 양쪽에서 불러오며 Edit 신규 저장은 후자로 하고 Edit 맵 목록에 스크롤과 개수 무제한을 넣는다, 6차 한국어(위 첫 항목), 7차 Options의 안티앨리어싱·FSR 1.0 선택(둘 다 기본값은 꺼짐)과 게임 적용이다. `5867f35e`로 7차에 2560×1440·3840×2160 지원이 추가됐다: 옵션 해상도 목록에 두 해상도를 넣고 이때 FSR 1.0을 자동으로 켜며, 실질 해상도는 1920×1080으로 두고 FSR로 목표 해상도까지 업스케일한다. 현재 기존 경로는 자체 `settings.json`(`DisplaySettings`)을 쓰고 새 경로는 R7에서 `options.cfg`로 옮기므로, 5차는 R7의 저장 구조 위에서 위치만 바꾸는 순서로 진행한다.

### 0.6 검증 단계와 기록 규칙

- **L1:** 원본 x86 fixture(기존 방식). **L2:** 같은 미션 Load/Start 상태가 cpppj와 같다. **L3:** 같은 고정 시계·입력열로 N프레임 진행한 상태 해시가 같다. **L4a(신규):** 같은 ui-script의 논리 캡처가 픽셀 차이 0이다. **L4b:** 소리 시점과 곡 선택이 같다. **L5:** 자연 완주·저장·재실행. 사용자 확인 대상이라 AI 작업과 단계 완료 기준에 넣지 않는다(0.5).
- cpppj 캡처는 오라클 SHA·스크립트·`--set` 값·프레임과 함께 기록한다. cpppj가 논리 캡처(`snapshot`)와 실제 출력 캡처(`snapshot-output`)를 구별하듯 dotnetpj도 둘을 구별한다. 실행에는 `originals/`가 아니라 `extracted/` 아래의 데이터 사본을 쓴다.
- 한 단위를 마칠 때 기록할 것: 옮긴 cpppj 파일과 기준 커밋, 연결 fixture와 행 수, 이식한 cpppj 검사 수, 은퇴시킨 기존 코드와 검사, 비교한 캡처 수와 차이, 남은 경계. L1 통과를 L2 이상으로 합산하지 않는다. 기존 근사 검사의 은퇴로 실패를 감추지 않으며, 원본 fixture 기대값을 C# 결과로 바꾸지 않는다.
- 클론·cpppj 창 실행은 AGENTS.md의 확인·예외 규칙을 따른다(VM-W11-Codex·HJOW-Athlon·vm-debian-codex는 예외 PC). HJOW-X3D를 포함한 그 밖의 PC에서는 실행 전에 목적을 설명하고 명시적 확인을 받는다. 이 계획은 원본 게임을 실행할 필요가 없다.

### 0.7 계속 유효한 결정·제약

- **목표 순서:** ① 원본 10.78과 동일한 동작 → ② 요구사항(화면비·60/120프레임·다국어·재실행 오류 방지) → ③ Linux. 이미 들어간 요구사항 기능(한국어 선택·와이드·전체화면)은 버리지 않는다. 새 경로로 옮길 때 cpppj 방식(논리 해상도·호환 전체화면)으로 다시 만들고, 한국어는 R8에서 다시 만든다(0.8 2번).
- **AGENTS.md 상위 요구(2026-11-02 `5867f35e` 기준):** cpppj의 후순위 4~7차(4차 로컬 MCP와 16:9 최대 1920×1080 해상도 지원, 5차 저장 위치·커스텀 맵·Edit 목록, 6차 한국어, 7차 안티앨리어싱·FSR 1.0과 2560×1440·3840×2160)가 **dotnetpj 후순위 요구에도 포함**된다. 원본과 같은 동작(목표 ①) 뒤에 R8에서 다루며 세부 내용은 [LEFT_JOBS.md](LEFT_JOBS.md) 1.1절과 [1.3절](LEFT_JOBS.md#agents-reflection-20261102)을 따른다. 7차의 FSR은 FSR 1.0이고 FSR 3.1 안은 쓰지 않는다. `ef0c6b91`에 따라 cpppj·dotnetpj 모두 **Windows 용 게임 실행 파일 이름이 `Netstorm.exe`**여야 한다. cpppj는 적용을 마쳤고, dotnetpj는 기존 경로 `Netstorm.Game`을 0.96에서 `Netstorm.exe`로 바꿨다. 새 경로 호스트로 기본 실행을 넘길 때의 이름 소유는 0.8 5번이다. 후순위가 아닌 즉시 적용 요구로 **11.01 표기·Credits**(`462c9a64`)와 **외부 접속 기능 문서화·클라이언트 유효성 검사 비활성화**(`0190ba95`)가 있으며 0.9의 해당 항목을 따른다.
- **9-1 의도적 차이(2026-10-10 사용자 결정)의 적용:** A(시간)는 R4의 Kernel/Regular, B(입력 큐)는 R1, C(`options.cfg`)는 R7, D(바쁜 대기)·F(창 동작)는 R7에서 판정, E(부분 다시 그리기)는 R1·R5에서 처리한다.
- **글꼴·명칭·주석:** 영어는 원본 `.chfnt`, 한국어는 D2Coding(3절 2번). D2Coding은 1.3.2를 유지한다(3절 7번). 기존 주석은 대상을 고칠 때 정비한다(3절 8번). AGENTS.md의 한국어 명칭 규칙을 따른다.
- **cpppj 공통 제약을 준용한다.**
  - CHEAT 항목은 Edit 편집에서만 나타난다.
  - 공격형 공중 3종(훨리기그·맨오워·더스트 데빌)은 cpppj 코드를 옮기되, 이 타입을 위한 실제 생산·생성·우클릭 재현과 추가 실행 관측은 하지 않는다.
  - 연녹색 직사각형 분석은 보류한다(원본에서도 관찰됨).
  - 대응 `.english`가 없을 때 Test Battle이 끝나는 것은 원본 동작이며 개선은 보류한다.
  - 모든 시나리오의 자연 완주와 전체 동일성은 사용자가 플레이하며 확인한다(0.5). 멀티플레이는 후순위다.
  - 실제 실행 검사에서 사제 포획/희생, 특정 적 공격, 시나리오 승리를 재현해야 하는 테스트는 건너뛴다([TODO.md](TODO.md)).
- **cpppj의 미완 범위는 cpppj를 따라간다:** View/zoom, 도킹, DD 조정·확인 UI 잔여, Chat GUI 드래그, 미니맵 내용물·grid·항목별 debug, Renderer 시차 경계는 [LEFT_JOBS.md](LEFT_JOBS.md) 3절의 잔여다. cpppj에 반영되기 전에는 dotnetpj에서 추정으로 만들지 않는다. 독립 미니맵, Credits 자동 전환, Chat 입력·전달, 마나 103·배율 Draw는 이미 cpppj에 반영됐으므로 이식 대상이다. 전체 화면·음향 동일성 대조(LEFT_JOBS.md 3절 5번)는 사용자 확인 대상이다.
- **근거 우선순위:** 원본 fixture → cpppj 보고서의 계약·경계 → cpppj 소스 → 녹화·스크린샷. cpppj는 HEAD만 쓰고 미커밋 변경은 쓰지 않는다. `cpppj/`·`LEFT_JOBS.md`는 수정하지 않고, 필요한 요청은 이 문서와 이식 계획 8절에 적어 사용자에게 전달한다. (2026-11-02에 사용자 요청으로 `LEFT_JOBS.md`와 이 문서를 함께 정리한 것은 이 원칙에 대한 사용자 지시 예외다.)
- **보호·커밋:** 원본/CD·AGENTS.md는 수정하지 않는다. push는 하지 않는다. 커밋 메시지는 `YYYYMMDD-NN DN 요약`이다. 새 문서·코드는 UTF-8과 한국어 주석 규칙을 따른다.

### 0.8 사용자 확인이 필요한 것 (확인 전까지 에이전트 기본값으로 진행)

1. **기존 경로 처리(결정 5):** R3부터는 메뉴, R5부터는 미션의 기본 실행이 새 경로로 바뀐다. 기존 1-1·1-2 근사 플레이는 `--legacy`로만 남기고 R6 뒤 삭제한다. 그동안 새 경로에서 플레이할 수 있는 범위가 일시적으로 줄 수 있다.
2. **한국어(0.7):** 새 경로는 R8 전까지 영어 원본 화면만 제공하고, 한국어 선택은 `--legacy`에서만 쓸 수 있다. 한국어가 계속 필요하면 D2Coding 단색 글리프 출력을 R3으로 앞당긴다.
3. **cpppj 미복원 경로(결정 6):** 새 경로는 cpppj와 같은 명시 오류·경계를 쓴다. 이전 3절 9번의 "근사 유지" 기본값은 기존 경로에만 남는다.
4. **프로젝트 배치(결정 3) — 해결:** `Netstorm.Client`로 만들었다.
5. **`Netstorm.exe`를 어느 실행 파일이 갖는가(AGENTS.md ef0c6b91):** dotnetpj에는 실행 파일이 둘이다. 기존 게임 `Netstorm.Game`(0.96에서 `NetstormClone.exe`를 `Netstorm.exe`로 바꿨다, 배포 대상)과 새 경로의 개발 호스트 `Netstorm.Client.Host`(`Netstorm.Client.Host.exe`, 아직 배포하지 않음)다. 한 배포 폴더에 `Netstorm.exe`는 하나뿐이다. **현재:** 배포되는 `Netstorm.Game`의 `AssemblyName`이 이미 `Netstorm`이라 AGENTS.md 요구는 충족한다. 결정 5의 기본 실행 교체 때(R3 메뉴·R5 미션) `Netstorm.exe`가 새 경로로 진입하게 하는 방법은 둘이다. A) `Netstorm.Client.Host`가 `Netstorm.exe`를 이어받고 기존 Game은 `--legacy` 전용 이름(예: `NetstormLegacy`)으로 내리거나 새 호스트에 흡수한다. B) `Netstorm.Game`이 계속 `Netstorm.exe`이고 새 경로를 라이브러리로 호출한다. 어느 쪽이든 사용자가 보는 이름은 `Netstorm.exe` 하나다. 원하는 방식이 있으면 R3 착수 전에 알려 달라.
6. **4차 MCP의 dotnetpj 해석(AGENTS.md bfccadf5):** AGENTS.md는 'cpppj의 4~7차 목표 내용이 dotnetpj 후순위 요구사항에도 포함된다'고만 적었다. 4차 MCP는 dotnetpj 개발을 위해 게임을 분석하는 용도라서, dotnetpj 클론에도 같은 MCP를 붙이라는 뜻인지 cpppj 쪽 MCP를 개발에 활용하라는 뜻인지 문장만으로는 정해지지 않는다. **기본값:** R8 전에는 구현하지 않고 cpppj의 MCP 결과가 나온 뒤 사용자에게 확인한다. 2026-11-01 AGENTS.md `990a3541`이 4차에 "게임 내에서 모든 조작, 화면 캡처 및 녹화(사운드 포함)가 가능하여야 한다"를 명시했다. 어느 쪽 해석이든 R8의 4차 범위에 이 세 가지(전체 조작·캡처·소리 포함 녹화)를 포함한다. 2026-11-02 `5867f35e`가 이를 "로컬 MCP"로 고쳐 쓰고 4차에 와이드 해상도(16:9, 최대 1920×1080)를 함께 묶었다. "로컬" 한정은 같은 PC의 게임 프로세스를 조작하는 도구라는 뜻으로 보이지만 dotnetpj 클론에 붙이라는 뜻인지는 여전히 확정되지 않았다. 와이드 시야 확장의 방식은 0.4의 `Game/NetstormGame` 교체 행(cpppj의 논리 해상도 방식)을 따른다.

### 0.9 바로 다음 작업

체크 항목은 AI 작업 대상이다. 자연 완주, 전체 동일성, Windows 11 실기는 체크 항목이 아니며 단계 전환을 막지 않는다(0.5).

재개하면 먼저 `python dotnetpj/tools/cpp_sync.py`와 `--ports`로 cpppj 변경 영향을 확인한다. cpppj는 2026-11-02 현재 단위695/표시 11.01이며(0.1의 cpppj 기준) fixture의 `reflected-commit`(`f95a8db2`)보다 앞서 있다. 영향 검토 전에는 기준 커밋을 올리지 않는다.

- [ ] **R4 실제 호스트·월드 조립 (재개 지점):** 미션 객체, Fort reader, Guide 교체·수명, 미션 제어 입력, 공통 Start, AI 설정·생성, Guide 단계·Lifecycle은 이식과 L1 대조를 마쳤지만 검사에서는 명시 효과로만 연결돼 있다. 다음을 실제 호스트와 세션에 조립한다.
  - 완료한 부품의 범위는 1절 색인과 이관 파일의 0.66~0.126을 본다. 미션 세션의 Fort 선택기·지연 reader 소비(0.75), Guide 소유자·명령·Tutorial 반납(0.77), 호스트 프레임의 큐 소비·Guide 우선 분배기(0.78)는 이식했으나 실제 호스트에서 분배기를 설치하고 같은 Player·제한 표·메뉴·월드 효과에 연결해야 한다(0.66, 0.68, 0.69, 0.76~0.79).
  - Tutorial Init의 설정·전역·이름·요새 요청·Header 순서(0.105), 실제 옵션 초기화(전체 옵션 바이트 fixture 대조, 0.106), 요새 요청의 상태144바이트·Zone 복제(0.107), Guide와의 flags·demo·tutorialNumber·missionNumber 및 현재 Config 결합(0.108), 기술 허용 목록과 공유 제한 표 쓰기(0.109), 초기화 순서 조립(0.110)은 이식했다. 실제 템플릿/옵션 소유자·요새 선택·상태 실행·실제 타입 조회와 호스트 단계 연결은 미완이다.
  - Start의 실제 Config·Player·SP·AI·Gump 효과(0.70)
  - AI 설정의 Session 소유자, 실제 AI 생성자·초기 갱신·전략 AI(0.71, 0.72)
  - `PlayerParticipation`의 현재 getter는 이식했다(0.74). 실제 세션 바인딩과 동맹 수신 `ReceiveAlliance` 등 나머지는 미완료다.
  - **Template/DataManager 계층 (0.127에서 미이식 확인, 0.128에서 원문 계층 이식, 0.130~0.136에서 저장소 TS0~TS6 이식, F1 이후·로더는 미이식):** 0.128에서 `o/RawMemStream`, `RawDataManager*`(Lifetime·Selection·List·Resize·Image·FileSave·Initialize·Dump), `RawSnart`, `RawTemplateLifetime`·`RawTemplateLoad`·`RawTemplateSectionNames`, `RawFortImage`, `RawFortCurrentName`, `RawPlayerTemplateState`, `FortSerializationGlobals`와 `Template`의 타입 표가 필요 없는 부분(`FortTemplate` 자료·섹션 조회·`ReadSections`)을 `Netstorm.Core/Original`로 옮기고 원본 x86 fixture 14개(합 약 2만 행 이상)로 대조했다. 남은 것은 [Template 저장소 이식 계획](docs/dotnet-template-storage-plan-20261101.md)을 따른다(설계 네 가지는 2026-11-01 사용자 확정, 3절 12번). TS0(버퍼형·선행)은 0.130에서, TS1(설치·선택)은 0.131에서, TS2(동적 섹션)는 0.132에서, TS3(파일)은 0.133에서, TS4(빈 manager·로그인 이미지·송신 패킷)는 0.134에서, TS5(Editor 파일 계층)는 0.135에서, TS6(새 맵·이름·목록)은 0.136에서 완료했다. 다음은 F1 타입 표·`Parse`(재개 지점)이고, 뒤이어 F2 Fort 적재·저장 26개 모듈, F3 `RawCampaignSession` 연결이 온다. 미션 Fort 적재와 Edit 진입은 F3까지 끝나야 동작하며, TS 단계만으로는 화면 변화가 거의 없다.
  - 0.5 R4의 나머지 범위
- [ ] **R3 잔여 → 이정표 D1:** 위젯 본체와 실제 호스트의 표시·입력 경로는 대부분 옮겼으나(1절 색인의 호스트·메뉴 행) R3 전체 완료로 세지 않는다. 남은 것:
  - 대화창 분리(`ParseDialogPage`)와 `ConfigOverwriteAndQueueLifetime`의 남은 부분, MessageGump·대화창
  - 맵/파일/기술 목록 owner, 실제 상위 메뉴 소유·분배, `RawMenuControls`의 전체 모달·등록 소유자와 링크·명령 소비자, Guide 우선 연결 등 기타 상위 명령·실제 맵/기술 목록 조립, HTML 그림 자산과 다른 파생·월드 통합 출력
  - 전체 자산 조립, `ReadAssetQuickHelpType`의 TYPE/AST·요구 에너지 후처리, 음향
  - `RawTextEntryControls`의 Console·부분 생성 정리·시스템 클립보드, Save as·숫자 창·ChatWindow, `RawBackgroundGifControls`의 노드·창 소유자
  - 호스트 프레임·입력 큐, report·kernel-tell: 메인 메뉴 라벨 좌표는 실제 버튼에 연결했고(0.85) 하위 화면 라벨과 실제 호스트 명령 소비는 남는다.
  - D1 메뉴 캡처 비교. R0-1의 오라클은 당시 `0cc1d33d` 빌드이고 cpppj 산출물 이름도 `Netstorm.exe`로 바뀌었으므로, 비교 전에 현재 HEAD로 다시 빌드해 SHA를 기록한다.
- [ ] **R2 잔여 세션 연결:** 실자산 어댑터, 수집·정렬·kind 분배, `SpriteDraw` 클래스·그림자 alias 공유, `owner-capture` C행 480개는 R4·R5에서 잇는다.
- [ ] **9-1 A 나머지(2절):** 정수 틱을 실제 시각·원본 타이머로 옮기는 일을 세션 교체와 함께 처리한다.
- [ ] 이후 R3(D1) → R4(M1) → R5(D2) → R6 → R7 → R8 순서로 진행하고, 단위마다 이 절의 체크와 [0.4](#04-기존-dotnetpj-소스의-재수정-판정) 판정표를 갱신한다.
- [ ] **11.01 패치 요구 (AGENTS.md `462c9a64`, 2026-11-01 추가, 미착수):** 새 버전 모두 버전 표기를 11.01로 하고, 메인 메뉴의 Credits 목록(기존 2개)에 세 번째 항목 "Netstorm 11.01 Patch Credits"를 추가한다. 선택하면 다른 Credits 메시지 창과 같은 창이 나오되 제목은 "NetStorm 11.01 Patch Credits", 내용은 HJOW 개발과 AI 사용에 대한 안내다. **cpppj는 685에서 적용을 마쳤다**([문서](docs/exe/cpp-product-1101-20261101.md)): 목록 순서는 `Netstorm 10.72 Patch Credits` → `Netstorm Original Credits` → `Netstorm 11.01 Patch Credits`이고, 새 창은 같은 HTML 창 공장·돌 테두리·Back/Cancel을 쓰며 본문은 문서의 `Patch1101.Credits`를 따른다. 제품 버전은 `v11.01`이고 `--version`·Windows FileVersion/ProductVersion도 11.01이다. 원본 판본 표기(10.78/10.72)와 옛 Credits는 복원 기준이라 바꾸지 않는다. dotnetpj는 기존 경로(`Netstorm.Game`)와 새 경로(Credits는 R3 범위) 양쪽에 적용하며, 버전 표기가 나오는 위치(타이틀·About·창 제목·배포 이름 등)를 먼저 조사한다. 세 프로젝트의 문구가 같도록 cpppj 문서의 본문을 옮긴다.
- [ ] **외부 접속 기능 문서화·클라이언트 유효성 검사 비활성화 (AGENTS.md `0190ba95`, 2026-11-01 추가):** 로컬 멀티플레이 서버 접속을 제외한 외부 도메인/IP 접속 기능은 모두 찾아 따로 문서화하고, 클라이언트 유효성 검사(구 서버 검증·security.dat·Not Validated 안내)는 비활성화한다. cpppj는 684에서 원본의 기능·주소 목록과 처리 방침을 만들었다([문서](docs/exe/cpp-external-services-20261101.md), [목록](cpppj/external-endpoints-20261101.json)). dotnetpj는 이 목록을 기준으로 삼는다: 구 클라이언트 검증·master/root 인터넷 서버·ping/진단은 이식해도 연결하지 않고, 사용자가 선택하는 도움말 링크 실행과 로컬 서버/LAN 접속(3차)은 별도 경계로 둔다. 2026-11-02 C# 소스(.cs) 정적 검색에서 소켓·HTTP 클라이언트 코드는 없었고(`Netstorm.Net`은 `.csproj`뿐) 외부 실행은 기존 경로 `Netstorm.Game/HelpWindow.cs`의 사용자 선택 링크 실행 1곳(교체 대상)이었다. 남은 일: dotnetpj 쪽 소비자 목록을 단위 문서로 남기고, 새 경로의 Help 링크(R3)·시작 화면의 검증 안내·`Netstorm.Net`(3차)을 옮길 때마다 갱신한다.
- **이월한 이식 한계(2026-11-02 정리 때 0.126 이하 단위 기록의 '남은 작업'에서 옮김, 이후 단위에서 해소됐다는 기록이 없는 것만):**
  - 0.80: 초기 구름 카메라 원점은 임시 범위이고 실제 카메라(R5)·시간별 이동·부분 dirty는 미연결이다. 512×512 구름 파일이 없거나 크기가 다를 때의 원본 생성 fallback은 이식하지 않아 명시 오류다.
  - 0.81: 반복 `--set`은 C# 호스트가 병합하고 같은 이름은 마지막 값을 쓰지만 cpppj는 마지막 묶음만 보관한다. 오라클 연결 때 계약 차이를 정리한다.
  - 0.86·0.87: 프레임 사이의 빠른 물리 입력 이력, 공통 음향·음악(R6 범위)과 옵션 변경 중 재생 관리가 남았다.
  - 0.93: 부분 생성 중 돌 버튼 내부 배경 할당 실패의 모든 사례를 검증하지 않았다.
  - 0.126: cpppj와의 이관 장부 교차 읽기, `MatchDisk`의 Win32 특수 경계(절대 경로·긴 경로)는 검증하지 않았다. Native 이름 변경 장치와 Save as의 주입 지점은 0.136(TS6)에서 이식했고, 실제 Editor State1 월드·저장·Save as의 Template/현재 이름 세션 소유자는 미이식이다(F3 범위).

### 0.10~0.126 — 이관

2026-10-28 문서 작업 기록과 단위 기록 0.11~0.65는 [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073)로, 단위 기록 0.66~0.126은 [이관 파일의 2026-11-02 절](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)로 옮겼다. 완료 범위는 1절 색인을 본다. 아래 0.127 이후는 별도 단위 문서가 없고 다음 작업(Template 저장소 F1 이후)의 직접 근거라 본문에 남겼다. 0.126 이하에서 이후 단위로 해소되지 않은 한계는 0.9의 '이월한 이식 한계' 항목으로 옮겼다.

### 0.136 단위 기록 — Template 저장소 TS6: 이름·새 맵·목록 (2026-11-02, HJOW-Athlon)

- **구현:** (1) `Netstorm.Core/Original/RawNewFort.cs`(새): cpppj `o/RawNewFort`를 옮겼다. `CreateNewFortManager`(004bfba0: 기존 manager 진단 0x172 → 헤더 +4=0·+16=인자 → 존재 재조회가 참이면 임시 manager 생성·이름 읽기·삭제·소멸·해제 → 최종 manager 생성·헤더 +0 공개 → 초기화, 임시 manager가 NULL이면 가상 이름 접근 실패로 멈춤), `CreateNewFortData`(004b3920: Subscriber -1, "Unnamed" 복사, 설정·제목·등록, Editor가 아니면 초기 레코드 값과 난수 254, SP 차감, CoreData 10바이트 지움·쓰기, 하위 WORD가 0이 아닌 seed가 나올 때까지 재시도, Subscriber·Territory·Money 저장, State kind·알림), `InstallMissingNewFort`(004b5ca0)와 장치 묶음 클래스 4개·전용 대리자 3개. (2) `Netstorm.Client/Host/NewFortManagerDevices.cs`(새, `CreateNewEditorFortManager`), `ClientFortGlobals.cs`(새: 현재·대기 이름 256바이트 두 버퍼, 선택 FileSpec 포인터, 최초 dirty가 1인 목록 캐시, 선택 모드), `FortCurrentNameDevices.cs`(새: 이름 부모를 이름 버퍼·저장소·설정·작업본 파일 장치에 연결, `SelectConfiguredNewFort` 포함). (3) `Netstorm.Client/Mission/SoloMapFileSpec.cs`(새: 같은 9바이트 키), `SoloMapCatalog.cs`(재작성: 직접 구현했던 서명·섹션 판독 `Probe`를 없애고 분류용·메타용 임시 `CampaignTemplateStorage`와 `RawPlayerTemplateState`(Patch1078)로 바꿨다. 장치는 cpppj처럼 `SoloMapCatalogEffects` 하나로 받으며 TYPE 개수·미정 바이트·사용자·번역·파일 키·공유 직렬화 전역·공유 목록 상태·열거/읽기 장치·공유 이름표를 갖는다), `FortSelection.SetPendingFortName`의 물리 판(목적·원본 겹침과 진단 뒤 현재 원본 재읽기를 보존). (4) `HostFortNameConfirmation`에 세션이 정하는 Template 소유자 헤더(`header`)와 현재 이름 설정(`setName`) 선택 인자를 더했다(기본은 기존의 명시 오류). `HostEditorSelection`은 새 생성자로 목록을 만들고 TYPE 개수는 10.78 상수 188을 임시로 쓴다(F1의 `RiftTypeTable`이 오면 교체).
- **C#에서 달라진 모양:** C++ DWORD 참조(`uint32_t&`)는 읽기·쓰기 대리자 쌍(`RawNewFort`의 Subscriber·CoreActive·CoreSeed)이고, `InstallMissingNewFort`의 alternate는 `ref uint`다. `std::span`을 받는 장치는 전용 대리자(`NewFortBytes`·`NewFortSubscriberWrite`·`NewFortTerritoryWrite`·`SoloMapAllocationBytes`)다. `SoloMapFileSpec.Current()`는 문자열 사본이라 키를 바꿔도 이미 받은 값은 안 바뀌고, 같은 물리 버퍼는 `Buffer`로 따로 노출한다. 이름 버퍼의 바이트는 UTF-8이고 `ClientFortGlobals.Current()/Pending()`가 UTF-8 문자열로 읽는다. cpppj `FortCurrentNameDevices`가 하는 두 dirty DWORD의 주소 비교는 C#의 `FortCurrentNameGlobals`가 대리자 쌍이라 할 수 없어 생략했다(호출자가 `ClientFortGlobals.Catalog`의 dirty에 연결해야 하고 검사가 효과로 확인한다). 경로 비교(`SamePath`)는 CRT처럼 ASCII 대소문자만 무시한다. 설정 기록은 C# `OriginalText.Decode`가 유효한 UTF-8을 UTF-8로 읽으므로 한글 이름이 올바른 문자열로 기록된다(cpppj는 바이트를 CP1252로 보고 UTF-8로 바꾸므로 비ASCII 이름이 이중 인코딩되는 차이가 있다).
- **검증:** (1) 원본 x86 fixture `new-fort-manager`(48행)·`new-fort-data`(1,248행)·`new-fort-missing`(26행)·`new-fort-missing-bits`(78행)을 새로 연결해 **1,400행 모두 일치**했다(불일치 0). 외부 재진입으로 전역이 바뀌는 13종 변이표와 seed 하위 WORD 0 재시도를 포함한다. Core `RawNewFortTests` 6개(x86 4 + 물리 경계 직접 1 + 기존 파일·미연결 장치 직접 1). (2) Client 새 32개: `NewFortManagerDevicesTests` 7(cpp 5 + 직접 2: 진단 미연결·슬롯 범위, 한글 경로 UTF-8), `FortCurrentNameDevicesTests` 14(cpp 7 + 직접 7: 삭제 장치 없음, newFort 선택, 한글 이름, 대소문자만 다른 같은 이름, 접두 이름 삭제, 절대 FileSpec 대소문자, 경로 비교 훅), `SoloMapCatalogStorageTests` 7(cpp 6 + 직접 1: 기본 키 전환), `EditorCatalogWorkspaceTests` 1(cpp `EditorCatalogFiles` 1), `HostFortNameConfirmationTests` 3(직접: 실제 저장소·이름 장치로 이어진 Save as, 헤더 미주입, 이름 장치 미주입). 기존 `SoloMapCatalogTests` 2개와 `EditorCatalogWorkspaceTests`의 목록 도우미는 **생성자만 새 모양으로 바꿨고 기대값은 바꾸지 않았다**. 실제 원본 433개를 새 저장소 경로로 읽는 `ReadsRealFortTypesAndBuildsEditableRows`가 그대로 통과했고, 독립 원시 읽기로 세 모드×편집 허용 조합의 필터·50개 상한·손상·작성자·행을 대조하는 `ProtectedFilesAndCache`도 통과했다. Client 전체 Release 841개(809 + 32), Core 전체 1,125개(1,119 + 6) 통과, 실패·건너뜀 0. PORT_MAP 무결성 2개와 fixture 목록 164개 검사도 통과했다. 변이 66종(Core `RawNewFort` 27, `NewFortManagerDevices` 8, `SoloMapCatalog` 13, `SoloMapFileSpec` 2, `ClientFortGlobals` 2, `FortCurrentNameDevices` 10, `FortSelection` 2, `HostFortNameConfirmation` 2)을 `extracted/dn-mutation-ts0` 사본에서 시도했다. 첫 실행에서 5종을 놓쳤다. 이 중 경로 비교의 길이 확인 제거(H05)와 대문자 하한 경계(H12)는 실제 경로가 항상 같은 확장자로 끝나 관측되지 않아, 장치가 연결한 비교 훅을 직접 호출하는 검사를 더해 검출했다. 남은 3종은 등가 또는 관측 불가다: 삭제 때 경로를 먼저 해석하는 호출 제거(D08, C# `EditorFiles.Delete`가 안에서 같은 해석을 하는 중복), 목록 임시 Template의 헤더 isMine=1 쓰기를 0으로 바꾸는 변이(E04)와 분류용 임시 Template의 isMine=0을 1로 바꾸는 변이(E17)이다. 이 값을 읽는 Notify·저장 경로를 목록이 부르지 않아 cpppj에서도 관측되지 않는다. 나머지 63종은 검출했다. 원본 x86 fixture가 없는 어댑터 계층(저장소 소비자 장치)은 L1이 아니고, `RawNewFort`는 fixture 대조라 L1이다.
- **옮기지 않은 cpppj 검사:** `SoloMapCatalogTests` 3개 — `DiskEnumerationAndArchiveRead`(TAFF 아카이브 등록이 필요하고 편집 작업본의 아카이브 전용 읽기 검사가 같은 경계를 확인한다), `CurrentSubscriberSection`(저장소 TS1의 Subscriber 읽기 경계 검사가 같은 구간을 확인한다), `TellMetadataUsesActualFilesAndCurrentNames`(대화 페이지 파서와 결합하며 Tell 맵 열 검사 `TellMapMenusTests`가 맡는다). `CampaignNewFortTests.inc` 3개·`CampaignNewFortConfigTests.inc` 1개는 `RawCampaignSession`, `TemplateDiagnosticClientTests`의 Client 필터 부분은 `ClientDiagnostics`가 필요해 F3으로 넘긴다.
- **PORT_MAP:** `RawNewFort.cpp/.h` → `RawNewFort.cs` 2행 `L1`(기준 커밋 `51e2d8f4`), `NewFortManagerDevices`·`ClientFortGlobals`·`FortCurrentNameDevices`의 각 .cpp/.h 6행, `SoloMapCatalog.h/.cpp` → `SoloMapFileSpec.cs` 2행, `SoloMapCatalog.h` → `SoloMapCatalog.cs` 1행을 `port`(기준 커밋은 각 cpppj 파일의 마지막 변경 커밋 `51e2d8f4`·`f4493776`·`2c7b3437`·`c4b75d7a`)로 더했다. 385행(L1 199, port 186). `cpp_sync.py --ports`가 보고한 변경 쌍 중 `SaveFortNameDialog`·`RawMenuSaveFort.inc`(HTML 문자 숫자 입력 창 `967ca80d`)는 이 단위의 범위가 아니라 재대조하지 않았다(기준 커밋을 올리지 않음).
- **실제 호스트:** `Netstorm.Client.Host.exe --window --frames 20`이 종료 코드 0으로 끝났다(HJOW-Athlon, AGENTS.md 확인 면제 시스템). 이 실행은 호스트 시작에서 새 목록 생성자(`HostEditorSelection`)가 연결되는 것까지만 보며, Edit 버튼을 눌러 목록을 여는 호스트 경로는 실행하지 않았다(그 경로의 목록 읽기는 실제 원본 433개를 쓰는 Client 검사가 대신 확인한다). 이 실행 뒤 추적 파일 `assets/game-data/d/options.cfg`가 바뀌어 있어(실행이 다시 쓴 것으로 보이며 원인이 `--set tellTips=0`의 저장인지 시작 시 기록인지는 확인하지 않았다) `git checkout`으로 되돌렸다. 호스트를 실행한 뒤에는 이 파일의 변경 여부를 확인한다. 이름 장치와 새 맵 manager 장치는 호스트가 아직 소비하지 않는다. 원본·CD 파일은 수정하지 않았고 원본 게임을 실행하지 않았다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다.
- **남은 것·주의:** F1(`RiftTypeTable`·`RiftType`, `FortTemplate.Parse`)부터다. `HostEditorSelection`의 TYPE 개수 상수 188과 `FortTemplate.ReadSections`를 쓰는 검사들은 F1에서 실제 타입 표로 바꾼다. `HostFortNameConfirmation`의 `header`·`setName` 실제 연결(현재 Template 소유자와 전체 snapshot)은 F3이다. `FortCurrentNameDevices`의 비ASCII 경로는 한글 이름 검사로만 확인했다. 같은 작업 트리에서 다른 세션이 cpppj와 분석기 파일을 수정·커밋했고(`85e62114`·`8c2caaaf`) 이 단위는 그 파일들을 쓰지 않았다(HEAD 기준만 사용).

### 0.135 단위 기록 — Template 저장소 TS5: Editor 파일 계층과 한글 작업 경로 이름 (2026-11-02, HJOW-Athlon)

- **구현:** (1) `EditorFortFileDevices.ExistingEditorTemplateValid`(정적 메서드): cpppj 자유 함수(원문 0041aad0)를 옮겼다. 현재 헤더 첫 DWORD의 네 바이트 중 하나라도 0이 아니면 1이고 토큰 조회나 파일 서명은 보지 않는다(호스트가 모르는 비영 토큰도 1). 9칸 밖은 `ArgumentOutOfRangeException(slot)`, null 저장소는 `ArgumentNullException`이다. `BindLoad`는 0.127에서 이미 옮겼고 이번에 `LoadFile`에 실제로 연결해 검사했다(읽기·readonly 판정·ASCII `stricmp` 비교). (2) `Netstorm.Client/Host/TemplateFileNames.cs`(새): manager+c의 이름 바이트 문자열(문자 하나가 바이트 하나, Core `RawTemplateLoad`와 같은 표기)과 호스트의 실제 문자열 사이를 UTF-8로 바꾼다. cpppj는 이름이 UTF-8 `std::string`이라 한글·이모지 작업 경로(5차 목표의 사용자 홈 경로 포함)가 그대로 manager에 UTF-8 바이트로 들어가지만, TS1~TS4의 C# 저장소는 이름을 문자 하나 = 바이트 하나로만 쓰고 읽어 한글 경로를 담지 못하고 저장 때 쓰기 장치에 깨진 경로가 넘어갈 상태였다(그 단계의 검사는 모두 ASCII 이름이라 드러나지 않았다). 변환을 `Image` 생성자·`ConstructName`·`CurrentName`과 이름이 호스트 장치로 나가는 지점(읽기, readonly, 이름 비교, 서명 거부 요청, 권한, 쓰기)과 들어오는 지점(`LoadFile` 요청 이름)에 넣었다. 저장소 안쪽(Core 호출, 이름표 검색, `Dump` 출력)은 계속 바이트 문자열이다. ASCII 이름은 동작이 같다. 이름 한도는 UTF-8 바이트 수 256 미만이다(한글 85자까지).
- **C#에서 달라진 모양:** cpppj의 `ExistingEditorTemplateValid`는 헤더 파일의 자유 함수지만 C#은 `EditorFortFileDevices`의 정적 메서드다. cpppj의 `FortTemplate::Parse`는 아직 없어(F1) 검사에서는 같은 섹션 읽기만 하는 `FortTemplate.ReadSections`로 대신했다. cpppj 검사는 실제 보호 루트(`originals`·`originalCD`)를 읽어 보호 경로로 넘겼지만 C# 검사는 임시 폴더에 둔 복사본을 보호 루트로 쓴다(저장 경로가 틀려도 원본에 쓸 수 없게 한다). 실제 TEST01 원문은 읽기 전용 입력으로만 쓰고 끝에 다시 읽어 변하지 않았음을 확인한다. `FortSelectionHooks.Core`는 `Memory<byte>`를 요구해 검사에서 `state.Get(slot)`을 부른 뒤 `storage.State(slot).Bytes`를 돌려준다.
- **검증:** `Client.Tests`에 `EditorTemplateFileTests` 14개를 추가했다. cpppj 검사 10개: `EditorFileWriterTests`의 실제 저장(현재 manager 이름·서명·기존 끝 보존, 한글·이모지 경로) 1개, `EditorFilesTests` 5개(TEST01 Notify 저장·다시 읽기와 저장 억제·숨김 소유자·빌린 헤더 isMine·+16 관문, 새 manager 서명 -1 정규화와 signed 타입 수·현재 섹션 검색, 짧아진 Template의 기존 끝 보존, 보호 이름 거부, Fort 선택의 State·Notify·저장·다시 읽기), `EditorFortFileDevicesTests`에서 저장소를 쓰던 부분 4개(저장 억제 DWORD 관문, 유효성, 준비한 수정본 Load, 실제 readonly 속성이 Config와 독립으로 헤더 +8에 반영). 직접 4개: 한글 이름의 UTF-8 보관과 바이트 수 한도, 빈 manager 생성의 같은 규칙, 서명 거부 요청이 실제 경로를 받음, `LoadFile`이 장치에 실제 경로를 넘기고 다시 읽으면 같은 이름으로 판정. Client 전체 Release 809개(새 14개) 통과, 실패·건너뜀 0. Core는 이 단위에서 바꾸지 않았고 PORT_MAP 무결성 2개만 다시 돌려 통과했다. 변이 31종(`ExistingEditorTemplateValid` 7, 이름 변환 도우미 3, 이름이 흐르는 호출 지점 14, 서명 flag·FE 판정·0x46 3, Template 쓰기 잘라쓰기 1, `BindLoad` 3)을 `extracted/dn-mutation-ts0` 사본에서 시도했다. 첫 실행에서 4종을 놓쳤다. 슬롯 경계 `>=`→`>`(E4)는 슬롯 9에서 `Slice`가 같은 예외 형식을 던져 가려졌기 때문이라 검사가 예외 매개변수 이름(`slot`)과 `uint.MaxValue`를 확인하게 고쳐 검출했다. 남은 3종은 등가 변이다: `Image` 생성자·`ConstructName`·`ManagerByteCount`가 바이트 수 대신 문자 수로 한도를 검사해도 Core `ConstructManager`가 변환된 바이트 문자열이 256자 이상이면 같은 `ArgumentException`을 내는 중복 방어다. 28종을 검출했다. 원본 x86 fixture가 없는 어댑터 계층이라 L1이 아니다.
- **PORT_MAP:** `CampaignTemplateStorage.cpp` → `TemplateFileNames.cs` 1행을 `port`(기준 커밋 `ce5f58f9`)로 추가했다. cpppj에 대응 파일이 없는 C# 보조 변환이라 이름을 쓰는 저장소 본체 파일에 대응시켰다. 374행(L1 197, port 177). `EditorFortFileDevices` 대응 행의 기준 커밋(`473432cd`)은 `cpp_sync.py --ports`상 변경이 없어 그대로다.
- **실제 호스트:** 저장소와 Editor 파일 장치를 호스트가 아직 소비하지 않아 호스트 실행은 하지 않았다. 원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다.
- **남은 것·주의:** TS6(`RawNewFort`·`NewFortManagerDevices`, `ClientFortGlobals`·`FortCurrentNameDevices`, `SoloMapCatalog.Probe`를 실제 저장소로 교체, `HostFortNameConfirmation` 연결 범위 판정)부터다. `BindCurrentName`은 manager+c의 256바이트를 그대로 빌려주므로 이름 변경 장치(TS6)가 한글 경로를 UTF-8 바이트로 쓰는지 그때 확인한다. `Dump`가 출력하는 파일명은 바이트 문자열 그대로라 로그에서 한글 경로는 UTF-8 바이트 문자로 나온다(cpppj도 UTF-8 바이트를 그대로 출력). 짝이 맞지 않는 UTF-8(옛 ANSI 이름)은 manager에서 꺼낼 때 U+FFFD로 바뀌어 왕복하지 않는다(현재 호출 경로는 모두 UTF-8 경로다). 같은 작업 트리에서 다른 세션이 cpppj 파일(`RawConstructionNotice`·`UberGump` 등)을 수정·추가 중이었고 이 단위는 그 파일들을 쓰지 않았다(HEAD 기준만 사용).

### 0.134 단위 기록 — Template 저장소 TS4: 빈 manager·로그인 이미지·송신 패킷 (2026-11-02, HJOW-Athlon)

- **구현:** (1) `Netstorm.Core/Original/RawMissionLoginPackets.cs`: cpppj `o/RawMissionLoginPackets`를 옮겼다. `MissionFortPacket`(이미지+20바이트 예약 `Storage`와 길이 WORD만 빌리는 `Bytes()`), `MakeMissionFortPacket`(전체 0xff 채움, 종류 0x2a, 길이 WORD는 초기값 4 이상, owner·predictable·이미지 크기·watcher 필드, 끝 3바이트는 0xff 보존), `FinalizeMissionFortPacket`(observer false면 읽지 않음, 크기 0xffffffff 진단, `크기+17`을 하위 16비트로 감아 씀), `RawMissionLoginPackets.SendFort`(현재 Template 없으면 자연 false → 로그 → 누락 manager 진단 뒤 재조회 → 이미지 → 길이 WORD → watcher → 예약·확정 → 송신). 장치 묶음 `MissionFortPacketReads`·`MissionFortPacketHooks`와 송신 대리자 `MissionFortPacketSend`. (2) `Netstorm.Client/Host/CampaignTemplateStorage.Login.cs`(공개 면): `AllocateEmptyManager`(276 고정, 아직 생성하지 않은 빈 Image), `ConstructEmptyManager`(원문 생성자와 공유 version 0), `ManagerBytes`, `InitializeEmptyManager`(공유 이름표로 35개 Create, 진단 장치 선택), `ManagerLifetime`, `BindDiagnosticLog`, `ReadLoginImage`(임시 저장소에 지역 Template를 읽고 `NONAME`에 Subscriber·Territory·TypeNames·CoreData와 현재 `Terr%02d`만 복제해 덤프·직렬화한 뒤 target→source 순으로 반납, 전역 9칸 헤더·manager·커서·직렬화 버전·이름표는 건드리지 않음). (3) `CampaignTemplateStorage.Image.cs`에 빈 생성자(모든 표준 위치를 연결된 것으로 표시), `ConstructName`, `NextStreamHandle`, `CampaignTemplateStorage.ImageLogin.cs`에 `CloneSectionFrom`(두 manager의 노드·버퍼를 한 복제 장치로 빌림, 대상 노드 핸들이 원본 범위와 겹치면 거부), `Dump`(파일명→접두 로그→이름표→current 검색→합계, 출력 필터는 장치가 판정하고 검색은 생략하지 않음), `SaveImage`(파일 쓰기 없이 서명 flag·섹션 조회를 사본 장치에 채워 35개 이름 이미지를 작성).
- **C#에서 달라진 모양:** `MissionFortPacketReads`·`MissionFortPacketHooks`는 `record`라 `with` 식 복사가 C++의 값 전달과 같다(`TemplateFileLoadHooks`와 같은 방식). 자유 함수 두 개는 `RawMissionLoginPackets`의 static 메서드다. 9명 Player 바이트는 `ReadOnlyMemory<byte>`로 빌리며 복사하지 않아 `InitialLength`가 Player 바이트를 바꾸는 행(모드 3)의 원문 순서를 그대로 보존한다. `BindDiagnosticLog`는 `DataManagerTrace`(수준, 문자열)를 받는다. `new(nothrow)`의 NULL 반환(`AllocateEmptyManager`가 0)은 C#에 할당 실패 경로가 없어 옮기지 않았다. `Terr%02d`는 DWORD 비트를 signed로 보고 음수는 부호 뒤에 그대로 쓴다("Terr-5"). `TemplateFileLoadHooks`는 `ReadLoginImage`가 `with`로 `CurrentSlot`(항상 9)과 `RefreshTitle`(없음)만 덮어쓴 사본을 쓴다.
- **검증:** (1) 원본 x86 fixture `fort-image-packets`(이미 0.128에서 연결) 중 이번에 송신(S) 93행과 예약(F) 10행을 대조한다: 현재 Template 읽기·길이 WORD 읽기·로그·누락 진단의 순서(trace), 이미지, 길이 확정 전후 예약 전체, 최종 wire를 행마다 비교하며 **103행 모두 일치**했다(불일치 0). 저장(I) 116행은 0.128이므로 이제 219행 전체가 연결된다. 보호 fort 8개 입력은 `FortTemplate.Parse`(F1) 대신 `ReadSections`를 쓴다. (2) `Core.Tests`에 `MissionFortPacketsMatchNative103Rows`·`MissionFortPacketSelectedBoundaries`(cpppj `FortImagePackets_SelectedBoundaries` + 판본·Player 크기·정확히 0x8000 이미지 경계를 직접 더함) 2개. (3) `Client.Tests`의 `CampaignTemplateLoginTests` 8개: cpppj 검사 중 `CampaignTemplateLoginImage` 2개(실제 `thewarbegins.fort`에서 다섯 섹션만 남기고 영역 0·7·19 각각의 payload·빈 나머지·전역 owner/이름표/현재 선택 보존, 정상 false·장치 예외·부분 읽기 예외·현재 Terr 누락 뒤 전역 보존과 다음 정상 읽기), `TemplateDiagnosticClient`의 저장소 호출 부분 1개(덤프 두 줄과 필터된 장치에서도 같은 이미지, 장치 미연결 거부), `DataManagerInitializeTests`의 빈 Image 1개(`FortTemplate::Parse`는 `ReadSections`로, 앞 34개 빈 섹션의 포인터 0·용량 0과 마지막 Terr19의 16384 용량, TEST01 reader와 이름표 공유), 직접 4개(영역 이름 서식, 빈 Image 계약, 늦은 표준 섹션만 만들 때 가짜 선행 노드 없음, 재초기화 root 진단·수명). Core 전체 Release 1,119개(새 2개), Client 전체 795개(새 8개)와 PORT_MAP 무결성 2개 통과, 실패·건너뜀 0. 변이 34종을 `extracted/dn-mutation-ts0` 사본에서 시도했다. 첫 실행에서 5종을 놓쳤다. 이 중 빈 Image의 표준 위치 연결표시 제거(C4)는 늦은 표준 섹션만 먼저 만드는 경우를 직접 확인하는 검사가 없었기 때문이라 `EmptyImageDoesNotProjectPhantomPredecessors`를 더해 검출했다(사본의 증분 빌드가 시각이 오래된 복사 파일을 다시 컴파일하지 않아 한 번 미검출로 잘못 나왔으므로 사본에 복사한 파일은 반드시 `touch`한다). 남은 4종은 등가 변이다: 생성 이름 미기록(C3, `Image.Name`은 `CurrentName()`이 manager+c에서 다시 읽어 덮어쓰는 투영이고 다른 읽기가 없다), 복제 뒤 `Resize(end)` 제거(C16, `Copy`가 원문 end만큼 정확히 할당해 무동작이며 원문 data가 0이라 복사를 건너뛰는 경우는 `ReadLoginImage`의 파일 원본에서 만들어지지 않는다), 복제 중 이전 버퍼 반납 제거(C17, 임시 NONAME 섹션의 용량만 달라지고 이미지·덤프에 반영되지 않는다), 복제 노드 불일치 방어 검사 제거(C19, `CloneStream`은 항상 방금 할당한 노드를 돌려줘 도달 불가). 나머지 30종을 검출했다. 원본 x86 fixture가 없는 어댑터 계층(저장소)은 L1이 아니고, `RawMissionLoginPackets`는 fixture로 대조해 L1이다.
- **PORT_MAP:** `RawMissionLoginPackets.cpp/.h` → `RawMissionLoginPackets.cs` 2행을 `L1`(기준 커밋 `05d7cb57`)로, `CampaignTemplateStorage.cpp` → `CampaignTemplateStorage.Login.cs`·`CampaignTemplateStorage.ImageLogin.cs` 2행을 `port`(기준 커밋 `ce5f58f9`)로 추가했다. 373행(L1 197, port 176). `cpp_sync.py --ports`는 Template·MemStream·DataManager 계열 cpppj 파일과 `RawMissionLoginPackets`의 변경이 없음을 확인했다.
- **실제 호스트:** 저장소와 패킷 계층을 호스트가 아직 소비하지 않아 호스트 실행은 하지 않았다. 외부 접속 관점: `MissionFortPacketSend`는 호스트가 공급하는 대리자일 뿐이며 이 단위는 소켓·HTTP 코드를 추가하지 않았다(로컬 멀티플레이 송신은 3차).
- **남은 것·주의:** TS5(`ExistingEditorTemplateValid`, `EditorFortFileDevices.BindLoad`의 실제 사용, 0.127에서 미룬 `EditorFileWriter` 1·`EditorFiles` 5·`EditorFortFileDevices` 4)부터다. cpppj `TemplateDiagnosticClientTests`의 Client 필터·FILE·공유 버퍼·NULL Chat 부분은 `ClientDiagnostics`가 없어 F3으로 넘긴다(저장소는 덤프를 `DataManagerTrace` 장치에 한 번씩 전달하는 데까지 검증). `CampaignMissionFortPathTests`·`CampaignLoginObjectStreamTests`·`CampaignMissionReloadSourceTests`는 `RawCampaignSession`이 필요해 F3이고, `NewFortManagerDevicesTests`·`SoloMapCatalog_ProbeManagersShareClientSectionNames`는 TS6이다. `CloneSectionFrom`의 대상 노드 핸들 이어쓰기(`NextStreamHandle`)는 원본이 아니라 호스트 노드 식별용이다. 같은 작업 트리에서 다른 세션이 cpppj 파일(`RawFortSubscriberChange` 등)을 수정·추가 중이었고 이 단위는 그 파일들을 쓰지 않았다(HEAD 기준만 사용).

### 0.133 단위 기록 — Template 저장소 TS3: 파일 읽기·저장 (2026-11-02, HJOW-Athlon)

- **구현:** cpppj `client/CampaignTemplateStorage`의 파일 범위를 `CampaignTemplateStorage.ImageFile.cs`(`Image` partial)로 옮겼다: `ReadFile(TemplateFileLoadHooks)`(빈 manager면 35칸 빈 요새로 시작, 아니면 현재 목록과 동기화한 뒤 원본 파일 읽기 부모·서명·섹션 병합을 실행하고 성공·예외 모두 `BindFileStreams`로 원문이 붙인 노드를 표준 위치로 인계), `ReadFile(bytes, typeCount)`(빈 manager 전용 래퍼), `BindFileStreams`, `SaveFile`(manager+10c를 'F'로, FE 판정 미설정(0xffffffff)이면 0으로 정규화한 현재 값으로 서명 flag 결정, 35개 이름을 실제 목록에서 찾아 직렬화), `SectionByIndex`. 저장소 공개 면에는 `InstallFile`·`LoadFile`·`SaveFile`·`BindCurrentName`을 더했다. 파일 이미지는 입력 토큰(0x30000), 저장 이미지는 토큰(0x50000)으로 노드·임시 토큰과 겹치지 않게 했다.
- **C#에서 달라진 모양:** `TemplateFileLoadHooks.Read`는 `byte[]`를 돌려주며 빈 배열이 원문 open 실패이고 예외와 구별된다. `SaveFile`은 호출자가 넘긴 `FortImageHooks`를 바꾸지 않고 사본에 서명 flag·섹션 조회를 채운다(C++의 값 전달). `BindCurrentName`은 값 전달 대신 받은 `FortCurrentNameHooks` 객체에 채워 같은 객체를 돌려준다. 잘못된 서명의 파일 제거 요청은 `InstallFile`에서 `InvalidDataException`(보호 원문은 삭제하지 않는다)이고 파일 열기/서명 거부는 `InvalidOperationException`이다.
- **검증:** `CampaignTemplateFileTests` 19개: cpppj 검사 13개(`DataManagerReadTests` 2, `CampaignTemplateLoadFileTests` 9, `TemplateSerializationGlobalsTests` 1 중 InstallFile 부분, `CampaignTemplateDynamicTests` 1 중 파일 저장 부분), **원본 데이터 .fort 433개 전체의 읽기→저장 왕복 1개**, 직접 5개(저장 관문과 호출자 장치 불변, 서명 flag, 타입 수 진단, 누락 섹션 거부, 이름 부모 연결). 왕복은 `d/*.fort` 433개를 `InstallFile`로 읽고 `SaveFile`로 다시 쓴 뒤 섹션 35개와 flag를 입력 파일 자체와 대조하며 **433개 모두 일치**했다(불일치 0). 원본 데이터의 사실: 433개 모두 섹션이 35개이고 flag는 0(427개)과 0xfe(6개)뿐이며, 87개는 35번째 섹션 뒤에 뒤꼬리 바이트(1~759바이트)가 있다. 저장 결과는 항상 뒤꼬리 없이 35섹션이므로 그 87개는 뒤꼬리만 사라지고 섹션·flag는 같다. `TemplateStorageTestSupport`에 노드 관찰(`Observe`)과 할당 배열 조회(`ArrayOf`)를 모았다. Client 전체 Release 787개와 PORT_MAP 무결성 2개 통과, 실패·건너뜀 0. 변이 23종을 `extracted/dn-mutation-ts0` 사본에서 시도했다. 첫 실행에서 7종을 놓쳐 부분 읽기 뒤 요새 35칸과 동기화 후 목록 유지, 빌린 칸에서의 저장 검사를 더해 3종을 검출했다. 남은 4종은 등가 변이다: 파일 읽기 뒤 `PublishCursor`(`BindRenamedBuffer`가 이미 공개), 전체 이름 표 개수 확인(`ReadImage`가 이름표 전체를 돌아 도달 불가), 빈 파일의 입력 토큰 반환(서명 읽기가 end 0을 같은 false로 처리), 저장 이미지 포인터 확인(항상 같은 토큰). 원본 x86 fixture가 없는 어댑터 계층이라 L1이 아니다.
- **L2 판정:** 계획 7절 4번대로 cpppj에 manager·노드 덤프를 요청하지 않고 433개 왕복으로 대신했다. 이 왕복은 읽기→저장이 입력 파일과 같다는 것까지만 보증하며 cpppj 월드 적재와의 비교는 아니다.
- **PORT_MAP:** `CampaignTemplateStorage.cpp` → `CampaignTemplateStorage.ImageFile.cs` 1행을 `port`로 추가했다(기준 커밋 `ce5f58f9`). 369행(L1 195, port 174).
- **실제 호스트:** 저장소를 호스트가 아직 소비하지 않아 호스트 실행은 하지 않았다.
- **남은 것·주의:** TS4(`AllocateEmptyManager`·`ConstructEmptyManager`·`InitializeEmptyManager`·`CloneSectionFrom`·`Dump`·`SaveImage`·`ReadLoginImage`·`BindDiagnosticLog`, `RawMissionLoginPackets`, `fort-image-packets` 송신·예약 103행)부터다. cpppj `DataManagerSelectionTests::EconomyMetaSelectSameManagerAndBorrowedCursors`는 Money/Meta 로더(F2)가 필요해 그때 옮긴다. `EditorFortFileDevices.BindLoad`가 만든 읽기 장치를 `LoadFile`에 연결하는 소비는 TS5다. 같은 작업 트리에서 다른 세션이 cpppj 파일을 수정 중이었고 이 단위는 그 파일들을 쓰지 않았다(HEAD 기준만 사용).

### 0.132 단위 기록 — Template 저장소 TS2: 동적 섹션 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `client/CampaignTemplateStorage`의 동적 섹션 범위를 옮겼다. `Image`를 partial로 나눠 `CampaignTemplateStorage.ImageSections.cs`에 `WithMemory`(임시 할당을 노드 토큰으로 인계하고 end로 크기를 줄여 커서 재공개), `WriteStream`, `CompactTail`, `ListEffects`(노드·버퍼 반납과 마지막 압축), `CreateSection`(같은 이름 제거→새 마지막 노드, 크기 0·용량 16384), `InitializeState`, `AccessSection`(노드 빌림·예외 시 부분 쓰기 보존), `BindRenamedBuffer`(같은 배열을 새 표준 위치/동적 저장소로 인계), `FindSection`, `RenameSection`, `EnsureStateSize`(임시 노드 확장)를 더했다. 저장소 공개 면에는 `InitializeState`·`CreateSection`·`CreateManagerSection`·`FindSection`·`SelectSection`·`RenameSection`·`CurrentWriteStream`·`WriteSection`·`AccessSection`·`EnsureStateSize`를 더했고, `TemplateWriteStream.cs`에 쓰기 참조(`readonly record struct (Manager, Section, Generation)`, 기본값은 없음)와 인라인 쓰기 대리자 `TemplateSectionAccess(Span<byte> fields, MemStreamHooks hooks)`를 두었다. `internal SectionBuffer(TemplateWriteStream)`은 검사가 이름 교체 뒤에도 같은 할당(배열 객체)인지 구별하는 용도로 C++의 `data()` 포인터 비교를 대신한다.
- **C#에서 달라진 모양:** `std::move(vector)`는 `RawSectionBuffer.TakeFrom`(배열을 넘기고 원본은 용량 0)이다. `WriteStream`은 람다가 `Span`을 캡처할 수 없어 입력을 배열로 복사하므로, 입력이 같은 섹션 버퍼의 구간인 호출은 원본의 DWORD 단위 순방향 복사와 다를 수 있다(현재 호출처에 그런 경우가 없다. 소비자를 옮길 때 확인한다). 노드 목록 순서 보조 기록 `StreamOrder`는 원본 충실도를 위해 같은 시점에 갱신하지만 아무도 읽지 않아 관측되지 않는다.
- **검증:** `CampaignTemplateSectionsTests` 18개: cpppj 검사 11개(`CampaignTemplateDynamicTests` 3 중 파일 저장 확인은 TS3라 빼고, `DataManagerCreateTests` 1, `DataManagerResizeTests` 3, `DataManagerStreamsTests` 1, `PlayerStateInitializationTests` 2, `TemplateSerializationGlobalsTests` 1)와 직접 7개(파싱 구간 밖 표준 이름으로 교체, 중첩 빌림·구조 변경 거부, 예외 시 부분 쓰기와 State 커서 공개, 쓰기 참조의 오래된/가짜/기본값 거부, manager 고정 생성, State 확장 중 구조 변경 거부, State 입력 길이·누락 진단). 두 검사 클래스의 공용 도우미는 `TemplateStorageTestSupport`로 모았다. Client 전체 Release 768개와 PORT_MAP 무결성 2개 통과, 실패·건너뜀 0. 변이 33종을 `extracted/dn-mutation-ts0` 사본에서 시도했다. 첫 실행에서 4종(이름 교체 때 seeded 표시, 확장 뒤 노드 토큰, 목록 반납의 커서 해제, 확장 임시 버퍼 크기 조정)을 놓쳐 `RenameBeyondParsedSectionsDoesNotDuplicateStandardNode`와 확장 직후 원본 노드 필드 관측을 더해 앞의 2종을 검출했다. 남은 2종은 등가 변이다: 목록 반납의 `RetireCursor`는 같은 연산이 이어서 같은 이름 노드를 다시 공개하므로 도달 가능한 흐름에서 결과가 같고, 확장 임시 버퍼의 `Resize(end)`는 end가 항상 요청 크기와 같아 무동작이다. 원본 x86 fixture가 없는 어댑터 계층이라 L1이 아니다.
- **PORT_MAP:** `CampaignTemplateStorage.cpp` → `CampaignTemplateStorage.ImageSections.cs`, `CampaignTemplateStorage.h` → `TemplateWriteStream.cs` 2행을 `port`로 추가했다(기준 커밋 `ce5f58f9`). 368행(L1 195, port 173). `cpp_sync.py --ports`는 Template·MemStream·DataManager 계열 cpppj 파일의 변경이 없음을 확인했다.
- **실제 호스트:** 저장소를 호스트가 아직 소비하지 않아 호스트 실행은 하지 않았다.
- **AGENTS.md 변경:** 사용자 커밋 `990a3541`이 4차(MCP)에 "게임 내 모든 조작, 화면 캡처 및 녹화(사운드 포함)"를 명시했다. 0.8의 6번에 반영했다(구현은 R8).
- **남은 것·주의:** TS3(파일 읽기·저장·`BindFileStreams`·`InstallFile`·`LoadFile`·`SaveFile`·`BindCurrentName`)부터이며 433개 왕복 검사가 근거다. `CloneSectionFrom`·`Dump`·`SaveImage`·`AllocateEmptyManager` 계열은 TS4다. cpppj `DynamicTwentyTerritoriesAndCurrentSave`의 저장 확인 부분은 TS3에서 옮긴다. 이 단위에서도 `Image`의 파일 입력 생성자 경로와 `Constructed`는 소비자가 없어 검사하지 않았다. 같은 작업 트리에서 다른 세션이 cpppj 파일(`IndexedBitmapFile` 등)을 수정 중이었고 이 단위는 그 파일들을 쓰지 않았다(HEAD 기준만 사용).

### 0.131 단위 기록 — Template 저장소 TS1: 설치·선택 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `client/CampaignTemplateStorage`의 설치·선택 범위를 `Netstorm.Client/Host/CampaignTemplateStorage.cs`(공개 면)와 `CampaignTemplateStorage.Image.cs`(비공개 중첩 `Image`)로 옮겼다. 저장소: 9칸 헤더 180바이트와 `Install`·`Borrow`·`Clear`·`ClearAll`(원본 `RawTemplateLifetime`에 효과 연결), `Template`·`State`·`SelectState`·`Territory`·`SelectTerritory`·`Money`/`SelectMoney`·`Badges`/`SelectBadges`·`Technology`/`SelectTechnology`·`Name`·`FileHeader`·`SectionOrder`·`CurrentSection`·`ReadSubscriberName`·`GrowState`·`CommitStateGrowth`·`OwnedImages`·`SerializationGlobals`/`SerializationVersion`·`SectionNamesOwner`. `Image`: 276바이트 manager, 60바이트 노드 표(`SortedDictionary<uint, Stream>`, 핸들 오름차순 순회에 의존), `Buffer`(표준 위치는 `FortTemplate.Sections`, 비표준 이름은 노드의 `Extra`), `SyncStreams`(새 표준 위치만 목록 끝에 붙이고 data 토큰=용량 있음, end=크기, alloc=용량을 노드에 동기화), `PublishCursor`/`RetireCursor`/`SyncCursorPosition`, `SelectLoadSection`/`LoadCursor`, `ReadSubscriberName`, `Destroy`. 소멸은 설치된 요새와 State·지형 커서를 비운다(C++에서는 해제된 참조지만 C# 커서는 클래스라 호출자가 들고 있는 참조도 비워 읽기 오류를 막는다). `Core`의 `RawStorageBytes`를 쓰기 위해 `Netstorm.Core/AssemblyInfo.cs`에 `InternalsVisibleTo("Netstorm.Client")`를 더했다(도우미를 다시 구현하지 않기 위한 공개이며 실행 경로의 동작은 바뀌지 않는다).
- **C#에서 달라진 모양:** `Install`은 `FortTemplate`을 참조로 받으므로 소유권을 넘기며, 호출 뒤에도 원본을 쓰는 호출자는 `Clone()`한 사본을 넘긴다. `Headers`는 `Memory<byte>` 속성(`RawPlayerTemplateState`가 요구), `GrowState`는 `Memory<byte>`, `FileHeader`는 `(Signature, FeFlag)` 튜플, `SerializationVersion`은 속성(`uint&` 반환 대신)이다. `.at()`의 `out_of_range`는 `ArgumentOutOfRangeException`, `logic_error`는 `InvalidOperationException`, `invalid_argument`는 `ArgumentException`, `overflow_error`는 `OverflowException`으로 옮겼다. `FortTemplate::Parse`는 F1이라 검사는 `ReadSections`를 쓴다.
- **검증:** `CampaignTemplateStorageTests` 19개: cpppj 검사에서 옮긴 7개(`TemplateLifetimeTests` 2, `DataManagerLifetimeTests` 1, `DataManagerSelectionTests` 3 중 `EconomyMetaActualChildrenAndEmptySelectionModes`는 Money/Meta 로더가 F2라 로더가 진행하는 위치를 직접 써서 줄인 형태, `TemplateSerializationGlobalsTests` 1 중 SoloMapCatalog 앞부분)와 직접 12개(빌림 계약·재설치·이름 검증·섹션 순서·Subscriber 읽기 경계·누락 커서·State 확장 보존·빈 읽기 거부와 쓰기 허용 선택·커서 끝=크기/존재=용량·재빌림·소멸 뒤 커서 비움·이름표 owner). Client 전체 Release 750개, Core 전체 1,117개 통과, 실패·건너뜀 0. 변이 36종을 `extracted/dn-mutation-ts0` 사본에서 시도해 35종을 검출했다(첫 실행 기록: 노드 end를 크기 대신 용량으로 쓰는 변이가 미검출이라 `CursorEndFollowsSizeAndPresenceFollowsCapacity`를 더해 재확인했다). 남은 1종(이름 길이 한도 `>=256`→`>256`)은 하위 `RawTemplateLoad.ConstructManager`가 같은 길이 검사로 같은 예외를 내므로 도달 가능한 입력에서 동작이 같은 등가 변이다. 원본 x86 fixture가 없는 어댑터 계층이라 L1이 아니다.
- **PORT_MAP:** `CampaignTemplateStorage.cpp` → `CampaignTemplateStorage.cs`·`CampaignTemplateStorage.Image.cs`, `CampaignTemplateStorage.h` → `CampaignTemplateStorage.cs` 3행을 `port`로 추가했다(기준 커밋 `ce5f58f9`는 두 파일의 마지막 변경 커밋). 366행(L1 195, port 171).
- **실제 호스트:** 저장소를 호스트가 아직 소비하지 않아 호스트 실행은 하지 않았다.
- **남은 것·주의:** TS2(동적 섹션 `WithMemory`·`WriteStream`·`AccessSection`·`CompactTail`, `CreateSection`·`InitializeState`·`RenameSection`·`EnsureStateSize`와 쓰기 참조)부터다. `Image.RetireCursor`는 TS1에서 호출처가 없어(`ListEffects`가 TS2) 직접 검사가 없으며, `Image`의 파일 입력 생성자 경로(`fileImage: true`)와 `Constructed` 상태도 TS3~TS4에서 쓰이므로 그때 검사한다. cpppj `DataManagerSelectionTests::CampaignTemplates_EconomyMetaSelectSameManagerAndBorrowedCursors`는 `InstallFile`이 필요해 TS3에서 옮긴다. 같은 작업 트리에서 다른 세션이 cpppj 파일(`UrlInput` 등)과 `LEFT_JOBS.md`를 수정 중이었고 이 단위는 그 파일들을 쓰지 않았다(HEAD 기준만 사용).

### 0.130 단위 기록 — Template 저장소 TS0: 용량을 가진 섹션 버퍼 (2026-11-01, HJOW-Athlon)

- **구현:** [Template 저장소 이식 계획](docs/dotnet-template-storage-plan-20261101.md)의 TS0을 마쳤다. `Netstorm.Core/Original/RawSectionBuffer.cs`는 cpppj가 섹션에 쓰는 `std::vector<uint8_t>`의 크기·용량 분리를 옮긴 것이다(`Count`/`Capacity`, `Span`/`Memory`, `Resize`(용량 안은 재할당 없이 늘어난 구간만 0 채움, 넘으면 `max(용량+용량/2, 요청)`), `Reserve`(요청 그대로), `Clear`(크기만), `Release`(`vector{}.swap`), `TakeFrom`(이동 대입, 원본은 용량 0), `FromBytes`/`Clone`(용량=크기), 인덱서는 용량 안이라도 크기 밖이면 거부). `FortTemplate.Sections`를 `List<byte[]>`에서 `List<RawSectionBuffer>`로 바꾸고(`ReadSections`·`Section`·`RawFortImage`) 깊은 복사 `FortTemplate.Clone()`을 더했다(타입 표가 필요한 해석 필드는 F1에서 같은 클래스에 더할 때 함께 복사한다). `FortStreamCursors.cs`에 커서 `FortTerritoryStream`·`FortMoneyCursor`·`FortMetaStream`을 클래스로 옮겼다(저장소가 인스턴스 하나를 평생 소유하고 `Reset`/`Set`으로 필드만 바꾼다. 로더 `RawFortWorldLoad`·`RawFortMoneyLoad`·`RawFortMetaLoad`는 F2). hook 묶음 8개(`DataManagerDumpHooks.Selection`, `DataManagerImageHooks.Streams`, `DataManagerFileHooks.Image`, `DataManagerListHooks.Lifetime`, `DataManagerResizeHooks`의 Selection·List·Memory, `TemplateLoadHooks.Lifetime`)를 `{ get; set; }`으로 열었다.
- **검증:** `RawSectionBufferTests` 13개(용량 규칙·MSVC 성장·감춰진 바이트 비부활·`TakeFrom`·재할당 뒤 옛 `Memory` 위험의 고정·`ReadSections`/`Clone`의 용량)와 `FortStreamCursorTests` 3개를 더했다. 기존 `X86RawTemplateTests`의 `Sections` 사용 9곳은 `RawSectionBuffer.FromBytes`로 바꿨고 기대값은 바꾸지 않았다. Core 전체 Release 1,117개, Client 전체 731개 통과, 실패·건너뜀 0. 변이 24종(Resize 0 채움 제거·성장 2배·요청 무시, Reserve 1.5배·줄이기, Clear/Release 용량 처리, TakeFrom 원본 미비움·자기 이동, FromBytes 여유 용량, Clone 용량 보존·섹션 공유·변환표/뒤꼬리/해시 누락, 인덱서·Span 용량 기준, 재할당 시 감춰진 바이트 복사, 음수 검사 제거, 커서 Reset/Set/기본 Present, ReadSections 여유 용량)을 `extracted/dn-mutation-ts0` 사본에서 시도해 모두 검출했다(중간에 재할당 시 감춰진 바이트 복사를 못 잡아 `Resize_BeyondCapacityDoesNotResurrectShrunkBytes`를 더한 뒤 다시 실행했다. 첫 실행 기록: 이 변이만 직접 검사가 없었다). 저장소의 추적 파일은 변이 중에도 건드리지 않았다. 원본 x86 fixture가 없는 계층이라 L1이 아니다.
- **PORT_MAP:** `RawFortWorldLoad.h`·`RawFortMoneyLoad.h`·`RawFortMetaLoad.h` → `FortStreamCursors.cs` 3행을 `port`로 추가했다(기준 커밋은 각 헤더의 마지막 변경 커밋). `RawSectionBuffer`는 대응하는 cpppj 소스가 없는 표준 라이브러리 대체라 행이 없다. 363행(L1 195, port 168). `Template.cpp/.h`는 변경이 없어 기준 커밋 `d48ea756`을 그대로 둔다.
- **실제 호스트:** 새 계층을 호스트가 소비하지 않아 호스트 실행은 하지 않았다.
- **남은 것:** TS1(설치·선택)부터이며 계획서 4절의 첫 `미착수` 단계를 따른다. 주의: 이 단위의 `FortTemplate`은 기존 호출자(`RawFortImage` 검사)가 쓰는 범위만 `RawSectionBuffer`로 바꿨고, 저장소가 만드는 `Clone()` 소비는 아직 없다. 같은 작업 트리에서 다른 세션이 cpppj 파일(`UrlInput`·`RawMenuControls` 등)과 `LEFT_JOBS.md`를 수정 중이었고 이 단위는 그 파일들을 쓰지 않았다(HEAD 기준만 사용).

### 0.129 단위 기록 — ClientTemplateSectionNames 이식 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `client/ClientTemplateSectionNames`를 `Netstorm.Client/Host/ClientTemplateSectionNames.cs`로 옮겼다. Client 수명의 전역 005c8aa4 이름표 한 개를 파일 reader와 신규 manager 초기화가 함께 빌리는 lazy owner이며, 최초 `Names()`에서만 `RawTemplateSectionNames.Get`(0.128)을 실행해 512바이트를 할당하고 앞 277바이트를 쓴다. 할당이 NULL이면 물리 실패로 남기고 현재 token을 0으로 보존한다. 복사·이동하지 않는다.
- **검증:** `ClientTemplateSectionNamesTests` 2개(lazy 할당·재사용·SNART 제자리 변환·미정 꼬리 235바이트 보존, 할당 NULL 실패)를 cpppj 검사 2개에서 옮겼다. 변이 2종 중 NULL 할당을 성공 token으로 바꾸는 것은 검출되고, 도달 불가한 방어 검사(token 1인데 배열 없음)를 지우는 것은 등가 변이라 검출되지 않는다. Client 전체 Release 731개 통과. `PORT_MAP.tsv` 360행.
- **남은 것:** `CampaignTemplateStorage` 본체와 소비자는 [Template 저장소 이식 계획](docs/dotnet-template-storage-plan-20261101.md)으로 옮겼다. 계획을 세우며 다시 세어 보니 저장소 의존 cpppj 검사는 이 기록 당시 적은 약 30개가 아니라 약 150개(저장소 자체 33, 소비자 장치 26, 적재·저장 약 9, 세션 통합 약 80)이고, `fort-image-packets`의 남은 송신·예약 행은 0.9에 적었던 104개가 아니라 103개(머리글 제외, 송신 93·예약 10)다.

### 0.128 단위 기록 — DataManager·Template 원문 계층 이식 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `o/`의 DataManager·Template 원문 계층을 `Netstorm.Core/Original`로 직역했다. `RawMemStream`(할당·복사·쓰기·압축: DWORD 단위 읽기→쓰기, 진단·free 뒤 필드 재조회, signed 용량 비교), `RawDataManagerLifetime`(MemStream/DataManager 소멸, 효과 뒤 head 재조회), `RawSnart`(SNART 이름표 제자리 변환), `RawDataManagerSelection`(검색·선택, CRT stricmp 경계는 ASCII만 허용하고 비ASCII는 거부), `RawDataManagerList`(추가·제거, stale current 보존, 선행 링크 진단 뒤 재조회), `RawDataManagerResize`(생성·복제·이름 교체·최소 확장, 임시 이름 `T$e$m$p`), `RawDataManagerImage`(mark 진행·섹션 병합·Template 서명·파일 읽기 부모, 반납은 한 번만), `RawDataManagerFileSave`(저장 부모, 이미지 생성→저장 억제·숨김 관문→이름/권한/쓰기→반납), `RawDataManagerInitialize`, `RawDataManagerDump`(`%04d` 음수 서식 포함), `RawTemplateLifetime`(Template 9칸 20바이트 기록의 초기화·반납), `RawTemplateLoad`(생성자·가상 파일명·기존 파일 유지 Load), `RawTemplateSectionNames`(Client 전역 이름표 최초 생성 277바이트), `RawFortImage`(35 섹션 직렬화, 0x8000 고정 할당), `RawFortCurrentName`(현재 Fort 이름 부모와 manager 파일명 setter), `RawPlayerTemplateState`(State 28바이트 getter·Notify·빈 State 생성), `FortSerializationGlobals`, `FortTemplate`(타입 표가 필요 없는 부분만: 35 섹션 이름 순서, 이름별 섹션, 타입 번호 변환, `ReadSections`). 공용 비정렬 DWORD 읽기·쓰기는 `RawStorageBytes`(internal)로 모았고(cpppj는 파일마다 반복), 불투명 핸들→바이트 뷰 조회용 대리자 `RawBytesLookup`·`RawReadOnlyBytesLookup`을 만들었다. `MemStreamAssertion`·`MemStreamHooks`는 `RawFortSpWrite.cs`에서 `RawMemStream.cs`로 옮기고 버퍼 조회를 `byte[]` 대신 `Span` 반환 대리자로 바꿨다(기존 Fort SP writer 검사는 그대로 통과).
- **C#에서 달라진 모양:** `std::span` 반환/인자는 C#에서 `Func`로 쓸 수 없어 전용 대리자(`RawBytesLookup`, `SnartNamesLookup`, `DataManagerFileReader` 등)로 옮겼다. 저장 상태(`DataManagerFileSaveState`)와 현재 Fort 이름 전역의 `dirty`는 C++의 DWORD 참조 대신 읽기/쓰기 대리자로 받아 원본의 읽기 시점(이미지 생성 뒤, 제목 갱신 뒤)을 보존한다. `PlayerTemplateStateStream`은 span을 필드로 둘 수 없어 `Memory<byte>`를 가진 클래스로 옮겼다(참조 의미 유지). 하위 hooks 묶음은 C++의 값 복사 대신 같은 객체를 공유한다.
- **검증:** 원본 x86 fixture 14개를 `PORT_MAP`·`cpp-fixture-manifest.tsv`(SHA-256은 새 항목만 추가, 기존 항목과 `reflected-commit`은 그대로)·`Netstorm.Core.Tests.csproj`에 연결했다: `datamanager-streams` 404행, `-lifetime` 392, `-selection` 864(머리글 제외), `-resize` 286, `-create` 220, `-read` 144, `-file-save` 144, `data-manager-initialize` 64, `template-section-names` 12, `template-lifetime-v2` 240, `fort-current-name` 876관측, `fort-image-packets`의 저장(I) 116행, `player-state-expand` 99, `player-state-initialization` 2,560. 행마다 원본이 관측한 전체 메모리(716·412·256·796·17,380·1,400바이트 등)와 외부 효과 순서를 비교한다. cpppj 순수 계층 검사를 `X86RawDataManagerList/Resize/Image/FileSave`·`X86RawTemplate` 5개 파일 44개로 옮겼고(저장소가 필요한 검사는 제외), fixture가 닿지 않는 경계 3곳(MemStream 쓰기 용량의 signed 비교, 목록에 없는 대상 제거의 사후 진단, Fort 이미지 마지막 길이 필드 경계)은 직접 검사로 더했다. 변이 10종(MemStream 용량 비교를 unsigned로, 목록 제거 사후 진단 제거, 현재 이름의 readonly 0 쓰기 제거, State pos 0 쓰기 제거, Load의 readonly 정규화, Fort 이미지 길이 필드 경계, 저장 길이 보관, SNART FF 표시, mark +2, 크기 확장 후 추가 제거)이 각각 새 검사를 실패시킴을 확인했고(MemStream 용량·목록 제거 사후 진단·Fort 이미지 경계 3종은 처음에 미검출이라 위 직접 검사를 더한 뒤 재확인) 원본 파일을 복원했다.
- **실제 호스트:** 이 단위는 새 계층을 호스트가 아직 소비하지 않으므로 호스트 실행은 하지 않았다.
- **미완료·차이:** 0.9의 R4 Template/DataManager 항목 뒤쪽(저장소·`RawFortWorldLoad` 커서 형식·`FortTemplate.Parse`·미션 로그인 패킷)이 남았다. cpppj 검사 중 `CampaignTemplate*`·`FortNotify`·`FortCurrentNameDevices`와 `fort-image-packets`의 송신(S)·예약(F) 행은 위 선행 조건 때문에 옮기지 않았다. `RawDataManagerDump`의 trace 출력, `RawTemplateLoad`/`RawFortImage` 직접 검사는 원본 x86 fixture가 없는 직역 검사라 L1이 아니다(`PORT_MAP`에서 `RawDataManagerDump`·`RawTemplateLoad`·`FortSerializationGlobals`·`Template`은 `port`). `FortTemplate`은 `Template.cpp`의 일부만 옮겼으므로 `PORT_MAP`의 `Template.cpp/.h` 행은 부분 이식이다. 기존 `Netstorm.Assets.FortFile`은 같은 형식의 이전 독립 판독기로 남겨 두었고 연결하지 않았다(은퇴는 R4 Fort 적재 단위). 같은 작업 트리에서 다른 세션이 cpppj 파일(`AddRandom` 등)을 수정·커밋 중이었고 이 단위는 그 파일들을 쓰지 않았다(HEAD 기준만 사용).

### 0.127 단위 기록 — 편집 파일 장치 잔여 이식 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `EditorFiles`의 남은 부분을 `Netstorm.Client/Host/EditorFiles.cs`에 옮겼다. `Exists`(열리는 일반 파일만 5, 디렉터리·부재 0), `ReadOnly`(readonly 속성만 읽고 Config와 독립, 빈 이름 진단 0x480·stat 실패 진단 0x48e 뒤 명시 오류), `MakeWritable`(모드 0x180만 받아 readonly 속성만 풀고 실패는 무시), `Writer`/`WriteTemplate`(원문 0041bb90: 기존 파일은 자르지 않는 `r+b`로 먼저 열고 실패할 때만 `w+b`로 만들어 기존 파일의 뒤쪽을 보존하며 부모 폴더를 만들지 않음), `SiblingNamespace`(작업본·원본을 모두 보호하는 `-state` 형제 장치, 이름 범위·외부 링크 경계 거부), `WriteAtomic`(임시 파일 flush 뒤 Windows `MoveFileExW` write-through 교체, 접근 거부·공유/잠금 위반·매핑 거부만 20ms 간격 최대 51회 재시도)이다. `EditorCatalogLedger`는 이 `WriteAtomic`을 거쳐 장부를 갱신한다(0.126의 단발 `File.Move` 차이 해소). `EditorFortFileDevices.cs`는 cpppj `EditorFortFileDevices`의 `CurrentPath`(현재 이름 진단 39번·같은 FileSpec 선택·`PathSpec`·작업 namespace 해석), `PrepareCurrent`(부모 폴더만 만들고 없는 작업본만 원본에서 복사, 준비/해석 효과 주입 가능), `Exists`/`ReadOnly`/`Read`(원본으로 fallback 안 함), `BindLoad`(읽기·권한·ASCII stricmp 이름 비교)이다. `TemplateFileHooks.cs`에 cpppj `CampaignTemplateStorage.h`의 `TemplateFileWriteHooks`·`TemplateFileLoadHooks` 두 형식만 옮겼다.
- **검증:** `EditorFilesDeviceTests` 19개를 추가했다. cpppj 검사 중 Template 저장소가 필요 없는 14개를 한글·이모지 경로의 임시 작업본에서 직역하고(`EditorFileWriterTests` 3·`EditorFilesTests` 3(보호 경로 1, `EditorWorkingRoot` 표기 정규화 2를 `EditorWorkspacePaths`로)·`EditorAtomicWriteTests` 2·`EditorFortFileDevicesTests` 7, 저장소가 필요한 부분은 뺌) 직접 검사 5개를 더했다(존재·readonly·권한, readonly 진단, 형제 metadata, 장부 재시도 연결, 준비·해석 효과). 저장소가 필요한 cpppj 검사 8개(Writer 1·Files 5·FortFileDevices 2)와 `Prepared`·`ReadOnly` 검사 안의 Load·Save 부분은 Template 이식 뒤 옮긴다. 기존 장부·작업본 10개와 합쳐 집중 29개, Client 전체 Release 729개(기존 710 + 19) 통과, 실패·건너뜀 0. 변이 3종(Template 쓰기를 항상 잘라쓰기로, 원자 교체 재시도 제거, readonly 속성 무시)이 각각 새 검사를 실패시킴을 확인하고 복원했다. 원본 x86 fixture가 없는 직역 검사라 L1이 아니다. `PORT_MAP.tsv` 323행(`EditorFortFileDevices`·`CampaignTemplateStorage.h`·`EditorWorkspacePaths` 대응 4행 추가, `EditorFiles` 기준 커밋 `473432cd`로 재대조).
- **실제 호스트:** 격리 데이터 복사본(`extracted/dotnet-editor-127/`, `NETSTORM_DATA`로 지정)에서 새 호스트로 Edit를 열어 1024×768 목록 캡처를 확인했다(작업본 fort 433개, 장부 9,546바이트로 0.126과 같음, 원본 폴더 fort 433개 그대로, 종료 0). 이번에는 실제 호스트가 새 파일 장치를 직접 소비하지 않으며, 장부 갱신 경로만 새 `WriteAtomic`을 거친다. cpppj와의 장부 상호 읽기·화면 비교는 하지 않았다. 금지된 플레이 재현은 하지 않았다.
- **미완료·차이:** Template 저장소 계층 전체(0.9의 R4 항목)와 `ExistingEditorTemplateValid`, `EditorFortFileDevices.BindLoad`의 실제 소비자(Load/Save), Native 이름 변경, 실제 Editor State1 월드·저장·Save as의 Template/현재 이름 소유자는 미이식이다. cpppj의 `RelativePath`는 옮기지 않았고 상대 키는 `EditorWorkspacePaths.Resolve`가 계산한다. Windows 외의 `WriteAtomic`은 write-through와 재시도가 없는 준비용 `File.Move`이며 durable 검증으로 주장하지 않는다. `ReadOnly`의 Windows 외 동작(쓰기 비트)은 관측하지 않았다. 새 파일 장치의 이름 정규화는 cpppj처럼 ASCII만 소문자로 접으며, 기존 `GameFileSystem.NormalizePath`(비ASCII도 접음)와는 차이가 있다(이번에는 고치지 않음). 같은 작업 트리에서 다른 세션이 cpppj 파일(`UberEditor.inc` 등)을 수정 중이었고 이 단위는 그 파일들을 쓰지 않았다(HEAD 기준만 사용).

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
| R0-4 (10-28, 0.14~0.21) | SP 가산·초기화, Fort SP writer, 타입 변경·비석/주문 전환, 풀 정리·SID 목록, 사제 panic·기도·회복, 그래프 비표면 좌표, 자산 후처리 4종 | `docs/dotnet-r04-*.md` | 유지. R4 세션 조립 |
| R1-2 (10-28, 0.13·0.22) | 캡처·스크립트 도구, `cpp_frame_compare.py`, MonoGame 출력 호스트 | [문서 5·6절](docs/dotnet-client-screen-20261028.md) | 새 경로 |
| R2 (10-28~29, 0.23~0.28) | 팔레트, VFX 원시, 색 표, 변환·합성·이미지·시작, 판본·표시자료·캡처, 일반 객체 그리기 | [SpriteDraw](docs/dotnet-spritedraw-20261029.md) | 새 경로. 세션 연결은 R4·R5 |
| R3a~R3g (10-29, 0.29~0.37) | Gump 기반·트리·수명·사건/입력·프레임·클립·배경·글꼴·버튼 | `docs/dotnet-gump-*.md` | 새 경로 |
| R3h (10-29, 0.38~0.48) | StyleText, Slider·다중 줄 Widget, 메뉴 기본·제어·장식·생성·출력·입력 | `docs/dotnet-style*.md`, `docs/dotnet-slider-widgets-20261029.md`, `docs/dotnet-scrolling-widget-20261029.md`, `docs/dotnet-menu-*.md`, `docs/dotnet-decoration-20261029.md` | 새 경로 |
| R3i~R3j (10-29, 0.49~0.53) | 문자 입력창·Console, ImageGump, MetaImage | `docs/dotnet-text-entry-*.md`, `docs/dotnet-image-gump-20261029.md`, `docs/dotnet-meta-image-*.md` | 새 경로 |
| R3k (10-29~31, 0.54~0.65) | 배경 GIF, HTML 본문·링크·부모·창·메타, 도움말, 스크립트 명령, Config 객체, ConfigInterface, MissionScript | `docs/dotnet-background-gif-gump-20261029.md`, `docs/dotnet-html-*.md`, `docs/dotnet-quick-help-*.md`, `docs/dotnet-script-command-20261030.md`, `docs/dotnet-config-*.md`, `docs/dotnet-mission-script-20261031.md` | 새 경로 |
| 미션 조립 부품 (10-31, 0.66~0.73) | Fort reader, Guide 교체·수명, 미션 제어 입력, 공통 Start, AI 설정·생성, Guide 단계·Lifecycle | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126) 0.66~0.73 | L1. 실제 세션 연결은 R4 |
| 호스트 입력·타이틀·메인 메뉴 (11-01, 0.74~0.88) | Player 참여 현재 조회, 미션 Fort 파일 지연 선택, Tutorial 미션의 Player 반납 경계, Guide 미션 소유·제어 명령, 호스트 입력 큐와 Guide 우선 분배, 실제 타이틀 자산 출력, 초기 Kencloud 구름 표시, 호스트 시작 Config(`--set`) 전달, 공통 돌 버튼 트리, 메인 메뉴 버튼 표시·입력·명령 분배·라벨, 메뉴 시계와 버튼 음향 경계, 호스트 포인터 폴링, 실제 Config Tell 인자와 섹션 선택 | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126) 0.74~0.88 | 새 경로(L1 일부). 미션·월드 조립은 R4 |
| 공통 트리·HTML 창 (11-01, 0.89~0.99) | 버튼 트리 모달, HTML 파생 노드의 공통 트리 연결, fortGump 장식 생성·출력, 호스트 HTML 부모·버튼 소유자, 본문·슬라이더 조립, 실제 글리프 출력, 창 배치, 파서·명령 예약, 메뉴 열 조립, 전체 창 생성기 | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126) 0.89~0.99 | 새 경로 |
| 기존 경로 이름·배포 (11-01, 0.96) | `Netstorm.Game` 어셈블리를 `Netstorm`으로 바꾸고 배포 스크립트·README·클론 스모크 도구 16개의 실행 경로 갱신. Release 전체 2,074개 통과, ZIP 1,376항목을 압축 해제해 `Netstorm.exe` 시작·종료 코드 0 확인 | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126) 0.96 | 기존 경로 |
| 통합 표시·메뉴 입력 (11-01, 0.100~0.104) | 원본 표시 목록 CRT 정렬(원본 1,155행 대조), 실제 공통 트리 수집·정렬·출력, 메인 입력 큐·다음 Tick Tell·HTML 닫기·Config·메인 종료 명령, 메뉴 행 입력·포인터 통지·원본 커서·이름 소리, Tell @/#/% 파일 목록과 캠페인 선택 | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126) 0.100~0.104 | 새 경로(정렬은 L1) |
| Tutorial Init·초기화 조립 (11-01, 0.105~0.110) | Tutorial Init(원본 152행), 미션 전투 옵션 초기화(전체 옵션 바이트 fixture 대조), 요새 요청의 상태144바이트·Zone 복제, Guide와 Init 상태 공유, 기술 허용 목록·공유 제한 표 쓰기, 초기화 순서 조립 | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126) 0.105~0.110 | L1. 실제 소유자 조립은 R4 |
| 호스트 Help·Options·옵션 저장 (11-01, 0.111~0.114) | Help 팝업의 About 행과 Tell, Options·해상도·음량 팝업과 설정 토글, 화면 모드 절대 변경·미리보기·확인 명령, 변경된 옵션의 options.cfg 저장과 재시작 화면 크기 복원 | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126) 0.111~0.114 | 새 경로. 9-1 C의 일부 |
| Edit 맵 목록·이름·작업본 (11-01, 0.115~0.127) | 편집 맵 목록 50칸 캐시와 분류·행 표시, Tell 맵 열 분할·파일 키 복구, AskEditFort/ReadFort·편집 복귀 State 예약, 기본 디스크 열거와 Fort 섹션 기반 맵 메타, 호스트 Edit 입력·맵 목록, 새 Fort 이름 확인·호스트 문자 입력·이름/숫자 모달, 편집 이관 장부·작업본 장치, 편집 파일 장치(존재·readonly·권한·Template 쓰기·원자 쓰기) | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126) 0.115~0.126, 이 문서 0.127 | 새 경로 |
| DataManager·Template 저장소 (11-01~02, 0.128~0.136) | 원문 계층(MemStream·DataManager·Template·Fort 이미지·State, 원본 x86 fixture 14개), ClientTemplateSectionNames, 저장소 TS0~TS5(용량을 가진 섹션 버퍼·설치/선택·동적 섹션·파일 읽기/저장·빈 manager·로그인 송신 이미지·Editor 파일 계층, 원본 .fort 433개 읽기→저장 왕복 일치, 송신 패킷 `fort-image-packets` 219행 전체 일치, 한글 작업 경로의 UTF-8 이름 보관, 새 맵 manager·신규 자료·파일 부재 부모 `RawNewFort` 원본 x86 1,400행 일치, 현재 Fort 이름 장치, 실제 저장소를 쓰는 편집 맵 목록) | 이 문서 0.127~0.136, [이식 계획](docs/dotnet-template-storage-plan-20261101.md) | 새 경로. F1 이후 미착수 |

최신 전체 빌드·검사 결과는 0.1절을 따른다. Assets·Core의 기존 경로 검사 수를 새 경로의 진행도로 세지 않는다. 새 경로의 진행도는 Client 검사와 0.5절의 실제 단계 완료 기준을 함께 본다.

---

<a id="original-parity-plan"></a>

## 2. 9-1 결정 — 의도적 차이는 원본처럼 동작하도록 바꾼다 (2026-10-10 사용자 결정)

> **2026-10-28 정리:** 옛 9절 표(dotnetpj가 일부러 다르게 둔 것)는 이관 파일로 옮겼다. 아래 결정문의 "위 표"는 그 표를 말하며, 같은 항목은 아래 A~F 표에 모두 있다. 아래 "이 결정으로 바꾸지 않는 것"의 MonoGame·슬롯 문단은 0절 결정 1·2로 갱신됐다. MonoGame은 창·입력·오디오 장치·최종 출력 계층으로 유지하고 게임 화면은 8비트 논리 화면에 그린다. cpppj가 원본 배치를 쓰는 곳은 C#도 같은 배치를 쓴다. A~F의 남은 처리 시점은 [0.7](#07-계속-유효한-결정제약)을 따르며, 옛 순서 제안(A → C → B → D·E·F)은 이관했다.

**결정:** 위 표의 차이는 원본처럼 동작하도록 바꾼다. **지금 구조를 둔 채 원본 게임의 경험을 거의 동일하게 만들 수 있으면 구조는 유지해도 되지만, 그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다.** 위 표의 "이유" 칸(요구사항·편의)은 더 이상 차이를 그대로 두는 근거가 아니다. 이 작업은 목표 순서의 ①(원본과 완전히 동일한 동작, [0.7](#07-계속-유효한-결정제약))에 속한다.

**항목마다 밟는 순서:** ① 그 차이 때문에 원본과 달라지는 동작을 구체적으로 적는다 → ② 원본 근거(cpppj 의 x86 기대값·`--dump-*` 출력·분석 문서·녹화)와 같은 입력으로 비교한다 → ③ 차이가 없거나 플레이에서 구별할 수 없으면 구조를 유지하고 그 근거를 이 문서에 남긴다 → ④ 구별되면 먼저 지금 구조 안에서 고치고, 그것으로 안 되면 구조를 원본 방식으로 바꾼다. **비교 근거 없이 "같다"고 적지 않는다.**

**2026-10-11 A 판정·첫 단계(`HJOW-Athlon`):** 24Hz 구조를 그대로 유지해도 같다는 판단은 폐기했다. 명령의 틱 대기·정지 중 1틱 진행·붕괴 스캔 33개/틱은 구별되는 차이이며 프레임 처리로 바꿨다. 명령과 다리 스캔은 같은 화면 갱신에서 처리하고 정지 입력은 시각을 늘리지 않는다. 실제 세션에 원본 10.78 Scan **1,948행**·Init **4행**을 연결해 시각·커서·다음 시각 double 비트가 일치했다. [범위·검증·제한](docs/dotnet-frame-parity-20261011.md).

- [x] **A 완료분:** 명령·프레임 붕괴 스캔·정지 시각 보존, 화면 루프, 숫자 시계, 세션 시계 연결 구현은 끝났다. 근거는 1절의 2026-10-11~10-13 행 문서다.
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
12. **Template 저장소 이식 방식 — 해결(2026-11-01 사용자 결정): 계획서의 기본값 네 가지를 그대로 진행한다.** (1) 크기와 용량을 따로 갖는 `RawSectionBuffer`를 만들고 `FortTemplate.Sections`를 그 형으로 바꾼다. (2) 타입 표는 `RiftTypeTable`을 직역하고 `TypeCatalog` 어댑터로 `Parse`를 먼저 완성하지 않는다. (3) `FortTemplate`의 위치는 F1에서 `RiftTypeTable` 위치와 함께 확정하고 그때까지 `Netstorm.Core/Original`에 둔다. (4) 저장소의 cpppj 대조는 요청하지 않고 TS3의 fort 433개 왕복 검사로 대신한다. 근거와 단계는 [Template 저장소 이식 계획](docs/dotnet-template-storage-plan-20261101.md) 7절이다.

---

## 4. 검증 명령과 기록 규칙 (옛 12절)

```powershell
# 저장소 루트에서 기준선과 단계별 검증 (헤드리스)
dotnet build dotnetpj/Netstorm.sln -c Release
dotnet test dotnetpj/Netstorm.sln -c Release

# cpppj 콘솔 출력 대조 (헤드리스 명령만 사용. --run 등 게임/창 실행은 해당 PC의 규칙을 별도로 확인)
cpppj/build/bin/Release/Netstorm.exe --dump-types originals
cpppj/build/bin/Release/Netstorm.exe --dump-world originals thewarbegins
cpppj/build/bin/Release/Netstorm.exe --inspect-bridges originals

# cpppj 반영 범위 확인 (W01, 2026-10-16). 저장소 루트에서 실행. 마지막 반영 커밋은 cpp-fixture-manifest.tsv 의 reflected-commit
python dotnetpj/tools/cpp_sync.py                                    # 보고 (읽기 전용, 연결 fixture 가 어긋나면 종료 코드 1)
python dotnetpj/tools/cpp_sync.py --update --reflect HEAD            # HEAD까지 영향 검토·반영을 모두 마친 경우에만 갱신
python dotnetpj/tools/cpp_sync.py --update --add 이름:Core           # 새 fixture 를 연결할 때 목록에 등록 (Assets 도 가능)
```

```powershell
# cpppj 오라클 캡처 (2026-10-28 추가, 0절 결정 4). 창이 뜨므로 AGENTS.md 확인·예외 규칙을 따른다.
# 데이터는 originals/ 대신 extracted/ 아래 사본을 쓴다. steps.tsv 한 줄은 "프레임<TAB>동작<TAB>인자"다
# (click·right·key·esc·pointer·snapshot 등. snapshot 인자는 저장할 BMP 경로).
cpppj/build/bin/Release/Netstorm.exe --run <데이터 사본> --window --no-audio --frames 120 `
  --set "SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0" `
  --ui-script steps.tsv --ui-report states.tsv            # 미션 직행은 --mission thewarbegins 등을 더한다
# 캡처 비교 도구: python dotnetpj/tools/cpp_frame_compare.py A.bmp B.bmp [--diff diff.png]  /  --dir 폴더A 폴더B (종료 코드 0 일치·1 차이·2 입력 오류)
# dotnetpj 쪽의 같은 실행 인자(--frames/--ui-script/--ui-report/--set)는 Netstorm.Client.Host 가 받는다.
# PowerShell 5.1의 Start-Process -PassThru 는 종료 코드를 비우므로 `& exe 인자; $LASTEXITCODE` 로 읽는다.
```

- 클론 창을 띄우는 검사(`tools/clone_*_smoke.ps1`, `dotnet run --project dotnetpj/src/Netstorm.Game`)는 AGENTS.md의 확인·예외 규칙을 따른다.
- 원본 x86 기대값 대조만 돌릴 때: `dotnet test -c Release --filter "FullyQualifiedName~X86"`. 새 기대값 파일은 `tests/Netstorm.Core.Tests/Netstorm.Core.Tests.csproj`에 링크로 추가하고(사본 금지) 위 `cpp_sync.py --update --add`로 SHA-256 목록에 등록한다(빠뜨리면 `EveryLinkedFixtureIsRegistered`가 실패).
- 새 코드의 **상수·함수·구조체·클래스·멤버함수·멤버변수·반복문**에 한국어 주석을 단다(AGENTS.md 2026-10-13 확장). **주 내용은 설명과 사용법**이고, 언제·왜 추가/수정했는지는 그 뒤에 적는다. C# 에서는 `/// <summary>` 에 무엇을 하는지와 쓰는 방법(인자·반환·호출 조건)을 쓰고, 원본 주소·근거 문서와 추가/수정 이력은 summary 의 뒤쪽이나 `<remarks>` 에 둔다. 반복문은 바로 위 `//` 주석으로 무엇을 도는지 적는다. 기존 주석은 그 주석이 설명하는 대상을 수정할 때 이 규칙에 맞춘다(2026-10-10 사용자 결정). 새 파일은 UTF-8 로 쓴다.
- 원본 주소를 주석에 적을 때는 10.78 주소를 쓰고, 근거 문서(`docs/exe/cpp-*.md`)를 함께 적는다.
- 작업이 끝난 항목은 이 문서에서 체크하거나 지운다. 중단하면 변경 파일·검사 결과·다음 항목을 0절의 해당 체크리스트와 단위 기록에 남긴다. 끝난 단위의 상세 기록이 길어지면 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)로 옮기고 1절 색인만 갱신한다(2026-10-28).
- AGENTS.md 와 원본 디렉터리(`originals/`, `originalCD/` 등)는 수정하지 않는다.
- **Git:** `dotnetpj`·`docs`와 `LEFT_JOBS.dotnetpj*.md`는 저장소에 LF로 저장된다. 기본 `autocrlf`로 add하고 `-c core.autocrlf=false`를 쓰지 않는다. 커밋 번호는 다른 PC·세션과 겹칠 수 있으므로 `git log`로 당일 마지막 번호를 확인해 이어 쓴다.
- **빌드 환경:** Git Bash에서 `cmake --build … -- /p:…`를 실행하면 `/p:`가 경로로 바뀌어 MSBuild가 실패하므로 PowerShell로 실행한다. `cpp_sync.py` 보고가 cp949 콘솔에서 깨지면 `PYTHONUTF8=1`을 설정하거나 `python -X utf8`로 실행한다. 오라클에는 게임 실행 파일만 필요하며, cpppj 전체 빌드는 검사 대상까지 포함해 오래 걸린다.
- **오라클 자료:** 데이터 사본 `extracted/dotnet-oracle-data/`, 캡처 `extracted/dotnet-oracle-captures/`, 기준선 `extracted/dotnet-r0-baseline/`은 Git 제외이며 작업 PC에만 있다. 실행 전후 `originals/` 해시를 비교한다.
- **fixture·포트 맵:** 한 fixture는 한 소비자 프로젝트에서 검사한다(`config-x86.tsv`는 Assets.Tests). 포트 맵의 `port` 행은 fixture 대조가 없는 이식이며 L1이 아니다.

---

## 5. 참고 문서 (옛 13절)

| 주제 | 문서 | cpppj 기준 구현 |
|---|---|---|
| **현행 계획·재수정·다음 작업(2026-10-28)** | [이 문서 0절](#dotnet-current-plan-20261028) | cpppj `src/o`·`src/client` 전체, [cpppj 소스 맵](cpppj/SOURCE_MAP.md) |
| 8비트 화면 출력·전체화면 호환 | [cpp-modern-fullscreen-20261027.md](docs/exe/cpp-modern-fullscreen-20261027.md), [cpp-screen-resolution-menu-20261027.md](docs/exe/cpp-screen-resolution-menu-20261027.md) | `client/Screen.cpp`, `client/ScreenPresentation.cpp`, `client/ClientMain.cpp` |
| 이관 기록(2026-10-22 이전 계획·날짜별 완료 절·옛 0~11·14절·단위 기록 0.10~0.65) | [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md) | — |
| **cpppj → dotnetpj 이식 계획(W01~W20 상세, 2026-10-16)** | [dotnet-cpp-port-plan-20261016.md](docs/dotnet-cpp-port-plan-20261016.md) | `client/RawCampaignSession`, `client/RawSurfaceWorld`의 `RawWorldCore` |
| **원본 월드 코어 W02 첫 조각(슬롯 풀·생성자·Create/Take, 2026-10-16)** | [dotnet-original-core-20261016.md](docs/dotnet-original-core-20261016.md) | `o/SidPool.cpp`, `o/SquidFactory.cpp`, `o/TypeConstructors.inc`, `o/DerivedConstructors.inc` |
| **Template 저장소 이식 계획(TS0~TS6, F1~F3, 2026-11-01)** | [dotnet-template-storage-plan-20261101.md](docs/dotnet-template-storage-plan-20261101.md) | `client/CampaignTemplateStorage` |
| **cpppj의 11.01 표기·Credits / 외부 접속 목록(2026-11-01)** | [cpp-product-1101-20261101.md](docs/exe/cpp-product-1101-20261101.md), [cpp-external-services-20261101.md](docs/exe/cpp-external-services-20261101.md) | 단위685·684 |
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

**2026-11-02 이관(HJOW-Athlon, 문서만, 사용자 요청):** 정리 직전(`3b2b9e58 / 20261102-97 DN`) 전체 원문을 [이관 파일 끝](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)에 그대로 보관했다. 본문에서 뺀 것은 단위 기록 0.66~0.126(0.127 이후는 TS 단계의 직접 근거라 남김), 0.1의 단위별 검사 수 나열, 0.9의 단위별 완료 서술과 완료 체크 항목이다. 0.126 이하에서 이후 단위로 해소되지 않은 한계는 0.9의 '이월한 이식 한계' 항목으로 옮겼다(0.67의 두 항목은 0.68에서 해소돼 제외했다). AGENTS.md 최근 변경(4차 로컬 MCP·와이드 해상도, 7차 2560×1440·3840×2160 FSR, 11.01, 외부 접속 문서화)은 0.7·0.8·0.9에 반영했다. 빌드·검사·게임 실행·디컴파일은 하지 않았다. `LEFT_JOBS.md`와 이 문서를 함께 고친 것은 `LEFT_JOBS.md`를 수정하지 않는다는 0.7 원칙에 대한 사용자 지시 예외다.

**2026-10-31 이관(HJOW-X3D, 문서만):** 정리 직전 전체 원문을 [이관 파일 끝](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073)에 그대로 보관했다. 본문에서 뺀 것은 머리 안내 3개, 2026-10-28 당시 상태표와 진단 상세, 완료한 R0~R2 상세, 0.9의 완료 항목, 0.10, 단위 기록 0.11~0.65, 2절의 완료 줄이다. 빌드·검사·게임 실행·디컴파일은 하지 않았다.

**2026-10-28 이관(VM-W11-Codex):** 0절 재수립으로 더 이상 현행 의미가 없어진 내용을 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)로 옮겼다. 내용은 고치지 않고 그대로 옮겼으며, 그 파일 안의 "현재·다음·미구현" 표현은 당시 상태다.

- **옮긴 것:** 2026-10-22 이전 계획, 2026-10-09~10-23 날짜별 완료·인계 절, 구 머리말과 0~8절(2026-10-07~10 당시 1~6단계·8절 이식/대기 구분), 옛 9절 표와 9-1의 옛 순서 제안, 옛 10절 1·3·4번(해결·구현이 끝난 확인 항목), 11절(과거 미확인 항목 재분류), 14절(LEFT_JOBS.md에서 이관한 2026-09-27~10-05 이력).
- **본문에 남긴 것:** 0절 현행 계획, 1절 완료 범위 색인, 2절 9-1 결정, 3절 옛 10절의 유효한 결정, 4절 검증 명령(옛 12절), 5절 참고 문서(옛 13절).
- **앵커:** 옮긴 절의 `<a id>`(`dotnet-current-plan-20261022`·`sid-session-plan`·`legacy-dotnet-history`·`legacy-dotnet-031`~`107` 등)는 이관 파일에 같은 이름으로 남겼다. LEFT_JOBS.old.md·LEFT_JOBS.webpj.md·`docs/`의 기존 링크가 이 안내로 오도록 같은 이름의 빈 앵커를 바로 아래에 두었다. 그런 링크로 들어왔다면 이관 파일의 같은 앵커를 본다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
