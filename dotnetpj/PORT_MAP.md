# dotnetpj 포트 맵 — cpppj 소스를 C#으로 옮긴 대응표

2026-10-28 (`VM-W11-Codex`, 단위 R0-3). [LEFT_JOBS.dotnetpj.md 0절](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261028)의 구조 결정 2·3에 따라 cpppj `src/o`·`src/client`를 C#으로 직역할 때 쓰는 대응표와 규칙이다. 데이터는 [PORT_MAP.tsv](PORT_MAP.tsv)이고, 이 문서는 형식·갱신 규칙·배치 규칙을 설명한다. cpppj의 원본 주소 대응은 [cpppj/SOURCE_MAP.md](../cpppj/SOURCE_MAP.md)가 소유한다.

## 1. PORT_MAP.tsv

한 줄이 cpppj 소스 하나와 C# 파일 하나의 대응이다. 하나의 C# 파일이 여러 cpppj 소스를 근거로 삼으면 여러 줄이 된다.

| 열 | 뜻 |
| --- | --- |
| `cpp` | 저장소 기준 cpppj 소스 경로 (예: `cpppj/src/o/SidPool.cpp`) |
| `csharp` | `dotnetpj/src` 기준 C# 파일 경로 (예: `Netstorm.Core/Original/RawSidPool.cs`) |
| `base` | 그 cpppj 소스를 C#과 재대조해 반영을 마친 cpppj 커밋 |
| `status` | `L1` 원본 fixture와 대조했고 세션에는 연결하지 않음 / `session` 세션에 연결함 / `port` 새 경로(0절)로 직역해 옮김 / `retire` 대체 예정인 기존 구현 |

머리말의 `# floor-commit:`은 W01 동기화 장치의 반영 커밋과 같은 값으로, 이 커밋까지의 cpppj 변경은 검토했다고 본다.

### 초판 생성 방식과 한계 (2026-10-28)

`python dotnetpj/tools/cpp_sync.py --ports-init`가 C# 파일 주석에 적힌 `cpppj/src/…` 경로를 모아 68행을 만들었다. 각 행의 `base`는 "C# 파일을 마지막으로 고친 시점의 cpppj 커밋"과 floor 중 늦은 쪽이다.

- 주석에 cpppj 경로를 적지 않은 C# 파일은 초판에 없다. 그런 파일은 [LEFT_JOBS.dotnetpj.md 0.4](../LEFT_JOBS.dotnetpj.md#04-기존-dotnetpj-소스의-재수정-판정)의 판정표로 관리하고, 재대조할 때 이 맵에 행을 추가한다.
- 새 경로(`port`)를 옮기는 단위는 파일을 만들 때 행을 함께 추가한다. 빠뜨리면 R0 이후의 영향 추적이 그 파일을 놓친다.
- `status` 초기값은 모두 `L1`이다. `Original` 모듈이 세션에 연결된 적은 없다.
- 이 맵은 "소스가 바뀌었는가"만 알려 준다. 바뀐 내용이 C#에 영향을 주는지는 사람이 diff와 fixture로 판정한다.

## 2. 사용법

```powershell
python dotnetpj/tools/cpp_sync.py --ports                                 # 기준 이후 바뀐 cpppj 소스와 사라진 파일 보고 (읽기 전용)
python dotnetpj/tools/cpp_sync.py --ports-reflect HEAD --ports-file RawSidPool.cs   # 재대조를 마친 C# 파일의 base를 올림
dotnet test dotnetpj/tests/Netstorm.Core.Tests -c Release --filter CppPortMap          # 형식·경로 무결성
```

- `--ports-reflect`는 파일을 하나씩 지정해야 하며 전체를 한 번에 올리지 않는다. 재대조(fixture 재실행, diff 검토, 필요한 수정)를 마친 뒤에만 쓴다.
- 보고 결과가 "바뀜"이어도 연결 fixture가 같고 C# 검사가 통과하면 L1 일치는 유지된다. 이 신호는 세션 조립(R4) 때 놓치기 쉬운 의미 변경(장기 전역 수명, 공유 실패점, 새 입력 경로)을 찾는 데 쓴다.
- 단위를 마칠 때 영향 검토를 끝낸 범위까지만 `cpp-fixture-manifest.tsv`의 reflected-commit과 이 맵의 base를 올린다.

## 3. 새 경로의 배치·이름 규칙 (R1부터 적용)

cpppj의 파일·클래스·함수 경계와 이름을 따른다. 이름이 같아야 `PORT_MAP.tsv`와 diff 추적이 기계적으로 가능하다.

| cpppj 위치 | C# 위치 | 규칙 |
| --- | --- | --- |
| `src/o/*` | `Netstorm.Core/Original/` | 기존 55파일을 유지한다. 파일 이름은 `Raw` 접두 여부까지 cpppj 이름의 CamelCase를 따르되, 기존 이름은 바꾸지 않는다 |
| `src/client/*` 중 플랫폼 독립 | `Netstorm.Client/` (새 프로젝트, MonoGame 참조 금지) | `Screen.cpp` → `Screen.cs`처럼 같은 이름. 하위 폴더는 영역별(`Display/`, `Gump/`, `Hud/`, `Session/`, `Input/`, `Sound/`, `Editor/`)로 두되 네임스페이스는 `Netstorm.Client.<영역>`이다 |
| `src/client`의 Win32·GDI·DirectDraw·WINMM 부분 | `Netstorm.Game/Host/` | MonoGame/OS 대체 경계. cpppj와 다른 점과 이유를 한국어 주석에 쓴다 |
| `src/platform/*`, `src/zacket/*` | `Netstorm.Client` 또는 `Netstorm.Net` | `Bitmap`·`FileOutput` 등은 .NET 파일/이미지 API로 대체 |
| `src/app/main.cpp`의 검사 인자 | `Netstorm.Game/Program.cs` | `--run`·`--window`·`--frames`·`--set`·`--mission`·`--ui-script`·`--ui-report`·`--screenshot` 등 같은 이름 |

- 한 클래스·함수가 cpppj와 1:1이 아닌 경우(예: C++의 `.inc` 표나 전역 DWORD)는 C# 주석에 대응 cpppj 위치를 적고, 구조가 달라진 이유를 쓴다.
- 새 파일을 만들 때 첫 `<summary>`에 `cpppj/src/<o|client>/<파일>` 경로를 적는다. `--ports-init`이 이 경로를 읽는다(이미 있는 `PORT_MAP.tsv`에는 행을 직접 추가한다).
- 이식한 cpppj 검사(`cpppj/tests/*Tests.cpp`)는 같은 이름의 xUnit 클래스로 옮긴다. 원본 fixture는 `Link`로 연결하고 `cpp-fixture-manifest.tsv`에 등록한다.

## 4. 단계별 이식 대상 (계획)

실행 순서와 완료 기준은 [0.5절](../LEFT_JOBS.dotnetpj.md#05-실행-순서-r0r8)을 따른다. 아래는 대상 cpppj 파일군과 목적지의 요약이다. 파일 수는 2026-10-28 HEAD 기준이다.

| 단계 | cpppj 파일군 | 목적지 |
| --- | --- | --- |
| R1 | `Screen`, `ScreenPaletteApply`, `ScreenResolutions`, `ScreenPresentation`, `InputEvent`, `UserInput`의 큐, `ClientMain`의 루프 골격 | `Netstorm.Client/Display`, `Netstorm.Client/Input`, `Netstorm.Game/Host` |
| R2 | `VFXDraw`·`VFXMappedDraw`·`VFXLine`·`VFXTransform`, `RawShapeToBuffer`, `SpriteDraw`, `*ColorMaps`, `Palette*`, `RawColorSort*`, `BitmapFont`, `GifImage`, `RawImage` | `Netstorm.Client/Display` |
| R3 | `Gump*`(약 15개), 위젯류, `UberGump`·`UberRootGump`, `NativeMainMenuState`, Tell·Dialog·StyleText·Markup·QuickHelp, `Cursor` | `Netstorm.Client/Gump` |
| R4 | `RawCampaignSession*`, `RawSurfaceWorld`·`RawSurfaceStorage`, `State`, `Mission*`, 미구현 `Raw*` 월드 모듈 | `Netstorm.Client/Session`, `Netstorm.Core/Original` |
| R5 | `Renderer`, `DrawSort`·`DrawSubmission`, 수집 모듈, `SquidRenderer`, 시각 시스템, `RawCamera*`, HUD·미니맵·덱 | `Netstorm.Client/Display`, `Netstorm.Client/Hud` |
| R6 | 입력·선택·커서, 게임 메뉴, 우클릭 메뉴, 규칙 모듈, `RawAi*`, `Sound*` | `Netstorm.Client/Input`, `Netstorm.Client/Session`, `Netstorm.Client/Sound`, `Netstorm.Core/Original` |
| R7 | Editor 계열, 설정(`Config*` 저장), 배포 | `Netstorm.Client/Editor` 외 |

## 5. 현재 상태 (2026-10-28)

- 행 226개: `L1` 109개(초판 68개 + R0-4 9개 + R2a 6개 + R2b 2개 + R2c 10개 + R2d-1 2개 + R3h-4a 4개 + R3i-1 2개 + R3i-2 2개 + R3j-1 2개 + R3j-2 2개), `port` 117개(`Netstorm.Client`의 화면·입력·호스트·그리기 계층 41개와 원본 수집 항목 1개·Gump 기반 72개와 캡처·스크립트 3개). `--ports` 기준 cpppj 소스가 바뀐 쌍은 29개이며 `L1` 27개·`port` 2개다. 변경의 성격은 [재대조 영향표](../docs/dotnet-cpp-resync-20261028.md)에 정리했다. R0-4에서 `RawSquidReward.cs`·`RawSpStore.cs`·`RawSquidFactory.cs`·`RawSidPool.cs`·`SidPool.cs`·`RawPriestState.cs`·`RawPriestDamage.cs`·`RawPriestPreDestroy.cs`·`RawGraph.cs`·`RawAssetPostPop.cs`·`OriginalTypeRecord.cs` 11개 행의 base를 올리고(35→25개) 9개 행을 새로 추가했다([단위 문서](../docs/dotnet-r04-sp-bits-20261028.md), [단위 문서](../docs/dotnet-r04-fort-sp-write-20261028.md), [단위 문서](../docs/dotnet-r04-retype-conversion-20261028.md), [단위 문서](../docs/dotnet-r04-pool-sidlists-20261028.md), [단위 문서](../docs/dotnet-r04-priest-actions-20261028.md), [단위 문서](../docs/dotnet-r04-rawgraph-region-20261028.md), [단위 문서](../docs/dotnet-r04-assetpostpop-20261028.md)). R1-2a에서 호스트 8개 행을 새로 추가했다([단위 문서](../docs/dotnet-client-screen-20261028.md) 6절). R2a에서 팔레트 6개 행을 새로 추가하고 `Screen.cs` 행의 base를 올렸다([단위 문서](../docs/dotnet-client-screen-20261028.md) 7절). R2b에서 그리기 원시 6개 행을 새로 추가했다([단위 문서](../docs/dotnet-client-screen-20261028.md) 8절). R2c에서 색 표 10개 행을 새로 추가했다([단위 문서](../docs/dotnet-client-screen-20261028.md) 9절). R2d-1에서 변환·합성·이미지·시작 8개 행을 새로 추가했다(`L1` 2개·`port` 6개, [단위 문서](../docs/dotnet-client-screen-20261028.md) 10절). R2d-2에서 판본·표시자료·캡처 5개 행을 새로 추가하고(`port` 5개) `RawSidPool.cs`·`OriginalTypeRecord.cs` 2개 행의 base를 올렸다([단위 문서](../docs/dotnet-client-screen-20261028.md) 11절).
- `Netstorm.Client`의 `port` 행은 [R1-1 문서](../docs/dotnet-client-screen-20261028.md)가 만들었다. 새 이식 쌍은 `python dotnetpj/tools/cpp_sync.py --ports-add CPP:CSHARP[:STATUS]`로 추가한다(`--ports-init`은 초판 전용).

- R2d-3에서 일반 객체 출력·픽셀 경계·수집 항목 4행을 추가했다([검증과 남은 연결](../docs/dotnet-spritedraw-20261029.md)). `spritedraw` 명령열 1,389행 L1은 통과했으며 실제 세션/실자산 연결은 R4·R5에 남는다.

- R3a에서 Gump 생성/등록·위치/수집 4행을 추가했다([검증과 남은 연결](../docs/dotnet-gump-base-20261029.md)). 원본 fixture 2,112행의 전체 바이트/표/출력/캐시/가상 투영 순서를 대조했고 트리 부착·수명·입력·Draw 조립은 다음 단위다.

- R3b에서 Gump 트리/배치 2행을 추가했다([검증과 남은 연결](../docs/dotnet-gump-tree-20261029.md)). 원본1,722행의 전체 노드/전역·가상 호출 당시 상태를 대조했으며 실제 메뉴/세션 조립은 남는다.

- R3c에서 Gump 수명/루트 교체2행을 추가했다([검증과 남은 연결](../docs/dotnet-gump-lifetime-20261029.md)). 원본761행의 전체 노드/표/전역·콜백 당시 상태를 대조하고 실제 생성/트리/기본 소멸자/해제/등록 재사용을 연결했다.

- R3d-1에서 Gump 사건/입력4행을 추가했다([검증과 남은 연결](../docs/dotnet-gump-events-input-20261029.md)). 원본1,467행의 전체 노드/전역/표·콜백 당시 상태를 대조하고 실제 선택·캡처/dialog/overlay·수명을 연결했다.

- R3d-2에서 Gump 프레임2행을 추가했다([검증과 남은 연결](../docs/dotnet-gump-frame-20261029.md)). 원본810행의 전체 노드/등록/idle 표·콜백 상태를 대조하고 실제 지연 삭제/통지/재그리기 요청을 연결했다.

- R3e에서 Gump Draw/Visual/픽셀 대상6행을 추가했다([검증과 남은 연결](../docs/dotnet-gump-draw-20261029.md)). 원본1,620행의 클립/투영/버튼 출력과 헤드리스 픽셀을 대조했다. 실제 공통 정렬/분배는 R5에 남는다.

- R3f에서 배경 생성/질감/현재 자산 출력6행을 추가했다([검증과 남은 연결](../docs/dotnet-gump-background-20261029.md)). 원본804행과 실제 RLE 픽셀을 대조했다. GameAssets 전용 생성자는 전체 자산 로더 조립 때 연결한다.

- R3g-1에서 BitmapFont/Windows GDI·GumpText/GumpTextDraw8행을 추가했다([검증과 남은 연결](../docs/dotnet-gump-font-20261029.md)). 원본1,719행과 실제18캐시·GDI 생성/글리프 픽셀을 대조했다. 돌 버튼 측정 바인딩과 Renderer 장면은 해당 조립 단위에서 잇는다.

- R3g-2에서 ButtonGump/StoneButtonGump/GumpHelp6행을 추가하고 GumpText/배경 생성의4행 기준을 갱신했다([검증과 남은 연결](../docs/dotnet-gump-button-20261029.md)). 원본742행과 실제 캡처/배경/글리프/수명을 연결했다. 공통 CRT 정렬/분배와 실제 메뉴 조립은 남는다.

- R3h-1에서 StyleText/한 줄TextGump4행을 추가했다([검증과 남은 연결](../docs/dotnet-styletext-single-20261029.md)). 독립 원본 실행 기대값이 없어L1이 아닌 정적 계약/실제 글꼴·픽셀 통합이다. 다중 줄/메뉴 조립과 캡처 비교는 남는다.

- R3h-2에서 StyleTextBuffer/Markup/다중 줄TextGump6행을 추가하고 한 줄TextGump2행 기준을 갱신했다([검증과 남은 연결](../docs/dotnet-stylebuffer-scroll-20261029.md)). 독립 원본 실행 자료를 추가하지 않은 정적 계약/실제 글꼴·SHP 혼합 픽셀 통합이다. 텍스트 파서/위젯 생성·수명·입력과 실제 메뉴 조립은 남는다.

- R3h-3a에서 StyleTextMarkup/ScrollingTextGumpMarkup/StyleTextBufferStore6행을 추가하고 다중 줄TextGump2행 기준을 갱신했다([검증과 남은 연결](../docs/dotnet-stylemarkup-parser-20261029.md)). Widgets 파일의 버퍼 저장소 부분만 별도 C# 파일로 옮겼다. 독립 원본 실행 자료를 추가하지 않은 정적 계약/실제 파서·버퍼·글꼴·SHP 통합이며 Slider/텍스트 위젯 생성·입력과 전체 자산/음향 조립은 남는다.

- R3h-3b-1에서 ScrollGump/ScrollGumpWidgets4행을 추가하고 배경 생성2행 기준을 갱신했다([검증과 남은 연결](../docs/dotnet-slider-widgets-20261029.md)). 실제 Slider 값·부착·입력/돌 화살표·질감 손잡이 생성·출력·수명/다중 줄 표시 연결이며 독립 원본 실행 대조/L1은 아니다. 다중 줄 Widget 생성·입력과 실제 메뉴/Renderer 조립은 남는다.

- R3h-3b-2에서 ScrollingTextGumpWidgets2행을 추가했다([검증과 남은 연결](../docs/dotnet-scrolling-widget-20261029.md)). 실제 버퍼/Slider 생성·입력/수명과 현재 가상 분배이며 독립 원본 실행 대조/L1은 아니다. MenuGump 전체와 실제 메뉴/Renderer 조립은 남는다.

- R3h-4a에서 MenuGump/MenuGumpLayout4행을 L1로 추가하고 StyleText2행 기준을 갱신했다([검증과 남은 연결](../docs/dotnet-menu-layout-20261029.md)). 원본 메뉴308/레이아웃260행의 전체 바이트·콜백/CRC·반환이 일치하며 실제 항목 소유/글꼴·픽셀/트리·프레임을 연결했다. 최상위 기존 관찰은 cpppj-root 읽기 전용 Link로 목록에 등록했다. Controls/Widgets/Input/Draw와 실제 메뉴/Renderer 조립은 남는다.

- R3h-4b에서 MenuGumpControls2행을 port로 추가했다([검증과 남은 연결](../docs/dotnet-menu-controls-20261029.md)). 실제 항목/제목/설정 조회와 CD 문맥/선택 helper를 연결했다. 독립 PE 관찰이 없는 정적 계약·실제 조립 검사이며 L1은 아니다. Widgets/Input/Draw와 실제 메뉴/Renderer 조립은 남는다.

- R3h-4c에서 GumpDecoration2행을 port로 추가했다([검증과 남은 연결](../docs/dotnet-decoration-20261029.md)). 실제 배경/원색 모서리·변경 영역·수명과 물리 SHP 픽셀을 연결했다. 독립 PE 관찰/L1은 아니며 전체 GameAssets와 MenuGumpWidgets/Input/Draw 조립은 남는다.

- R3h-4d에서 MenuGumpWidgets/GumpFormBinding4행을 port로 추가했다([검증과 남은 연결](../docs/dotnet-menu-widgets-20261029.md)). 실제 생성/장식·제목/controlled 표/부착/소유 수명과 공용 SID Form을 연결했다. 독립 PE 관찰/L1은 아니며 Input/Draw와 실제 메뉴/Renderer 조립은 남는다.

- R3h-4e에서 MenuGumpDraw2행을 port로 추가했다([검증과 남은 연결](../docs/dotnet-menu-draw-20261029.md)). 실제 배경/강조/구분선/체크·문자/자식 표시와 원본 J02 픽셀·판본별 큰 정수Y를 대조했다. 독립 PE Draw 관찰/L1은 아니며 Input과 실제 메뉴/Renderer 조립은 남는다.

- R3h-4f에서 MenuGumpInput2행을 port로 추가했다([검증과 남은 연결](../docs/dotnet-menu-input-20261029.md)). 실제 사건/캡처·선택/명령·자식 열기/닫기·부모 통지·수명과 소리 인자를 연결했다. 독립 PE 입력 관찰/L1은 아니며 실제 메뉴 상위 소유/Renderer/장치 조립은 남는다.

- R3i-1에서 TextEntryBuffer2행을 L1, TextEntryStorage2행을 port로 추가했다([검증과 남은 연결](../docs/dotnet-text-entry-buffer-20261029.md)). 원본570행의 전체 상태/메모리·효과/반환/중단이 일치하며 실제 본문/줄 표 소유를 연결했다. TextEntryGump 위젯/장치·전체 메뉴 조립은 남는다.

- R3i-2에서 TextEntryGump.h/.cpp 2행을 L1로 추가했다([구현과 검증](../docs/dotnet-text-entry-gump-20261029.md)). 원본2292행의 파생/자식/버퍼·메모리/외부 효과가 일치하며 같은 실제 공통 트리/Slider/글꼴/픽셀/수명 연결도 작성했다. RawTextEntryControls의 전체 소유 조립·실제 호스트/메뉴 화면은 후속이다.

- R3j-1에서 ImageGump.h/.cpp2행을 L1로 추가했다([구현과 검증](../docs/dotnet-image-gump-20261029.md)). 원본221입력의 전체0xb4노드/효과/반환이 일치한다. 실제SHP/256색표/16.16 VFX와200×200 적중·공통트리/출력/수명도 연결했으나 전체GameAssets/아이콘 소유자·덱/메뉴 호스트 조립과 전체 화면 판정은 후속이다.

- R3j-2에서 MetaImageGump.h/.cpp2행을 L1로 추가했다([구현과 검증](../docs/dotnet-meta-image-gump-20261029.md)). 원본326행의 기하/등록/가상효과가 일치하며 현재RawImage/실제VFX·색표/클립·공통트리와 비소유수명을 연결했다. MetaImageList514행·창/메뉴 상위 소유와 호스트 조립은 후속이다.
