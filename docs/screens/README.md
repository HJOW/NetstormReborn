# 원본 스크린샷 목록과 관찰 요약

`screenShots/` 폴더에 있는 원본 게임 캡처의 목록과, 캡처마다 확인한 사실을 정리한다.
캡처는 모두 사용자가 **외부 캡처 도구**로 찍은 것이다. 창 제목 표시줄·창 테두리가 들어 있고, 작업표시줄과 창 바깥 영역이 일부 들어간 파일도 있다.
해상도는 모두 원본 옵션 **1024×768 창 모드**다 (Options → Resolution 에서 1024 by 768 선택 상태).

* 좌표를 잴 때는 파일마다 제목 표시줄(약 31~35px)과 왼쪽 테두리(2~4px) 경계를 먼저 측정해 클라이언트 영역 1024×768 을 잘라낸다. 캡처마다 창 위치와 캡처 범위가 조금씩 다르다.
  측정 예: `Bridge the Gap - Started.png` (4, 32), `mainMenu.png` (2, 33), `help - NetStorm Instructions.png` (4, 34), `Dissolved Alliance! - Started.png` (2, 32)
* 파일 이름 규칙: 메인 메뉴 계열은 `mainMenu - 메뉴 - 하위 메뉴.png`, 미션 화면은 `미션 이름 - 상황.png`
* 상세 분석 노트가 있는 캡처: [main-menu.md](main-menu.md), [bridge-the-gap-start.md](bridge-the-gap-start.md), [the-war-begins-start.md](the-war-begins-start.md), [save-the-island-start.md](save-the-island-start.md), [dissolved-alliance-start.md](dissolved-alliance-start.md)
* **미션 시작 카메라**: 원본은 플레이어 1 사제 칸 기준점을 클라이언트 약 (525, 393) 에 둔다 (캡처 3장 공통, [dissolved-alliance-start.md](dissolved-alliance-start.md) 2절). 맵 뷰어도 같은 위치로 시작하므로 1024×768 뷰어 캡처와 원본 캡처를 바로 겹쳐 볼 수 있다.

## 원본 게임 전제 (사용자 확인, 2026-09-28)

* **원본의 해상도는 4:3 세 가지뿐**이다. Options → Resolution 하위 메뉴: `640 by 480`, `800 by 600`, `1024 by 768`. 16:9·16:10 은 지원하지 않았다.
  → 클론의 16:9·16:10 지원(AGENTS.md)은 원본 참고 자료가 없는 **새 기능**이다.
* **카메라는 위치 이동(스크롤)만 된다. 높이·확대 배율은 바꿀 수 없다** (적어도 원본에서는).
  따라서 한 화면에 들어오는 맵 범위는 해상도로만 정해지며, 섬 3개가 있는 Dissolved Alliance! 는 전체를 한 장에 찍을 수 없어 시점을 옮겨 섬마다 따로 찍었다.
* 화면 끝 스크롤은 Options 메뉴의 **`Edge Scroll in Fullscreen`** 선택 항목으로 켜고 끈다 (캡처에서는 켜짐 상태).

## 1. 메인 메뉴 계열 (창 제목 "NetStorm Main Menu")

공통: 구름 배경이 클라이언트 전체를 채우고 640×480 타이틀 그림이 가운데 (192, 144) 에 놓인다. 대화상자는 돌 무늬 배경 + 네 귀퉁이 장식 테두리이며 타이틀 그림 위 가운데에 뜬다.
목록 항목의 동그란 파란 점은 선택·완료 표시, 흐린 글자는 아직 열리지 않은 항목으로 보인다 (완료 변수 `{Done<미션>}` 과 대응 추정 — [mission-script.md](../formats/mission-script.md)).
**잠긴(흐린 글자) 미션을 클릭하면 아무 반응이 없다** (사용자 확인, 2026-09-29). 안내·경고 문구나 소리 없이 목록 화면이 그대로 남는다.

| 파일 | 화면 | 관찰 |
|---|---|---|
| `mainMenu.png` | 메인 메뉴 | 버튼 8개 2줄: Campaign · Multiplayer · Demo · Help / Edit · Credits · Options · Quit |
| `mainMenu - Campaign.png` | Campaign 대화상자 | "Which section would you like to play?" — Early Missions, Priest Training / Struggle For Freedom, A Nation Rises, Complete Victory / User Made Campaigns (구분선 3묶음), Back |
| `mainMenu - Campaign - Early Missions.png` | 튜토리얼 목록 | 인용문(갈색 글자)·설명 + 1 Bridge the Gap ~ 6 Raw Power (6개 모두 파란 점), Back |
| `mainMenu - Campaign - Priest Training.png` | 패치판 튜토리얼 | 설명·서명(`-BlueCheese-`, 파란 링크 글자) + Menu, Bridging, Acid Bars, X-Bows, Death Bridge, Man o' Wars, Sun Barricade, **Fury War·Secret 은 흐린 글자** |
| `mainMenu - Campaign - Struggle For Freedom.png` | 캠페인 1 | 1 The War Begins! ~ 6 Dissolved Alliance (6개 파란 점) |
| `mainMenu - Campaign - A Nation Rises.png` | 캠페인 2 | 1~3 파란 점, **4 Guard My Back·5 Surrounded! 는 점 없음** (미완료) |
| `mainMenu - Campaign - Complete Victory.png` | 캠페인 3 | 1 Breaking Through ~ 5 Final Confrontation, 모두 흐린 글자·점 없음 (잠김) |
| `mainMenu - Campaign - User Made Campaigns.png` | 팬 캠페인 | 세로 스크롤 목록 (Legendary Missions Of Ass, Against evil thunder, The Great Battle, Priest Training, Smart Bulfs, Cedz's Campaign, The Fuji Files, The Year of Creation …) |
| `mainMenu - Multiplayer.png` | 멀티플레이 준비 | 창 제목이 `NetStorm Multiplayer Game "<저장 이름>" owner: <플레이어 이름>` 으로 바뀜. 배경은 구름 + **자기 요새 섬**(초록 풀밭, 주황 테두리, 사제·제단). 왼쪽 아래 "Multiplayer Options" 창: Save Game, Player Name(각각 `>` 버튼), Server: Find Local Server, Protocol: TCP/IP, Connect! / Cancel. 사이드바 없음 |
| `mainMenu - Demo.png` | 데모 선택 | The Storm Rages, Demo of "Guard My Back", Demo of "Dissolved Alliance", Cancel |
| `mainMenu - Help.png` | Help 드롭다운 | 버튼 아래 펼침 메뉴: General Help - F1, Technical Help, Version |
| `help - NetStorm Instructions.png` | 도움말 창 | `help.english` 의 `F1Help` 절. 돌 테두리 창(약 450×350)·세로 스크롤바·Back/OK. 메뉴에서는 `<?{global.inMission}>` 의 F8 안내 줄이 숨겨짐. `~lblue~.` 파란 글자, `<c>` 노란 글자 |
| `mainMenu - Edit.png` | 요새 편집 | "Load Battle Map" — 2열 목록(battle4, battle5 … BC1Menu … b0~b14, Battle1~3 …, 대소문자 섞인 파일 이름), Create New Map / Cancel. 타이틀 그림보다 큰 세로 창 |
| `mainMenu - Credits.png` | 제작진 | Netstorm 10.72 Patch Credits, Netstorm Original Credits, Cancel. **10.72는 제작진 명단 제목이며 보유 exe의 버전 표시는 아니다** ([근거](../sources/README.md#2-보유-exe-의-버전)) |
| `mainMenu - Options.png` | Options 드롭다운 | Direct Draw / Full Screen, Resolution > / Sound On●, Play Music●, Wind Noise●, Speaker Swap L/R, Sound Effect Volume >, Music Volume > / Edge Scroll in Fullscreen●, Auto-Demo●, Tell Tips at Startup●, Pause - Shift-F9 (노란 단축키) / Pass Server Diagnostic (● = 켜짐 표시) |
| `mainMenu - Options - Resolution.png` | 해상도 하위 메뉴 | 640 by 480, 800 by 600, **1024 by 768●** — 4:3 만 존재 |
| `mainMenu - Options - Sound Effect Volume.png` | 효과음 음량 | Volume 1~5, **3●** |
| `mainMenu - Options - Music Volume.png` | 음악 음량 | Volume 1~5, **2●** |
| `mainMenu - Options - Toggle Server Diagnostic Status.png` | 확인 대화상자 | "Toggle Server Diagnostic Status" 설명 + Yes / No |

### 1.0 사용자 확인 동작 (2026-09-29)

- **잠긴 미션 클릭 → 아무 반응 없음.** 캠페인 목록에서 흐린 글자(아직 열리지 않은) 미션을 눌러도 화면이 바뀌지 않는다.
- **안내 창에서는 ESC 가 동작하지 않는다.** "Did You Know?" 팁 창, "Not Validated" 창, "NetStorm Demo" 안내 창 같은 안내 창에서 ESC 를 눌러도 아무 일도 일어나지 않는다. 버튼(OK 등)으로만 닫힌다. 아래 1.1절의 도구 관찰(팁 창·도움말 창·데모 안내 창의 ESC 무반응)과 일치한다.
  - 클론 구현: 안내 창은 ESC 로 닫지 않는다. 게임 화면에서 ESC 는 매뉴얼대로 상단 메뉴 막대 표시 전환·들고 있는 다리 반환에 쓰인다 ([조작 요약](../sources/game-manual.md#4-조작-요약-행-404439)). 안내 창이 열린 동안 ESC 가 이 동작으로 이어지는지는 확인하지 않았다.

### 1.1 원본 실행으로 확인한 메뉴 동작 (2026-09-29, 분석 도구 + Wine)

`analyzeManager`로 원본 복사본을 1024×768 창 모드로 실행하고 도구 입력만으로 확인했다(테스트 구간에는 사용자에게 조작하지 않도록 요청함). 증거는 git 제외 경로 `extracted/analyzeManager/20260929T071149864Z-507f034c3833/`(`screens/`, `x11/`, `report.md`). 좌표는 클라이언트 기준이다.

- **시작 순서:** Activision 시작 화면 → 메인 메뉴 위에 "Not Validated" 창, 그 위에 "Did You Know?" 팁 창이 겹쳐 뜬다. 팁 창 OK (661,436) → "Not Validated" 창만 남음 → 그 OK (511,464) → 메인 메뉴.
- **팁 창은 본문 클릭(500,400)과 ESC에 반응하지 않는다**(화면 변화 0). 버튼으로만 닫힌다.
- **메인 메뉴에서 F1 → 도움말 창 "NetStorm Instructions"** 이 열린다. 창 위치 약 (288,40)~(738,390), OK (530,369).
- **도움말 스크롤:** 본문 빈 곳을 위로 100px 드래그하면 내용이 38px 올라갔고, 다시 아래로 100px 드래그하면 맨 위(0)로 돌아왔다. 아래 스크롤 화살표 (717,343) 한 번 클릭은 5px 이동. 38px가 드래그 비율 때문인지 스크롤 끝에 닿아서인지는 아직 구분하지 못했다(작은 드래그로 추가 확인 필요).
- **도움말 창에서 ESC는 창을 닫지 않는다**(1초 뒤 변화 없음).
- **Auto-Demo:** 메인 메뉴에서 마지막 입력(도움말 OK) 후 **약 45초** 동안 입력이 없으면 데모 "The Storm Rages!"가 자동으로 시작된다(창 제목 `NetStorm Demo "The Storm Rages!"`). 시작 직후 "NetStorm Demo" 안내 창(OK 버튼 약 (554,457))이 뜨고, 이 창은 **ESC 두 번에도 닫히지 않았다**. 대기 시간을 마지막 입력부터 세는지, 메뉴 표시부터 세는지는 아직 구분하지 못했다. → 1.2절에서 **마지막 입력 기준**으로 확인.

### 1.2 원본 실행으로 확인한 메뉴·데모 동작 (2026-09-29 저녁, Windows `HJOW-Athlon`)

Windows 10 Pro(지정 예외 시스템 2)에서 `analyzeManager`로 원본 복사본을 1024×768 창 모드로 실행하고 도구 입력만으로 확인했다. 증거는 git 제외 경로 `extracted/analyzeManager/20260929T113907044Z-929836edfdde/`(`screens/`, `events-0001.jsonl`, `report.md`). 시각은 도구 이벤트의 UTC 기록이다. 사용자 요청으로 도중에 중단했다.

- **시작 순서·좌표는 1.1절과 같다:** 팁 창 OK (661,436) → "Not Validated" OK (511,464) → 메인 메뉴.
- **Auto-Demo 대기 시간은 마지막 입력부터 센다(확인).** 마지막 창 OK(11:39:55.466) 뒤 약 37초에 메뉴 바깥 구름 배경 (60,700)을 한 번 클릭했다(화면 변화 0). 데모 화면 전환은 그 클릭(11:40:33.195)으로부터 **45.1초** 뒤(11:41:18.257, 200ms 간격 감시)에 감지되었고, 창 OK로부터는 82.8초였다. 즉 **아무 반응이 없는 빈 곳 클릭도 대기 시간을 처음부터 다시 세게 한다.** → 1.3절에서 마우스 이동만으로도 초기화되고, 대화상자가 닫힌 시점부터도 센다는 것을 확인했다.
- **Auto-Demo 진입 화면:** 구름 배경 + 왼쪽 사이드바(Storm Power 0, 빈 칸)만 있는 화면 가운데에 **"Starting Mission..." 창과 Cancel 버튼**이 뜬다(창 약 (355,345)~(755,423), Cancel 약 (553,398)). 몇 초 뒤 전투 화면으로 바뀌고 창 제목은 `NetStorm Demo "The Storm Rages!"`이다. Cancel은 누르지 않았다.
- **이번 Auto-Demo에서는 "NetStorm Demo" 안내 창이 보이지 않았다.** 데모 전투 화면이 보인 뒤 약 3·7·11·15초에 캡처했지만 안내 창 없이 진행되었다. → 1.3절: 같은 PC에서 안내 창이 **뜬 Auto-Demo도 있었다**(조건 미확인).
- **데모 중 ESC → 화면 맨 위 메뉴 막대가 나타난다(확인).** 안내 창이 없는 상태에서 ESC(80ms)를 누르자 사이드바 오른쪽(x≈84~1024, y≈0~16)에 돌 무늬 막대와 **Game · View · Options · Players · About**가 표시되었다. 글자 위치는 대략 Game x≈90~120, View 143~170, Options 194~238, Players 247~289, About 302~336. 데모 카메라는 계속 스스로 움직인다.
- 이 세션의 두 번째 ESC는 전송 직후 전면 창이 VS Code로 바뀌어 결과로 쓰지 않았다. → 1.3절에서 ESC가 막대를 켜고 끄는 것과 Game 드롭다운을 확인했다.

### 1.3 메인 메뉴 전환·데모·미션·편집기 메뉴 조사 (2026-09-29 밤, Windows `HJOW-Athlon`)

Windows 10 Pro(지정 예외 시스템 2)에서 원본 복사본을 1024×768 창 모드로 실행했다. 사용자는 원격 데스크톱 창을 띄워 두고 조작하지 않았다. 증거는 git 제외 경로 `extracted/analyzeManager/20260929T120526716Z-327849a23cac/`(`screens/`, `events-0001.jsonl`)에 있다. 시각은 도구 이벤트의 UTC 기록이며, 화면 이름 뒤 괄호는 증거 PNG 해시의 앞 8자리다.

**조작 공통**
- 메인 메뉴 버튼(Help 등)은 **커서를 먼저 올린 뒤 클릭해야** 펼침 메뉴가 열렸다. 커서 이동 없이 바로 누른 80ms 클릭에는 반응이 없었다(변화 0). 원인(커서 이동 메시지가 먼저 필요한지, 누름 시간 문제인지)은 구분하지 않았다.
- 펼침 메뉴의 `>` 항목(Resolution, Sound Effect Volume, Music Volume)은 **올려 두기만 해서는 열리지 않고 클릭해야** 오른쪽에 하위 메뉴가 열린다. 예: Resolution 하위 메뉴는 약 (718,311)~(808,362)이다(`5a2e84ad`).
- 목록 항목은 커서를 올리면 어두운 막대로 강조된다(Edit 맵 목록에서 확인).
- 창 모드에서 커서를 화면 맨 위(y=8, y=1)에 2초 두어도 카메라는 움직이지 않았다(위상 상관 이동량 0). 가장자리 스크롤이 창 모드에서 꺼져 있다는 exe 분석([edge-scroll.md](../exe/edge-scroll.md))과 일치한다.

**Auto-Demo 대기 시간 (1.2절 보완, 확정)**
- 대기 시간은 45초이며, **마지막 입력과 대화상자가 닫힌 시점 중 늦은 쪽**부터 센다.
  - **마우스 이동만으로도 초기화된다.** 마지막 클릭(12:33:49.644) 34.1초 뒤에 커서만 옮겼다(12:34:23.758). 데모 전환은 그 이동 **45.1초 뒤**(12:35:08.9)에 감지되었고, 클릭으로부터는 79.3초 뒤였다.
  - **대화상자가 열려 있는 동안은 데모가 시작되지 않고, 닫힌 뒤부터 다시 센다.** Credits 창이 자동으로 넘어가다 닫힌 시각(12:11:55.97)에서 **45.1초 뒤**(12:12:41.1)에 데모가 시작되었다. 마지막 입력(12:11:06)으로부터는 95초 뒤였다.
- 설정 키는 `autoDemo`(켜기/끄기, `setup.cfg` 기본 1)다. 45초 값이 exe 어디에 있는지는 찾지 못했다.
- **로딩 창의 문구는 경우마다 다르다.** "Starting Mission..."(1.2절)이 뜨기도 하고, **"Connecting to Game Server - Countdown 29."**(`9e1ffa6e`, Cancel 버튼)가 뜨기도 했다. 데모도 게임 서버 연결 과정을 거친다.
- **Auto-Demo 안내 창은 뜰 때와 안 뜰 때가 있다(조건 미확인).** 12:30:31 무렵 시작된 Auto-Demo에서는 "NetStorm Demo" 안내 창(`demostormrages.english` `[Demo]`)이 떴다가, 12:31:03 캡처에서는 사라져 있었다(`13679048`→`b4c07401`). 1.2절과 이 세션의 다른 Auto-Demo 두 번에서는 보이지 않았다.

**데모 중 메뉴 막대와 Game 메뉴 (확정)**
- **ESC는 상단 메뉴 막대를 켜고 끈다.** 한 번 누르면 나타나고 다시 누르면 사라진다(막대 영역 캡처 `9390060…`→`36e85d97`→`9390060…`).
- 데모의 Game 메뉴: **Restart Demo / Exit Demo / (구분선) / Quit Game**, 약 (86,16)~(192,84)(`d562b72d`). 항목 y는 약 25·42·75다.
- **Exit Demo를 누르면 확인 창 없이 메인 메뉴로 돌아간다**(돌아온 화면이 기본 메인 메뉴와 같은 해시 `9a5aa24c`).
- Demo 메뉴에서 **Demo of "Guard My Back"**을 고르면 창 제목이 `NetStorm Demo "Guard My Back"`이 되고 **"Demo of NetStorm" 안내 창**이 뜬다(`d3353340`, OK 버튼). 이 창은 입력 없이 **약 15초 뒤 저절로 닫힌다.** 선택 클릭 12:14:47.140 → 첫 캡처 12:14:48.8부터 12:15:02.4까지 그대로 있다가 12:15:03.6에 사라졌다. `guardmyback.english` `[Demo]`의 `$Timeout=15,DoNothing,0`과 일치한다.

**Help 펼침 메뉴**
- 펼침 메뉴는 약 (631,313)~(755,362)이며 항목은 General Help - F1(y≈319), Technical Help(y≈336), Version(y≈353)이다(`c6dc9b1b`).
- **Technical Help는 게임 밖 프로그램을 실행한다.** exe `Clientmain.cpp`(`FUN_00435e40`)가 설정 `techHelpExe = "\help\help.exe"`, `techHelpFile = "\help\readme.hlp"`로 실행한다. Windows 10에서는 이 호출이 Windows 도움말 앱(`HelpPane.exe`)으로 넘어가 **Microsoft Edge 새 창이 앞으로 나왔고**, 게임 창은 포커스를 잃었다. 분석 중에는 누르지 않는다. 클론에서는 게임 안 도움말(readme 내용)로 대체하는 것이 적절하다.
- **Version → "NetStorm" 창**(`37feb1f2`): "Version v10.78", "(c) 1997 Titanic Entertainment, Inc. and Activision Inc.", "10.78 Patch by Ticonderoga Entertainment.", OK. 창은 약 (338,283)~(686,482), OK는 (511,457)이다. `tell.english` `[About]`의 `{version}`·`{gamemaster}.{gameminor}` 치환 결과다 → **보유 exe = 10.78** ([버전 판단](../sources/README.md#2-보유-exe-의-버전)).

**도움말 창 스크롤 (1.1절 보완, 확정)**
- 드래그는 **1:1로 스크롤**된다. 본문 빈 곳 (650,230)→(650,210)을 20px 드래그하자 내용이 정확히 20px 올라갔다.
- 1.1절의 "100px 드래그 → 38px"은 **스크롤 끝**에 닿았기 때문이다. 60px를 더 드래그해도(누적 80px) 38px에서 멈췄고, 마지막 줄 "Titanic were the creators of NetStorm…"이 본문 아래 끝에 온다(`a4e927e6`). 메인 메뉴의 F1 도움말 첫 화면은 최대 38px만 스크롤된다.

**Credits**
- Credits 선택 창(`7642b2d5`): 제목 "NetStorm Credits", 목록 "Netstorm 10.72 Patch Credits"(y≈392) / "Netstorm Original Credits"(y≈409), Cancel (511,437).
- 패치 제작진 첫 쪽(`625d1d11`)에는 More / Back / Cancel 버튼이 있다. **쪽마다 창 크기와 버튼 위치가 다르다.** 둘째 쪽은 버튼 y≈597로 내려간다(`4044b22f`).
- **각 쪽은 입력이 없으면 20초 뒤 다음 쪽으로 넘어가고, 마지막 쪽은 20초 뒤 닫혀 메인 메뉴로 돌아간다.** 측정: 12:11:15.8 → 12:11:35.9 (20.1초), 12:11:35.9 → 12:11:55.97 (20.0초, 메인 메뉴 `9a5aa24c`). `tell.english` `[CreditsNew…]`의 `$Timeout=20`과 일치한다. Original Credits 쪽은 열지 않았다.

**Campaign**
- 기본 메뉴 Campaign 버튼은 `tell.english` **`[UCampaign]`** 화면을 연다("Which section would you like to play?", 6묶음, Back, `1c0a6091`). `[Campaign]` 절은 `<$tell,offical6.Overview>` 한 줄뿐이라 이 화면과 다르다. `[UCampaign]`에는 `$Timeout=120,Tell,Blank`가 있어 120초 동안 입력이 없으면 닫힐 것으로 보인다(미측정).
- Early Missions(`e472f50b`, 기존 캡처와 같음) → **Back**을 누르면 Campaign 선택 창으로 돌아간다(같은 해시 `1c0a6091`).

**미션 진입 흐름 (Early Missions → 1 Bridge the Gap)**
1. 목록 항목 클릭 → "Starting Mission..." 로딩 창(Auto-Demo와 같은 화면 `6d55f55d`).
2. 약 2초 뒤 창 제목이 `NetStorm Mission "Bridge the Gap!"`로 바뀌고 맵 위에 **브리핑 "NetStorm!" 창**(MORE 버튼 (554,513))이 뜬다(`edeb0146`). 조작 안내: 왼쪽 클릭 집기·놓기, 오른쪽 클릭 정보, 전체화면 또는 640×480 창 모드에서 가장자리 스크롤, **ALT를 누르고 있으면 커서 쪽으로 스크롤(모든 모드)**, Escape는 상단 메뉴.
3. **브리핑 창이 떠 있는 동안에도 게임은 진행된다.** 사이드바의 다리 조각이 4개 → 5개 → 6개로 차는 것을 확인했다(`edeb0146`, `d8d49e2b`, `1cceaec0`).
4. **브리핑 창이 열린 동안 ESC는 아무 반응이 없다**(변화 0, 메뉴 막대 안 나타남).
5. MORE → 둘째 쪽 "Scrolling"(F4로 내 섬 복귀, F8 지시 다시 보기, BACK / OK, `abca0904`) → OK로 닫힘.
6. 미션 중 Game 메뉴(`a6078b73`): **Review Mission Objectives - F8 / (구분선) / Restart Mission / Leave Mission / (구분선) / Quit Game**, 약 (86,16)~(291,115).
7. Leave Mission → **"Leave Mission?" 확인 창**(`ce7b6435`): "Do you wish to quit this mission and return to the Main Menu now?", 버튼 **Main Menu (429,443) / Replay Mission (553,443) / Continue Mission (678,443)**. `tell.english` `[ABORT]`(`LeaveBattle,1` / `MissionRestart,0` / `DoNothing,0`)와 같다.
8. Main Menu → **메인 메뉴로 바로 돌아간다**(Campaign 목록이 아님, `9a5aa24c`).

**미션 중 메뉴 막대의 나머지 메뉴** (Bridge the Gap!, 막대 글자 x: Game≈104, View≈155, Options≈216, Players≈268, About≈318)
- **View**(`d6d94176`): Hide buildings - F2, Hide/Show Island Themes - Shift F3, View Home Temple - F4, View Your Priest - F5, View Netstorm Knowledge - F6, Hide Island Ownership - F7, Review Mission Objectives - F8, View Player List - F9.
- **Options**(`af9396d0`): Direct Draw / Full Screen, Resolution > / Sound On●, Play Music●, Wind Noise●, Speaker Swap L/R, Sound Effect Volume >, Music Volume > / Edge Scroll in Fullscreen●, **Cursor Snap To Grid●**, Auto-Demo●, Tell Tips at Startup●, **Pause - Shift-F9** / **Counterclockwise Piece Rotation - C**. 메인 메뉴 Options와 달리 Pass Server Diagnostic이 없다.
- **Players**(`f7452eb4`): `● You >` 한 줄(혼자 하는 미션).
- **About**(`45593f56`): General Help - F1, Technical Help / Version.

**Edit (전투 맵 편집기)**
- Edit → "Load Battle Map" 창(`f5e7201b`): "Select the battle you wish to load from the list below.", 2열 목록(스크롤바 없음), Create New Map / Cancel. 창은 약 (340,96)~(686,672)이다.
- 목록에서 **Battle1**을 고르면 **편집기 모드**로 들어간다. 창 제목은 `NetStorm Editor Game "Battle1"`이다(`fb80d63d`). 사이드바에 다리 조각과 모든 유닛·건물·주문 아이콘이 두 줄로 나오고, Storm Power 숫자 칸은 비어 있다.
- 편집기 메뉴 막대는 **Game · Edit · View · Options · About**이다(Players 없음, 글자 x: Game≈104, Edit≈155, View≈216, Options≈268, About≈318).
  - Game: Test Battle / Main Menu / Quit Game (`cc11419f`)
  - Edit: Save as..., Add Island, Set All Bridge > (`b7dc804d`)
  - View: F2·Shift F3·F4·F5·F7·F9 항목만 있다(F6·F8 없음, `70445aaf`).
  - Options: 미션 중 Options와 비슷하지만 Pause·조각 회전이 없고 Pass Server Diagnostic이 있다(`3bab3662`).
  - About: 미션 중과 같다(`f4462714`).
- 편집기 Game → Main Menu → **"Leave Edit Mode" 창**: "Would you like to save this map now?", Yes / No (553,429) / Cancel. `tell.english`의 `SaveGoMain,1` / `SaveGoMain,0` / `DoNothing,0`과 같다. **No → 메인 메뉴.** 저장하지 않았다.

**메인 메뉴 Options (1절 표 보완)**
- 이번 실행의 메인 메뉴 Options에는 **Pause - Shift-F9 줄이 없었다**(`5a7b78bc`). 항목: Direct Draw / Full Screen, Resolution > / Sound On●, Play Music●, Wind Noise●, Speaker Swap L/R, Sound Effect Volume >, Music Volume > / Edge Scroll in Fullscreen●, Auto-Demo●, Tell Tips at Startup● / Pass Server Diagnostic.
  - 메뉴 생성 코드는 Pause 줄을 `DAT_00594fa4`·`DAT_005c85a4` 중 하나가 켜졌을 때만 넣는다. 사용자 캡처 `mainMenu - Options.png`에 Pause가 있는 것은 이 조건 때문으로 보인다(조건 변수의 의미는 미확인).
- 펼침 메뉴는 버튼 위쪽으로 겹쳐 열린다: 약 (553,296)~(716,545). 항목 y는 302·319 / 352·369·386·403·420·437 / 470·487·504 / 537이다.
- Pass Server Diagnostic → **"Toggle Server Diagnostic Status" 확인 창**(`d76c376a`): 서버에서 로그오프해야 적용된다는 설명과 Yes (487,450) / No (536,450). **No → 메인 메뉴.**
- 켜기/끄기 항목과 해상도·전체화면은 누르지 않았다.

## 2. 미션 화면 공통 (창 제목 `NetStorm Mission "<미션 제목>"`)

* 왼쪽 사이드바(폭 약 82px): 맨 위 Storm Power 숫자 + 아이콘, 그 아래 다리 조각 6칸(3×2), 사제, 생산 가능 유닛 아이콘 세로 배열, 맨 아래 미니맵.
* **Storm Power 숫자 색**: 여유 있으면 흰색(2650·2950·5650·10650), 부족해지면 노란색(1400·1650), 더 부족하면 빨간색(200·450). exe 확인(2026-09-29): **SP ≤ 1000 빨강, 1001~2000 노랑, 2001 이상 흰색** (`Combatgump.cpp` `0043da10`) — 캡처 값 모두 일치.
* 사이드바 유닛 아이콘이 붉게 칠해진 캡처가 있다 (`Playing 9`, Storm Power 200) → 비용 부족 표시로 추정.
* 게임 화면 맨 위에 마우스를 대면 메뉴 막대가 나타난다: Game · View · Options · Players · About (`In game menu`).
* 오브젝트를 **마우스 오른쪽 버튼으로 클릭**하면 정보 창(컨텍스트 메뉴)이 뜬다 (사용자 확인. 워크샵 메뉴로 유닛을 사이드바 덱에 등록해야 생산·건설 가능 — [workshop-deck.md](../gameplay/workshop-deck.md)): 제목 `<이름> Level I`, `Owner:`·`Alignment:`·`Class:` (값은 노란 글자), 명령 목록, 하위 메뉴(`>`)는 오른쪽에 두 번째 창으로 열린다. 비용·환급액 뒤에는 Storm Power 아이콘이 붙는다.
* 선택한 유닛에는 모서리 괄호 모양 선택 표시와 체력 막대가 보인다 (`Playing 5`). 배치 중인 건물에는 흰 사각 테두리와 비용(예: 400)이 표시된다 (`Playing 2`).

## 3. The War Begins! (캠페인 1-1)

| 파일 | 관찰 |
|---|---|
| `The War Begins! - Briefing.png` | 시작 브리핑: 미션 스크립트 `[A.]` 절 (제목 `<h2>`, 인용문 갈색 글자, 본문). 버튼 **Review Knowledge / Play Mission**. Storm Power 3000. 배경은 이미 맵 화면 |
| `The War Begins! - Playing 1.png` | Storm Power 450(빨강). 적 섬은 **갈색 풀밭 + 빨간 테두리** + 회오리 신전(windVortex), 휘리기그(windFlyer)·포대 다수 |
| `The War Begins! - Playing 2.png` | 1650(노랑). 건물 배치 중 표시(흰 사각형 + 400). 하늘의 노란 별 여러 개 = 배치 중인 유닛의 **공격 범위** 표시 (사용자 확인) |
| `The War Begins! - Playing 3.png` | 사제 정보 창: High Priest Level I / Owner: You / Alignment: None / Class: High Priest / Construct >, View Netstorm Knowledge / About, Player > → 하위 "Construct Building": Temple >, Workshop >, Build Level 1 Altar for 500 |
| `The War Begins! - High Priest Context Menu.png` | 같은 사제 메뉴를 다른 시점에서 (Storm Power 2800) |
| `The War Begins! - Playing 4.png` | 2950. 자기 섬에 건설 중인 건물의 **어두운 그림자 모양 표시** |
| `The War Begins! - Playing 5.png` | 5650. 적 신전이 없어진 뒤 적 섬이 **초록 풀밭 + 주황 테두리**로 바뀜. 자기 섬에 제단 완성, 선택된 유닛(체력 막대) |
| `The War Begins! - Playing 6.png` | 5650. 5 와 비슷한 시점, 시점 이동 |
| `The War Begins! - Playing 7.png` | 골렘 정보 창: Golem Level I / Owner: You / Alignment: Sun / Class: Ground Transport / Salvage gains 100 / About / Player > |
| `The War Begins! - Playing 8.png` | 제단 위에서 희생 진행 중으로 보이는 장면 |
| `The War Begins! - Victory.png` | 10650. "Success!" 창: `[Succeeded]` 절 문구 + **Leave Missions / Next Mission** |
| `The War Begins! - Sun Workshop Context Menu.png` | **Sun Workshop 우클릭 메뉴**. 사이드바에 아직 유닛 아이콘이 없음(등록 전). Sun Workshop Level I / Alignment: Sun / Class: Production / View Current Production >, Put Knowledge into Production >, Upgrade costs 800 / Salvage gains 200 / About / Player > → 하위 "Knowledge Available — Production Slots Available:": Rain Generator, Sun Cannon, Whirlibase |
| `The War Begins! - Rain Temple Context Menu.png` | **Rain Temple 우클릭 메뉴**: Rain Temple / Alignment: Rain / Class: Energy / View Netstorm Knowledge / Salvage gains 1250 / About / Player > |
| `The War Begins! - In game menu.png` | 메뉴 막대 Game 펼침: Review Mission Objectives - F8 / Restart Mission, Leave Mission / Quit Game |
| `The War Begins! - Playing 9.png` | 다른 판: Storm Power 200(빨강), 사이드바 아이콘 붉은색. 적 섬은 초록 풀밭 + 주황 테두리, 적 제단에 자기 사제가 올라가 있음 |
| `The War Begins! - Lose 1.png` | "Failure!" 창: `[Failed]` 절 문구 + **Continue** |
| `The War Begins! - Lose 2.png` | 이어서 "Would you like to attempt the mission **The War Begins!** again?" (미션 이름 노란 글자) + **Replay Mission / Leave Missions** |

## 4. 에너지 공급 범위 (2026-09-28 추가)

선택한 템플·Generator 둘레를 **그 원소 모양 아이콘**이 돌며 공급 범위를 보여 준다 (노란 별은 공격 범위). 측정: [elements-energy.md](../gameplay/elements-energy.md) 6절.

| 파일 | 관찰 |
|---|---|
| `Normal Mission - Temple - Generating Range.png` | The War Begins! (일반 미션). Rain Temple 둘레 물방울 아이콘 5개 → 반지름 **30칸** |
| `Normal Mission - Generator - Generating Range.png` | The War Begins!. 배치 중인 Rain Generator(400) 둘레 물방울 6개 → **30칸** (일반 미션에서는 템플·Generator 동일) |
| `Tutorial - Temple - Generating Range.png` | 튜토리얼 2 "Secret Workshop". 배치 중인 Wind Temple(5000) 둘레 조개껍데기 아이콘 7개 → **약 14칸** (튜토리얼 일부만 축소) |

## 5. Dissolved Alliance! (캠페인 1-6)

카메라 확대·축소가 없어 섬 3개를 한 화면에 담을 수 없으므로, 시점을 옮겨 섬마다 찍었다.

| 파일 | 관찰 |
|---|---|
| `Dissolved Alliance! - Started.png` | 시작 직후. 가운데 **갈색 풀밭 섬 + 파란 테두리**(회오리 = 바람 신전, **플레이어**), 왼쪽 **눈 덮인 섬**(Duke of Rain), 오른쪽 끝 붉은 영역(Prince of Thunder). Storm Power 4000 = `myStartMoney`. 맵 뷰어 대조: [dissolved-alliance-start.md](dissolved-alliance-start.md) |
| `Dissolved Alliance! - Playing 1.png` | Duke of Rain(소유자 3) 섬 전체: 눈·얼음 지면, 파란 테두리, 비 신전(물웅덩이 모양)과 다른 대형 건물, 수집기·유닛들. 오른쪽에 가운데 섬 가장자리 (2026-09-28 정정: 처음에 플레이어 섬으로 잘못 적었음) |
| `Dissolved Alliance! - Playing 2.png` | Prince of Thunder 섬 전체: **어두운 회색 돌 지면 + 빨간 테두리**, 번개 신전(전기 구체), 붉은 수정 모양 유닛 다수. 미니맵에 빨간 영역 |

## 6. 여러 캡처에서 나온 추정 (확인 필요)

* **섬 지면 테마 = 영역에 있는 신전의 원소** (기존 추정 강화, [the-war-begins-start.md](the-war-begins-start.md)):
  비 신전 → 눈·얼음 지면, 바람(회오리) 신전 → 갈색 풀밭, 번개 신전 → 어두운 돌 지면, **신전이 없으면 초록 풀밭** (The War Begins! 에서 적 신전이 사라진 뒤 초록으로 바뀜, 멀티플레이 준비 화면의 자기 섬도 초록).
  `isle.type` 타일 이름 `RAGRASS`·`WIGRASS`·`THGRASS`·`rgrass` 와의 대응, 해 원소(sun) 의 처리, 바뀌는 시점(즉시/점진)은 exe·동적 분석으로 확인한다.
* 사이드바 아이콘의 붉은 칠 = 비용 부족, 흐린 목록 글자 = 잠긴 미션.

## 7. 섬 소유권 (사용자 확인, 2026-09-28)

* **섬 테두리 색은 섬의 소유권을 나타낸다.** 캡처에서 플레이어 섬은 파란색, 적 섬은 빨간색, 소유자가 없는 섬은 주황색 테두리다.
* **그 플레이어의 신전(temple)이 섬에 있어야 소유권을 얻는다.** The War Begins! 에서 적 신전이 없어지자 적 섬 테두리가 빨강 → 주황으로 바뀐 것(`Playing 1` → `Playing 5`)이 이 규칙과 맞는다.
* 소유한 섬에서만 건물·유닛을 배치하고 **다리를 짓기 시작**할 수 있다.
* 소유권이 없는 섬은 **지나갈 수는 있지만**, 건물·유닛 배치와 다리 시작은 할 수 없다.
* 규칙 명세: [docs/gameplay/island-ownership.md](../gameplay/island-ownership.md)
