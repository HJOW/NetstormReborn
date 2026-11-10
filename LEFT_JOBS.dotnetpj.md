# LEFT_JOBS.dotnetpj — dotnetpj(C# + MonoGame) 작업 계획 및 인수인계

이 문서는 dotnetpj의 **현재 상태, 계획, 남은 작업, 유효한 사용자 결정**을 담는다. cpppj와 공통 사항은 [LEFT_JOBS.md](LEFT_JOBS.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md), 과거 기록은 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)에 있다.

**읽는 순서:** [한눈에 보기](#dotnet-overview) → [0.9 바로 다음 작업](#09-바로-다음-작업) → 필요할 때 0절의 나머지. [0절 현행 계획](#dotnet-current-plan-20261028)이 이 문서와 [이식 계획 문서](docs/dotnet-cpp-port-plan-20261016.md)의 순서·구조 결정보다 우선한다. 1절은 완료 범위 색인, 2·3절은 계속 유효한 사용자 결정, 4·5절은 검증 명령과 참고 문서, 6절은 이관 기록이다.

**단위를 마치면:** 한눈에 보기의 표, 0.1의 값, 0.9의 체크 항목을 갱신한다. 상세는 `docs/dotnet-*.md` 단위 문서에 남기고, 문서가 없는 작은 단위만 [0.10 단위 기록](#dotnet-unit-records)에 짧게 적는다(다음 번호 **0.280**). 0.279 이하의 단위 기록은 이관 파일에 있다.

**2026-11-10 정리:** 단위 기록 0.154~0.279와 0.9의 단위별 완료 체크 목록을 [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279)에 원문 그대로 옮기고, 완료 범위는 묶음으로 줄여 0.9와 1절에 남겼다. 그동안 반영되지 않았던 AGENTS.md 변경(8차 시나리오 편집·5차 확장·9차 알려진 버그·글꼴 외부 배포·세 프로젝트 평가 기준)과 cpppj의 새 오라클 기능(`session-dump`·`--fixed-frame-ms`)·재반영 요청을 0.5·0.7·0.8·0.9에 넣었다. 상세는 6절 첫 항목이다.

---

<a id="dotnet-overview"></a>

## 한눈에 보기 — dotnetpj 진행 상황 (2026-11-10, 단위 0.287 기준)

### 실행·플레이 가능 여부와 원본과의 동일성

AGENTS.md(`551d27ae`)의 두 기준으로 평가한다. dotnetpj에는 실행 경로가 둘이다(0.3 결정 5).

| 경로 | 실행·플레이 가능 여부 | 원본과의 동일성 |
| --- | --- | --- |
| **기존 경로** `Netstorm.Game`<br>(`Netstorm.exe`, 현재 배포 대상) | **실행·플레이 가능.** 메인 메뉴와 캠페인 1-1·1-2를 근사 규칙으로 플레이할 수 있다. 새 기능은 넣지 않는 동결 상태다(실행 오류 수정만). | **낮다.** 녹화·팬게임 기반 근사(RGBA 합성, 자체 UI 배치, 근사 세션 `BattleSession`)라 원본 동일성의 기준이 될 수 없다. 새 경로가 같은 범위를 대체하면 삭제한다. |
| **새 경로** `Netstorm.Client.Host`<br>(개발 호스트, 아직 배포 안 함) | **메뉴 일부만 실행.** 타이틀, 메인 메뉴 8버튼, Help·Options(해상도·음량)·Credits(11.01 포함), 캠페인 선택, Edit 맵 목록·새 맵 이름까지다. **미션 월드가 연결되지 않아 플레이할 수 없다.** | **부품은 높지만 전체는 미확정.** cpppj를 구조 그대로 직역했고 원본 x86 fixture 232개로 부품을 대조했다(PORT_MAP L1 329행). 그러나 **cpppj와의 화면 캡처 비교(L4a)와 세션 상태 비교(L2·L3)는 아직 0건**이다. 부품 대조(L1) 통과를 화면·플레이 동일성으로 합산하지 않는다. |

### 단계별 진행 (R0~R8, 0.5)

| 단계 | 내용 | 이정표 | 상태 |
| --- | --- | --- | --- |
| R0 | 기준선·동기화(cpppj 오라클, PORT_MAP, `cpp_sync.py`) | — | ✅ 완료 |
| R1 | 8비트 화면 호스트·입력 큐·오라클 도구 | — | ✅ 완료 |
| R2 | 그리기 원시·팔레트·글꼴 | — | ✅ 본체 완료(세션 연결은 R4·R5) |
| R3 | Gump UI와 메인 메뉴 계열 | D1 메뉴 픽셀 일치 | 🔶 진행 중 — 위젯·호스트 대부분 이식, 캡처 비교 0건 |
| R4 | 월드 세션 조립 | M1 시작 상태 L2 | 🔶 진행 중 — 월드 아래층(Template·Fort·지형·섬·Player·저장소·프로세스·월드 본체)은 완료, **F3 세션은 껍데기와 Stub 관문뿐** |
| R5 | 월드 표시·카메라·HUD | D2 진입 화면 픽셀 일치 | ⬜ 미착수 |
| R6 | 플레이 입력·규칙·소리·AI | M2·M3 | ⬜ 미착수 |
| R7 | 미션 전체·Edit·설정·배포 | M4 | ⬜ 미착수(Edit 목록·`options.cfg` 저장 일부는 R3에서 선행) |
| R8 | AGENTS.md 4~8차, 60/120프레임, Linux, LAN 멀티 | — | ⬜ 미착수(즉시 요구인 11.01·Credits·외부 접속 문서화는 완료) |

### 숫자로 보는 현재

| 항목 | 값 |
| --- | --- |
| 마지막 커밋 | 0.287 `20261110-92 DN`(LAN TCP/UDP 루프백 사전 구현) |
| PORT_MAP | 642줄(자료 637행: L1 329 · port 308) |
| 원본 fixture | 연결 232개(`cpp_sync.py` 0종료). `reflected-commit`은 영향 검토 전이라 `f95a8db2` |
| 마지막 전체 검사 완주 | Core **1,831개**(0.285)·Client **906개**(0.286)·Assets **366개**(0.280, 실패 0). 0.287은 지시대로 검사 없이 사전 구현만 했다 |
| cpppj 기준 | cpppj는 확정 제품1248 / 표시 11.01(2026-11-10). dotnetpj가 마지막으로 영향 검토를 맞춘 시점은 단위861(2026-11-04) 이전이다 |

### 남은 일 — 순서대로

1. **재개 준비(바로):** 전체 회귀 재기준선 → cpppj 변경 영향 확인 → cpppj 재반영 요청 2건(Demo `Demoville`·Help 스크롤 화살표 색) → 새 경로에 cpppj와 같은 `session-dump`·고정 시각 실행 장치 만들기.
2. **R4 마무리(M1):** 정상 지형 생성 본체와 `RawFortWorldLoadBinding`(F2c) → `RawCampaignSession` 본체(Fort 적재·AI·메뉴, F3) → 실제 호스트에 미션 조립 → 34경로 시작 상태를 cpppj `session-dump`와 대조(L2).
3. **R3 마무리(D1):** 남은 위젯·모달·대화창을 옮기고 메뉴 화면을 cpppj와 캡처 비교한다. 이때 메뉴의 기본 실행이 새 경로로 바뀐다.
4. **R5(D2) → R6(M2·M3) → R7(M4):** 미션도 새 경로가 기본이 되고, 1-1·1-2가 새 경로에서 진행되면 기존 경로와 그 검사를 삭제한다. `Netstorm.exe`가 새 경로로 넘어간다(0.8 5번).
5. **R8:** AGENTS.md 4~8차(로컬 MCP·16:9, 사용자 홈 저장, 한국어, 안티앨리어싱·FSR 1.0, 시나리오 편집) → 60/120프레임 → Linux → LAN 멀티플레이·Outpost.
6. **사용자 직접 확인(AI 작업 아님):** 모든 시나리오의 자연 완주와 전체 동일성.

### 사용자 확인을 기다리는 것 (0.8)

- **5번** `Netstorm.exe`를 새 경로로 넘기는 방식(A 새 호스트가 이름을 이어받기 / B 기존 Game이 새 경로를 호출) — R3의 기본 실행 교체 전.
- **6번** 4차 로컬 MCP를 dotnetpj 클론에도 붙이는지 — cpppj MCP(단위935~937)가 나와서 이제 판단할 수 있다.
- **7번(신규)** AGENTS.md 9차(10.78 알려진 버그 34건)를 dotnetpj에 어떻게 반영할지.
- **2번** 한국어를 R8보다 앞당겨 새 경로에 넣을지.

---

<a id="dotnet-current-plan-20261028"></a>

## 0. 2026-10-28 현행 계획 — cpppj 그래픽 복원분을 기준으로 dotnetpj 재구성

### 0.1 현재 값 (2026-11-10, 단위 0.280 기준)

| 항목 | 값 |
| --- | --- |
| 진행 상태 | 0.287까지 커밋됐다. 전체 목표는 미완료다. 다음 단위(0.288)는 R4 F3 본체 계속(메뉴·Fort 적재 본체)부터 시작한다. |
| 현재 단계 | R0·R1 완료, R2 본체 완료, R3 진행 중, R4 진행 중(완료 범위는 0.9의 체크 목록과 1절 색인), R5 이후 미착수. |
| 실행·플레이 가능 여부 | 기존 경로 `Netstorm.Game`(`Netstorm.exe`, 0.96)은 1-1·1-2 근사 플레이가 가능하다. 새 경로 `Netstorm.Client.Host`는 메뉴 계열 일부까지이고 미션 월드가 없어 플레이할 수 없다. 기존 코드·검사 은퇴는 0건이다. |
| 원본과의 동일성 | 실제 메뉴·게임 캡처 비교는 0건이다. 오라클 메인 메뉴 캡처의 결정성만 확인했다([문서 5절](docs/dotnet-client-screen-20261028.md)). L1(원본 x86 fixture) 통과를 L2 이상으로 합산하지 않는다(0.6). |
| 마지막 구현 커밋 | 0.287 `20261110-92 DN`(LAN TCP/UDP 루프백 사전 구현). |
| 검사 | 마지막 전체 완주는 Core 1,831개(0.285)·Client 906개(0.286)·Assets 366개(0.280)이며 모두 실패 0이다. 0.287은 지시대로 검사를 건너뛰고 Release 빌드(경고·오류 0)만 확인했다. |
| 포트 맵 | `PORT_MAP.tsv` 총 642줄(자료 637행: L1 329, port 308). 0.287의 `LocalTcpTransport.h/.cpp`·`LocalUdpTransport.h/.cpp` → `LocalTcpTransport.cs`·`LocalUdpTransport.cs` 4행(port)이 마지막 추가다. |
| fixture | 연결 232개(개별 OK, `cpp_sync.py`는 미반영분 때문에 0종료 아님). 반영 커밋(`reflected-commit`)은 영향 검토 전이라 `f95a8db2`에 그대로 둔다. |
| cpppj 기준 | 이식은 단위마다 cpppj HEAD의 해당 파일만 읽고 미커밋 변경은 쓰지 않는다. 영향 검토 전이라 `reflected-commit`과 PORT_MAP 기존 행의 `base`는 올리지 않았다(신규 행만 파일별 HEAD). cpppj 현재 값(2026-11-10 확정 제품1248, 작업 중 1249)은 [LEFT_JOBS.md](LEFT_JOBS.md) 0.1절을 따른다. |
| 디컴파일 | 마지막 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**(R 전용은 2026-11-07, 같은 호스트, LEFT_JOBS.md 0.1절)이다. 이식 근거는 cpppj 소스·fixture·문서이므로 디컴파일이 필요 없다. 산출물을 직접 읽어야 할 때만 AGENTS.md대로 호스트를 확인하고 재디컴파일한다. |

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
  - 상태는 cpppj 헤드리스 출력(`--dump-world`·`--dump-types`·`--inspect-*`)과, 2026-11-09에 cpppj에 추가된 ui-script `session-dump` 단계(L2) 및 `--fixed-frame-ms` 고정 시각 실행(L3)으로 비교한다([형식](docs/exe/cpp-session-state-dump-20261109.md), 이식 계획 8절 요청 1·2의 해결).
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
| `package-dotnetpj.ps1` | 유지. 프로젝트·자산 구성이 바뀔 때마다 갱신하고 압축 해제 실행을 확인. 글꼴은 exe 옆 `fonts/`에 별도 파일로 넣는다(0.7) | — | 매 전환 |

### 0.5 실행 순서 (R0~R8)

각 단계의 괄호는 [이식 계획](docs/dotnet-cpp-port-plan-20261016.md) 5절의 W 번호다. W 번호와 그 fixture 목록은 근거 목록으로 계속 쓰고, 순서와 완료 기준은 이 절을 따른다.

**단계 전환 기준 (2026-10-31 사용자 결정, cpppj와 같음):**

- **자연 완주·전체 동일성:** 모든 시나리오(Early Missions, Priest Training, 캠페인, 데모)의 자연 완주와 전체 동일성은 사용자가 직접 실행·플레이하며 테스트한다. AI 작업 대상이 아니며, 단계에 남은 작업이 이것뿐이면 다음 단계로 넘어간다. 각 단계 완료 기준의 L1~L4 대조(원본 fixture, cpppj 오라클과의 상태·캡처 비교)는 계속 AI가 한다.
- **Windows 11:** Windows 10에서 오류 없이 구동되면 Windows 11에서도 문제가 없을 가능성이 높다고 본다. Windows 10 확인을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다.
- **제보 처리:** 사용자가 플레이 중 오류나 원본과의 차이를 제보하면 해당 항목을 AI 작업으로 올린다.

**R0 — 기준선과 동기화 재정비 (W01 확장) — 완료.** 오라클 확정, 재대조 영향표, `PORT_MAP`, `cpp_sync.py --ports*`를 마쳤고 뒤처진 C# 여섯 묶음도 고쳤다(1절).

**R1 — 8비트 화면 호스트·입력 큐·오라클 도구 (W14 일부, 9-1 B·E) — 완료.** `Netstorm.Client`, `Netstorm.Client.Host`, `--ui-script`, `snapshot`, `cpp_frame_compare.py`를 만들었다. 라벨 클릭·report·kernel-tell은 R3에, `session-dump`·고정 시각 실행은 R4(0.9 재개 준비)에 남는다.

**R2 — 그리기 원시·팔레트·글꼴 (W14 일부) — 본체 완료.** VFX, 색 표, 팔레트, `RawShapeToBuffer`, `SpriteDraw`, 글꼴, GIF의 본체와 L1 대조를 마쳤다. 실자산 연결과 대표 장면의 캡처 비교는 R3~R5에서 잇는다.

**R3 — Gump UI와 메인 메뉴 계열 (W15) → 이정표 D1: 메뉴 화면 픽셀 일치 — 진행 중**

- 옮길 것: Gump 프레임워크 전체(`Gump`·`GumpTree`·`GumpCreate`·`GumpLifetime`·`GumpEvents`·`GumpInput`·`GumpFrame`·`GumpDraw`·`GumpCollection`·`GumpVisual`·`GumpBackground*`·`GumpDecoration`·`GumpHelp`·`GumpPointerBoundary`·`GumpFormBinding`), 버튼·돌 버튼·메뉴·스크롤·텍스트·텍스트 입력·HTML 창·메시지·이미지·배경 GIF 위젯, `UberGump`·`UberRootGump`·`RawUberRootControls`·`NativeMainMenuState`, 로딩 화면(`MissionLoadingMessage`·`RawMenuLoading`), Tell·Dialog·StyleText·Markup·QuickHelp, 기본 커서(`Cursor`).
- 범위: 메인 메뉴 8버튼, Campaign/Demo/Help/Options(해상도·음량·Window Mode)/Credits/Tips/Quit, 캠페인 목록, 브리핑 창.
- **완료 기준:** 같은 ui-script로 각 화면·하위 메뉴·Help 쪽·Credits 쪽을 캡처해 cpppj와 차이 0이다. 이때부터 메뉴는 새 경로가 기본이 되고, 메뉴에서 미션에 들어가는 경로는 R5 전까지 기존 경로를 쓴다(0.8 1번).

**R4 — 월드 세션 조립 (W02·W03 잔여, W04~W07) → 이정표 M1: 시작 상태 L2 — 진행 중**

- 옮길 것: 시간·Kernel·Process·Regular, SP 저장소·보상·난수의 실제 연결, Fort 적재(객체·월드·메타·돈·소유자 마감·TypeNames·Deck), 지형·섬·농지·영역, 다리 연결, Player·기술·색, 미션 Start·로그인·타이머·Guide, AI 생성과 초기 갱신, 생산 Deck.
- `RawSurfaceWorld`/`RawWorldCore`와 `RawCampaignSession`의 소유 구조를 그대로 옮기고, 기존 `Original` 모듈을 그 안에 끼운다.
- **완료 기준:** 34경로 각각의 Load/Start 뒤 상태가 cpppj와 같다. cpppj 쪽 상태는 ui-script `session-dump` 단계로 얻고(판번호 1 형식), dotnetpj 호스트도 같은 형식의 덤프를 낸다. 비교 실행은 양쪽 모두 같은 `--fixed-frame-ms`와 빈 사용자 루트를 쓴다(4절).

**R5 — 월드 표시·카메라·HUD (W14 나머지, W15의 HUD) → 이정표 D2: 진입 화면 픽셀 일치**

- 월드 표시: `Renderer`·`DrawSort`·`DrawSubmission`·`RawRenderCollection`·`ObjectCollection`·`TileGroupCollection`·`PreviewCollection`·`PreviewDraw`·`SquidRenderer`, 지형 바인딩, 타입별 Draw(신전·받침·Factory·다리·레인 볼텍스·일반 자산·덱·연소), 시각 시스템(Burning·Construction·Dissolve·FieryTail·Lightning·Shrapnel·Smoke·RenderProcess), KenCloud, 파티클, 번개 화면, Fort 팔레트 전환.
- 카메라: `RawCameraView`·`RawCameraCommit`·`RawCameraPresent`·`CameraHome`·`UberGumpCamera`·`RawEdgeScroll`.
- HUD: `UberHud`·`HudDock`·`HudNsInfo`·채팅 표시, 미니맵(`MinimapGeometry`·`RawMinimapRaster`·`RawMinimapControls`), `FloatingSpGump`·`ProductionHintGump`, 덱(`InventoryGump`).
- **완료 기준:** 34경로의 진입 직후와 N프레임 뒤 캡처가 cpppj와 차이 0이다. 애니메이션 장면은 같은 `--set`(maxFPS·sleepPerLoop 등)과 고정 시계(`--fixed-frame-ms`) 조건에서 비교하고, 조건을 맞출 수 없는 장면은 비교 범위를 기록한다. 이 시점에 미션 실행의 기본도 새 경로로 바꾼다.

**R6 — 플레이 입력·규칙·소리 교체 (W08~W13, W16, W19의 AI) → 이정표 M2·M3**

- 입력·선택·커서: `UserInput`·`GameWorldInput`·`RawUserPick`·`RawObjectHit`·`RawUserSelection`·`RawUserHighlight`·`RawCampaignCursor*`·`RawCursor*`.
- 게임 메뉴: QWASZX(`RawProductionControls`·`ProductionShortcuts`), ESC 가로·하위 메뉴(`RawGameMenuBar`·`RawGameMenus`·`GameMenuActions`), Player List·F9, 우클릭 메뉴 전체(`RawObjectMenu*`·`RawKnowledgeMenu`·`RawFactoryMenu`·`RawPriestMenu`·`RawPlayerMenu`·`RawCarrierMenu`), Warning·MessageBoard.
- 규칙: 사제·경로·운반·수확, 배치·건설·생산, 전투·방어·파생 건물, 의식·신전·제단, 미션 진행·안내·승패, 공통 전략 AI(`RawAi*`). AI는 cpppj에서 실제 세션 연결까지 끝났으므로 W19 "대기"에서 이식 대상으로 바꾼다. 단 cpppj도 정상 간격의 AI 행동을 자연 플레이로 대조하지는 않았다. `arc-spire`·`acid-fence`·`ice-tower` fixture는 전체 몸체 클래스 대조용이라 해당 클래스를 옮길 때 연결한다.
- 소리: 소리 Process, 음악 선택·흐름, 장면 음악. 장치는 MonoGame.
- **완료 기준:** 고정 입력열의 L3 상태 해시(`--fixed-frame-ms` 실행의 프레임별 `session-dump`)와 대표 장면 캡처가 일치한다. 1-1의 건설 파이프라인이 원본 경로로 동작한다(M2). 1-1·1-2가 새 경로에서 진행되면 기존 경로와 그 검사를 삭제한다.

**R7 — 미션 전체·Edit·설정·배포 (W17·W18, 9-1 C·D·F, Edit) → 이정표 M4**

- 34경로 전체, 데모 3개의 반복, 진행 저장과 다음 미션 해제, Retry/Next/Abort를 옮긴다.
- Edit(편집 메뉴·섬 추가·저장·Test Battle·SaveGoMain, `.english` 등 언어 파일)를 옮긴다. AGENTS.md상 Edit 제외는 webpj에만 해당한다. 대응 `.english`가 없을 때 Test Battle이 끝나는 원본 동작은 이 단계까지 그대로 두고 R8의 8차에서 고친다.
- `options.cfg`를 원본의 시점·형식으로 읽고 쓴다(9-1 C). 단 전체화면 전환 뒤 재실행 오류는 허용하지 않는다. 9-1 D·F를 판정하고 Windows 10에서 확인한다. Windows 11 실기 검증은 단계 전환 조건이 아니다. 이 단계의 저장 위치는 원본과 같은 게임 폴더 `d/`이며, 사용자 홈으로의 이동은 R8의 5차에서 한다. Edit 맵 목록의 원문 50개 상한도 5차까지 원본대로 둔다.
- 배포 스크립트를 갱신하고(실행 파일 이름은 `Netstorm.exe`, 글꼴은 exe 옆 `fonts/`), 압축 해제한 사본에서 실행되는지 확인한다.
- **완료 기준:** 34경로 짧은 진입과 핵심 장면이 cpppj와 일치한다. 자연 완주(L5)는 사용자 확인 대상이라 완료 기준에 넣지 않는다.

<a id="dotnet-r8-requirements"></a>

**R8 — 요구사항 확장(① 완료 뒤, W20)**

AGENTS.md는 "cpppj의 **4~8차** 목표 내용이 dotnetpj 후순위 요구사항에도 포함된다"고 적는다(`98b2f20c`, 2026-11-02에 4~7차에서 넓어짐). 각 차수의 해석과 세부 계약은 cpppj의 구현([LEFT_JOBS.md](LEFT_JOBS.md) 1절의 단계별 현황)을 기준으로 맞추고, 결정 2에 따라 cpppj가 마감한 범위를 옮긴다. 기존 경로에 이미 있는 요구사항 기능(한국어 선택·와이드·전체화면)은 버리지 않고 새 경로에서 cpppj 방식으로 다시 만든다(0.7).

| 차수 | dotnetpj에서 할 일 | cpppj 현황(2026-11-10) |
| --- | --- | --- |
| **4차** 로컬 MCP·와이드 | 로컬 MCP(해석은 0.8 6번 — 게임 내 모든 조작·화면 캡처·소리 포함 녹화). 16:9 와이드 해상도 최대 1920×1080(해상도 지원만, 그래픽 크기 스케일 조정 없음, cpppj 논리 해상도 방식, 0.4 첫 행) | 구현 범위 마감(단위935~937, [MCP](docs/exe/cpp-mcp-server-20261106.md)) |
| **5차** 사용자 홈 저장·맵 목록 | 설정·캠페인 진행을 `%USERPROFILE%\Saved Games\Netstorm\Saves\`에 저장한다. 커스텀 맵은 기존 폴더와 `...\Netstorm\CustomMaps\` 양쪽에서 읽고, Edit 신규 맵은 `CustomMaps`에 저장한다. Edit 맵 목록에 스크롤을 넣고 개수 상한(원문 50)을 없앤다. **`aaafbbc4` 확장:** 관리자 권한 없이는 쓸 수 없는 위치에 설치해도 동작해야 하므로, 게임이 구동 중 자동 생성하는 **모든 파일·디렉터리**(`options.cfg`·`fullScreenStateFile.dat`·진행 저장·편집 작업본·캡처·로그 등)를 `%USERPROFILE%\Saved Games\Netstorm\` 아래로 옮기고 읽는 코드도 함께 바꾼다. 게임 폴더는 읽기 전용으로 가정한다. 기존 경로의 `settings.json`도 대상이다. 새 경로는 R7의 `options.cfg` 저장 구조 위에서 위치만 바꾸는 순서로 한다 | 구현 범위 마감(단위932~934, [사용자 루트](docs/exe/cpp-user-data-root-20261105.md): `CustomMaps\d\NAME.fort`, 최초 Edit의 원본 433맵 복사, 한 화면 50행 스크롤) |
| **6차** 한국어 | D2Coding 글리프를 8비트 화면에 그리는 방식(단색 또는 팔레트 단계)을 정해 구현한다. 원문 8비트 문자열과 화면 표시용 부호화를 구분한다([cpppj 글꼴 경계](docs/exe/cpp-font-face-encoding-20261103.md)). AGENTS.md의 한국어 명칭 규칙을 따른다 | 일부 구현(단위938 번역 502·Language 메뉴·korean→english 대체, K1~K5 남음) |
| **7차** 안티앨리어싱·FSR 1.0 | Options에서 안티앨리어싱·FSR 1.0을 선택적으로 켠다(둘 다 기본값 꺼짐). FSR 1.0 지원 환경이면 해상도 목록에 2560×1440·3840×2160을 넣고, 이때 FSR을 자동으로 켜며 실질 해상도는 1920×1080으로 두고 업스케일한다. FSR 3.1 안은 쓰지 않는다 | 구현 범위 마감(단위939) |
| **8차** 시나리오 편집(이번 정리에서 처음 반영) | Edit에서 맵·유닛 배치뿐 아니라 시나리오 자체를 편집한다. 편집 모드 전용 UI를 따로 두어 플레이어 초기 자본·테크·적 구성·동맹 등을 화면에서 편집한다. 새 맵을 만들 때 `.english` 기본 내용을 자동 생성하고, `.english`가 없을 때 Test Battle에서 게임이 끝나는 문제를 고친다 | 미구현(S1~S4 계획) |

- **글꼴 배포(`75e4eed9`, 2026-11-10):** 글꼴 파일은 실행 파일(어셈블리 리소스 포함)에 내장하지 않는다. 배포판의 `Netstorm.exe`와 같은 폴더에 `fonts/` 디렉터리를 두고 별도 파일로 읽는다. 한국어는 저장소 `fonts/`의 D2Coding, 일본어·기타 언어는 같은 폴더의 Noto Sans·Noto Sans JP를 쓴다. 라이선스 파일(`D2Coding-License.txt`·`NotoSans-License.txt`)도 함께 배포한다. 현재 기존 경로는 D2Coding 1.3.2를 출력 폴더 `fonts/`로 복사하고(`Netstorm.Game.csproj`의 `CopyToOutputDirectory`) 배포 스크립트도 별도 파일로 넣으므로 이 규칙을 지킨다. 새 경로에 TTF 출력을 넣을 때(6차)와 `Netstorm.exe`를 새 경로로 넘길 때 같은 방식을 유지한다.
- **9차(알려진 버그 34건, `b5197d48`):** AGENTS.md는 9차를 cpppj 목표로만 두고 dotnetpj 포함 범위를 4~8차로 적었다. 처리 방식은 0.8 7번의 기본값을 따른다.
- 그 뒤 60/120프레임, Linux(DesktopGL), TCP/IP 로컬 멀티플레이·Outpost(최후순위)를 진행한다.

### 0.6 검증 단계와 기록 규칙

- **L1:** 원본 x86 fixture(기존 방식). **L2:** 같은 미션 Load/Start 상태가 cpppj와 같다(cpppj `session-dump`). **L3:** 같은 고정 시계·입력열로 N프레임 진행한 상태 해시가 같다(cpppj `--fixed-frame-ms` + 프레임별 `session-dump`). **L4a:** 같은 ui-script의 논리 캡처가 픽셀 차이 0이다. **L4b:** 소리 시점과 곡 선택이 같다. **L5:** 자연 완주·저장·재실행. 사용자 확인 대상이라 AI 작업과 단계 완료 기준에 넣지 않는다(0.5).
- cpppj 캡처·덤프는 오라클 SHA·스크립트·`--set` 값·`--fixed-frame-ms` 값·프레임과 함께 기록한다. cpppj가 논리 캡처(`snapshot`)와 실제 출력 캡처(`snapshot-output`)를 구별하듯 dotnetpj도 둘을 구별한다. 실행에는 `originals/`가 아니라 `extracted/` 아래의 데이터 사본과 빈 사용자 루트를 쓴다.
- 한 단위를 마칠 때 기록할 것: 옮긴 cpppj 파일과 기준 커밋, 연결 fixture와 행 수, 이식한 cpppj 검사 수, 은퇴시킨 기존 코드와 검사, 비교한 캡처·덤프 수와 차이, 남은 경계. L1 통과를 L2 이상으로 합산하지 않는다. 기존 근사 검사의 은퇴로 실패를 감추지 않으며, 원본 fixture 기대값을 C# 결과로 바꾸지 않는다.
- 클론·cpppj 창 실행은 AGENTS.md의 확인·예외 규칙을 따른다(VM-W11-Codex·HJOW-Athlon·vm-debian-codex는 예외 PC). HJOW-X3D를 포함한 그 밖의 PC에서는 실행 전에 목적을 설명하고 명시적 확인을 받는다. 이 계획은 원본 게임을 실행할 필요가 없다.

### 0.7 계속 유효한 결정·제약

- **목표 순서:** ① 원본 10.78과 동일한 동작 → ② 요구사항(화면비·60/120프레임·다국어·재실행 오류 방지, AGENTS.md 4~8차) → ③ Linux. 이미 들어간 요구사항 기능(한국어 선택·와이드·전체화면)은 버리지 않는다. 새 경로로 옮길 때 cpppj 방식(논리 해상도·호환 전체화면)으로 다시 만들고, 한국어는 R8에서 다시 만든다(0.8 2번).
- **AGENTS.md 상위 요구(2026-11-10 `75e4eed9` 기준):**
  - **후순위 요구:** cpppj의 4~8차가 dotnetpj 후순위 요구에 포함된다(`98b2f20c`). 내용과 cpppj 현황은 [R8 표](#dotnet-r8-requirements)에 있다. 9차는 0.8 7번이다.
  - **즉시 적용 요구:** Windows 실행 파일 이름 `Netstorm.exe`(`ef0c6b91`, 기존 경로는 0.96에서 적용, 새 경로로의 이름 이전은 0.8 5번), 11.01 표기·Credits 3항목(`462c9a64`, 0.150 완료), 외부 접속 기능 문서화·클라이언트 유효성 검사 비활성화(`0190ba95`, 0.151 완료, 갱신 규칙은 아래 '외부 접속 문서 갱신').
  - **글꼴(`75e4eed9`):** 글꼴은 exe에 내장하지 않고 exe 옆 `fonts/`에 둔다(R8 표 아래 항목). 영어 화면은 원본 `.chfnt`를 쓴다(3절 2번).
  - **진행 요약(`551d27ae`):** 진행 상황 요약 요청에는 cpppj·dotnetpj·webpj를 각각 실행·플레이 가능 여부와 원본 동일성으로 평가한다. dotnetpj 쪽 값은 [한눈에 보기](#dotnet-overview)를 쓴다.
  - **문서 보호(`3051892a`):** AGENTS.md와 루트 README.md의 "개요" 단락은 수정하지 않는다. README.en.md의 "Overview"는 README.md "개요"의 번역으로만 고친다. dotnetpj 상태를 README에 반영할 때 이 단락을 건드리지 않는다.
  - **기존 파일 재활용(`851054a8`):** 원 개발사 지원이 끊긴 게임이므로 필요하면 기존 파일을 재활용할 수 있다(원본·CD 디렉터리 자체는 수정하지 않는다).
- **9-1 의도적 차이(2026-10-10 사용자 결정)의 적용:** A(시간)는 R4의 Kernel/Regular, B(입력 큐)는 R1, C(`options.cfg`)는 R7, D(바쁜 대기)·F(창 동작)는 R7에서 판정, E(부분 다시 그리기)는 R1·R5에서 처리한다.
- **글꼴·명칭·주석:** 영어는 원본 `.chfnt`, 한국어는 D2Coding(3절 2번). D2Coding은 1.3.2를 유지한다(3절 7번, 저장소에는 1.4.0도 있다). 기존 주석은 대상을 고칠 때 정비한다(3절 8번). AGENTS.md의 한국어 명칭 규칙을 따른다.
- **cpppj 공통 제약을 준용한다.**
  - CHEAT 항목은 Edit 편집에서만 나타난다.
  - 공격형 공중 3종(훨리기그·맨오워·더스트 데빌)은 cpppj 코드를 옮기되, 이 타입을 위한 실제 생산·생성·우클릭 재현과 추가 실행 관측은 하지 않는다.
  - 연녹색 직사각형 분석은 보류한다(원본에서도 관찰됨).
  - 대응 `.english`가 없을 때 Test Battle이 끝나는 것은 원본 동작이다. 목표 ①(R7까지)에서는 그대로 두고, AGENTS.md 8차에 따라 R8에서 고친다(새 맵의 `.english` 기본 내용 자동 생성 포함).
  - 모든 시나리오의 자연 완주와 전체 동일성은 사용자가 플레이하며 확인한다(0.5). 멀티플레이는 후순위다.
  - 실제 실행 검사에서 사제 포획/희생, 특정 적 공격, 시나리오 승리를 재현해야 하는 테스트는 건너뛴다([TODO.md](TODO.md)).
- **cpppj의 미완 범위는 cpppj를 따라간다:** View/zoom, 도킹, DD 조정·확인 UI 잔여, Chat GUI 드래그, 미니맵 내용물·grid·항목별 debug, Renderer 시차 경계는 [LEFT_JOBS.md](LEFT_JOBS.md)의 잔여다. cpppj에 반영되기 전에는 dotnetpj에서 추정으로 만들지 않는다. 독립 미니맵, Credits 자동 전환, Chat 입력·전달, 마나 103·배율 Draw는 이미 cpppj에 반영됐으므로 이식 대상이다. 전체 화면·음향 동일성 대조는 사용자 확인 대상이다.
- **근거 우선순위:** 원본 fixture → cpppj 보고서의 계약·경계 → cpppj 소스 → 녹화·스크린샷. cpppj는 HEAD만 쓰고 미커밋 변경은 쓰지 않는다. `cpppj/`·`LEFT_JOBS.md`는 수정하지 않고, 필요한 요청은 이 문서와 이식 계획 8절에 적어 사용자에게 전달한다. (2026-11-02·2026-11-04에 사용자 요청으로 `LEFT_JOBS.md`와 이 문서를 함께 정리한 것은 이 원칙에 대한 사용자 지시 예외다.)
- **외부 접속 문서 갱신:** 새 경로의 Help 링크(R3)·시작 화면의 검증 안내·`Netstorm.Net`(3차)을 옮길 때마다 [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md)의 소비자 목록을 갱신한다. 구 클라이언트 검증·master/root 인터넷 서버·ping/진단은 이식해도 연결하지 않고 성공을 합성하지 않는다. 사용자가 선택하는 도움말 링크 실행과 로컬 서버/LAN 접속(3차)은 별도 경계다(0.151).
- **보호·커밋:** 원본/CD·AGENTS.md는 수정하지 않는다. push는 하지 않는다. 커밋 메시지는 `YYYYMMDD-NN DN 요약`이다. 새 문서·코드는 UTF-8과 한국어 주석 규칙을 따른다.

### 0.8 사용자 확인이 필요한 것 (확인 전까지 에이전트 기본값으로 진행)

1. **기존 경로 처리(결정 5):** R3부터는 메뉴, R5부터는 미션의 기본 실행이 새 경로로 바뀐다. 기존 1-1·1-2 근사 플레이는 `--legacy`로만 남기고 R6 뒤 삭제한다. 그동안 새 경로에서 플레이할 수 있는 범위가 일시적으로 줄 수 있다.
2. **한국어(0.7):** 새 경로는 R8 전까지 영어 원본 화면만 제공하고, 한국어 선택은 `--legacy`에서만 쓸 수 있다. 한국어가 계속 필요하면 D2Coding 단색 글리프 출력을 R3으로 앞당긴다.
3. **cpppj 미복원 경로(결정 6):** 새 경로는 cpppj와 같은 명시 오류·경계를 쓴다. 이전 3절 9번의 "근사 유지" 기본값은 기존 경로에만 남는다.
4. **프로젝트 배치(결정 3) — 해결:** `Netstorm.Client`로 만들었다.
5. **`Netstorm.exe`를 어느 실행 파일이 갖는가(AGENTS.md `ef0c6b91`):** dotnetpj에는 실행 파일이 둘이다. 기존 게임 `Netstorm.Game`(0.96에서 `NetstormClone.exe`를 `Netstorm.exe`로 바꿨다, 배포 대상)과 새 경로의 개발 호스트 `Netstorm.Client.Host`(`Netstorm.Client.Host.exe`, 아직 배포하지 않음)다. 한 배포 폴더에 `Netstorm.exe`는 하나뿐이다. **현재:** 배포되는 `Netstorm.Game`의 `AssemblyName`이 이미 `Netstorm`이라 AGENTS.md 요구는 충족한다. 결정 5의 기본 실행 교체 때(R3 메뉴·R5 미션) `Netstorm.exe`가 새 경로로 진입하게 하는 방법은 둘이다. A) `Netstorm.Client.Host`가 `Netstorm.exe`를 이어받고 기존 Game은 `--legacy` 전용 이름(예: `NetstormLegacy`)으로 내리거나 새 호스트에 흡수한다. B) `Netstorm.Game`이 계속 `Netstorm.exe`이고 새 경로를 라이브러리로 호출한다. 어느 쪽이든 사용자가 보는 이름은 `Netstorm.exe` 하나다. 원하는 방식이 있으면 R3 착수 전에 알려 달라.
6. **4차 MCP의 dotnetpj 해석:** AGENTS.md는 'cpppj의 4~8차 목표 내용이 dotnetpj 후순위 요구사항에도 포함된다'고만 적었다(`98b2f20c`에서 4~7차를 4~8차로 넓힘). 4차 MCP는 dotnetpj 개발을 위해 게임을 분석하는 용도라서, dotnetpj 클론에도 같은 MCP를 붙이라는 뜻인지 cpppj 쪽 MCP를 개발에 활용하라는 뜻인지 문장만으로는 정해지지 않는다. `990a3541`이 4차에 "게임 내에서 모든 조작, 화면 캡처 및 녹화(사운드 포함)가 가능하여야 한다"를 명시했고, `5867f35e`가 이를 "로컬 MCP"로 고쳐 쓰며 와이드 해상도(16:9, 최대 1920×1080)를 함께 묶었다. **기본값:** R8 전에는 구현하지 않는다. 어느 해석이든 R8의 4차 범위에 이 세 가지(전체 조작·캡처·소리 포함 녹화)를 포함한다. cpppj MCP가 단위935~937에서 마감됐으므로([문서](docs/exe/cpp-mcp-server-20261106.md)) 이제 사용자 확인을 받을 수 있다. 와이드 시야 확장의 방식은 0.4의 `Game/NetstormGame` 교체 행(cpppj의 논리 해상도 방식)을 따른다.
7. **9차(10.78 알려진 버그 34건)의 dotnetpj 반영(AGENTS.md `b5197d48`, 2026-11-08 추가):** AGENTS.md는 9차를 cpppj 목표에만 두었고 dotnetpj 포함 범위는 4~8차다. cpppj는 34건을 NB01~NB34로 등록만 했고 조사·수정(B1~B4)은 미착수다([목록](docs/exe/cpp-known-bugs-plan-20261109.md)). **기본값:** dotnetpj가 34건을 따로 조사·수정하지는 않는다. cpppj에 9차 수정이 들어간 뒤 그 파일을 이식하게 되면 결정 2·4(cpppj가 판정 기준)에 따라 cpppj HEAD대로 함께 옮기고, 해당 단위 기록에 "9차 수정 NBxx 포함"이라고 표시한다. 9차 수정을 dotnetpj에서 빼기(원본 동작 유지)를 원하거나 dotnetpj에서도 따로 조사하기를 원하면 이 기본값을 바꾼다.

### 0.9 바로 다음 작업

체크 항목은 AI 작업 대상이다. 자연 완주, 전체 동일성, Windows 11 실기는 체크 항목이 아니며 단계 전환을 막지 않는다(0.5).

**재개 준비 (다음 단위 0.280부터, 이 순서로):**

- [x] **전체 회귀 재기준선(0.280):** `dotnet test dotnetpj/Netstorm.sln -c Release` 완주 — Assets 366개·Client 895개·Core 1,826개(합계 3,087개) 실패 0. 이전 기준선(Core 1,576개/0.211, Client 858개·Assets 366개/0.157) 이후 68단위(0.212~0.279) 동안의 증가분(250·37개)을 흡수했다.
- [x] **cpppj 영향 확인(0.281):** `git diff --name-only f95a8db2..HEAD -- cpppj/`로 PORT_MAP 대조 — 자료 629행 중 458행이 영향(L1 247·port 211, 고유 cpppj 파일 387개, 변경 파일 4,323개). 영역은 `o/` 282행·`client/` 170행·`app/` 4행·`zacket/` 2행이다. R4 우선 65행(ClientMain·RawMenuControls·Renderer·Screen 계열·RawBridge·RawIsland·RawMissionStart·RawPlayer·RawSurfaceWorld·RawNormalTerrainWorldBinding 등)을 목록으로 뽑았다. `reflected-commit`·`base`는 올리지 않았다(재대조 완료 범위만 올린다는 규칙). 재대조는 R4 작업에서 파일을 건드릴 때 늦게 하는 방식으로 한다. 기능 교정 중 dotnetpj와 직접 닿는 것은 `hpPerSec` TYPE 피해율(1246·1247, 기존 경로 `BattleSession`만 사용하므로 새 경로 이식 때 반영)·재반영 2건(0.282)·`session-dump`·`--fixed-frame-ms`(0.283 이후)다. `--ports`는 0.147 이후 시간 초과라 쓰지 않았다. 작업 트리의 미커밋 cpppj 변경(1249 작업 중)은 보지 않았다.
- [x] **cpppj 재반영 요청 2건(0.282)**([cpp-webpj-requests-20261109.md](docs/exe/cpp-webpj-requests-20261109.md) 3·4절, cpppj `c91d9b84`가 원문 기준으로 바꾼 것):
  - Demo 버튼 Tell 이름: `Netstorm.Client/Host/HostMainMenu.cs`의 `("Tell","DEMOVILLE")`를 원문과 같은 `("Tell","Demoville")`로 바꿨고 `HostStartupSceneTests`의 `DEMOVILLE` 단언(`priorTell` 포함)을 고쳤다. `tell.english`의 섹션 머리 `[DEMOVILLE]`은 자료라 그대로이며, 섹션 조회는 대소문자를 가리지 않는다(`Config.Section` ASCII 소문자 비교).
  - Help 스크롤 화살표 색: `Netstorm.Client/Host/HostHtmlText.cs`가 ScrollingText 화살표에 `SliderType=menu.BackgroundType, SliderFrame=1`을 넘겨 팔레트 1번(붉은 갈색)으로 그렸다. 원본처럼 현재 팔레트의 갈색(named 9)·검정(basic 0) 색 번호를 넘긴다(cpppj `RawMenuControls::SetPalette`, `titleState.sliderType/sliderFrame`). 새 `SliderArrowType`·`SliderArrowFrame` 읽기 속성과 내부 `WidgetState` 노출로 검사에 연결했다.
- [x] **L2·L3 장치(0.283·0.284):** cpppj의 ui-script `session-dump` 단계(판번호 1 형식)와 `--fixed-frame-ms` 고정 시각 실행을 `Netstorm.Client.Host`에도 같은 문법·형식으로 만들었다([형식](docs/exe/cpp-session-state-dump-20261109.md)). 미션 월드가 없을 때의 `world 0` 덤프(머리·난수·Kernel까지)부터 맞추고 세션 조립이 진행될 때마다 행을 늘린다. 이식 계획 8절 요청 1·2의 dotnetpj 쪽은 해결로 표시한다(요청 3~5는 남음).
  - **고정 시각 실행(0.283):** `--fixed-frame-ms N`(1~60000)·`--fixed-unix-seconds S`(단독 사용 거부)를 `HostOptions`에 받고, `HostTimeSource`(기본 OS 시계·고정 가상 시계, 원점 1,000,000ms·946684800초, uint32 감김)를 `HostFrameDriver.BeginFrame`마다 전진·`ClientHostGame` 메뉴 시각에 연결했다. `Program`은 호스트 시계 원점보다 먼저 켠다. cpppj 전역과 달리 호스트 소유 인스턴스다.
  - **`session-dump`·world 0 덤프(0.284):** `UiScriptRunner`에 `session-dump` 관측 단계(그리기 뒤 저장, 빈 경로 거부·미연결 장치 명시 예외)와 `BindSessionDump`를 넣고, `SessionStateDump.FormatWorld0`(판번호 1·ASCII·LF·탭, float/double 비트 정수, FNV-1a 64 포함)과 `HostSessionDump`(호스트 소유 Kernel·GameRandom·CrtRandom·프레임의 world 0 쓰기, 게임 시각은 세션 시계 미연결이라 0)를 `ClientHostGame`에 연결했다. `HostFrameDriver.BindSessionDump`로 늦게 묶는다.

**R4 월드 세션 조립 (재개 지점):**

- 완료 묶음(상세는 1절 색인과 이관 파일의 단위 기록):
  - [x] Template/DataManager와 저장소 TS0~TS6(0.128~0.136)
  - [x] Fort 타입 표·적재·저장 F1·F2a~F2c-12(0.137~0.155)
  - [x] 지형·섬 생성 1·2차와 Fort 사제 준비(0.156~0.160), 일반지형 요청형(0.174)
  - [x] 다리·섬 연결 본체와 판본별 전수 재생(0.161·0.162)
  - [x] F2d Player Core·초기화·Chat·참여·CoreData·Player 월드, 캠페인 월드 전역·적재, 서피스 삭제 분배기, 저장 나머지(0.163~0.171)
  - [x] 서피스 월드 자료형·권한·바람, 게임 난수·보드 각도·바람 진행, 저장소 껍데기·해시 선택·물리 자료형·풀 생성(0.172~0.180)
  - [x] 게임 시계, 커널·프로세스 상태·기본/정기 프로세스, 프로세스 호스트(저장소·묶음·껍데기·부착·통지/검색·삭제·정기 실행·정기 예약·대여), 표시 진단(0.181~0.205·0.213~0.216). Host 전송(0.220)은 검사 없는 사전 구현
  - [x] 지형 바인딩 계약·패턴 개수·첫 기록·군집 후처리·정상 바인딩 껍데기·정상 지형 후처리 관문·지형 생성 입력(0.200·0.202·0.203·0.207·0.211·0.217·0.225)
  - [x] 서피스 월드 껍데기, Fort 적재·Forms 부착·Form Pop 바인딩, 공장 부착·폼 생성·분리·진단·HP·Take·약화·현재 생성(0.206·0.209·0.210·0.212·0.213·0.215·0.218·0.219·0.221~0.224)
  - [x] 월드 본체 조립(0.226~0.264): 변경 플래그·수정 가능 TYPE·로딩 깊이·설치 모드·객체 조회·프레임 시각·적재 구간·삭제 상태/연결/분배·커널 차용·그래프 적재·소유자/받침 지정·종속 해제·프레임 소유·보고서·생성/해제/배치 연결·표면 순회·붕괴 몸체/스캔·표시 부모/수집/연결·연결 조회·초기 적재·`RunFrame`·호스트 대여·상태 입출력 경계·종료 반납·Core 보기·수명 분배기·사건 처리기·감시 쌍. 월드 전송(0.230)은 사전 구현
  - [x] F3 세션 껍데기·Fort 씨앗 적재·프레임/보고서 위임·섬 개수 2종(0.265~0.271), 세션 연결 사전 구현(0.268, 검사 없음)
  - [x] F3 Stub 관문 8종(0.272~0.279): `StartConstruction`, `CreateAt`/`DestroyAt`, `LocalOwner`/`TemplateOwner`/`BattleActive`, `ReloadTheamAt`, `AddRandomObjects`, `GameMenuReloadCount`/`GameMenuReloadSid`, `SynchronizeProductionContents`, `MetaMenuType`. **주소·미적재·본체 미연결의 명시 오류만 있고 본체는 없다**(결정 6).
- 남은 항목:
  - [x] **F2c 정상 생성 본체(0.285):** `RawNormalTerrainWorldBinding`에 `Generate`·`GenerateOriginal`·`StampContext` 본체를 넣었다(cpppj `RawNormalTerrainWorldBinding.cpp` 직역, `port`). 월드 장치(난수·CRT·도장 효과·군집 배치)는 호출자가 넘기고 군집은 생성 입력 연결 때 보관한다. 32비트 감김·signed 나눗셈은 unchecked로 원본 비트 해석을 따른다. `RawFortWorldLoadBinding`은 대리자 주입 껍데기 그대로이며 실제 본체 연결은 F3 실제 호스트·월드 조립에서 한다.
  - [ ] **F3 본체(진행 중):** 위 Stub 8종의 실제 본체(Fort 적재 본체·ownership·위치 명령 파서·완전 배치·outposts/temples 목록·Deck 동기화·메타메뉴 별칭 조회), 메뉴, 세션 진단의 AI·연결·메뉴 부분(0.269). 네트워크 연결(0.268)은 3차 범위다. 미션 Fort 적재와 Edit 진입은 F3까지 끝나야 동작한다.
  - [x] **F3 소유자 본체(0.286):** `LocalOwner`·`TemplateOwner`·`BattleActive`에 실제 본체를 넣었다. 세션이 `PlayerWorldState`(battleOwner·templateOwner 기본값 1, 원본 `gameplayGlobals->player` 상당)와 자산 Battle 상태(기본 거짓, 원본 `assetState.inBattle` 상당)를 소유하며 설치 본체가 갱신할 내부 `PlayerState`·`InBattle`을 둔다(cpppj 공유 참조 `&playerState.battleOwner` 상당). 나머지 5종(건설·위치·지형·배치·재장전·생산·메타메뉴)은 본체 미연결 Stub 그대로다(필요 상태가 세션에 없어 결정 6으로 둔다).
  - [ ] **AI:** AI 설정의 Session 소유자, 실제 AI 생성자·초기 갱신·전략 AI(0.71·0.72).
  - [ ] **실제 호스트·월드 조립:** 미션 객체, Fort reader, Guide 교체·수명, 미션 제어 입력, 공통 Start, AI 설정·생성, Guide 단계·Lifecycle은 이식과 L1 대조를 마쳤지만 검사에서는 명시 효과로만 연결돼 있다. 실제 호스트에 분배기를 설치하고 같은 Player·제한 표·메뉴·월드 효과에 연결한다(0.66, 0.68, 0.69, 0.75~0.79). Tutorial Init의 실제 템플릿/옵션 소유자·요새 선택·상태 실행·실제 타입 조회·호스트 단계 연결(0.105~0.110), Start의 실제 Config·Player·SP·AI·Gump 효과(0.70), `PlayerParticipation`의 실제 세션 바인딩과 동맹 수신 `ReceiveAlliance` 등(0.74)이 남았다.
  - [ ] **미뤄 둔 전수 재생(월드 스택이 생겼으므로 F3 조립과 함께):** `islandpostpop-x86.tsv` 1988행·`islandlifecycle-x86.tsv` 2055행(0.157·0.158·0.160, 끝나면 해당 PORT_MAP 행을 L1로 올린다), 지형 패턴 개수 장면값 재생(0.202), Player 월드의 CD·전수 재생(0.167), 0.154에서 옮기지 않은 cpp 검사(`FortWorldLoadBindingTests`·`NormalTerrainWorldBindingTests`·`IslandLifecycleTests`·`IslandPostPopTests` 등), 0.134~0.139에서 F3로 미룬 검사(`CampaignMissionFortPathTests`·`CampaignLoginObjectStreamTests`·`CampaignMissionReloadSourceTests`·`TemplateDiagnosticClientTests` 일부·`SoloMapCatalogTests`·`CampaignNewFortTests`·`CampaignNewFortConfigTests`·`FortMoneyLoad_SharesCurrentWorldOwnerStoreAndRandom`).
  - [ ] **M1 판정:** 34경로의 Load/Start 뒤 상태를 cpppj `session-dump`와 대조한다(L2).
- [ ] **R3 잔여 → 이정표 D1:** 위젯 본체와 실제 호스트의 표시·입력 경로는 대부분 옮겼으나(1절 색인의 호스트·메뉴 행) R3 전체 완료로 세지 않는다. 남은 것:
  - 대화창 분리(`ParseDialogPage`)와 `ConfigOverwriteAndQueueLifetime`의 남은 부분, MessageGump·대화창
  - 맵/파일/기술 목록 owner, 실제 상위 메뉴 소유·분배, `RawMenuControls`의 전체 모달·등록 소유자와 링크·명령 소비자, Guide 우선 연결 등 기타 상위 명령·실제 맵/기술 목록 조립, HTML 그림 자산과 다른 파생·월드 통합 출력
  - 전체 자산 조립, `ReadAssetQuickHelpType`의 TYPE/AST·요구 에너지 후처리, 음향
  - `RawTextEntryControls`의 Console·부분 생성 정리·시스템 클립보드, Save as·숫자 창·ChatWindow, `RawBackgroundGifControls`의 노드·창 소유자
  - 호스트 프레임·입력 큐, report·kernel-tell: 메인 메뉴 라벨 좌표는 실제 버튼에 연결했고(0.85) 하위 화면 라벨과 실제 호스트 명령 소비는 남는다.
  - D1 메뉴 캡처 비교. R0-1의 오라클은 당시 `0cc1d33d` 빌드이고 cpppj 산출물 이름도 `Netstorm.exe`로 바뀌었으므로, 비교 전에 현재 HEAD로 다시 빌드해 SHA를 기록한다. 재반영 요청 2건(위)을 먼저 끝낸다.
- [ ] **R2 잔여 세션 연결:** 실자산 어댑터, 수집·정렬·kind 분배, `SpriteDraw` 클래스·그림자 alias 공유, `owner-capture` C행 480개는 R4·R5에서 잇는다.
- [ ] **9-1 A 나머지(2절):** 정수 틱을 실제 시각·원본 타이머로 옮기는 일을 세션 교체와 함께 처리한다.
- [ ] 이후 R3(D1) → R4(M1) → R5(D2) → R6 → R7 → R8 순서로 진행하고, 단위마다 이 절의 체크와 [0.4](#04-기존-dotnetpj-소스의-재수정-판정) 판정표를 갱신한다. R8의 차수별 할 일은 [R8 표](#dotnet-r8-requirements)를 따른다.
- 즉시 적용 요구인 **11.01 표기·Credits**(0.150)와 **외부 접속 문서화·클라이언트 유효성 검사 비활성화**(0.151)는 완료했다(1절 색인). 외부 접속 문서의 갱신 규칙은 0.7을 따른다.

**이월한 이식 한계 (이후 단위에서 해소됐다는 기록이 없는 것만):**

- 0.80: 초기 구름 카메라 원점은 임시 범위이고 실제 카메라(R5)·시간별 이동·부분 dirty는 미연결이다. 512×512 구름 파일이 없거나 크기가 다를 때의 원본 생성 fallback은 이식하지 않아 명시 오류다.
- 0.81: 반복 `--set`은 C# 호스트가 병합하고 같은 이름은 마지막 값을 쓰지만 cpppj는 마지막 묶음만 보관한다. 오라클 연결 때 계약 차이를 정리한다.
- 0.86·0.87: 프레임 사이의 빠른 물리 입력 이력, 공통 음향·음악(R6 범위)과 옵션 변경 중 재생 관리가 남았다.
- 0.93: 부분 생성 중 돌 버튼 내부 배경 할당 실패의 모든 사례를 검증하지 않았다.
- 0.126: cpppj와의 이관 장부 교차 읽기, `MatchDisk`의 Win32 특수 경계(절대 경로·긴 경로)는 검증하지 않았다. 실제 Editor State1 월드·저장·Save as의 Template/현재 이름 세션 소유자는 미이식이다(F3 범위).
- 0.127: Windows 외의 `WriteAtomic`은 write-through·재시도가 없는 준비용 `File.Move`이며 durable 검증이 아니다. `ReadOnly`의 Windows 외 동작은 관측하지 않았다. 새 파일 장치의 이름 정규화(ASCII만 접음)와 기존 `GameFileSystem.NormalizePath`(비ASCII도 접음)의 차이는 고치지 않았다. cpppj `RelativePath`는 옮기지 않았다.
- 0.128: 기존 `Netstorm.Assets.FortFile`은 연결하지 않은 이전 독립 판독기로 남아 있으며 은퇴는 R4 Fort 적재 단위에서 한다.
- 0.132: `WriteStream`은 입력을 배열로 복사하므로 입력이 같은 섹션 버퍼의 구간이면 원본의 DWORD 순방향 복사와 다를 수 있다(현재 호출처에는 없음, 소비자를 옮길 때 확인).
- 0.134: `AllocateEmptyManager`의 할당 실패(NULL) 경로는 이식하지 않았다.
- 0.135: 짝이 맞지 않는 UTF-8(옛 ANSI) 이름은 manager에서 꺼낼 때 U+FFFD로 바뀌어 왕복하지 않는다. `Dump`의 파일명은 바이트 문자열 그대로 출력된다.
- 0.136: `HostFortNameConfirmation`의 `header`·`setName` 실제 연결(현재 Template 소유자·전체 snapshot)은 F3이다. `FortCurrentNameDevices`의 두 dirty DWORD 주소 비교는 생략했다(호출자가 `ClientFortGlobals.Catalog`에 연결해야 함). cpppj는 비ASCII 이름을 CP1252→UTF-8로 이중 인코딩하는 차이가 있다.
- 0.137·0.138: `HostEditorSelection`의 TYPE 개수 188 상수 교체와 `ReadSections` 검사의 `Parse` 이식은 F3(세션 표 소유)이다. `from_chars` 지수 언더플로 거부는 MSVC 동작을 확인하지 못해 옮기지 않았다. `TypeRequiredEnergyTests`·`ArcherTypeDataTests`·`CanonTypeDecoder` 계열과 세션 통합 검사는 F2·F3에서 잇는다.
- 0.152·0.208·0.220·0.230·0.268: `LocalMissionTransport`·로컬 미션 패킷·Host 전송(`WriteFormPacket`)·월드 전송(`SynchronizePeer`·`TransmitDeletion`)·세션 연결 Stub은 검사 없는 사전 구현이거나 명시 오류다. Zone·로그인·분배 검사는 F3 세션 연결 때, TCP/IP 소켓과 송수신은 3차 로컬 구현 때 `LocalNetworkPolicy`의 루프백 전용으로 더한다.
- 0.153: `AddRandom_Native288`의 대본 GameMenu 후반부(`QueueScriptCommand`·`GameMenuScriptCode`)는 R6에서 옮긴다.
- 0.154: 판본 분기 잔여(원문 근거는 있으나 fixture에서 관측되지 않음) — `RawSquidFactory.Take`·`PostCreate`의 나머지 CD 분기, `RawPop`의 무효 void CD throw·겹침 CD 항상 쓰기, `RawUnpop`의 추가 비트 CD 위치, `RawPostPop`·`RawUnpop`의 표시 지원 판본 overload 연결. 탐색기 발자국 조건은 원문(`footX<0`) 대신 기존 C# 조건(`FootX<1`)을 유지했다(기존 직접 검사 `RawFinderTests.FootprintValidationRejectsBadFeet`와 충돌, 차이로 기록). `RawPathSearch` 전체·`RawCanonPlacementPipeline`·`MakeFortRandomPlacementHooks`는 R6이다.
- 0.159: 배치 지형의 파이프라인 어댑터·통합 검사는 R6 범위로 뺐다.
- 0.176: `RawSurfaceStorage.UpdateAmbientWind`의 F3 연결이 남았다.
- 0.182: `Kernel` 진단 5종은 Client·`SquidProcess`가 필요해 옮기지 않았다(이후 해소 기록 없음).
- 0.221~0.224: 공장 HP의 소비(`TypeHitPoints`)는 R6, Create/Take 부모 진단 전달은 월드·세션 조립 때 잇는다.
- 0.242: 월드 보고서의 효과·삭제 요청 계수기 증가 연결(수명주기 쪽)은 미연결이다.

<a id="dotnet-unit-records"></a>

### 0.10 단위 기록 (0.288부터)

문서가 없는 작은 단위만 아래에 최신순으로 짧게 적는다. 한 기록에는 범위, 구현(새 파일·수정 파일), C#에서 달라진 모양, 검증(집중·전체·변이), PORT_MAP·fixture 변화, 보호·실행·외부 접속 관점, 남은 것을 적는다(0.6). 0.11~0.279는 이관 파일에 있다: [0.11~0.65](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073), [0.66~0.126](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126), [0.127~0.153](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154), [0.154~0.279](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279).

**0.287 — LAN TCP/UDP 루프백 사전 구현 (2026-11-10, HJOW-Athlon, 검사 없음).** 범위: 목표 지시의 멀티플레이 사전 구현. 근거: cpppj `LocalTcpTransport.h/.cpp`·`LocalUdpTransport.h/.cpp`(base `a76499e3`)만 읽고 미커밋 변경은 쓰지 않았다. 구현: 새 `Netstorm.Net/LocalTcpTransport.cs`(리스너·수락 pending -1~-31·트랜시버 0~21·Connect·AssignConnection·Send·Flush·Poll·Status·20바이트 주소, 4바이트 헤더+선언 WORD·집합 type4·폐기 0x7f/0x80·부분 송신 꼬리·would-block 순서 보존), 새 `Netstorm.Net/LocalUdpTransport.cs`(12번 단일 노드·Open·Broadcast·Send·Poll·LocalAddress). C#에서 달라진 모양: Winsock 대신 .NET 소켓, 연결 등록표·분배·observer·hostname LAN 조회는 세션 연결 때 잇는다(분배 경계는 `LocalMissionTransport.FrameDispatch`로 받는다). cpppj의 RFC1918·link-local 허용과 달리 dotnetpj는 `LocalNetworkPolicy`의 루프백 전용을 적용하고, UDP 브로드캐스트는 루프백 목적지로만 보낸다. 검증: 지시대로 검사를 건너뛰고 Release 솔루션 빌드 경고·오류 0만 확인했다. 실제 루프백 송수신 검사는 3차 로컬 구현 때 한다. PORT_MAP 4행 추가(port, L1 아님). fixture 변화 없음. 보호: 원본/CD·AGENTS.md 미수정, 게임 실행 없음. 외부 접속 변화 없음(루프백 강제 유지). 남은 것: F3 본체 계속(0.288)부터 계속한다.

**0.286 — F3 소유자 본체 (2026-11-10, HJOW-Athlon).** 범위: 0.9 R4 'F3 본체' 중 ownership 3종. 근거 확인: cpppj `LocalOwner`→`gameplayGlobals->player.battleOwner`, `TemplateOwner`→`templateOwner`, `BattleActive`→`assetState.inBattle`(원본 00594fbc, 표시 상태 없음). `PlayerWorldState` 기본값(battleOwner·templateOwner 1)이 원본과 같음을 확인했다. 구현: `RawCampaignSession.cs`에 `_player`·`_inBattle` 소유 상태와 설치 본체용 내부 `PlayerState`·`InBattle`을 두고 3개 Stub 본체를 읽기로 교체(적재 관문 유지). 나머지 5종은 필요 상태(사제·장식·지형·간헐천·비행선 TYPE, outposts/temples 목록, Deck, 파서·배치 파이프라인)가 세션에 없어 Stub으로 둔다(결정 6). 검증: Release 빌드 경고·오류 0, `RawCampaignSessionTests` 34건(기존 거부 1건을 값 읽기 1건으로 교체: 기본값 1/1/0과 갱신 반영) 통과, Client 전체 906건 실패 0(`--no-incremental` 재빌드 뒤). 대표 변이(LocalOwner가 templateOwner 반환)가 새 검사에서 검출됨을 확인하고 원복했다. PORT_MAP·fixture 변화 없음(이미 있는 `RawCampaignSession` 행의 base는 두지 않았다). 보호: 원본/CD·AGENTS.md 미수정, 게임 실행 없음. 외부 접속 변화 없음. 남은 것: F3 본체 계속(메뉴·Fort 적재) 또는 멀티플레이어 사전 구현(0.287)부터 계속한다.

**0.285 — F2c 정상 생성·도장 본체 (2026-11-10, HJOW-Athlon).** 범위: 0.9 R4 'F2c 나머지'. 구현: `RawNormalTerrainBinding.cs`의 `RawNormalTerrainWorldBinding`에 `Generate`·`StampContext`·`GenerateOriginal` 본체(cpppj `RawNormalTerrainWorldBinding.cpp` 직역, `port`), `BindGenerationInputs`에서 군집 보관(`_farm`), `using Netstorm.Core.Simulation` 추가. C#에서 달라진 모양: 월드 장치(NetstormRandom·CRT 상자·도장 효과·군집 배치 효과)는 인자로 받고, `RawFortWorldLoadBinding`의 대리자 주입 구조와 같은 방식이다. 32비트 감김·signed 나눗셈은 unchecked, `area == 0`은 DivideByZeroException, 게임 시각·세션 연결은 F3로 미룬다. 검증: Release 빌드 경고·오류 0, `RawNormalTerrainBindingTests` 28건(새 5건: 거부 2건·일반 성장 1건·원본 일반 1건·원본 군도 1건, 같은 입력 두 장면 동일 SID) 통과, Core 전체 1,831건 실패 0(`--no-incremental` 재빌드 뒤). 대표 변이(군도 조건 `==0`→`!=0`)가 원본 군도 검사에서 검출됨을 확인하고 원복했다. 이관 과정에서 빈 `Create` 스텁(SID 0)과 청크 `Side` 0·범위 밖 bounds가 실패를 냈으며 풀 할당 스텁·유효 방향·포함 bounds로 고쳤다. PORT_MAP·fixture 변화 없음(이미 있는 `RawNormalTerrainWorldBinding` 행의 base는 두지 않았다). 보호: 원본/CD·AGENTS.md 미수정, 게임 실행 없음. 외부 접속 변화 없음. 남은 것: F3 본체(0.286)부터 계속한다.

**0.284 — `session-dump` 단계·world 0 덤프 (2026-11-10, HJOW-Athlon).** 범위: 0.9 '재개 준비' L2·L3 장치 나머지. 구현: 새 `Netstorm.Client/Automation/SessionStateDump.cs`(cpppj `SessionStateDump.h/.cpp` 중 world 0 서식·FNV-1a 64 직역), 새 `Netstorm.Client/Host/HostSessionDump.cs`(호스트 소유 Kernel·GameRandom·CrtRandom·프레임 읽기, 비Squid 행은 pid/0/0, 게임 시각 0), `UiScriptRunner`에 `session-dump` 관측 단계·`BindSessionDump`(빈 경로 거부·미연결 명시 예외), `HostFrameDriver.BindSessionDump`, `ClientHostGame` 소유·연결. C#에서 달라진 모양: world 1 행(pool·slot·object·player·sp·graph·totals)은 세션 조립 때 늘린다. 검증: Release 빌드 경고·오류 0, 새 `SessionStateDumpTests` 5건(판번호 1 정확 문자열 2건·판본 거부·FNV 공개값·파일 바이트 일치)과 `CaptureAndScriptTests` 1건(단계·거부 3종) 통과, Client 전체 906건 실패 0(`--no-incremental` 재빌드 뒤). 대표 변이(world 행 0→1)가 3건에서 검출됨을 확인하고 원복했다. PORT_MAP 2행 추가(`SessionStateDump.h/.cpp`→`SessionStateDump.cs`, base `c91d9b84`, port). fixture 변화 없음. 보호: 원본/CD·AGENTS.md 미수정, 게임 실행 없음. 외부 접속 변화 없음. 남은 것: R4 F2c 나머지(0.285)부터 계속한다.

**0.283 — 고정 시각 실행 장치 (2026-11-10, HJOW-Athlon).** 범위: 0.9 '재개 준비' L2·L3 장치 중 고정 시각 절반. 구현: 새 `Netstorm.Client/Host/HostTimeSource.cs`(cpppj `ClientTimeSource.h/.cpp` 직역, 전역 대신 호스트 소유 인스턴스), `HostOptions`에 `--fixed-frame-ms`·`--fixed-unix-seconds`(단독 거부), `HostFrameDriver`에 선택 시계·`BeginFrame` 전진, `ClientHostGame`의 메뉴 시각 4곳을 시계로 교체, `Program`에서 시계 원점보다 먼저 켜고 driver·game에 전달. C#에서 달라진 모양: 기본 모드는 기존 Stopwatch/TickCount 상당이라 동작이 같고, 고정 모드에서만 프레임당 N ms 전진·maxFPS 대기는 호스트 몫으로 남는다(PumpMessages 상당의 별도 대기 없음). 검증: Release 빌드 경고·오류 0, `ClientHostTests` 19건(새 4건: 인자·기본 OS 시계·고정 전진/감김/거부·드라이버 전진) 통과, Client 전체 900건 실패 0(`--no-incremental` 재빌드 뒤). 대표 변이(전진 조건을 `_elapsed != 0`으로 좁힘)가 2건에서 검출됨을 확인하고 원복했다. PORT_MAP 2행 추가(`ClientTimeSource.h/.cpp`→`HostTimeSource.cs`, base `c91d9b84`, port). fixture 변화 없음. 보호: 원본/CD·AGENTS.md 미수정, 게임 실행 없음. 외부 접속 변화 없음. 남은 것: `session-dump` 단계·world 0 덤프(0.284)부터 계속한다.

**0.282 — cpppj 재반영 2건 (2026-11-10, HJOW-Athlon).** 범위: 0.9 '재개 준비' 세 번째 항목. 구현: `HostMainMenu.cs` Demo Tell `DEMOVILLE`→`Demoville`(원문 10.78·10.72와 같음, cpppj `UberGump.cpp` 수정 대응), `HostHtmlText.cs` ScrollingText 슬라이더 색을 `BackgroundType/1`에서 팔레트 갈색(named 9)·검정(basic 0)으로(cpppj `SetPalette` 대응, `SliderArrowType`·`SliderArrowFrame`·내부 `WidgetState` 추가). C#에서 달라진 모양: 호스트 접합부만 바뀌고 Gump 본체· `tell.english` 자료는 그대로다. 검증: Release 빌드 경고·오류 0, `HostStartupSceneTests` 24건 통과(기존 2건 수정·새 1건), Client 전체 896건 실패 0(`--no-incremental` 재빌드 뒤). 대표 변이(슬라이더 색을 옛값으로 되돌림)가 새 검사에서 검출됨을 확인하고 원복했다. PORT_MAP 행 없음(호스트 접합부라 대상 아님)·fixture 변화 없음. 보호: 원본/CD·AGENTS.md 미수정, 게임 실행 없음. 외부 접속 변화 없음. 남은 것: L2·L3 장치(0.283)부터 계속한다.

**0.281 — cpppj 영향 확인 (2026-11-10, HJOW-Athlon, 문서만).** 범위: 0.9 '재개 준비' 두 번째 항목. 구현: 코드 변경 없음, 0.1·0.9만 갱신. 방법: `git diff --name-only f95a8db2..HEAD -- cpppj/`(HEAD 기준, 미커밋 1249 변경 제외)와 PORT_MAP 629행 교차. 결과: 영향 458행(L1 247·port 211, 고유 387파일), 영역 `o/` 282·`client/` 170·`app/` 4·`zacket/` 2, R4 우선 65행 목록 확정. 검증: 빌드·검사 실행 없음(0.280 기준선 유지). PORT_MAP·fixture 변화 없음(`reflected-commit` 유지). 보호: 원본/CD·AGENTS.md 미수정, 게임 실행 없음. 외부 접속 변화 없음. 남은 것: 재반영 2건(0.282)부터 계속한다.

**0.280 — 전체 회귀 재기준선 (2026-11-10, HJOW-Athlon, 문서만).** 범위: 0.9 '재개 준비' 첫 항목. 구현: 코드 변경 없음, `LEFT_JOBS.dotnetpj.md`의 0.1·0.9만 갱신. 검증: `dotnet test dotnetpj/Netstorm.sln -c Release` 완주 — Assets 366개·Client 895개·Core 1,826개(합계 3,087개) 실패 0, 기간 약 18분. PORT_MAP·fixture 변화 없음(연결 232개 개별 OK, `cpp_sync.py`는 미반영분으로 0종료 아님, `reflected-commit` 유지). 보호: 원본/CD·AGENTS.md 미수정, 게임 실행 없음(빌드·정적 검사만 해당되는 실행 확인 면제 범위). 외부 접속 변화 없음. 남은 것: cpppj 영향 확인(0.281)부터 계속한다.

---

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
| Fort 타입 표·적재·저장 F1~F2c-11 (11-02~04, 0.137~0.149·0.153·0.154) | F1 `RiftTypeTable`·`.type` 파서·`FortTemplate.Parse`(두 판본, x86 기준점·비용·techBit·생성자 주소 대조, 실제 `.fort` TypeNames 해시 패치 29·CD 424개 일치), F2a Money·Meta, F2b TypeNames·Deck·Contents·Subscriber, F2c-1~3 Territory·팔레트·객체 적재 본체·객체 스트림, F2c-4~8 저장 모듈 14종·`ChunkMap`·FinishOwner, F2c-9 변환 저장소, F2c-10 AddRandom 부모·입력, F2c-11 섬 목록·단일 배치·군집·게임 보조·Notify. 단위마다 대표 변이 검사를 실행했다 | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) 0.137~0.153, [2026-11-10 이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279) 0.154 | 대부분 L1(PORT_MAP 참조). 세션 연결은 F3 |
| 11.01 표기·Credits (11-03, 0.150) | 새 경로 `ProductVersion`·`ProductPresentation`·Tell 본문 3항·`Patch1101` 등록, 기존 경로 `MainMenuView` 버전 창 11.01·Credits 3항 목록과 같은 상세 창. 원본 판본 표기와 옛 Credits는 유지 | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) 0.150, [cpppj 685 문서](docs/exe/cpp-product-1101-20261101.md) | 새 경로·기존 경로(`port`) |
| 외부 접속 문서화·클라이언트 유효성 검사 비활성화 (11-03, 0.151) | C# 소켓·HTTP 코드 없음 확인, 외부 실행은 기존 경로 Help의 사용자 선택 링크 1곳, 구 검증은 연결·성공 합성 없음 | [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md) | 문서. 새 경로 이식 때마다 갱신(0.7) |
| 멀티플레이 사전 구현 (11-03, 0.152) | `Netstorm.Net`의 `LocalNetworkPolicy`(루프백·리슨만 허용)·`LocalMissionTransport` 바이트 큐 골격. 사용자 지시로 검사 없음 | [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) 0.152 | 사전 구현(`port`), 검증 완료 아님 |
| F2c-12·지형·섬 생성 (11-07, 0.155~0.160) | Canon 디코더·섬 빌더/배치/검색·Fort 월드 적재 부모(0.155), 지형 생성·도장·성장 기반 7종과 섬 인구·추가·전투·중앙·군도 분리 5종(0.156), Fort 사제 준비(0.157), 섬 수명주기(0.158), 배치 지형 2종(0.159), 섬 후처리(0.160). 대표 변이 검사 각 1종 이상 검출 | [2026-11-10 이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279) 0.155~0.160 | 대부분 L1, 섬 수명주기·후처리는 port(전수 재생은 0.9 남은 항목). 세션 연결은 F3 |
| 다리 연결·F2d Player·캠페인 월드 (11-07, 0.161~0.171) | `RawBridgeConnect`와 판본별 3,558행 전수 재생, Player Core 장부·CoreData·초기화·Chat·참여·CoreData 갱신·Player 월드, 캠페인 월드 전역·적재 본체, 서피스 삭제 분배기, 저장 나머지(538행 전수) | 같은 이관 파일 0.161~0.171 | L1·port(PORT_MAP 참조). 세션 연결은 F3 |
| 서피스 월드 기반·프로세스·호스트 (11-07, 0.172~0.225) | 서피스 월드 자료형·권한·바람, 일반지형 요청형, 게임 난수·보드 각도·바람 진행, 저장소 껍데기·해시 선택/주소·물리 자료형·풀 생성, 게임 시계, 커널·프로세스 상태·기본/정기 프로세스, 프로세스 호스트 전체(저장소·묶음·껍데기·부착·통지/검색·삭제·정기 실행/예약·대여), 표시 진단, 지형 바인딩 계약·패턴·첫 기록·군집 후처리·정상 지형 관문·생성 입력, 서피스 월드 껍데기, Fort 적재·Forms·Pop 바인딩, 공장 부착·폼 생성·분리·진단·HP·Take·약화·현재 생성. Host 전송(0.220)·로컬 미션 패킷(0.208)은 사전 구현 | 같은 이관 파일 0.172~0.225 | 새 경로(L1 일부). 실제 세션 연결은 F3 |
| 월드 본체 조립 (11-07~10, 0.226~0.264) | `RawSurfaceWorld` 본체: 변경 플래그·수정 가능 TYPE·로딩 깊이·설치 모드·객체 조회·프레임 시각·적재 구간·삭제 상태/연결/분배·커널 차용·그래프 적재·소유자/받침 지정·종속 해제·프레임 소유·보고서·생성/해제/배치 연결·표면 순회·붕괴 몸체/스캔·표시 부모/수집/연결·연결 조회·초기 적재·`RunFrame`·가상 삭제 현행 모드·호스트 대여·상태 입출력 경계·종료 반납·Core 보기·수명 분배기·사건 처리기·감시 쌍. 월드 전송(0.230)은 사전 구현 | 같은 이관 파일 0.226~0.264 | 새 경로(`port` 위주). 세션 연결은 F3 |
| F3 세션 껍데기·Stub (11-10, 0.265~0.279) | `RawCampaignSession` 생성·선택·월드/Core 위임, Fort 씨앗 적재, 프레임·보고서 위임, 섬 개수 2종, 연결 사전 구현, Stub 관문 8종(본체 없음, 결정 6) | 같은 이관 파일 0.265~0.279 | 새 경로(`port`). 본체는 0.9의 F3 남은 항목 |

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
| C | 자체 `settings.json` 을 쓰고 `options.cfg` 는 읽기만 한다 (원본은 `<게임 폴더>/d/options.cfg` 를 시작·변경 시 저장하고 `fullscreenStateFile.dat` 를 만들고 지운다) | 옵션이 저장되는 시점과 위치, 원본 게임과 설정을 주고받는지 | [cpp-options-reconstruction.md](docs/exe/cpp-options-reconstruction.md), 이미 C# 테스트에 연결한 options x86 기대값 16개 | `options.cfg` 를 원본의 시점·형식으로 읽고 쓴다. **단 AGENTS.md 의 "전체화면 전환 뒤 재실행 오류는 클론에서 발생하지 않아야 한다"는 그대로 지킨다.** 저장 위치는 R8의 5차에서 사용자 홈으로 옮긴다 |
| D | 그리기 직전 제한의 대기는 Sleep/Yield로 CPU를 양보한다(2026-10-12; 원본은 바쁜 대기) | 실제 간격과 흔들림·초과 대기가 달라질 수 있다. 모든 환경에서 14ms라고 단정하지 않는다 | main-loop.md 4·7절. 원본 전체 루프의 실측 대조는 후속이며 현재 D 미판정 | 바쁜 대기로 바꾼다(CPU 한 코어를 계속 쓴다) |
| E | 매 프레임 전체를 다시 그린다 (원본은 변경 사각형 100개 표로 부분만 다시 그린다) | 화면 결과가 같아야 한다 | [cpp-renderer-reconstruction.md](docs/exe/cpp-renderer-reconstruction.md), 화면 캡처 비교 | 변경 영역 표를 옮긴다 |
| F | 뮤텍스·레지스트리·CD 찾기·창 크기 고정이 없다 | 창 크기 조절·중복 실행처럼 사용자가 보는 동작 | [cpp-screen-reconstruction.md](docs/exe/cpp-screen-reconstruction.md) | 사용자가 보는 동작만 원본대로 맞춘다 |

**이 결정으로 바꾸지 않는 것:** MonoGame 을 쓰는 것과 raw 슬롯·vtable 을 옮기지 않는 것은 플랫폼(Linux 목표)과 내부 표현의 문제다(2026-10-16: 원본 월드 코어 저장소에 한해 슬롯 바이트를 옮기도록 바꿨다, 3절 11번). 화면·소리·규칙의 결과가 원본과 같아야 한다는 기준만 적용한다. CD 10.72·10.82 값을 쓰지 않는 것은 기준 판본(10.78)을 따르는 것이라 원본과의 차이가 아니다.

---

## 3. 사용자 결정·확인 기록 (옛 10절)

> **2026-10-28:** 번호는 옛 10절 번호를 그대로 쓴다. 해결·구현이 끝난 1·3·4번은 이관 파일로 옮겼다. 이번 재수립에서 새로 생긴 확인 사항은 [0.8](#08-사용자-확인이-필요한-것-확인-전까지-에이전트-기본값으로-진행)에 있다.

2. **영어 UI 글꼴(5-3) — 해결(2026-10-10 사용자 결정): 언어가 영어일 때의 글꼴은 기존 게임의 글꼴을 따른다.** 판단 기준은 **선택한 언어**다. 언어가 영어이면 UI 글자를 원본 비트맵 글꼴(`.chfnt`)로 그리고 폭도 그 글꼴로 잰다. 언어가 한국어이면 지금처럼 D2Coding 을 쓴다. 영어 화면에 원본 글꼴에 없는 글자(한글 등)가 섞인 문자열은 D2Coding 으로 대신 그린다(에이전트 해석 — 개발용 표시와 아직 번역 분기가 없는 문구가 깨지지 않게 하려는 것이다). 옛 구현 상태(5-3)는 이관 파일에 있다. **2026-10-28:** 새 경로에서는 R2·R3의 `BitmapFont`·Gump 글리프로 영어를 그린다. 한국어 D2Coding은 R8(0.8 2번)에서 8비트 화면용으로 다시 만든다. **2026-11-10:** 글꼴 파일은 exe에 내장하지 않고 exe 옆 `fonts/`에서 읽는다(AGENTS.md `75e4eed9`, R8 표 아래 항목).
5. **"원본과 완전히 동일한 동작"과 9절의 의도적 차이 — 해결(2026-10-10 사용자 결정): 원본처럼 동작하도록 바꾼다. 지금 구조를 둔 채 원본 게임의 경험을 거의 동일하게 만들 수 있으면 구조는 유지해도 되지만, 그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다.** 항목별 판정과 작업은 [9-1](#original-parity-plan)에 있다. 같은 날 먼저 적었던 에이전트 기본값("구조를 두고 cpppj 와 대조")은 폐기했다.
6. **이미 들어간 요구사항 기능의 처리(2026-10-10 추가, 사용자 확인 전).** 한국어 선택·와이드 화면 시야 확장·테두리 없는 전체화면 창은 이미 구현돼 있다(자체 `settings.json` 은 [9-1](#original-parity-plan) C 의 판정 대상으로 옮겼다). 기본값(에이전트 해석): 되돌리지 않고 유지한다. 기본 실행(영어·4:3)에서 원본과 같은 동작을 먼저 맞추고, 새 요구사항 기능은 ① 뒤로 미룬다. **2026-10-28:** 0.7과 0.8 2번에 따라, 새 경로로 옮길 때 cpppj 방식(논리 해상도·호환 전체화면)으로 다시 만들고 한국어는 R8에서 다시 만든다.
7. **D2Coding 1.4.0 교체 — 해결(2026-10-10 사용자 결정): 중요도가 낮다.** 당분간 1.3.2 를 그대로 쓴다. 다른 작업을 미루면서 하지 않으며, 글꼴 코드를 고칠 일이 있거나 여유가 있을 때 한다. 교체할 때 확인할 것: 코드 3곳의 파일명(`Netstorm.Game.csproj`·`NetstormGame.cs`·`TrueTypeCollectionTests.cs`), 배포 스크립트의 글꼴 경로와 고지문, TTC face 순서·글자 폭·줄 높이, `TrueTypeCollectionTests` 기대값, 한국어 화면 캡처, README 두 개의 파일명. (2026-11-10: AGENTS.md `75e4eed9`에서 1.4.0 추가 안내 문장이 빠졌으나 이 결정은 그대로다.)
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
# (click·click-if·right·key·esc·pointer·snapshot·session-dump 등. snapshot·session-dump 인자는 저장할 파일 경로).
cpppj/build/bin/Release/Netstorm.exe --run <데이터 사본> --window --no-audio --frames 120 `
  --set "SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0" `
  --ui-script steps.tsv --ui-report states.tsv            # 미션 직행은 --mission thewarbegins 등을 더한다
# 캡처 비교 도구: python dotnetpj/tools/cpp_frame_compare.py A.bmp B.bmp [--diff diff.png]  /  --dir 폴더A 폴더B (종료 코드 0 일치·1 차이·2 입력 오류)
# dotnetpj 쪽의 같은 실행 인자(--frames/--ui-script/--ui-report/--set)는 Netstorm.Client.Host 가 받는다.
# PowerShell 5.1의 Start-Process -PassThru 는 종료 코드를 비우므로 `& exe 인자; $LASTEXITCODE` 로 읽는다.

# cpppj 실제 Session 상태 덤프(L2)·고정 시각 실행(L3) (2026-11-09 cpppj 추가, 이식 계획 8절 요청 1·2의 해결).
# 결정론 비교에는 가상 시계(--fixed-frame-ms)와 빈 사용자 루트를 쓴다. 기본 OS 시계 두 실행은 게임 시각부터 달라 비교할 수 없다.
# WelcomeGiven=1 이 없으면 환영 창 때문에 메뉴 클릭이 실패하고, --mission 뒤 'Play Mission' 은 늦게 나타나므로 click-if 를 여러 프레임에 둔다.
$env:NETSTORM_USER_DATA = "<빈 폴더>"
cpppj/build/bin/Release/Netstorm.exe --run <데이터 사본> --mission thewarbegins --fixed-frame-ms 25 --window --mute-audio --frames 250 `
  --set "SCREENW=1024;SCREENH=768;WelcomeGiven=1;autoDemo=0;tellTips=0" --ui-script steps.tsv --ui-report report.tsv
# steps.tsv 예: "5<TAB>click-if<TAB>Play Mission", "90<TAB>session-dump<TAB><출력 경로>.tsv"
# 덤프 형식(판번호 1, ASCII·LF·탭, float/double 은 비트 정수): docs/exe/cpp-session-state-dump-20261109.md
```

- 클론 창을 띄우는 검사(`tools/clone_*_smoke.ps1`, `dotnet run --project dotnetpj/src/Netstorm.Game`)는 AGENTS.md의 확인·예외 규칙을 따른다.
- 원본 x86 기대값 대조만 돌릴 때: `dotnet test -c Release --filter "FullyQualifiedName~X86"`. 새 기대값 파일은 `tests/Netstorm.Core.Tests/Netstorm.Core.Tests.csproj`에 링크로 추가하고(사본 금지) 위 `cpp_sync.py --update --add`로 SHA-256 목록에 등록한다(빠뜨리면 `EveryLinkedFixtureIsRegistered`가 실패).
- 새 코드의 **상수·함수·구조체·클래스·멤버함수·멤버변수·반복문**에 한국어 주석을 단다(AGENTS.md 2026-10-13 확장). **주 내용은 설명과 사용법**이고, 언제·왜 추가/수정했는지는 그 뒤에 적는다. C# 에서는 `/// <summary>` 에 무엇을 하는지와 쓰는 방법(인자·반환·호출 조건)을 쓰고, 원본 주소·근거 문서와 추가/수정 이력은 summary 의 뒤쪽이나 `<remarks>` 에 둔다. 반복문은 바로 위 `//` 주석으로 무엇을 도는지 적는다. 기존 주석은 그 주석이 설명하는 대상을 수정할 때 이 규칙에 맞춘다(2026-10-10 사용자 결정). 새 파일은 UTF-8 로 쓴다.
- 원본 주소를 주석에 적을 때는 10.78 주소를 쓰고, 근거 문서(`docs/exe/cpp-*.md`)를 함께 적는다.
- 작업이 끝난 항목은 이 문서에서 체크하거나 지운다. 중단하면 변경 파일·검사 결과·다음 항목을 0.9의 체크리스트와 0.10 단위 기록에 남긴다. 끝난 단위의 상세 기록이 길어지면 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)로 옮기고 1절 색인만 갱신한다(2026-10-28).
- **전체 검사 완주:** 단위마다 집중 검사로 넘어가더라도, 여러 단위가 쌓이면(목표: 20단위 또는 한 작업일마다) 솔루션 전체 검사를 끝까지 돌려 0.1의 기준선을 새로 적는다(2026-11-10 정리 때 0.212~0.279가 전체 미완주였던 교훈).
- AGENTS.md 와 원본 디렉터리(`originals/`, `originalCD/` 등)는 수정하지 않는다.
- **Git:** `dotnetpj`·`docs`와 `LEFT_JOBS.dotnetpj*.md`는 저장소에 LF로 저장된다. 기본 `autocrlf`로 add하고 `-c core.autocrlf=false`를 쓰지 않는다. 커밋 번호는 다른 PC·세션과 겹칠 수 있으므로 `git log`로 당일 마지막 번호를 확인해 이어 쓴다. 같은 작업 트리에서 다른 세션이 cpppj·`LEFT_JOBS.md`를 고치는 일이 잦으므로 add는 이 단위의 파일만 경로로 지정한다.
- **빌드 환경:** Git Bash에서 `cmake --build … -- /p:…`를 실행하면 `/p:`가 경로로 바뀌어 MSBuild가 실패하므로 PowerShell로 실행한다. `cpp_sync.py` 보고가 cp949 콘솔에서 깨지면 `PYTHONUTF8=1`을 설정하거나 `python -X utf8`로 실행한다. 오라클에는 게임 실행 파일만 필요하며, cpppj 전체 빌드는 검사 대상까지 포함해 오래 걸린다.
- **오라클 자료:** 데이터 사본 `extracted/dotnet-oracle-data/`, 캡처 `extracted/dotnet-oracle-captures/`, 기준선 `extracted/dotnet-r0-baseline/`은 Git 제외이며 작업 PC에만 있다. 실행 전후 `originals/` 해시를 비교한다.
- **fixture·포트 맵:** 한 fixture는 한 소비자 프로젝트에서 검사한다(`config-x86.tsv`는 Assets.Tests). 포트 맵의 `port` 행은 fixture 대조가 없는 이식이며 L1이 아니다.
- **변이 검사 운전:** 최근 단위(0.141 이후)는 별도 사본 자동 순환 대신 대표 변이를 수동으로 넣고 제품·검사 빌드 뒤 `--no-build` 집중 실행으로 검출을 확인한 다음 원복하고 집중·전체 검사를 다시 통과시켰다. 원복 뒤 증분 빌드가 옛 결과를 쓰지 않게 필요하면 `--no-incremental`로 다시 빌드한다(0.211). 사본(`extracted/dn-mutation-ts0`)을 쓸 때는 드라이버의 다중 행 패턴 들여쓰기가 틀리면 일치 0개로 조용히 건너뛰므로 개수 assert로 잡고, 사본에 복사한 파일은 반드시 `touch`한다(시각이 오래되면 증분 빌드가 옛 변이 바이너리를 그대로 돌린다). 무한 루프로 걸리는 변이도 검출로 기록한다(0.138·0.141·0.148).
- **options.cfg 변경 판별:** 호스트나 원본 게임을 실행한 뒤 추적 파일 `assets/game-data/d/options.cfg`가 바뀌어 있으면 누구의 실행 때문인지 먼저 가린다(같은 시기 다른 세션이 원본 게임을 실행했을 수 있다). 내가 만든 변경일 때만 되돌린다(0.136의 교훈).

---

## 5. 참고 문서 (옛 13절)

| 주제 | 문서 | cpppj 기준 구현 |
|---|---|---|
| **현행 계획·재수정·다음 작업(2026-10-28)** | [이 문서 0절](#dotnet-current-plan-20261028) | cpppj `src/o`·`src/client` 전체, [cpppj 소스 맵](cpppj/SOURCE_MAP.md) |
| **cpppj 실제 Session 덤프·고정 시각 실행(L2·L3 오라클, 2026-11-09)** | [cpp-session-state-dump-20261109.md](docs/exe/cpp-session-state-dump-20261109.md) | `client/SessionStateDump`, `client/ClientTimeSource`, `app/main.cpp` |
| **cpppj의 다른 프로젝트 요청 처리·dotnetpj 재반영 대상(2026-11-09)** | [cpp-webpj-requests-20261109.md](docs/exe/cpp-webpj-requests-20261109.md) | `client/UberGump.cpp`, `client/RawMenuControls` |
| **AGENTS.md 4~9차의 cpppj 구현(R8 참고)** | [LEFT_JOBS.md](LEFT_JOBS.md) 1절, [사용자 루트(5차)](docs/exe/cpp-user-data-root-20261105.md), [로컬 MCP(4차)](docs/exe/cpp-mcp-server-20261106.md), [글꼴 경계(6차)](docs/exe/cpp-font-face-encoding-20261103.md), [알려진 버그 NB01~NB34(9차)](docs/exe/cpp-known-bugs-plan-20261109.md) | `platform/UserDataRoot` 등 |
| 8비트 화면 출력·전체화면 호환 | [cpp-modern-fullscreen-20261027.md](docs/exe/cpp-modern-fullscreen-20261027.md), [cpp-screen-resolution-menu-20261027.md](docs/exe/cpp-screen-resolution-menu-20261027.md) | `client/Screen.cpp`, `client/ScreenPresentation.cpp`, `client/ClientMain.cpp` |
| 이관 기록(2026-10-22 이전 계획·날짜별 완료 절·옛 0~11·14절·단위 기록 0.10~0.279와 정리 직전 원문 4개) | [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md) | — |
| **cpppj → dotnetpj 이식 계획(W01~W20 상세, 2026-10-16)** | [dotnet-cpp-port-plan-20261016.md](docs/dotnet-cpp-port-plan-20261016.md) | `client/RawCampaignSession`, `client/RawSurfaceWorld`의 `RawWorldCore` |
| **원본 월드 코어 W02 첫 조각(슬롯 풀·생성자·Create/Take, 2026-10-16)** | [dotnet-original-core-20261016.md](docs/dotnet-original-core-20261016.md) | `o/SidPool.cpp`, `o/SquidFactory.cpp`, `o/TypeConstructors.inc`, `o/DerivedConstructors.inc` |
| **Template 저장소 이식 계획(TS0~TS6, F1~F3, 2026-11-01)** | [dotnet-template-storage-plan-20261101.md](docs/dotnet-template-storage-plan-20261101.md) | `client/CampaignTemplateStorage` |
| **cpppj의 11.01 표기·Credits / 외부 접속 목록(2026-11-01)** | [cpp-product-1101-20261101.md](docs/exe/cpp-product-1101-20261101.md), [cpp-external-services-20261101.md](docs/exe/cpp-external-services-20261101.md) | 단위685·684 |
| dotnetpj 외부 접속 문서(2026-11-03) | [dotnet-external-endpoints-20261103.md](docs/dotnet-external-endpoints-20261103.md) | — |
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

**2026-11-10 이관(HJOW-Athlon, 문서만, 사용자 요청):** 정리 직전(이 파일의 마지막 커밋 `b00afaa6 / 20261110-82 DN`) 전체 원문을 [이관 파일 끝](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279)에 그대로 보관했다(원문 SHA-256 `11a76d0e04794ce348f63a30a0e673234974af02d39e23571b701a8ba4244ce4`, 보관본과 바이트 일치 확인). 이관 파일 머리에 정리 직전 원문 보관본 4개의 목록 한 줄을 더했다.

- **본문에서 뺀 것:** 단위 기록 0.154~0.279 전체(남은 작업이 모두 0.9로 모였고 같은 형식의 Stub 기록이 반복돼 본문에 남길 직접 근거가 없다), 0.1 '현재 단계' 칸의 모듈 나열과 '검사' 칸의 단위별 서술, 0.9 R4의 단위별 `[x]` 하위 항목 약 130줄(묶음 10줄로 줄임).
- **새로 넣은 것:** 맨 앞의 [한눈에 보기](#dotnet-overview)(두 기준 평가·단계표·숫자·남은 일 순서·확인 대기), 0.9의 '재개 준비' 4항목(전체 회귀 재기준선 — 0.212 이후 전체 미완주 확인, cpppj 영향 확인 — 단위861→제품1247, cpppj 재반영 2건 `Demoville`·스크롤 화살표 색, `session-dump`·고정 시각 장치), F3 Stub 8종의 본체 목록, 미뤄 둔 전수 재생 목록, 0.10 단위 기록 자리(다음 0.280), 1절 색인의 0.155~0.279 묶음 5행, 4절의 `session-dump`·`--fixed-frame-ms` 명령과 전체 검사 완주 주기 규칙, 5절의 새 참고 문서 4행.
- **AGENTS.md 반영(그동안 dotnetpj 문서에 없던 것):** `98b2f20c`(2026-11-02) 8차 시나리오 편집과 dotnetpj 포함 범위 4~7차→4~8차 — R8 표·0.7·0.8 6번. `aaafbbc4`(2026-11-02) 5차 확장(관리자 권한 없는 설치 위치, 자동 생성 파일 전부를 `Saved Games\Netstorm\` 아래로) — R8 표. `b5197d48`(2026-11-08) 9차 알려진 버그 34건 — 0.8 7번(새 확인 항목, 기본값: cpppj 수정을 이식 때 함께 옮김). `75e4eed9`(2026-11-10) 글꼴 exe 내장 금지·exe 옆 `fonts/`·일본어 등 다른 글꼴 — R8 표 아래·0.7·3절 2·7번(기존 경로는 이미 준수 확인: `CopyToOutputDirectory`, 배포 스크립트 별도 파일). `551d27ae`(2026-11-09) 세 프로젝트 평가 기준 — 한눈에 보기·0.7. `3051892a`·`851054a8`(2026-11-06) README 개요 보호·기존 파일 재활용 — 0.7. `61067075`(cpppj 플랫폼 명시)는 cpppj 전용이라 넣지 않았다.
- **cpppj 쪽 변화 반영:** 이식 계획 8절 요청 1·2가 cpppj `session-dump`·`--fixed-frame-ms`로 해결됐으므로 R4·R6 완료 기준과 0.6의 L2·L3를 그 도구로 고쳤다. "Session 덤프가 없으면 cpppj에 요청" 문장은 지웠다. 0.8 6번에 cpppj MCP 마감(단위935~937)을 적었다.
- 빌드·검사·게임 실행·디컴파일은 하지 않았다. `LEFT_JOBS.md`·cpppj·AGENTS.md는 수정하지 않았다(같은 작업 트리에서 다른 세션이 `LEFT_JOBS.md`와 cpppj 파일을 고치는 중이었고 손대지 않았다).

**2026-11-04 이관(HJOW-Athlon, 문서만, 사용자 요청):** 정리 직전(`ee69e4a4 / 20261104-25 DN`, 저장소 HEAD `0c06fa66`) 전체 원문을 [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154)에 그대로 보관했다. 본문에서 뺀 것은 단위 기록 0.127~0.153(0.154는 최신 단위라 남김), 0.1의 단위별 검사 수·PORT_MAP·fixture 증가 나열, 0.9의 단위별 완료 서술과 완료 체크 항목(11.01·외부 접속)이다. 완료됐지만 체크되지 않았던 Template·Fort 단계(TS0~TS6·F1·F2a~F2c-11)를 0.9의 하위 체크 목록에 `[x]`로 표시하고, 1절 색인에 0.137~0.154 행을 더했으며 마지막 행의 "F1 미실행·F2 이후 미착수" 표기를 고쳤다. 이관한 단위의 해소되지 않은 한계는 0.9의 '이월한 이식 한계'로, 변이 운전·options.cfg 판별 교훈은 4절로 옮겼다. 0.1의 디컴파일 기준(2026-11-03)과 cpppj 현재 값(861)을 공통 문서에 맞췄다. 빌드·검사·게임 실행·디컴파일은 하지 않았다.

**2026-11-02 이관(HJOW-Athlon, 문서만, 사용자 요청):** 정리 직전(`3b2b9e58 / 20261102-97 DN`) 전체 원문을 [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)에 그대로 보관했다. 본문에서 뺀 것은 단위 기록 0.66~0.126(0.127 이후는 TS 단계의 직접 근거라 남김), 0.1의 단위별 검사 수 나열, 0.9의 단위별 완료 서술과 완료 체크 항목이다. 0.126 이하에서 이후 단위로 해소되지 않은 한계는 0.9의 '이월한 이식 한계' 항목으로 옮겼다(0.67의 두 항목은 0.68에서 해소돼 제외했다). AGENTS.md 최근 변경(4차 로컬 MCP·와이드 해상도, 7차 2560×1440·3840×2160 FSR, 11.01, 외부 접속 문서화)은 0.7·0.8·0.9에 반영했다. 빌드·검사·게임 실행·디컴파일은 하지 않았다. `LEFT_JOBS.md`와 이 문서를 함께 고친 것은 `LEFT_JOBS.md`를 수정하지 않는다는 0.7 원칙에 대한 사용자 지시 예외다.

**2026-10-31 이관(HJOW-X3D, 문서만):** 정리 직전 전체 원문을 [이관 파일](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073)에 그대로 보관했다. 본문에서 뺀 것은 머리 안내 3개, 2026-10-28 당시 상태표와 진단 상세, 완료한 R0~R2 상세, 0.9의 완료 항목, 0.10, 단위 기록 0.11~0.65, 2절의 완료 줄이다. 빌드·검사·게임 실행·디컴파일은 하지 않았다.

**2026-10-28 이관(VM-W11-Codex):** 0절 재수립으로 더 이상 현행 의미가 없어진 내용을 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)로 옮겼다. 내용은 고치지 않고 그대로 옮겼으며, 그 파일 안의 "현재·다음·미구현" 표현은 당시 상태다.

- **옮긴 것:** 2026-10-22 이전 계획, 2026-10-09~10-23 날짜별 완료·인계 절, 구 머리말과 0~8절(2026-10-07~10 당시 1~6단계·8절 이식/대기 구분), 옛 9절 표와 9-1의 옛 순서 제안, 옛 10절 1·3·4번(해결·구현이 끝난 확인 항목), 11절(과거 미확인 항목 재분류), 14절(LEFT_JOBS.md에서 이관한 2026-09-27~10-05 이력).
- **본문에 남긴 것:** 0절 현행 계획, 1절 완료 범위 색인, 2절 9-1 결정, 3절 옛 10절의 유효한 결정, 4절 검증 명령(옛 12절), 5절 참고 문서(옛 13절).
- **앵커:** 옮긴 절의 `<a id>`(`dotnet-current-plan-20261022`·`sid-session-plan`·`legacy-dotnet-history`·`legacy-dotnet-031`~`107` 등)는 이관 파일에 같은 이름으로 남겼다. LEFT_JOBS.old.md·LEFT_JOBS.webpj.md·`docs/`의 기존 링크가 이 안내로 오도록 같은 이름의 빈 앵커를 바로 아래에 두었다. 그런 링크로 들어왔다면 이관 파일의 같은 앵커를 본다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
