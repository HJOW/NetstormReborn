# LEFT_JOBS.dotnetpj — dotnetpj(C# + MonoGame) 작업 계획 및 인수인계

이 문서는 dotnetpj의 **현재 상태, 계획, 남은 작업, 유효한 사용자 결정**을 담는다. cpppj와 공통 사항은 [LEFT_JOBS.md](LEFT_JOBS.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md), 과거 기록은 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)에 있다.

[0절 현행 계획](#dotnet-current-plan-20261028)이 이 문서와 [이식 계획 문서](docs/dotnet-cpp-port-plan-20261016.md)의 순서·구조 결정보다 우선한다. 1절은 완료 범위 색인, 2·3절은 계속 유효한 사용자 결정, 4·5절은 검증 명령과 참고 문서다.

단위를 마치면 0.1절의 값과 0.9절의 체크 항목을 갱신한다. 상세는 `docs/dotnet-*.md` 단위 문서에 남기고, 문서가 없는 작은 단위만 0.66 이후의 단위 기록에 짧게 적는다.

<a id="dotnet-current-plan-20261028"></a>

## 0. 2026-10-28 현행 계획 — cpppj 그래픽 복원분을 기준으로 dotnetpj 재구성

### 0.1 현재 값 (2026-11-01, 단위 기록 0.127 기준)

| 항목 | 값 |
| --- | --- |
| 진행 상태 | **작업 진행 중.** 전체 목표는 미완료다. |
| 현재 단계 | R0·R1 완료, R2 본체 완료, R3 진행 중, R4 일부 이식. R5 이후는 미착수 |
| 마지막 구현 커밋 | 0.127 편집 파일 장치 잔여 이식(이 문서를 포함한 커밋); 이전 구현 `76a8a25a` (0.126) |
| 검사 | 0.74 전체 Release 2,043개(Assets 364, Core 1,052, Client 627) 통과, 실패·건너뜀 0. 0.75는 MissionScript 집중 4개, 0.76은 Tutorial 반납 집중 3개, 0.77은 Guide 관련 집중 13개, 0.78은 호스트·Guide 집중 15개, 0.79는 호스트 집중 14개, 0.80은 시작 화면·호스트 집중 12개, 0.81은 시작 설정·호스트 집중 13개, 0.82는 돌 버튼 트리 집중 12개, 0.83은 실제 메뉴 자산·버튼 집중 15개, 0.84는 메뉴 입력 집중 16개, 0.85는 호스트·스크립트 집중 28개, 0.86은 호스트 Release 빌드와 메뉴·버튼 집중 2개, 0.87은 호스트·스크립트·시작 화면 집중 31개, 0.88은 Tell·실제 시작 화면·fixture 목록 집중 14개, 0.89는 공통 버튼·입력·HTML 집중 61개, 0.90은 공통 버튼·HTML 공장·시작 화면 집중 27개, 0.91은 시작 화면·공통 트리·장식 집중 20개, 0.92는 시작 화면·공통 트리·HTML 공장 집중 29개와 호스트 Release 빌드, 0.93은 시작 화면·공통 트리·다중 줄 집중 53개, 0.94는 시작 화면·다중 줄·글리프 집중 49개, 0.95는 시작 화면·HTML 배치·공장 집중 30개가 통과했다. 0.96 Release 전체 2,074개(Assets 364, Core 1,055, Client 655) 통과, 실패·건너뜀 0. 0.97은 호스트·HTML·Guide 집중 29개, 0.98은 호스트·HTML 집중 39개, 0.99는 호스트·전체 창·Tell 집중 34개, 0.100은 정렬 및 fixture 등록 집중 5개, 0.101은 호스트·정렬·Gump Draw 집중 47개, 0.102는 호스트·입력 집중 31개, 0.103은 호스트·메뉴 입력·스크립트 집중 62개, 0.104는 Client 집중 38개와 Assets 파일 시스템 집중 2개, 0.105는 Tutorial Init 집중 1개(원본 152행), 0.106은 옵션·Tutorial Init 집중 3개, 0.107은 요새 요청 집중 2개, 0.108은 Guide·Tutorial Init 집중 4개, 0.109는 기술 목록 집중 2개, 0.110은 초기화 조립·Guide·요새·기술 집중 9개, 0.111은 HostStartupScene 집중 18개, 0.112는 같은 집중 18개와 확장된 메뉴 검사 1개, 0.113은 화면 모드·메뉴 집중 1개, 0.114는 옵션 저장 집중 3개와 Core Config 집중 10개, 0.115는 맵 목록 집중 2개, 0.116은 Tell 맵 목록 집중 3개, 0.117은 편집 Fort 선택 집중 2개, 0.118은 맵 목록 집중 2개와 파일 시스템 집중 1개, 0.119는 Edit·Campaign·Help 집중 3개, 0.120은 Fort 이름 확인 집중 3개, 0.121은 호스트 문자 입력·Editor 집중 8개, 0.122는 이름·숫자 모달과 이름 확인·문자 입력 집중 6개, 0.123은 실제 트리 모달·Edit·이름 입력 집중 8개, 0.124는 이름 확인·경로·모달 집중 6개, 0.125는 장부·경로·이름 확인 집중 3개가 통과했다. 0.126은 Client 전체 Release 710개와 Assets 파일 시스템 집중 3개가 통과했다. 0.127은 편집 파일 장치·장부 집중 29개(새 19개)와 Client 전체 Release 729개가 통과했다. Assets·Core의 기존 경로 검사 수를 새 경로의 진행도로 세지 않는다. |
| 포트 맵 | `PORT_MAP.tsv` 323행 (L1 167, port 156) |
| fixture | 연결 146개. 반영 커밋(`reflected-commit`)은 영향 검토 전이라 `f95a8db2`에 그대로 둔다. |
| 화면 비교 | 실제 메뉴·게임 캡처 비교는 아직 0건이다. 오라클 메인 메뉴 캡처의 결정성만 확인했다([문서 5절](docs/dotnet-client-screen-20261028.md)). |
| 기본 실행 | 기존 경로 `Netstorm.Game`(`Netstorm.exe`)의 1-1·1-2 근사 플레이다. 기존 코드·검사 은퇴는 0건이다. |
| cpppj 기준 | 단위637 제품, 커밋 `a421aa6c` ([LEFT_JOBS.md](LEFT_JOBS.md) 0.1절). 소스는 HEAD만 쓴다. |
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
- AGENTS.md 4~7차: 4차 MCP(해석은 0.8 6번), 5차 설정·캠페인 진행 저장을 `%USERPROFILE%\Saved Games\Netstorm\Saves\`로 옮기고 커스텀 맵을 기존 폴더와 `...\CustomMaps\` 양쪽에서 불러오며 Edit 신규 저장은 후자로 하고 Edit 맵 목록에 스크롤과 개수 무제한을 넣는다, 6차 한국어(위 첫 항목), 7차 Options의 안티앨리어싱·FSR 1.0 선택(둘 다 기본값은 꺼짐)과 게임 적용이다. 현재 기존 경로는 자체 `settings.json`(`DisplaySettings`)을 쓰고 새 경로는 R7에서 `options.cfg`로 옮기므로, 5차는 R7의 저장 구조 위에서 위치만 바꾸는 순서로 진행한다.

### 0.6 검증 단계와 기록 규칙

- **L1:** 원본 x86 fixture(기존 방식). **L2:** 같은 미션 Load/Start 상태가 cpppj와 같다. **L3:** 같은 고정 시계·입력열로 N프레임 진행한 상태 해시가 같다. **L4a(신규):** 같은 ui-script의 논리 캡처가 픽셀 차이 0이다. **L4b:** 소리 시점과 곡 선택이 같다. **L5:** 자연 완주·저장·재실행. 사용자 확인 대상이라 AI 작업과 단계 완료 기준에 넣지 않는다(0.5).
- cpppj 캡처는 오라클 SHA·스크립트·`--set` 값·프레임과 함께 기록한다. cpppj가 논리 캡처(`snapshot`)와 실제 출력 캡처(`snapshot-output`)를 구별하듯 dotnetpj도 둘을 구별한다. 실행에는 `originals/`가 아니라 `extracted/` 아래의 데이터 사본을 쓴다.
- 한 단위를 마칠 때 기록할 것: 옮긴 cpppj 파일과 기준 커밋, 연결 fixture와 행 수, 이식한 cpppj 검사 수, 은퇴시킨 기존 코드와 검사, 비교한 캡처 수와 차이, 남은 경계. L1 통과를 L2 이상으로 합산하지 않는다. 기존 근사 검사의 은퇴로 실패를 감추지 않으며, 원본 fixture 기대값을 C# 결과로 바꾸지 않는다.
- 클론·cpppj 창 실행은 AGENTS.md의 확인·예외 규칙을 따른다(VM-W11-Codex·HJOW-Athlon·vm-debian-codex는 예외 PC). HJOW-X3D를 포함한 그 밖의 PC에서는 실행 전에 목적을 설명하고 명시적 확인을 받는다. 이 계획은 원본 게임을 실행할 필요가 없다.

### 0.7 계속 유효한 결정·제약

- **목표 순서:** ① 원본 10.78과 동일한 동작 → ② 요구사항(화면비·60/120프레임·다국어·재실행 오류 방지) → ③ Linux. 이미 들어간 요구사항 기능(한국어 선택·와이드·전체화면)은 버리지 않는다. 새 경로로 옮길 때 cpppj 방식(논리 해상도·호환 전체화면)으로 다시 만들고, 한국어는 R8에서 다시 만든다(0.8 2번).
- **AGENTS.md 상위 요구:** cpppj의 후순위 4~7차(4차 MCP, 5차 저장 위치·커스텀 맵·Edit 목록, 6차 한국어, 7차 안티앨리어싱·FSR 1.0)가 **dotnetpj 후순위 요구에도 포함**된다. 원본과 같은 동작(목표 ①) 뒤에 R8에서 다루며 세부 내용은 [LEFT_JOBS.md](LEFT_JOBS.md) 1.1절을 따른다. 7차의 FSR은 `7dd1ab4e` 기준 FSR 1.0이고 FSR 3.1 안은 쓰지 않는다. `ef0c6b91`에 따라 cpppj·dotnetpj 모두 **Windows 용 게임 실행 파일 이름이 `Netstorm.exe`**여야 한다. cpppj는 적용을 마쳤고 dotnetpj는 미착수다. 후순위가 아니라 배포물에 직접 보이는 요구이므로 0.8 5번과 0.9의 이름 적용 항목을 따른다.
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
- **근거 우선순위:** 원본 fixture → cpppj 보고서의 계약·경계 → cpppj 소스 → 녹화·스크린샷. cpppj는 HEAD만 쓰고 미커밋 변경은 쓰지 않는다. `cpppj/`·`LEFT_JOBS.md`는 수정하지 않고, 필요한 요청은 이 문서와 이식 계획 8절에 적어 사용자에게 전달한다.
- **보호·커밋:** 원본/CD·AGENTS.md는 수정하지 않는다. push는 하지 않는다. 커밋 메시지는 `YYYYMMDD-NN DN 요약`이다. 새 문서·코드는 UTF-8과 한국어 주석 규칙을 따른다.

### 0.8 사용자 확인이 필요한 것 (확인 전까지 에이전트 기본값으로 진행)

1. **기존 경로 처리(결정 5):** R3부터는 메뉴, R5부터는 미션의 기본 실행이 새 경로로 바뀐다. 기존 1-1·1-2 근사 플레이는 `--legacy`로만 남기고 R6 뒤 삭제한다. 그동안 새 경로에서 플레이할 수 있는 범위가 일시적으로 줄 수 있다.
2. **한국어(0.7):** 새 경로는 R8 전까지 영어 원본 화면만 제공하고, 한국어 선택은 `--legacy`에서만 쓸 수 있다. 한국어가 계속 필요하면 D2Coding 단색 글리프 출력을 R3으로 앞당긴다.
3. **cpppj 미복원 경로(결정 6):** 새 경로는 cpppj와 같은 명시 오류·경계를 쓴다. 이전 3절 9번의 "근사 유지" 기본값은 기존 경로에만 남는다.
4. **프로젝트 배치(결정 3) — 해결:** `Netstorm.Client`로 만들었다.
5. **`Netstorm.exe`를 어느 실행 파일이 갖는가(AGENTS.md ef0c6b91):** dotnetpj에는 실행 파일이 둘이다. 기존 게임 `Netstorm.Game`(`NetstormClone.exe`, 배포 대상)과 새 경로의 개발 호스트 `Netstorm.Client.Host`(`Netstorm.Client.Host.exe`, 아직 배포하지 않음)다. 한 배포 폴더에 `Netstorm.exe`는 하나뿐이다. **기본값:** 지금은 배포되는 `Netstorm.Game`의 `AssemblyName`만 `Netstorm`으로 바꾼다. 결정 5의 기본 실행 교체 때(R3 메뉴·R5 미션) `Netstorm.exe`가 새 경로로 진입하게 하는 방법은 둘이다. A) `Netstorm.Client.Host`가 `Netstorm.exe`를 이어받고 기존 Game은 `--legacy` 전용 이름(예: `NetstormLegacy`)으로 내리거나 새 호스트에 흡수한다. B) `Netstorm.Game`이 계속 `Netstorm.exe`이고 새 경로를 라이브러리로 호출한다. 어느 쪽이든 사용자가 보는 이름은 `Netstorm.exe` 하나다. 원하는 방식이 있으면 R3 착수 전에 알려 달라.
6. **4차 MCP의 dotnetpj 해석(AGENTS.md bfccadf5):** AGENTS.md는 'cpppj의 4~7차 목표 내용이 dotnetpj 후순위 요구사항에도 포함된다'고만 적었다. 4차 MCP는 dotnetpj 개발을 위해 게임을 분석하는 용도라서, dotnetpj 클론에도 같은 MCP를 붙이라는 뜻인지 cpppj 쪽 MCP를 개발에 활용하라는 뜻인지 문장만으로는 정해지지 않는다. **기본값:** R8 전에는 구현하지 않고 cpppj의 MCP 결과가 나온 뒤 사용자에게 확인한다.

### 0.9 바로 다음 작업

체크 항목은 AI 작업 대상이다. 자연 완주, 전체 동일성, Windows 11 실기는 체크 항목이 아니며 단계 전환을 막지 않는다(0.5).

재개하면 먼저 `python dotnetpj/tools/cpp_sync.py`와 `--ports`로 cpppj 변경 영향을 확인한다.

- [ ] **R4 실제 호스트·월드 조립 (재개 지점, 0.73):** 미션 객체, Fort reader, Guide 교체·수명, 미션 제어 입력, 공통 Start, AI 설정·생성, Guide 단계·Lifecycle은 이식과 L1 대조를 마쳤지만 검사에서는 명시 효과로만 연결돼 있다. 다음을 실제 호스트와 세션에 조립한다.
  - 미션 세션의 Fort 선택기·지연 reader(0.75), Guide 소유자·명령 및 Tutorial 반납은 0.77에서 연결했다. 0.78에서 호스트 프레임의 큐 소비와 Guide 우선 분배기를 추가했다. 0.79에서 실제 파일·설정·팔레트·타이틀 화면, 0.80에서 초기 구름 캐시 출력, 0.81에서 --set의 실제 Config 시작 인자를 호스트에 연결했다. 실제 호스트에서 분배기를 설치하고 같은 Player·제한 표·메뉴·월드 효과를 연결해야 한다(0.66, 0.68, 0.69, 0.76~0.79)
  - 0.105에서 Tutorial Init의 설정·전역·이름·요새 요청·Header 순서를 이식하고, 0.106에서 실제 옵션 초기화를 이식해 전체 옵션 바이트까지 fixture로 대조했다. 0.107에서 요새 요청의 상태144바이트·Zone 복제를 이식했다. 0.108에서 Guide와 Tutorial Init의 flags·demo·tutorialNumber·missionNumber 및 현재 Config를 같은 소유자에 결합했다. 0.109에서 기술 허용 목록과 공유 제한 표 쓰기를 이식했다. 0.110에서 현재 Guide·옵션·요새 요청·기술 제한을 초기화 순서로 조립했다. 실제 템플릿/옵션 소유자/요새 선택·상태 실행/실제 타입 조회와 호스트 단계 연결은 미완이다.
  - Start의 실제 Config·Player·SP·AI·Gump 효과(0.70)
  - AI 설정의 Session 소유자, 실제 AI 생성자·초기 갱신·전략 AI(0.71, 0.72)
  - `PlayerParticipation`의 현재 getter는 이식했다(0.74). 실제 세션 바인딩과 동맹 수신 `ReceiveAlliance` 등 나머지는 미완료다.
  - **Template/DataManager 계층 미이식 (0.127에서 확인, 다음 작업의 공통 선행 조건):** dotnetpj 전체에 `DataManager`·`FortTemplate`·`FortImageData`가 없다. cpppj `client/CampaignTemplateStorage`, `client/ClientTemplateSectionNames`, `o/Template`, `o/RawDataManager*`(Dump·FileSave·Image·Initialize·Lifetime·List·Resize·Selection), `o/RawFortImage`, `o/RawMemStream`, `o/RawPlayerTemplateState`, `o/RawTemplateLifetime`·`RawTemplateLoad`·`RawTemplateSectionNames`, `o/RawFortCurrentName`, `o/FortSerializationGlobals`(테스트 제외 약 2,400줄)가 대상이다. 미션 Fort 적재(R4), Editor State1 월드·저장·Save as의 Template/현재 이름 소유자(`HostFortNameConfirmation`의 `Header`·`SetName`), `EditorFortFileDevices.BindLoad`의 소비자와 `ExistingEditorTemplateValid`가 모두 이것을 기다린다. 0.127에서 `TemplateFileWriteHooks`·`TemplateFileLoadHooks` 두 형식만 `TemplateFileHooks.cs`로 먼저 옮겼다. 다음 단위는 `RawMemStream`→`RawDataManager*`→`Template`→`CampaignTemplateStorage` 순서의 이식과 원본 fixture 대조를 권한다.
  - 0.5 R4의 나머지 범위
- [ ] **R3 잔여 → 이정표 D1:** 위젯 본체는 대부분 옮겼으나 R3 전체 완료로 세지 않는다.
  - 대화창 분리(`ParseDialogPage`)와 `ConfigOverwriteAndQueueLifetime`의 남은 부분, MessageGump·대화창
  - 맵/파일/기술 목록 owner, 실제 상위 메뉴 소유·분배, `RawMenuControls`의 전체 모달·등록 소유자와 링크·명령 소비자. 0.83에서 공통 돌 버튼 트리와 실제 셰이프·글꼴로 메인 메뉴 여덟 버튼을 표시했다. 0.84에서 같은 트리의 캡처 입력과 여덟 원본 상위 명령 분배를 연결했다. 0.86에서 실제 호스트 시계와 버튼 음향 장치 경계를 연결했다. 0.87에서 물리·스크립트 포인터 폴링을 호스트에 연결했다. 0.88에서 실제 Config를 쓰는 Tell 인자·섹션 선택과 시작 화면 소유자를 연결했다. 0.89에서 버튼 트리에 기존 모달 처리기를 연결했다. 0.90에서 같은 트리에 파생 노드 할당·serial 조회와 가상 사건/수명 장치 연결을 제공하고 HTML 부모/버튼의 실제 입력·정리를 검증했다. 0.91에서 실제 fortGump 배경과 I 모서리를 공유 트리의 장식 생성·출력에 연결했다. 0.92에서 실제 호스트의 HTML 부모·버튼 소유자를 공통 트리와 큐에 설치했다. 0.93에서 같은 트리에 실제 글꼴·본문 버퍼·슬라이더를 조립했다. 0.94에서 실제 파일 글꼴로 개별 본문을 논리 표면에 클립 출력한다. 0.95에서 실제 본문·버튼·화면/메뉴 뷰를 전체 창 Fit에 연결했다. 0.97에서 실제 Config 조건·두 번 파서의 링크 수명과 HTML 명령의 실제 큐 예약을 연결했다. 0.98에서 실제 controlled 메뉴 열 생성·측정·Fit·부분 생성 정리를 연결했다. 0.99에서 같은 실제 Config·파서·메타·모달과 전체 창 생성기를 조립했다. 0.100에서 통합 그리기에 필요한 원본 표시 목록 정렬을 이식하고 원본 1,155행과 대조했다. 0.101에서 실제 공통 트리 수집·정렬과 배경/버튼/장식/본문/슬라이더/메뉴 표시를 같은 표면에 연결했다. 0.102에서 메인 입력 큐·다음 Tick Tell과 HTML 닫기·Config 및 메인 종료 명령을 연결했다. 0.103에서 실제 메뉴 항목 입력·포인터 통지·원본 커서와 이름 소리를 연결했다. 0.104에서 Tell @/#/% 파일 목록과 같은 장기 Config·FileFinder를 연결했다. 0.111에서 실제 Help 팝업의 About 행과 Tell을 연결했다. 0.112에서 Options·해상도·음량 팝업과 설정 토글을 연결했다. 0.113에서 화면 모드 절대 변경·미리보기·확인 명령을 실제 호스트에 연결했다. 0.114에서 변경된 옵션을 실제 options.cfg에 저장하고 재시작 시 화면 크기를 복원한다. 0.115에서 편집 맵 목록의 원본 50칸 캐시와 분류·행 표시를 이식했다. 0.116에서 Tell 맵 열 분할·파일 키 복구와 메타 연결 경계를 이식했다. 0.117에서 AskEditFort/ReadFort와 편집 복귀 State 예약을 이식했다. 0.118에서 기본 디스크 열거와 실제 Fort 섹션 기반 맵 메타 조회를 조립했다. 0.119에서 실제 호스트 Edit 입력·Tell 맵 목록과 ReadFort 예약을 연결했다. 0.120에서 새 Fort 이름의 확인·취소 본문을 이식했다. 0.126에서 편집 작업본 장치와 이관 장부를 맵 목록·새 이름 충돌 검사에 연결했다. 0.127에서 같은 작업본 장치에 존재·readonly·권한·Template 쓰기·형제 metadata·원자 쓰기를, 현재 Fort 파일 장치(경로·준비·존재·읽기)를 더했다. Guide 우선 연결 등 기타 상위 명령·실제 맵/기술 목록 조립, HTML 그림 자산과 다른 파생·월드 통합 출력은 아직 미완이다.
  - 전체 자산 조립, `ReadAssetQuickHelpType`의 TYPE/AST·요구 에너지 후처리, 음향
  - 0.121에서 실제 호스트 트리에 TextEntry/버퍼/두 Slider와 입력·그리기·수명을 조립했다. `RawTextEntryControls`의 Console·부분 생성 정리·시스템 클립보드, Save as·숫자 창·ChatWindow, `RawBackgroundGifControls`의 노드·창 소유자
  - 호스트 프레임·입력 큐, report·kernel-tell. 0.85에서 메인 메뉴 라벨 좌표를 실제 버튼에 연결했으며 하위 화면 라벨과 실제 호스트 명령 소비는 남는다.
  - D1 메뉴 캡처 비교. R0-1의 오라클은 당시 `0cc1d33d` 빌드이고 cpppj 산출물 이름도 `Netstorm.exe`로 바뀌었으므로, 비교 전에 현재 HEAD로 다시 빌드해 SHA를 기록한다.
- [ ] **R2 잔여 세션 연결:** 실자산 어댑터, 수집·정렬·kind 분배, `SpriteDraw` 클래스·그림자 alias 공유, `owner-capture` C행 480개는 R4·R5에서 잇는다.
- [ ] **9-1 A 나머지(2절):** 정수 틱을 실제 시각·원본 타이머로 옮기는 일을 세션 교체와 함께 처리한다.
- [ ] 이후 R3(D1) → R4(M1) → R5(D2) → R6 → R7 → R8 순서로 진행하고, 단위마다 이 절의 체크와 [0.4](#04-기존-dotnetpj-소스의-재수정-판정) 판정표를 갱신한다.
- [ ] **11.01 패치 요구 (AGENTS.md `462c9a64`, 2026-11-01 추가, 미착수):** 새 버전 모두 버전 표기를 11.01로 하고, 메인 메뉴의 Credits 목록(기존 2개)에 세 번째 항목 "Netstorm 11.01 Patch Credits"를 추가한다. 선택하면 다른 Credits 메시지 창과 같은 창이 나오되 제목은 "NetStorm 11.01 Patch Credits", 내용은 HJOW 개발과 AI 사용에 대한 안내다. dotnetpj는 기존 경로(`Netstorm.Game`)와 새 경로(Credits는 R3 범위) 양쪽에 적용해야 하고, 버전 표기가 나오는 위치(타이틀·About·창 제목·배포 이름 등)를 먼저 조사한다. 세 프로젝트의 문구가 같도록 구현 전에 cpppj HEAD의 반영 여부와 문구를 확인한다.
- [x] **기존 경로 `Netstorm.exe` 이름 적용:** 0.96에서 빌드·배포·유지 중인 실행 도구와 안내를 갱신하고 ZIP의 시작을 확인했다. 새 경로 기본 실행 전환은 별도 작업이다.

### 0.10~0.65 — 이관

2026-10-28 문서 작업 기록과 단위 기록 0.11~0.65는 [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073)로 옮겼다. 완료 범위는 1절 색인을 본다. 아래 0.66 이후는 별도 단위 문서가 없고 다음 작업의 직접 근거라 본문에 남겼다.

### 0.127 단위 기록 — 편집 파일 장치 잔여 이식 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `EditorFiles`의 남은 부분을 `Netstorm.Client/Host/EditorFiles.cs`에 옮겼다. `Exists`(열리는 일반 파일만 5, 디렉터리·부재 0), `ReadOnly`(readonly 속성만 읽고 Config와 독립, 빈 이름 진단 0x480·stat 실패 진단 0x48e 뒤 명시 오류), `MakeWritable`(모드 0x180만 받아 readonly 속성만 풀고 실패는 무시), `Writer`/`WriteTemplate`(원문 0041bb90: 기존 파일은 자르지 않는 `r+b`로 먼저 열고 실패할 때만 `w+b`로 만들어 기존 파일의 뒤쪽을 보존하며 부모 폴더를 만들지 않음), `SiblingNamespace`(작업본·원본을 모두 보호하는 `-state` 형제 장치, 이름 범위·외부 링크 경계 거부), `WriteAtomic`(임시 파일 flush 뒤 Windows `MoveFileExW` write-through 교체, 접근 거부·공유/잠금 위반·매핑 거부만 20ms 간격 최대 51회 재시도)이다. `EditorCatalogLedger`는 이 `WriteAtomic`을 거쳐 장부를 갱신한다(0.126의 단발 `File.Move` 차이 해소). `EditorFortFileDevices.cs`는 cpppj `EditorFortFileDevices`의 `CurrentPath`(현재 이름 진단 39번·같은 FileSpec 선택·`PathSpec`·작업 namespace 해석), `PrepareCurrent`(부모 폴더만 만들고 없는 작업본만 원본에서 복사, 준비/해석 효과 주입 가능), `Exists`/`ReadOnly`/`Read`(원본으로 fallback 안 함), `BindLoad`(읽기·권한·ASCII stricmp 이름 비교)이다. `TemplateFileHooks.cs`에 cpppj `CampaignTemplateStorage.h`의 `TemplateFileWriteHooks`·`TemplateFileLoadHooks` 두 형식만 옮겼다.
- **검증:** `EditorFilesDeviceTests` 19개를 추가했다. cpppj 검사 중 Template 저장소가 필요 없는 14개를 한글·이모지 경로의 임시 작업본에서 직역하고(`EditorFileWriterTests` 3·`EditorFilesTests` 3(보호 경로 1, `EditorWorkingRoot` 표기 정규화 2를 `EditorWorkspacePaths`로)·`EditorAtomicWriteTests` 2·`EditorFortFileDevicesTests` 7, 저장소가 필요한 부분은 뺌) 직접 검사 5개를 더했다(존재·readonly·권한, readonly 진단, 형제 metadata, 장부 재시도 연결, 준비·해석 효과). 저장소가 필요한 cpppj 검사 8개(Writer 1·Files 5·FortFileDevices 2)와 `Prepared`·`ReadOnly` 검사 안의 Load·Save 부분은 Template 이식 뒤 옮긴다. 기존 장부·작업본 10개와 합쳐 집중 29개, Client 전체 Release 729개(기존 710 + 19) 통과, 실패·건너뜀 0. 변이 3종(Template 쓰기를 항상 잘라쓰기로, 원자 교체 재시도 제거, readonly 속성 무시)이 각각 새 검사를 실패시킴을 확인하고 복원했다. 원본 x86 fixture가 없는 직역 검사라 L1이 아니다. `PORT_MAP.tsv` 323행(`EditorFortFileDevices`·`CampaignTemplateStorage.h`·`EditorWorkspacePaths` 대응 4행 추가, `EditorFiles` 기준 커밋 `473432cd`로 재대조).
- **실제 호스트:** 격리 데이터 복사본(`extracted/dotnet-editor-127/`, `NETSTORM_DATA`로 지정)에서 새 호스트로 Edit를 열어 1024×768 목록 캡처를 확인했다(작업본 fort 433개, 장부 9,546바이트로 0.126과 같음, 원본 폴더 fort 433개 그대로, 종료 0). 이번에는 실제 호스트가 새 파일 장치를 직접 소비하지 않으며, 장부 갱신 경로만 새 `WriteAtomic`을 거친다. cpppj와의 장부 상호 읽기·화면 비교는 하지 않았다. 금지된 플레이 재현은 하지 않았다.
- **미완료·차이:** Template 저장소 계층 전체(0.9의 R4 항목)와 `ExistingEditorTemplateValid`, `EditorFortFileDevices.BindLoad`의 실제 소비자(Load/Save), Native 이름 변경, 실제 Editor State1 월드·저장·Save as의 Template/현재 이름 소유자는 미이식이다. cpppj의 `RelativePath`는 옮기지 않았고 상대 키는 `EditorWorkspacePaths.Resolve`가 계산한다. Windows 외의 `WriteAtomic`은 write-through와 재시도가 없는 준비용 `File.Move`이며 durable 검증으로 주장하지 않는다. `ReadOnly`의 Windows 외 동작(쓰기 비트)은 관측하지 않았다. 새 파일 장치의 이름 정규화는 cpppj처럼 ASCII만 소문자로 접으며, 기존 `GameFileSystem.NormalizePath`(비ASCII도 접음)와는 차이가 있다(이번에는 고치지 않음). 같은 작업 트리에서 다른 세션이 cpppj 파일(`UberEditor.inc` 등)을 수정 중이었고 이 단위는 그 파일들을 쓰지 않았다(HEAD 기준만 사용).

### 0.126 단위 기록 — 편집 작업본 장부 연결 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `EditorFiles`(경로 해석·보호 루트·최초 복사·OS 순서 열거·현재 읽기·준비·쓰기·삭제)와 `EditorCatalogWorkspace`(`Import`·`ImportDisk`·`Prepare`·`Enumerate`·`Read`)를 `Netstorm.Client/Host/EditorFiles.cs`·`EditorCatalogWorkspace.cs`로 옮기고 0.125 장부에 연결했다. 완료된 키는 작업본에서 지워도 원본 loose 파일이나 아카이브로 되살아나지 않으며, 원본 디렉터리는 내용 없이 빈 작업 폴더가 되고, 명시 부재는 장부에 기록하지 않는다. 아카이브 전용 읽기용 `GameFileSystem.TryReadArchiveOnly`를 Assets에 추가했다(기본 폴더의 등록 순서 첫 일치만, 디스크·보조 폴더로 되돌아가지 않음). `EditorWorkspacePaths`는 검사용 명시 작업 루트를 받는다.
- **연결:** `SoloMapCatalog`가 열거·읽기 장치를 받아 `HostEditorSelection`이 작업본 장치를 소유하고 목록·분류·메타를 모두 이것으로 읽는다. `HostFortNameConfirmation`은 같은 장치로 경로를 해석하고 cpppj `RawMenuSaveFort`처럼 작업본에서만 파일·디렉터리 존재를 본다(0.124의 원본 loose 중복 확인 제거). `ClientHostGame`이 종료 시 장부 잠금을 반납한다.
- **검증:** 임시 원본·작업본의 직역 검사 9개(OS 순서·기존 사용자 내용과 읽기 전용 속성 보존, 새 저장 뒤 목록·재시작, 중복 이름 두 행, 처음 50칸, 삭제 완료 별칭·직접 준비 비부활, 아카이브 전용 읽기, 디렉터리·명시 부재·"."·".." 후보·형제 장부·접합 링크 거부, Pending·손상·잠금·장부 임시 파일 실패, 중첩 루트 거부)와 기존 장부·목록·호스트 검사를 합쳐 집중 14개 통과. 호스트 검사 2개는 실제 자산을 임시 작업본으로 바꿔 자산 옆에 `-editor` 폴더가 생기지 않게 했다. Client 전체 Release 710개와 Assets 집중 3개 통과.
- **실제 호스트:** 격리 데이터 복사본(`extracted/dotnet-editor-126/`)에서 새 호스트로 Edit를 열어 목록 화면을 확인했다(작업본 fort 433개, 장부 9,546바이트, 원본 폴더에는 새 파일 없음, 종료 0). 작업본 `battle4.fort`를 지우고 재실행하니 432개가 유지되고 목록에서 빠졌다(종료 0). cpppj와의 장부 상호 읽기·화면 비교는 하지 않았다. 금지된 플레이 재현은 하지 않았다.
- **미완료·차이:** (0.127에서 해소: `EditorFiles`의 readonly 조회·권한 변경·Template 쓰기와 형제 metadata 장치, `EditorFortFileDevices` 본체, 장부 교체의 재시도.) Native 이름 변경, 실제 Editor State1 월드·저장·Save as의 Template/현재 이름 소유자는 미이식이다. 장부 교체는 0.126 당시 cpppj의 최대 1초 재시도 없이 한 번 `File.Move`했다. cpppj와의 장부 교차 읽기·`MatchDisk`의 Win32 특수 경계(절대 경로·긴 경로)는 검증하지 않았다. 접합 링크는 임시 폴더에서만 확인했으며 전체 목표는 미완료다.

### 0.125 단위 기록 — 편집 이관 장부 (2026-11-01, HJOW-Athlon)

- cpppj 장부의 `NSEFORT` 버전 1, 원본/작업 루트 식별, 전체 경로별 대기·완료 상태, 배타 잠금, flush 후 형제 파일 교체 및 손상/갱신 실패 처리를 이식했다. 작업본과 장부 경로의 기존 링크를 검사한다.
- 격리 임시 디렉터리에서 상태 재개, 배타 잠금, 장부 손상 거부와 원본 보존 및 기존 이름 확인·경로 집중 3개 통과. cpppj와의 실제 장부 교차 실행 및 교체 실패·링크 교체 경합은 검증하지 않았다.
- **미완료:** (0.126에서 해소: 목록·복사·archive 경로 연결, 0.124 임시 원본 중복 검사 제거.) Windows 파일 교체의 재시도·durable 세부 차이는 후속 비교 대상으로 남는다.

### 0.124 단위 기록 — 새 이름 확인과 편집 경로 (2026-11-01, HJOW-Athlon)

- 원본 자산 옆 `-editor` 작업본 루트와 설정 상대·원본/작업본 절대 경로 매핑, 기존 링크 해석과 루트 이탈 검사를 추가했다. 새 이름 확인은 실제 문자 입력을 `SaveFortName.Confirm`에 전달하고 중복이 없으면 문자열 고리와 같은 양쪽 인자의 ReadFort를 큐에 넣는다. 실제 편집 선택 분배기까지의 State1 예약을 검사했다.
- Release 호스트 빌드와 실제 자산 기반의 기존 이름 충돌·새 이름 큐·공통 모달 및 경로 검사 집중 6개가 통과했다. 실제 게임 구동이나 월드 설치는 이번 단위에서 실행하지 않았다.
- **미완료·차이:** (0.126에서 이관 장부·작업본 목록 연결과 임시 중복 검사 교체를 마쳤다. 아래는 당시 기록이다.) cpppj 이관 장부/잠금/복사와 작업본 목록은 미이식이다. 현재 목록이 원본 loose 파일을 직접 열거하므로 이름 충돌에서도 원본 파일과 작업본을 함께 확인하는 임시 경계를 둔다. 이미 이관 후 삭제된 이름의 동작은 cpppj와 다르므로 장부·목록 이식 시 반드시 교체한다. Save as의 Template/현재 이름 소유자와 State1 월드 설치도 미연결이며 전체 플레이 목표는 미완료다.

### 0.123 단위 기록 — 호스트 이름 모달 조립 (2026-11-01, HJOW-Athlon)

- 실제 공통 Gump 트리에 `SaveFortNameDialog`의 장식·문자 입력·제목·돌 버튼·모달·사건·수명을 연결하고 `NewFort` 명령을 호스트에 분배한다. 한 줄 `TextGump`도 공통 스타일·그리기·소멸에 연결했다.
- Release 집중 8개 통과. 격리 데이터 사본을 쓰는 실제 호스트에서 Edit → Create New Map으로 이름 창 표시를 확인하고 정상 종료했다. 원본과 화면 비교는 하지 않았다.
- **미완료:** 이름 확인은 편집 작업본·파일 존재 검사·Template 및 ReadFort/월드 소유자가 미연결이어서 명시 오류를 낸다. 기존 이름 저장·숫자 송금 및 전체 Editor/플레이 목표도 미완료다. 다음에는 cpppj `EditorFiles`·catalog 경로와 `UberEditor.inc` 확인 장치를 실제 동일 작업본·큐에 연결한다.

### 0.122 단위 기록 — 이름·숫자 모달 본체 이식 (2026-11-01, HJOW-Athlon)

- `cpppj/src/client/SaveFortNameDialog.cpp`의 이름 창과 송금 숫자 창 생성·자식 배치·모달 등록 및 확인/취소/Enter 사건을 공유 장치 경계로 옮겼다. 제목 폭, 배경, 입력 제한, 버튼 serial 및 지연 삭제 순서를 보존한다.
- Release 빌드와 이름·숫자 모달의 대역 저장소 배치·사건 검사, 기존 확인 및 실제 트리 문자 입력 집중 6개가 통과했다. 원본 x86 fixture 비교나 화면 비교는 하지 않았다.
- **미완료:** 새 창 본체를 실제 호스트 공통 트리와 `NewFort` 사건에 연결하고 실제 제목·버튼·모달 및 파일/월드 소유자에 접속해야 한다. 사용자 목표 전체는 미완료다.

### 0.121 단위 기록 — 호스트 문자 입력 조립 (2026-11-01, HJOW-Athlon)

- **구현:** 기존 원본 TextEntry 본체를 실제 호스트 공통 트리·버퍼·두 Slider·글꼴·픽셀·입력·idle·삭제에 조립하고 제품 호스트의 정렬 출력에 설치했다. 화면 출력 장치는 Draw 호출 수명에만 빌리며 고아 버퍼는 종료 시 정리한다.
- **검증:** 실제 트리에서 이름 버퍼 생성·문자 입력·그리기·자식 우선 반납 및 기존 TextEntry/Editor 검사 8개가 통과했다. 초기 빌드의 반환값 delegate 연결 2곳을 수정했다. 별도 데이터 복사본에서 실제 호스트 Edit 목록을 다시 열고 종료 0을 확인했다. 금지된 플레이 재현은 하지 않았다.
- **잔여:** Console·시스템 클립보드와 부분 생성의 모든 노드 정리, SaveFortName 대화창 생성·버튼·확인 연결은 미완료다. 현재 문자 입력기는 호스트에 설치되었으나 Create New Map에서는 아직 생성하지 않는다. Editor 월드와 전체 목표도 미완료다.

### 0.120 단위 기록 — 새 Fort 이름 확인 이식 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `SaveFortNameDialog`의 확인·취소 본문을 이식했다. 현재 문자 입력의 첫 줄, 원본 alnum 호출과 허용 문자 변환, 실제 경로·중복 검사, 현재 Fort 모드 재조회, readonly 변경과 이름/ReadFort 효과 순서를 보존한다.
- **검증:** 독립 원본 288행의 결과·헤더·효과 순서를 대조하고, 변경 효과 실패의 앞선 readonly 쓰기 및 콜백 중 모드 변경을 검사했다. 집중 3개 통과. 실제 게임 실행이나 금지된 플레이 재현은 하지 않았다.
- **잔여:** 실제 TextEntry·SaveFortName 대화창 및 작업 파일/Template 소유자에 연결되지 않았다. Create New Map과 Editor 월드·저장, 전체 목표는 미완료다.

### 0.119 단위 기록 — 실제 호스트 Edit 목록 연결 (2026-11-01, HJOW-Athlon)

- **구현:** 실제 메인 Edit 입력을 AskEditFort·현재 pending 이름·맵 캐시와 LoadEditable Tell에 연결하고, 같은 HTML 메뉴의 실제 Fort 행을 ReadFort 문자열 고리·State1 예약에 연결했다. 아직 월드 소비자가 없으므로 다음 Poll은 해당 State를 명시 오류로 중단한다.
- **검증:** 실제 자산의 Edit 버튼→맵 목록→실제 포인터 행 선택→pending/State1을 검사했고 기존 Campaign·Help까지 집중 3개 통과. 별도 데이터 복사본으로 새 호스트를 실행해 두 열 맵 목록 캡처 및 종료 0을 확인했다(`extracted/dotnet-edit-119/`). 실제 월드를 설치하지 않았고 금지된 플레이 재현은 하지 않았다.
- **잔여:** 전체 Template/DataManager와 EditorCatalogWorkspace의 작업 복사본·삭제 정책, State1 설치·생성·저장 및 Editor HUD가 미완료다. 현재 목록은 기본 데이터 디스크를 읽으며 원본/cpppj 화면 비교도 하지 않았다. Create New Map과 행 선택 뒤 편집은 아직 불가능하고 전체 목표는 미완료다.

### 0.118 단위 기록 — 실제 단독 맵 파일 목록 (2026-11-01, HJOW-Athlon)

- **구현:** 기본 디스크만 OS 순서로 열거하는 MatchDisk와 실제 Fort 35개 섹션의 Subscriber·State를 읽는 SoloMapCatalog를 추가했다. 분류와 목록 조회는 각각 파일을 다시 읽고, 빈 State는 호출자가 제공한 미정 28바이트를 원본 규칙으로 초기화하며 짧은 State는 확장한다. 서명/섹션 범위 오류는 파일을 삭제하지 않고 거부한다.
- **검증:** 실제 배포 Fort 433개의 종류를 각 파일의 원문 바이트와 대조하고 편집 목록 최대50칸을 구성했다. 별도 임시 파일에서 빈/짧은 State와 사용자·손상 표시, 기본 디스크와 보조/아카이브 분리를 검증했다. Client 집중 2개와 Assets 집중 1개 통과. 실제 게임 실행과 금지된 플레이 재현은 하지 않았다.
- **잔여:** 이 메타 reader는 cpppj의 전체 CampaignTemplateStorage/DataManager 수명·파일 진단과 EditorCatalogWorkspace의 작업 복사본/삭제 의미를 아직 대체하지 못한다. 현재 제품 호스트에는 설치하지 않았으며 실제 Tell·Editor State/월드, 전체 목표는 미완료다. MatchDisk의 절대 경로·Win32 특수 경계도 추가 대조가 필요하다.

### 0.117 단위 기록 — 편집 Fort 선택과 State 예약 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `FortSelection`의 AskEditFort/ReadFort 분배, 현재 pending·목록·Tell 호출, State6/State1 예약과 전투 복귀 시 현재 Template/시간/섬 소유 조회 순서를 이식했다. 미션 Fort 요청과 같은 144바이트 다음 State를 빌릴 수 있다. 선택된 외부 효과만 필수로 요구하며 복사본 대신 현재 전역을 재조회한다.
- **검증:** 독립 원본 3,672행의 상태·두 State 바이트·효과 순서와 입력 해제/부분 실패를 대조했다. 집중 2개 통과. 실제 게임 실행과 금지된 플레이 재현은 하지 않았다.
- **잔여:** 원본 입력/목적 메모리가 겹치는 pending 이름의 바이트 span 오버로드는 미이식이다. 제품 호스트의 실제 파일 목록·Template·선택 저장소·상태 실행 및 Editor 월드가 필요하며 Edit과 전체 목표는 미완료다.

### 0.116 단위 기록 — Tell 맵 목록 부모 이식 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `TellMapMenus`의 단독/일반 맵 목록 선택, 실제 캐시 재조회에 따른 열 분할과 SOLO 키 복구를 이식했다. 같은 RawSoloMapList 행·ReadFort 명령 및 HTML 메타의 현재 열 수에 연결하는 장치를 제공한다.
- **검증:** 독립 원본 1,728행의 열 수·dirty·파일 키·호출 순서를 대조하고, 실제 메타 처리에서 16개 행의 두 열 분할을 검증했다. 집중 테스트 3개 통과. 실제 게임 실행이나 금지된 플레이 재현은 하지 않았다.
- **잔여:** 실제 파일 목록·Template 메타와 연결한 SoloMapCatalog, AskEditFort/ReadFort 및 Editor State/월드가 필요하다. 새 장치는 제품 호스트에서 아직 설치되지 않았고 Edit 및 전체 목표는 미완료다.

### 0.115 단위 기록 — 편집 맵 목록 캐시 이식 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `RawSoloMapList`의 파일 종류 필터, dirty 캐시 정리·재사용, 50칸 소유 이름/플래그, 현재 선택과 손상 행 표시를 이식했다. 파일 열거·Template·사용자·번역은 실제 소유자가 제공할 경계로 남겼으며 콜백 뒤 현재 필드를 재조회한다.
- **검증:** 독립 원본 fixture 216행의 전체 50칸과 행·하위 호출 순서, 콜백 중 캐시 변경 검사를 포함한 집중 2개 통과. 실제 게임 실행이나 금지된 플레이 재현은 하지 않았다.
- **잔여:** Tell 맵 목록 부모, 실제 파일 분류·Template 소유자·AskEditFort/ReadFort 및 Editor State/월드를 연결해야 한다. Edit은 아직 동작하지 않으며 전체 목표는 미완료다.

### 0.114 단위 기록 — 옵션 저장과 재시작 화면 크기 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `ClientOptionsFile`의 CP1252 원본 XOR, Unicode 경로용 서명 뒤 UTF-8 BOM, 쓰기 실패 시 변경 표시 보존을 이식했다. 시작과 변경된 프레임에서 options.cfg를 저장하고, 다음 시작에 SCREENW/SCREENH·프레임 속도·전체화면 설정을 읽는다. 명시 실행 인자는 저장 설정보다 우선한다.
- **검증:** 인코딩·Unicode 왕복·쓰기 실패·명시 인자 우선 집중 3개와 Core Config 집중 10개가 통과했다. 실제 새 호스트에서 별도 데이터 복사본을 사용해 640×480 확인→옵션 파일 쓰기→재실행을 수행했으며 재시작 논리 캡처는 640×480, 두 실행 종료 0이다(`extracted/dotnet-options-114/`). 원본 사용자 설정은 수정하지 않았고 금지된 게임 진행 재현은 하지 않았다.
- **잔여:** Windows 활성 매핑 오류 1224의 cpppj 전용 대체 저장 경로는 미이식이다. DirectDraw 실패 표식, 음악·스테레오, 기본 실행 경로 전환, Guide·월드 및 전체 목표도 미완료다.

### 0.113 단위 기록 — 화면 모드 명령 연결 (2026-11-01, HJOW-Athlon)

- **구현:** Options의 Window Mode와 스크립트의 절대 화면 모드, 조정 미리보기·15초 확인 Tell, 확인된 전체화면 모드 기록을 실제 화면 장치와 Config에 연결했다. 이전 전체화면에서 다시 전체화면을 조정할 때 창 복원 크기를 덮어쓰지 않으며 잘못된 조합은 창 변경 전에 거부한다.
- **검증:** 실제 공통 메뉴 입력 큐에서 절대 명령·미리보기 XOR·확인 Tell 순서를 검사했다. 집중 테스트 1개 통과. 실제 새 호스트의 해상도 선택 경고와 전체화면→Window Mode를 확인했고 두 실행 모두 종료 0(`extracted/dotnet-screen-113/`). 마지막 유효성 검사는 해당 실행 이후 추가하고 집중 테스트를 다시 통과했다. 금지된 게임 진행 재현은 하지 않았다.
- **잔여:** SETSIZE 확인 뒤 options.cfg 실제 저장 및 재시작, DirectDraw 실패 표식 처리, 커서/렌더러의 나머지 모드 효과와 원본 픽셀 비교는 미완료다. 미션·월드 및 전체 목표도 미완료다.

### 0.112 단위 기록 — 실제 호스트 Options 메뉴 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj의 Options·해상도 여섯 개·음량 다섯 단계 메뉴를 실제 controlled Gump에 연결하고 체크 표시와 비활성 행을 현재 Config에서 구성했다. 토글/음량 선택은 같은 Config를 쓰고 효과음 재생 중지·현재 인스턴스 음량 변경을 장치에 반영한다.
- **검증:** 같은 Gump·큐에서 Auto-Demo 및 음량 행 선택과 설정·음향 갱신 호출을 검사했다. HostStartupScene 집중 18개와 확장 검사 1개 통과. 실제 새 호스트에서 Options 및 해상도 목록을 열어 캡처했고 두 실행 모두 종료 0(`extracted/dotnet-options-112/`). 금지된 게임 진행 재현은 하지 않았다.
- **잔여:** SETSIZE 확인 후 실제 해상도 변경·전체화면 명령, 설정 파일 저장, 음악 장치·좌우 스피커 반전·바람 소리, 원본과 픽셀/실제 PCM 비교는 미완료다. Guide·월드와 전체 목표도 미완료다.

### 0.111 단위 기록 — 실제 호스트 Help 팝업 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj의 상위 Help 팝업과 About NetStorm 행을 실제 공통 controlled Gump·입력 큐·다음 Tick Tell에 연결했다. 선택 명령은 큐의 원본 코드 마스킹을 적용한 값으로 소비한다. Options 등 미구현 상위 메뉴는 여전히 명시 경계에 둔다.
- **검증:** 실제 자산과 같은 Gump 트리에서 Help 버튼→행 선택→About Tell의 집중 검사 및 HostStartupScene 18개 통과. 첫 테스트 빌드의 Encoding 이름 누락과 큐 마스킹에 따른 명령 불일치를 수정했다. 실제 새 호스트에서 Help 팝업 및 About 창 캡처를 확인하고 두 실행 모두 종료 0이었다(`extracted/dotnet-help-111/`). 원본/cpppj 화면 비교는 하지 않았으며 About은 현재 Version v0.0으로 표시된다. 사제 포획·희생, 특정 적 공격, 승리를 재현하지 않았다.
- **잔여:** Help의 나머지 기능, Options, Edit, Guide/월드 연결과 전체 목표는 미완료다. 새 호스트는 아직 배포 기본 실행 경로가 아니다.

### 0.110 단위 기록 — 튜토리얼 초기화 조립 (2026-11-01, HJOW-Athlon)

- **구현:** 현재 Guide와 같은 옵션 87바이트·제한 표·State 요청에 Tutorial Init을 조립했다. 실제 요새 선택 뒤 Zone을 복제하고 다음 상태에 예약한 뒤 현재 미션의 기술 Header를 적용한다. 템플릿 정리·요새 선택·타입 조회·상태 예약은 호출자의 실제 소유자를 필수로 받는다.
- **검증:** 실제 배포 Tutorial1~6 스크립트와 타입 자산으로 Guide 교체, 초기화, State7 요청·Zone 복제·옵션·기술 제한 및 반납을 검사했다. 초기 테스트의 후반 미션 sunArcher 허용 추정이 틀려 cpppj에서 명시 검증한 첫 두 미션만 검사하도록 수정했다. 관련 집중 9개 통과. 실제 게임 실행은 하지 않았다.
- **잔여:** 현재 조립기는 검사에서만 사용한다. 제품 호스트의 실제 템플릿 관리자·요새 선택·상태 큐·타입 조회·Player 및 월드에 연결해야 한다. 캠페인·에디터·UI·멀티플레이 및 목표 전체는 미완료다.

### 0.109 단위 기록 — 미션 기술 허용 목록 이식 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `MissionTechnology`의 항목별 현재 목록 재조회, allow/deny/all, 실제256칸 중 앞255칸 전체 쓰기와 개별 타입 쓰기를 이식했다. 실제 MissionScript Config의 좁은 문자·512바이트 경계와 공유 제한 표를 사용하며 등록되지 않은 타입의 물리 범위 밖 쓰기는 거부한다.
- **검증:** 실제 원본 타입 자산을 이용해 독립 원본 55행의 전체 표와 호출 순서를 대조했다. 현재 MissionScript와 제한 표, 마지막 보존 칸 및 미등록 타입 거부를 포함한 집중 2개 통과. 첫 빌드의 MissionScript 이름 충돌을 명시 형식으로 수정했다. 게임 실행은 하지 않았다.
- **잔여:** 실제 Client 타입 조회·제한 표를 초기화 Header와 호스트에 장기 연결하고 요새 State7 및 월드를 설치해야 한다. 목표 전체는 미완료다.

### 0.108 단위 기록 — Guide와 Tutorial Init 상태 공유 (2026-11-01, HJOW-Athlon)

- **구현:** Tutorial Init이 Guide의 실제 flags·demo와 반납이 사용하는 missionNumber·Gate tutorialNumber를 사본 없이 쓰도록 결합했다. Guide 소유자의 초기화는 현재 MissionScript의 Config 읽기/쓰기를 사용하고 외부 템플릿·옵션·요새·Header 효과를 실제 순서로 호출한다. 초기화 지역과 원래 이름은 같은 Runtime 수명에 둔다.
- **검증:** 실제 MissionScript를 사용하는 생성→초기화→반납에서 현재 전역·Config와 효과 순서를 검사했고, 기존 원본 152행을 포함한 집중 4개가 통과했다. 게임 실행은 하지 않았다.
- **잔여:** 호스트가 Guide 입력과 초기화 단계를 호출하고 실제 템플릿·옵션·요새 요청·기술 표·State7 및 월드를 소유해야 한다. 현재는 제품 호스트에 설치되지 않아 플레이할 수 없다. 전체 목표는 미완료다.

### 0.107 단위 기록 — 미션 요새 요청 이식 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `MissionFortRequest`의 요새 선택, 현재 주소·포트의 State7 요청 쓰기, 512바이트 로컬 Zone 작성과 소유 복제, 복제 뒤 현재 순번 증가를 이식했다. 지연 State 실행 자체는 수행하지 않는다.
- **검증:** 독립 원본 144행에서 144바이트 요청과 양쪽 이웃, Zone 512바이트, 86바이트 복사본, 효과 순서와 콜백 중 전역 변경을 대조했다. 복사본 수명·중복 복제 거부를 포함한 집중 검사 2개가 통과했다. 게임 실행은 하지 않았다.
- **잔여:** 실제 호스트의 장기 State·주소·포트·요새 선택·취소 큐 및 State7/월드에 연결해야 한다. 로컬 로그인과 전송, 멀티플레이 및 목표 전체는 미완료다.

### 0.106 단위 기록 — 미션 전투 옵션 초기화 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `ResetMissionBattleOptions`의 10.78 87바이트 초기값과 예약 40바이트·양쪽 이웃 보존을 이식했다. Tutorial Init의 검사에서 실제 초기화를 사용한다.
- **검증:** 옵션 초기화 원본 296행 중 초기화 R/P 264행의 전체 저장소와 이웃, Tutorial Init 원본 152행의 87바이트까지 비교했다. 입력 계약 거부를 포함한 집중 3개 통과. 나머지 F 32행의 파생 float 경로 및 옵션의 파생 계산은 미이식이다. 실제 게임을 실행하지 않았다.
- **잔여:** 옵션 저장소를 실제 호스트에 장기 소유시키고 Tutorial Init과 요새 요청·월드에 연결해야 한다. 목표 전체는 미완료다.

### 0.105 단위 기록 — Tutorial Init 이식 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `TutorialInit`의 현재 전역·Config 읽기, ASCII 이름 정규화와 80바이트 지역, 구독자 복사, 요새 요청·기술 Header 및 flags 순서를 이식했다. 도달한 효과가 없으면 오류로 중단한다.
- **검증:** 독립 x86 fixture 152행의 상태·지역 문자열·이름·호출 순서를 집중 검사했다. 옵션 87바이트는 아직 실제 초기화 효과가 연결되지 않아 비교하지 않았다. 게임 실행과 사제 포획·희생, 특정 적 공격, 승리 재현은 하지 않았다.
- **잔여:** 실제 호스트 Guide·Player·템플릿·옵션·기술 제한·요새 요청과 월드 설치를 연결해야 한다. 메뉴 시작은 여전히 미연결이며 목표 전체는 미완료다.

### 0.104 단위 기록 — Tell 파일 목록과 캠페인 선택 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `TellFileMenus`의 @/#/% 경로를 이식해 파일별 장기 Config 등록·첫 경로/index·prefix 정렬, 각 행의 `cur.file`, 제목/구분선, 문자열 고리와 inline 두 번 치환·오류 복구를 실제 HTML 메타에 연결했다. 실제 파일 탐색은 기본 디스크→기본 TAFF→보조 디스크→보조 TAFF 순서와 첫 이름을 유지하고, 파일 읽기의 보조 디스크/TAFF 우선순위도 바로잡았다. 생성한 Config는 호스트 종료 때 반납한다.
- **검증:** 원본 fixture @/# 384행, inline 288행, 오류 672행을 대조했다. Client 집중 38개와 Assets 파일 시스템 집중 2개 통과. 첫 파일 시스템 검사는 폴더 경로의 대소문자 보존이 달라 1실패했고, FileSpec의 입력 폴더 표기를 따르도록 수정해 통과했다. 실제 호스트에서 Campaign→Early Missions 및 Campaign→Priest Training의 목록 캡처와 종료 0을 확인했다. 첫 미션 행은 `0200008b` Guide Begin 미연결로 종료1을 확인했으며 게임 진행이나 사제 포획·희생, 특정 적 공격, 승리를 재현하지 않았다.
- **잔여:** FileFinder의 절대 경로와 Win32 와일드카드 세부 동작·아카이브 등록 순서는 별도 원본 대조가 필요하다. 첫 미션 명령을 실제 Guide/세션에 연결해야 하며, 데모 시작·Edit·다른 메뉴·HUD·멀티플레이 사전 구현 및 전체 동일성은 미완료다. cpppj 화면 캡처 비교는 하지 않았다.

### 0.103 단위 기록 — 실제 메뉴 행 입력·원본 커서 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `RawMenuControls`와 `NotifyPointer`를 따라 같은 공통 Gump에 메뉴 행 선택·강조·명령/등록 ID·닫힘 통지, 프레임 포인터/overlay 및 이름별 효과음을 연결했다. 배포된 원본 18개 커서 비트맵을 SDL 핸들로 생성해 본문·링크 포인터에 적용하고 종료 시 반납한다. 호스트 입력은 큐 전에 포인터 통지를 한다.
- **검증:** 호스트·메뉴 입력·스크립트 집중 62개 통과. 실제 Credits 첫 메뉴 행의 선택·명령 고리·기존 창 정리·다음 Tell을 검증했다. 실제 호스트에서 Credits 첫 행을 눌러 `NetStorm 10.72 Patch Credits` 화면 캡처와 종료 0을 확인했다. 실제 PCM 소리는 확인하지 않았다. 사제 포획·희생, 특정 적 공격, 승리는 재현하지 않았다.
- **스크립트 교정:** 첫 실제 실행은 포인터 이동 시 커서 장치 미연결로 종료1이었고 원본 커서 연결 뒤 재실행해 통과했다. 이어서 Cancel 회귀에서 창이 보존된 듯 보였으나, 그리기가 밀린 Update에서 앞선 snapshot이 입력 위치를 막아 Credits 클릭 자체가 건너뛰어진 것이었다. 입력/관측 위치를 분리하고 놓친 입력을 여전히 차단하는 회귀 검사를 추가했다. 수정 뒤 실제 실행에서 시작→열림은 36,972픽셀 차이, 시작→Cancel은 파일 바이트 일치, 종료0이다. 앞선 0.102의 Cancel 비교만으로는 열림을 입증하지 못했던 점을 정정한다.
- **잔여:** 모든 Credits 쪽·Help/Options/Edit·캠페인/데모 목록 및 미션·Guide 연결, HTML 그림 자산, cpppj 화면 비교, HUD·멀티플레이 사전 구현은 미완료다. 원본 커서의 실제 물리 표시·음향 장치는 별도 비교하지 않았다.

### 0.102 단위 기록 — 실제 메인 입력 큐·Tell 연결 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `ActivatePageAction`·`Event`·`Tick`을 따라 실제 메인 Gump 입력, 한 칸의 다음 Tick Tell, HTML 일반 Tell/Config/DoNothing/MenuJustClosed, 메인 종료 요청·반환을 같은 입력 큐와 문자열 고리에 연결했다. 삭제 요청과 공통 Cleanup을 분리하고 호스트 프레임에서 Cleanup→Idle을 호출한다. 미구현 상위/일반 명령은 명시 경계로 남긴다.
- **검증:** 호스트·입력 집중 31개 통과. 실제 배포 Credits 버튼→다음 Tick 모달 생성→버튼 클릭·지연 정리·화면 복원, Config 쓰기와 메인 종료 큐를 확인했다. 실제 `Netstorm.Client.Host.exe` UI 스크립트로 Credits BMP를 확인했다. 당시 Cancel 비교는 열기 중간 캡처의 차이를 확인하지 않아 클릭 누락 가능성이 있었으며 0.103에서 스크립트 결함을 수정해 열림 차이와 닫힘 일치를 다시 확인했다. 사제 포획·희생, 특정 적 공격, 승리는 재현하지 않았다.
- **잔여:** Credits 내부 메뉴 항목은 입력 미연결이다. Campaign·Demo의 맵/파일/기술 목록과 미션, Help/Options/Edit 상위 명령, Guide 우선 분배·링크, 종료 효과음/상태와 기타 게임 명령은 남는다. cpppj와 화면 비교는 수행하지 않았고 전체 시나리오·HUD·멀티플레이 사전 구현과 원본 동일성은 미완료다.

### 0.101 단위 기록 — 실제 Gump 수집·정렬·출력 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `RawMenuControls::CollectDrawing/WithDrawing`처럼 실제 공통 트리의 표시 목록을 5,000칸에 수집하고 0.100 원본 정렬을 적용한 뒤 같은 화면·클립·글꼴 문맥에서 배경, 돌 버튼, 장식, HTML 본문, 슬라이더와 메뉴를 분배한다. 실제 호스트의 초기 및 dirty 출력에 연결하고 재그리기 때 타이틀 배경을 복원한다. 메뉴 체크 표시도 실제 fortGump J 프레임을 쓴다.
- **검증:** 호스트·정렬·Gump Draw 집중 47개 통과. 초기 여덟 버튼은 기존 실제 버튼 출력과 전체 픽셀이 같고, 실제 Credits 창의 생성·출력·닫기 뒤 픽셀 복원과 일반/체크 메뉴 출력도 확인했다. 실제 호스트 `--window --frames 10`의 프레임5 BMP가 생성되고 종료 코드 0이다. 사제 포획·희생, 특정 적 공격, 승리는 재현하지 않았다.
- **잔여:** 메뉴 명령과 큐 소비가 연결되지 않아 실제 클릭으로 Tell을 여는 경로는 아직 없다. HTML 그림 TYPE/자산, 다른 파생 Gump·월드 통합 표시, 원본 cpppj 화면 픽셀 비교, 메뉴 입력·목록과 전체 미션/편집·멀티플레이 사전 구현은 미완료다. 전체 화면 복원은 현재 시작 화면 경로이며 월드 dirty 경로가 아니다.

### 0.100 단위 기록 — 원본 표시 목록 CRT 정렬 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `DrawSort`와 `CompareDrawOrder`의 signed 깊이·float 좌표 비교, 10.78 중앙값 피벗과 CD 첫 칸 피벗·작은 구간 정렬을 C#에 이식했다. 동률도 1인 비교와 항목 전체 교환, 유한 좌표 선검사를 보존한다. 포트 맵은 스프라이트 wrapper가 아직 없으므로 두 쌍을 `port`로 등록했다.
- **검증:** 원본 관측 `drawsort-x86.tsv` 1,155행(10.78 325, CD 415, 10.37 415)의 전체 16바이트 항목 순서와 wrapper 생략을 대조했다. 정렬 및 fixture 등록 집중 5개 통과. 최초 빌드는 공유 `X86Fixture` 네임스페이스 누락으로 실패했고 수정 후 통과했다. 창·게임·전체 검사는 실행하지 않았다.
- **잔여:** 실제 Gump 수집 결과와 픽셀 분배에 정렬을 아직 연결하지 않았고 `SortRenderSprites` wrapper는 미이식이다. HTML 표시·입력, 미션·Edit·HUD와 멀티플레이 사전 구현 및 전체 동일성은 미완료다.

### 0.99 단위 기록 — 실제 호스트 HTML 전체 창 생성기 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `RawMenuHtml`·`LoadingHtmlContext`에 따라 실제 설정 치환과 원본 BYTE 변환, 창 할당·두 번 파서·메타·본문·Fit·모달·큐·공유 문자열 고리를 전체 생성기에 연결하고 실제 호스트에 설치했다. `ShowTell`은 같은 Config의 Tell 섹션을 조회해 생성한다. 현재 메뉴 전역은 호스트에서 미션/편집 연결 전 0을 제공하며 맵·파일·기술 등 선택 목록 후크는 미연결로 남긴다.
- **검증:** 호스트·전체 창·Tell 집중 34개 통과. 실제 Credits 본문이 모달과 본문·버튼을 만들고 Enter로 닫혀 큐와 자식 정리를 거치는 경로, 일반 메뉴 메타가 같은 창에 부착되는 경로를 검사했다. `Netstorm.Client.Host.exe --window --frames 10` 종료 코드 0. 실제 Tell 화면 캡처·메인 메뉴 입력·자연 플레이는 검증하지 않았다.
- **잔여:** 상위 Tell 명령과 입력 큐 소비·전체 Gump 그리기·메뉴 입력·목록 공급·링크 실행은 아직 연결되지 않았다. 실제 클릭으로 Credits를 볼 수 있다는 뜻이 아니며 전체 목표와 멀티플레이 사전 구현은 미완료다.

### 0.98 단위 기록 — 실제 호스트 HTML 메뉴 열 조립 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `RawMenuControls`·`RawMenuHtml`에 따라 같은 공통 Gump 트리, 실제 fortGump·팔레트·글꼴, 본문과 공유하는 StyleText 조각 및 Config 색 조회로 controlled 메뉴 열을 조립했다. HTML Fit의 현재 메뉴/부착을 연결하고, 메타 항목 작성에 같은 메뉴 장치를 제공하며 부착 전 실패한 메뉴도 스택에서 빼고 자식 우선 정리한다. 실제 호스트가 장치를 생성한다.
- **검증:** 호스트·HTML 창·공장 집중 39개 통과. 실제 `$Menu`·`$Checked`의 항목, 명령/문자열, 크기, 부모 부착, 소유 조각과 미부착 메뉴 반납을 검증했다. 첫 빌드는 Config ReadInt의 bool 반환을 void 대리자에 직접 연결해 실패하여 수정했다. 첫 검사 38통과·1실패는 시험 문자열의 명령 인자 뒤 구분 공백이 없어 원본 정규식이 줄바꿈을 포함한 것이며 구분 공백을 넣고 재실행해 39통과했다. 실제 `Netstorm.Client.Host.exe --window --frames 10`은 종료 코드 0이다. 사제 포획·희생, 특정 적 공격, 승리는 재현하지 않았다.
- **잔여:** HTML 전체 공장과 메타 소유자, 맵/파일 목록, 메뉴 입력·그리기, 큐 및 상위 명령 소비는 미연결이다. Tell 표시와 미션 플레이는 완성되지 않았으며 전체 목표와 멀티플레이 사전 구현도 미완료다.

### 0.97 단위 기록 — 실제 호스트 HTML 파서·명령 예약 (2026-11-01, HJOW-Athlon)

- **구현:** 실제 `ConfigInterface.SubstituteLocal`을 HTML 조건에 연결하고 창 소유자의 공통 노드와 같은 두 번 파서·링크 주소 수명을 조립했다. 원본 BYTE와 Config의 유니코드는 Windows-1252 경계에서 변환한다. cpppj의 `DialogScriptCode`와 하위 이름표를 따라 HTML 명령을 현재 입력 큐와 공유 문자열 고리에 예약하고 창 반납 때 링크 별칭을 제거한다. 실제 호스트에서 파서를 설치한다.
- **검증:** 호스트·HTML 파서·창·Guide 집중 29개 통과. 실제 설정 변경 뒤 조건 재평가, 명령 큐의 코드·문자열, Kill 뒤 Cleanup 전 링크 유효 및 반납 뒤 무효를 확인했다. GUI·전체 검사는 재실행하지 않았다.
- **잔여:** 전체 HTML 공장·메타 메뉴·상위 명령/입력 소비·링크 실행과 통합 그리기는 연결되지 않았다. 새 파서가 설치됐다는 사실은 Tell 창이 표시되거나 미션 플레이가 가능하다는 뜻이 아니다. 전체 목표와 멀티플레이 사전 구현도 미완료다.

### 0.96 단위 기록 — 기존 경로 실행 파일 이름과 배포 (2026-11-01, HJOW-Athlon)

- **구현:** 기존 `Netstorm.Game` 어셈블리 이름을 `Netstorm`으로 바꾸고 배포 스크립트, README, 현행 설명과 16개 클론 스모크 도구의 실행 경로를 갱신했다. Linux 산출물 이름도 `Netstorm`이다. 원본과 이름이 같으므로 도구는 실행 파일 전체 경로를 쓴다.
- **검증:** `dotnet test dotnetpj/Netstorm.sln -c Release` 2,074개 통과(Assets 364, Core 1,055, Client 655), 실패·건너뜀 0. win-x64 프레임워크 종속 ZIP 1,376항목을 생성하고 압축 해제한 `NetstormReborn-dotnetpj/Netstorm.exe`를 창 모드로 짧게 시작해 캡처 생성과 종료 코드 0을 확인했다. 처음 압축 해제본 실행 명령은 ZIP 최상위 폴더를 빠뜨려 실패했으며 올바른 경로로 재실행했다. 사제 포획·희생, 특정 적 공격, 승리 재현은 실행하지 않았다.
- **잔여:** 이는 기존 근사 경로의 이름 변경이다. 새 호스트의 명령 소비·HTML 공장·월드 조립, 모든 시나리오·Edit·HUD·멀티플레이 사전 구현과 원본 동일성은 미완료다. `cpp_sync.py`는 연결 fixture 134개 일치를 보고했지만 반영 기준 이후 cpppj 변경 영향은 전체 검토하지 않았으므로 기준 커밋을 갱신하지 않았다.

### 0.95 단위 기록 — 실제 호스트 HTML 창 배치 (2026-11-01, HJOW-Athlon)

- **구현:** 기존 `HtmlWindowFit`을 실제 호스트의 공통 트리·본문·돌 버튼·논리 화면과 메뉴 뷰에 연결했다. 현재 화면/뷰를 매번 조회하며 본문과 버튼을 재배치하고 원본 깊이로 맞춘다. 메뉴 열 스택은 아직 연결하지 않아 해당 분기는 명시 오류로 남는다.
- **검증:** 실제 배포 글꼴·자산으로 두 버튼과 본문을 배치해 화면 중앙, 버튼 너비와 깊이, 본문 교체 후 재배치 및 자식 반납을 검증했다. 시작 화면·HTML 배치·공장 집중 30개 통과. GUI·화면 비교·전체 Release 재실행은 하지 않았다.
- **남은 작업:** HTML 메타의 메뉴 열/목록, 실제 HTML 공장과 링크, 상위 명령·입력 큐 소비, 창 전체 그리기 정렬, 본문 그림 TYPE/자산·커서/음향을 연결해야 한다. Tell 창의 실제 GUI 표시, D1과 플레이는 미완료다. 새 fixture·포트 맵 변경은 없으며 cpppj와 공통 파일은 수정하지 않았다. 사용자가 현재 단위 완료·커밋 뒤 목표 일시 중지를 요청했다.

### 0.94 단위 기록 — HTML 본문 실제 글리프 출력 (2026-11-01, HJOW-Athlon)

- **구현:** `HostHtmlText`의 기존 다중 줄 조각 Draw 경계를 공통 트리의 실제 파일 글꼴·현재 논리 화면 픽셀·Gump 클립에 연결했다. 본문 출력 동안만 화면과 글리프 문맥을 보유하고 종료/예외 시 반납하며, 출력 뒤 해당 영역을 갱신한다. 그림 조각의 자산/출력과 창 전체 정렬은 아직 미연결이다.
- **검증:** 실제 배포 글꼴로 두 줄 본문을 생성·배치·출력하여 영역 안 픽셀 변경과 영역 밖 픽셀 보존, 자식 및 조각 반납을 확인했다. 시작 화면·다중 줄·글리프 집중 49개 통과. GUI·cpppj 픽셀 비교·전체 Release 재실행은 하지 않았다.
- **남은 작업:** 본문 출력은 개별 호출 경계만 준비했으며 실제 호스트 프레임에서 HTML 전체를 그리지 않는다. 그림 TYPE/자산, 커서/이름 소리, 전체 Fit·메뉴/HTML 공장·링크·명령 소비·Program 입력 및 출력 정렬이 남는다. D1과 플레이는 미완료다. 새 fixture·포트 맵 변경은 없으며 cpppj와 공통 파일은 수정하지 않았다.

### 0.93 단위 기록 — 실제 호스트 HTML 본문·슬라이더 조립 (2026-11-01, HJOW-Athlon)

- **구현:** 새 호스트의 HTML 창 소유자에 실제 파일 글꼴, 스타일 조각/버퍼, 다중 줄 본문과 공통 트리의 Slider·화살표·손잡이를 연결했다. 원본 사건 코드와 배경 타입을 사용하고 본문·슬라이더의 가상 부착·입력·수명 분배를 설치했다. 생성 도중 미부착 Slider 자식도 HTML 소유자가 추적한다.
- **검증:** 실제 배포 글꼴로 두 줄 본문을 파싱·배치하고 Slider 및 세 자식, 전체 등록 반납과 조각 정리를 검증했다. 시작 화면·공통 트리·다중 줄 집중 53개 통과, 호스트 Release 빌드 통과. GUI·본문 픽셀 출력·화면 비교·전체 Release 재실행은 하지 않았다.
- **남은 작업:** 실제 본문 Draw·그림 TYPE/자산·설정 옵션·커서/이름 소리, 전체 Fit·메뉴/HTML 공장·링크·명령 소비·Program 입력 및 출력 정렬이 미연결이다. 부분 생성 중 돌 버튼 내부 배경 할당 실패의 모든 사례도 아직 검증하지 않았다. 현재 Tell 창은 GUI에 표시되지 않고 D1/플레이는 미완료다. 새 fixture·포트 맵 변경은 없으며 cpppj와 공통 파일은 수정하지 않았다.

### 0.92 단위 기록 — 실제 호스트 HTML 부모·버튼 소유자 (2026-11-01, HJOW-Athlon)

- **구현:** 새 Client.Host가 메뉴와 같은 Gump 트리·실제 입력 큐·벽 시각을 사용하는 `HostHtmlWindows`를 소유한다. 실제 장식/돌 버튼 생성, 공통 모달·사건 분배, 원시 명령 큐, 등록된 창 및 부분 생성 객체의 정리를 연결했고 종료 시 반납한다. 본문·링크·전체 배치 장치는 아직 연결 전이므로 해당 경로를 성공으로 위장하지 않는다.
- **검증:** 실제 자산의 호스트 메뉴 트리에서 HTML 부모와 돌 버튼을 생성해 입력→원시 큐→모달 해제→자식 우선 정리 및 미연결 본문 부분 생성 뒤 반납을 확인했다. 시작 화면·공통 트리·HTML 공장 집중 29개와 호스트 Release 빌드 통과(경고·오류 0). GUI·실제 음향·화면 비교·전체 Release 재실행은 하지 않았다.
- **남은 작업:** 본문/스크롤·전체 배치·HTML 공장/메타·링크 명령과 현재 메뉴 상위 명령 분배, Program의 입력 소비 및 정렬/출력을 실제 호스트에 이어야 한다. 아직 Tell 창을 표시할 수 없으며 D1과 플레이도 미완료다. 새 fixture·포트 맵 변경은 없으며 cpppj와 공통 파일은 수정하지 않았다.

### 0.91 단위 기록 — 실제 fortGump 장식 생성과 출력 (2026-11-01, HJOW-Athlon)

- **구현:** 시작 메인 메뉴의 실제 `fortGump` 타입·SHP와 같은 공통 트리, 배경 생성기·파일 팔레트로 HTML 파생 창의 배경과 I 프레임 네 모서리를 생성하고 8비트 논리 표면에 그리는 `HostGumpDecoration`을 연결했다. 공통 트리와 장식 장치를 메인 메뉴 소유자에서 후속 HTML 조립에 제공한다.
- **검증:** 실제 배포 자산의 HTML 파생 노드를 생성하고 원본 타입·I 시작 프레임을 확인했다. 실제 화면의 네 모서리 부근 픽셀 변경과 등록 해제를 검증했다. 시작 화면·공통 트리·장식 집중 20개 통과. GUI·음향·cpppj와의 화면 비교·전체 Release 재실행은 하지 않았다.
- **남은 작업:** 실제 Program에 HTML 창 생성기·본문/메뉴/정렬/명령 소유자와 입력 큐를 조립해야 한다. 현재 장식 생성/출력은 검사에서 사용했으며 열린 HTML 창을 화면에 자동 표시하지 않는다. 플레이와 D1은 미완료다. 새 fixture·포트 맵 변경은 없으며 cpppj와 공통 파일은 수정하지 않았다.

### 0.90 단위 기록 — HTML 파생 노드의 실제 공통 트리 연결 (2026-11-01, HJOW-Athlon)

- **구현:** 호스트의 `GumpButtonTree`가 같은 실제 노드 저장소에서 파생 객체를 할당·등록 전 반납하고 serial을 조회하며 기존 트리·사건·입력·프레임·수명 가상 장치와 배경 생성기를 추가 소유자에게 제공한다. 등록됐거나 생성/연결된 노드를 임시 할당 해제 API로 반납하는 것은 거부한다. 기존 버튼과 모달은 같은 객체를 계속 사용한다.
- **검증:** 같은 실제 트리에 HTML 부모·장식과 돌 버튼을 생성하고 가상 사건/소멸 처리를 연결했다. 버튼을 눌러 HTML 원시 명령과 모달 해제, 자식 우선 정리·serial 반납을 확인하면서 기존 메인 버튼은 유지했다. 버튼·HTML 공장·시작 화면 집중 27개 통과. GUI·실제 음향·화면 비교·전체 Release 재실행은 하지 않았다.
- **남은 작업:** 위 HTML 연결은 검사 조립이다. 실제 Program에 HTML 소유자·본문/메뉴/출력/명령 장치를 조립하고 입력 큐 및 Tell에 연결해야 한다. D1과 플레이는 미완료다. 새 fixture·포트 맵 변경은 없으며 cpppj 파일과 공통 파일은 수정하지 않았다.

### 0.89 단위 기록 — 공통 버튼 트리 모달과 실제 Tell 연결 검사 (2026-11-01, HJOW-Athlon)

- **구현:** 이미 이식된 `GumpModal`을 새 호스트의 `GumpButtonTree`에 연결해 같은 노드·dialog 표·입력·수명을 공유하고 모달 변경을 화면 갱신 경계에 알린다. 이 단계는 HTML 노드나 창 공장을 호스트에 설치하지는 않는다.
- **검증:** 실제 버튼 트리의 중첩 모달 깊이, 자식 깊이 이동, 중복 등록, 입력 제한, 이전 모달 복구와 자식 우선 삭제를 검사했다. 별도 헤드리스 HTML 조립에서 실제 배포 Credits Tell과 Config를 전체 파서·본문·모달까지 통과시키고 반납을 확인했다. 버튼·입력·HTML 집중 검사 61개 통과. GUI·음향 출력·화면 비교·전체 Release 재실행은 하지 않았다.
- **남은 작업:** 헤드리스 HTML 소유자의 노드 할당·장식·본문·메뉴·그리기·명령 장치를 실제 공통 트리와 호스트 Config에 조립하고, Program 입력 및 상위 명령에 연결해야 실제 GUI가 동작한다. D1은 미완료다. cpppj 파일과 공통 파일은 수정하지 않았다.

### 0.88 단위 기록 — 실제 Config Tell 인자와 섹션 선택 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj `TellParameters`·`TellSection`의 원본 토큰, 인자의 순차 치환, 임시 param 등록 수명, 현재/이전 이름과 등록 객체→미션→매번 새 tell 파일 조회를 같은 `ConfigInterface`에 이식했다. 시작 화면의 같은 클라이언트 수명에 Tell 상태를 두고 호출 중 HTML 창 생성 경계를 제공한다. 실제 창 공장은 아직 연결되지 않았다.
- **검증:** 새 fixture `tell-parameters` 1,070행과 `tell-section` 858행을 각 원본 관측에 대조하고 목록에 등록했다. 실제 배포 자산에서 Campaign·Demo·Credits 버튼 입력을 검사 안에서 Tell 조회와 대체 창 생성 콜백까지 연결해 세 본문을 확인했다. 실제 Config 우선순위·빈 본문·param 수명 등 포함 집중 검사 14개 통과, 호스트 Release 빌드 통과. 실제 GUI·HTML 화면·전체 Release 재실행은 하지 않았다. 포트 맵 행은 추가하지 않았다.
- **남은 작업:** 실제 Program의 입력 소비·상위 명령 분배와 HTML 노드/파서/모달/그리기를 공통 트리에 조립해야 메뉴가 작동한다. 창 콜백은 검사 대역으로만 연결했으므로 GUI 플레이나 D1 완료가 아니다. cpppj 파일과 공통 파일은 수정하지 않았다.

### 0.87 단위 기록 — 실제 호스트 포인터 폴링 (2026-11-01, HJOW-Athlon)

- **구현:** `Client::Poll`과 `app/main.cpp` 스크립트 우선 경계에 맞춰 호스트의 마우스·키 상태, 비활성·화면 밖 판정, 논리 좌표 제한과 스크립트 포인터 우선 조회를 버튼 트리에 연결했다. 누름 이력은 MonoGame 프레임 입력 펌프가 관측한 전이를 폴링까지 보관한다. 따라서 프레임 사이에 눌렀다 뗀 물리 입력까지 감지하는 Win32 `GetAsyncKeyState`의 정확한 이력은 아직 재현하지 못한다.
- **검증:** 좌표·화면 경계·스크립트 선택과 프레임에서 포인터 전달을 포함한 호스트·스크립트·시작 화면 집중 31개가 통과했다. 새 호스트도 검사 빌드에서 컴파일됐다. 전체 Release 재실행, 실제 GUI·음향·화면 비교는 하지 않았다.
- **남은 작업:** Program은 여전히 입력 큐 소비자와 상위 메뉴 명령 소비자·하위 창을 설치하지 않아 버튼은 실제로 작동하지 않는다. 프레임 사이의 빠른 물리 입력 이력, 공통 음향·음악과 D1도 남는다. cpppj 파일과 공통 파일은 수정하지 않았다.

### 0.86 단위 기록 — 메뉴 시계와 버튼 음향 경계 (2026-11-01, HJOW-Athlon)

- **구현:** 새 MonoGame 호스트의 버튼 트리에 단조 실시간 시계와 `sound/button.wav` 재생 경계를 연결했다. 실제 파일 시스템·설정의 sound, soundVolume, maxSimulSounds를 읽으며 `ClientAudio::VolumeFromStep`의 데시벨 단계를 선형 진폭으로 변환한다. MonoGame이 요구하는 최소 표본율을 맞추고 장치·파일 오류 시 실행을 유지하며 재생 인스턴스를 종료할 때 해제한다. 이는 공통 음향 시스템 전체가 아닌 메뉴 버튼 전용 경계다.
- **검증:** 새 호스트 Release 빌드 경고·오류 0, 메뉴·버튼 관련 기존 집중 검사 2개 통과. `cpp_sync.py`는 완료했고 `--ports`는 완료 전에 중단해 결과를 확인하지 못했다. 기존 fixture 반영 기준은 바꾸지 않았다. 전체 Release, 실제 음향 출력·클릭, GUI 실행과 화면 비교는 하지 않았다.
- **남은 작업:** Program은 여전히 버튼 입력 소비자를 설치하지 않았고 포인터 폴링·상위 명령 소비자·하위 창도 미연결이다. 따라서 위 음향은 실제 메뉴 입력에서 아직 실행되지 않으며 전체 음향·음악, 옵션 변경 중 재생 관리와 D1도 미완료다. cpppj 파일은 수정하지 않았다.

### 0.85 단위 기록 — 실제 메인 메뉴 라벨과 스크립트 입력 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj HEAD `5fb44d9d`의 `UberGump::ControlPoint`에 따라 같은 실제 Gump 경계의 첫 일치 라벨 중심을 자동 입력에 제공한다. 스크립트 실행기 생성 뒤 현재 메뉴가 설치되므로 늦게 조회기를 연결하고, 실제 Client.Host가 메인 메뉴 생성 직후 이를 등록한다. 현재는 메인 메뉴 여덟 버튼만 조회하며 이름은 대소문자를 구별한다.
- **검증:** 실제 배포 자산·Config·Gump와 호스트 프레임/스크립트 큐를 연결해 `click Campaign`이 눌림·해제를 거쳐 `Tell,UCampaign`에 도달하고 `click-if` 없는 라벨은 건너뛰는 것을 확인했다. 호스트·스크립트 집중 28개 통과. 새 fixture·포트 맵 변경, 화면 비교, 은퇴 코드·검사는 없다. 전체 Release는 0.74의 2,043개가 마지막이다.
- **남은 작업:** 실제 Program은 버튼 입력 소비자를 설치하지 않았고 시계·음향·부모 명령 소비자·하위 창이 미연결이다. 따라서 현재 GUI에서 라벨 위치는 찾지만 눌림은 실행되지 않는다. 하위 메뉴의 현재 조회·report·kernel-tell도 남는다. GUI 실행과 디컴파일은 하지 않았으며 cpppj 미커밋 변경은 근거로 쓰지 않았다.

### 0.84 단위 기록 — 메인 메뉴 버튼 입력과 명령 분배 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj HEAD `4979ee23`의 `UberGump.cpp` 메인 메뉴 순서에 따라 같은 돌 버튼 트리의 실제 캡처 사건과 serial을 여덟 상위 명령(`Tell`·`Menu`·`AskEditFort`·`RequestQuitApp`) 및 인자로 연결했다. 클릭 외의 부모 사건은 기존 실제 처리 경계를 보존하고, 상위 명령 소비자가 없으면 선택 시 명시 오류다.
- **검증:** 실제 배포 자산의 여덟 버튼을 눌렀다 놓아 각 명령/인자·소리 호출과 버튼 밖에서 놓은 취소를 확인했다. 기존 돌 버튼 검사를 포함한 Client 집중 16개 통과. 새 fixture·포트 맵 변경, 화면 비교, 은퇴 코드·검사는 없다. 전체 Release는 0.74의 2,043개가 마지막이다.
- **남은 작업:** Program은 아직 이 Route를 입력 큐에 붙이지 않았고 실제 포인터 폴링·시계·음향·상위 명령 소비자·하위 창도 미연결이다. 따라서 실행 화면의 버튼은 여전히 동작하지 않는다. GUI 실행과 디컴파일은 하지 않았으며 cpppj 미커밋 변경은 근거로 쓰지 않았다.

### 0.83 단위 기록 — 실제 메인 메뉴 버튼 표시 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj HEAD `3f39d0a3`의 `UberGump.cpp`와 `RawButtonControls.cpp`에 따라 실제 `fortGump` 타입/A01 SHP·원본 비트맵 글꼴·파일 팔레트와 0.82 공통 돌 버튼 트리를 연결했다. 메인 메뉴 여덟 버튼을 원본 4열 2행 좌표에 타이틀 위로 출력한다. 노드 redraw 요청은 호스트가 다음 화면 갱신으로 보관한다.
- **검증:** 실제 배포 자산으로 여덟 버튼 각각의 출력 영역을 확인하는 검사 포함 집중 15개 통과. HJOW-Athlon에서 새 Client.Host를 창 모드 3프레임 실행해 종료 0, 프레임 2 논리 BMP에서 타이틀·구름·버튼 여덟 개를 육안 확인했다. cpppj/원본과 픽셀 비교는 하지 않았다. 새 fixture·포트 맵 변경, 은퇴 코드·검사는 없다. 전체 Release는 0.74의 2,043개가 마지막이다.
- **남은 작업:** 이 단계는 **표시만** 한다. 입력 소비자·실제 포인터/시계/음향·부모 메뉴 명령·하위 화면과 공통 정렬은 미연결이므로 여덟 버튼은 동작하지 않는다. 동작하는 메뉴와 D1 완료가 아니다. 구름 애니메이션·월드도 미연결이다. 원본 게임 실행·디컴파일은 하지 않았으며 cpppj 미커밋 변경은 근거로 쓰지 않았다.

### 0.82 단위 기록 — 공통 돌 버튼 트리 조립 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj HEAD `97d32749`의 `RawButtonControls.cpp` 공통 부분에 맞춰 Gump 등록표·루트·노드·수명·캡처·입력·idle을 하나의 저장소로 묶고 여러 StoneButton과 배경 자식의 생성·측정·부착·입력·삭제를 연결했다. 같은 배경 자산·글꼴·8비트 표면에서 배경과 부모의 선·글리프 출력도 호출할 수 있다. 시계·폴링·음향·redraw·부모 사건은 실제 호스트가 명시적으로 제공한다.
- **검증:** 돌 버튼 집중 검사 12개 통과(신규 2개). 두 버튼의 실제 캡처·부모 통지·개별 삭제와 같은 노드의 배경/글리프 픽셀 출력을 확인했다. 최초 Draw 검사는 검사 글꼴 폭이 99여서 화면 밖으로 그려져 실패했고, 의도한 2픽셀 폭으로 입력을 수정한 후 통과했다. 중간 빌드는 색표 필드 누락으로 실패 후 수정됐다. 새 fixture·포트 맵 변경, 화면 비교, 은퇴 코드·검사는 없다. 전체 Release는 0.74의 2,043개가 마지막이다.
- **남은 작업:** 현재 트리는 실제 메인 메뉴·호스트에 설치되지 않았다. 부분 생성 예외 시 소유자 정리, 실제 자산/글꼴/시계/음향, 전체 공통 정렬·메뉴 부모와 클릭 명령의 소비를 연결해야 한다. 이 단위는 메뉴 표시나 플레이 가능을 뜻하지 않는다. 게임/창 실행·디컴파일은 하지 않았고 cpppj 미커밋 변경은 근거로 쓰지 않았다.

### 0.81 단위 기록 — 호스트 시작 Config 설정 전달 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj HEAD `d209023b`의 `app/main.cpp`·`ClientMain.cpp`에 따라 새 호스트의 `--set` 현재 설정을 실제 `ConfigInterface.Startup` 명령 인자로 전달한다. 게임 데이터의 설정·팔레트·타이틀 경로도 같은 Config를 사용한다. 기존 C# 호스트의 반복 --set 병합 및 같은 이름 마지막 값 계약은 유지한다(cpppj의 반복 --set은 마지막 묶음만 보관하므로 이 차이는 미해결).
- **검증:** 실제 배포 자산으로 시작 Config가 설정 파일보다 지정된 `tellTips`·`maxFPS`를 우선하는 통합 검사 포함 호스트 집중 13개 통과. 새 fixture·포트 맵 변경, 화면 비교, 은퇴 코드·검사는 없다. 전체 Release는 0.74의 2,043개가 마지막이다. 게임/창 실행과 디컴파일은 하지 않았다.
- **남은 작업:** 메뉴 Gump·입력 소유자 조립, 실제 월드·프레임 효과와 구름 이동·부분 갱신은 여전히 미구현이다. 반복 --set의 cpppj 계약과 C# 기존 인자 계약 차이는 오라클 연결 시 정리한다. cpppj 미커밋 변경은 근거로 쓰지 않았다.

### 0.80 단위 기록 — 초기 Kencloud 자산 표시 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj HEAD `1bd953cd`의 `ClientKencloudGlobals.cpp`·`RawKencloudRender.cpp`를 기준으로 `CloudFileSpec`의 첫 descriptor 512×512 구름을 읽어 카메라 원점 0의 전체 dirty 초기 캐시를 8비트 화면에 반복 복사하고 타이틀을 그 위에 합성한다. 512×512 파일 부재·다른 크기의 원본 생성 fallback은 아직 이식하지 않아 명시 오류다.
- **검증:** 시작 화면·호스트 집중 검사 12개 통과. 실제 배포 자산에서 구름의 가로·세로 반복 픽셀을 확인했다. HJOW-Athlon에서 새 Client.Host를 창 모드 3프레임 실행해 종료 0, 프레임 2 캡처의 구름·타이틀을 육안 확인했다. 원본 또는 cpppj와 픽셀 비교는 하지 않았다. 새 fixture·포트 맵 변경, 은퇴 코드·검사는 없다. 전체 Release는 0.74의 2,043개가 마지막이다.
- **남은 작업:** 구름 초기 카메라 원점은 임시 범위이고 실제 카메라·시간별 이동·부분 dirty와 파일 부재 시 난수 생성은 미연결이다. 메뉴 버튼·입력 소유자·월드·음향도 없다. 이 화면은 D1이나 플레이 가능의 근거가 아니다. 원본 게임 실행·디컴파일은 하지 않았고 cpppj 미커밋 변경은 근거로 쓰지 않았다.

### 0.79 단위 기록 — 실제 타이틀 자산의 호스트 출력 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj HEAD `26ec793c`의 `ClientMain.cpp`·`UberGump.cpp`를 따라 게임 데이터의 실제 파일 시스템과 Config를 열고 `fortPal`/`GamePalSpec` 팔레트 및 `BackgroundGifSpec(titleMenu)` GIF를 8비트 화면에 연결했다. 타이틀의 투명 픽셀을 보존하고 640×480 기준 중심에 복사한다. 데이터가 없는 기존 호스트 골격은 유지한다. 새 경로 Program이 실제 자산을 발견하면 이를 사용한다.
- **검증:** 배포 자산을 직접 읽는 검사 포함 호스트 집중 14개 통과. HJOW-Athlon 예외 PC에서 새 Client.Host를 창 모드 3프레임 실행해 종료 0, 프레임 2 논리 BMP 캡처에서 중앙 타이틀을 육안 확인했다. 파일 `work/dotnet-title.bmp`는 작업 환경의 임시 자료다. 오라클과 픽셀 비교는 하지 않았다. 최초 빌드는 IndexedImage 형식 모호성으로 실패해 수정 후 통과했다. 새 fixture·포트 맵 변경, 은퇴 코드·검사는 없다. 전체 Release는 0.74의 2,043개가 마지막이다.
- **남은 작업:** 바깥 구름, 실제 Gump 버튼·메뉴·Guide 입력 설치, 월드 및 음향은 없다. 타이틀 표시만으로 D1 또는 플레이 가능을 뜻하지 않는다. 원본 게임 실행이나 디컴파일은 하지 않았고 cpppj 미커밋 변경은 근거로 쓰지 않았다.

### 0.78 단위 기록 — 호스트 입력 큐와 Guide 우선 분배 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj HEAD `bead72a4`의 `UberGump::Input/Event`에 맞춰 MonoGame Update가 스크립트·물리 입력 뒤 실제 입력 소비자를 호출한다. Guide 분배기는 미션 명령을 먼저 처리하고 미소비 사건을 실제 UI에 전달하며 종료 요청이 없을 때 Tell을 동기화한다. 같은 처리 중 예약한 사건도 순서대로 소비하고 종료 요청 뒤 큐를 남긴다. 소비자를 설치하지 않은 현재 골격은 큐를 보존한다.
- **검증:** 호스트·Guide 집중 검사 15개 통과(신규 2개). 같은 큐의 스크립트·Guide·UI 입력, Abort가 예약한 후속 명령, 종료 시 큐 보존을 창 없이 검사했다. 새 fixture·포트 맵 변경, 화면 비교, 은퇴 코드·검사는 없다. 전체 Release는 0.74의 2,043개가 마지막이다.
- **남은 작업:** 실제 Program은 아직 Guide·UI 소유자를 생성하거나 입력 소비자를 설치하지 않는다. 포인터 폴링·카메라 pan 관문·미션 프레임과 화면/월드/Player 효과도 미연결이다. 현재 구현은 새 게임의 플레이나 원본 화면 일치를 입증하지 않는다. 게임/창 실행과 디컴파일은 하지 않았으며 cpppj 미커밋 변경은 근거로 쓰지 않았다.

### 0.77 단위 기록 — Guide 미션 소유와 제어 명령 연결 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj HEAD `ceec6cc4`의 `UberGump.cpp`와 `CampaignGuideRelease.cpp`에 따라 Guide 소유자가 실제 MissionScript·Runtime을 보유하고 같은 입력 큐·문자열 고리의 Begin/Restart/Fork/End·Abort·First를 분배한다. 설정 source를 교체 전에 보유하고 재시작의 정규화된 모드·데모를 복원한다. Tutorial 반납은 0.76의 Player AI와 Config 해제에 연결했다. 공장 실패로 Runtime 없이 남은 스크립트도 명시 반납할 수 있다.
- **검증:** Guide 관련 Client 집중 검사 13개 통과(신규 소유자 통합 2개). 실제 Config·스크립트·큐·문자열 고리·Player 저장소로 분기/재시작/종료/Begin 및 생성 실패 뒤 등록 해제를 확인했다. 최초 검사 2회는 테스트 설정의 경로 이스케이프 오류로 실패했고 정정 뒤 통과했다. 새 fixture·포트 맵 변경, 화면 비교, 은퇴 코드·검사는 없다. 전체 Release는 0.74의 2,043개가 마지막이다.
- **남은 작업:** 실제 MonoGame 호스트는 이 소유자를 아직 생성하지 않으며 Player·제한 표·로딩 화면·월드/프레임 효과와 Fort 선택기 소비는 미연결이다. 비 Tutorial 미션은 cpppj처럼 기본 반납 경로만 사용한다. 현재 이 단위는 게임 플레이나 화면 일치를 입증하지 않는다. 게임/창 실행과 디컴파일은 하지 않았으며 cpppj 미커밋 변경은 근거로 쓰지 않았다.

### 0.76 단위 기록 — Tutorial 미션의 실제 Player 반납 경계 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj HEAD `075196df`의 `o/RawTutorialMissionRelease.h/.cpp`에 따라 허용 표→회수·승급 제한 및 미션/튜토리얼 번호·단계·변경 표식 0→같은 PlayerParticipation의 owner 1~8 brain 반납→기본 Mission 반납 순서를 이식했다. 본문 예외에도 기본 Mission을 반납하며 이전 부분 효과는 유지한다.
- **검증:** 공유 Player 바이트와 상태를 쓰는 집중 검사 3개 통과. 정상 순서, AI 소멸 실패, 허용 표 갱신 실패와 기본 Mission 정리를 검사했다. 원본 fixture 신규 연결과 포트 맵 변경은 없고 L1 대조나 화면 비교는 아니다. 전체 Release는 0.74의 2,043개가 마지막이다. 은퇴 코드·검사는 없다.
- **남은 작업:** 실제 Guide 소유자가 이 반납 경계와 스크립트·문자열 고리·입력 큐를 소유하도록 연결하고, 실제 Player·허용 표 및 세션에 바인딩해야 한다. 현재 단위만으로 Guide나 게임이 실행되지는 않는다. 게임/창 실행 및 디컴파일은 하지 않았고 cpppj 미커밋 변경은 근거로 쓰지 않았다.

### 0.75 단위 기록 — 미션 Fort 파일 지연 선택 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj HEAD `8e2d77c1`의 `client/Mission.cpp`·`MissionFortSource.h`에 맞춰 현재 fortSpec과 manager 이름을 I/O 전에 결정하는 `FortFileSelector`를 이식했다. 선택이 보유한 reader는 미션 반납·설정 변경 뒤에도 선택한 경로와 source를 사용한다. 기존 즉시 reader도 선택→읽기 순서로 바꿔 mapper가 I/O보다 먼저 실행된다.
- **검증:** MissionScript 집중 검사 4개 통과(신규 2개). 지연 I/O, 미션 반납 후 경로 고정과 읽기가 설정을 바꾸는 경우의 매핑 순서를 확인했다. 새 fixture·포트 맵 변경, 화면 비교, 은퇴 코드·검사는 없다. 전체 검사는 0.74의 2,043개가 마지막이다.
- **남은 작업:** 실제 RawCampaignSession의 같은 이름 manager 재사용과 선택기의 소비·월드 조립은 미구현이다. 게임/창 실행 및 디컴파일은 하지 않았다. cpppj의 미커밋 파일은 근거로 쓰지 않았다.

### 0.74 단위 기록 — Player 참여 현재 전역 조회 (2026-11-01, HJOW-Athlon)

- **구현:** cpppj HEAD `fd102f81`의 `o/RawPlayerParticipation`(해당 파일 마지막 변경 `3cea1b14`)에 맞춰 로컬 owner·PlayerKnown·battle·metamap·localTransport의 현재 조회 다섯 개를 원자적으로 바인딩한다. 효과 뒤에는 각 읽기 위치에서 재조회하며, 미연결이면 기존 fixture의 공유 상태를 쓴다. 부분 바인딩 실패는 기존 연결을 유지한다.
- **검증:** PlayerParticipation 집중 검사 7개 통과(기존 원본 fixture 포함, 신규 3개). 전체 Release 2,043개(Assets 364, Core 1,052, Client 627) 통과, 실패·건너뜀 0이다. 콜백 중 현재 값 변경과 부분 바인딩 실패를 검사했다. 연결 fixture 132개 해시 일치, 포트 맵은 기준 이후 변경 신호 52쌍이다. 새 fixture·포트 맵 변경, 화면 비교, 은퇴한 코드·검사는 없다. cpppj 미커밋 변경은 근거로 쓰지 않았다.
- **남은 작업:** 현재 조회를 실제 Session에 바인딩하지 않았다. `ReceiveAlliance` 등 나머지 재대조 및 호스트·월드 조립이 남는다. 전체 목표는 미완료다. 게임/창 실행과 디컴파일은 하지 않았다.

### 0.66 단위 기록 — 미션 Fort reader 수명 (2026-10-31, HJOW-Athlon)

- **구현:** cpppj HEAD `fa5baf3c`의 `client/Mission.cpp`를 기준으로 `ReadFortImage`와 `FortReader`를 이식하고 기존 `FortFileReader`가 미션 객체 대신 설정과 선택 source를 보유하도록 정리했다. 선택 이름으로 현재 `fortSpec`을 한 번 풀며 명시 source의 부재를 기본 자산으로 우회하지 않는다.
- **검증:** `MissionScriptTests` 집중 2개 통과. 명시 source, 미션 반납 후 현재 설정 변경, 파일 경로 매핑 및 부재를 검사했다. 전체 Release 검사 2,017개(Assets 364, Core 1,040, Client 613) 통과, 실패·건너뜀 0이다. 창·원본 게임 실행과 디컴파일은 하지 않았다.
- **남은 작업:** 이 reader는 아직 실제 미션 세션에 연결되지 않았다. Guide 생성/반납, 목록 owner, 대화창·상위 메뉴 및 R4 이후 세션·HUD·입력·Edit·멀티플레이 사전 구현이 남는다. 전체 목표는 미완료다.

### 0.67 단위 기록 — Guide 미션 교체·명령 분배 (2026-10-31, HJOW-Athlon)

- **구현:** cpppj `GuideMissionTransitions.h/.cpp`(기준 `d406adf8`)의 Begin·Close·Fork·Restart·End 및 명령 분배와 `MissionGuideInput`의 재시작 캡처를 `Netstorm.Client/Mission/GuideMissionTransitions.cs`로 이식했다. 종료·반납 중 재진입 조회, 분기 이름의 해제 전 복사, 음수 DWORD 분기 번호, 진단 뒤 Begin 진행 순서를 보존한다. 포트 맵 2행은 L1이며 실제 세션 연결을 뜻하지 않는다.
- **검증:** 독립 `guide-transitions-x86.tsv` 286행 중 교체·분배 T 256행을 직접 비교해 일치했고, 미등록 명령과 빈 Close도 검사했다. S 30행은 문자열 고리·큐 연결 범위여서 이번 검사에 포함하지 않았다. fixture 원본을 링크하고 해시 목록에 등록했다. 전체 Release 2,019개(Assets 364, Core 1,040, Client 615)가 통과했고 실패·건너뜀은 0이다.
- **남은 작업:** 실제 Guide의 미션 소유자·생성/반납, 문자열 고리와 입력 큐 및 메뉴·세션 연결은 미완료다. 현재 C# Begin 인자는 소유 문자열이므로 C++에서 빌린 가변 버퍼가 해제 중 바뀌는 특수 경계는 아직 표현하지 않는다. 미션 생성자에서 파일 reader 예외 시 Config 등록 반납도 다음 단위에서 확인한다. 전체 목표는 미완료이며 게임/창 실행이나 디컴파일은 하지 않았다.

### 0.68 단위 기록 — Guide 스크립트 생성·반납 및 예외 수명 (2026-10-31, HJOW-Athlon)

- **구현:** cpppj `Mission.h/.cpp`의 Guide 스크립트 생성·반납을 `GuideMissionScriptLifetime`에 이식했다. 생성 후 모드→설정 공개, 반납 시 소유 슬롯/Config 등록 해제→모드→설정 알림 순서를 유지한다. 생성 중 reader 예외에서 C++ 멤버 소멸과 같이 Config 등록을 반납하도록 `MissionScript` 생성자를 수정했다. 0.67에서 남긴 빌린 이름은 `Begin(Func<string>)`으로 해제 후 읽도록 연결할 수 있게 했다.
- **검증:** 실제 Config 알림·reader 예외 및 빈 재반납과 빌린 이름을 포함한 집중 8개 검사와 0.67의 x86 256행이 통과했다. 전체 Release 2,023개(Assets 364, Core 1,040, Client 619) 통과 후 실제 스크립트 분기·종료 연결 검사를 추가하여 해당 집중 5개가 통과했다. `Mission.h/.cpp` 포트 맵 2행 추가, fixture 신규 등록 없음.
- **남은 작업:** Guide의 실제 세션 소유자와 이벤트 큐에 이 수명 함수를 연결하지 않았다. 메뉴·월드·HUD·Edit·멀티플레이 사전 구현과 전체 목표는 미완료다. 게임/창 실행 및 디컴파일은 하지 않았다.

### 0.69 단위 기록 — 미션 제어 입력과 큐 연결 (2026-10-31, HJOW-Athlon)

- **구현:** cpppj `MissionGuideInput.h/.cpp`(기준 `def7d68a`) 중 미션 제어 이름 조회·공통 파서/고리/큐 예약, Abort·First 분배와 F8 예약을 이식했다. 명령을 큐에서 꺼내 문자열 고리의 이름을 Guide 교체에 연결하는 경로를 실제 `MissionScript`·`ConfigInterface`로 검사했다. 포트 맵의 두 행은 **부분 이식**이며 데모 설정·현재 단계 Tell·MissionFirst 상태 갱신은 아직 빠져 있다.
- **검증:** 독립 `guide-transitions-x86.tsv`의 이전에 미검사였던 S 30행이 모두 일치했고 기존 T 256행도 집중 검사에서 통과했다. F8 수정키/키 뗌/비활성 미션, Abort 확인·Leave 예약, 실제 큐를 거친 미션 스크립트 교체를 검사했다. 집중 5개 통과. 전체 Release 2,027개(Assets 364, Core 1,040, Client 623)가 통과했고 실패·건너뜀은 0이다. fixture 추가 없음.
- **남은 작업:** 이 입력 경로는 실제 호스트의 Guide/세션 루프에 아직 연결되지 않았다. MissionGuideInput의 남은 단계 안내, Guide 소유자·메뉴·월드·HUD·Edit·멀티플레이 사전 구현이 남으며 전체 목표는 미완료다. 게임/창 실행이나 디컴파일은 하지 않았다.

### 0.70 단위 기록 — 원본 공통 미션 시작 (2026-10-31, HJOW-Athlon)

- **구현:** cpppj `o/RawMissionStart.h/.cpp`(기준 `93d26800`)의 설정 조회·tellAt·SP·기술·사제/AI·Gump·간헐천·카메라·시계 효과 순서를 `Netstorm.Core/Original/RawMissionStart.cs`에 이식했다. 프레임 게이트를 공유하며 현재 소유자/편집 모드/PlayerKnown을 원본 위치에서 읽고, 공유 포획·간헐천 저장소에 결과를 쓴다. 실제 효과가 누락되면 완료 상태를 쓰지 않고 실패한다.
- **검증:** 독립 `mission-start-x86.tsv` 전체 205행 중 Start 48행의 상태와 모든 효과를 일치시키고 반대 fallback을 둔 현재 getter로 같은 48행을 재생했다. 효과 도중 소유자가 바뀌는 순서와 필수 효과 누락도 검사해 집중 3개 통과. 나머지 AI 설정/생성 156행은 이번 범위가 아니다. fixture를 원본 링크 및 해시 목록에 등록했고 포트 맵 2행은 L1이다. 전체 Release 2,030개(Assets 364, Core 1,043, Client 623)가 통과했고 실패·건너뜀은 0이다.
- **남은 작업:** 실제 미션 Config·Player·SP·AI·Gump 및 세션 루프의 효과 연결과 AI 설정/생성 구현이 필요하다. 메뉴·월드·HUD·Edit·멀티플레이 사전 구현과 전체 목표는 미완료다. 게임/창 실행이나 디컴파일은 하지 않았다.

### 0.71 단위 기록 — 원본 미션 AI 설정 (2026-10-31, HJOW-Athlon)

- **구현:** cpppj `o/RawMissionAiSettings.h/.cpp`(기준 `081cfdc9`)의 원본 600바이트 기본값·능력 비트·공통 및 소유자 설정 적용을 `Netstorm.Core/Original/RawMissionAiSettings.cs`에 이식했다. 설정마다 현재 미션 값을 다시 조회하고 동맹·능력 비트를 누적하며, 원본 미초기화 바이트와 예외 시 부분 효과를 보존한다. 실제 `MissionScript`의 설정을 준비기에 전달하는 통합 검사도 작성했다.
- **검증:** 독립 `mission-start-x86.tsv`의 AI 설정 124행(init 20, apply 64, ability 32, prepare 8)이 전체 600바이트와 조회·진단 순서까지 일치했다. 효과 누락·크기 오류와 실제 미션 설정 연결도 통과했다. 기존 fixture를 사용했으며 포트 맵 2행을 L1로 추가했다. 전체 Release 2,033개(Assets 364, Core 1,045, Client 624)가 통과했고 실패·건너뜀은 0이다.
- **남은 작업:** AI 설정의 실제 Session 소유자·생성자/초기 갱신 및 전략 AI 연결은 미완료다. 같은 fixture의 AI 생성 32행도 아직 이식·검사하지 않았다. 전체 목표는 미완료이며 게임/창 실행 및 디컴파일은 하지 않았다.

### 0.72 단위 기록 — AI 생성과 Player 반납 수명 (2026-10-31, HJOW-Athlon)

- **구현:** cpppj `o/RawAiCreate.h/.cpp`(기준 `53a0e4f9`)의 색 변경→기존 brain→NoTemple→실제 생성→Player 부착→초기 갱신 순서를 이식했다. `PlayerParticipation`에 cpppj의 정상 reset 증빙·소유자 반납·파괴 콜백 중 소비 금지·반납 뒤 재설치 거부를 추가했다. 실제 AI 본체/초기 갱신이 없으면 가짜 brain을 부착하지 않는다. 이 수명 추가는 PlayerParticipation의 최신 cpppj 변화 전체를 이식했다는 뜻이 아니다.
- **검증:** `mission-start-x86.tsv`의 AI 생성 32행이 Player 180바이트와 효과 순서까지 일치했다. 기존 Player 원본 검사 및 증빙 소유·중복·재진입·원시 변조 검사 등 집중 8개 통과. 이로써 이 fixture의 start 48, AI 설정 124, create 32 등 독립 입력 204행을 각 대응 단위에서 검사했다. 포트 맵 2행 L1 추가, fixture 신규 등록 없음. 전체 Release 2,037개(Assets 364, Core 1,049, Client 624)가 통과했고 실패·건너뜀은 0이다.
- **남은 작업:** 실제 AI 생성자·초기 갱신·전략 AI 및 세션과의 연결은 미완료다. PlayerParticipation의 현재 getter 등 나머지 cpppj 변화도 별도 재대조해야 한다. 메뉴·월드·HUD·Edit·멀티플레이 사전 구현과 전체 목표는 미완료다. 게임/창 실행 및 디컴파일은 하지 않았다.

### 0.73 단위 기록 — Guide 단계 반복·안내 및 정지 (2026-10-31, HJOW-Athlon)

- **구현:** cpppj `o/RawMissionGuide.h/.cpp`, `client/MissionLifecycle.h/.cpp`와 `MissionGuideInput`의 남은 단계 안내·First·데모 설정(기준 `3f7b990a`)을 이식했다. 현재 미션 포인터 재조회, 대기/대화창 관문, 같은 호출의 단계 반복, 월드 진입 Pause와 기존 `MissionFrameGate`의 정지·재지연을 같은 상태로 연결했다.
- **검증:** 독립 x86 `mission-lifecycle` 831행, `mission-first` 278행, `mission-common` 108행이 전체 상태 및 효과 순서까지 일치했다. 집중 검사 3개 통과. 세 fixture를 원본 링크와 해시 목록에 등록하고 Guide·Lifecycle 포트 맵 4행을 L1로 추가했다. 전체 Release 2,040개(Assets 364, Core 1,049, Client 627)가 통과했고 실패·건너뜀은 0이다.
- **남은 작업:** 검사의 Init·Start·Frame·UI는 명시 효과이며 실제 호스트·월드·Guide 소유자에 조립되지 않았다. 미션 월드·HUD·입력·Edit·멀티플레이 사전 구현과 전체 목표는 미완료다. 게임/창 실행 및 디컴파일은 하지 않았다. 사용자 요청으로 이번 단위의 문서·커밋 후 목표를 일시 중지한다. 재개 시 AGENTS.md 및 공통·dotnetpj 인계를 다시 읽고 실제 호스트·월드 조립부터 이어간다.

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
| 미션 조립 부품 (10-31, 0.66~0.73) | Fort reader, Guide 교체·수명, 미션 제어 입력, 공통 Start, AI 설정·생성, Guide 단계·Lifecycle | 이 문서 0.66~0.73 | L1. 실제 세션 연결은 R4 |

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

**2026-10-31 이관(HJOW-X3D, 문서만):** 정리 직전 전체 원문을 [이관 파일 끝](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073)에 그대로 보관했다. 본문에서 뺀 것은 머리 안내 3개, 2026-10-28 당시 상태표와 진단 상세, 완료한 R0~R2 상세, 0.9의 완료 항목, 0.10, 단위 기록 0.11~0.65, 2절의 완료 줄이다. 빌드·검사·게임 실행·디컴파일은 하지 않았다.

**2026-10-28 이관(VM-W11-Codex):** 0절 재수립으로 더 이상 현행 의미가 없어진 내용을 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)로 옮겼다. 내용은 고치지 않고 그대로 옮겼으며, 그 파일 안의 "현재·다음·미구현" 표현은 당시 상태다.

- **옮긴 것:** 2026-10-22 이전 계획, 2026-10-09~10-23 날짜별 완료·인계 절, 구 머리말과 0~8절(2026-10-07~10 당시 1~6단계·8절 이식/대기 구분), 옛 9절 표와 9-1의 옛 순서 제안, 옛 10절 1·3·4번(해결·구현이 끝난 확인 항목), 11절(과거 미확인 항목 재분류), 14절(LEFT_JOBS.md에서 이관한 2026-09-27~10-05 이력).
- **본문에 남긴 것:** 0절 현행 계획, 1절 완료 범위 색인, 2절 9-1 결정, 3절 옛 10절의 유효한 결정, 4절 검증 명령(옛 12절), 5절 참고 문서(옛 13절).
- **앵커:** 옮긴 절의 `<a id>`(`dotnet-current-plan-20261022`·`sid-session-plan`·`legacy-dotnet-history`·`legacy-dotnet-031`~`107` 등)는 이관 파일에 같은 이름으로 남겼다. LEFT_JOBS.old.md·LEFT_JOBS.webpj.md·`docs/`의 기존 링크가 이 안내로 오도록 같은 이름의 빈 앵커를 바로 아래에 두었다. 그런 링크로 들어왔다면 이관 파일의 같은 앵커를 본다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
