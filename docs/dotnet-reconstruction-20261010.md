# dotnetpj 복원 기록 — 커서 표·배경 GIF 색·미션 시작 값·원본 영어 글꼴·창/메뉴 치수·캠페인 목록 자료 (2026-10-10)

`HJOW-X3D`(Windows 11, .NET SDK 10.0.401)에서 [LEFT_JOBS.dotnetpj.md](../LEFT_JOBS.dotnetpj.md)의 5-4·5-5·5-6과 6단계를 진행했고, 같은 날 사용자 결정("언어가 영어일 때의 글꼴은 기존 게임의 글꼴을 따른다")에 따라 5-3(7절)을 이어서 했다. 이 PC는 마지막 디컴파일 PC(`HJOW-Athlon`)가 아니고 디컴파일을 하지 않기로 한 PC라서 **디컴파일 결과를 읽지 않았다.** 커밋된 cpppj C++ 소스·분석 문서·원본 자료 파일(GIF·COL·`.fort`·미션 스크립트)과, 이미 추출돼 있던 실행 파일 리소스(`extracted/res/Netstorm/`)만 썼다.

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

## 5. 검증 (1~4절 시점)

7절(원본 영어 글꼴)까지 마친 뒤의 최종 수치는 7절 끝에 있다.

- Release 빌드 **경고 0·오류 0**. 착수 기준선 888개(Assets 285 + Core 603).
- 단위 검사 **940개 통과·실패/스킵 0**(Assets 316 + Core 624). 새 검사 52개: GIF 10, 커서 표 8 + 핫스팟 13, 본섬 마스크 4, 받침 기준점 4, cpppj 월드 대조 13. 기존 x86 fixture 는 수정하지 않았고 x86 대조 50개가 그대로 통과한다.
- 클론 창(창 모드):
  - 메뉴 → 캠페인 → 1-1 브리핑 → 플레이(배치·다리 집기) → 나가기 → 1-2 → 나가기: **UI 검사 20개 통과**, stderr 비어 있음.
  - `clone_test01_smoke.ps1`: 영어 4:3/16:9·한국어 4:3/16:10, **4개 조합·96개 검사 통과**. TEST01 의 AI 2·3 은 이제 SP 0 으로 시작한다.
  - `clone_combat_smoke.ps1`: 12개 장면 통과.
  - 캡처는 Git 제외 `extracted/screens/dotnet-x3d-20261010/` 에 있다.
- 돌리지 않은 것: `clone_ui_smoke.ps1`(전체화면 전환·해상도 저장 포함). 이번 변경은 화면 설정 코드를 건드리지 않았다.

## 6. 남은 것

- 5-6 캠페인 목록을 `offical*.english`·`Done...` 값에서 읽기 → 같은 날 후속으로 자료 읽기만 만들었다(9절). 화면 연결·완료 값 저장이 남았다.
- 5-1 전체 월드 표시 목록, 9-1 의도적 차이 판정, LEFT_JOBS.dotnetpj.md 8절의 cpppj 대기 항목.
- 7절 끝의 "창·메뉴 치수" 목록 (글꼴을 맞춘 뒤 드러난 차이) → 같은 날 후속으로 대부분 고쳤다(8절). 돌 버튼 모양·도움말 줄 간격 등이 남았다.

## 7. 원본 영어 글꼴 (5-3)

**사용자 결정(2026-10-10): 언어가 영어일 때의 글꼴은 기존 게임의 글꼴을 따른다.** 언어가 한국어이면 지금처럼 D2Coding 을 쓴다.

### 어느 글꼴을 어디에 쓰는가 — 원본 캡처로 확인

`.chfnt` 캐시 18개로 같은 문자열을 합성해 원본 창 캡처의 밝은 글자 픽셀과 겹쳐 보았다(일치율 = 교집합 ÷ 합집합). 캡처는 색이 정확히 보존되지 않았지만 글자 모양은 1:1 이다.

| 캡처 | 문자열 | 일치한 캐시 | 일치율 |
|---|---|---|---:|
| `The War Begins! - Briefing.png` | 본문 "Having conquered and enslaved the people" | `!Arial.normal.14.700` (슬롯 0) | 1.000 |
| 같은 캡처 | 제목 "The War Begins!" | `!Arial.normal.20.700` (슬롯 3) | 1.000 |
| 같은 캡처 | 버튼 "Play Mission" | `!Arial.normal.14.700` (슬롯 0) | 0.912 |
| `mainMenu - Options.png` | 옵션 목록 13줄 가운데 11줄 | `!Arial.normal.14.700` (슬롯 0) | 1.000 |
| `help - NetStorm Instructions.png` | 제목 "NetStorm Instructions" | `!Arial.normal.20.700` (슬롯 3) | 1.000 |
| 같은 캡처 | 본문 "help window." | `!Arial.normal.14.0` (슬롯 5) | 1.000 |
| 같은 캡처 | "GAME HELP"·"GAME SUPPORT" | `!Arial.bold.14.0` = `!Arial.normal.14.700` | 1.000 |

- 브리핑·안내 창의 본문, 버튼, 목록 행은 모두 **Arial 14픽셀 굵게(슬롯 0)** 다. cpppj `UberGump` 는 본문에 슬롯 5 를 쓰는데, 원본 화면은 슬롯 0 이다(cpppj 쪽 어댑터 차이 — cpppj 미수정, 인계 문서에 적었다).
- 도움말 본문만 **보통 굵기(슬롯 5)** 이고 굵은 글씨는 슬롯 0 과 같은 그림이다.
- 줄 간격은 글꼴 높이 14, 문단 사이는 빈 줄 하나(14)다. 작은 글자(슬롯 6, 지식 창 카드 이름 등)는 대조할 캡처가 없어 근거 없는 대응이다.

### 구현

- `Netstorm.Assets`: `BitmapGlyph.OffsetX/OffsetY`(글리프 상자 위치), `BitmapFont.CachePath`(슬롯 → 캐시 파일), 새 [`BitmapFontAtlas`](../dotnetpj/src/Netstorm.Assets/BitmapFontAtlas.cs)(코드 32~255 를 한 장으로 모은다. 칸의 왼쪽 위가 글자를 놓는 점이고 칸 높이는 줄 높이로 같다).
- `Netstorm.Game`: 새 [`OriginalFonts`](../dotnetpj/src/Netstorm.Game/OriginalFonts.cs)가 아틀라스를 글꼴 라이브러리의 정적 글꼴(`StaticSpriteFont`)로 만든다. 그래서 기존의 `MeasureString`·`DrawString` 호출 124군데를 고치지 않고 같은 글꼴 객체만 바꿔 넣는다. 폭은 원본 전진 폭의 합, 높이는 줄 높이로 일정하다.
- `NetstormGame` 이 **언어가 영어일 때만** `OriginalFonts` 를 만들어 `OriginalUiSkin`(본문·보통·제목·작은 글꼴)에 넣고, 게임 화면(맵·메뉴)에 UI 글꼴을 넘긴다. 개발용 화면(상태 화면·스프라이트 브라우저)은 D2Coding 그대로다. 캐시를 읽지 못하면 D2Coding 으로 계속한다.
- 영어 화면에 원본 글꼴에 없는 글자(한글)가 섞인 문자열은 `UiTextExtensions.DrawString` 이 D2Coding 으로 대신 그린다(예: 아직 번역 분기가 없는 알림 "Thunder Cannon 등록"). 이때 폭은 원본 글꼴로 잰 값이라 위치가 조금 어긋날 수 있다.
- 원본 글꼴일 때 함께 바꾼 배치: 줄 높이 14·문단 간격 14(`OriginalUiSkin.LineHeight`·`ParagraphGap`, 한국어는 16·12 그대로), 버튼 글자 위치 `x + 1 + (폭 − 글자 폭) ÷ 2`·`y + (19 − 14) ÷ 2`, 그림자는 불투명 검정, 안내 창 버튼은 **모두 같은 폭 = 가장 긴 문구 + 11**.

### 화면 대조

| 대조 | 결과 |
|---|---|
| 메인 메뉴 버튼 글자(Campaign·Help·Options·Quit) — 원본 캡처와 같은 좌표계 | 가로 +1 을 넣기 전에는 제자리 일치율 0.31~0.40 이고 원본이 1픽셀 오른쪽이었다. 넣은 뒤 제자리 일치율 0.81~0.96, 가장 잘 겹치는 이동량 (0,0) |
| 브리핑 창 — 줄바꿈 | 열 줄 모두 원본과 같은 단어에서 줄이 바뀐다 |
| 브리핑·승리·패배 창 버튼 | 원본 폭 117·99·99 = 가장 긴 문구 106·88·88 + 11, 사이 16. 클론도 같은 식으로 그린다 |
| 한국어 화면(메인·캠페인·미션·브리핑 1-1·1-2) | 글꼴 변경 전후 캡처가 **픽셀 단위로 같다** |

### 글꼴을 맞춘 뒤 드러난 창·메뉴 치수 차이 — 이번에 고치지 않았다

원본 캡처에서 잰 값이다(클라이언트 좌표, 1024×768).

- **테두리가 2픽셀이다.** 안내 창·옵션 목록 모두 밝은 선 2줄·어두운 선 2줄이다. 클론은 1픽셀이다.
- **브리핑 창:** 원본 351×283(클론 350×278). 제목은 창 왼쪽 위에서 (+50, +20), 본문 첫 줄 +55, 버튼 위쪽은 창 아래에서 −35. 클론은 (+48, +20), +54, −35.
- **옵션 목록:** 행 간격 **17**(클론 18), 구분 구간 16(어두운 선 +8, 밝은 선 +9. 클론 12), 폭 166 = 가장 긴 문구 135 + 31(클론 고정 180), 글자 x 는 목록 왼쪽 +15, 목록 왼쪽 끝 x 551(클론 553), 첫 행 글자 위치 (566, 295)(클론 (568, 300)). 원본에는 "Pause - Shift-F9" 행이 있고 `>` 표시가 더 작다.
- **도움말:** 본문 줄 간격이 약 15 다(클론 17).
- **본문의 연속 공백:** 원본은 "shackles.  Who" 처럼 두 칸을 그대로 그린다. 클론은 한 칸으로 줄인다(`TutorialDialogScript` 의 공백 정리).
- **도움말의 색 글씨(`<c>`):** 원본은 보통 굵기에 색만 바꾼다("drag" 노랑, "blue" 파랑). 클론은 `<b>` 와 같은 스타일로 묶어 굵은 흰색으로 그린다.

### 최종 검증

- Release 빌드 **경고 0·오류 0**, 단위 검사 **946개 통과·실패/스킵 0**(Assets 322 + Core 624). 글꼴 검사 6개 추가: 슬롯 경로·치수 4, 18개 캐시의 측정 폭 = 그리기 폭·글리프 상자 범위 1, 아틀라스 칸 배치·글자 픽셀·문자열 폭 1.
- 클론 창(창 모드): 영어 메뉴 → 옵션 → 캠페인 → 브리핑 → 플레이 → Game 메뉴 → 떠나기 9개, 한국어 메뉴 → 1-1 → 1-2 왕복 20개, `clone_test01_smoke` 4개 조합·96개, `clone_combat_smoke` 12개 장면 통과. 영어 캡처(메인·옵션·미션 목록·브리핑·Game 메뉴·지식 창·우클릭 메뉴·도움말)를 직접 확인했다. 캡처는 `extracted/screens/dotnet-x3d-20261010/`.
- `clone_ui_smoke`(전체화면 전환 포함)는 이번에도 돌리지 않았다.

## 8. 창·메뉴 치수의 원본화 (7절 끝의 목록, 같은 날 후속)

사용자 요청 "dotnetpj 다음 순서도 진행"에 따라 7절 끝에 적어 둔 치수 차이를 고쳤다. 방법은 7절과 같다 — `.chfnt` 캐시로 같은 문자열을 합성해 원본 캡처(`screenShots/*.png`) 위에서 글자 위치를 픽셀 단위로 찾고, 행·열의 밝기 변화를 훑어 테두리 선을 찾았다. 값은 모두 1024×768 클라이언트 좌표다. 원본 게임·복사본 실행 없음, 디컴파일 결과를 읽지 않음.

### 잰 값

**목록 메뉴 (옵션·도움말·미션 메뉴 줄의 목록·하위 목록)**

| 항목 | 값 |
|---|---|
| 행 | 목록 맨 위부터 바로 시작, 높이 **17** |
| 구분 구간 | 높이 **16**, 어두운 선 +8·밝은 선 +9 |
| 테두리 | **2픽셀**, 목록 안쪽에 그린다(첫·마지막 행과 겹친다). 밝은 선은 위·왼쪽, 어두운 선은 아래·오른쪽 |
| 글자 | (목록 왼쪽 + 15, 행 위 + 1) |
| 폭 | 가장 긴 문구 + **31** |
| 하위 목록 표시 `>` | 본문과 같은 굵은 글꼴, 문구 끝 + 4 (문구 폭에 10 을 더한 것으로 센다) |
| 하위 목록 | 왼쪽 = 부모 목록의 오른쪽 끝, 첫 행 = 연 행과 같은 높이 |
| 옵션 목록 | 왼쪽 = Options 버튼의 가로 중심(551), 위 = 화면 세로 중심 − 90(294), 166×269, "Pause - Shift-F9" 를 포함해 13행 |
| 도움말 목록 | 왼쪽 = Help 버튼의 가로 중심(630), 위 = 버튼 위쪽(311) |
| 미션 메뉴 줄 | 높이 17, 탭은 x 84 부터 간격 53 고정(글자 +5, y 1). 목록 왼쪽 = 탭 x + 1, 위 = 17. Game 목록 207×100 |

**우클릭 메뉴**

| 항목 | 값 |
|---|---|
| 바깥 위 여백 | 16 |
| 제목 → 첫 정보 줄 | 21, 정보 줄 간격 15 |
| 정보 줄 → 안쪽 판 | 16 (정보 줄이 없으면 15) |
| 안쪽 판 | 좌우 16 들여 놓고 **눌린 모양** 2픽셀 테두리(위·왼쪽 어둡고 아래·오른쪽 밝다) |
| 행 | 17. Storm 아이콘이 있는 행은 **19**이고 글자는 아래에서 2픽셀 띄워 맞춘다 |
| 구분 구간·아래 여백 | 16·16 |
| 하위 메뉴 | 왼쪽 = 부모 안쪽 판 오른쪽 − 16. 위는 간격 16 으로 계산해 첫 행이 연 행과 같은 높이가 된다(정보 줄이 없는 창은 1픽셀 위) |

**안내·브리핑 창**

| 항목 | 값 |
|---|---|
| 테두리 | 2픽셀 |
| 제목·본문 | 제목 (창 x + 여백, 창 y + 20), 본문 첫 줄 창 y + 55, 줄 간격 14, 문단 사이 14 |
| 아래 목록(있을 때) | 위 = 본문 끝 + 25, 폭 = 가장 긴 문구 + 31, 왼쪽 = 창 x + (창 폭 − 목록 폭 + 1) ÷ 2 |
| 버튼 | 위 = 목록이 있으면 목록 끝 + 10, 없으면 본문 끝 + 25. 폭은 모두 같고(가장 긴 문구 + 11) 사이 16, 왼쪽 = 창 x + (창 폭 − 전체 폭 + 1) ÷ 2 |
| 높이 | 버튼 위 + 19 + 16 (제목 있는 창 = 본문 높이 + 115) |
| 폭/여백 | 브리핑 351/50, 승리 375/30, 캠페인 첫 창 238/50, 캠페인 장 창 458/30, TryAgain 275 |

**돌 버튼·색·공백**

- 돌 버튼: 바깥 1픽셀 어두운 선(약 (55,51,54)), 그 안 위·왼쪽 밝은 선 2픽셀(약 (190,170,150)), 오른쪽 어두운 선 2픽셀·아래 1픽셀(약 (85,80,72)), 바탕은 어둡게 하지 않은 메뉴 질감. 글자 x = 버튼 x + 1 + (폭 − 글자 폭) ÷ 2, y = 버튼 y + 2.
- 색: 노란 값·단축키 = 팔레트 197 (242,228,152), `<i>` 강조 = 팔레트 96 (233,214,186).
- 공백: 빈칸만 이어진 구간은 그대로 그린다("holy man!  Now"). 줄바꿈이 낀 공백 구간은 한 칸. `{Text}` 다음 줄이 빈칸으로 시작하면 첫 줄이 3픽셀 밀린다.

### 고친 것

- [`OriginalUiSkin`](../dotnetpj/src/Netstorm.Game/OriginalUiSkin.cs): 행 17·구분 16·글자 들여쓰기 15·폭 여유 31·테두리 2 등의 상수, 2픽셀 `Frame`(눌린 모양 선택), `Separator`, `MenuLabelWidth`·`MenuWidth`·`MenuTextPosition`·`MenuRow`, 색 `ValueColor`·`EmphasisColor`.
- [`MainMenuView`](../dotnetpj/src/Netstorm.Game/MainMenuView.cs): 도움말·옵션 목록을 위 치수로 다시 만들었다. 옵션에 "Pause - Shift-F9" 행을 넣었고(`PauseRequested` → `FortMapViewer.TogglePause`), 하위 목록은 부모 오른쪽에 붙는다. 영어 해상도 문구는 "W by H".
- [`FortMapViewer.MissionMenu`](../dotnetpj/src/Netstorm.Game/FortMapViewer.MissionMenu.cs): 메뉴 줄 높이 17, 탭 간격 53, 목록 폭은 문구에 맞춤.
- [`FortMapViewer.ContextMenu`](../dotnetpj/src/Netstorm.Game/FortMapViewer.ContextMenu.cs): 위 표의 여백·행 높이·눌린 테두리·하위 메뉴 위치. 제단 문구는 아라비아 숫자("Build Level 1 Altar for 500"). 마지막 건설 목록(좁은 창)은 예전 수치 그대로다.
- [`FortMapViewer.TutorialDialog`](../dotnetpj/src/Netstorm.Game/FortMapViewer.TutorialDialog.cs): 본문 시작 55, 창 높이 계산, 여백(브리핑 50·그 밖 30), 폭(승리·패배 375, 제목 있는 창 351, 제목 없는 창 300), 버튼 묶음 가로 위치 +1 반올림, 연속 공백 유지.
- [`TutorialDialogScript`](../dotnetpj/src/Netstorm.Assets/TutorialDialogScript.cs): `NormalizeSpaces`. [`HelpWindow`](../dotnetpj/src/Netstorm.Game/HelpWindow.cs): 값·강조 색을 위 팔레트 색으로.
- 스모크 스크립트의 클릭 좌표: `tools/clone_ui_smoke.ps1`(옵션 줄, 1-2 브리핑 버튼), `tools/clone_buttons_smoke.ps1`, `tools/clone_help_options_smoke.ps1`.

### 화면 대조와 검증 (이 절의 변경까지)

| 대조 | 결과 |
|---|---|
| 옵션 목록 글자 — 원본 캡처와 제자리 | 일치율 0.98~0.999 |
| 해상도 하위 목록 | 0.990 |
| 도움말 목록 | 위치가 같다 |
| 미션 Game 목록 | 0.925 |
| 브리핑 창 제목·본문·버튼 | 창 기준 상대 위치가 같다 |
| 사제 우클릭 메뉴와 Construct 하위 메뉴 | 창 기준 상대 위치가 같다 |

- Release 경고/오류 0, 단위 검사 **946개 통과**(Assets 322 + Core 624).
- 클론 창(창 모드, 숨김 시작) 통과: `clone_buttons_smoke` 4, `clone_construct_smoke` 2, `clone_help_options_smoke` Menu 18·Mission 15, `clone_move_smoke` 2, `clone_production_smoke` 2, `clone_recordplay_details` 10, `clone_test01_smoke` 4개 조합, `clone_test02_smoke` 3, `clone_combat_smoke` 12, 한국어 메뉴 → 1-1 → 1-2 왕복 20, `clone_ui_smoke` 의 옵션 앞부분 20.
- **돌리지 않은 것:** `clone_ui_smoke.ps1` 의 전체화면 전환 줄(`click-center 138,-82`). 좌표만 계산해 넣었다.
- **이번 변경과 무관하게 실패하는 것:** `clone_help_options_smoke.ps1 -Mode Workshop` 이 `construct sunFactory 98,98 → CommandRejected: 건물은 섬 위에만 지을 수 있음` 으로 멈춘다. 이 작업 전 커밋(`59fb516`)을 임시 작업 폴더에 꺼내 돌려도 똑같이 실패했다(임시 폴더는 지웠다). 고치지 않았다.

### 재지 못했거나 고치지 않은 것

- **돌 버튼 모양은 재기만 했다.** `OriginalUiSkin.Button` 은 예전 모양(질감 + 검정 16% 덮기 + 1픽셀 테두리)이다.
- 도움말 본문 줄 간격(약 15, 클론 17), 도움말 `<c>` 색 글씨의 굵기, `{Text}` 뒤 빈칸의 3픽셀 밀림.
- 창 폭·여백을 정하는 규칙(위 표는 창마다 잰 값이다), 옵션 목록의 세로 위치 규칙(잰 값 "중심 − 90" 을 썼다).
- 클론의 TryAgain 창 폭은 300 이다(원본 275).

## 9. 캠페인 목록 (5-6) — 자료 읽기만 만들었고 화면에 연결하지 않았다

사용자 지시(2026-10-10): "코드 수정이 마무리되는 대로 테스트는 진행하지 말고 인수인계". 그래서 이 절의 코드는 **Release 빌드(경고/오류 0)만 확인했고 단위 검사와 클론 창 검사를 돌리지 않았다.** 8절의 검증 수치는 이 절의 코드를 넣기 전의 것이다.

### 원본 스크립트의 구조 (`extracted/tarc/d/` 로 확인)

- `setup.cfg`: `guideSpec = "{DataDir}\offical*.{currentLanguage}"`, `userSpec = "{DataDir}\guide*.{currentLanguage}"`, `H2 = "~w"`, `Text = "~w"`, `CText = "~i"`.
- `tell.english` 의 `[UCampaign]`: 제목 `<h2>{H2}Campaign</h2>`, 본문 "Which section would you like to play?`<br>`", `$Timeout=120,Tell,Blank`, **`$Menu=@{guideSpec},Tell,{cur.file}.overview`**, `$Button=Back,Tell,Blank`.
- `offical*.english`: 머리 값 `title = "…"`. `offical2sep`·`offical5sep` 는 `title="---"` 뿐이라 구분선이 된다. 파일 이름순은 offical1, 2, 2sep, 3, 4, 5, 5sep, 6 이고 제목은 Early Missions, Priest Training, ---, Struggle For Freedom, A Nation Rises, Complete Victory, ---, User Made Campaigns.
- 각 파일의 `[Overview]`: **`$Checked=문구,MissionBegin,미션 파일,완료식,가능식`** 과 `$Button=Back,Tell,UCampaign`. 예: `$Checked=2 Master of Whirligigs,MissionBegin,MasterOfWhirligigs,{DoneMasterOfWhirligigs},{DoneTheWarBegins}`. `offical6` 은 `$Menu=%{userSpec},Tell,{cur.file}.overview`.
- 완료 값은 미션의 `[Succeeded]` 본문에 있는 `<$Config,Done{mission.fileName}=1>` 이 적는다.
- cpppj(`DialogScript.cpp`·`UberGump.cpp` 의 `ExpandMenus`)는 완료식·가능식을 설정 치환 뒤 `ParseLong(값) != 0` 으로 본다. 값이 없는 `{DoneX}` 는 치환되지 않고 남아 0 이다. `offical2` 의 가능식은 `2`~`7` 같은 상수와 여러 `{Done…}` 을 이어 붙인 값도 쓴다.

### 만든 코드 (컴파일만 확인)

- 새 [`CampaignMenu`](../dotnetpj/src/Netstorm.Assets/CampaignMenu.cs): `CampaignMenu.Open(resources, "UCampaign" 또는 "offical3.overview")` → `CampaignPage(Target, Content, Items, TrailingBlankLines)`. `$Menu=@…`/`%…` 는 `GameFileSystem.Find` 로 파일을 찾아 이름순으로 늘어놓고(선택 언어 파일이 없으면 영어), 제목이 `--` 로 시작하면 구분선, `{cur.file}` 은 파일 이름으로 바꾼다. `$Checked` 의 완료식·가능식은 `MissionConditions.Evaluate` 로 본다. **아직 어느 화면도 이 클래스를 쓰지 않는다.**
- [`TutorialDialogScript`](../dotnetpj/src/Netstorm.Assets/TutorialDialogScript.cs): `FindConfigCommands(body)`(본문의 `<$Config,키=값>`), `ConfigHandler`(섹션을 열 때 그 명령을 받는 처리기 — **아직 아무 데도 연결하지 않았다**), `PrepareContent` 를 어셈블리 안에서 쓸 수 있게 함, `StripControls`(제목·본문의 `~w`·`~i`·`~y`·`~r`·`~o`·`~B`·`~E`·`~.`·`~숫자`·`~[…]` 를 지우고 `~~` 는 `~` 로. 색·굵기는 반영하지 않는다).
- **`StripControls` 는 이미 쓰이는 안내·브리핑 창의 글에도 적용된다.** 지금까지는 이런 표시가 글자 그대로 보였다(`bc*76` 미션 등). 기존 단위 검사에 영향이 있는지는 돌려 보지 않아 모른다.

### 남은 일

1. 단위 검사를 돌려 946개가 그대로 통과하는지 본다. 그 뒤 `CampaignMenu`·`FindConfigCommands`·`StripControls` 검사를 더한다(장 목록의 순서와 구분선 두 개, Struggle For Freedom 의 여섯 줄과 `Done…` 값에 따른 완료·가능, 영어 대체).
2. 완료 값 저장: `DisplaySettings` 에 스크립트 값 표(예: `ScriptValues`)를 두어 `settings.json` 에 저장하고, `NetstormGame` 이 그 표를 설정 층으로 올린다(`ConfigStore.FromPairs`). `FortMapViewer` 가 `TutorialDialogScript.ConfigHandler` 를 이 표에 연결한다. 원본은 `options.cfg` 에 적는다 — 저장 위치는 9-1 의 C 판정과 함께 정한다.
3. `MainMenuView` 의 "campaigns"·"missions" 화면을 `CampaignPage` 로 그린다. 치수는 8절 표(첫 창 238/50, 장 창 458/30, 목록 위 = 본문 끝 + 25, 행 17, 구분 16, 완료 표시 점, Back 버튼 = 목록 끝 + 10). 자동화가 쓰는 화면 이름("campaigns"·"missions")은 그대로 둔다.
4. `CampaignAccess`(1-1·1-2 만 열림)와의 관계: 클론이 아직 돌릴 수 없는 미션은 스크립트 조건이 참이어도 막아야 한다(가능 = 스크립트 조건 그리고 `CampaignAccess.IsAvailable`). 한국어 문구는 지금처럼 클론 안의 번역을 쓴다.
5. 스모크 스크립트: 목록 좌표가 바뀌고, 1-2 를 여는 검사는 설정에 `DoneTheWarBegins=1` 을 미리 넣어야 한다.
