# dotnetpj 복원 기록 — 커서 표·배경 GIF 색·미션 시작 값·원본 영어 글꼴 (2026-10-10)

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

- 5-6 캠페인 목록을 `offical*.english`·`Done...` 값에서 읽기.
- 5-1 전체 월드 표시 목록, 9-1 의도적 차이 판정, 8절의 cpppj 대기 항목.
- 7절 끝의 "창·메뉴 치수" 목록 (글꼴을 맞춘 뒤 드러난 차이).

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
