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

- 행 105개: `L1` 83개(초판 68개 + R0-4 9개 + R2a 6개), `port` 22개(`Netstorm.Client`의 화면·입력·호스트 계층 19개와 캡처·스크립트 3개). `--ports` 기준 cpppj 소스가 바뀐 쌍은 26개이며 모두 `L1` 행이다. 변경의 성격은 [재대조 영향표](../docs/dotnet-cpp-resync-20261028.md)에 정리했다. R0-4에서 `RawSquidReward.cs`·`RawSpStore.cs`·`RawSquidFactory.cs`·`RawSidPool.cs`·`SidPool.cs`·`RawPriestState.cs`·`RawPriestDamage.cs`·`RawPriestPreDestroy.cs`·`RawGraph.cs`·`RawAssetPostPop.cs`·`OriginalTypeRecord.cs` 11개 행의 base를 올리고(35→25개) 9개 행을 새로 추가했다([단위 문서](../docs/dotnet-r04-sp-bits-20261028.md), [단위 문서](../docs/dotnet-r04-fort-sp-write-20261028.md), [단위 문서](../docs/dotnet-r04-retype-conversion-20261028.md), [단위 문서](../docs/dotnet-r04-pool-sidlists-20261028.md), [단위 문서](../docs/dotnet-r04-priest-actions-20261028.md), [단위 문서](../docs/dotnet-r04-rawgraph-region-20261028.md), [단위 문서](../docs/dotnet-r04-assetpostpop-20261028.md)). R1-2a에서 호스트 8개 행을 새로 추가했다([단위 문서](../docs/dotnet-client-screen-20261028.md) 6절). R2a에서 팔레트 6개 행을 새로 추가하고 `Screen.cs` 행의 base를 올렸다([단위 문서](../docs/dotnet-client-screen-20261028.md) 7절).
- `Netstorm.Client`의 `port` 행은 [R1-1 문서](../docs/dotnet-client-screen-20261028.md)가 만들었다. 새 이식 쌍은 `python dotnetpj/tools/cpp_sync.py --ports-add CPP:CSHARP[:STATUS]`로 추가한다(`--ports-init`은 초판 전용).
