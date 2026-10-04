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

클론 구현: `dotnetpj/src/Netstorm.Assets/MissionScript.cs`

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

### 조건 태그와 본문 준비 (Htmlgump.cpp 분석, 2026-09-28 후속)

클론 구현: `MissionConditions.Filter`·`Evaluate`, `MissionScript.PrepareSection`.
원본 `0046c090`의 `?` 분기와 `0046c810`의 표시 상태를 디컴파일·어셈블리로 확인했다.
`004cf270` 대화상자 경로는 본문 전체를 설정 치환(`00441660`)한 뒤 HTML 처리기로 넘긴다.
조건 피연산자는 `00441740`으로 한 번 더 치환한다.

| 태그 예 | 표시 조건 |
|---|---|
| `<?{ready}>…</?>` | 치환된 값의 10진 정수 접두어가 0이 아님 |
| `<?!{ready}>…</?>` | 위 조건의 부정 |
| `<??left=right>…</?>` | 문자열 완전 일치, 대소문자 구분 |
| `<?g left=right>…</?>` | 왼쪽 정수가 오른쪽보다 큼 |
| `<?ge left=right>…</?>` | 왼쪽 정수가 오른쪽 이상 |
| `<?l left=right>…</?>` | 왼쪽 정수가 오른쪽보다 작음 |
| `<?le left=right>…</?>` | 왼쪽 정수가 오른쪽 이하 |
| `<?l! left=right>…</?>` | 비교 결과의 부정. `<?!l …>`도 같은 의미 |

`g/l/e`는 대소문자를 무시한다. `!`는 비교 접두어 앞 또는 뒤에 둘 수 있으며
두 위치에 모두 있어도 서로 상쇄하지 않는다. 접두어 검사에서는 앞 공백을 건너뛰지 않는다.
따라서 `<?!0>`은 참, `<? !0>`은 숫자가 아닌 `!0`을 읽어 거짓이다.

피연산자 읽기 `00487620`·`00487710`은 앞 공백·탭을 건너뛰고 따옴표를 제거한다.
따옴표 안의 `=`는 구분자가 아니다. 비교 왼쪽은 `=`까지, 오른쪽은 `>`까지 읽는다.
일반 숫자 조건은 `=` 또는 `>`에서 멈춘다. 따옴표를 포함한 입력 한도는 78문자(원본 8bit 문자)다.
클론은 이 한도를 유니코드 문자열의 문자 수에 적용하며 문자열 일치는 ordinal 비교로 확장한다.

숫자는 원본 MSVC `atol`(`004e6656`, 호출 입구 `004e66de`)과 같이 ASCII 공백·부호·10진
숫자 접두어만 읽는다. `true`, 빈 값, 미지정 변수의 `{Not Found:…}`는 0이고 `0x10`도 0이다.
후행 문자와 소수점 뒤는 읽지 않는다. 32비트 넘침은 버린다(`4294967296` → 0).
산술식이나 `&&`·`||`를 해석하는 조건 문법은 확인되지 않았다.

**원본의 중첩 처리는 일반적인 조건 블록 스택이 아니다.** 표시 중 거짓 조건을 만나면 숨기고,
숨김 중에는 다른 여는 조건을 검사하지 않는다. 첫 `</?>`에서 표시를 재개한다.
참인 여는 조건의 범위도 별도로 쌓지 않는다. 아래 원본 동작을 회귀 검사로 고정했다.

```text
<?1>outer<?0>hidden</?>tail</?> → outertail
<?0><?1>hidden</?>tail</?>      → tail
```

`Filter`는 표시되는 일반 텍스트·HTML·인라인 명령을 보존하고 숨긴 본문과 조건 태그를 제거한다.
`PrepareSection`은 섹션을 찾아 이 처리를 거친 뒤 `FindCommands`를 호출한다.
`MissionCommand.Line`은 준비된 본문 안의 줄 번호이며, `$Checked` 인자 안의 조건도 먼저 처리한다.
명령 실행, HTML 스타일 변환, 원본의 줄바꿈→공백 처리는 포함하지 않는다.
불완전한 조건 태그는 오류 없이 제거하며 닫는 조건이 없는 숨김 상태는 본문 끝까지 유지한다.

사용 예:

```csharp
// 자산과 런타임 설정을 구성한다. 실제 게임에서는 준비된 본문을 UI·명령 처리기에 전달한다.
var resources = new GameResources(GameFileSystem.Open(dataDirectory), "english");
LoadedMission? mission = resources.TryLoadMission("tell");
// param.3은 CraftWarning이 비교하는 현재 자원 수다.
resources.Settings.Push(new ConfigText("3 = 3"), "param");
PreparedMissionSection? section = mission?.Script.PrepareSection("CraftWarning", resources.Settings);
```

원본 파일 668개에는 여는 조건 태그 3,088개, 고유 조건식 800개가 있었다.
실제로 비교 태그를 쓰는 `tell.english`의 `CraftWarning`을 자원 수 3·4·5에서 검사했다.
기준값 `ShardtoStone=4`보다 작으면 부족 안내, 4 이상이면 제작 가능 안내만 남는다.
전체 원본 스크립트의 섹션 준비 검사와 숫자·문자열·중첩·불완전 태그·버튼 추출 검사를 추가했다.
새 검사 49개를 포함해 전체 **133개 통과**, 솔루션 빌드 성공(기존 NU1900 조회 경고).
원본 실행 중 UI 결과와 직접 비교하는 동적 검증은 후속 작업이다.

### `[Header]` 키 (빈도순 주요 항목, `aiN` 의 N 은 AI 플레이어 번호 2~8)

| 키 | 예 | 의미 |
|---|---|---|
| `missionType` | `"Tutorial"` | 미션 종류 |
| `missionNumber` | `15` | 번호 |
| `title` | `"Menu"` | 제목 |
| `myStartMoney` | `7000` | 플레이어 시작 Storm Power (게임 내 재화) |
| `myTech` | `"suncannon;sunblocker;..."` 또는 `"all"` | 플레이어가 가진 기술 (`;` 구분, `.type` 이름). `all`은 대소문자와 관계없이 생산 그룹이 있는 타입 전체로 확장한다 |
| `myAllyList` | | 플레이어 동맹 |
| `moreGeysers`, `randGeysers` / `randomGeysers` | `1` | 가이저 추가/무작위 배치 |
| `denySalvage`, `denyAscend`, `techAllowed`, `allowAnyCapture` | | 규칙 제한의 **시작 값**. 튜토리얼 단계 처리가 실행 중에 바꾼다 — 튜토리얼 2 는 `denySalvage = 1` 로 시작해 단계 H 에서 0 으로, `techAllowed` 에 없는 sunFactory 를 단계 B 에서 허용으로 바꾼다 ([mission-header-flags.md](../exe/mission-header-flags.md)) |
| `loadFort` | | 불러올 요새 |
| `aiNName` | `"Juggler of Thunder"` | AI 이름 |
| `aiNTech` | `"thunderVortex;..."` | AI 기술 |
| `aiNStartMoney` | `2600` | AI 시작 자금 |
| `aiNAllyList` | | AI 동맹 |
| `aiNTimeBetweenMoves` | | AI 행동 간격 |
| `aiNCollectors` | `4` | AI 수집 유닛 수 |
| `aiNGeyserAttachments` | `5` | AI 가이저 연결 수 |
| `aiNcolor` | `red` | AI 색상. exe 의 이름 목록 `/none/blue/red/white/green/purple/yellow/lightblue/orange/` 에서 찾은 순번(1~8)이 색 번호가 된다(대소문자 무시, 목록에 없는 `cyan`·`magenta` 는 무시). 기본 색 번호는 플레이어 번호와 같다. AI 플레이어를 만들 때 적용된다 — [근거](../videos/test01-visuals-20261003.md) 3절 |
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

`Tell`(다른 섹션/대화상자 열기), `DoNothing`, `MissionBegin`(미션 시작, 인자 = 미션 파일 이름), `MissionAbort`, `MissionRestart`, `NextMission`, `ShowTechnology`(F6 지식 창 열기, 인자는 무시 — [분석](../exe/show-technology.md)), `GetTechnology`(지식 획득 + `NewTech` 안내, 인자 = 타입 번호), `LeaveBattle`, `ScreenModeChange`, `GoMultiplayer`, `SaveGoMain`, `Config`, `QuitApp`, `TellTip`, `Salvage`, `Capture`, `SetAllBridge`, `ViewPriest`, `URL` 등 약 40종 (+ 멀티플레이 서버 관련 `Chal*`, `LaunchRootServer`, `AccountSetup`).

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

## 커스텀 맵 만들기 (AGENTS.md 2026-10-03, TEST01 예)

AGENTS.md가 정리한 커스텀 맵의 구성이다. 예는 사용자가 만든 `originals/d/TEST01.fort` + `originals/d/TEST01.english`다.

| 파일 | 만드는 방법 | 내용 |
|---|---|---|
| `<이름>.fort` | 게임 안 **Edit 메뉴**로 생성·수정한다 | 섬·다리·건물·유닛 배치([fort.md](fort.md)) |
| `<이름>.english` | 사용자(개발자)가 **텍스트 편집기로 직접 작성**한다 | 사용자·인공지능 구성, 시작 SP, 브리핑 창 내용 |
| `<이름>.<언어 이름>` | `.english`와 같은 방식으로 작성한다 | 다른 언어 지원. 확장자가 언어 이름이다(예: 원본의 `.german`) |

* 제작 방법 참고 자료(AGENTS.md): [방법 1](https://blog.naver.com/hujinone22/221213700658), [방법 2 — Starting Editing](https://netstorm.fandom.com/wiki/Starting_Editing). 두 페이지는 아직 읽어 정리하지 않았다.
* 원본에서 여는 방법: 메인 메뉴 Edit의 목록은 이름순 앞 50개만 보이므로 **Create New Map에 기존 이름을 입력**해 연다. 편집기 Esc → Game → Test Battle로 시험 전투를 한다([녹화 노트](../videos/record-play-edit-test01-20261003.md)).

`TEST01.english`의 주요 내용과 원본 화면에서 확인한 대응이다. "확인"은 2026-10-03 녹화에서 화면으로 본 것이고, 나머지는 키 이름에 따른 해석이다.

| 스크립트 | 의미 | 확인 |
|---|---|---|
| `title="TEST01"` | 미션 제목 | 브리핑 제목으로 표시 — 확인 |
| `myStartMoney=50000` | 사용자 시작 SP | 시험 전투 시작 Storm Power 50000 — 확인 |
| `myTech="all"` | 사용자 시작 기술 전부 습득 | 워크샵의 Knowledge Available에 전체 목록 — 확인. `techAllowed`의 생산 허용 여부와 구분 |
| `myAllyList="2"` | 사용자 동맹 목록 | 미확인 |
| `ai2Name="Luitenent of Wind"`, `ai2color=orange`, `ai3Name="Thunder Demon"`, `ai3color=red` | 인공지능 2·3의 이름과 색 | **시험 전투에서는 색이 적용되지 않았다**: 소유자 2 는 빨강, 소유자 3 은 흰색(기본색 = 플레이어 번호)이었다. 편집기 Test Battle 이 AI 플레이어를 만들지 않기 때문으로 추정 — [화면 요소 대조](../videos/test01-visuals-20261003.md) 3절. 이름 표시는 미확인 |
| `aiNTech="all"`, `aiNStartMoney=0`, `aiNGeyserAttachments=1`, `aiNCollectors=1`, `aiNTimeBetweenMoves=1`, `aiNBridgeDrawRate=1`, `aiNAllyList` | 인공지능의 기술·자금·수집·행동 간격·다리 뽑기 속도·동맹 | 미확인(AI 행동은 분석하지 않음) |
| `ai2Ability="!USE_PRIEST_TO_COLLECT;!BUILD_DAIS"`, `ai3Ability="!USE_PRIEST_TO_COLLECT"` | 인공지능 능력 끄기(`!`) | 미확인 |
| `[A.]`의 `~[IsunBalloon.a1]<h2>TEST01</h2>` + `$Button=EQUIPMENT,ShowTechnology,55` · `$Button=Go!,DoNothing,0` | 시작 브리핑 창의 그림·제목·버튼 | 풍선 그림(sunBalloon 의 기본 프레임 A00 — `a1` 은 없는 프레임이라 기본으로 대체), 그림 옆 제목, 아래 줄의 기울임 글, EQUIPMENT / Go! 버튼 — 확인. `~[I타입.프레임]` 은 글 사이에 스프라이트를 넣는 표기다([설명](../videos/test01-visuals-20261003.md) 8절) |
| `[Succeeded][Ai4PriestDead]`, `[Failed][GoodTeamDead]` | 승리·패배 조건과 그때의 창 | 미확인(녹화에서 끝까지 가지 않음) |

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
