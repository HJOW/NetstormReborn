# dotnetpj 복원 기록 — 커서 표·배경 GIF 색·미션 시작 값 (2026-10-10)

`HJOW-X3D`(Windows 11, .NET SDK 10.0.401)에서 [LEFT_JOBS.dotnetpj.md](../LEFT_JOBS.dotnetpj.md)의 5-4·5-5·5-6과 6단계를 진행했다. 이 PC는 마지막 디컴파일 PC(`HJOW-Athlon`)가 아니고 디컴파일을 하지 않기로 한 PC라서 **디컴파일 결과를 읽지 않았다.** 커밋된 cpppj C++ 소스·분석 문서·원본 자료 파일(GIF·COL·`.fort`·미션 스크립트)과, 이미 추출돼 있던 실행 파일 리소스(`extracted/res/Netstorm/`)만 썼다.

- 원본 게임·복사본 실행 없음. 새 디컴파일·Ghidra 내보내기 없음. `cpppj/` 소스·AGENTS.md·LEFT_JOBS.md·원본 폴더 변경 없음. 커밋/푸시 없음.
- cpppj 는 **수정하지 않고 빌드만** 했다(`cpppj/build/`, Git 제외). 콘솔 명령 `--dump-world` 로 네 미션의 시작 상태를 얻었다. 이 명령은 원본 자료 폴더를 읽기만 하며 창을 만들지 않는다. 작업을 시작한 뒤(2026-10-10 01:06 이후) `originals/`·`originalCD/` 에 만들어지거나 바뀐 파일이 없음을 파일 시각으로 확인했다.
- 클론 창 검사는 창 모드(숨김 시작)로만 했다. 전체화면 전환이 들어 있는 `clone_ui_smoke.ps1` 은 돌리지 않았다.

## 1. 배경 GIF 의 색 (5-5)

원본(`00419ec0` → `004dae60`)은 GIF 의 **색 번호**를 화면에 그대로 복사하고 GIF 안의 RGB 표로 색을 다시 맞추지 않는다([cpp-menu-reconstruction.md](exe/cpp-menu-reconstruction.md)). 클론은 `Texture2D.FromStream` 으로 GIF 내장 RGB 표를 쓰고 있었다. 먼저 두 표가 같은지 비교했다.

| 그림 | 크기 | 쓰는 색 번호 | 게임 팔레트(`GIFCLOUD.COL`)와 RGB 가 다른 번호 | 영향 픽셀 | 채널 최대 차 |
|---|---|---:|---:|---:|---:|
| `d/titleMenu.gif` | 639×480 | 181 | **177** | 295,080 | 12 |
| `d/Gifcloud.gif` | 512×512 | 18 | 1 (243번) | 8,860 | 3 |
| `d/Gifcloud2.GIF` | 512×512 | 13 | 0 | 0 | 0 |

같지 않으므로 색 번호를 해독해 게임 팔레트로 칠하도록 바꿨다.

- 새 [`GifImage`](../dotnetpj/src/Netstorm.Assets/GifImage.cs): GIF87a/89a 첫 이미지의 색 번호 해독(인터레이스·투명 번호·LZW 코드 폭 증가·사전 재설정·손상 거부). 기준 구현은 `cpppj/src/client/GifImage.cpp` 다.
- `MainMenuView.LoadImage` 가 팔레트를 받아 `GifImage.Decode` → `SpriteAnimation.ToTexture` 로 텍스처를 만든다. 메뉴의 타이틀·구름과 미션 화면의 하늘(`FortMapViewer` 의 `Gifcloud.gif`)이 같은 경로를 쓴다.
- **타이틀 그림을 640 폭으로 늘려 그리던 것도 고쳤다.** `titleMenu.gif` 는 639×480 인데 `Rectangle(…, 640, 480)` 에 그려 가로로 1픽셀 늘어났다. 원본(cpppj `UberGump` 의 `Paste`)처럼 640×480 자리의 왼쪽 위에 제 크기로 놓는다.

**검증**

- 단위 검사 10개(`GifImageTests`): 실제 그림 5개(`titleMenu`·`Gifcloud`·`Gifcloud2`·`victory`·`Defeat`)의 색 번호 전체 SHA-256 을 Pillow 해독 결과와 대조, 내장 표와 게임 팔레트가 다른 번호 수(177·1), 합성 GIF 의 인터레이스·이미지 위치·배경·투명, 손상 파일 거부(모든 길이의 잘림 포함).
- 화면: 클론 1024×768 메인 메뉴 캡처에서 버튼 영역을 뺀 **768,282픽셀 전부**가 "GIF 색 번호 × 게임 팔레트"의 독립 합성(Python)과 같다. 같은 캡처를 옛 방식(GIF 내장 표) 합성과 비교하면 482,653픽셀만 같다. 영어·한국어 캡처 모두 같은 결과다.
- 참고: `screenShots/mainMenu.png`(원본 창 캡처)는 색이 정확히 보존된 캡처가 아니어서(같은 구름 영역에 820색) 픽셀 일치 판정에는 쓰지 못했다. 다만 243번 색은 COL 값 `(82,90,99)` 만 캡처에 있고 GIF 표 값 `(81,87,101)` 은 없었으며, 타이틀 영역의 평균 절대 차도 COL 쪽이 작았다(5.74 대 6.92). 원본 판정의 주 근거는 cpppj 문서의 디컴파일 결과다.

## 2. 커서 표 (5-4)

원본 커서 번호 1~18 의 그룹 표(10.78 `005423a8`)를 [`GameCursor`·`GameCursors`](../dotnetpj/src/Netstorm.Core/Rules/GameCursor.cs)로 옮겼다. 열거형 값이 원본 커서 번호다.

- 그룹 → 그림 번호는 `extracted/res/Netstorm/RT_GROUP_CURSOR_*.bin` 에서 읽었다(그룹마다 그림 하나, 모두 32×32 단색, 308바이트).
- 나머지 그림 13개(`RT_CURSOR_3·4·9~19.bin`)를 `assets/game-data/cursors/` 에 복사했다. 기존 5개는 추출본과 SHA 가 같다. 번호 8 과 13 은 파일 내용이 같다.
- `OriginalCursor` 가 18개를 모두 SDL 커서로 만든다. **상황별 선택은 넣지 않았다.** `FortMapViewer.CursorAt` 의 다섯 상태 매핑(TEST02 관찰)은 그대로다. 쓰임이 확인되지 않은 13개의 이름은 그림의 생김새만 적었다([표](../assets/game-data/cursors/README.md)).
- 검사: `GameCursorTests` 8개(번호·그룹 순서, 확인된 다섯 가지의 리소스 유지, 18개 동봉 파일의 존재·형식·중복 없음, 범위 밖 번호 거부), `CursorBitmapTests` 의 핫스팟 검사를 5개에서 18개로 확대. 클론 창이 18개를 만들고 정상 시작함을 스모크로 확인했다.

## 3. 메인 메뉴 확인 (5-6)

[cpp-menu-reconstruction.md](exe/cpp-menu-reconstruction.md) 의 수치와 `MainMenuView` 를 코드로 맞춰 봤다.

| 항목 | 결과 |
|---|---|
| 버튼 폭 75·높이 19·피치 79, 시작 `(356,311)`, 둘째 줄 y 334 | 같다 (`cx − 156`, `cy − 73 + 23`) |
| 아래쪽 판정 +1픽셀 | 같다 (`OriginalUiSkin.ButtonHitExtraHeight`) |
| 돌 버튼은 누름을 잡고 같은 영역에서 뗄 때 실행, 호버 변화 없음 | 같다 (`ButtonGump`) |
| 목록 항목은 호버 강조, 누르는 즉시 실행. 오른쪽 버튼·잠금 항목은 실행하지 않음 | 같다 |
| 타이틀 그림 위치·크기 | **달랐다 → 고침** (1절) |
| 캠페인 목록을 `offical*.english` 에서 모으고 `Done...` 값으로 완료·잠금을 읽음 | **다르다 — 미착수.** 클론은 목록과 문구가 코드에 고정돼 있고 공개 여부를 `CampaignAccess`(1-1·1-2)로 정한다 |

## 4. 미션 시작 값 (6단계)

### 기대값

`dotnetpj/tools/export_cpp_world.py` 가 `NetstormCpp.exe --dump-world originals <미션>` 의 `player`·`object` 줄을 [`world-start-1078.tsv`](../dotnetpj/tests/Netstorm.Core.Tests/Fixtures/world-start-1078.tsv)로 내보낸다(플레이어 8줄, 오브젝트 2,036줄). **원본 x86 실행 결과가 아니라 cpppj 의 현재 월드 어댑터 출력**이며, 파일 머리말에 관련 cpppj 소스 세 개의 SHA-256 을 남겼다.

### 6-1. 소유자

| 비교 | 결과 |
|---|---|
| 저장 오브젝트의 수·순서·타입·영역·칸 좌표·프레임·수량·내용물 수 | 네 미션 2,036개 **모두 같다** (수정 없이 통과) |
| 소유자 — 세션이 쓰던 저장 값 그대로(`Owner ?? 0`) | noIsland 666개(세션 엔티티가 아니라 영향 없음)와 **Save the Island! 의 residence 2개**(저장 값 0, cpppj 1)가 달랐다 |
| 소유자 — 정규화 적용 뒤(`FortObject.LoadOwner`) | 2,036개 **모두 같다** |

- 결정: 세션(`BattleMap`·`BattleSession`·`BattleSessionFactory`)이 `FortObject.LoadOwner` 를 쓴다. 저장 소유자 바이트 0·9 이상 → 1(원본 `004bdc60`), geyser·buried·island 타입 → 0.
- 소유자를 저장하지 않는 타입을 cpppj 어댑터는 1 로 두지만 원본 근거가 없어(원본은 호출자가 준 영역 소유자를 쓴다) 0 으로 뒀다. 네 미션에는 그런 오브젝트가 **없음**을 검사가 확인한다.
- 받침 색을 고르는 `FortIslandSupports` 는 noIsland 의 저장 소유자를 계속 쓴다(cpppj 는 받침 위 건물의 소유자를 쓰는 어댑터다). 받침 소유자의 실제 전파는 8절 대기 항목이다.

### 6-2. 시작 SP·지식·동맹·색

| 미션 | 플레이어 | 수정 전 C# | cpppj | 수정 뒤 |
|---|---|---|---|---|
| The War Begins! | 1 / 2 | 3000 / 0 | 3000 / 0 | 같다 |
| Save the Island! | 1 / 2 | 2000 / **전투 옵션 금액**, AI 지식 없음 | 2000 / 2000, AI 지식 6개 | 같다 |
| tutorial1 | 1 | 0 | 0 | 같다 |
| TEST01 | 1 / 2 / 3 | 50000 / **전투 옵션 금액** / **전투 옵션 금액**, AI 지식 없음 | 50000 / 0 / 0, AI 지식 all | 같다 |

- 원인: AI 의 시작 금액·지식을 번호 없는 키(`aiStartMoney`·`aiTech`)로만 읽고 공개 캠페인 미션(1-1·1-2)에만 적용했다. 게임 경로에서는 그 밖의 미션의 AI 가 맵에 저장된 `Money`(1-1·Save the Island 는 100000)로 시작했다.
- 수정: `MissionStart` 가 `aiNStartMoney`·`aiNTech` 를 번호별로 읽고, 플레이어 2 는 번호 없는 구식 키를 대신 쓸 수 있다(`AiStormPower`·`AiKnowledgeFor`). 동맹(`aiAllyList`)·색(`aiColor`)도 같은 구식 키 규칙을 따른다. 세션은 미션이 있으면 모든 AI 에게 이 값을 적용한다(없으면 0·빈 목록).
- 동맹 비트와 색 번호는 수정 없이 같았다(TEST01: 1·2 동맹, 색 2 → 주황 8·3 → 빨강 2).
- **임시 전략 AI(`BattleSession.CampaignAi`)가 도는 미션은 여전히 1-1·1-2 뿐이다.** 다른 미션의 AI 는 시작 값만 맞고 행동하지 않는다.

### 6-3. 본섬 마스크

`MapRenderingTests.Original_IslandMaskMatchesCppWorld`: 네 미션의 256×256 마스크 SHA-256 과 본섬 칸 수(1,505·2,082·763·5,172)가 [cpp-world-reconstruction.md](exe/cpp-world-reconstruction.md) 의 표와 같다. 기존에는 두 미션만 있었다. 수정 없이 통과했다.

### 6-4. 받침 표시

`FortIslandSupportTests.Original_SupportAnchorsMatchCppGrouping`: cpppj `GameWorld::BuildTerrain` 의 묶는 순서(createsisland·geyser 오브젝트의 칸 → noIsland 칸)를 검사 안에 옮겨 기준점 집합을 비교했다. 네 미션(받침 13·26·0·53개)에서 `FortIslandSupports.Find` 와 같다. 뷰어는 받침의 island·islandStalag 를 기준점 칸에 그대로 그리며(y 이동 없음) cpppj 와 같다. 수정 없이 통과했다.

### 6-5. 최대 HP

세션 엔티티의 시작 HP 는 타입 `maxHitPoints` 이고 cpppj 덤프의 HP 열과 네 미션에서 같다(`SessionEntities_StartWithCppOwnersAndHitPoints`). mana(타입 158)의 "약화 옵션이 켜지면 최대 HP ÷ 4"는 **C# 에 없다.** `BattleOptions` 에 해당 옵션이 없고 세션이 mana 를 HP 있는 엔티티로 만들지도 않는다. cpppj 도 이 조건을 생성자 인자(`weakenedMana`, 기본 꺼짐)로만 받고 어느 전투 옵션인지는 연결하지 않았으므로 지금은 넣지 않았다.

## 5. 검증

- Release 빌드 **경고 0·오류 0**. 착수 기준선 888개(Assets 285 + Core 603).
- 단위 검사 **940개 통과·실패/스킵 0**(Assets 316 + Core 624). 새 검사 52개: GIF 10, 커서 표 8 + 핫스팟 13, 본섬 마스크 4, 받침 기준점 4, cpppj 월드 대조 13. 기존 x86 fixture 는 수정하지 않았고 x86 대조 50개가 그대로 통과한다.
- 클론 창(창 모드):
  - 메뉴 → 캠페인 → 1-1 브리핑 → 플레이(배치·다리 집기) → 나가기 → 1-2 → 나가기: **UI 검사 20개 통과**, stderr 비어 있음.
  - `clone_test01_smoke.ps1`: 영어 4:3/16:9·한국어 4:3/16:10, **4개 조합·96개 검사 통과**. TEST01 의 AI 2·3 은 이제 SP 0 으로 시작한다.
  - `clone_combat_smoke.ps1`: 12개 장면 통과.
  - 캡처는 Git 제외 `extracted/screens/dotnet-x3d-20261010/` 에 있다.
- 돌리지 않은 것: `clone_ui_smoke.ps1`(전체화면 전환·해상도 저장 포함). 이번 변경은 화면 설정 코드를 건드리지 않았다.

## 6. 남은 것

- 5-3 원본 영어 글꼴(`.chfnt`)의 UI 출력·폭 측정 연결 — 사용자 확인 대기(LEFT_JOBS.dotnetpj.md 10절 2번).
- 5-6 캠페인 목록을 `offical*.english`·`Done...` 값에서 읽기.
- 5-1 전체 월드 표시 목록, 9-1 의도적 차이 판정, 8절의 cpppj 대기 항목.
