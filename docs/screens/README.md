# 원본 스크린샷 목록과 관찰 요약

`screenShots/` 폴더에 있는 원본 게임 캡처의 목록과, 캡처마다 확인한 사실을 정리한다.
캡처는 모두 사용자가 **외부 캡처 도구**로 찍은 것이다. 창 제목 표시줄·창 테두리가 들어 있고, 작업표시줄과 창 바깥 영역이 일부 들어간 파일도 있다.
해상도는 모두 원본 옵션 **1024×768 창 모드**다 (Options → Resolution 에서 1024 by 768 선택 상태).

* 좌표를 잴 때는 파일마다 제목 표시줄(약 31~35px)과 왼쪽 테두리(2~4px) 경계를 먼저 측정해 클라이언트 영역 1024×768 을 잘라낸다. 캡처마다 창 위치와 캡처 범위가 조금씩 다르다.
  측정 예: `Bridge the Gap - Started.png` (4, 32), `mainMenu.png` (2, 33), `help - NetStorm Instructions.png` (4, 34), `Dissolved Alliance! - Started.png` (2, 32)
* 파일 이름 규칙: 메인 메뉴 계열은 `mainMenu - 메뉴 - 하위 메뉴.png`, 미션 화면은 `미션 이름 - 상황.png`
* 상세 분석 노트가 있는 캡처: [bridge-the-gap-start.md](bridge-the-gap-start.md), [the-war-begins-start.md](the-war-begins-start.md), [save-the-island-start.md](save-the-island-start.md)

## 원본 게임 전제 (사용자 확인, 2026-09-28)

* **원본의 해상도는 4:3 세 가지뿐**이다. Options → Resolution 하위 메뉴: `640 by 480`, `800 by 600`, `1024 by 768`. 16:9·16:10 은 지원하지 않았다.
  → 클론의 16:9·16:10 지원(AGENTS.md)은 원본 참고 자료가 없는 **새 기능**이다.
* **카메라는 위치 이동(스크롤)만 된다. 높이·확대 배율은 바꿀 수 없다** (적어도 원본에서는).
  따라서 한 화면에 들어오는 맵 범위는 해상도로만 정해지며, 섬 3개가 있는 Dissolved Alliance! 는 전체를 한 장에 찍을 수 없어 시점을 옮겨 섬마다 따로 찍었다.
* 화면 끝 스크롤은 Options 메뉴의 **`Edge Scroll in Fullscreen`** 선택 항목으로 켜고 끈다 (캡처에서는 켜짐 상태).

## 1. 메인 메뉴 계열 (창 제목 "NetStorm Main Menu")

공통: 구름 배경이 클라이언트 전체를 채우고 640×480 타이틀 그림이 가운데 (192, 144) 에 놓인다. 대화상자는 돌 무늬 배경 + 네 귀퉁이 장식 테두리이며 타이틀 그림 위 가운데에 뜬다.
목록 항목의 동그란 파란 점은 선택·완료 표시, 흐린 글자는 아직 열리지 않은 항목으로 보인다 (완료 변수 `{Done<미션>}` 과 대응 추정 — [mission-script.md](../formats/mission-script.md)).

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
| `mainMenu - Credits.png` | 제작진 | Netstorm 10.72 Patch Credits, Netstorm Original Credits, Cancel → **보유 exe 는 10.72 패치판** |
| `mainMenu - Options.png` | Options 드롭다운 | Direct Draw / Full Screen, Resolution > / Sound On●, Play Music●, Wind Noise●, Speaker Swap L/R, Sound Effect Volume >, Music Volume > / Edge Scroll in Fullscreen●, Auto-Demo●, Tell Tips at Startup●, Pause - Shift-F9 (노란 단축키) / Pass Server Diagnostic (● = 켜짐 표시) |
| `mainMenu - Options - Resolution.png` | 해상도 하위 메뉴 | 640 by 480, 800 by 600, **1024 by 768●** — 4:3 만 존재 |
| `mainMenu - Options - Sound Effect Volume.png` | 효과음 음량 | Volume 1~5, **3●** |
| `mainMenu - Options - Music Volume.png` | 음악 음량 | Volume 1~5, **2●** |
| `mainMenu - Options - Toggle Server Diagnostic Status.png` | 확인 대화상자 | "Toggle Server Diagnostic Status" 설명 + Yes / No |

## 2. 미션 화면 공통 (창 제목 `NetStorm Mission "<미션 제목>"`)

* 왼쪽 사이드바(폭 약 82px): 맨 위 Storm Power 숫자 + 아이콘, 그 아래 다리 조각 6칸(3×2), 사제, 생산 가능 유닛 아이콘 세로 배열, 맨 아래 미니맵.
* **Storm Power 숫자 색**: 여유 있으면 흰색(2650·2950·5650·10650), 부족해지면 노란색(1400·1650), 더 부족하면 빨간색(200·450). 정확한 기준값은 exe 에서 확인 필요 (사용자 설명과 일치).
* 사이드바 유닛 아이콘이 붉게 칠해진 캡처가 있다 (`Playing 9`, Storm Power 200) → 비용 부족 표시로 추정.
* 게임 화면 맨 위에 마우스를 대면 메뉴 막대가 나타난다: Game · View · Options · Players · About (`In game menu`).
* 오브젝트를 누르면 정보 창(컨텍스트 메뉴)이 뜬다: 제목 `<이름> Level I`, `Owner:`·`Alignment:`·`Class:` (값은 노란 글자), 명령 목록, 하위 메뉴(`>`)는 오른쪽에 두 번째 창으로 열린다. 비용·환급액 뒤에는 Storm Power 아이콘이 붙는다.
* 선택한 유닛에는 모서리 괄호 모양 선택 표시와 체력 막대가 보인다 (`Playing 5`). 배치 중인 건물에는 흰 사각 테두리와 비용(예: 400)이 표시된다 (`Playing 2`).

## 3. The War Begins! (캠페인 1-1)

| 파일 | 관찰 |
|---|---|
| `The War Begins! - Briefing.png` | 시작 브리핑: 미션 스크립트 `[A.]` 절 (제목 `<h2>`, 인용문 갈색 글자, 본문). 버튼 **Review Knowledge / Play Mission**. Storm Power 3000. 배경은 이미 맵 화면 |
| `The War Begins! - Playing 1.png` | Storm Power 450(빨강). 적 섬은 **갈색 풀밭 + 빨간 테두리** + 회오리 신전(windVortex), 휘리기그(windFlyer)·포대 다수 |
| `The War Begins! - Playing 2.png` | 1650(노랑). 건물 배치 중 표시(흰 사각형 + 400). 하늘에 반짝이는 별 모양 효과 여러 개 |
| `The War Begins! - Playing 3.png` | 사제 정보 창: High Priest Level I / Owner: You / Alignment: None / Class: High Priest / Construct >, View Netstorm Knowledge / About, Player > → 하위 "Construct Building": Temple >, Workshop >, Build Level 1 Altar for 500 |
| `The War Begins! - High Priest Context Menu.png` | 같은 사제 메뉴를 다른 시점에서 (Storm Power 2800) |
| `The War Begins! - Playing 4.png` | 2950. 자기 섬에 건설 중인 건물의 **어두운 그림자 모양 표시** |
| `The War Begins! - Playing 5.png` | 5650. 적 신전이 없어진 뒤 적 섬이 **초록 풀밭 + 주황 테두리**로 바뀜. 자기 섬에 제단 완성, 선택된 유닛(체력 막대) |
| `The War Begins! - Playing 6.png` | 5650. 5 와 비슷한 시점, 시점 이동 |
| `The War Begins! - Playing 7.png` | 골렘 정보 창: Golem Level I / Owner: You / Alignment: Sun / Class: Ground Transport / Salvage gains 100 / About / Player > |
| `The War Begins! - Playing 8.png` | 제단 위에서 희생 진행 중으로 보이는 장면 |
| `The War Begins! - Victory.png` | 10650. "Success!" 창: `[Succeeded]` 절 문구 + **Leave Missions / Next Mission** |
| `The War Begins! - Sun Workshop Context Menu.png` | Sun Workshop Level I / Alignment: Sun / Class: Production / View Current Production >, Put Knowledge into Production >, Upgrade costs 800 / Salvage gains 200 / About / Player > → 하위 "Knowledge Available — Production Slots Available:": Rain Generator, Sun Cannon, Whirlibase |
| `The War Begins! - Rain Temple Context Menu.png` | Rain Temple / Alignment: Rain / Class: Energy / View Netstorm Knowledge / Salvage gains 1250 / About / Player > |
| `The War Begins! - In game menu.png` | 메뉴 막대 Game 펼침: Review Mission Objectives - F8 / Restart Mission, Leave Mission / Quit Game |
| `The War Begins! - Playing 9.png` | 다른 판: Storm Power 200(빨강), 사이드바 아이콘 붉은색. 적 섬은 초록 풀밭 + 주황 테두리, 적 제단에 자기 사제가 올라가 있음 |
| `The War Begins! - Lose 1.png` | "Failure!" 창: `[Failed]` 절 문구 + **Continue** |
| `The War Begins! - Lose 2.png` | 이어서 "Would you like to attempt the mission **The War Begins!** again?" (미션 이름 노란 글자) + **Replay Mission / Leave Missions** |

## 4. Dissolved Alliance! (캠페인 1-6)

카메라 확대·축소가 없어 섬 3개를 한 화면에 담을 수 없으므로, 시점을 옮겨 섬마다 찍었다.

| 파일 | 관찰 |
|---|---|
| `Dissolved Alliance! - Started.png` | 시작 직후. 왼쪽 **눈 덮인 섬**(플레이어), 가운데 **갈색 풀밭 섬 + 파란 테두리**(회오리 신전), 오른쪽 끝 붉은 영역. Storm Power 4000 = `myStartMoney` |
| `Dissolved Alliance! - Playing 1.png` | 플레이어 섬 전체: 눈·얼음 지면, 파란 테두리, 비 신전(물웅덩이 모양)과 다른 대형 건물, 수집기·유닛들. 오른쪽에 가운데 섬 가장자리 |
| `Dissolved Alliance! - Playing 2.png` | Prince of Thunder 섬 전체: **어두운 회색 돌 지면 + 빨간 테두리**, 번개 신전(전기 구체), 붉은 수정 모양 유닛 다수. 미니맵에 빨간 영역 |

## 5. 여러 캡처에서 나온 추정 (확인 필요)

* **섬 지면 테마 = 영역에 있는 신전의 원소** (기존 추정 강화, [the-war-begins-start.md](the-war-begins-start.md)):
  비 신전 → 눈·얼음 지면, 바람(회오리) 신전 → 갈색 풀밭, 번개 신전 → 어두운 돌 지면, **신전이 없으면 초록 풀밭** (The War Begins! 에서 적 신전이 사라진 뒤 초록으로 바뀜, 멀티플레이 준비 화면의 자기 섬도 초록).
  `isle.type` 타일 이름 `RAGRASS`·`WIGRASS`·`THGRASS`·`rgrass` 와의 대응, 해 원소(sun) 의 처리, 바뀌는 시점(즉시/점진)은 exe·동적 분석으로 확인한다.
* 사이드바 아이콘의 붉은 칠 = 비용 부족, 흐린 목록 글자 = 잠긴 미션.

## 6. 섬 소유권 (사용자 확인, 2026-09-28)

* **섬 테두리 색은 섬의 소유권을 나타낸다.** 캡처에서 플레이어 섬은 파란색, 적 섬은 빨간색, 소유자가 없는 섬은 주황색 테두리다.
* **그 플레이어의 신전(temple)이 섬에 있어야 소유권을 얻는다.** The War Begins! 에서 적 신전이 없어지자 적 섬 테두리가 빨강 → 주황으로 바뀐 것(`Playing 1` → `Playing 5`)이 이 규칙과 맞는다.
* 소유한 섬에서만 건물·유닛을 배치하고 **다리를 짓기 시작**할 수 있다.
* 소유권이 없는 섬은 **지나갈 수는 있지만**, 건물·유닛 배치와 다리 시작은 할 수 없다.
* 규칙 명세: [docs/gameplay/island-ownership.md](../gameplay/island-ownership.md)
