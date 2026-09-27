# 미션 스크립트 (`<이름>.<언어>`)

> 분석 상태: **문법 개요 파악** (통계 기반). 각 명령의 정확한 의미는 4단계(실행 파일 분석)에서 확정한다.
> 통계 대상: 느슨한 `originals/d/` + TAFF 아카이브의 미션·메뉴 스크립트 668개 (`xlat.*`, `config.*` 제외)

## 개요

미션, 캠페인 메뉴, 튜토리얼, 인게임 대화상자를 모두 이 형식으로 기술한다.
확장자가 언어 이름이며(`.english`, `.german` …) 텍스트 인코딩은 Windows-1252, 줄바꿈 CRLF.
미션 맵 데이터는 같은 이름의 `.fort` 파일에 있다 (예: `tutorial1.english` ↔ `tutorial1.fort`).

공식 파일은 TAFF 아카이브 안에 있고, 느슨한 `d/` 폴더의 파일 대부분은 팬 제작(NSHQ 등) 미션이다.
팬 제작 파일에는 오타(`$Buton=`, `MssionBegin`, 대소문자 혼용)가 많으므로 파서는 **대소문자 무시 + 알 수 없는 명령 경고 후 무시**로 동작해야 한다.

## 구조

```
[Header]
키 = 값
...

[섹션이름]
HTML 부분집합 텍스트 + 인라인 명령
$명령=인자,...
```

* `//` 로 시작하는 줄은 주석.
* 섹션은 `[이름]` 으로 시작해 다음 섹션까지 이어진다. 섹션 하나가 대화상자 한 화면에 해당한다.

### 원본 해석 규칙 (Config.cpp 정적 분석, 2026-09-28)

클론 구현: `src/Netstorm.Assets/MissionScript.cs`

* **머리 값**: 원본은 미션 파일을 설정 파일로 읽는다 (`missionSpec` → FUN_00440380). 따라서 `[Header]` 섹션에 한정되지 않고
  **파일 전체에서 처음 나온 `키 = 값`** 을 쓴다 (규칙은 [config.md](config.md) "설정 조회 규칙"). 다른 미션의 머리 값은 `{@미션.키}` 로 읽는다.
* **섹션 찾기** (FUN_0043fe60): 앞 공백·탭을 건너뛴 첫 글자가 `[` 인 줄에서, 줄 안의 모든 `[` 위치마다 `[이름]` 을 대소문자 무시로 비교한다.
  * 그래서 `[Succeeded][BadTeamDead]`, `[Succeeded] [BadTeamDead]`, 오타 `[[Succeeded][BadTeamDead]` 모두 두 이름으로 찾을 수 있다.
  * 비교 길이는 `[이름]` 까지이므로 `[END] `(뒤 공백), `[objective1]1` 도 일치한다. 들여쓴 머리 줄도 찾을 수 있다.
* **섹션 본문** (FUN_00440570): 머리 줄 다음 줄부터 **0열이 `[`** 인 줄 앞까지. 들여쓴 `[..]` 줄은 앞 섹션 본문에 포함된다.
* **줄 명령**: `$이름=` 은 줄 머리뿐 아니라 줄 가운데에도 올 수 있고(`<?{outpost}>$Button=…`, `$Button=A,x,0 $Button=B,y,0`),
  인자 안에 조건 태그 `<?조건>…</?>` 가 섞인다(`$Checked=…,<?!{vn_alive}>1</?>`). 조건을 먼저 평가한 뒤 명령을 해석해야 하므로
  `MissionScript.FindCommands` 는 조건을 평가하지 않은 **어휘 단위**만 돌려준다 (정확한 해석은 9단계 인터프리터).
* 원본 전체 스크립트(느슨한 `d/` + 아카이브, `xlat.*`·`config.*` 제외 668개)를 `MissionScript` 로 예외 없이 읽는 것을 검사한다 (`TextResourceTests.Original_AllMissionScripts`).

### `[Header]` 키 (빈도순 주요 항목, `aiN` 의 N 은 AI 플레이어 번호 2~8)

| 키 | 예 | 의미 |
|---|---|---|
| `missionType` | `"Tutorial"` | 미션 종류 |
| `missionNumber` | `15` | 번호 |
| `title` | `"Menu"` | 제목 |
| `myStartMoney` | `7000` | 플레이어 시작 Storm Power (게임 내 재화) |
| `myTech` | `"suncannon;sunblocker;..."` | 플레이어가 가진 기술 (`;` 구분, `.type` 이름) |
| `myAllyList` | | 플레이어 동맹 |
| `moreGeysers`, `randGeysers` / `randomGeysers` | `1` | 가이저 추가/무작위 배치 |
| `denySalvage`, `denyAscend`, `techAllowed`, `allowAnyCapture` | | 규칙 제한 |
| `loadFort` | | 불러올 요새 |
| `aiNName` | `"Juggler of Thunder"` | AI 이름 |
| `aiNTech` | `"thunderVortex;..."` | AI 기술 |
| `aiNStartMoney` | `2600` | AI 시작 자금 |
| `aiNAllyList` | | AI 동맹 |
| `aiNTimeBetweenMoves` | | AI 행동 간격 |
| `aiNCollectors` | `4` | AI 수집 유닛 수 |
| `aiNGeyserAttachments` | `5` | AI 가이저 연결 수 |
| `aiNcolor` | `red` | AI 색상 |
| `aiNBridgeDrawRate` | `1` | AI 다리 조각 뽑기 속도 |
| `aiNAbility` | `"!USE_PRIEST_TO_COLLECT"` | AI 능력 플래그 (`!` = 끔) |
| `aiNMoneyRechargeRate`, `aiNstuffRefreshRate`, `aiNOff`, `aiNoTemple`, `aiNEnemy` | | 기타 AI 설정 |

### 섹션 이름 (이벤트)

| 섹션 | 발생 시점 (추정) |
|---|---|
| `[A.]`, `[Overview]` | 미션 시작 브리핑 |
| `[@1]`, `[@2]` … | 번호로 호출되는 대화상자 (버튼·F키 등) |
| `[Options]` | 옵션 메뉴 |
| `[Succeeded]` / `[Succeed]` | 승리 |
| `[Failed]` | 패배 |
| `[BadTeamDead]` | 적 전멸 |
| `[aiNPriestDead]`, `[aiNPriestCaptured]`, `[aiNPriestSaved]` | AI N 의 프리스트 사망/포획/구출 |
| `[aiNTempleHalfDead]`, `[aiNTempleDead]` | AI N 의 신전 반파/파괴 |
| `[init]` | 미션 초기화 |
| `[END]` | 종료 |
| 그 외 | 사용자 정의 섹션 (`Tell` 명령으로 이동) |

### 줄 단위 명령 (`$명령=...`)

| 명령 | 빈도 | 형식 / 의미 |
|---|---|---|
| `$Button` | 4420 | `$Button=표시문구,동작,인자` — 버튼 |
| `$Timeout` | 1356 | 대화상자 자동 진행 시간 |
| `$Menu` | 666 | 메뉴 항목 |
| `$Checked` | 493 | `$Checked=표시문구,동작,인자,완료조건,활성조건` — 체크 표시가 붙는 목록(캠페인 메뉴) |
| `$OnExit` | 106 | 대화상자 닫을 때 동작 |
| `$Input` | 9 | 입력란 |

### 버튼 동작 (주요)

`Tell`(다른 섹션/대화상자 열기), `DoNothing`, `MissionBegin`(미션 시작, 인자 = 미션 파일 이름), `MissionAbort`, `MissionRestart`, `NextMission`, `ShowTechnology`, `GetTechnology`, `LeaveBattle`, `ScreenModeChange`, `GoMultiplayer`, `SaveGoMain`, `Config`, `QuitApp`, `TellTip`, `Salvage`, `Capture`, `SetAllBridge`, `ViewPriest`, `URL` 등 약 40종 (+ 멀티플레이 서버 관련 `Chal*`, `LaunchRootServer`, `AccountSetup`).

### 인라인 명령 (`<$명령,인자...>`, 텍스트 안에 삽입)

`Config`(변수 설정, 예: `<$Config,Done{mission.filename}=1>`), `GetTechnology`, `AddRandom`, `PlaySound`, `StartSmoke`, `InflictDamage`, `MoveDialog`, `Tell`, `DestroyAt`, `ViewSpot`, `DumpSquids`, `ChangeMoney`, `MakeMine`, `Salvage`, `CheatAllTechnology`, `SendBucks`, `Warning`, `Meltdown`, `DigUp`, `Lightning` 등 약 30종.

### HTML 부분집합

`<h1>`~`<h4>`, `<p>`, `<br>`, `<i>`, `<b>`, `<a ...>`(링크), `<c>`, `<q>`, `<center>`, `<font>`, `<background="이름">`(배경 그림), `<info>`.

### 변수 치환 `{이름}`

* 설정 변수: `{DoneTutorial1}` 처럼 `options.cfg` 에 저장되는 값 (`<$Config,...>` 로 기록)
* 미션 정보: `{mission.fileName}`, `{param.1}` …
* 용어 치환: `{vortex}` → "Temple" 등 ([xlat.md](xlat.md))
* 전역 설정: `{global.ddFlipping}` 등 (DirectDraw 옵션)
* 팬 미션은 `{sw_var1}`, `{atweapon1}` 같은 사용자 변수를 대량으로 사용한다

## 공식 튜토리얼·캠페인 구성

메뉴 스크립트 `offical1~6.english` (철자 원문 그대로) 기준. 괄호 안은 미션 파일 이름.

| 구분 | 메뉴 제목 | 미션 |
|---|---|---|
| **튜토리얼** | Early Missions | Bridge the Gap!(Tutorial1), Secret Workshop(Tutorial2), Capture The Priest(Tutorial3), Tactical Combat(Tutorial4), Subtle Defense(Tutorial5), Raw Power(Tutorial6) |
| 튜토리얼 (패치판 추가) | Priest Training | Menu, Bridging, Acid Bars, X-Bows, Death Bridge, Man o' Wars, Sun Barricade, Fury War, Secret (`bc1menu76` ~ `bc9ssun_bars76`) |
| 캠페인 | Struggle For Freedom | The War Begins!, Master of Whirligigs, Save the Island!, Fragile Fortune, Thundering Power!, Dissolved Alliance |
| 캠페인 | A Nation Rises | The Noose, Rain Vs. Rain, Run For It!, Guard My Back, Surrounded! |
| 캠페인 | Complete Victory | Breaking Through, To The Rescue!, Vicious, Enemy Territory, Final Confrontation |
| 팬 제작 | User Made Campaigns | |

* **"Early Missions"(Tutorial1~6) 는 캠페인이 아니라 튜토리얼이다** (2026-09-28 사용자 확인. 미션 헤더도 `missionType = "Tutorial"`, `tutorialNumber` 를 가짐).
  Bridge the Gap! 에는 가이저가 없다 (`moreGeysers = 0`) — 원본 화면 관찰: [bridge-the-gap-start.md](../screens/bridge-the-gap-start.md)
* 각 미션은 이전 미션 완료 변수(`{Done<이전 미션>}`)가 참이어야 열린다.
* 주의: 캠페인 미션(예: The War Begins!)도 헤더가 `missionType = "Tutorial"` 이다 → 헤더 값으로 튜토리얼/캠페인을 구분할 수 없다. 구분은 메뉴 스크립트(`offical*.english`) 소속으로 한다.
* 섹션 머리에 이름이 두 개 붙은 경우가 있다: `[Succeeded][BadTeamDead]` (두 이벤트가 같은 내용) → 파서가 지원해야 한다.
* AGENTS.md 의 플레이 영상과의 대응은 아직 확인하지 않았다 (5단계). "튜토리얼" 영상은 Early Missions, "캠페인 1-x / 2-x" 영상은 Struggle For Freedom 이후의 캠페인 미션일 것으로 추정한다.
