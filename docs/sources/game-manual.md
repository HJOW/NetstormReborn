# `GAME.HLP` — 게임 매뉴얼 "The Book of Nimbus" 요약

> 원문: `originals/help/GAME.HLP` → helpdeco 추출 `extracted/helpdeco/GAME/GAME.txt` (재현: [hlp.md](../formats/hlp.md)).
> 별도로 추가된 [공식 매뉴얼 PDF](../../originals/help/manual.pdf)의 조작·화면·일부 비용을 선별 대조했다([결과](pdf-manual.md)). 이 문서의 내용과 행 번호는 계속 `GAME.HLP` 추출본 기준이며, PDF 전체와의 대조는 아직 하지 않았다.
> **원판(1997) 매뉴얼**이다. 보유 exe 는 후대 패치판(10.75 이상, [README.md](README.md))이므로 수치·규칙은 `.type`·exe·플레이 자료로 다시 확인한다.
> 행 번호는 `GAME.txt` 기준. 짧은 규칙 요약은 [help-manual.md](../gameplay/help-manual.md), 이 문서는 전체 정리본이다.

## 1. 용어 (행 36~47)

| 용어 | 뜻 |
|---|---|
| Building | 템플·워크샵·알타·아웃포스트. **사제만** 짓는다 |
| Unit | 건물·다리를 뺀, 만들거나 놓는 모든 것 |
| Battle Unit | 유닛 중 공격·방어용. Shooter(포대) / Aerial Attacker(공중 공격) / Blocker(방어) |
| Transport | 결정 수집·사제 포획·주문 학습 유닛. Ground(다리 필요) / Aerial(어디든). 사제도 수송 기능을 하지만 다른 사제는 포획 못 함 |
| Generator | 자기 원소 에너지 1개를 내는 유닛 |
| Energy | Wind·Rain·Thunder. "Sun Energy" = 아무 에너지 |
| Storm Geyser / Crystal / Storm Power | 가이저가 결정을 내고, 결정을 템플(또는 아웃포스트)로 가져가면 Storm Power(돈)가 된다. 결정 1개 = **200 SP** (행 1027) |
| Neutral Island | 멀티플레이 전용 가운데 섬들. 누구나 건설·다리 연결 가능, 아웃포스트를 지으면 소유 |
| Home Island | 시작 섬 |

## 2. 규칙

### 에너지 (행 48~56)
* 템플·Generator 는 각각 에너지 **1개**를 **고정 반지름 원**으로 공급한다. 좌클릭하면 범위가 보인다.
* 원소 유닛은 그 원소 에너지가 필요하다. 필요 에너지가 여러 개면 **서로 다른 공급원 여러 개가 겹치는 곳**에 지어야 한다.
* 에너지는 **건설할 때만** 필요하고, 지은 뒤에는 공급원이 없어도 동작한다.
* 레벨 = 필요 에너지 개수 (행 590). 예시 문장: "Level One Bulf = Thunder 1, Level Two Whirlibase = Sun 2, Level Three Thunder Cannon = Thunder 2 + Sun 1".
  단 Thunder Cannon 은 같은 매뉴얼의 유닛 항목(행 926)에서 **Level II, Thunder 1 + Sun 1** 로 적혀 있어 매뉴얼 안에서도 서로 다르다 (`.type` 은 level 2).

### Storm Power (행 57~78)
* 얻는 방법 4가지: 가이저 결정 수집, 적 유닛·건물 파괴 보상(원가의 **25%**, 멀티 옵션으로 조정), 자기 유닛 회수(건강하면 **25%**, 손상되면 덜), 떨어진 결정 줍기.
* 가이저 기본 **2000 SP** (멀티에서 조정). 수송 유닛은 명령 후 가장 가까운 가이저 ↔ 가장 가까운 템플·아웃포스트를 계속 오간다. 지상 수송은 다리 연결 필요.
* 가이저 우클릭 = 남은 양 표시.
* Production 창 위 숫자가 **1000 미만이면 빨간색** (행 251). 비용보다 SP 가 적은 유닛 아이콘은 빨간색 (행 249).
* Whirligig·Man o' War 처럼 기지가 만드는 비행체는 비용이 없어 파괴 보상도 없다 (행 272).

### 건물 (행 79~106)
* 사제 우클릭 → `Construct>` → Temple> / Workshop> / Altar / Outpost. SP 가 모자라면 비용이 빨간 글자.
* 커서가 빨간 실루엣 → 놓을 수 있으면 원래 색 → 좌클릭하면 사제가 걸어가서 몇 초 뒤 완성. 자리는 비어 있어야 한다.
* **템플의 네 역할**: 섬 소유(적은 그 섬에 아무것도 못 지음) / 사제 체력 재생 / **다리와 골렘 생성**(템플이 없으면 못 만듦) / 결정 → SP 변환.
* 워크샵: 우클릭 → `Put Knowledge into Production>`. **자기 원소 유닛만**, Sun Workshop 은 예외로 다른 원소 Generator 도. 한 섬에 여러 원소 워크샵 가능.
* 생산 창에 뜬 유닛을 좌클릭 → 실루엣 → 좌클릭 배치. 배치 조건: **빈 자리 + SP + 에너지**.
* **유닛은 내 섬, 아군 다리 끝, 중립 섬에만**. **건물은 섬 위에만**(다리 끝 불가) (행 98).
* **Stream of Power (행 99~101)**: 유닛을 놓으면 워크샵(또는 아웃포스트)에서 SP 줄기가 지그재그로 날아가 닿아야 활성화된다. 그래서 **본섬과 다리로 이어지지 않은 곳에는 유닛을 만들 수 없다**.
* 워크샵 생산 칸: **Level I 2칸, II 3칸, III 4칸**. 업그레이드는 2번까지. Sun Workshop 이 업그레이드가 더 싸다. 워크샵이 파괴되면 거기서 생산하던 것을 못 만든다.

### 지식·사제 (행 107~137)
* 사제를 **체력 절반까지** 깎으면 기절하고 보호막(빛나는 고리)을 두른다 → 수송 유닛으로 포획(커서가 손 모양).
* 템플이 있으면 기절한 사제가 에너지를 받아 회복한다 → 빨리 잡아야 한다.
* 포획된 사제는 운반 유닛의 생명력을 빨아들인다. 운반 유닛이 파괴되거나 내려놓으면 **완전히 회복된 채 풀려난다**.
* 발밑 다리가 무너진 사제는 떨어지지 않고 구름 위에 떠 있다 → 밑에 다리를 놓으면 내려앉는다. 공중 수송으로 적의 떠 있는 사제를 잡을 수 있다.
* 희생: 알타(내 본섬·점령한 적 섬·중립 섬에 건설) → 사제를 운반한 수송 유닛을 알타 가운데로 → 내 사제로 알타를 단검 커서로 클릭 → 사제가 알타를 돌며 룬 5개(Wind·Sun·Rain·Thunder·Storm) 새김 → 적 사제 소멸, 지식 획득. 룬 하나마다 받침 다리 하나가 사라진다 (행 996).
* 알타가 희생 중에 파괴되면 사제가 풀려난다. 캠페인은 얻는 지식이 정해져 있고 멀티는 고른다.
* 멀티: 알타 업그레이드(본섬에서만)로 Level II·III 지식을 고를 수 있다. 지식을 고르면 알타는 사라진다.
* 지식을 얻어도 워크샵에 등록해야 만들 수 있다. `View Knowledge (F6)`.

### 다리 — "다리 놓기 십계명" (행 138~159)
1. 가이저·적 섬·중립 섬에 연결하려면 다리 끝이 **정확히** 닿아야 한다 (넘치거나 모자라면 안 됨).
2. 어디에도 연결되지 않은 다리는 금이 가다가 무너진다.
3. 놓으면 생산 창의 그 칸은 **무작위 다른 조각**으로 바로 채워진다.
4. 생산 창의 조각은 처음엔 금 간 상태이고, 쓰지 않으면 곧 굳는다.
5. **Edge Farm(가장자리 초록 식물)** 이 있는 곳에서는 다리를 낼 수 없다.
6. 가이저에 연결하면 그 가이저가 "내 것"이 되어 거기서 다리를 더 낼 수 있다 (적도 수집은 가능하지만 다리는 못 냄).
7. 다리·가이저 소유는 색으로 표시된다.
8. 적 다리의 열린 끝에 연결해 수송 유닛이 걸어갈 수는 있지만, 적 다리에서 새 다리·유닛은 못 만든다.
9. 적 섬·중립 섬에서는 아웃포스트를 지은 뒤에야 다리를 낼 수 있다.
10. 적 유닛에서 다리를 낼 수 없다.
* 조각을 든 채 우클릭 = 회전. 쓸모없는 조각은 다른 곳에 써서 비우라는 힌트.
* 다리로 상대 진로를 막는 전술, 가이저 옆을 지나쳤다가 마지막에 연결하는 요령.

### 전투 (행 14~15, 262~277)
* 전투 유닛은 **알아서 가장 가까운 목표**를 고르고, 잡은 목표가 파괴될 때까지 쏜다 (무적인 목표라도 가장 가까우면 고른다).
* 사격 유닛이 폭발하면 주변 유닛도 피해를 입고 연쇄 폭발한다. 템플도 폭발한다. 회수(salvage)는 폭발하지 않지만 다리에는 폭발과 같은 영향.
* 폭발 근처의 금 간 다리는 파괴되고, 금 안 간 다리는 금이 가거나 끝이 들쭉날쭉해진다.
* 조준은 적 유닛이 **땅에 닿는 지점** 기준.
* 다리의 가지(spur)에 유닛을 놓으면 유닛이 파괴돼도 연결이 유지된다.
* Sun Cannon·Thunder Cannon 은 동서남북 직선으로만 쏜다. Crossbow 는 60도 부채꼴. 선택하면 사거리와 방향이 보인다.

## 3. 화면·메뉴 (행 185~260)

* 메인 메뉴: Campaign(캠페인 4개) · Multiplayer · Demo(컴퓨터 대 컴퓨터 3개) · Help · **Intro**(CD 가 있을 때만) · Credits · Options · Quit.
  (보유 패치판 캡처에는 Intro 대신 **Edit** — [screens/README.md](../screens/README.md))
* 게임 화면 = Battle 창 + Production 창(왼쪽). **Esc** 로 위쪽 메뉴 5개 표시.
  * Game: Review Mission Objectives(F8) · Restart Mission · Leave Mission/Main Menu · (멀티) Leave Battle in Shame/Honorably · Declare a Draw · Quit Game
  * View: Hide Buildings(F2) · Hide Chat(F3) · View Home Temple(F4) · View Your Priest(F5) · View Knowledge(F6) · Friends · **Hide Island Ownership(F7)** · Review Mission Objectives(F8) · View Player List(F9) · Refresh
  * Options: Direct Draw/Full Screen · Adjust Direct Draw(Safe & Slow Mode / Safe & Jumpy Mouse / **Parallax Clouds** / Reduce Tearing) · Window Mode · Sound On · Play Music · Wind Noise · Speaker Swap · 효과음·음악 음량 1~5 · **Edge Scroll in Fullscreen**(전체화면 또는 **640×480 창 모드**에서만) · **Cursor Snap to Grid** · Auto-Demo · Tell Tips at Startup · Pause(Shift-F9)
  * Players · About(General Help F1, Version)
* Production 창: 다리(무료)와 유닛. 맨 위 SP, 맨 아래 미니맵(흰 사각형 = 현재 보이는 영역, 클릭·드래그로 이동).
* 좌클릭으로 선택하면 체력 막대(초록 → 노랑 → 빨강). SP 가 모자란 유닛을 집으려 하면 알림과 함께 SP 숫자가 깜빡인다.

## 4. 조작 요약 (행 404~439)

PDF 매뉴얼의 기본 조작·명령 요약(PDF 22~26·94~95쪽)도 아래 마우스·스크롤·생산 절차와 부합한다. 현재 패치판에 추가·변경된 키와 메뉴는 [패치 이력](patch-history.md#3-입력-키)과 [패치판 캡처](../screens/main-menu.md)를 우선한다([PDF 대조](pdf-manual.md)).

| 입력 | 동작 |
|---|---|
| 커서를 화면 끝에 | 그 방향 스크롤 (전체화면) |
| Alt (또는 휠 버튼) | 커서 쪽으로 스크롤, 가장자리에 가까울수록 빠름 |
| Shift + 스크롤 | 동서남북 직선 스크롤 (창 모드는 Alt-Shift) |
| 좌클릭 (Production 창) | 유닛·다리 집기 |
| 좌클릭 (Battle 창) | 선택·체력 막대 / 든 것 배치 / 선택한 수송 유닛 이동 |
| Shift + 좌클릭 | 수송 유닛 여러 개 선택 |
| 좌클릭 (미니맵) | 그 지점으로 이동, 드래그로 빠른 스크롤 |
| 우클릭 | 대상 메뉴(About 포함) / **든 것 회전**(다리, Thunder Cannon·Wind Tower·Crossbow 같은 방향 유닛) |
| Esc | 메뉴 표시/숨김, 든 다리를 생산 창으로 되돌림 |
| Shift-1~0 / 1~0 | 화면 위치 저장 / 이동 |
| Tab | 최근 만든 유닛 5개 순환 |
| F1~F9 | 도움말 / 건물 숨김 / 채팅 / 본 템플(H) / 사제(P) / 지식 / 소유 표시 / 목표 / 플레이어 목록 |
| Shift-F9 · Pause | 일시정지 |
| Ctrl-F5 · N | 수송 유닛 순환 선택 |
| Q W A S (Z X) | 생산 창 다리 칸 집기 (6칸 멀티에서 Z X 추가) |

패치판에서 추가·변경된 키는 [patch-history.md](patch-history.md) "입력".

## 5. 튜토리얼 안내 (행 290~384, "Early Missions")

| 미션 | 배우는 것 | 매뉴얼 설명 요점 |
|---|---|---|
| 1 Bridge the Gap | 스크롤, 다리 놓기·회전, 가이저 연결, 사제로 결정 수집 | F4 로 템플 이동. 다리는 아군 다리·본섬·연결한 가이저에만. 도중에 가이저가 나타난다. **결정 1개 = 200 SP, 600 SP 모으면 완료** |
| 2 Secret Workshop | 건물 짓기 | 사제로 **Wind Temple** → **Sun Workshop** → 생산 칸 2개 중 첫 칸에 Sun Disc Thrower 등록 → 배치. 배치 3조건(생산 중·SP·에너지) |
| 3 Capture the Priest | 골렘·수집·포획, **Generator 로 "전력선"** | 템플·워크샵이 미리 지어져 있음. 생산 창에 다리 4개 + 골렘. **Wind Temple 범위가 적 섬까지 안 닿으니 Wind Generator 를 줄지어 놓으라** (Generator 자체도 에너지 1개 필요 → 첫 Generator 는 템플 범위 안). 유닛은 다리 위가 아니라 다리 끝 바로 옆 하늘에 놓는다 |
| 4 Tactical Combat | 방향 사격, 희생 | 적 Sun Cannon 은 직선으로만 쏨 → 대각선에서 Disc Thrower 로 공격. 이 미션은 Sun Cannon 지식을 무료로 줌. 알타 짓고 희생 |
| 5 Subtle Defense | 방어탑, 워크샵 업그레이드, 주문 | 적 Thunder Cannon 이 처음부터 템플을 공격 → Stone Tower 로 막기. Level I 워크샵은 2칸이라 업그레이드 또는 워크샵 추가. **적 템플이 있는 섬에는 유닛을 지을 수 없다** → 골렘이 오벨리스크에서 Devastation 주문을 배워 사제를 기절시킴 |
| 6 Raw Power | 원소 유닛 | 골렘 대신 **Air Ship**(Wind 2 + Sun 1): 템플 1개 + Wind Generator 2개 필요. Crossbow(우클릭으로 방향). 적 Sun Barricade 를 먼저 제거 |

※ 범위 축소와 튜토리얼 흐름: 스크립트 `tutorial2.english` 가 "Energy Part 1~3"(템플 범위 보기), `tutorial3.english` 가 "Generators Part 1~"(Generator 로 범위 확장)를 가르친다.
보유 패치판 exe 는 **튜토리얼 2 처리 함수** 첫 단계에서 공급 범위를 14칸으로 줄인다 ([battle-options.md](../exe/battle-options.md) 4절, 캡처 `Tutorial - Temple - Generating Range.png` 도 튜토리얼 2).
튜토리얼 3 처리 함수에는 범위 설정 코드가 없으므로, 매뉴얼이 말하는 튜토리얼 3 의 "범위 부족"은 (a) 전역 옵션 값이 튜토리얼 2 에서 이어지거나 (b) 튜토리얼 3 맵의 섬 거리가 30칸보다 먼 것으로 설명될 수 있다 — 확인 필요.

## 6. 멀티플레이 (행 474~553)

* Multiplayer Options 창: Save Game(섬 이름 — 섬마다 레벨·신뢰도 저장), Player Name, Server(서버 목록·추가: DNS·포트·별명), Find Local Server(LAN), Connect.
* **Serenisphere(Challenge Arena)**: 플레이어 = 떠 있는 작은 섬. 클릭한 곳으로 섬이 이동. 우클릭으로 다른 섬 정보. **Battle Ring 9개**, 링마다 점 8개(최대 8명). 처음 붙은 사람이 **BattleMaster**.
* 섬 **Level** = 가진 지식 양. 모든 지식을 얻은 뒤 희생하면 Level 1 로 돌아가고 **Rank** +1 (Rank 마다 유닛 체력·피해 **+25%**). **Reliability** = 끝낸 게임 / 시작한 게임.
* 안쪽 작은 링 = 레벨별 Zone.
* Ready 체크 → BattleMaster 만 Start Battle. 두 명마다 CD 1장 필요.
* BattleMaster 옵션(원판): Bridge Slots 2/4/6, Unit Rate slow/medium/fast, Generator Range short/normal/long, Kill Reward 0/25/50/100%, SP per Geyser 1000/2000/3000, Excluded Units. 패치판 옵션 표: [battle-options.md](../exe/battle-options.md).
* 중립 섬: 가이저·오벨리스크(주문). 연결은 자유, 다리를 내려면 아웃포스트. 아웃포스트 = 소유 + 적 건설 금지 + 결정 반납처 + **SP 줄기 출발점**. 두 명이 동시에 아웃포스트를 지으면 서로 상쇄돼 아무도 소유하지 못한다(세 번째가 완성될 때까지).
* 채팅: F3 또는 Enter. `이름:` 으로 귓속말, `Allies:` 로 동맹에게. Tab 이름 자동완성.
* 동맹: 유닛 우클릭 → Ally. 동맹은 **내 다리·섬에서 다리를 낼 수 있고** 내 유닛이 공격하지 않는다. 상대가 맞동맹을 안 해도 효과가 있다.
* 승리: 적 사제를 모두 잡아 희생.

## 7. 유닛·주문 핸드북 (행 572~1106)

원판 매뉴얼 수치와 현재 `.type`(패치판) 값을 나란히 적는다. 원판 → 1997-10 README.DOC 변경 → 패치 이력 순서는 [patch-history.md](patch-history.md) 5절.
PDF의 Sun Disc Thrower 200 SP, Ice Tower 400 SP 등 선별한 구버전 비용은 이 표의 `GAME.HLP` 값과 일치한다. 현재 비용은 오른쪽 `.type` 열을 따른다([PDF 쪽별 비용 대조](pdf-manual.md#원판-비용과-보유-패치판의-차이)).

| 유닛 (`.type`) | 매뉴얼: 레벨 · 체력 · 사거리 · 피해 · 비용 · 필요 에너지 | 현재 `.type`: level · hp · range · hpPerSec · cost |
|---|---|---|
| Golem (`sunwalker`) | I · 50 · - · - · 400 · Sun 1 | 1 · 50 · - · - · (cost 없음, 템플이 생성) |
| Balloon (`sunBalloon`) | II · 50 · - · - · 600 · Sun 2 | 2 · 100 · - · - · 600 |
| Sun Disc Thrower (`sunArcher`) | I · 400 · 8 · 10 · 200 · Sun 1 | 1 · 300 · 9 · 15 · 300 |
| Whirlibase (`sunaviary`) | II · 200 · 30 · - · 400 · Sun 2 | 2 · 200 · 30 · - · (cost 없음) |
| Whirligig (`sunFlyer`) | II · 10 · 30 · 10 · - · 없음 | - · 50 · 30 · 10 · - |
| Stone Tower (`sunBlocker`) | I · 2000 · - · - · 300 · Sun 1 | 1 · 2000 · - · - · 400 |
| Sun Cannon (`suncannon`) | I · 600 · 20 · 14 · 400 · Sun 1 | 1 · 600 · 20 · 16 · 400 |
| Sun Barricade (`sunFence`) | III · 800 · 50 · - · 600 · Sun 3 | 1 · 700 · 50 · - · 300 |
| Sun Workshop (`sunFactory`) | 2000 · 800 | 3000 · 800 |
| Wind Generator (`windBattery`) | I · 800 · 400 · Sun 1 | 1 · 700 · 400 |
| Sail Skater (`windwalker`) | I · 100 · 600 · **Wind 1** | 2 · 100 · 600 |
| Air Ship (`windBalloon`) | III · 800 · 1200 · Wind 2 + Sun 1 | 3 · 655 · 1200 |
| Devil Maker (`windaviary`) | III · 600 · 30 · 800 · Wind 2 + Sun 1 | 3 · 400 · 30 · 800 |
| Dust Devil | 무적 · 30 · 20 · 10초 지속, 20초마다 재생성, 금 안 간 다리를 금 가게 함 | (타입 이름 미확인) |
| Crossbow (`windArcher`) | II · 400 · 16 · 30 · 500 · Wind 1 + Sun 1 · 60도 부채꼴 | 2 · 490 · 16 · 25 · 550 |
| Wind Tower (`windBlocker`) | II · 600 · 800 · Wind 1 + Sun 1 · 한쪽 면 무적 | 2 · 600 · 800 |
| Wind/Rain/Thunder Workshop | 4000 · 1000 | 4000 · 1000 |
| 각 Temple (`*Vortex`) | 5000 · 5000 | 5000 · 5000 |
| Rain Generator (`rainBattery`) | I · 800 · 400 · Sun 1 | 1 · 700 · 400 |
| Cloud Floater (`rainBalloon`) | III · 200 · 1000 · Rain 2 + Sun 1 · 명중률 1/20 | 3 · 200 · 1000 |
| Ice Cannon (`raincannon`) | II · 600 · 28 · 20 · 600 · Rain 1 + Sun 1 · 파편 피해 | 2 · 600 · 30 · 20 · 600 |
| Man o' War Pool (`rainaviary`) | II · 200 · 28 · 400 · Rain 1 + Sun 1 | 3 · 200 · 28 · 600 |
| Man o' War (`rainFlyer`) | 200 · 28 · 20 · 1분 수명(파괴 시 연장), 지상 수송 선호 | - · 150 · 28 · 16 |
| Acid Barricade (`rainFence`) | I · 800 · 50 · 400 · **Rain 1** · 사이를 지나는 적 녹임 | 2 · 700 · 45 · 400 |
| Ice Tower (`rainBlocker`) | II · 800 · 400 · Rain 1 + Sun 1 · 파괴돼도 재생 | 2 · 1060 · 800 |
| Crystal Crab (`rainwalker`) | II · 180 · 600 · Rain 1 + Sun 1 · 다른 수송 유닛을 기절시키고 훔침 | 1 · 200 · 500 |
| Thunder Generator (`thunderBattery`) | I · 800 · 400 · Sun 1 | 1 · 700 · 400 |
| Bulf (`bulf`) | I · 1200 · 600 · **Thunder 1** · 가장 튼튼, 느림 | 1 · 800 · 500 |
| Bulwark (`thunderBlocker`) | II · 5000 · 800 · Thunder 1 + Sun 1 · 공중 공격에 무적 | 2 · 3900 · 800 |
| Thunder Cannon (`thundercannon`) | II · 1000 · 42 · 40 · 1200 · Thunder 1 + Sun 1 · 한 방향 | 2 · 1000 · 42 · 40 · 1200 |
| Vander Tower (`thunderArcher`) | III · 400 · 15 · 20 · 600 · Thunder 2 + Sun 1 · 공중 대상 | 3 · 600 · 15 · 35 · 600 |
| Arc Spire (`thunderFence`) | II · 1400 · 35 · 50 · 400 · Thunder 1 + Sun 1 · 적에게만 피해 | 1 · 1000 · 45 · 45 · 400 |
| High Priest | 100 | 100 · range 30 |
| Outpost (`outpost`) | I · 2000 · 600 · Sun 1 | 2000 · 600 |
| Altar (`Altar`) | 1500 · 500 · 에너지 없음 | 1500 · 500 |

기타 핸드북 항목: Residence(옮기거나 파괴·건설 불가), Edge Farm(가장자리 식물, 다리 연결 불가), Storm Geyser(전투 중 새 가이저가 더 생김), Storm Crystal(200 SP).

### 주문 (행 1031~1106)

* 수송 유닛이 **오벨리스크 위로 지나가면** 주문을 배운다. 유닛당 1개, 파괴될 때까지 유지, SP 만 있으면 무제한 사용. 시전 중 잠시 멈춤. 시전자는 영향받지 않음.
* 사제는 우클릭 → Pray 로 **Devastation** 을 얻을 수 있고 수송 유닛보다 싸다 (`.type` `bombSpecialOne` Devastation cost 100 으로 보임).
* 좌클릭 = 주문 범위 표시, 우클릭 = 시전.

| 주문 (`.type`) | 매뉴얼: 범위 · 비용 · 효과 | 현재 `.type`: level · range · cost |
|---|---|---|
| Point Blast (`bombExplodeSmall`) | 1 · 300 · 짧은 범위 피해 | 1 · 1 · 300 |
| Devastation (`bombExplodeMedium`) | 3 · 400 · 중간 범위 피해 | 3 · 3 · 400 |
| Decimation (`bombExplodeLarge`) | 7 · 800 · 넓은 범위 피해 | 1 · 7 · 800 |
| Heal (`bombHeal`) | 9 · 200 · 체력 회복, 마비 해제, 투명 해제 | 2 · 9 · 200 |
| Invisibility (`bombInvisible`) | 7 · 1000 · 투명(목표·집기 불가, 방어탑은 계속 막음) | 1 · 7 · 1000 |
| Paralysis (`bombParalyze`) | 7 · 1000 · 이동·사격 정지, 주문 시전 불가 | 2 · 7 · 1000 |
| Bridge Harden (`bombHardener`) | 7 · 150 · 다리 파괴 불가 | 2 · 7 · 150 |
| Treason (`bombTreason`) | 7 · 2000 · 범위 안 유닛(사제 외 수송 포함) 소유권 획득 | 1 · 7 · 2000 |

패치판 추가 주문(Graviton, Bombardment, Whirlwind/Twister/Vortex, Hydra/Wave/Flood, Thunder Strike, Thunderstorm)은 매뉴얼에 없다 → [patch-history.md](patch-history.md).
