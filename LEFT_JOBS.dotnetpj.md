# LEFT_JOBS.dotnetpj — dotnetpj(C# + MonoGame) 작업 계획 및 인수인계

이 문서는 dotnetpj의 **현재 상태, 계획, 남은 작업, 유효한 사용자 결정**을 담는다. cpppj와 공통 사항은 [LEFT_JOBS.md](LEFT_JOBS.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md), 과거 기록은 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)에 있다.

[0절 현행 계획](#dotnet-current-plan-20261028)이 이 문서와 [이식 계획 문서](docs/dotnet-cpp-port-plan-20261016.md)의 순서·구조 결정보다 우선한다. 1절은 완료 범위 색인, 2·3절은 계속 유효한 사용자 결정, 4·5절은 검증 명령과 참고 문서다.

단위를 마치면 0.1절의 값과 0.9절의 체크 항목을 갱신한다. 상세는 `docs/dotnet-*.md` 단위 문서에 남기고, 문서가 없는 작은 단위만 이 문서의 단위 기록(현재 0.154부터)에 짧게 적는다. 0.153 이하는 이관 파일에 있다.

**2026-11-04 정리:** 이 문서를 현재 상태·계획·남은 작업 중심으로 다시 정리했다(6절 첫 항목). 단위 기록 0.127~0.153과 0.1의 단위별 수치 나열은 [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154)에 원문 그대로 보관했다. 완료됐지만 체크되지 않았던 Template·Fort 단계(F1~F2c-11)는 0.9에서 `[x]`로 표시했고, 그 단위들에서 해소되지 않은 한계는 0.9의 '이월한 이식 한계'로 옮겼다.

<a id="dotnet-current-plan-20261028"></a>

## 0. 2026-10-28 현행 계획 — cpppj 그래픽 복원분을 기준으로 dotnetpj 재구성

### 0.1 현재 값 (2026-11-07, 단위 기록 0.213 기준 — `/goal` 지시로 재개 후 작업 중)

| 항목 | 값 |
| --- | --- |
| 진행 상태 | **재개 후 작업 진행 중(`/goal` 지시).** 전체 목표는 미완료다. 최근 완료 단위는 0.213(공장 부착 배선)이며, 다음 단위는 F2d·F3 나머지(Host 부착·월드 본체 조립·`RawCampaignSession` 연결)이다. |
| 현재 단계 | R0·R1 완료, R2 본체 완료, R3 진행 중, R4 일부 이식(Template 저장소 TS0~TS6과 Fort 단계 F1·F2a~F2c-12·연결 본체·전수 재생·Player Core·초기화·Chat·참여·장부·갱신·월드 본체 10모듈 완료, R4 지형·섬 생성 1차 12모듈·2차 5모듈 완료, 게임 난수·보드 각도·바람 진행·저장소 껍데기·해시 선택/주소·물리 자료형·풀 생성/전체 대여·게임 시계·커널·프로세스 상태·호스트 저장소 껍데기·프로세스 기본/정기·호스트 묶음·표시 진단·호스트 껍데기·부착 부모·호출 수명·런타임 훅·프로세스 부착·포함 링크·호스트 조회·Form 부착·Content 부착·호스트 통지/검색·Form 삭제 통지·호스트 잔류 조회·지형 바인딩 계약·호스트 대여·지형 패턴 개수·첫 기록·군집 후처리·Host 삭제 본체·Host 정기 실행·서피스 월드 껍데기·Fort 적재 바인딩·Forms 부착 바인딩·정상 바인딩 껍데기·Form Pop 바인딩·공장 부착 배선 완료, Host 부착 잔여·월드 본체 조립·F3 미착수). R5 이후는 미착수 |
| 실행·플레이 가능 여부 | 플레이 가능한 기본 실행은 기존 경로 `Netstorm.Game`(`Netstorm.exe`, 0.96)의 1-1·1-2 근사 플레이다. 새 경로 호스트 `Netstorm.Client.Host`는 타이틀·메인 메뉴 등 메뉴 계열 화면 일부까지이고 미션 월드는 없어 플레이할 수 없다. 기존 코드·검사 은퇴는 0건이다. |
| 원본과의 동일성 | 실제 메뉴·게임 캡처 비교는 아직 0건이다. 오라클 메인 메뉴 캡처의 결정성만 확인했다([문서 5절](docs/dotnet-client-screen-20261028.md)). L1(원본 x86 fixture) 통과를 L2 이상으로 합산하지 않는다(0.6). |
| 마지막 구현 커밋 | 0.212 `ad60373c / 20261107-141 DN`(Form Pop 바인딩, 이전: 0.211 `ce7c03de / 20261107-139 DN`, 0.210 `e142d16f / 20261107-138 DN`). |
| 검사 | 0.213에서 Release **Core 집중 50개**(공장 직접 7개·호스트 39개 + 무결성·매니페스트 4개) 통과, 실패·건너뜀 0. Core 전체는 0.211 재기준선 1,576개가 마지막이며 변경이 기존 1개 확장+신규 2개뿐이라 기존 검사에 영향을 주지 않으며(솔루션 빌드 경고·오류 0), 무결성·매니페스트 검사는 통과했다. Client·Assets는 0.164 값을 유지한다. 단위별 집중 검사·변이 검사 수는 0.154~0.213 기록과 [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154)의 0.127~0.153을 본다. |
| 포트 맵 | `PORT_MAP.tsv` 총 613줄(자료 608행: L1 329, port 279)이다. 0.212에서 2행(port 2)을 더한 뒤 0.213에서 유지했다. 단위별 증가 내역은 0.155~0.213 기록과 이관 파일에 있다. |
| fixture | 연결 232개(`cpp_sync.py` 0종료). 반영 커밋(`reflected-commit`)은 영향 검토 전이라 `f95a8db2`에 그대로 둔다. |
| cpppj 기준 | 이식은 단위마다 cpppj HEAD의 해당 파일만 읽고 미커밋 변경은 쓰지 않는다(0.167은 월드 본체 기준 `fe3d121d`). 영향 검토 전이라 `reflected-commit`(`f95a8db2`)과 PORT_MAP 기존 행의 `base`는 올리지 않았다(신규 행만 파일별 HEAD). cpppj 현재 값은 [LEFT_JOBS.md](LEFT_JOBS.md) 0.1절을 따른다. |
| 디컴파일 | 공통 기록의 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**(LEFT_JOBS.md 0.1절)이다. 이식 근거는 cpppj 소스·fixture·문서이므로 디컴파일이 필요 없다. 산출물을 직접 읽어야 할 때만 AGENTS.md대로 재디컴파일한다. |

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
- **근거 우선순위:** 원본 fixture → cpppj 보고서의 계약·경계 → cpppj 소스 → 녹화·스크린샷. cpppj는 HEAD만 쓰고 미커밋 변경은 쓰지 않는다. `cpppj/`·`LEFT_JOBS.md`는 수정하지 않고, 필요한 요청은 이 문서와 이식 계획 8절에 적어 사용자에게 전달한다. (2026-11-02·2026-11-04에 사용자 요청으로 `LEFT_JOBS.md`와 이 문서를 함께 정리한 것은 이 원칙에 대한 사용자 지시 예외다.)
- **외부 접속 문서 갱신:** 새 경로의 Help 링크(R3)·시작 화면의 검증 안내·`Netstorm.Net`(3차)을 옮길 때마다 [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md)의 소비자 목록을 갱신한다. 구 클라이언트 검증·master/root 인터넷 서버·ping/진단은 이식해도 연결하지 않고 성공을 합성하지 않는다. 사용자가 선택하는 도움말 링크 실행과 로컬 서버/LAN 접속(3차)은 별도 경계다(0.151).
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

재개하면 먼저 `python dotnetpj/tools/cpp_sync.py`와 `--ports`로 cpppj 변경 영향을 확인한다. cpppj는 2026-11-04 현재 단위861/표시 11.01이며(0.1의 cpppj 기준) fixture의 `reflected-commit`(`f95a8db2`)보다 앞서 있다. 영향 검토 전에는 기준 커밋을 올리지 않는다. `--ports`는 0.147~0.154에서 계속 시간 초과였으므로, 그동안은 단위가 읽는 cpppj 파일의 `git log`로 기준을 직접 확인했다.

- [ ] **R4 실제 호스트·월드 조립 (재개 지점):** 미션 객체, Fort reader, Guide 교체·수명, 미션 제어 입력, 공통 Start, AI 설정·생성, Guide 단계·Lifecycle은 이식과 L1 대조를 마쳤지만 검사에서는 명시 효과로만 연결돼 있다. 다음을 실제 호스트와 세션에 조립한다.
  - 완료한 부품의 범위는 1절 색인과 이관 파일의 0.66~0.126을 본다. 미션 세션의 Fort 선택기·지연 reader 소비(0.75), Guide 소유자·명령·Tutorial 반납(0.77), 호스트 프레임의 큐 소비·Guide 우선 분배기(0.78)는 이식했으나 실제 호스트에서 분배기를 설치하고 같은 Player·제한 표·메뉴·월드 효과에 연결해야 한다(0.66, 0.68, 0.69, 0.76~0.79).
  - Tutorial Init의 설정·전역·이름·요새 요청·Header 순서(0.105), 실제 옵션 초기화(전체 옵션 바이트 fixture 대조, 0.106), 요새 요청의 상태144바이트·Zone 복제(0.107), Guide와의 flags·demo·tutorialNumber·missionNumber 및 현재 Config 결합(0.108), 기술 허용 목록과 공유 제한 표 쓰기(0.109), 초기화 순서 조립(0.110)은 이식했다. 실제 템플릿/옵션 소유자·요새 선택·상태 실행·실제 타입 조회와 호스트 단계 연결은 미완이다.
  - Start의 실제 Config·Player·SP·AI·Gump 효과(0.70)
  - AI 설정의 Session 소유자, 실제 AI 생성자·초기 갱신·전략 AI(0.71, 0.72)
  - `PlayerParticipation`의 현재 getter는 이식했다(0.74). 실제 세션 바인딩과 동맹 수신 `ReceiveAlliance` 등 나머지는 미완료다.
  - **Template/DataManager·Fort 적재/저장 단계** ([Template 저장소 이식 계획](docs/dotnet-template-storage-plan-20261101.md), 설계 네 가지는 3절 12번). 단위별 상세는 이관 파일의 0.128~0.153과 이 문서의 0.154 기록이다.
    - [x] 원문 계층(MemStream·DataManager·Template·Fort 이미지·State, 0.128)·`ClientTemplateSectionNames`(0.129)
    - [x] 저장소 TS0~TS6: 용량을 가진 섹션 버퍼·설치/선택·동적 섹션·파일 읽기/저장·빈 manager/로그인 송신 이미지·Editor 파일 계층·새 맵/이름/목록(0.130~0.136)
    - [x] F1 타입 표·`.type` 파서·`FortTemplate.Parse`(0.137에서 작성, 0.138에서 첫 실행·전체 재확인·변이·PORT_MAP L1로 확정)
    - [x] F2a Money·Meta 로더(0.139), F2b TypeNames·Deck·Contents·Subscriber 적재(0.140)
    - [x] F2c-1 Territory·팔레트(0.141), F2c-2 객체 적재 본체(0.142), F2c-3 객체 스트림·Meta ContentsHooks(0.143)
    - [x] F2c-4~F2c-8 저장 모듈 14종·`ChunkMap`·FinishOwner(0.144~0.148), F2c-9 변환 저장소(0.149), F2c-10 AddRandom 부모·입력(0.153), F2c-11 섬 목록·단일 배치·군집·게임 보조·Notify(0.154), F2c-12 Canon 디코더·섬 빌더/배치/검색·Fort 월드 적재 부모(`CanonDecoder`·`CanonTypeDecoder`·`RawCheckingFlag`·`RawIslandBuilder`·`RawIslandPlacement`·`RawIslandQueries`·`RawFortWorldLoad`·`MakeFortWorldChildrenHooks`, 0.155)
    - [x] R4 지형·섬 생성 1차(`TerrainBuilder`·`RawSquidRegion`·`RawFrameChoose`·`RawTerrainBitmap`·`RawTerrainStamp`·`RawTerrainContext`·`RawTerrainGrowth`·`RawIslandPopulation`·`RawIslandAdd`·`RawBattleIsland`·`RawCentralIsland`·`RawIslandPartition`, 0.156)
    - [x] R4 Fort 사제 준비(`RawFortPriestPreparation`, 0.157)
    - [x] R4 섬 수명주기(`RawIslandLifecycle`, 0.158)
    - [x] R4 배치 지형 2종(`RawCanonPlacementTerrain`·`RawPriestPlacementTerrain`과 공유 형식 3종, 0.159)
    - [x] R4 섬 후처리(`RawIslandPostPop`와 연결 전역·훅 형식, 0.160)
    - [x] F2c 다리·섬 연결 본체(`RawBridgeConnect`, 0.161)
    - [x] F2c 다리 연결 전수 재생·이웃 CD 정렬(판본별 3,558행, 0.162)
    - [x] F2d Player Core 장부·CoreData(`RawPlayerCoreRecords`·`RawPlayerCoreData`, 0.163)
    - [x] F2d Player 초기화·Chat 채널(`RawPlayerInitialization`·`RawChatChannels`, 0.164)
    - [x] F2d Player 참여·초기화 재생(`RawPlayerParticipation`·202행 전수, 0.165)
    - [x] F2d CoreData 완성·갱신(장부 검사·`ClientCoreDataGlobals`·월드 상태·`RawPlayerCoreRefresh`, 0.166)
    - [x] F2d Player 월드 본체(`RawPlayerWorld`·직접 11종, 0.167)
    - [x] F2d 캠페인 월드 전역(`CampaignWorldGlobals`·빌림/복제/대입 직접 6종, 0.168)
    - [x] F2d 캠페인 월드 적재 본체(`RawCampaignWorldLoad`·생성/CopyName/현재 Player/사제 후처리/Load 순서 직접 9종, 0.169)
    - [x] F2d 서피스 삭제 분배기(`RawSurfaceLifecycle`·생성/공통 유지/가상 표 분기 직접 12종, 0.170)
    - [x] F2d 저장 나머지(`RawSavedState` 쓰기·판독·538행 전수 재생 직접 13종, 0.171)
    - [x] F2d 서피스 월드 자료형(`SurfaceSeed`·`SurfaceSnapshot`·`SurfaceDisplayHooks` 직접 6종, 0.172)
    - [x] F2d 서피스 월드 권한(`RawAuthorityState`·초기값/독립/관문 직접 3종, 0.173)
    - [x] F2c 일반지형 요청형(`NormalTerrainBranch`·`NormalTerrainRequest`·농지 후처리 진단 직접 6종, 0.174)
    - [x] F2d 서피스 월드 바람(`AmbientWindState`·초기값/독립/읽기쓰기 직접 3종, 0.175)
    - [x] F2d 게임 난수·보드 각도·바람 진행(`GameRandom`·`CrtRandom`·`RawGameRandomSeed` 직접 11종, `BoardAngleDiagnostics`·`Normalize` 1088행 전수 1종+직접 3종, `RawAmbientWind.Step` 1920행 전수 1종+직접 2종, 0.176 — 0.175의 `StepAmbientWind` 잔여 해소)
    - [x] F2d 서피스 저장소 껍데기(`BoardTargetState`·`BridgeDecayScanState`·`RawSurfaceStorage` 진단·바람·대여 직접 11종, 0.177)
    - [x] 해시 선택·주소 전수 재생(`RawSquidHash` `Select`·`At`·`MutableEntries`와 811행 전수 1종+직접 3종, 0.178 — W02 잔여 `At`·`Select` 해소)
    - [x] 저장소 물리 자료형(`RawTotalMadeStorage`·`RawGraphStorage`·`RawPathSearchWork/State/Storage` 직접 5종, 0.179)
    - [x] 저장소 풀 생성·전체 대여(자산 벡터·풀·spot·그래프·집계·섬·청크 접합과 `Acquire` 직접 4종, 0.180)
    - [x] 게임 시계·프레임 시각(`GameClock`·`FrameTime`과 114행 전수 1종+직접 4종, 0.181)
    - [x] 커널·기본 프로세스(`BaseProcess`·`Kernel` 직접 6종, 0.182 — 진단 5종은 Client·`SquidProcess` 필요라 제외)
    - [x] 프로세스 상태(`SquidProcessState`·`RegularHandler` 직접 6종, 0.183 — 전송·Host 본체는 `SquidProcess` 필요라 제외)
    - [x] 프로세스 호스트 저장소 껍데기(게임 보조 절차 번호·`RawProcessHostStorage` 생성·장치·상태 직접 6종과 저장소 `Processes` 접합, 0.184 — Host 본체·대여·회수는 `SquidProcess` 필요라 제외)
    - [x] 프로세스 기본·정기 껍데기(`SquidProcess`·`RegularProcess` 식별자·예약 직접 6종, 0.185 — 부착·실행 본체는 Host 본체 단위에서 잇는다)
    - [x] 호스트 묶음(`ProcessFormTransmission`·`ProcessHostRuntimeBindings` 직접 5종, 0.186 — Host 본체·대여는 잔여)
    - [x] 표시 진단(`SquidDisplaySink`·`SquidDisplayView`·프레임 오류 상세와 저장소 `RecordDisplayFrameError` 직접 5종, 0.187 — 표시 본체는 R5 범위)
    - [x] 프로세스 호스트 껍데기(`SquidProcessHost` 생성·조회·번호 직접 6종, 0.188 — 부착·검색·삭제·전송 본체는 잔여)
    - [x] 부착 부모 함수(`AttachBaseProcess` 48행 전수 1종+직접 2종, 0.189 — Host `Attach` 접합은 잔여)
    - [x] 호출 수명·부착 배선(`RuntimeCallGuard`·프로세스 배선 직접 3종, 0.190 — Host 본체에서 소비)
    - [x] 월드 런타임 훅(`RawWorldRuntimeHooks`·전송 대리자 2종 직접 3종, 0.191 — 월드 본체에서 소비)
    - [x] 프로세스 부착(`ProcessAttachHooks`·`AttachProcessForm` 48행 전수 1종+직접 1종, 0.192 — Host 접합은 잔여)
    - [x] 포함 링크(`ContentNoticeEntry`·`FormLinkLayout`·부모/이전 getter·setter 직접 4종, 0.193 — 분리 본체는 잔여)
    - [x] 호스트 조회(`FromForm`·`Word`·`CreateOrTakeForm` 직접 3종, 0.194 — 부착 본체에서 소비)
    - [x] Form 부착(`FormAttachHooks`·`AttachCommonForm` 864행 전수 1종+직접 1종, 0.195 — Host 접합은 잔여)
    - [x] Content 부착(`ContentAttachHooks`·`AttachSortedContent` 41행 전수 1종+직접 1종, 0.196 — Host·정렬 소비자는 잔여)
    - [x] 호스트 통지·검색(`NotifyDependency`·`FindEvent`·`FindEventMasked`·`IsProcessForm` 직접 4종, 0.197 — 실행·삭제 본체는 잔여)
    - [x] Form 삭제 통지(`FormPreDestroy`·`FormParentDying` 직접 4종, 0.198 — `Kill`·`Hooks`·`UnpopForm`은 잔여)
    - [x] 호스트 잔류 조회(`Owns`·`DrainRuntimeBindings`·`HasOnlyRegularProcesses` 직접 3종, 0.199 — 실행·삭제 본체는 잔여)
    - [x] 지형 바인딩 계약(분기·범위·타입 연결 직접 3종, 0.200 — 바인딩 본체는 잔여)
    - [x] 호스트 대여(`Host`·`Bind`·`Use`·증빙·회수 직접 7종, 0.201 — 실행·삭제 본체는 잔여)
    - [x] 지형 패턴 개수(`PatternCount` 직접 3종, 0.202 — 생성·도장·군집 본체는 잔여)
    - [x] 첫 기록(`FirstRecord` 직접 4종, 0.203 — 생성·도장·군집 본체는 잔여)
    - [x] 군집 후처리(`PostprocessCurrentFarms` 직접 4종, 0.207 — 생성·도장 본체는 잔여)
    - [x] Host 삭제 본체(`UnpopForm`·`Kill`·`Hooks` 직접 9종, 0.204 — 정기 실행·`RefreshForm`·`AddRegular`는 잔여)
    - [x] Host 정기 실행(`RunRegular`·`RunFrame` 배선 직접 10종, 0.205 — `RefreshForm`·`AddRegular`·할당 배선은 잔여)
    - [x] 서피스 월드 껍데기(`RawSurfaceWorld` 생성·선택·저장소 직접 4종, 0.206 — 본체 조립은 잔여)
    - [x] F2c Fort 적재 바인딩(`MakeFortWorldLoadBinding` 직접 6종, 0.209 — 정상 바인딩 본체 조립은 잔여)
    - [x] Forms 부착 바인딩(`BindAttach`·바인딩 위임 직접 5종, 0.210 — 공장·Host 부착은 잔여)
    - [x] 정상 바인딩 껍데기(`RawNormalTerrainWorldBinding` 생성·위임 직접 4종, 0.211 — 생성·도장 본체는 잔여)
    - [x] Form Pop 바인딩(`FormPopHooks`·`PopCommonForm`·`BindPop` 직접 6종, 0.212 — 공장·Host 부착은 잔여)
    - [x] 공장 부착 배선(바인딩 보관·현재 Pop·부착·분리·직접 Pop 직접 7종+Host 연결 2종, 0.213 — Host 부착 본체는 잔여)
    - [ ] F2c 나머지: `RawFortWorldLoadBinding`(R4 지형 `RawNormalTerrainWorldBinding` 및 `RawSurfaceWorld` 필요)
    - [ ] F2d 나머지: `RawSurfaceWorld`
    - [ ] F3: `RawCampaignSession` 연결. 미션 Fort 적재와 Edit 진입은 F3까지 끝나야 동작하며, TS·F 단계만으로는 화면 변화가 거의 없다.
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
- 즉시 적용 요구인 **11.01 표기·Credits**(0.150)와 **외부 접속 문서화·클라이언트 유효성 검사 비활성화**(0.151)는 완료했다(1절 색인). 외부 접속 문서의 갱신 규칙은 0.7을 따른다.
- **이월한 이식 한계(이후 단위에서 해소됐다는 기록이 없는 것만):**
  - 0.80: 초기 구름 카메라 원점은 임시 범위이고 실제 카메라(R5)·시간별 이동·부분 dirty는 미연결이다. 512×512 구름 파일이 없거나 크기가 다를 때의 원본 생성 fallback은 이식하지 않아 명시 오류다.
  - 0.81: 반복 `--set`은 C# 호스트가 병합하고 같은 이름은 마지막 값을 쓰지만 cpppj는 마지막 묶음만 보관한다. 오라클 연결 때 계약 차이를 정리한다.
  - 0.86·0.87: 프레임 사이의 빠른 물리 입력 이력, 공통 음향·음악(R6 범위)과 옵션 변경 중 재생 관리가 남았다.
  - 0.93: 부분 생성 중 돌 버튼 내부 배경 할당 실패의 모든 사례를 검증하지 않았다.
  - 0.126: cpppj와의 이관 장부 교차 읽기, `MatchDisk`의 Win32 특수 경계(절대 경로·긴 경로)는 검증하지 않았다. 실제 Editor State1 월드·저장·Save as의 Template/현재 이름 세션 소유자는 미이식이다(F3 범위).
  - 0.127: Windows 외의 `WriteAtomic`은 write-through·재시도가 없는 준비용 `File.Move`이며 durable 검증이 아니다. `ReadOnly`의 Windows 외 동작은 관측하지 않았다. 새 파일 장치의 이름 정규화(ASCII만 접음)와 기존 `GameFileSystem.NormalizePath`(비ASCII도 접음)의 차이는 고치지 않았다. cpppj `RelativePath`는 옮기지 않았다.
  - 0.128: 기존 `Netstorm.Assets.FortFile`은 연결하지 않은 이전 독립 판독기로 남아 있으며 은퇴는 R4 Fort 적재 단위에서 한다.
  - 0.132: `WriteStream`은 입력을 배열로 복사하므로 입력이 같은 섹션 버퍼의 구간이면 원본의 DWORD 순방향 복사와 다를 수 있다(현재 호출처에는 없음, 소비자를 옮길 때 확인).
  - 0.134: `AllocateEmptyManager`의 할당 실패(NULL) 경로는 이식하지 않았다. `CampaignMissionFortPathTests`·`CampaignLoginObjectStreamTests`·`CampaignMissionReloadSourceTests`와 `TemplateDiagnosticClientTests`의 Client 필터·FILE·공유 버퍼·NULL Chat 부분은 F3(`RawCampaignSession`·`ClientDiagnostics` 필요)에서 옮긴다.
  - 0.135: 짝이 맞지 않는 UTF-8(옛 ANSI) 이름은 manager에서 꺼낼 때 U+FFFD로 바뀌어 왕복하지 않는다. `Dump`의 파일명은 바이트 문자열 그대로 출력된다.
  - 0.136: `HostFortNameConfirmation`의 `header`·`setName` 실제 연결(현재 Template 소유자·전체 snapshot)은 F3이다. `FortCurrentNameDevices`의 두 dirty DWORD 주소 비교는 생략했다(호출자가 `ClientFortGlobals.Catalog`에 연결해야 함). cpppj는 비ASCII 이름을 CP1252→UTF-8로 이중 인코딩하는 차이가 있다. `SoloMapCatalogTests` 3개, `CampaignNewFortTests` 3개·`CampaignNewFortConfigTests` 1개는 옮기지 않았다(F3).
  - 0.137·0.138: `HostEditorSelection`의 TYPE 개수 188 상수 교체와 `ReadSections` 검사의 `Parse` 이식은 F3(세션 표 소유)이다. `from_chars` 지수 언더플로 거부는 MSVC 동작을 확인하지 못해 옮기지 않았다. `TypeRequiredEnergyTests`·`ArcherTypeDataTests`·`CanonTypeDecoder` 계열과 세션 통합 검사는 F2·F3에서 잇는다.
  - 0.139: `FortMoneyLoad_SharesCurrentWorldOwnerStoreAndRandom`은 `RawSurfaceWorld`가 필요해 F3에서 옮긴다.
  - 0.152: `LocalMissionTransport`는 바이트 큐 골격만 있고 검사가 없다. Zone·로그인·분배 검사는 F3 세션 연결 때, TCP/IP 소켓은 3차 로컬 구현 때 `LocalNetworkPolicy`의 루프백 전용으로 더한다.
  - 0.153: `AddRandom_Native288`의 대본 GameMenu 후반부(`QueueScriptCommand`·`GameMenuScriptCode`)는 R6에서 옮긴다.
  - 0.154: 판본 분기 잔여와 탐색기 발자국 조건 차이, 옮기지 않은 지형 Builder 계열 검사는 아래 0.154 기록의 '남은 것'과 'C#에서 달라진 모양' (6)을 따른다.
  - 0.175: `StepAmbientWind`는 `GameRandom` 미이식으로 옮기지 않았다 — 0.176에서 해소했다(`RawSurfaceStorage.UpdateAmbientWind`·F3 연결은 남는다).

### 0.10~0.153 — 이관

2026-10-28 문서 작업 기록과 단위 기록 0.11~0.65는 [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073)로, 단위 기록 0.66~0.126은 [이관 파일의 2026-11-02 절](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)로, 단위 기록 0.127~0.153은 [이관 파일의 2026-11-04 절](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154)로 옮겼다. 완료 범위는 1절 색인을 본다. 아래 0.154는 가장 최근 단위이고 다음 작업(F2c 나머지)의 직접 근거라 본문에 남겼다. 이관한 단위에서 해소되지 않은 한계는 0.9의 '이월한 이식 한계'로 옮겼다.

### 0.154 단위 기록 — F2c-11 섬 목록·단일 배치·군집·게임 보조·Notify (2026-11-04, HJOW-Athlon)

- **범위:** 섬 목록 1종(`RawIslandRuntime`과 `ResetRawIslandRecords`), 표면 조회 도우미 1종(`RawPathSurfaceAt`만, 전체 `RawPathSearch` 제외), 단일 배치 1종(`RawFortRandomPlacement`의 `Place`·`MayReplace`·`Retype`·`MakeFortSinglePlacementHook`), 군집 1종(`RawFarmCluster`와 `MakeFarmClusterPlacementHook`), 게임 보조 1종(`RawGameAuxState`), Notify 검사 2종(`FortNotifyTests`, 제품 `RawPlayerTemplateState`는 0.128 기이식) 및 cpp 검사 직역 3종(`IslandRuntimeTests` 8개·`FarmClusterTests` 4개). `MakeFortRandomPlacementHooks`(배치 파이프라인 미이식), `RawFortWorldLoad` 전체·`MakeFortWorldChildrenHooks`·`MakeFortWorldLoadBinding`(섬 Builder 계열 미이식, R4 지형 범위)은 제외했다.
- **구현(새 파일 5개, 모두 `Netstorm.Core/Original`):** `RawIslandRuntime.cs`(100개 32바이트 목록·owner 변경·Player 첫 지역·`RawIslandStorage`·진단·훅), `RawPathSurface.cs`(`RawPathPoint`는 기존 `RawSquidPoolReset.cs` 것을 빌리고 `Cell`·`Inside`·`SurfaceAt`만 옮겼다), `RawFortRandomPlacement.cs`(`CanonPlacementQuery` 최소 구조체·상태·훅·`Place`·`MayReplace`·`Retype`·단일 배치 연결), `RawFarmCluster.cs`(군도·시드·수량·실패 예산·군집 연결), `RawGameAuxState.cs`(헤더 전용 전역 묶음). **기존 파일 수정 4개:** `RawFinder.cs`(CD 매립 바이트 35·1 미만 좌표 assert), `RawSquidFactory.cs`(판본별 기본 가상 표·쓰기 표 조회·생성자 적용·최초 Pop 추가 비트 35), `RawPop.cs`(판본별 표시 지원·추가 비트 35·단계 위치 31), `OriginalPopDisplay.cs`·`OriginalUnpopDisplay.cs`(판본별 지원 overload). **생성기 3종 확장·자료 3종 재생성:** `gen_original_constructors.py`(CD 쓰기 표 71개)·`gen_original_pop_vtables.py`(CD 표 16개)·`gen_original_unpop_vtables.py`(CD 표 68개)와 `OriginalConstructors.Data.cs`·`OriginalPopDisplay.Data.cs`·`OriginalUnpopDisplay.Data.cs`(`--check` 통과).
- **C#에서 달라진 모양:** (1) `uint32_t&` 전역(revision·changed)은 읽기·쓰기 대리자 쌍으로 옮겼다(TS1 규칙). (2) `std::span` 기록은 `byte[]` 참조 보관으로 옮겼다. (3) C++ `Sid`는 `int` 슬롯 번호로 옮겼다. (4) `Ftol` 절삭·`Truncate`·float 비트 변환은 `unchecked`로 감김을 보존했다. (5) C# 풀·해시는 10.78 스트라이드(50바이트) 전용이므로 CD 장면도 50바이트 풀로 돌리고, CD raw 대조는 앞 36바이트로 자른다(F2c-2 선례). 생성자 188개 가드가 있는 `RawSquidFactory`에는 CD 장면에서 빈 기록 패딩 복사본을 쓴다(F2c-2 선례). (6) 탐색기 발자국 조건은 원문(`footX<0`) 대신 기존 C# 조건(`FootX<1`)을 유지했다. 기존 직접 검사(`RawFinderTests.FootprintValidationRejectsBadFeet`)가 0을 거부하므로 원문대로 바꾸지 않았으며 차이로 기록한다. (7) `CanonPlacementQuery`는 `o/RawCanonPlacement.h`의 형식만 옮겼다. (8) `FarmClusterState`의 공유 군도 포인터는 `Func&lt;uint&gt;` 대리자로 옮겼다.
- **검증:** 집중 22건 통과 — `IslandRuntimeTests` 8개(패치·CD 전수 1009행씩 4개 + 공유 Player 2개 + 직접 4개: Reset 대조·저장소 별칭·거부·실제 소유자), `FarmClusterTests` 5개(패치·CD 전수 104행씩 2개 + 거부 1개 + 공유 군도 104행 1개 + 게임 보조 직접 1개), `FortNotifyTests` 2개(1596행 전수 + 빈 이름 경계 직접 1개), `RawFinderTests` 직접 1개(CD 1 미만 좌표 assert·패치 계속), `RawSquidFactoryTests` 직접 6건(CD 기본 가상 표·CD 쓰기 적용·CD 추가 바이트 theory 4개). **불일치 0.** fixture 3개(`islandruntime` 2018행·`farmcluster` 208행·`fort-notify` 1596행)를 링크·등록했다(203개). Core 전체 Release **1,243개**, Client 전체 **858개**, Assets 전체 **366개** 통과, 실패·건너뜀 0. PORT_MAP 무결성 2개와 fixture 목록 검사도 통과했다. **옮기지 않은 cpp 검사:** `FortWorldLoadTests` 3종·`FortWorldLoadBindingTests` 1종·`NormalTerrainWorldBindingTests`·`IslandBuilderTests`·`IslandLifecycleTests`·`IslandPlacementTests`·`IslandPopulationTests`·`IslandPostPopTests`·`IslandQueriesTests`(R4 지형 Builder 계열 필요), `MakeFortRandomPlacementHooks`와 `RawPathSearch` 전체·`RawCanonPlacementPipeline` 계열(R6 규칙 필요), `FarmClusterTests`의 GameAux 104행은 옮겼다.
- **PORT_MAP:** 신규 8행(`RawIslandRuntime` 2행·`RawFortRandomPlacement` 2행 L1, `RawCanonPlacement.h` 1행 port, `RawFarmCluster` 2행 L1, `RawGameAuxState.h` 1행 port, 기준 HEAD `da27e191`). 기존 9행 기준 상향(`RawSquidFinder.cpp`·`SquidFactory.cpp`·`RawPlayerTemplateState` 2행·`SquidPop.cpp` 2행·`SquidUnpop.cpp` 1행·`TypeConstructors.inc`·`DerivedConstructors.inc`, 모두 L1 유지). `RawPathSurface.cs`는 대응 파일 없는 보조라 행이 없다(`Sid.cs` 선례). 467행(L1 263, port 204).
- **변이 검사:** 대표 6종 중 **5종을 검출**하고 **1종을 등가**로 판정했다(M1 섬 목록 모드 분기 제거 → 4개 실패로 검출, M2 지역 비교 반전 → 3개 실패로 검출, M3 탐색기 CD 매립 위치 patch 회귀 → CD만 실패로 검출, M4 생성자 CD 기본 가상 표 patch 회귀 → CD만 실패로 검출, M5 Pop 표시 CD 표 patch 회귀 → CD 1개 실패로 검출). M2 첫 시도(덩어리 좌표 `/16`→`/8`)는 균일 기록에서 관측이 같아 등가로 판정했다. 개발 중 검사 입력 오류 4건도 대조가 잡아 고쳤다. (1) 효과 기록 `text += ':' + value`의 문자 덧셈(콜론 누락, 효과 adler 불일치). (2) 같은 패턴의 바뀐 슬롯 번호 표기(`number + ':'`, 번호+58·콜론 누락). (3) 생성 슬롯 사전 확보 부족(10000번까지만 확보해 두 번째 생성부터 미할당 실패 → 고객 영역 끝까지 확보). (4) 신규 직접 검사의 번호 착각(새 풀의 첫 할당은 5번인데 50번으로 적음, 기존 검사 3건을 함께 바꾸었다가 되돌림). 별도 사본 운전은 쓰지 않고, 위 6종을 수동으로 제품·검사 빌드 뒤 `--no-build` 집중 실행으로 확인했다(각 변이는 실패로 검출 후 원복, 원복 뒤 집중 41개 통과를 재확인하고 전체 1,243개 통과를 최종 확인했다).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 보호 원본은 새 검사가 읽기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리에서 다른 세션이 cpppj를 수정·커밋 중이었고(작업 중 `21cf8caf`→`da27e191`, ritual·Take 계열) 이 단위는 HEAD의 F2c-11 파일만 읽었으며 그 파일들에 변경은 없었다(`git log`로 확인). `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. `cpp_sync.py --update --add`로 fixture 3개를 등록했고, `--ports`는 시간 초과로 직접 기준을 확인했다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규·상향 행의 기준만 HEAD).
- **남은 것:** F2c 나머지(`RawFortWorldLoad`·`RawFortWorldLoadBinding`, R4 지형 `RawIslandBuilder` 계열 필요)·F2d(World·저장 나머지)·F3 `RawCampaignSession` 연결부터다. **이월한 판본 분기 잔여(원문 근거는 있으나 이 단위 fixture·검사에서 관측되지 않아 두었다):** `RawSquidFactory.Take`·`PostCreate`의 나머지 CD 분기(당일 cpppj 98651032 Take 재작업과 함께 볼 것), `RawPop`의 무효 void CD throw·겹침 CD 항상 쓰기, `RawUnpop`의 추가 비트 CD 위치, `RawPostPop`·`RawUnpop`의 표시 지원 판본 overload 연결, `RawSquidFinder` 발자국 원문 조건(이 단위에서 기존 검사와 충돌 확인), `RawPathSearch` 전체·`RawCanonPlacementPipeline`·`MakeFortRandomPlacementHooks`(R6).

### 0.155 단위 기록 — F2c-12 Canon 디코더·섬 빌더/배치/검색·Fort 월드 적재 부모 및 UnpopVtables 동기화 (2026-11-07)

- **범위:** Canon 디코더 3종(`RawCheckingFlag`·`CanonDecoder`·`CanonDecoderTables`·`CanonTypeDecoder`), 섬 빌더/배치/검색 3종(`RawIslandBuilder`·`RawIslandPlacement`·`RawIslandQueries`), Fort 월드 적재 부모 1종(`RawFortWorldLoad`·`MakeFortWorldChildrenHooks`), `OriginalUnpopDisplay.Data.cs` 동기화(`0x0050da88U` 마스크 `1`→`3`), cpp 검사 직역 5종(`CanonTypeDecoderTests` 6개·`IslandBuilderTests` 4개·`IslandPlacementTests` 2개·`IslandQueriesTests` 3개·`FortWorldLoadTests` 3개) 및 기존 `X86RawUnpopTests.DisplaySupportTableMatchesOriginalVtables` 갱신. `RawFortWorldLoadBinding`은 `RawNormalTerrainWorldBinding`·`RawSurfaceWorld`가 필요해 제외했다.
- **구현(새 파일 8개, 모두 `Netstorm.Core/Original`):** `RawCheckingFlag.cs`(`RawCheckingFlagState`), `CanonDecoder.cs`(8×8 섹션 마스크 디코더·`CreateCanonDecoder`·`DecodeCanonMask`), `CanonDecoderTables.Data.cs`(64×15 압축 표와 오프셋 표), `CanonTypeDecoder.cs`(타입별 마스크·범위·`DecodeTypeCanonFootprint`·`ApplyTypeCanonToEntry`·27개 생성자 패턴 표), `RawIslandBuilder.cs`(원본 112바이트 `Islandbuilder`·`Initialize`·`PlaceLoadedPiece`·`PlaceTerrainPiece`·`FillPieceHashes`·`MakeLoadedIslandBuilderHook`), `RawIslandPlacement.cs`(`PlaceIslandOnSurface`·`PlaceLoadedIslandAt`·`MakeIslandPlacementBuilderHooks`), `RawIslandQueries.cs`(`NotFound=127`·`FindIslandInBox`·`FindIslandAtPoint`·`FindNearestIsland`·`FindEmptyIslandSlot`·`SetIslandSpotFlags`·`ClearIslandSpotFlags`·`FindBattleSpotInIsland`), `RawFortWorldLoad.cs`(`LoadFortWorld`·`LoadFortWorldChildren`·`MakeFortWorldChildrenHooks`). **기존 파일 수정 2개:** `OriginalUnpopDisplay.Data.cs`(`gen_original_unpop_vtables.py` 재생성), `X86RawUnpopTests.cs`.
- **검증:** 집중 19건 통과(`CanonTypeDecoderTests` 6개: `canontype-x86.tsv` 3927행·`canon-decoder-checking-x86.tsv` 156행 + 직접 4개, `IslandBuilderTests` 4개: `islandbuilder-x86.tsv` 1156행 패치/CD + 직접 2개, `IslandPlacementTests` 2개: `island-placement-x86.tsv` 4896행 패치/CD, `IslandQueriesTests` 3개: `islandqueries-x86.tsv` 8490행 + 직접 1개, `FortWorldLoadTests` 3개: `fortworldload-x86.tsv` 534행 + 직접 1개, `DisplaySupportTableMatchesOriginalVtables` 1개). fixture 6개를 링크·등록했다(총 207개, `cpp_sync.py` 0종료). 변이 검사로 `RawIslandQueries.NotFound`(`127u`→`126u`) 변이 시 `IslandQueriesTests` 3개 모두 실패로 검출됨을 확인하고 원복했다. Release 전체 **Core 1,261개, Client 858개, Assets 366개** 통과, 실패·건너뜀 0.
- **PORT_MAP:** 신규 16행(`CanonDecoder` 3행·`CanonTypeDecoder` 3행·`Islandbuilder` 2행·`RawCheckingFlag.h` 1행·`RawFortWorldLoad` 2행·`RawIslandBuilder` 2행·`RawIslandPlacement` 2행·`RawIslandQueries` 2행 중 L1 10행·port 6행), 기존 1행(`UnpopVtables.inc` → `fbc27346564e5245d36c00e0b151777204a22efe`) 기준 상향. 총 483행(L1 273, port 210).
- **남은 것:** R4 섬·지형 본체(`TerrainBuilder`·`RawSquidRegion`·`RawFrameChoose`·`RawTerrain*`·`RawIslandLifecycle`·`RawIslandPostPop`·`RawIslandPopulation`·`RawIslandAdd`·`RawFortPriestPreparation`·`RawBattleIsland`·`RawCentralIsland`·`RawIslandPartition`·`RawCanonPlacementTerrain`·`RawPriestPlacementTerrain`)와 `RawNormalTerrainWorldBinding`·`RawFortWorldLoadBinding`·F2d(World·저장 나머지)·F3 `RawCampaignSession` 연결.

### 0.156 단위 기록 — R4 지형·섬 생성 컨텍스트·성장·도장 및 섬 인구·추가·전투·중앙·군도 분리 1차 이식 (2026-11-07)

- **범위:** 지형 생성·도장·성장 기반 7종(`TerrainBuilder`·`RawSquidRegion`·`RawFrameChoose`·`RawTerrainBitmap`·`RawTerrainStamp`·`RawTerrainContext`·`RawTerrainGrowth`), 섬 인구·편집기 섬 추가·전투/중앙/군도 분리 5종(`RawIslandPopulation`·`RawIslandAdd`·`RawBattleIsland`·`RawCentralIsland`·`RawIslandPartition`) 및 cpp 검사 직역 6종(`TerrainContextTests` 2개·`TerrainGrowthTests` 2개·`TerrainArchHelpersTests` 4개·`TerrainSeedTests` 4개·`TerrainStampTests` 3개·`IslandPopulationTests` 2개).
- **구현(새 파일 12개, 모두 `Netstorm.Core/Original`):** `TerrainBuilder.cs`(원본 본섬 성장·통로·프린지 후보·간헐천/나무/다리 배치·8이웃 표·로컬 LCG 보조), `RawSquidRegion.cs`(`0043de40`/CD `0047ff90` 지역 슬롯 필드 기록·무효/범위 밖/비섬 진단), `RawFrameChoose.cs`(`0042ae70`/CD `004bb560` 초기 지형 및 다리 공유 최대 256 후보 추첨), `RawTerrainBitmap.cs`(256×256 1비트 비트맵·4바이트 정렬 스캔라인), `RawTerrainStamp.cs`(`004c19a0`·`004c1770` 초기 99칸 CRT 캐시·8이웃 variant 마스크·프린지 생성), `RawTerrainContext.cs`(`004c0ef0`·`004c15b0`·`004c16f0` 65536 문자/지역 지도·별칭/타입 매핑·테두리 표시·도장 후 정규화), `RawTerrainGrowth.cs`(`004c10a0` 일반 성장·통로·빈틈 보정·`FirstChunkSeed`·`Scatter`·`GrowRectangle`·`RemoveSparseCell`·`RemoveSparse`·`ClearPlacement`), `RawIslandPopulation.cs`(`0040e120`·`0040e340`·`0040e240` 거주 구역·비보스 간헐천·나무/식물 무작위 배치·초기 타입 치환), `RawIslandAdd.cs`(`0046f910` 편집기 섬 드롭·임시 충돌 검사·`PlaceIslandOnSurface`·지형 생성·그래프·영역 저장), `RawBattleIsland.cs`(`00470b10` 전투 섬 사각형 선택·나무 제거·영역 배정), `RawCentralIsland.cs`(`00470410`·`00470820` 중앙 섬 테두리 탐색·사각형 선택·성배/식물 배치), `RawIslandPartition.cs`(`00470100` 군도 잔여 조각 분리·BFS 라벨링·최소 크기/간격 검사). **기존 파일 수정 1개:** `RawCheckingFlag.cs`(함수 대리자 생성자 3종 추가).
- **검증:** 집중 17건 통과(`TerrainContextTests` 2개: `terraincontext-x86.tsv` 224행 + 직접 1개, `TerrainGrowthTests` 2개: `terraingrowth-x86.tsv` 108행 + 직접 1개, `TerrainArchHelpersTests` 4개: `terrain-arch-helpers-x86.tsv` 1734행, `TerrainSeedTests` 4개: `terrain-seed-x86.tsv` 378행 + 직접 2개, `TerrainStampTests` 3개: `terrainstamp-x86.tsv` 1560행 패치/CD + 직접 1개, `IslandPopulationTests` 2개: `island-population-x86.tsv` 118행 + `IslandAdd` 직접 1개). fixture 6개를 링크·등록했다(총 213개, `cpp_sync.py` 0종료). 변이 검사로 `RawTerrainGrowth.RunNormal`의 `FixGaps(2)`→`FixGaps(1)` 변이 시 `TerrainGrowth_ReplaysBothNativeFullArrays`가 즉시 실패로 검출함을 확인하고 원복했다. Release 전체 **Core 1,278개, Client 858개, Assets 366개** 통과, 실패·건너뜀 0.
- **PORT_MAP:** 신규 24행(`RawBattleIsland` 2행 port, `RawCentralIsland` 2행 port, `RawFrameChoose` 2행 L1, `RawIslandAdd` 2행 L1, `RawIslandPartition` 2행 port, `RawIslandPopulation` 2행 L1, `RawSquidRegion` 2행 L1, `RawTerrainBitmap` 2행 port, `RawTerrainContext` 2행 L1, `RawTerrainGrowth` 2행 L1, `RawTerrainStamp` 2행 L1, `TerrainBuilder` 2행 L1). 총 507행(L1 289, port 218).
- **남은 것(재개 후 0.157에서 사제 준비만 완료 — 다음 단위 0.158):** R4 지형·섬 생성 2차 나머지(`RawIslandLifecycle`·`RawIslandPostPop`·`RawCanonPlacementTerrain`·`RawPriestPlacementTerrain`·`RawTerrainRun`·`RawTerrainPipeline`·`RawNormalTerrainWorldBinding` 및 관련 x86 fixture 대조 검사), 이어서 `RawFortWorldLoadBinding`·F2d(World·저장 나머지)·F3 `RawCampaignSession` 연결.

### 0.157 단위 기록 — R4 Fort 사제 준비 이식 (2026-11-07, HJOW-Athlon)

- **범위:** Fort 사제 준비 1종(`RawFortPriestPreparation`의 `Run`·`Collect`·`Marked`) 및 cpp 검사 직역 1종(`FortPriestPreparationTests` 2개). `RawIslandLifecycle`·`RawIslandPostPop`·`RawCanonPlacementTerrain`·`RawPriestPlacementTerrain`·`RawTerrainRun`·`RawTerrainPipeline`·`RawNormalTerrainWorldBinding`은 제외했다.
- **구현(새 파일 1개, `Netstorm.Core/Original`):** `RawFortPriestPreparation.cs`(PATCH 004923e0 기본 기술 4종·현재 관문 재조회·지역 20개 원문 순서 처리, 00492120 현재 목록 재조회·유한 용량 계약, 004921a0 첫 실제 순서 비트 0x80 검사, 현재 DWORD 번호 물리 범위·리틀 엔디언 DWORD·미연결 장치 거부 보조). **기존 파일 수정 1개:** `Netstorm.Core.Tests.csproj`(fixture 링크 1행 추가).
- **검증:** 집중 2건 통과(`FortPriestPreparationTests.Native13ActualParentAndListHelpers`: `fort-priest-preparation-x86.tsv` 13행 전수 + `SelectedFailureKeepsEarlierTechnologyEffects` 직접 1개). fixture 1개를 링크·등록했다(총 214개, `cpp_sync.py` 0종료). 변이 검사로 `Marked`의 `0x80u`→`0x40u` 변이 시 전수 검사가 즉시 실패로 검출함을 확인하고 원복했다. Release 전체 **Core 1,280개, Client 858개, Assets 366개** 통과, 실패·건너뜀 0. PORT_MAP 무결성 2개와 fixture 목록 검사 포함 6개도 통과했다. `--ports`는 기존처럼 시간 초과라 단위가 읽는 cpppj 파일의 `git log`로 기준을 직접 확인했다.
- **PORT_MAP:** 신규 2행(`RawFortPriestPreparation.cpp/.h` → `RawFortPriestPreparation.cs`, 모두 L1, 기준 `b8ce42c7`). 총 509행(L1 291, port 218).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 보호 원본은 새 검사가 읽기·쓰기 모두 풀 범위 안에서만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리에서 다른 세션의 cpppj 미커밋 변경(`cpppj/tests/SubscriberGlobalsTests.cpp` 1개)은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2c 나머지·F2d·F3부터다. `RawFortWorldLoadBinding`(R4 지형 `RawNormalTerrainWorldBinding` 및 `RawSurfaceWorld` 필요), F2d World·저장 나머지(`RawSurfaceWorld` 63KB·`RawSurfaceStorage` 등 월드 스택), F3 `RawCampaignSession` 연결 순이다. `RawIslandPostPop`·`RawIslandLifecycle`의 x86 전수 재생(`islandpostpop-x86.tsv` 1988행·`islandlifecycle-x86.tsv` 2055행)은 월드 스택 하네스(생성·소유자·Pop 실제 모듈)가 필요해 F3 실제 조립 때 잇는다. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 지시에 따라 건너뛴다. 멀티플레이는 3차 로컬 구현 때 `LocalNetworkPolicy` 루프백 전용으로 사전 구현만 잇는다.

### 0.213 단위 기록 — 공장 부착 배선 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 공장 부착 배선 7종(분리·전송·종류·깊이 보관 3종·현재 Pop 연결·부착·분리·직접 Pop 본체 4종)과 Host 분리 공장 경로 2종 및 직접 검사 9종. Host 부착 본체(`Attach`·`RefreshForm`·`AddRegular`)·공장 진단 묶음·분리 종단 재작업은 후속 단위에서 잇는다.
- **구현(기존 파일 수정 2개·새 검사 1개):** `Netstorm.Core/Original/RawSquidFactory.cs` 확장(대여 4종·현재 Pop 연결·부착·분리·직접 Pop), `RawSquidProcessHost.cs` 확장(런타임·직접 공장 분리 연결), 검사 `RawFactoryAttachTests.cs`(직접 7개: 깊이 계약·현재 Pop 계약·공유·genus·전송·분리·직접 Pop)와 `RawSquidProcessHostTests.cs` 확장(직접 2개: 직접·런타임 공장 분리). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 원문 공유 깊이 `uint32_t&`는 읽기·쓰기 공유 상자(`UIntBox`)로 옮겼다(0.168 규칙). (2) 빈 대여(null 분리·종류·전송)는 미연결로 두고 호출 때 기본값을 쓴다(원문 빈 `std::function` 그대로). (3) 분리 종단 설정은 명시 오류로 옮겼다(결정 6 — `Unpop` 종단 재작업에서 잇는다). (4) 공장 진단 묶음은 두지 않았다(`Take`를 포함한 기존 경로가 진단 없이 동작하므로, 필요해지면 `BindDiagnostics`로 잇는다).
- **정정:** 0.205 기록의 검사 수(호스트 직접 39개·프로세스 8개)는 호스트 38개·프로세스 9개가 맞다(집중 47개 합계는 그대로다). 이 단위에서 호스트 39개·공장 7개·프로세스 9개를 재확인했다.
- **검증:** 집중 50건 통과(공장 직접 7개·호스트 39개 + 무결성·매니페스트 4개). **불일치 0.** 변이 검사로 깊이 재연결 검사 삭제 변이 시 계약 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.211 재기준선 1,576개가 마지막이며 변경이 기존 2개 확장+신규 검사 1개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `SquidFactory.h/.cpp`·`SquidProcess.h/.cpp` → 각 C# 행의 본체 범위 확장). 총 613줄 유지(자료 608행: L1 329, port 279).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·슬롯·커널 범위 안의 대여·부착·분리만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(Host 부착 본체·월드 본체 조립)·F3부터다(0.9절 참조).

### 0.212 단위 기록 — Form Pop 바인딩 이식 (2026-11-07, HJOW-Athlon)

- **범위:** Form Pop 바인딩 3종(`FormPopHooks`·`PopCommonForm`·`BindPop`·바인딩 위임의 풀 기본값) 및 직접 검사 6종. 공장·Host 부착(`factory.AttachForm`·`Host.Attach` 계열)은 후속 단위에서 잇는다.
- **구현(새 파일 2개·기존 파일 수정 1개):** `Netstorm.Core/Original/RawFormPop.cs`(6장치 묶음·void 해제·플래그 보완·고정 전송·깊이 증감·VT 선택·잔량 대조), 검사 `RawSquidFormsPopTests.cs`(직접 6개: 빈 묶음 거부·빈 바인딩 기본값·고정 전송·전송 누락 거부·깊이 불일치 보고·미복원 표 거부). **기존 파일 수정 1개:** `RawSquidForms.cs`(바인딩 보관·지역 깊이·진입·위임). PORT_MAP·fixture 변경은 행 2개뿐이다.
- **C#에서 달라진 모양:** (1) 원문 이동 보관(`std::move`)은 복제 보관(`Clone`)으로 옮겼다(0.210 선례). (2) 바인딩 전에는 기존 직접 순서를 그대로 둔다 — 직접 검사(`PopFlagCompletionAndTransmitBoundary`)가 선행 보완을 고정하므로, 두 경로 통합은 그 검사 개정과 함께 후속에서 판단한다(0.210 선례). (3) 지역 깊이는 `uint` 필드로 옮겼다(DWORD 감김 보존). (4) `Sid`·고정 인자는 int·uint 리터럴로 옮겼다.
- **검증:** 집중 43건 통과(Pop 직접 6개·Forms 회귀 33개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 빌드 오류 2건을 빌드가 잡아 고쳤다(바이트 연산 추론·검사 오타). 변이 검사로 권한 기본값 삭제 변이 시 직접 3개가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.211 재기준선 1,576개가 마지막이며 변경이 기존 1개 확장+신규 2개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`RawFormPop.h/.cpp` → `RawFormPop.cs`, 모두 port, 기준 `09ad72b0`). 총 613줄(자료 608행: L1 329, port 279). 대응 검사가 직접 대조라 L1이 아니다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·슬롯 범위 안의 생성·Pop만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(공장 부착·Host 부착·월드 본체 조립)·F3부터다(0.9절 참조).

### 0.211 단위 기록 — 정상 바인딩 껍데기 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 정상 바인딩 껍데기 1종(`RawNormalTerrainWorldBinding`의 생성 계약·검사·개수·기록·후처리 위임) 및 직접 검사 4종과 Core 전체 재기준선(0.176 이후 첫 완주). 생성·도장·군집 조립(`Generate` 계열)은 월드 조립 때 잇는다.
- **구현(기존 파일 수정 2개):** `Netstorm.Core/Original/RawNormalTerrainBinding.cs` 확장(소유 고정 껍데기·공유 조각 상자·진단 복제·검사·위임 5종), `RawNormalTerrainBindingTests.cs` 확장(장면·바인딩 도우미와 직접 4개: 생성 계약·정적 일치·공유 번호 실시간·진단 복제). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 원문 `uint32_t&` 조각 번호는 읽기·쓰기 공유 상자(`UIntBox`)로 옮겼다(0.168 규칙). (2) 원문 이동 진단 묶음은 복제 보관(`Clone`)으로 옮겼다(호출자 mutant 방지). (3) 월드 소유 대신 호출자가 같은 실제 저장소를 직접 넘긴다(바인딩 본체 단위에서 owner 고정으로 잇는다, 0.200 선례).
- **검증:** 집중 18건 통과(바인딩 직접 18개: 기존 14개+신규 4개) 후 Core 전체 **1,576개** 통과(0.176의 1,416개 이후 첫 완주·새 기준선), 실패·건너뜀 0. 변이 검사로 생성 연결 검사 삭제 변이 시 계약 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다(원복 뒤 전체 1,576개로 확정). Release 솔루션 빌드 경고·오류 0.
- **PORT_MAP:** 변경 없음(이미 port인 `RawNormalTerrainWorldBinding.h/.cpp` → `RawNormalTerrainBinding.cs` 행의 본체 범위 확장, 기준 `2d50804c`). 총 611줄 유지(자료 606행: L1 329, port 277).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값·표 범위 안의 생성·위임만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(공장 부착·Host 부착·월드 본체 조립)·F3부터다(0.9절 참조).

### 0.210 단위 기록 — Forms 부착 바인딩 이식 (2026-11-07, HJOW-Athlon)

- **범위:** Forms 부착 바인딩 2종(`BindAttach` 보관·바인딩 위임의 풀 기본값·공통 몸체 호출) 및 직접 검사 5종. 공장·Host 부착(`factory.AttachForm`·`Host.Attach` 계열)은 후속 단위에서 잇는다.
- **구현(기존 파일 수정 1개·새 검사 1개):** `Netstorm.Core/Original/RawSquidForms.cs` 확장(바인딩 보관·장치별 풀 기본값·공통 몸체 위임), 검사 `RawSquidFormsAttachTests.cs`(직접 5개: 빈 묶음 거부·빈 바인딩 기본값·genus 보고·content 일치 보고·장치 관측). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 원문 이동 보관(`std::move`)은 복제 보관(`Clone`)으로 옮겼다(호출자 mutant 방지, 기존 묶음 규칙). (2) 바인딩 전에는 기존 직접 순서를 그대로 둔다 — 직접 검사(`AttachRejectsInvalidParents`)가 선행 거부를 고정하므로, 두 경로 통합은 그 검사 개정과 함께 후속에서 판단한다. (3) 풀 기본값은 `RawContainedLink` 현재 배치 조회·공통 `Pop`으로 옮겼다(원문 기본값 그대로).
- **검증:** 집중 37건 통과(부착 직접 5개·Forms 회귀 28개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사·빌드 오류 2건을 잡아 고쳤다(`SidAllocation` 이름 공간 누락 — `Simulation` using 추가, 바인딩 경로 미반영 — 기존 검사 유지 확인). 변이 검사로 genus 기본값 삭제 변이 시 직접 3개가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 1개 확장+신규 검사 1개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `SquidForms.h/.cpp` → `RawSquidForms.cs` 행의 본체 범위 확장). 총 611줄 유지(자료 606행: L1 329, port 277).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·슬롯 범위 안의 생성·부착만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(공장 부착·Host 부착·월드 본체 조립)·F3부터다(0.9절 참조).

### 0.209 단위 기록 — Fort 적재 바인딩 이식 (2026-11-07, HJOW-Athlon)

- **범위:** Fort 적재 바인딩 1종(`MakeFortWorldLoadBinding`의 풀 일치·Money 선택 관문·자식 연결·생성 위임·Money 지연 선택·Graph 재구성 위임) 및 직접 검사 6종. 정상 바인딩 본체(`GenerateOriginal` 고정)·월드 본체 조립은 후속 단위에서 잇는다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawFortWorldLoadBinding.cs`(같은 풀 정체성·Money 선택 누락 거부·자식 훅 복제 연결·생성 대리·지연 Money 선택·Graph 위임), 검사 `FortWorldLoadBindingTests.cs`(직접 6개: 풀·선택 관문 2종·장치 거부·자식 설치·Money 지연·생성 전달·Graph 위임). fixture 변경 없음.
- **C#에서 달라진 모양:** (1) 원문 월드 참조는 호출자가 같은 풀·그래프를 직접 넘긴다(바인딩 본체 단위에서 owner 고정으로 잇는다, 0.200 선례). (2) 정상 생성 호출은 호출자 대리자로 넘긴다(`GenerateOriginal` 미이식, 월드 조립 때 고정). (3) C++ 참조 캡처 수명은 GC 공유로 옮겼다(호출자 수명 보장은 주석에 명시). (4) 자식 연결은 복제본에만 쓰고 입력 묶음을 바꾸지 않는다.
- **검증:** 집중 10건 통과(바인딩 직접 6개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사 오류 1건을 빌드가 잡아 고쳤다(`TechnologyParent`의 `Sid` 반환 — `new Sid(0)`으로 정정). 변이 검사로 풀 일치 반전(`!` 삭제) 변이 시 전체 6개가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`RawFortWorldLoadBinding.h` 기준 `860030e7`·`.cpp` 기준 `2d50804c` → `RawFortWorldLoadBinding.cs`, 모두 port). 총 611줄(자료 606행: L1 329, port 277). 대응 검사가 직접 대조라 L1이 아니다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·깊이 범위 안의 생성·위임만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(할당 배선·정상 바인딩 본체·월드 본체 조립)·F3부터다(0.9절 참조).

### 0.208 단위 기록 — 로컬 미션 패킷 사전 구현 (2026-11-07, HJOW-Athlon)

- **범위:** Zone·로그인 패킷 작성 3종(`MakeLocalMissionZone`·`MakeGuideMissionZone`·`MakeLocalMissionLogin`)·판독 1종(`ReadLocalMissionLogin`)·보조 5종(상수·`Read`·`Put`·`Text`·`Copy`·`Packet`) 및 사용자 지시에 따른 검사 생략. 설치·분배기 연결(`InstallLocalMissionZone`·`Dispatch`·연결대·수신대)은 F3 세션 연결 때 잇는다.
- **구현(기존 파일 수정 1개):** `Netstorm.Net/LocalMissionTransport.cs` 확장(`LocalMissionPackets` 정적 클래스 — 512 예약·네 슬롯·86 확정·안내 한 슬롯·Fort 이미지 길이 확정·뷰 범위 검증). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 원문 `string_view`는 NUL 절삭 `string`+Latin-1로 옮겼다(원문 비ASCII 보존 선례). (2) 원문 span 반환은 배열 복사·`ReadOnlySpan` 뷰로 옮겼다(대리자 인자·클래스 필드 제약). (3) 원문 `out_of_range`·`invalid_argument`는 `ArgumentOutOfRangeException`·`ArgumentException`으로 옮겼다. (4) `Need`·퇴역 CD 검증 상수·설치 몸체는 분배기 단위에서 잇는다.
- **검증:** 사용자 지시로 검사를 두지 않고 Release 솔루션 빌드(경고·오류 0)로만 확인했다. 변경이 `Netstorm.Net` 1개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `LocalMissionTransport.h/.cpp` → `LocalMissionTransport.cs` 행의 본체 범위 확장, 기준 `108f76cc`). 총 609줄 유지(자료 604행: L1 329, port 275).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 코드는 패킷 바이트 범위 안의 읽기·쓰기만 한다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았고(`RequireLoopback` 경계를 그대로 둔다), 외부 도메인·IP 접속 기능은 건드리지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** 멀티플레이 나머지는 3차 로컬 구현 때 `LocalNetworkPolicy` 루프백 전용으로 잇는다. F2d 나머지(할당 배선·월드 본체 조립)·`RawFortWorldLoadBinding`·F3은 0.9절 순서를 따른다.

### 0.207 단위 기록 — 군집 후처리 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 군집 후처리 1종(`PostprocessCurrentFarms`의 저장소 검사·섬 순회·선택 필터·첫 기록·범위·사각·개수·배치 위임) 및 직접 검사 4종. 생성·도장 본체(`Generate`·`StampContext`·`BindGenerationInputs`·`GenerateOriginal`)는 월드 조립 때 잇는다.
- **구현(기존 파일 수정 2개):** `Netstorm.Core/Original/RawNormalTerrainBinding.cs` 확장(저장소 검사·unsigned→signed 개수·선택 필터·첫 기록 재사용·양판본 Bounds 부재·CD Bounds 못 찾음·16배 사각·개수·배치 위임), `RawNormalTerrainBindingTests.cs` 확장(배치 기록·세 섬 실행 도우미와 직접 4개: null 거부·선택 필터·사각/개수·판본별 진단 순서). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 원문 `bit_cast<int32_t>` 개수 해석은 `unchecked((int))`로 옮겼다(회마다 다시 읽는다). (2) 군집 몸체 소유 대신 호출자가 같은 배치 효과를 대리자로 넘긴다(바인딩 본체 단위에서 owner 고정으로 잇는다, 0.200 선례). (3) 원문 필수 후처리 진단은 null이면 건너뛰는 선택 대리자로 옮겼다(0.174 진단 묶음 규칙).
- **검증:** 집중 18건 통과(바인딩 직접 14개 + 무결성·매니페스트 4개). **불일치 0.** 변이 검사로 선택 필터 반전(`!=`→`==`) 변이 시 후처리 3개가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 1개 확장+검사 1개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `RawNormalTerrainWorldBinding.h/.cpp` → `RawNormalTerrainBinding.cs` 행의 본체 범위 확장, 기준 `2d50804c`). 총 609줄 유지(자료 604행: L1 329, port 275).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·청크 범위 안의 생성·조회·위임만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(할당 배선·월드 본체 조립)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.206 단위 기록 — 서피스 월드 껍데기 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 서피스 월드 껍데기 1종(`RawSurfaceWorld`의 생성 계약·판본/지형/표 선택·저장소 빌림·자동 생성) 및 직접 검사 4종. 적재·표시·삭제·프로세스 본체 조립(`Load`·`Core`·`RunFrame` 등)은 후속 단위에서 잇는다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawSurfaceWorld.cs`(TYPE 빈 표 거부·같은 표 공유·저장소 빌림·없으면 같은 판본 자동 생성), 검사 `RawSurfaceWorldTests.cs`(직접 4개: 생성 계약·선택 반환·자동 생성·공유 저장소). fixture 변경 없음.
- **C#에서 달라진 모양:** (1) 원문 `shared_ptr` 공유 소유는 같은 참조 보관으로 옮겼다(GC가 수명을 맡는다). (2) 원문 span 자산 입력(frames·shapes·links)은 껍데기에 두지 않고 조립 단위에서 잇는다. (3) 미조립 본체(`Core` 등)는 두지 않았다(0.188 호스트 껍데기 선례 — 명시 오류 대신 범위 축소).
- **검증:** 집중 8건 통과(월드 직접 4개 + 무결성·매니페스트 4개). **불일치 0.** 변이 검사로 지형 번호 고정(`_terrainType`→`0u`) 변이 시 선택 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`RawSurfaceWorld.h/.cpp` → `RawSurfaceWorld.cs`, 모두 port, 기준 `b8cadde1`). 총 609줄(자료 604행: L1 329, port 275). 대응 검사가 직접 대조라 L1이 아니다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값 범위 안의 생성·조회만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(할당 배선·월드 본체 조립)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.205 단위 기록 — Host 정기 실행 이식 (2026-11-07, HJOW-Athlon)

- **범위:** Host 정기 실행 2종(`RunRegular`의 부모·시각·추상 관문·처리기 호출·재예약·삭제, `RegularProcess.RunFrame`의 Host 배선)과 내부 접근자 2종(`SquidProcess.KernelId`·`RegularProcess.ApplyHandlerResult`) 및 직접 검사 10종. `RefreshForm`·`AddRegular`·다리 낙하 예약·할당 배선은 후속 단위에서 잇는다.
- **구현(기존 파일 수정 3개):** `Netstorm.Core/Original/RawSquidProcessHost.cs` 확장(빈·죽음·미래·void·추상 관문·간헐천 당김·대여 유지 호출·삭제 판정·재예약·0 삭제), `RawSquidProcess.cs` 확장(커널 번호 내부 제공·결과 일괄 적용·연결 시 실행 배선), `RawSquidProcessHostTests.cs` 확장(직접 10개: 죽음 부모·미래/void·간헐천 당김·양수 재예약·0 삭제·패치 NaN·CD NaN 무시·추상 판본 분기·처리기 삭제·RunFrame 연결). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 원문 필드 직접쓰기(`payload_`·`time_`·`count_`)는 원문 순서 그대로 두는 내부 일괄 적용으로 옮겼다(0.190 SetIdentity 선례). (2) 원문 `pid_` 읽기는 `protected`를 열지 않고 내부 getter로 옮겼다. (3) 원문 NaN 판정은 `float.IsNaN`으로 옮겼다(CD판은 재예약하지 않는다). (4) 미연결 `RunFrame`은 명시 오류를 유지한다(결정 6 — 0.185 선례).
- **검증:** 집중 51건 통과(호스트 직접 39개·프로세스 8개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사 오류 1건을 실행이 잡아 고쳤다(처리기 교체 뒤 옛 스파이 관측 — 교체 처리기 자체 관측으로 정정). 변이 검사로 0 비교 완화(`>`→`>=`) 변이 시 삭제 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 2개 확장+검사 1개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `SquidProcess.h/.cpp` → `RawSquidProcessHost.cs`·`RawSquidProcess.cs` 행의 본체 범위 확장, 기준 `6fefcff5`). 총 607줄 유지(자료 602행: L1 329, port 273).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·슬롯·커널 범위 안의 생성·호출·재예약만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(`RefreshForm`·`AddRegular`·할당 배선·월드 본체 조립)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.204 단위 기록 — Host 삭제 본체 이식 (2026-11-07, HJOW-Athlon)

- **범위:** Host 삭제 본체 3종(`UnpopForm`의 런타임·직접·공통 분기, `Kill`의 권한·번호 관문·공통 삭제, `Hooks`의 선택·분리·효과 분배) 및 직접 검사 9종. 정기 실행(`RunRegular`·`RunFrame` 배선)·`RefreshForm`·`AddRegular`·다리 낙하 예약은 후속 단위에서 잇는다.
- **구현(기존 파일 수정 2개):** `Netstorm.Core/Original/RawSquidProcessHost.cs` 확장(대여 확정·공장 미연결 거부·Form 공장 명시 오류·공통 해제 fallback·권한·번호·죽음 관문·선택 기본·효과 분기), `RawSquidProcessHostTests.cs` 확장(직접 9개: 공통 해제·런타임 무공장·직접 공장 명시 오류·무권한 생략·불량 폼 거부·뿌리 삭제·선택 기본·방출 전달/거부·분리 배선). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 원문 `RuntimeCall` 스택 수명은 `RuntimeCallGuard`로 옮겼다(0.190 선례). (2) C# 공장에 Form 분리가 없어 공장 분기는 명시 오류로 옮겼다(결정 6 — `factory.UnpopForm`·`BindFormDetach` 이식 때 잇는다). (3) 독립 Host fallback은 같은 공통 해제(`RawSquidForms.Unpop`)로 옮겼다(원문 `SquidForms` fallback 그대로). (4) `Sid`·`ProcessId`는 int 슬롯·uint 번호로 옮겼다(기존 규칙).
- **검증:** 집중 33건 통과(호스트 직접 29개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사 장면 오류 3건을 실행이 잡아 고쳤다(가상 표 바이트 누락 — 타입 2 생성자 표 기록, 무권한 장면의 Boss 초기값 — `Boss=false` 별도 장면, Form 뿌리의 방출 경로 — 커널 등록·PID 기록 후 커널 제거 관측). 변이 검사로 죽음 검사 삭제 변이 시 거부 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 1개 확장+검사 1개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `SquidProcess.h/.cpp` → `RawSquidProcessHost.cs` 행의 본체 범위 확장, 기준 `6fefcff5`). 총 607줄 유지(자료 602행: L1 329, port 273).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·슬롯·커널 범위 안의 생성·조회·삭제만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(Host 정기 실행·`RefreshForm`·`AddRegular`·할당 배선·월드 본체 조립)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.203 단위 기록 — 첫 기록 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 첫 기록 1종(`FirstRecord`의 섬 부재·Y 바깥 X 안쪽 순회·부호 지역 비교·CD 전용 후처리·영 기록 반환) 및 직접 검사 4종. 생성·도장·군집 본체(`Generate` 계열)는 후속 단위에서 잇는다.
- **구현(기존 파일 수정 2개):** `Netstorm.Core/Original/RawNormalTerrainBinding.cs` 확장(청크·섬 null 거부·CD 부재 선보고·Y 바깥 X 안쪽 이중 순회·sbyte 부호 비교·6바이트 복사 반환·CD 못 찾음 후보고·빗나감 영 기록), `RawNormalTerrainBindingTests.cs` 확장(직접 4개: null 거부·Y 바깥 순서와 복사본 독립·CD 전용 후처리 순서와 패치 조용·부호 비교). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 원문 `int8_t` 캐스트 비교는 `unchecked((sbyte))` 부호 확장으로 옮겼다(0xFF는 -1이며 255와 만나지 않는다). (2) 원문 값 반환 `std::array<uint8_t,6>`은 호출자 쓰기가 지도를 바꾸지 않는 새 `byte[6]` 복사로 옮겼다. (3) 원문 필수 후처리 진단은 null이면 건너뛰는 선택 대리자로 옮겼다(0.174 진단 묶음 규칙). (4) 월드 소유 대신 호출자가 같은 실제 청크·섬·판본·진단을 직접 넘긴다(바인딩 본체 단위에서 owner 고정으로 잇는다, 0.200 선례).
- **검증:** 집중 14건 통과(바인딩 직접 10개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 제품 오류 1건을 빌드가 잡아 고쳤다(`PatternCount`의 반환·닫기 누락 — `return count;` 복원). 검사 편집 오류 1건을 기록 대조로 고쳤다(패턴 개수 전수 검사 삭제 — 복원 후 10개 유지). 변이 검사로 순회 순서 뒤바꿈(Y 바깥→X 바깥) 변이 시 순서 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 2개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `RawNormalTerrainWorldBinding.h/.cpp` → `RawNormalTerrainBinding.cs` 행의 본체 범위 확장, 기준 `2d50804c`). 총 607줄 유지(자료 602행: L1 329, port 273).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·청크 범위 안의 생성·조회만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(Host 실행·삭제 잔여·월드 본체 조립)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.202 단위 기록 — 지형 패턴 개수 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 패턴 개수 1종(`PatternCount`의 조각 범위·패턴 마스크·회전 디코더·유효 순회) 및 직접 검사 3종. 생성·도장·군집 본체(`Generate` 계열)는 후속 단위에서 잇는다.
- **구현(기존 파일 수정 2개):** `Netstorm.Core/Original/RawNormalTerrainBinding.cs` 확장(6바이트 기록 검사·영역 패턴 선택·회전 디코더 구성·유효 순회), `RawNormalTerrainBindingTests.cs` 확장(직접 3개: 범위 거부·빈 프레임 0·덮는 프레임 비빈 셀 일치와 독립 디코더 대조). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 원문 고정 6바이트 span은 길이 검사를 두는 `ReadOnlySpan<byte>`로 옮겼다(길이가 다르면 거부). (2) 월드 소유 대신 호출자가 같은 실제 프레임·판본·진단을 직접 넘긴다(바인딩 본체 단위에서 owner 고정으로 잇는다, 0.200 선례). (3) 68개 영역 패턴에 6비트 마스크(0~63)라 패턴 번호 범위 검사는 방어용으로 둔다.
- **검증:** 집중 10건 통과(바인딩 직접 6개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 빌드 오류 1건을 빌드가 잡아 고쳤다(`0x3fu`의 uint 추론 — `0x3f`로 정정). 변이 검사로 최초 칸 건너뜀(`for (;`→`for (Advance();`) 변이 시 일치 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 2개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `RawNormalTerrainWorldBinding.h/.cpp` → `RawNormalTerrainBinding.cs` 행의 본체 범위 확장, 기준 `2d50804c`). 총 607줄 유지(자료 602행: L1 329, port 273).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값·표 범위 안의 생성·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(Host 실행·삭제 잔여·월드 본체 조립)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조). cpp 장면값 재생(`{3,1,0,7,0,0}`→3, 실제 조각 프레임 필요)은 월드 조립 때 잇는다.

### 0.201 단위 기록 — 호스트 대여 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 호스트 대여 6종(`Host`·`ReclaimForAllocation`·`BindLegacyAllocationReclaim`·`Bind`·`Use`·`ClearPass`·`Returned`) 및 직접 검사 7종. 실행 본체(`RunRegular`)·삭제 본체(`Kill`·`Hooks`·`UnpopForm`)는 후속 단위에서 잇는다.
- **구현(기존 파일 수정 2개):** `Netstorm.Core/Original/RawProcessHostStorage.cs` 확장(장기 Host·단일 대여·증빙·legacy 회수), `RawProcessHostStorageTests.cs` 확장(직접 7개: Host 고정·대여 계약·증빙·guard·legacy·소진·반납). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 공유 callable 수명은 약한 참조로 옮겼다(GC가 거두면 조회 거부). (2) 이동 전용 증빙은 소비 표시로 옮겼다. (3) `Sid`는 int 슬롯 번호로 옮겼다(기존 규칙).
- **검증:** 집중 17건 통과(직접 13개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사 오류 1건을 실행이 잡아 고쳤다(호출 중 대여 수 assert 위치 — emit 안에서 확인). 변이 검사로 반납 해제 삭제 변이 시 대여 검사 2개가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 2개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `RawProcessHostStorage.h/.cpp` 행의 본체 범위 확장). 총 607줄 유지(자료 602행: L1 329, port 273).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·슬롯 범위 안의 생성·대여만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(Host 실행·삭제 잔여·월드 본체 조립)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.200 단위 기록 — 지형 바인딩 계약 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 바인딩 계약 4종(분기·범위·일반·저장 검사) 및 직접 검사 3종. 바인딩 본체(`Generate` 계열)는 후속 단위에서 잇는다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawNormalTerrainBinding.cs`(분기·범위·군도 거부·타입/지역/생성자 연결 검사), 검사 `RawNormalTerrainBindingTests.cs`(직접 3개: 분기/범위·군도·연결). fixture·링크·매니페스트 변경 없음(총 232개 유지).
- **C#에서 달라진 모양:** (1) 공유 스칼라 참조는 값형 필드를 직접 읽는다(바인딩 본체 단위에서 owner 고정으로 잇는다). (2) `Sid`·검사 상수는 기존 규칙·값을 쓴다. (3) 생성자 주소 대조는 기존 `OriginalConstructors.Address`를 쓴다.
- **검증:** 집중 7건 통과(직접 3개 + 무결성·매니페스트 4개). **불일치 0.** 변이 검사로 생성자 연결 검사 삭제 변이 시 연결 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`RawNormalTerrainWorldBinding.h/.cpp` → `RawNormalTerrainBinding.cs`, 모두 port — 계약 범위). 총 607줄(자료 602행: L1 329, port 273).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값 범위 안의 생성·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(Host 실행·삭제 잔여·월드 본체 조립)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.199 단위 기록 — 호스트 잔류 조회 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 호스트 잔류 조회 3종(`Owns`·`DrainRuntimeBindings`·`HasOnlyRegularProcesses`) 및 직접 검사 3종. 실행 본체(`RunRegular`)·삭제 본체(`Kill`·`Hooks`·`UnpopForm`)는 후속 단위에서 잇는다.
- **구현(기존 파일 수정 2개):** `Netstorm.Core/Original/RawSquidProcessHost.cs` 확장(같은 Host 포인터 판정·대여 일치 반납·Regular 판별), `RawSquidProcessHostTests.cs` 확장(직접 3개: 소유 판정·비정기 guard·대여 반납). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 원문 `dynamic_cast` 검사는 `is`·`as` 형변환으로 옮겼다. (2) 역참조 증빙 수명은 GC가 맡고 대여 일치는 참조 비교로 둔다. (3) `Sid`는 int 슬롯 번호로 옮겼다(기존 규칙).
- **검증:** 집중 24건 통과(호스트 직접 20개 + 무결성·매니페스트 4개). **불일치 0.** 변이 검사로 소유 판정 완화 변이 시 소유 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 2개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `SquidProcess.h/.cpp` → `RawSquidProcessHost.cs` 행의 본체 범위 확장). 총 605줄 유지(자료 600행: L1 329, port 271).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·슬롯 범위 안의 생성·조회만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(Host 실행·삭제 잔여·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.198 단위 기록 — Form 삭제 통지 이식 (2026-11-07, HJOW-Athlon)

- **범위:** Form 삭제 통지 2종(`FormPreDestroy`·`FormParentDying`) 및 직접 검사 4종. `Kill`·`Hooks`·`UnpopForm`은 후속 단위에서 잇는다.
- **구현(기존 파일 수정 2개):** `Netstorm.Core/Original/RawSquidProcessHost.cs` 확장(통지→재조회 제거→완료 순서와 빈 조회 생략), `RawSquidProcessHostTests.cs` 확장(직접 4개: 통지/제거/완료·PID 재조회·거부·부모 통지). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 원문 DWORD 재조립은 낮은 바이트부터 합친다(0.194 선례). (2) 비어 있지 않은 다른 커널 클래스는 빈 성공으로 숨기지 않는다(원문 그대로 거부). (3) `Sid`는 int 슬롯 번호로 옮겼다(기존 규칙).
- **검증:** 집중 21건 통과(호스트 직접 17개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사 장면 오류 2건을 실행이 잡아 고쳤다(이종 판정의 파생 상속 오해 — 일반 기본 프로세스로 정정). 변이 검사로 PID 재조회 삭제 변이 시 재조회 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 2개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `SquidProcess.h/.cpp` → `RawSquidProcessHost.cs` 행의 본체 범위 확장). 총 605줄 유지(자료 600행: L1 329, port 271).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·슬롯 범위 안의 생성·조회만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(Host 실행·삭제 잔여·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.197 단위 기록 — 호스트 통지·검색 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 호스트 통지·검색 4종(`NotifyDependency`·`Find`·`FindEvent`·`FindEventMasked`·`IsProcessForm`) 및 직접 검사 4종. 실행 본체(`RunRegular`)·삭제 본체(`Kill`·`Hooks`)는 후속 단위에서 잇는다.
- **구현(기존 파일 수정 2개):** `Netstorm.Core/Original/RawSquidProcessHost.cs` 확장(서버 관문·PID 통지·체인 순회·가상 표 판정), `RawSquidProcessHostTests.cs` 확장(직접 4개: 통지·체인·거부·가상 표). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 종속 체인 무한 순회는 개수 guard로 옮겼다(같은 throw·위치는 다르다). (2) `dynamic_cast` 검사는 `as` 형변환으로 옮겼다. (3) `Sid`·검사 상수는 기존 규칙·값을 쓴다.
- **검증:** 집중 17건 통과(호스트 직접 13개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사 장면 오류 2건을 실행이 잡아 고쳤다(찾는 슬롯의 자산 분류·순환 검사의 일치 Lines). 변이 검사로 서버 관문 삭제 변이 시 통지 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 2개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `SquidProcess.h/.cpp` → `RawSquidProcessHost.cs` 행의 본체 범위 확장). 총 605줄 유지(자료 600행: L1 329, port 271).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·슬롯 범위 안의 생성·조회만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(Host 실행·삭제 본체·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.196 단위 기록 — Content 부착 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 정렬 부착 2종(`ContentAttachHooks`·`AttachSortedContent`) 및 cpp 검사 직역 1종+직접 1종(41행 전수·판본 거부). Host·정렬 소비자(나머지 2종)는 후속 단위에서 잇는다.
- **구현(새 파일 2개, 기존 파일 수정 1개):** `Netstorm.Core/Original/RawContentAttach.cs`(사망 진단·정렬 삽입·꼬리 Pop·통지 순서), 검사 `RawContentAttachTests.cs`(전수 1개: 41행 사건·내용·슬롯 대조, 직접 1개). **기존 파일 수정 1개:** `Netstorm.Core.Tests.csproj`(fixture 링크 1행). fixture 1개를 링크·등록했다(총 232개, `cpp_sync.py` 0종료).
- **C#에서 달라진 모양:** (1) 풀 const_cast 조회·쓰기는 같은 어셈블리의 `Slot`·`RawSlot`으로 옮겼다. (2) span 지역 캡처 금지는 풀 재조회로 옮겼다. (3) 원문 `std::string` 던지기는 검사 훅의 예외 메시지로 옮겨 사유를 대조한다. (4) 멈춤 표식 구조체는 검사 전용 예외로 옮겼다(원문 관측 표식).
- **검증:** 집중 6건 통과(전수 41행 1개+직접 1개+무결성·매니페스트 4개). **불일치 0.** 개발 중 검사 훅 오류 1건을 실행이 잡아 고쳤다(프로파일 변이 누락 — 원문과 대조해 보완). 변이 검사로 사망 조기 반환(`false`→`true`) 변이 시 전수 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개·링크 1행뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`RawContentAttach.h/.cpp` → `RawContentAttach.cs`, 모두 L1 — 41행 전수 대조 근거). 총 605줄(자료 600행: L1 329, port 271).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀 범위 안의 읽기·쓰기·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(Host 본체·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.195 단위 기록 — Form 부착 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 공통 부착 2종(`FormAttachHooks`·`AttachCommonForm`) 및 cpp 검사 직역 1종+직접 1종(864행 전수·부모 0 분기). Host 접합(나머지 4종, Host·월드 필요)은 후속 단위에서 잇는다.
- **구현(새 파일 2개, 기존 파일 수정 1개):** `Netstorm.Core/Original/RawFormAttach.cs`(부모 0 분기·부류 진단·setter·머리 연결·void 설정·Pop·꼬리 진단 순서), 검사 `RawFormAttachTests.cs`(전수 1개: 864행 사건·내용·슬롯 대조, 직접 1개). **기존 파일 수정 1개:** `Netstorm.Core.Tests.csproj`(fixture 링크 1행). fixture 1개를 링크·등록했다(총 231개, `cpp_sync.py` 0종료).
- **C#에서 달라진 모양:** (1) 풀 const_cast 조회·쓰기는 같은 어셈블리의 `Slot`·`RawSlot`으로 옮겼다. (2) span 지역 캡처 금지는 풀 재조회로 옮겼다. (3) 패치 파일 문자열의 드라이브 접두(`c:\ns\o\squid.h`)까지 그대로 둔다. (4) C# 풀 50바이트 전용이라 CD 합성 패턴·슬롯 대조를 판본 스트라이드(36)로 맞추고 앞부분으로 자른다(F2c-2 선례).
- **검증:** 집중 6건 통과(전수 864행 1개+직접 1개+무결성·매니페스트 4개). **불일치 0.** 개발 중 픽스처 파일명·열 구성 오해 2건을 실행이 잡아 고쳤다(실제 `form-attach-current-x86.tsv` 5열·CD 36바이트 패턴). 변이 검사로 부모 머리 쓰기 변이 시 전수 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개·링크 1행뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`RawFormAttach.h/.cpp` → `RawFormAttach.cs`, 모두 L1 — 864행 전수 대조 근거). 총 603줄(자료 598행: L1 327, port 271).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀 범위 안의 읽기·쓰기·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(Host 접합·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.194 단위 기록 — 호스트 조회 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 호스트 조회 3종(`FromForm`·`Word`·`CreateOrTakeForm`) 및 직접 검사 3종. 부착 본체(`Attach` 계열 소비)는 후속 단위에서 잇는다.
- **구현(기존 파일 수정 2개):** `Netstorm.Core/Original/RawSquidProcessHost.cs` 확장(비정렬 PID 조회·WORD 읽기·공장 선택 생성), `RawSquidProcessHostTests.cs` 확장(직접 3개: 번호 조회·생성/지정·공장 거부). PORT_MAP·fixture 변경 없음(이미 port인 행의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 원문 할당 플래그 2·0은 `SidAllocation` 열거로 옮겼다. (2) `CreateOrTakeForm`은 부착 본체 소비를 위해 내부로 열었다(원문 private). (3) `Take`의 서버 영역 계약은 기존 공장 검사를 그대로 쓴다.
- **검증:** 집중 13건 통과(호스트 직접 9개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사 오류 1건을 실행이 잡아 고쳤다(`Take` 서버 영역 계약 — 15000번대로 정정). 변이 검사로 공장 누락 검사 삭제 변이 시 거부 검사가 실패(예외 종류 변경)로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 2개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `SquidProcess.h/.cpp` → `RawSquidProcessHost.cs` 행의 본체 범위 확장). 총 601줄 유지(자료 596행: L1 325, port 271).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·슬롯 범위 안의 생성·조회만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(Host `Attach` 본체·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.193 단위 기록 — 포함 링크 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 포함 링크 2종(가상 표 허용 목록 자료·`FormLinkLayout`·부모/이전 getter·setter 6종)과 생성기 1종 및 직접 검사 4종. 분리 본체(`Detach` 등)는 후속 단위에서 잇는다.
- **구현(새 파일 3개):** `dotnetpj/tools/gen_contained_link_tables.py`(허용 목록 151개 생성·`--check`), `Netstorm.Core/Original/RawContainedLinkTables.Data.cs`(생성 자료: 네 Form 주소·패치 83·CD 68 항목), `Netstorm.Core/Original/RawContainedLink.cs`(배치 선택·WORD 조회·쓰기), 검사 `RawContainedLinkTests.cs`(직접 4개: 자료·배치·위치·쓰기). fixture·링크·매니페스트 변경 없음(총 230개 유지).
- **C#에서 달라진 모양:** (1) 원문 주소 표는 생성기가 읽어 C# 자료로 옮긴다(손 transcription 없음·`--check` 통과). (2) SID 구조체는 int 슬롯 번호로 옮겼다(기존 규칙). (3) 원문 span const_cast 조회·쓰기는 `Slot`·`RawSlot`으로 옮겼다. (4) 분리·진단 몸체는 제외했다.
- **검증:** 집중 8건 통과(직접 4개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 생성기 입력 오류 1건(Form 주소 파일 오지정 — 단언이 잡아 고침). 변이 검사로 부모 위치 뒤바꿈 변이 시 위치 검사 2개가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. 생성기 `--check` 통과. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 3개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`RawSquidContainedUnpop.h/.cpp` → `RawContainedLink.cs`, 모두 port — 링크 getter 범위). 총 601줄(자료 596행: L1 325, port 271). 자료 파일은 대응 행이 없다(생성기 선례).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 생성기는 원문 2개를 읽기만 했다. 새 검사는 풀 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(Host 본체·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.192 단위 기록 — 프로세스 부착 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 파생 부착 2종(`ProcessAttachHooks`·`AttachProcessForm`) 및 cpp 검사 직역 1종+직접 1종(48행 전수·장치 누락). Host 접합(나머지 2종, Host·월드 필요)은 후속 단위에서 잇는다.
- **구현(새 파일 2개, 기존 파일 수정 1개):** `Netstorm.Core/Original/RawProcessAttach.cs`(부모 변경 판정·진단·서버 분기·공통 부착 순서), 검사 `RawProcessAttachTests.cs`(전수 1개: 48행 사건·슬롯 대조, 직접 1개). **기존 파일 수정 1개:** `Netstorm.Core.Tests.csproj`(fixture 링크 1행). fixture 1개를 링크·등록했다(총 230개, `cpp_sync.py` 0종료).
- **C#에서 달라진 모양:** (1) 풀 const_cast 조회는 같은 어셈블리의 `RawSlot`으로 옮겼다(0.189 선례). (2) span 지역 캡처 금지는 풀 재조회로 옮겼다. (3) 원문 `std::string` 던지기는 검사 훅의 예외 메시지로 옮겨 사유를 대조한다. (4) C# 풀 50바이트 전용이라 CD 36바이트 대조는 앞부분으로 자른다(F2c-2 선례).
- **검증:** 집중 6건 통과(전수 48행 1개+직접 1개+무결성·매니페스트 4개). **불일치 0.** 개발 중 빌드 오류 1건(span 캡처 금지 — 풀 재조회로 고침). 변이 검사로 부착 플래그(`0x80`→`0x81` 혼합) 변이 시 전수 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개·링크 1행뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`RawProcessAttach.h/.cpp` → `RawProcessAttach.cs`, 모두 L1 — 48행 전수 대조 근거). 총 599줄(자료 594행: L1 325, port 269).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀 범위 안의 읽기·쓰기·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(Host 접합·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.191 단위 기록 — 월드 런타임 훅 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 전송 대리자 2종(`FormPopTransmit`·`BattlePacketSend`)과 런타임 훅 1종(`RawWorldRuntimeHooks` 12경계) 및 직접 검사 3종. 자산 삭제 대리자는 기이식(`RawSquidFormDispatch.cs`)이라 빌렸다. 월드 본체(`BindRuntime` 소비)는 후속 단위에서 잇는다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawWorldRuntimeHooks.cs`(대리자 2종·12경계 묶음과 복사), 검사 `RawWorldRuntimeHooksTests.cs`(직접 3개: 기본값·전달·복사). fixture·링크·매니페스트 변경 없음(총 229개 유지).
- **C#에서 달라진 모양:** (1) 원문 span 바이트는 배열 복사로 옮겼다. (2) `size_t` 개수는 쓰지 않고 대리자만 둔다. (3) 묶음 복사는 `Clone`으로 옮겼다(기존 훅 규칙). (4) 자산 삭제 대리자 중복 정의를 걷고 기존 것을 빌렸다(빌드가 잡아 고침).
- **검증:** 집중 7건 통과(직접 3개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 대리자 시그니처 오류 2건(void 반환 람다 값 반환·바이트 리터럴 추론)을 빌드가 잡아 고쳤다. 변이 검사로 복사 누락(`SquidPacket` 미복사) 변이 시 복사 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 4행(`RawSurfaceWorld.h`·`SquidForms.h`·`RawBattleBroadcast.h`·`SquidFormDispatch.h` → `RawWorldRuntimeHooks.cs`, 모두 port). 총 597줄(자료 592행: L1 323, port 269).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값·대리자 범위 안의 생성·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(Host 본체·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.190 단위 기록 — 호출 수명·부착 배선 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 호출 수명 1종(`RuntimeCallGuard`)과 프로세스 부착 배선 3종(Host 링크·런타임 링크·식별자 쓰기) 및 직접 검사 3종. Host `Attach` 본체의 소비는 후속 단위에서 잇는다.
- **구현(기존 파일 수정 2개):** `Netstorm.Core/Original/RawSquidProcess.cs` 확장(호출 guard·내부 배선), `RawSquidProcessTests.cs` 확장(직접 3개: 빈 guard·대여 수·배선 고정). PORT_MAP·fixture 변경은 행 1개뿐이다(신규 파일 없음).
- **C#에서 달라진 모양:** (1) 원문 스택 guard 소멸은 `IDisposable`으로 옮겼다(정상·예외 복귀 때 반납). (2) 원문 friend 필드 쓰기는 내부 배선으로 옮겼다. (3) `Host()` 접근자는 링크 null 가능으로 두었다(부착 전 null).
- **검증:** 집중 13건 통과(프로세스 직접 9개 + 무결성·매니페스트 4개). **불일치 0.** 변이 검사로 중복 반납 방지 삭제 변이 시 대여 수 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 2개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `SquidProcess.h/.cpp` → `RawSquidProcess.cs` 행의 본체 범위 확장). 총 593줄 유지(자료 588행: L1 323, port 265).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값 범위 안의 생성·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(Host 본체·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.189 단위 기록 — 부착 부모 함수 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 부착 부모 2종(`BaseProcessAttachFields`·`BaseProcessAttachHooks`·`AttachBaseProcess`) 및 cpp 검사 직역 1종+직접 2종(48행 전수·장치 누락·패치 비활성). Host `Attach`·`Reattach` 접합과 나머지 4종(Host 필요)은 후속 단위에서 잇는다.
- **구현(새 파일 2개, 기존 파일 수정 1개):** `Netstorm.Core/Original/RawBaseProcessAttach.cs`(진단→필드→부모→등록→Form→통지 순서·신규/지정 분기·판본 진단 경로), 검사 `RawBaseProcessAttachTests.cs`(전수 1개: 48행 호출·사건·필드·슬롯 대조, 직접 2개). **기존 파일 수정 1개:** `Netstorm.Core.Tests.csproj`(fixture 링크 1행). fixture 1개를 링크·등록했다(총 229개, `cpp_sync.py` 0종료).
- **C#에서 달라진 모양:** (1) 원문 `Sid&` 필드 묶음은 같은 참조 클래스 홀더로 옮겼다. (2) 풀 const_cast 조회는 같은 어셈블리의 `RawSlot`으로 옮겼다(범위 검사는 유지). (3) 원문 `std::string` 던지기는 검사 훅의 `InvalidOperationException("create")`로 옮겨 사유를 대조한다. (4) C# 풀 50바이트 전용이라 CD 36바이트 대조는 앞부분으로 자른다(F2c-2 선례).
- **검증:** 집중 7건 통과(전수 48행 1개+직접 2개+무결성·매니페스트 4개). **불일치 0.** 개발 중 풀 생성자 인자 순서 오류 1건을 빌드가 잡아 고쳤다. 변이 검사로 비활성 조기 반환(`false`→`true`) 변이 시 전수 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개·링크 1행뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`RawBaseProcessAttach.h/.cpp` → `RawBaseProcessAttach.cs`, 모두 L1 — 48행 전수 대조 근거). 총 593줄(자료 588행: L1 323, port 265).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀 범위 안의 읽기·쓰기·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(Host `Attach` 접합·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.188 단위 기록 — 프로세스 호스트 껍데기 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 호스트 껍데기 1종(`SquidProcessHost`의 두 생성자·런타임 선택·삭제/사건/전송 선택·공장 묶음·풀·번호·등록 조회) 및 직접 검사 6종. 부착·검색·삭제·전송 본체(`Attach` 계열·`FindEvent`·`Kill`·`RunRegular` 등)와 프로세스 역참조(`Owns`·`host_` 배선)는 후속 단위에서 잇는다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawSquidProcessHost.cs`(직접·장기 생성자·런타임 미연결 명시 오류·현재 위치 쓰기·판본 번호), 검사 `RawSquidProcessHostTests.cs`(직접 6개: 생성 계약·대여 요구·선택·공장 묶음·번호/등록). fixture·링크·매니페스트 변경 없음(총 228개 유지).
- **C#에서 달라진 모양:** (1) 직접 TYPE 사본과 장기 빌림을 `TypeCount`·`Type`에서 구별한다(원문 span 분기 그대로). (2) 미연결 런타임 선택은 명시 예외로 옮겼다(결정 6). (3) `Sid`·검사 복사는 기존 규칙을 따른다. (4) 삭제 몸체의 188개 검사는 기존 C# 한계라 CD 검사에서는 188개 표를 쓴다(차이로 기록).
- **검증:** 집중 10건 통과(직접 6개 + 무결성·매니페스트 4개). **불일치 0.** 변이 검사로 공유 개수 무시(`_sharedTypes`→`_types`) 변이 시 장기 생성 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`SquidProcess.h/.cpp` → `RawSquidProcessHost.cs`, 모두 port — 부착 본체 잔여). 총 591줄(자료 586행: L1 321, port 265).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·슬롯 범위 안의 생성·조회만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(ProcessHost 부착 본체·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.187 단위 기록 — 표시 진단 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 표시 진단 3종(`SquidDisplayView`·`SquidDisplaySink`·프레임 오류 상세)과 저장소 `RecordDisplayFrameError`(0.177 잔여 해소) 및 직접 검사 5종. 표시 본체(`SquidDisplay` 수집·카메라, R5 범위)는 제외했다.
- **구현(새 파일 2개, 기존 파일 수정 1개):** `Netstorm.Core/Original/RawSquidDisplaySink.cs`(뷰 기본 Q16·싱크 기본 검사 0/미연결 예외·상세 서식), 검사 `RawSquidDisplaySinkTests.cs`(직접 5개: 상세 서식·무효 분기·싱크·뷰·저장소 오류). **기존 파일 수정 1개:** `RawSurfaceStorage.cs`(`RecordDisplayFrameError`). fixture·링크·매니페스트 변경 없음(총 228개 유지).
- **C#에서 달라진 모양:** (1) 화면 사각형은 기존 `DisplayRect`를 빌렸다(0.172 선례). (2) 뷰·싱크 값형은 가변 노출을 위해 클래스로 옮겼다. (3) 부호 없는 추상값의 부호 출력은 `unchecked((int))`로 옮겼다. (4) 싱크 미연결 기본값은 명시 예외로 옮겼다(원문 기본 throw 그대로).
- **검증:** 집중 9건 통과(직접 5개 + 무결성·매니페스트 4개). **불일치 0.** 변이 검사로 유효 역치(`10`→`11`) 변이 시 상세 검사 2개가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개·확장 1개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`SquidDisplay.h/.cpp` → `RawSquidDisplaySink.cs`, 모두 port — 표시 본체 잔여). 총 589줄(자료 584행: L1 321, port 263).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값 범위 안의 생성·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(ProcessHost 본체·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조). 0.177의 `RecordDisplayFrameError` 잔여는 해소됐다.

### 0.186 단위 기록 — 호스트 묶음 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 전송 경계 1종(`ProcessFormTransmission`)·런타임 묶음 1종(`ProcessHostRuntimeBindings`) 및 직접 검사 5종. Host 본체·현재 대여(`Bind`·`Use`)·전송 상태(`RawSquidTransmissionState`, MP 인접)는 후속 단위에서 잇는다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawProcessHostBindings.cs`(네 콜백·8참조 묶음과 복사), 검사 `RawProcessHostBindingsTests.cs`(직접 5개: 기본값·전달·고정·복사). fixture·링크·매니페스트 변경 없음(총 228개 유지).
- **C#에서 달라진 모양:** (1) 원문 span 반환(`packet`)은 배열 복사로 옮겼다. (2) 개수(`size_t`)는 `ulong`으로 옮겼다. (3) `Sid`는 int 슬롯 번호로 옮겼다(기존 규칙). (4) 묶음 복사는 `Clone`으로 옮겼다(기존 훅 규칙).
- **검증:** 집중 9건 통과(직접 5개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사·제품 오류 3건을 빌드·실행이 잡아 고쳤다(바이트 리터럴 추론·공장 188개 가드·복사 누락). 변이 검사로 복사 누락(`Transmit` 미복사) 변이 시 복사 검사가 즉시 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 1행(`SquidProcess.h` → `RawProcessHostBindings.cs`, port — 묶음 잎만이라 L1이 아니다). 총 587줄(자료 582행: L1 321, port 261).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값·대리자 범위 안의 생성·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(ProcessHost 본체·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.185 단위 기록 — 프로세스 기본·정기 껍데기 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 프로세스 기본 1종(`SquidProcess`의 식별자·가상 통지)과 정기 예약 1종(`RegularProcess`의 생성·getter·작업 쓰기) 및 직접 검사 6종. 부착(Host·pid·form 바인딩)과 정기 실행 본체(`RunRegular`)는 Host 본체 단위에서 잇는다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawSquidProcess.cs`(식별자 기본 0·빈 가상 3종·미복원 통지 거부·정기 예약 자료), 검사 `RawSquidProcessTests.cs`(직접 6개: 식별자·빈 가상·통지 거부·생성·작업 쓰기·실행 미연결). fixture·링크·매니페스트 변경 없음(총 228개 유지).
- **C#에서 달라진 모양:** (1) 미연결 실행 본체는 명시 예외로 옮겼다(결정 6 — 미연결 경계의 명시 오류). (2) Host가 붙이는 필드는 명시 초기화로 두고 경고 없이 0을 둔다(붙이기는 Host 단위). (3) `Sid`는 int 슬롯 번호로 옮겼다(기존 규칙).
- **검증:** 집중 10건 통과(직접 6개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 변이 미검출 1건을 검사 입력(횟수 사전 설정 누락)으로 고친 뒤 횟수 초기화 삭제 변이가 즉시 실패로 검출됨을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`SquidProcess.h/.cpp` → `RawSquidProcess.cs`, 모두 port — 부착·실행 본체 잔여). 총 586줄(자료 581행: L1 321, port 260).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값 범위 안의 생성·쓰기·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(ProcessHost 본체·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.184 단위 기록 — 프로세스 호스트 저장소 껍데기 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 게임 보조 절차 번호 6종(`ProcessFormType` 등, 상자 공유형)과 호스트 저장소 껍데기 1종(`RawProcessHostStorage`의 생성·장치·커널·상태) 및 직접 검사 6종과 저장소 `Processes` 접합. Host 본체·현재 대여·legacy 회수·소멸 반납(`SquidProcess`·런타임 묶음 필요)은 후속 단위에서 잇는다.
- **구현(새 파일 2개, 기존 파일 수정 4개):** `Netstorm.Core/Original/RawProcessHostStorage.cs`(상태 owner 고정·장치 계약·판본 커널·시각/타입 전달), 검사 `RawProcessHostStorageTests.cs`(직접 6개: 절차 번호·생성 고정·CD 분기·장치 계약·장치 전달·판본 커널). **기존 파일 수정 4개:** `RawGameAuxState.cs`(6 DWORD 상자 공유형 확장·리셋 불변), `CampaignWorldGlobals.cs`(`DoubleBox` 가상화), `RawGameClock.cs`(`FrameTimeBox`), `RawSurfaceStorage.cs`(`Processes` 지연 생성).
- **C#에서 달라진 모양:** (1) 공유 DWORD 멤버는 값·상자를 함께 두어 기존 읽기쓰기를 유지하며 주소를 빌린다(`GeyserType` 호환 유지). (2) 시계 상자의 게임 시각 빌림은 `DoubleBox` 파생 브리지로 옮겼다. (3) 대여·회수 없는 계약 절은 생략하고 사유를 적었다(Host 본체 단위에서 복원). (4) 소멸자 반납은 GC에 맡기지 않고 월드 종료 경로에서 잇는다.
- **검증:** 집중 10건 통과(직접 6개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사 오류 1건을 실행이 잡아 고쳤다(풀 최소 용량 23003 — 100 거부). 변이 검사로 판본 커널 뒤바꿈 변이 시 판본 커널 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개·호환 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`RawProcessHostStorage.h/.cpp` → `RawProcessHostStorage.cs`, 모두 port). 총 584줄(자료 579행: L1 321, port 258).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 상자·값 범위 안의 읽기·쓰기·묶음만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(ProcessHost 본체·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.183 단위 기록 — 프로세스 상태 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 프로세스 상태 2종(`SquidProcessState`의 시각·권한·타입 묶음과 `RegularHandler` 대리자) 및 직접 검사 6종. 전송 묶음·런타임 묶음·Host 본체(`SquidProcess` 필요)와 저장소 `Processes` 접합은 후속 단위에서 잇는다.
- **구현(새 파일 2개, 기존 파일 수정 1개):** `Netstorm.Core/Original/RawSquidProcessState.cs`(빌림 우선 읽기쓰기와 독립 fallback·현재 번호 getter 6종), 검사 `RawSquidProcessStateTests.cs`(직접 6개: 초기값·독립·권한·시각·간헐천·번호 상자). **기존 파일 수정 1개:** `CampaignWorldGlobals.cs`(원문 `double*` 빌림용 `DoubleBox` additive 1종). fixture·링크·매니페스트 변경 없음(총 228개 유지).
- **C#에서 달라진 모양:** (1) 읽기쓰기 공유 스칼라는 상자(`DoubleBox`·`UIntBox`·`RawAuthorityState`)로 옮겼다(기존 공유 owner 규칙). (2) 읽기 전용 빌림 6종은 같은 상자를 `null` 가능 참조로 둔다. (3) 공유 쓰기가 독립 fallback을 덮지 않는 원문 분리를 그대로 둔다(실행이 검사로 확정했다).
- **검증:** 집중 10건 통과(직접 6개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사 오류 2건을 실행이 잡아 고쳤다(공유 해제 뒤 fallback 기대값 — 원문은 분리라 초기값으로 정정). 변이 검사로 간헐천 초기값(`122`→`121`) 변이 시 직접 3개가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개·additive 1개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 1행(`SquidProcess.h` → `RawSquidProcessState.cs`, port — 상태 잎만이라 L1이 아니다). 총 582줄(자료 577행: L1 321, port 256).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 상자·값 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(ProcessHost 본체·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.182 단위 기록 — 커널·기본 프로세스 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 기본 프로세스 1종(`BaseProcess`)·커널 1종(`Kernel`·`KernelDiagnostics`·용량 상수) 및 cpp 검사 직역 5종+직접 1종(빈 슬롯·소진·부분 효과·0번 덮어쓰기·소멸자 교체·경계). Client·`SquidProcess`가 필요한 진단 3종은 제외했다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawKernel.cs`(슬롯·단일 소유권·retired·개수·판본별 진단·소진 요약→상세→단언·0번 제외 순회), 검사 `RawKernelDiagnosticTests.cs`(직역 5개+경계 1개). fixture·링크·매니페스트 변경 없음(총 228개 유지).
- **C#에서 달라진 모양:** (1) 가상 소멸 효과는 커널이 직접 부르는 가상 `Dispose`로 옮겼다(GC 타이밍에 의존하지 않는다). (2) 설치 전 실패가 넘겨받은 인자까지 소멸시키는 원문 `unique_ptr` 풀기 의미를 `catch` 후 `Dispose`·재던지기로 옮겼다(실행이 잡아 추가했다). (3) 중복 등록 `Add(BaseProcess&)`는 `AddRef`로 옮겼다(C#에 참조 오버로드가 없다). (4) `size_t` 개수·번호는 `int`·`uint`로 옮겼다(슬롯 39999 범위).
- **검증:** 집중 10건 통과(직접 6개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사 대조 오류 1건을 실행이 잡아 고쳤다(소진 실패도 인자를 소멸시키는 의미 누락 → 부분 효과 검사 실패). 변이 검사로 `Clear` 범위(`1`→`0` 시작) 변이 시 0번 덮어쓰기 검사가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 2개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 4행(`BaseProcess.h/.cpp`·`Kernel.h/.cpp` → `RawKernel.cs`, 모두 port — 대응 검사가 직접 대조라 L1이 아니다). 총 581줄(자료 576행: L1 321, port 255).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 슬롯 범위 안의 등록·실행·반납만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(장기 ProcessHost·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.181 단위 기록 — 게임 시계·프레임 시각 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 게임 시계 2종(`FrameTime`·`GameClock`의 벽·게임·보정 묶음·정지·재개·리셋·캡처) 및 cpp 검사 직역 1종+직접 4종(공개 114행 전수·리셋 3종·보정 묶음). `RawClockSyncReset`(별도 시간 동기화 모듈)·`Kernel`·`ProcessHost`는 후속 단위에서 잇는다.
- **구현(새 파일 2개, 기존 파일 수정 1개):** `Netstorm.Core/Original/RawGameClock.cs`(`FrameTime`·412 unsigned 감김 나눗셈 순서·중첩 정지·보정 감산·리셋 1줄·캡처), 검사 `RawGameClockTests.cs`(전수 1개: 138행 중 raw 24행은 개수만·공개 114행 비트 대조, 직접 4개). **기존 파일 수정 1개:** `Netstorm.Core.Tests.csproj`(fixture 링크 1행). 구 경로 `Display/OriginalGameClock`은 동결 유지하며 신구 대응을 기록에 남긴다.
- **C#에서 달라진 모양:** (1) 원문 `uint32_t&` 보정·`shared_ptr<void>` 소유자는 읽기 대리자+보유 참조 쌍으로 옮겼다(기존 공유 owner 규칙). (2) `overflow_error`는 `OverflowException`으로 옮겼다(도달 불가라 검사는 없다). (3) `IsPaused`·`Reset`·`Capture`는 원문 시그니처 그대로 두었다. (4) raw 24행의 C++ 객체 바이트 대조는 이식하지 않고 개수로만 센다(배치 비의존).
- **검증:** 집중 9건 통과(전수 114행 1개+직접 4개+무결성·매니페스트 4개). **불일치 0.** fixture 1개를 링크·등록했다(총 228개, `cpp_sync.py` 0종료). 변이 검사로 리셋 범위(`pausedTotal`만→`pausedGame`도) 변이 시 직접 2개가 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. 개발 중 검사 오류 1건(메서드인 `IsPaused`를 프로퍼티로 호출 — 빌드가 잡아 고침). Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 3개·링크 1행뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 2행(`GameClock.h/.cpp` → `RawGameClock.cs`, 모두 L1 — 공개 전수 대조 근거). 총 577줄(자료 572행: L1 321, port 251).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 밀리초 범위 안의 계산만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(장기 ProcessHost·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.180 단위 기록 — 저장소 풀 생성·전체 대여 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 저장소 전체 생성자(자산 벡터·풀·spot·그래프·집계·섬·청크 접합)와 일반 설치 전체(`Acquire`) 및 직접 검사 4종. 풀 할당 진단 배선(`BindAllocation` 풀 측·`Processes` 필요)·`Processes`·`Source`·`MakeRawSurfaceStorage`·`RecordDisplayFrameError`(타입·셰이프 필요)는 후속 단위에서 잇는다.
- **구현(기존 파일 수정 2개):** `Netstorm.Core/Original/RawSurfaceStorage.cs` 확장(물리 12종 프로퍼티·전체 생성자·`Acquire` — 첫 대여는 풀·해시·spot을 건너뛰고 fresh Graph·집계·섬·청크·사제·검사를 둔다), `RawSurfaceStorageTests.cs` 확장(직접 4개: 전체 생성·자산 불일치·서버 공유·일반 설치 reset). PORT_MAP·fixture 변경 없음(이미 port인 파일의 본체 범위 확장).
- **C#에서 달라진 모양:** (1) 풀 서버 공유 포인터는 클로저(`() => _authority.Server`)로 옮겼다. (2) 실패점 `ref` 전달은 지역 복사→`Finish`→대입으로 옮겼다(프로퍼티는 `ref`가 안 된다). (3) 타입 벡터는 같은 객체 참조를 모아 둔다(월드가 전역을 직접 고치는 의미 보존). (4) 코드는 타입별 `Codes`의 스냅샷 복사본으로 둔다.
- **검증:** 집중 19건 통과(저장소 직접 15개 + 무결성·매니페스트 4개). **불일치 0.** 변이 검사로 첫 대여 건너뜀 삭제(`> 0`→`>= 0`) 변이 시 일반 설치 검사가 즉시 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 2개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 변경 없음(이미 port인 `RawSurfaceWorld.h`·`RawSurfaceStorage.cpp` 행의 본체 범위 확장). 총 575줄 유지(자료 570행: L1 319, port 251).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·배열 범위 안의 읽기·쓰기·대여만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2d 나머지(장기 ProcessHost·할당 배선·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.179 단위 기록 — 저장소 물리 자료형 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 집계 저장소 1종(`RawTotalMadeStorage`와 앞 255칸 reset), 그래프 저장소 1종(`RawGraphStorage`와 표 초기화), 경로 물리 3종(`RawPathSearchWork`·`RawPathSearchState`·`RawPathSearchStorage`) 및 직접 검사 5종. 허용 표 몸체·그래프 실행기·경로 검색 본체(`RawPathSearch`·훅, R6·패스 규칙 범위)와 저장소 생성자·`Acquire` 전체 접합은 후속 단위에서 잇는다.
- **구현(새 파일 4개):** `Netstorm.Core/Original/RawTotalMadeStorage.cs`(세 256DWORD·앞 255 reset·256번째 보존), `Netstorm.Core/Original/RawGraphStorage.cs`(255 레코드·4096 스택·앞 251 두 WORD·0번 사용/예약 하위·sentinel 0x7dfd), `Netstorm.Core/Original/RawPathSearchStorage.cs`(거리표·큐·전역 PE 초기값·공유 소유자), 검사 `RawStorageTablesTests.cs`(직접 5개: 집계 reset·그래프 reset·작업대·전역·공유). fixture·링크·매니페스트 변경 없음(총 227개 유지).
- **C#에서 달라진 모양:** (1) 원문 고정 배열은 같은 길이의 관리 배열로 옮겼다. (2) 공유 포인터는 nullable 참조로 옮겼다(GC 공유). (3) 큐 2바이트 항목은 평탄 `byte[]`로 두고 항목 순서를 주석에 적었다. (4) `short` 0x7DFD는 범위 안이라 그대로 쓴다. (5) `RawPathSearchState`의 외부 정책 선택자 포인터는 검색 본체 단위에서 잇는다.
- **검증:** 집중 9건 통과(직접 5개 + 무결성·매니페스트 4개). **불일치 0.** 개발 중 검사 입력 오류 2건을 실행이 잡아 고쳤다(그래프 0번 사용 표시와 전체 0 검사 충돌, 변이 미검출의 원인인 검사 루프의 제품 상수 사용 → 리터럴 255 고정). 변이 검사로 초기화 범위(`255`→`254`) 변이 시 집계 검사가 즉시 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 4개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 5행(`RawTotalMade.h/.cpp`·`RawGraph.h/.cpp`·`RawPathSearch.h`, 모두 port). 총 575줄(자료 570행: L1 319, port 251).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 배열 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일(`cpppj/src/CMakeLists.txt`·`root/` 5개)은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(저장소 풀 생성·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.178 단위 기록 — 해시 선택·주소 전수 재생 (2026-11-07, HJOW-Athlon)

- **범위:** 해시 선택·조회 3종(`RawSquidHash`의 `Select`·`Selected`·`At`·`MutableEntries`) 및 cpp 검사 직역 1종+직접 3종(주소 811행 전수·독립/리셋·공유 저장소·거부 경계). `Level` 605행·`Genus` 3216행은 기이식(`X86SpotAndOrderTests`)이라 제외했다.
- **구현(기존 파일 수정 1개·새 검사 1개):** `Netstorm.Core/Original/RawSquidHash.cs` 확장(선택 필드·선택 대입·선택 읽기·선택 단계 셀·가변 span, 생성자는 기본 0 유지로 기존 호출자 불변), 검사 `RawSquidHashAddressTests.cs`(전수 1개: Init 4·At 100·IntBucket 100·FloatBucket 607행 주소 별칭 대조, 직접 3개). fixture·링크·매니페스트 변경 없음(등록済 `hash-x86.tsv` 재사용, 총 227개 유지).
- **C#에서 달라진 모양:** (1) 원문 `uint16_t&` 반환은 `ref ushort`로 옮겼다(주소 별칭을 `Unsafe.AreSame`으로 대조). (2) 생성자의 선택 인자는 두지 않고 기본 0으로 두었다(원문 기본 인자와 같은 값, 기존 호출자 30여 곳 불변). (3) `size_t` 인덱스는 65535 이하라 `int`로 옮겼다. (4) 판본 분기·`Squid` 쪽(`EffectiveGenus`)은 기존 모듈 그대로이며 이 단위에서 건드리지 않았다.
- **검증:** 집중 8건 통과(주소 전수 811행 1개+직접 3개+무결성·매니페스트 4개). **불일치 0.** 변이 검사로 선택 무시(`Cell(_selected`→`Cell(0`) 변이 시 전수·직접 3개 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 기존 1개 확장뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 1행(`SquidHash.h` → `RawSquidHash.cs`, L1 — 주소 전수 대조 근거). 총 570줄(자료 565행: L1 319, port 246). 기존 `.cpp` 행(base `f95a8db2`)은 영향 검토 전이라 올리지 않았다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 배열 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(저장소 풀 생성·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조). W02 잔여였던 해시 `At`·`Select`는 해소됐다.

### 0.177 단위 기록 — F2d 서피스 저장소 껍데기 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 실패점 1종(`BoardTargetState`), 스캔 상태 1종(`BridgeDecayScanState` 자료형만), 저장소 껍데기 1종(`RawSurfaceStorage`의 공유 owner·검사·표시·할당·그래프 진단·바람·시계·간헐천·대여 관문) 및 직접 검사 11종. 풀·해시·spot·타입 벡터·경로 작업대·그래프·집계·섬·청크 물리, `Processes`·`Acquire` 전체·`Source`·`MakeRawSurfaceStorage`·`RecordDisplayFrameError`(타입·셰이프 필요)는 전체 생성 단위에서 잇는다.
- **구현(새 파일 3개, 기존 파일 수정 1개):** `Netstorm.Core/Original/RawBoardTargetState.cs`(실패점 기본 -10000), `Netstorm.Core/Original/RawSurfaceStorage.cs`(스캔 상태·fresh 기본: 권한 참·바람 0/1/0·연결 82/94/95/155/157·스캔 PATCH -1·주기 10·사제 9DWORD 0·준비 대여 관문·`Use`), 검사 `RawSurfaceStorageTests.cs`(직접 11개: fresh·CD 스캔·owner 고정/독립·검사 읽기·상세/진단·시계/로그·원문 1걸음·각도 뷰·간헐천·대여·할당). **기존 파일 수정 1개:** `CampaignWorldGlobals.cs`(원문 상세 빌림용 `StringBox` additive 1종).
- **C#에서 달라진 모양:** (1) 원문 `optional<string>&` 상세 owner는 `StringBox` 공유 상자로 옮겼다(`UIntBox` 규칙). (2) 스캔 상태 구조체는 가변 노출을 위해 클래스로 옮겼다(값 복사 의미는 대입으로 유지). (3) `Sid`·검사 복사·감김 규칙은 기존과 같다. (4) 전체 생성자는 풀·자산 장치가 필요해 두지 않았고 판본만 껍데기에 보관해 바람 진행에 쓴다.
- **검증:** 집중 15건 통과(저장소 직접 11개 + 무결성·매니페스트 4개). **불일치 0.** fixture 추가 없음(원문 1걸음은 0.176 등록 fixture를 빌려 `55·1걸음` 행과 비트 대조, 총 227개 유지, `cpp_sync.py`는 0.176 값 유지). 변이 검사로 연결 타입(`82`→`81`) 변이 시 fresh 검사가 즉시 실패로 검출함을 확인하고 `--no-incremental` 원복·재통과했다. Release 솔루션 빌드 경고·오류 0. Core 전체는 0.176 완주 1,416개가 마지막이며 변경이 신규 3개·additive 1개뿐이라 기존 검사에 영향을 주지 않는다.
- **PORT_MAP:** 신규 4행(`RawSurfaceWorld.h`·`RawSurfaceStorage.cpp`·`RawBoardTarget.h`·`Bridge.h` → 저장소 3행·실패점 1행, 모두 port). 총 569줄(자료 564행: L1 318, port 246).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값·대리자 범위 안의 읽기·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지(풀 생성·월드 본체)·`RawFortWorldLoadBinding`·F3부터다(0.9절 참조).

### 0.176 단위 기록 — F2d 게임 난수·보드 각도·바람 진행 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 게임 난수 3종(`GameRandom`의 `Next`·`State`·`SetState`, `CrtRandom`의 `Next`·`Borrow`·`Thread`·`SetState`, `RawGameRandomSeed`의 `NextCrt`·`Initialize`·`Reseed`), 보드 각도 2종(`BoardAngleDiagnostics`·`RawBoardAngle.Normalize`), 바람 진행 1종(`RawAmbientWind.Step`) 및 직접 검사 14종+전수 2종. `RawSurfaceStorage.UpdateAmbientWind` 본체·`RawSurfaceWorld` 본체·F3 세션 연결은 제외했다.
- **구현(새 파일 2개, 기존 파일 수정 1개):** `Netstorm.Core/Original/RawGameRandom.cs`(게임·CRT·시드, 구 경로 `Simulation/NetstormRandom`·`MsvcRandom`은 동결 유지하며 신구 대응을 주석에 명시), `Netstorm.Core/Original/RawBoardAngle.cs`(진단 묶음·판본별 끝각), `Netstorm.Core/Original/RawAmbientWind.cs` 확장(정적 `RawAmbientWind.Step` — 판본별 잡음 곱셈 순서·`Math.Atan2`·끝각 위임). 검사 `RawGameRandomTests.cs`(직접 11개)·`RawBoardAngleTests.cs`(1088행 전수 1개+직접 3개)·`RawAmbientWindStepTests.cs`(1920행 전수 1개+직접 2개). **기존 파일 수정 1개:** `Netstorm.Core.Tests.csproj`(fixture 링크 2행).
- **C#에서 달라진 모양:** (1) 원문 `uint32_t&` 공유 상태(CRT 빌림·시드의 CRT·모드)는 외부 `UIntBox` 공유 상자로 옮겼다(0.168 규칙). (2) 스레드 지역 공유 난수는 `ThreadLocal<CrtRandom>`으로 옮겼다. (3) C++ 나머지 연산은 C# 배정밀도 `%`로 옮겼다(0 절삭 나머지 동일). (4) 끝각 상수는 `359.9999f` 리터럴로 옮겼다(비트 `0x43b3fffd` 일치 확인). (5) `BoardAngleDiagnostics`의 검사 뷰는 호출 때마다 다시 읽는 대리자 바인딩으로 옮겼다(원문 비소유 주소 의미). (6) 구 경로 `NetstormRandom`·`MsvcRandom`과 식이 같으나 세션 조립 전까지 병존하며 은퇴는 F3에서 한다.
- **검증:** 집중 25건 통과(난수 직접 11개·각도 전수 1088행+직접 3개·바람 전수 1920행+직접 2개+바람 상태 3개+무결성·매니페스트 4개 중 집중 해당). **불일치 0.** fixture 2개를 링크·등록했다(총 227개, `cpp_sync.py` 0종료). 변이 검사 2종을 검출하고 원복했다(M1 게임 곱셈 `0x10003`→`0x10004` 시 직접·바람 전수 3개 실패, M2 끝각 `359.9999f`→`359.0f` 시 각도 3개 실패). 원복 뒤 `--no-incremental` 전체 빌드로 집중 25개 통과를 재확인하고 Core 전체 **1,416개** 통과(8분 17초), 실패·건너뜀 0, 솔루션 빌드 경고·오류 0을 확인했다. 증분 빌드가 변이 바이너리를 그대로 돌린 사례가 있어 원복 확인은 `--no-incremental`로 했다.
- **PORT_MAP:** 신규 8행(`SpStore.h`·`CrtRandom.h/.cpp`·`RawGameRandomSeed.h/.cpp` 5행 port, `RawBoardAngle.h/.cpp` 2행 L1, `RawAmbientWind.cpp` 1행 L1). 총 565줄(자료 560행: L1 318, port 242).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값·배열 범위 안의 읽기·계산만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일(`cpppj/src/root/` 5개)은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조). 0.175의 `StepAmbientWind` 잔여는 해소됐다.

### 0.175 단위 기록 — F2d 서피스 월드 바람 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 서피스 월드 바람 1종(`AmbientWindState`의 세 float 전역) 및 직접 검사 3종. `StepAmbientWind`(`GameRandom` 미이식)·`RawSurfaceStorage` 본체·`RawSurfaceWorld` 본체·F3 세션 연결은 제외했다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawAmbientWind.cs`(실제 PE 최초값 0·1·0인 세 바람 전역), 검사 `RawAmbientWindTests.cs`(직접 3개: 초기값·독립성·읽기쓰기). **기존 파일 수정 0개**(fixture 링크·매니페스트 변경 없음).
- **C#에서 달라진 모양:** 원문 float 세 값을 읽기쓰기 속성으로 옮겼다(초기 0·1·0 보존). 의존이 없어 추가 적응이 없다.
- **검증:** 집중 9건 통과(바람 직접 3개 + 무결성·매니페스트 6개). **불일치 0.** fixture 추가 없음(총 225개 유지, `cpp_sync.py` 0종료). 변이 검사로 초기값(`Y` 1→0) 변이 시 초기값 검사가 즉시 실패로 검출함을 확인하고 원복했다. Release 빌드 경고·오류 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 2개 추가뿐이라 기존 검사에 영향을 주지 않으며(집중·무결성 9건 통과), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 1행(`RawAmbientWind.h` → `RawAmbientWind.cs`, port, 기준 `e5a198df`). 총 557줄(자료 552행: L1 315, port 237). 대응 검사가 없어 port다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.174 단위 기록 — F2c 일반지형 요청형 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 일반지형 요청형 4종(`NormalTerrainBranch`·`NormalTerrainRequest`·`TerrainFarmPostpassAssertion`·`NormalTerrainDiagnostics`) 및 직접 검사 6종. `TerrainGenerationInputs`(GameRandom 등 미이식)·`MakeNormalTerrainFarmWorldHooks`·`RawNormalTerrainWorldBinding` 본체·`MakeFortWorldLoadBinding`·`RawSurfaceWorld` 본체·F3 세션 연결은 제외했다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawNormalTerrainRequest.cs`(세 분기·네 값 요청과 기본 미복원·네 후처리 진단·다섯 진단 묶음과 복사), 검사 `RawNormalTerrainRequestTests.cs`(직접 6개: 기본 미복원·네 값 보관·분기 구분·진단 기본 미연결·다섯 진단 전달·복사 공유/분리). **기존 파일 수정 0개**(fixture 링크·매니페스트 변경 없음).
- **C#에서 달라진 모양:** (1) 원문 값 구조체는 읽기전용 레코드 구조체로 옮겼다. (2) 기본 요청은 `Unrestored` 정적 값으로 옮겼다. (3) 훅 묶음 복사는 `Clone`으로 옮겼다(기존 훅 규칙).
- **검증:** 집중 12건 통과(요청형 직접 6개 + 무결성·매니페스트 6개). **불일치 0.** fixture 추가 없음(총 225개 유지, `cpp_sync.py` 0종료). 변이 검사로 기본 분기(미복원→일반) 변이 시 기본값 검사가 즉시 실패로 검출함을 확인하고 원복했다. 개발 중 검사 입력 오류 1건(없는 도장 진단 값 — 기존 값으로 고침)을 빌드가 잡아 고쳤다. Release 빌드 경고·오류 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 2개 추가뿐이라 기존 검사에 영향을 주지 않으며(집중·무결성 12건 통과), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 1행(`RawNormalTerrainWorldBinding.h` → `RawNormalTerrainRequest.cs`, port, 기준 `2d50804c`). 총 556줄(자료 551행: L1 315, port 236). 본체·통합 재생을 잇기 전이라 L1이 아니다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값·대리자 범위 안의 읽기·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.173 단위 기록 — F2d 서피스 월드 권한 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 서피스 월드 권한 1종(`RawAuthorityState`의 서버/권한 두 논리 전역) 및 직접 검사 3종. `RawSurfaceStorage` 본체(나머지 의존 미이식)·`RawSurfaceWorld` 본체·F3 세션 연결은 제외했다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawAuthorityState.cs`(두 PE 초기 true인 서버/권한 관문), 검사 `RawAuthorityStateTests.cs`(직접 3개: 초기값·독립성·관문 읽기쓰기). **기존 파일 수정 0개**(fixture 링크·매니페스트 변경 없음).
- **C#에서 달라진 모양:** 원문 DWORD 1 논리값을 `bool` 속성으로 옮겼다(초기 true 보존). 의존이 없어 추가 적응이 없다.
- **검증:** 집중 9건 통과(권한 직접 3개 + 무결성·매니페스트 6개). **불일치 0.** fixture 추가 없음(총 225개 유지, `cpp_sync.py` 0종료). 변이 검사로 초기값(`Server` true→false) 변이 시 초기값 검사가 즉시 실패로 검출함을 확인하고 원복했다. Release 빌드 경고·오류 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 2개 추가뿐이라 기존 검사에 영향을 주지 않으며(집중·무결성 9건 통과), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 1행(`RawAuthorityState.h` → `RawAuthorityState.cs`, port, 기준 `815fbd67`). 총 555줄(자료 550행: L1 315, port 235). 대응 검사가 없어 port다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.172 단위 기록 — F2d 서피스 월드 자료형 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 서피스 월드 자료형 3종(`SurfaceSeed`·`SurfaceSnapshot`·`SurfaceDisplayHooks`) 및 직접 검사 6종. `RawWorldCore`·`RawWorldRuntimeHooks`(미이식 의존 다수)·`RawSurfaceStorage`·`RawSurfaceWorld` 본체(63KB)·통합 장면 재생·F3 세션 연결은 제외했다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawSurfaceSeed.cs`(다섯 값 시드·번호+객체 스냅샷·억제/무효화 훅 묶음과 복사), 검사 `RawSurfaceSeedTests.cs`(직접 6개: 기본값·다섯 값 보관·번호+객체 동행·훅 기본 미연결·억제/무효화 전달·복사 공유/분리). **기존 파일 수정 0개**(fixture 링크·매니페스트 변경 없음).
- **C#에서 달라진 모양:** (1) 원문 값 구조체는 읽기전용 레코드 구조체로 옮겼다. (2) C++ `Sid`는 `int` 슬롯 번호로 옮겼다(기존 규칙). (3) 원문 `SquidDisplayRect`는 기존 `DisplayRect`를 빌렸다(R2 대조済). (4) 훅 묶음 복사는 `Clone`으로 옮겼다(기존 훅 규칙).
- **검증:** 집중 12건 통과(자료형 직접 6개 + 무결성·매니페스트 6개). **불일치 0.** fixture 추가 없음(총 225개 유지, `cpp_sync.py` 0종료). 변이 검사로 복사 누락(`Suppressed` 미복사) 변이 시 복사 검사가 즉시 실패로 검출함을 확인하고 원복했다. Release 빌드 경고·오류 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 2개 추가뿐이라 기존 검사에 영향을 주지 않으며(집중·무결성 12건 통과), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 1행(`RawSurfaceWorld.h` → `RawSurfaceSeed.cs`, port, 기준 `b8cadde1`). 총 554줄(자료 549행: L1 315, port 234). 본체·통합 재생을 잇기 전이라 L1이 아니다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 값·대리자 범위 안의 읽기·호출만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.171 단위 기록 — F2d 저장 나머지 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 저장 나머지 2종(`RawSavedState`의 `WriteSavedState`·`PollSavedStateWrite`·`ReadSavedState`와 전역·훅 형식 4종) 및 검사 13종(쓰기 직접 9개·판독 542행 전수 1개와 직접 3개). `SavedStateFilesTests` 쪽(Win32 삭제·메인 메뉴·Win32FileError, 별도 모듈)과 CD 판본·F3 세션 연결은 제외했다.
- **구현(새 파일 3개):** `Netstorm.Core/Original/RawSavedState.cs`(관문·진단→열기→개별 쓰기→닫기 순서와 부분 실패 반납·10초 폴링·서명/점이름/8레코드/꼬리/종료 판독 순서), 검사 `SavedStateWriteTests.cs`(직접 9개: 관문 2단언·976바이트 배치·점 진단·서버 수 진단·예외 닫기·폴링 3단언·복사 위반 2종·9기록 위반), 검사 `SavedStateReadTests.cs`(전수 1개·직접 3개: 선택 장치·물리 계약·짧은 꼬리). **기존 파일 수정 2개:** `Netstorm.Core.Tests.csproj`(fixture 링크 1행), `cpp-fixture-manifest.tsv`(542행 등록).
- **C#에서 달라진 모양:** (1) 원문 `uint32_t&` 전역은 `UIntBox` 공유 상자로 옮겼다(0.168 규칙). (2) 원문 span 대리자는 배열 뷰로 옮겼고(`Span`은 대리자 인자가 안 된다) 판독 read 훅은 실제 저장소에 직접 쓰게 `(버퍼, 위치, 폭)`을 받는다(훅 안 변이 효과를 부모 복사가 지우지 않게). (3) DWORD 직접 read는 부족분 상위 보존·예외분 전체 반영으로 옮겼다. (4) `const bool&` 전투·`double&` 기한은 값·`ref`로 옮겼다. (5) fixture trace의 주소 토큰만 정규화해 비교한다(L1 판정 유지).
- **검증:** 집중 19건 통과(쓰기 직접 9개·판독 전수 538관측+4물리·직접 3개 + 무결성·매니페스트 6개). **불일치 0.** fixture 1개를 링크·등록했다(총 225개, `cpp_sync.py` 0종료). 변이 검사로 서명(`0xa004e`→`0xa004f`) 변이 시 전수가 즉시 실패로 검출함을 확인하고 원복했다. 개발 중 검사·하네스 오류 6건(폴링 기한·이름 꼬리 위치·scratch 판본 단일화·trace 공백·initCore 실제 반영·중단 부분 반영)을 대조가 잡아 고쳤다. Release 빌드 경고·오류 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 3개 추가와 링크·매니페스트뿐이라 기존 검사에 영향을 주지 않으며(집중·무결성 19건 통과), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 4행(`RawSavedStateWrite.h/.cpp` 2행 port·`RawSavedStateRead.h/.cpp` 2행 L1, 기준 `4558729b`). 총 553줄(자료 548행: L1 315, port 233). 쓰기 쪽은 cpp 대응 검사가 없어 port다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 배열 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.170 단위 기록 — F2d 서피스 삭제 분배기 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 서피스 삭제 분배기 1종(`RawSurfaceLifecycle`의 생성 검증·`Hooks`·`Emit` 가상 표 분기·`EmitBridge` 5분기)과 훅 묶음 1종(`SurfaceLifecycleHooks` 4종) 및 직접 검사 12종. CD 판본·`SurfaceLifecycleTests` 통합 장면 재생(Renderer·Factory 필요)·저장 나머지·`RawSurfaceWorld`·F3 세션 연결은 제외했다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawSurfaceLifecycle.cs`(연결 타입 [70, 타입수) 검증·필수 훅 검증·내부 탐색기/다리/섬 몸체 조립·공통 유지 emit 교체·bridge pre/post·island pre·noIsland post 분기), 검사 `RawSurfaceLifecycleTests.cs`(직접 12개: 생성 거부 8단언·공통 유지·기타 효과 전달·모르는 표 전달·섬 사전/무인도 사후/다리 사전/다리 사후 경로·다리 분기 5종).
- **C#에서 달라진 모양:** (1) 원문 삭제 복사·이동 금지는 값 복사가 없는 C# 클래스와 훅 `Clone`으로 같은 의미를 둔다. (2) 원본 `this` 캡처 람다는 같은 참조의 지역 복사 캡처로 옮겼다. (3) 원문 private 분배(`Emit`·`EmitBridge`)는 직접 검사에서 호출할 수 있게 공개했다(0.169 선례). (4) C++ `Sid`는 `int` 슬롯 번호로 옮겼다(기존 규칙). (5) 첫 구현은 PATCH 전용이며 CD 가상 표 분기는 관측 전이다(0.167 선례).
- **검증:** 집중 18건 통과(분배 직접 12개 + 무결성·매니페스트 6개). **불일치 0.** fixture 추가 없음(총 224개 유지, `cpp_sync.py` 0종료). 변이 검사로 섬 가상 표(`0x00506d98`→`0x00506d99`) 변이 시 경로 검사가 즉시 실패로 검출함을 확인하고 원복했다. Release 빌드 경고·오류 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 2개 추가뿐이라 기존 검사에 영향을 주지 않으며(집중·무결성 18건 통과), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 2행(`RawSurfaceLifecycle.h/.cpp` → `RawSurfaceLifecycle.cs`, 모두 port, 기준 `225f2322`). 총 549줄(자료 544행: L1 313, port 231). 통합 장면 재생을 잇기 전이라 L1이 아니다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·배열 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.169 단위 기록 — F2d 캠페인 월드 적재 본체 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 캠페인 월드 적재 본체 1종(`RawCampaignWorldLoad`의 생성 검증·`CopyName`·`CurrentPlayer`·`FinishMissionPriests`·`Load` 11단계 순서·`Pool`)과 훅 묶음 1종(`CampaignWorldLoadHooks` 11종) 및 직접 검사 9종. CD 판본·`CampaignWorldLoadTests` 전수 재생(65행×2판본, 월드 스택 하네스 필요)·저장 나머지·`RawSurfaceWorld`·F3 세션 연결은 제외했다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawCampaignWorldLoad.cs`(동일 Pool/180바이트/필수 훅 검증·NUL 포함 복사·전역 재조회·prologue→Player→Builder/Territory/PlaceAll→Generate→Objects→Graph0→mission finish→Camera→현재이름 순서), 검사 `RawCampaignWorldLoadTests.cs`(직접 9개: 생성 거부 5단언·복사 stale 보존·복사 거부 2단언·현재 Player 선택/거부·후처리 생략/요구/호출·Load 11단계 순서·editor 0쓰기·Graph/Camera 사이 사제 준비).
- **C#에서 달라진 모양:** (1) 원문 `uint32_t&`·`int32_t&`(puzzlePiece·noRegion)는 읽기 대리자(`Func`)로 옮겼다(기존 `RawFortWorldLoad` 규칙). (2) 원문 span 뷰 반환(`CurrentPlayer`)은 메서드 반환 `Span`으로 옮겼다(필드가 아니라 허용). (3) 원문 값형 훅 묶음 복사는 `Clone`으로 옮겼다(기존 `FortWorldLoadHooks` 규칙). (4) C++ `Sid`는 `int` 슬롯 번호로 옮겼다(기존 규칙). (5) 첫 구현은 PATCH 전용이며 CD stride/위치 분기는 관측 전이다(0.167 선례).
- **검증:** 집중 15건 통과(적재 직접 9개 + 무결성·매니페스트 6개). **불일치 0.** fixture 추가 없음(총 224개 유지, `cpp_sync.py` 0종료). 변이 검사로 header 쓰기(`1`→`0`) 변이 시 순서 검사가 즉시 실패로 검출함을 확인하고 원복했다. 개발 중 검사 입력 오류 2건(2바이트 대상에 2바이트 복사 거부 기대·hex 기대값 오기)을 대조가 잡아 고쳤다. Release 빌드 경고·오류 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 2개 추가뿐이라 기존 검사에 영향을 주지 않으며(집중·무결성 15건 통과), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 1행(`RawCampaignWorldLoad.cpp` → `RawCampaignWorldLoad.cs`, port, 기준 `b8ce42c7`). 총 547줄(자료 542행: L1 313, port 229). 전수 재생을 잇기 전이라 L1이 아니다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·배열 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.168 단위 기록 — F2d 캠페인 월드 전역 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 캠페인 월드 전역 1종(`CampaignWorldGlobals`의 빌림·복제·대입 수명 규칙) 및 직접 검사 6종. `RawCampaignWorldLoad` 본체(Load·CopyName·CurrentPlayer·FinishMissionPriests)·저장 나머지·`RawSurfaceWorld`·F3 세션 연결은 제외했다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/CampaignWorldGlobals.cs`(독립 기본값 0·0·0·true·외부 `UIntBox` 공유 빌림·값 복제·목적지 바인딩 유지 대입), 검사 `CampaignWorldGlobalsTests.cs`(직접 6개: 기본값 독립·값 생성 소유·빌림/null fallback·복제 비전파·대입 바인딩 유지·양쪽 공유).
- **C#에서 달라진 모양:** (1) 원문 `uint32_t&` 두 참조는 외부 `UIntBox` 공유 상자로 옮겼다(0.154 TS1 규칙, null 슬롯은 자체 fallback). (2) 원문 값형 복사·이동·대입은 참조 재바인딩이 안 되는 C# 클래스라 값 복제 생성자와 `AssignFrom`(원본 선캡처 후 쓰기)으로 옮겼다. (3) C++17 prvalue 직접 구성은 정적 `BorrowSubscribers`로 옮겼다.
- **검증:** 집중 12건 통과(전역 직접 6개 + 무결성·매니페스트 6개). **불일치 0.** fixture 추가 없음(총 224개 유지, `cpp_sync.py` 0종료). 변이 검사로 기본값(`MissionPost` true→false) 변이 시 직접 1개가 실패로 검출함을 확인하고 원복했다. Release 빌드 경고·오류 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 2개 추가뿐이라 기존 검사에 영향을 주지 않으며(집중·무결성 12건 통과), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 1행(`RawCampaignWorldLoad.h` → `CampaignWorldGlobals.cs`, port, 기준 `b8ce42c7`). 총 546줄(자료 541행: L1 313, port 228). Load 본체 전수 재생을 잇기 전이라 L1이 아니다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 상자·배열 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.167 단위 기록 — F2d Player 월드 본체 이식 (2026-11-07, HJOW-Athlon)

- **범위:** Player 월드 본체 1종(`RawPlayerWorld`의 생성·후처리·조회 8종) 및 직접 검사 11종. CD 판본·`player-world-x86.tsv` 1680행 전수 재생(월드 스택 하네스 필요)·`RawCampaignWorldLoad`·저장 나머지·`RawSurfaceWorld`·F3 세션 연결은 제외했다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawPlayerWorld.cs`(PATCH 9×0xb4 기록·0x9c Technology·소유자 34·프레임 36·가상표 0x50ee08·예약 event1·payload5.0f 순서), 검사 `RawPlayerWorldTests.cs`(직접 11개: 생성 계약·가상표·검사·getter 2개·후처리 4개·생성 순서 2개). **기존 파일 수정 0개**(fixture 링크·매니페스트 변경 없음).
- **C#에서 달라진 모양:** (1) C++ `Sid`는 `int` 슬롯 번호로 옮겼다(기존 규칙). (2) 원문 span 9기록은 `byte[]`로 옮겼다(`Span`은 클래스 필드가 안 된다). (3) 읽기 전용 `Slot`과 쓰기용 `AllocatedBytes`를 구분한다(C# 풀은 free 검사를 하므로 검사 훅은 free 비트를 세우지 않는다). (4) `base` 이름은 C# 예약어라 `inner`로 바꿨다(주석에 명시).
- **검증:** 집중 17건 통과(월드 직접 11개 + 무결성·매니페스트 6개). **불일치 0.** fixture 추가 없음(총 224개 유지). 변이 검사로 Technology 위치(`0x9c`→`0x98`) 변이 시 직접 4개가 실패로 검출함을 확인하고 원복했다. 개발 중 검사 훅 오류 1건(소유자 효과에 free 비트를 세워 후속 `AllocatedBytes`가 거부 — 읽기용 `Slot`으로 해소)을 대조가 잡아 고쳤다. Release 빌드 경고·오류 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 2개 추가뿐이라 기존 검사에 영향을 주지 않으며(집중·무결성 17건 통과), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 2행(`RawPlayerWorld.cpp/.h` → `RawPlayerWorld.cs`, 모두 port, 기준 `fe3d121d`). 총 545줄(자료 540행: L1 313, port 227). x86 전수 재생을 잇기 전이라 L1이 아니다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀·배열 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.166 단위 기록 — F2d CoreData 완성·갱신 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 장부 검사 1종(`PlayerCoreRecordsTests` 3개)·CoreData 전역 1종(`ClientCoreDataGlobals`)·월드 상태 1종(`PlayerWorldState`)·갱신 1종(`RawPlayerCoreRefresh`와 618·16행 전수·직접 2개) 및 초기화 분리 검사 1종. `RawPlayerWorld` 본체·`RawCampaignWorldLoad`·세션 검사는 제외했다.
- **구현(새 파일 5개):** 검사 `PlayerCoreRecordsTests.cs`(186행 전수·직접 2개), `Netstorm.Core/Original/RawPlayerCoreRefresh.cs`(기술·다이스 순회와 하한 갱신), 검사 `PlayerCoreRefreshTests.cs`(618·16행 전수·직접 2개), `Netstorm.Core/Original/RawPlayerWorldState.cs`(현재 전역 6필드), `Netstorm.Client/Player/ClientCoreDataGlobals.cs`(장기 저장소와 빌림·읽기·선택·복원). **기존 파일 수정 2개:** `Netstorm.Core.Tests.csproj`(fixture 링크 3행), `PlayerInitializationTests.cs`(분리 검사 1종).
- **C#에서 달라진 모양:** (1) 원문 span 저장소는 배열 뷰로 옮겼다(`Span`은 클래스 필드가 안 된다). (2) 장부 뷰의 부분 배열은 독립 배열로 둔다(별칭 검사가 없으면 잘라 쓰지 않는다). (3) `TryGetArray`의 기본 `Memory` 성공 동작에 기대지 않고 빈 배열로 가린다(0.163 선례). (4) packet broadcast 동일 주소 단언은 내용·가드 대조로 대신한다.
- **검증:** 집중 10건 통과(장부 186행 전수 1개·직접 2개·갱신 618·16행 전수 2개·직접 2개·분리 1개는 Core 16건으로 확인). **불일치 0.** fixture 3개를 링크·등록했다(총 224개, `cpp_sync.py` 0종료). 변이 검사 1종(슬롯 폭 50→48)을 전수가 즉시 실패로 검출함을 확인하고 원복했다. 개발 중 별칭·서명 오류 3건과 라이브 쓰기 누락 2건을 대조가 잡아 고쳤다. Release 빌드 경고·오류 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 4개 추가와 기존 1개 수정뿐이라 기존 검사에 영향을 주지 않으며(집중·무결성 16건 통과), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 5행(`RawPlayerCoreRefresh` 2행 L1·`RawPlayerWorld.h` 1행 port·`ClientCoreDataGlobals` 2행 port). 총 543줄(자료 538행: L1 313, port 225).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 배열 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.165 단위 기록 — F2d Player 참여·초기화 재생 이식 (2026-11-07, HJOW-Athlon)

- **범위:** Player 참여 1종(`RawPlayerParticipation`의 로그인·동맹·brain 12종)과 초기화 202행 재생(`player-initialization-x86.tsv`) 및 검사 9종(참여 전수 2개·직접 6개·초기화 전수 1개). AI 생성 연계 1종(`RawAiCreate` 필요)과 `ClientCoreDataGlobals` 분리 검사 1종은 제외했다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawPlayerParticipation.cs`(로그인·동맹·수신·brain 부착·파괴·반납·증빙·전체 삭제), `RawPlayerParticipationTests.cs`(장면·608행·288행·직접 6개). **기존 파일 수정 3개:** `RawPlayerInitialization.cs`(메모리 훅 라이브 뷰), `PlayerInitializationTests.cs`(재생 하네스·202행 전수), `RawAiCreateTests.cs`(구 Simulation 이름 별칭 2행, 동작 불변).
- **C#에서 달라진 모양:** (1) 원문 span 저장소는 배열 뷰로 옮겼다(`Span`은 클래스 필드가 안 된다). (2) 이동 전용 증빙은 모듈 번호·순서 번호로 식별한다(GC에 기대지 않는다). (3) 초기화 메모리 훅은 `byte[]`에서 라이브 `Memory`로 바꿨다(0.164 직접 검사는 암묵 변환으로 그대로 통과). (4) `base` 이름은 쓰지 않았다(해당 없음). (5) packet broadcast 동일 주소 단언은 내용·가드 대조로 대신한다.
- **검증:** 집중 10건 통과(참여 `player-participation-x86.tsv` 608행·`player-alliance-packet-x86.tsv` 288행 전수 2개 + 직접 6개·초기화 202행 전수 1개 + 직접 1개는 Core 16건으로 확인). **불일치 0.** fixture 1개를 링크·등록했다(총 220개, `cpp_sync.py` 0종료). 변이 검사 2종(동맹 자기 bit 삭제·카운터 쓰기 삭제)을 각 검사가 즉시 실패로 검출함을 확인하고 원복했다. 개발 중 이름 충돌 1건(구 Simulation 모듈과 같은 이름 — 별칭으로 해소, 동작 불변)을 빌드가 잡아 고쳤다. Release 빌드 경고·오류 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 2개 추가와 기존 3개 수정뿐이라 기존 검사에 영향을 주지 않으며(집중·무결성 16건 통과), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 2행(`RawPlayerParticipation` 2행 L1). 총 533행(L1 305, port 228).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 배열 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.164 단위 기록 — F2d Player 초기화·Chat 채널 이식 (2026-11-07, HJOW-Athlon)

- **범위:** Player 초기화 1종(`RawPlayerInitialization`의 전체 236바이트 부모)과 Chat 채널 1종(`RawChatChannels`의 `Add`·`AddToChannel`·`Clear`·`ResetOccupied`) 및 검사 4종(Chat 전수 1개·직접 2개·초기화 직접 1개). 초기화 202행 재생(동맹·Chat 실제 모듈 필요)·`RawPlayerParticipation`·`ClientCoreDataGlobals`·`RawPlayerWorld`·월드는 제외했다.
- **구현(새 파일 2개):** `Netstorm.Core/Original/RawPlayerInitialization.cs`(이전 해제·malloc·405 DWORD 0·행·공장·동맹 순서), `Netstorm.Client/Chat/RawChatChannels.cs`(100×48 기록 선택·이름 정제·플래그). **기존 파일 수정 2개:** `Netstorm.Core.Tests.csproj`·`Netstorm.Client.Tests.csproj`(fixture 링크 각 1행).
- **C#에서 달라진 모양:** (1) 원문 주소 물리는 같은 배열 구간의 복사본 장치로 옮겼다(쓰기 관측이 없는 이 검사 한정). (2) 이름 바이트는 Latin-1로 주고받는다(원문 비ASCII 보존). (3) `Unknown%08x` 서식은 소문자 16진 8자리로 맞췄다. (4) `base` 이름은 쓰지 않았다(해당 모듈 없음).
- **검증:** 집중 4건 통과(Chat `chat-channels-x86.tsv` 2448행 전수 1개 + 직접 2개·초기화 직접 1개는 Core 1개와 Client 3개로 나뉜다 — Core 집중 7건·Client 3건으로 확인). **불일치 0.** fixture 1개를 링크·등록했다(총 219개, `cpp_sync.py` 0종료). 변이 검사 2종(금지 표 공백 제거·카운터 쓰기 삭제)을 각 검사가 즉시 실패로 검출함을 확인하고 원복했다. Release 빌드 경고·오류 0(불필요 using 1건은 제거했다). Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 4개 추가와 링크뿐이라 기존 검사에 영향을 주지 않으며, 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 4행(`RawPlayerInitialization` 2행 port·`RawChatChannels` 2행 L1). 총 531행(L1 303, port 228).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 배열 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.163 단위 기록 — F2d Player Core 장부·CoreData 이식 (2026-11-07, HJOW-Athlon)

- **범위:** Player Core 장부 1종(`RawPlayerCoreRecords`의 장부·Core 초기화·메타 쓰기 3종)과 CoreData 조회·선택·복원 1종(`RawPlayerCoreData`의 `Read`·`Select`·`Restore`) 및 cpp 검사 직역 1종(`PlayerCoreDataTests` 3개). `Restore` 본체는 함께 옮겼으나 그 검사는 이 파일에 없고, `RawPlayerInitialization`·`RawPlayerWorld`·`RawCampaignWorldLoad`·월드 스택은 제외했다.
- **구현(새 파일 2개, 모두 `Netstorm.Core/Original`):** `RawPlayerCoreRecords.cs`(9×16바이트 장부·부분 쓰기·패딩 경계), `RawPlayerCoreData.cs`(포인터 동일성 진단·0 정리 캐시·비영 alias·복원의 관문·부분 효과). **기존 파일 수정 1개:** `Netstorm.Core.Tests.csproj`(fixture 링크 1행 추가).
- **C#에서 달라진 모양:** (1) 원문 `span` 전역은 주소 식별이 필요해 `Memory<byte>` 뷰로 옮겼다(`Span`은 클래스 필드가 안 된다). (2) 빈 주소와 길이 0 외부 슬라이스를 구별한다(후자는 오류가 아니라 외부 이름이다). (3) `uint32_t&` 장기 전역은 읽기·쓰기 공유 객체로 옮겼다(0.154 TS1 규칙). (4) DWORD 감김·부분 load/store 순서는 `unchecked`·임시 4바이트로 보존했다.
- **검증:** 집중 3건 통과(`player-core-data-x86.tsv` 804행 전수 1개 + 직접 2개). **불일치 0.** fixture 1개를 링크·등록했다(총 218개, `cpp_sync.py` 0종료). 변이 검사로 Player 간격(`0xb4`→`0xac`) 변이 시 직접 검사가 즉시 실패로 검출함을 확인하고 원복했다. 개발 중 검사 하네스 오류 2건(소유자 지도 누락 오인 — 실제로는 `TryGetArray`의 기본 `Memory` 성공 동작 오해, 탐침으로 확정)을 대조가 잡아 고쳤다. Release Core 집중 9건(위 3건 + 무결성·매니페스트 6건) 통과, 실패·건너뜀 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 3개 추가와 링크 1행뿐이라 기존 검사에 영향을 주지 않으며(빌드 경고·오류 0), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 4행(`RawPlayerCoreRecords`·`RawPlayerCoreData` 각 2행, 모두 L1). 총 527행(L1 301, port 226).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 배열 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2d 나머지·F3부터다(0.9절 참조).

### 0.162 단위 기록 — F2c 다리 연결 전수 재생·이웃 CD 정렬 (2026-11-07, HJOW-Athlon)

- **범위:** `bridgeconnect-x86.tsv` 장면 재생 8종(Walk·Connect·Link·Owner·FindAt·IslandOwner·StalagOwner·IslandPost, 판본별 3,558행×3판본)과 이웃 탐색기의 CD 판본 분기 2곳. `RawTerrainRun`·`RawTerrainPipeline`은 대응 cpp 파일이 없어 `RawNormalTerrainWorldBinding`(`Generate`·`GenerateOriginal`) 이식 때 함께 정리한다.
- **구현(기존 파일 수정 1개):** `RawNeighbor.cs`(추가 비트·프레임 위치의 판본 분기 35·34, 주석의 CD 미이식 서술 정정). **검사 확장(기존 파일 수정 1개):** `BridgeConnectTests.cs`(장면 사전·사건·출생·Adler 재생 하네스와 전수 3개). 새 제품·fixture·PORT_MAP 행 없음.
- **C#에서 달라진 모양:** (1) C# 풀·해시는 10.78 스트라이드(50바이트) 전용이라 CD·10.37 장면도 50바이트 풀로 돌리고, raw·Adler·출생 대조는 앞 36바이트로 자른다(F2c-11 선례). CD 클라이언트 영역(5~5999)과 C# 영역(5~14999)은 장면 번호(5~127·100~111) 구간에서 동일하게 할당된다. (2) CD 이웃의 추가 비트·프레임은 판본 위치에서 읽는다(기존 직접 검사의 발자국 조건 차이는 그대로 둔다).
- **검증:** 집중 36건 통과 — 전수 3개(판본별 3,558행, **불일치 0**), 0.161 직접 4개·색 표 3개, 이웃·탐색기 회귀와 무결성·매니페스트. 변이 검사로 CD 추가 비트 위치(35→34) 변이 시 CD 전수가 즉시 실패로 검출함을 확인하고 원복했다(기대 이웃이 빠지고 엉뚱한 번호가 섞임). Release 빌드 경고·오류 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 기존 파일 2개뿐이라 기존 검사에 영향을 주지 않으며(회귀 36건 통과), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 변경 없음(이미 L1인 `RawBridgeConnect`·`RawSquidNeighbors` 계열 행의 검증만 깊어짐). 총 523행(L1 297, port 226).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다.
- **남은 것:** F2c 나머지·F2d·F3부터다(0.9절 참조).

### 0.161 단위 기록 — F2c 다리·섬 연결 본체 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 다리·섬 연결 본체 1종(`RawBridgeConnect`의 `OwnerColorFrame`·`SetIslandOwner`·`MakeOwnerDispatch`·`FindTypeAt`·`Link`·`PropagateOwner`·`Connect`·`ConnectPreview`·`IslandPostPopPrefix`·`IslandPostPop`) 및 검사 5종(색 표 전수 3개·거부·접두·미리보기·실시간 순회 직접 4개). 전수 장면 재생(Walk·Connect·Link·Owner·FindAt·IslandOwner 등 판본별 3,600여 행)은 C# 장면 하네스가 필요해 후속 단위로 남겼다.
- **구현(새 파일 1개, `Netstorm.Core/Original`):** `RawBridgeConnect.cs`(소유자 색·섬 소유자·분배·한 칸 조회·연결 객체 생성·소유자 전파·연결 순회·미리보기·받침 후처리, 판본별 소유자·추가 비트·프레임 위치). **기존 파일 수정 1개:** `Netstorm.Core.Tests.csproj`(fixture 링크 1행 추가).
- **C#에서 달라진 모양:** (1) C++ `Sid`는 `int` 슬롯 번호로 옮겼다(기존 규칙). (2) 미리보기 두 방식(`vector*`·`append`)은 `List<int>?`·`Action<int>?` 이중 인자로 옮겼다. (3) `RoundUp`의 x87 중간값은 배정밀도 덧셈으로 보존했다. (4) `base` 매개변수 이름은 C# 예약어라 `inner`로 바꿨다(주석에 명시).
- **검증:** 집중 7건 통과(색 표 `bridgeconnect-x86.tsv` Color 126행 판본별 42행씩 3개 + 직접 4개: 거부 11단언·접두 효과 보존·미리보기 가시성·실시간 순회). **불일치 0.** fixture 1개를 링크·등록했다(총 217개, `cpp_sync.py` 0종료). 변이 검사로 중립 프레임(8→9) 변이 시 색 표 검사가 즉시 실패로 검출함을 확인하고 원복했다. Release Core 집중 13건(위 7건 + 무결성·매니페스트 6건) 통과, 실패·건너뜀 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 2개 추가와 링크 1행뿐이라 기존 검사에 영향을 주지 않으며(빌드 경고·오류 0), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 2행(`RawBridgeConnect.cpp/.h` → `RawBridgeConnect.cs`, 모두 L1 — 색 표 전수 대조 근거). 총 523행(L1 297, port 226). `.h`는 상태·훅 형식(`RawBridgeConnectTypes.cs`, 0.160 port 행)과 본체 파일에 각각 걸린다(`SquidPop.cpp` 선례).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2c 나머지·F2d·F3부터다(0.9절 참조).

### 0.160 단위 기록 — R4 섬 후처리 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 섬 후처리 1종(`RawIslandPostPop`의 `SurfaceLetter`·`Prefix`·`SetSupportOwner`)과 연결 전역·훅 형식 2종(`BridgeConnectState`·`BridgeConnectHooks`) 및 직접 검사 6종. `RawBridgeConnect` 본체(`Connect`·`Link`·`PropagateOwner`·`FindTypeAt` 등)와 x86 전수 재생, `RawNormalTerrainWorldBinding`·`RawTerrainRun`·`RawTerrainPipeline`(`RawSurfaceWorld` 필요, F2d/F3 범위)은 제외했다.
- **구현(새 파일 2개, 모두 `Netstorm.Core/Original`):** `RawBridgeConnectTypes.cs`(연결 전역 7필드·외부 효과 5종·생성 플래그 상수), `RawIslandPostPop.cs`(서·북 프레임 글자 선택·받침 생성·H 소유자 전파·연결 순회, 3×3 소유자·표시 갱신·누락 진단, 판본별 프레임·소유자·추가 비트 위치). **기존 파일 수정 0개**(fixture 링크·매니페스트 변경 없음).
- **C#에서 달라진 모양:** (1) C++ `Sid`는 `int` 슬롯 번호로 옮겼다(기존 규칙). (2) 원문 `bool*` 공유 저장 게임은 읽기 대리자(`SavedGameSource`)로 옮겨 매 H 칸에서 현재 값을 다시 읽는다. (3) 생략한 소유자 지도는 표면 지도를 쓴다(원문 기본 인자). (4) `Cell`·발자국·부호 바이트 감김은 `unchecked`·범위 검사로 보존했다. (5) C# 풀은 10.78 스트라이드 전용이라 CD 분기는 같은 50바이트 풀에서 오프셋만 갈라 읽는다.
- **검증:** 집중 6건 통과(지도 조회 경계 직접 1개·생성자 거부 직접 1개·무비트 무동작 직접 1개·받침 생성 순서 직접 1개·3×3 전파·누락 진단 직접 1개·저장 게임 관문 직접 1개). 개발 중 검사 입력 오류 1건(소유자 지도 인자 누락)을 대조가 잡아 고쳤다. 변이 검사로 받침 묶음 한 변(`SupportSize` 3→2) 변이 시 전파 검사가 즉시 실패로 검출함을 확인하고 원복했다. Release Core 집중 12건(위 6건 + 무결성·매니페스트 6건) 통과, 실패·건너뜀 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 3개 추가뿐이라 기존 검사에 영향을 주지 않으며(빌드 경고·오류 0), 무결성·매니페스트 검사는 통과했다.
- **PORT_MAP:** 신규 3행(`RawBridgeConnect.h` 1행 port·`RawIslandPostPop` 2행 port). 총 521행(L1 295, port 226). x86 전수 재생을 잇기 전이라 L1이 아니다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** F2c 나머지·F2d·F3부터다(위 범위 문단 참조).

### 0.159 단위 기록 — R4 배치 지형 2종 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 일반 배치 지형 1종(`RawCanonPlacementTerrain`의 `Begin`·`BeginShape`·`Candidate`·`EndShape`·`Finish`·`RegionAt`), 사제 배치 지형 1종(`RawPriestPlacementTerrain`의 동명 6종), 공유 형식 3종(`PriestPlainCanonType`·`CanonPlacementCell`·`PriestPlacementQuery`) 및 cpp 검사 직역 2종(`CanonPlacementTerrainTests` 4개·`PriestPlacementTerrainTests` 4개). 파이프라인 연결 어댑터(`MakeCanonTerrainGeometryHooks`·`MakeCanonTerrainCollisionHooks`·`MakePriestTerrainGeometryHooks`·`MakePriestTerrainCollisionHooks`, 기하·충돌·미리보기·배치 본체가 필요한 통합 검사가 R6 범위라 제외)와 `RawIslandPostPop`(`BridgeConnect` 의존)·`RawNormalTerrainWorldBinding`·`RawTerrainRun`·`RawTerrainPipeline`(`RawSurfaceWorld` 의존, F2d/F3 범위)은 제외했다.
- **구현(새 파일 3개, 모두 `Netstorm.Core/Original`):** `RawPlacementTerrainTypes.cs`(공유 형식 3종), `RawCanonPlacementTerrain.cs`(초기 권한·원점·지면 마스크·144바이트 지역 배열·8×8 교집합 쓰기·부호 바이트 비교·최종 관계 경계), `RawPriestPlacementTerrain.cs`(초기 권한 참·같은 지역 배열·최종 거부 우회). **기존 파일 수정 1개:** `Netstorm.Core.Tests.csproj`(fixture 링크 2행 추가).
- **C#에서 달라진 모양:** (1) C++ `Sid`는 `int` 슬롯 번호로 옮겼다(기존 규칙). (2) C# 풀·해시는 10.78 스트라이드(50바이트) 전용이므로 CD·10.37 장면도 50바이트 풀로 돌리고, CD·10.37 raw 대조는 앞 36바이트로 자른다(F2c-11 선례). (3) `Truncate`·WORD·부호 바이트 감김은 `unchecked`·범위 검사로 보존했다. (4) `CanonPlacementQuery`는 0.154 기이식 형식을 그대로 썼다.
- **검증:** 집중 8건 통과(`CanonPlacementTerrainTests`: `canonterrain-x86.tsv` 11220행 판본별 3740행씩 3개 + 직접 1개, `PriestPlacementTerrainTests`: `priestterrain-x86.tsv` 6720행 판본별 2240행씩 3개 + 직접 1개). **불일치 0.** fixture 2개를 링크·등록했다(총 216개, `cpp_sync.py` 0종료). 변이 검사로 `Pitch` 12→11 변이 시 전수 검사가 즉시 실패로 검출함을 확인하고 원복했다. Release Core 집중 14개(위 8개 + 0.157~0.158 집중·무결성 6개) 통과, 실패·건너뜀 0. Core 전체는 시간 관계로 완주하지 못했으나 변경이 신규 파일 5개 추가와 링크 2행뿐이라 기존 검사에 영향을 주지 않으며(빌드 경고·오류 0), 무결성·매니페스트 검사는 통과했다. `--ports`는 기존처럼 시간 초과라 단위가 읽는 cpppj 파일의 `git log`로 기준을 직접 확인했다.
- **PORT_MAP:** 신규 7행(`RawCanonPlacementTerrain` 2행 L1·`RawPriestPlacementTerrain` 2행 L1·지원 형식 3행 port). 총 518행(L1 295, port 223).
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** 0.160부터 R4 지형·섬 생성 2차 나머지 4모듈을 잇는다(위 범위 문단 참조).

### 0.158 단위 기록 — R4 섬 수명주기 이식 (2026-11-07, HJOW-Athlon)

- **범위:** 섬 수명주기 1종(`RawIslandLifecycle`의 `IslandPreDestroy`·`SurfacePostDestroy`) 및 직접 검사 4종. `RawIslandPostPop`·배치 지형 2종·`RawTerrainRun`·`RawTerrainPipeline`·`RawNormalTerrainWorldBinding`과 `islandlifecycle-x86.tsv` 전수 재생(원본 장면 하네스 필요)은 제외했다.
- **구현(새 파일 1개, `Netstorm.Core/Original`):** `RawIslandLifecycle.cs`(00442240 받침 사전 파괴의 추상·매립 건너뜀·중심 1회 조회·0x1000 유지·살아 있는 다리 이웃만 지연 낙하 예약·공통 사전 파괴, 00442640 표면 사후 파괴의 추상·매립 건너뜀·칸 절삭·해시 1단계 일반 탐색·걷는 유닛만 가상 낙하·공통 사후 파괴, 분류 범위·좌표·칸 보조). **기존 파일 수정 0개**(fixture 링크·매니페스트 변경 없음).
- **C#에서 달라진 모양:** (1) C++ `Sid`는 `int` 슬롯 번호로 옮겼다(기존 규칙). (2) `uint32_t&` 전역이 없어 읽기·쓰기 대리자 쌍이 필요 없었다. (3) `Cell`·`Coordinate`의 감김·비트 해석은 `unchecked`로 보존했다. (4) 이웃 탐색기 풀 동일성은 `ReferenceEquals`로 검사했다.
- **검증:** 집중 4건 통과(훅 누락·타 풀 거부 직접 1개, 추상 받침 직접 1개, 표면 걷는 유닛 분배 직접 1개, 추상 표면 직접 1개) 및 0.157 집중·무결성 포함 12건 통과. 변이 검사로 걷는 유닛 판정(`TypeFlag2.Walker`→`TypeFlag2.Island`) 변이 시 분배 검사가 즉시 실패로 검출함을 확인하고 원복했다. Release Core 비X86 1,049개 통과, 실패·건너뜀 0. Core 전체는 시간 초과로 완주하지 못했으나 변경이 신규 파일 2개 추가뿐이라 기존 검사에 영향을 주지 않으며(빌드 경고·오류 0), 직전 전체 1,280개 통과 기준에 신규 4개를 더한 1,284개가 기대값이다.
- **PORT_MAP:** 신규 2행(`RawIslandLifecycle.cpp/.h` → `RawIslandLifecycle.cs`, 모두 port, 기준 cpp `6a0907de`·h `225f2322`). 총 511행(L1 291, port 220). x86 전수 재생을 잇기 전이라 L1이 아니다.
- **원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다.** 새 검사는 풀 범위 안의 읽기·쓰기만 했다. 외부 접속 관점: 이 단위는 소켓·HTTP 코드를 추가하지 않았다. 같은 작업 트리의 다른 세션 cpppj 변경·미커밋 파일은 건드리지 않고 보존했다. `options.cfg`는 만지지 않았다. 빌드·검사 외에 별도 창이 뜨는 작업은 없었다. 영향 검토 전이라 `reflected-commit`·PORT_MAP `base`는 올리지 않았다(신규 행의 기준만 파일별 HEAD).
- **남은 것:** 0.159부터 R4 지형·섬 생성 2차 나머지 6모듈을 잇는다. `RawIslandPostPop`은 `BridgeConnect` 계열이 필요해 그 의존과 함께 본다.

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
| Edit 맵 목록·이름·작업본 (11-01, 0.115~0.127) | 편집 맵 목록 50칸 캐시와 분류·행 표시, Tell 맵 열 분할·파일 키 복구, AskEditFort/ReadFort·편집 복귀 State 예약, 기본 디스크 열거와 Fort 섹션 기반 맵 메타, 호스트 Edit 입력·맵 목록, 새 Fort 이름 확인·호스트 문자 입력·이름/숫자 모달, 편집 이관 장부·작업본 장치, 편집 파일 장치(존재·readonly·권한·Template 쓰기·원자 쓰기) | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126) 0.115~0.126, [2026-11-04 이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) 0.127 | 새 경로 |
| DataManager·Template 저장소 (11-01~02, 0.128~0.136) | 원문 계층(MemStream·DataManager·Template·Fort 이미지·State, 원본 x86 fixture 14개), ClientTemplateSectionNames, 저장소 TS0~TS6(용량을 가진 섹션 버퍼·설치/선택·동적 섹션·파일 읽기/저장·빈 manager·로그인 송신 이미지·Editor 파일 계층, 원본 .fort 433개 읽기→저장 왕복 일치, 송신 패킷 `fort-image-packets` 219행 전체 일치, 한글 작업 경로의 UTF-8 이름 보관, 새 맵 manager·신규 자료·파일 부재 부모 `RawNewFort` 원본 x86 1,400행 일치, 현재 Fort 이름 장치, 실제 저장소를 쓰는 편집 맵 목록) | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) 0.128~0.136, [이식 계획](docs/dotnet-template-storage-plan-20261101.md) | 새 경로(`RawNewFort` 등 일부 L1) |
| Fort 타입 표·적재·저장 F1~F2c-11 (11-02~04, 0.137~0.149·0.153·0.154) | F1 `RiftTypeTable`·`.type` 파서·`FortTemplate.Parse`(두 판본, x86 기준점·비용·techBit·생성자 주소 대조, 실제 `.fort` TypeNames 해시 패치 29·CD 424개 일치), F2a Money·Meta, F2b TypeNames·Deck·Contents·Subscriber, F2c-1~3 Territory·팔레트·객체 적재 본체·객체 스트림, F2c-4~8 저장 모듈 14종·`ChunkMap`·FinishOwner, F2c-9 변환 저장소, F2c-10 AddRandom 부모·입력, F2c-11 섬 목록·단일 배치·군집·게임 보조·Notify. 단위마다 대표 변이 검사를 실행했다 | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) 0.137~0.153, 이 문서 0.154 | 대부분 L1(PORT_MAP 참조). 세션 연결은 F3 |
| 11.01 표기·Credits (11-03, 0.150) | 새 경로 `ProductVersion`·`ProductPresentation`·Tell 본문 3항·`Patch1101` 등록, 기존 경로 `MainMenuView` 버전 창 11.01·Credits 3항 목록과 같은 상세 창. 원본 판본 표기와 옛 Credits는 유지 | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) 0.150, [cpppj 685 문서](docs/exe/cpp-product-1101-20261101.md) | 새 경로·기존 경로(`port`) |
| 외부 접속 문서화·클라이언트 유효성 검사 비활성화 (11-03, 0.151) | C# 소켓·HTTP 코드 없음 확인, 외부 실행은 기존 경로 Help의 사용자 선택 링크 1곳, 구 검증은 연결·성공 합성 없음 | [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md) | 문서. 새 경로 이식 때마다 갱신(0.7) |
| 멀티플레이 사전 구현 (11-03, 0.152) | `Netstorm.Net`의 `LocalNetworkPolicy`(루프백·리슨만 허용)·`LocalMissionTransport` 바이트 큐 골격. 사용자 지시로 검사 없음 | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) 0.152 | 사전 구현(`port`), 검증 완료 아님 |
| R4 Fort 사제 준비 (11-07, 0.157) | `RawFortPriestPreparation`의 `Run`·`Collect`·`Marked`와 `FortPriestPreparationTests` 2개(원본 13행 전수·부분 실패 직접). 대표 변이 1종 검출 | 이 문서 0.157 | L1(PORT_MAP 참조). 세션 연결은 F3 |
| R4 섬 수명주기 (11-07, 0.158) | `RawIslandLifecycle`의 `IslandPreDestroy`·`SurfacePostDestroy`와 직접 검사 4개(훅·추상·걷는 유닛 분배). 대표 변이 1종 검출. x86 전수 재생은 장면 하네스 필요로 제외 | 이 문서 0.158 | port(PORT_MAP 참조). L1 전수는 후속. 세션 연결은 F3 |
| R4 배치 지형 2종 (11-07, 0.159) | `RawCanonPlacementTerrain`·`RawPriestPlacementTerrain` 6종씩과 공유 형식 3종, 전수 11220·6720행 대조 6개와 직접 2개. 대표 변이 1종 검출. 파이프라인 어댑터·통합 검사는 R6 범위로 제외 | 이 문서 0.159 | L1(PORT_MAP 참조). 세션 연결은 F3 |
| R4 섬 후처리 (11-07, 0.160) | `RawIslandPostPop` 3종과 연결 전역·훅 형식, 직접 검사 6개. 대표 변이 1종 검출. `BridgeConnect` 본체·x86 전수 재생은 월드 스택 하네스가 필요해 F3 조립 때 | 이 문서 0.160 | port(PORT_MAP 참조). L1 전수는 후속. 세션 연결은 F3 |
| F2c 다리·섬 연결 본체 (11-07, 0.161) | `RawBridgeConnect` 10종과 색 표 전수 126행 대조 3개·직접 4개. 대표 변이 1종 검출. 장면 전수 재생은 C# 하네스 후속 단위에서 | 이 문서 0.161 | L1(PORT_MAP 참조). 세션 연결은 F3 |
| F2c 다리 연결 전수 재생 (11-07, 0.162) | 장면 8종 판본별 3,558행 전수 대조 3개·이웃 CD 판본 분기. 대표 변이 1종 검출. 새 제품·fixture·장부 행 없음 | 이 문서 0.162 | L1(PORT_MAP 기존 행). 세션 연결은 F3 |
| F2d Player Core (11-07, 0.163) | `RawPlayerCoreRecords` 3종·`RawPlayerCoreData` 3종과 804행 전수 1개·직접 2개. 대표 변이 1종 검출 | 이 문서 0.163 | L1(PORT_MAP 참조). 세션 연결은 F3 |
| F2d Player 초기화·Chat (11-07, 0.164) | `RawPlayerInitialization` 전체 부모와 `RawChatChannels` 4종, 2448행 전수 1개·직접 3개. 변이 2종 검출. 초기화 202행 재생·참여·CoreDataGlobals는 후속 | 이 문서 0.164 | L1·port(PORT_MAP 참조). 세션 연결은 F3 |
| F2d Player 참여·초기화 재생 (11-07, 0.165) | `RawPlayerParticipation` 12종과 608·288·202행 전수 3개·직접 7개. 변이 2종 검출. AI 연계·CoreDataGlobals 검사는 후속 | 이 문서 0.165 | L1(PORT_MAP 참조). 세션 연결은 F3 |
| F2d CoreData 완성·갱신 (11-07, 0.166) | 장부 186행·갱신 618·16행 전수 3개·직접 4개·분리 1개와 전역·상태 본체. 변이 1종 검출. 월드 본체·세션 검사는 후속 | 이 문서 0.166 | L1·port(PORT_MAP 참조). 세션 연결은 F3 |
| F2d Player 월드 본체 (11-07, 0.167) | `RawPlayerWorld` 8종과 직접 검사 11개. 대표 변이 1종 검출. CD·전수 재생은 후속 | 이 문서 0.167 | port(PORT_MAP 참조). L1 전수는 후속. 세션 연결은 F3 |

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
- **변이 검사 운전:** 최근 단위(0.141~0.154)는 별도 사본 자동 순환 대신 대표 변이를 수동으로 넣고 제품·검사 빌드 뒤 `--no-build` 집중 실행으로 검출을 확인한 다음 원복하고 집중·전체 검사를 다시 통과시켰다. 사본(`extracted/dn-mutation-ts0`)을 쓸 때는 드라이버의 다중 행 패턴 들여쓰기가 틀리면 일치 0개로 조용히 건너뛰므로 개수 assert로 잡고, 사본에 복사한 파일은 반드시 `touch`한다(시각이 오래되면 증분 빌드가 옛 변이 바이너리를 그대로 돌린다). 무한 루프로 걸리는 변이도 검출로 기록한다(0.138·0.141·0.148).
- **options.cfg 변경 판별:** 호스트나 원본 게임을 실행한 뒤 추적 파일 `assets/game-data/d/options.cfg`가 바뀌어 있으면 누구의 실행 때문인지 먼저 가린다(같은 시기 다른 세션이 원본 게임을 실행했을 수 있다). 내가 만든 변경일 때만 되돌린다(0.136의 교훈).

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

**2026-11-04 이관(HJOW-Athlon, 문서만, 사용자 요청):** 정리 직전(`ee69e4a4 / 20261104-25 DN`, 저장소 HEAD `0c06fa66`) 전체 원문을 [이관 파일 끝](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154)에 그대로 보관했다. 본문에서 뺀 것은 단위 기록 0.127~0.153(0.154는 최신 단위라 남김), 0.1의 단위별 검사 수·PORT_MAP·fixture 증가 나열, 0.9의 단위별 완료 서술과 완료 체크 항목(11.01·외부 접속)이다. 완료됐지만 체크되지 않았던 Template·Fort 단계(TS0~TS6·F1·F2a~F2c-11)를 0.9의 하위 체크 목록에 `[x]`로 표시하고, 1절 색인에 0.137~0.154 행을 더했으며 마지막 행의 "F1 미실행·F2 이후 미착수" 표기를 고쳤다. 이관한 단위의 해소되지 않은 한계는 0.9의 '이월한 이식 한계'로, 변이 운전·options.cfg 판별 교훈은 4절로 옮겼다. 0.1의 디컴파일 기준(2026-11-03)과 cpppj 현재 값(861)을 공통 문서에 맞췄다. 빌드·검사·게임 실행·디컴파일은 하지 않았다.

**2026-11-02 이관(HJOW-Athlon, 문서만, 사용자 요청):** 정리 직전(`3b2b9e58 / 20261102-97 DN`) 전체 원문을 [이관 파일 끝](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)에 그대로 보관했다. 본문에서 뺀 것은 단위 기록 0.66~0.126(0.127 이후는 TS 단계의 직접 근거라 남김), 0.1의 단위별 검사 수 나열, 0.9의 단위별 완료 서술과 완료 체크 항목이다. 0.126 이하에서 이후 단위로 해소되지 않은 한계는 0.9의 '이월한 이식 한계' 항목으로 옮겼다(0.67의 두 항목은 0.68에서 해소돼 제외했다). AGENTS.md 최근 변경(4차 로컬 MCP·와이드 해상도, 7차 2560×1440·3840×2160 FSR, 11.01, 외부 접속 문서화)은 0.7·0.8·0.9에 반영했다. 빌드·검사·게임 실행·디컴파일은 하지 않았다. `LEFT_JOBS.md`와 이 문서를 함께 고친 것은 `LEFT_JOBS.md`를 수정하지 않는다는 0.7 원칙에 대한 사용자 지시 예외다.

**2026-10-31 이관(HJOW-X3D, 문서만):** 정리 직전 전체 원문을 [이관 파일 끝](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073)에 그대로 보관했다. 본문에서 뺀 것은 머리 안내 3개, 2026-10-28 당시 상태표와 진단 상세, 완료한 R0~R2 상세, 0.9의 완료 항목, 0.10, 단위 기록 0.11~0.65, 2절의 완료 줄이다. 빌드·검사·게임 실행·디컴파일은 하지 않았다.

**2026-10-28 이관(VM-W11-Codex):** 0절 재수립으로 더 이상 현행 의미가 없어진 내용을 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)로 옮겼다. 내용은 고치지 않고 그대로 옮겼으며, 그 파일 안의 "현재·다음·미구현" 표현은 당시 상태다.

- **옮긴 것:** 2026-10-22 이전 계획, 2026-10-09~10-23 날짜별 완료·인계 절, 구 머리말과 0~8절(2026-10-07~10 당시 1~6단계·8절 이식/대기 구분), 옛 9절 표와 9-1의 옛 순서 제안, 옛 10절 1·3·4번(해결·구현이 끝난 확인 항목), 11절(과거 미확인 항목 재분류), 14절(LEFT_JOBS.md에서 이관한 2026-09-27~10-05 이력).
- **본문에 남긴 것:** 0절 현행 계획, 1절 완료 범위 색인, 2절 9-1 결정, 3절 옛 10절의 유효한 결정, 4절 검증 명령(옛 12절), 5절 참고 문서(옛 13절).
- **앵커:** 옮긴 절의 `<a id>`(`dotnet-current-plan-20261022`·`sid-session-plan`·`legacy-dotnet-history`·`legacy-dotnet-031`~`107` 등)는 이관 파일에 같은 이름으로 남겼다. LEFT_JOBS.old.md·LEFT_JOBS.webpj.md·`docs/`의 기존 링크가 이 안내로 오도록 같은 이름의 빈 앵커를 바로 아래에 두었다. 그런 링크로 들어왔다면 이관 파일의 같은 앵커를 본다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
