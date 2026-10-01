# 네 번째 녹화의 내장 도움말 텍스트

> 2026-10-02. 녹화에서 방문한 **11개 주제**의 전체 내용을 한국어로 옮겨 정리했다.
> 영문 문장·인용문·링크의 정확한 원문은 [내장 도움말 전체 본문](../sources/in-game-help.md)에 모두 보존한다.
> 원본 2,727행·138앵커 정의·113본문 묶음이며, [원문 JSON](../sources/in-game-help-source.json)으로 원래 바이트를 복원할 수 있다.

녹화의 화면과 원본 아카이브 `d/help.english`를 대조했다.
스크롤로 화면 밖에 남은 문단도 원본 텍스트로 보완했다. 해당 문단까지 실제 화면에 모두 보였다고 주장하지 않는다.
게임 내 F1 도움말은 **`help/GAME.HLP`와 별개의 패치판 자료**다.
아래 수치는 도움말의 설명이며 실제 패치 실행 파일의 판정과 어긋날 수 있다.
이 문서의 인용문 번역은 작업용이며 클론의 확정 번역은 아니다.

## 1. 목차 — NetStorm Instructions

녹화 01:42~01:45, 이후 각 주제에서 Back으로 돌아오는 페이지. 원문 `F1Help`.

파란 글자를 클릭하면 더 많은 정보를 얻는다. 도움말 창은 클릭한 채 끌어서 스크롤할 수 있다.
미션 진행 중인 경우에는 **F8로 미션 목표를 다시 볼 수 있다**는 문장이 추가된다.
이번 도움말은 메인 메뉴에서 열었으므로 그 조건부 문장은 화면에 나오지 않는다.

GAME HELP(게임 도움말) 목록 전체:

- Campaign vs. Multiplayer Mode — 캠페인과 멀티플레이 모드
- User Interface — 사용자 인터페이스
- The World of Nimbus — Nimbus의 세계
- The Three Furies — 세 Fury
- High Priests — 고위 사제
- Unit Overview — 유닛 개요
- The Priest's Powers of Construction — 사제의 건설 능력
- How to Capture and Sacrifice — 포획과 희생 방법

GAME SUPPORT(게임 지원) 목록 전체:

- Customer Service / Technical Support — 고객 서비스·기술 지원. 원본 명령 `Tell,TechSupport`.
- Netstorm:HQ's Web Site (Updates and patches) — 업데이트·패치 사이트. 원본 주소 `http://www.netstormhq.com`.
- Ticonderoga Entertainment's Web Site (The creator of the latest patches) — 최근 패치 제작자 사이트. 원본 주소 `http://te.netstormhq.com`.

GAME INFORMATION(게임 정보) 전체:

- Activision's Web Site (Producer of NetStorm, Offers limited support) — 제작·유통과 제한적 지원 안내. 원본 주소 `http://www.activision.com`.
- NetStorm을 만든 Titanic은 안타깝게도 더 이상 존재하지 않는다는 문장.

이 주소들은 **원본 도움말에 적힌 과거 주소**로 보존했으며 이번에 접속하거나 현재 운영 상태를 확인하지 않았다.
하단 공통 버튼은 `Back`(이전 페이지)과 `OK`(닫기)다.

## 2. 캠페인과 멀티플레이 — Campaign vs. Multiplayer Mode

녹화 01:49~01:54, 원문 `campaignHelp / multiplayerHelp`.

NetStorm은 여러 캠페인 미션과 계속 즐길 수 있는 멀티플레이 모드를 제공한다.
Early Missions를 플레이하면 게임의 기본 동작을 배울 수 있다.

**Campaign:** 미션마다 정해진 Knowledge를 가지고 시작한다.
보통 적 High Priest를 Furies에게 희생해야 승리하며, 적을 제압하면 다음 미션으로 넘어간다.

**Multiplayer:** Nimbians는 단순한 Knowledge Profile로 시작한다.
적 사제를 희생할 때마다 새 지식을 얻어 전투 능력을 향상할 수 있다.
캠페인과 다르게 작은 섬을 여럿 장악하는 맵이며, 원래 캠페인에 없는 Outpost가 승리의 중요한 요소다.
Altar도 다르게 작동하므로 Zones와 Battle Master를 알아야 한다.
다른 플레이어와 채팅하려면 `.format` 명령을 참고할 수 있다.

본문 링크: High Priest / Furies / Multiplayer Quick Start / Sacrifice / Knowledge / Outpost /
Altar / Zones / Battle Master / Chat / .format.

## 3. 사용자 인터페이스 — NetStorm User Interface

녹화 01:57~02:23, 원문 `interfaceHelp`.

유닛을 왼쪽 클릭하면 선택하고, 창이나 유닛을 오른쪽 클릭하면 명령 메뉴가 열린다.
Alt를 누르면 커서 방향으로 화면이 이동한다.
전체화면 또는 바탕화면이 640×480인 창 모드에서는 커서를 화면 끝으로 옮기는 것으로 스크롤한다.
Microsoft IntelliMouse의 가운데 Wheel 버튼을 누르는 것은 Alt를 누르는 것과 같다고 안내한다.
Escape는 화면 위의 옵션 메뉴를 표시한다.

| 표시된 키 | 본문 설명 |
|---|---|
| F1 | 일반 도움말 |
| F2 | 뒤를 볼 수 있게 건물 숨김 |
| F3 | 채팅 창 표시·숨김 |
| F4 또는 H | 자기 Home Temple로 이동 |
| F5 | 자기 사제로 이동 |
| F6 | 보유 Knowledge 표시 |
| F7 | 섬 색 표시 전환 |
| F8 | 미션 목표 다시 보기 |
| F9 | 온라인 플레이어 표시(멀티플레이) |
| Shift-F3 | 섬 테마 표시·숨김 |
| Shift-F7 | 스크린샷 촬영 |
| Shift-F9 또는 Pause | 게임 일시정지 |
| Tab | 최근 배치한 유닛 다섯 개 순환 |
| Ctrl-F5 또는 N | 수송 유닛들을 차례로 선택 |
| T | 게임 타이머 표시·숨김 |
| P 또는 R | 사제 선택. 두 번 누르면 이동하며 선택 |
| D | 생산 창에서 마지막으로 선택한 유닛을 다시 집기 |
| C | 유닛과 다리의 회전 방식 변경. 원문 철자 `roate`도 보존 |
| U | 가장 최근 잃은 유닛 위치로 이동 |
| E | 마지막으로 사용 가능한 다리 칸 선택 |
| Q W / A S / Z X | 다리 조각을 직접 커서에 집기. Z X는 다리 여섯 칸인 게임에서만 해당 |
| Shift + 0~9 | 현재 화면 위치 저장 |
| 0~9 | 해당 번호로 저장한 화면 위치로 이동 |

여러 단축키는 채팅 창을 숨겨야 동작한다.
에너지 기호 설명에는 Wind / Rain / Thunder / Sun의 네 기호가 나온다.
창이나 유닛의 기능이 궁금하면 우클릭 → About를 선택한다.
초보자에게 도움말이 혼란스러울 수 있으므로 데모를 보고 Early Missions를 플레이하라고 권한다.
본문 링크: Chat Window / Multiplayer / Production Window / Early Missions.
이 녹화에서 직접 누른 키는 Escape뿐이며 위 표 전체를 키로 시험한 기록은 아니다.

## 4. 다리 — Bridge

녹화 01:08~01:10의 About 창, 원문 `bridgeType`.

인용문: “우리는 평화를 위해 다리를 짓지 않는다.” — General Jan Masaryk.

Pyrosphere, 즉 전투에서는 섬 가장자리에서 다리를 뻗는다.
신전을 지으면 화면 왼쪽 위 Production Window에 다리 조각이 생긴다.
처음에는 금 간 조각이지만 몇 초 동안 창에 남아 있으면 더 단단해진다.
금 간 조각과 굳은 조각을 쓰는 차이는 전투에서 실험하면 알 수 있다고 설명한다.

생산 창에서 조각을 각각 고르는 대신 Q/W/A/S/Z/X/E를 쓸 수 있다.
각 키는 인접한 다리 칸에 대응하고, E는 사용 가능한 조각을 집는다.
우클릭은 90도 회전이며 **C는 역방향 회전을 활성화**한다고 이 페이지는 추가로 설명한다.
놓을 수 없는 곳의 다리 조각은 빨간색이 된다.

다리로 자원·적 섬에 연결하고 유닛을 놓을 영역을 만든다.
고정 유닛은 다리 바로 위가 아니라 **다리 끝**에 놓는다.
자기 섬의 Temple부터 놓으려는 다리까지 연결이 계속 이어져 있어야 한다.
Edge Farm이 가로막으면 그 자리에 섬과 다리를 연결할 수 없다.
멀티플레이에서는 Generator를 우클릭 → Meltdown하여 적 유닛에 작은 피해를 주거나 다리를 끊을 수 있다고 덧붙인다.

본문 링크: Pyrosphere / Production Window / Units / Temple / Edge Farm / Generators.

## 5. Nimbus

녹화 02:24~02:27, 원문 `sphereHelp`.

Nimbus는 Serenisphere / Pyrosphere / Deusphere의 세 층으로 이루어진 세계다.
Serenisphere에서는 온라인 상대를 만나고, Pyrosphere에서는 적 High Priest를 포획하기 위해 싸운다.
Deusphere에서는 Furies가 끝없이 싸우며 전투를 위한 Storm Geysers를 Pyrosphere로 던져 올린다.
Serenisphere를 더 알아보라는 링크가 있다.

본문 링크: Serenisphere / Pyrosphere / Deusphere / High Priests / Storm Geysers.
원본의 Geyser 링크에는 `href` 대신 `herf`라고 쓴 오탈자가 있다.

## 6. Serenisphere

녹화 02:27~02:32, 원문 `serenisphereHelp`.

Serenisphere는 Nimbus의 가장 바깥층이며, 아래 Pyrosphere에서 싸울 동료 플레이어를 만나는 평화로운 장소다.
Challenge Ring을 우클릭하면 선택한 위치로 전투 참가한다.
아래에는 Zones가 보이며 처음에는 Knowledge 수준과 대략 맞는 Zone에서 시작한다.
다른 곳에서 전투를 시작하고 싶으면 Zone을 우클릭해 이동할 수 있다.
다른 곳을 좌클릭하여 자기 섬을 하늘에서 이동시킬 수도 있다.

첫 참가자는 BattleMaster다. 전투 규칙을 정하고 다른 참가자를 Ring에서 추방할 수 있다.
추방당한 참가자는 BattleMaster가 떠날 때까지 그 Ring에 다시 들어올 수 없다.
모두 참가하고 준비되면 BattleMaster가 Start Battle을 눌러 Pyrosphere로 내려간다.
시작 전에는 모든 플레이어가 이름 옆 색 상자를 클릭해야 하고, 전투의 자기 색은 그 상자 색과 같다.
Pyrosphere를 더 알아보라는 링크가 있다.

본문 링크: Nimbus / Challenge Rings / Zones / BattleMaster / Pyrosphere.
준비 확인과 색 상자 문장 등 화면 밖에 남은 부분도 원문으로 보완했다.

## 7. 세 Fury — The Three Furies of Nimbus

녹화 02:36~03:07, 원문 `themeHelp`. 이 앵커는 원본에 두 번 정의되어 두 본문을 모두 보존했다.

Nimbus 중심에는 끊임없는 폭풍이 일며 Wind / Rain / Thunder의 세 Fury가 세계를 지배하려고 끝없이 싸운다.
그 싸움이 Hidden Planet의 큰 조각들을 하늘로 뜯어 올리고, 새 섬 위에 플레이어의 백성 Nimbians가 산다.

죽음과 파괴는 Furies를 기쁘게 하므로 적 유닛을 파괴하면 그 유닛 Storm Power 값의 4분의 1을 받는다.
단, 개별 Aerial Attacker는 만드는 데 별도 SP가 들지 않으므로 예외다.
사제 희생은 더욱 기쁘게 하여 Battle Knowledge를 선물하게 한다.
각 Fury는 Wind / Rain / Thunder의 에너지 기호를 가지고 있다.

Sun이라는 공통 Knowledge도 있으며 Sun 기호로 나타낸다.
Sun 유닛은 어떤 에너지도 쓸 수 있지만 비교적 약하다고 설명한다.
각 Fury의 전문 Knowledge는 보통 그 Fury의 에너지를 요구한다.

| 원소 | 그림과 함께 적힌 구성 |
|---|---|
| Wind | 1) Wind를 숭배하는 Temple, 2) Wind 유닛을 생산하는 Workshop, 3) Wind Energy를 내는 Wind Generator, 4) 모든 Wind 유닛 |
| Rain | 1) Rain을 숭배하는 Temple, 2) Rain 유닛을 생산하는 Workshop, 3) Rain Energy를 내는 Rain Generator, 4) 모든 Rain 유닛 |
| Thunder | 1) Thunder를 숭배하는 Temple, 2) Thunder 유닛을 생산하는 Workshop, 3) Thunder Energy를 내는 Thunder Generator, 4) 모든 Thunder 유닛 |
| Sun | 대응 Fury는 없지만 모든 Fury가 제공. 1) 무원소 Sun 유닛을 생산하는 Workshop, 2) 모든 Sun 유닛 |

끝에는 새 지식을 위한 사제 희생 안내 링크가 있다.
본문 링크: Nimbus / Battle Units / Sacrifice / Knowledge / Energy 및 각 원소 Temple / Workshop / Generator.

## 8. 고위 사제 — High Priest

녹화 03:11~03:27, 원문 `priestType`. 화면에 표시된 고정 머리 글자 전체:

```text
High Priest
Alignment: None
Class: High Priest
Hits: 100
Range: 30
Damage: n/a
Cost in Storm Power: n/a
Energy to Build: None
```

이 머리는 `help.english`의 `<info>`를 처리할 때 게임이 덧붙이는 정보다.
본문과 따로 고정되므로 아래를 스크롤해도 남는다.

인용문: “너희 중 누가 Fury와 마주할 것인가? 누가 거룩함과 두려움을 알 것인가? 그리고 누가 파괴될 것인가?” — Book of Nimbus.

High Priest는 Furies의 Knowledge와 힘을 담는 궁극적인 존재다.
특수 건물과 Altar를 짓고 다른 사제를 Furies에게 희생한다.
사제는 Altar에서만 죽으며 전투 유닛에게는 죽지 않고 무력화될 뿐이다.
Temple이 있으면 전투 중 상처가 서서히 회복된다.

멀티플레이에서는 Pray로 Devastation Spell을 얻어 100 SP로 사용할 수 있다.
우클릭 → Pray를 선택해 작동 방식을 보라고 안내한다.
F5는 사제로 화면 이동, P 또는 R 한 번은 선택, 두 번은 이동하면서 선택이다.
끝에는 사제의 건설 능력과 새 지식을 위한 희생 링크가 있다.

본문 링크: Furies / Temple / Devastation Spell / Construction / Sacrifice.

## 9. 유닛 개요 — Battle Units

녹화 03:30~03:52, 원문 `unitHelp`의 **첫 정의(937행)**와 화면 목록이 일치한다.
전체 원문에는 레벨·목록이 다른 뒤 정의(2064행)도 있다.
예를 들어 첫 정의는 Sun Barricade Level One과 Whirlibase Level Two,
뒤 정의는 Sun Barricade Level Three이며 Whirlibase 행이 없다.
아래는 녹화 화면에 대응하는 첫 정의를 옮긴 표이며, 모든 경우의 실행 파일 레벨 판정을 확정하는 표가 아니다.

Battle Units는 Sun / Wind / Rain / Thunder의 네 종류다.
필요하면 Salvage할 수 있으며 공격에는 공통 Tactics가 쓰인다고 설명한다.

| 원소 | 표의 레벨 | 모든 유닛과 역할 |
|---|---|---|
| Sun | One | Golem — Ground Transport; Sun Disc Thrower — Shooter; Sun Cannon — Shooter; Stone Tower — Defense; Sun Barricade — Defense |
| Sun | Two | Whirligig — Aerial Attack; Whirlibase — Aerial Attack Base; Balloon — Aerial Transport |
| Wind | One | Wind Generator — Source of Energy |
| Wind | Two | Sail Skater — Ground Transport; Crossbow — Shooter; Wind Tower — Defense |
| Wind | Three | Dust Devil — Aerial Attack; Devil Maker — Aerial Attack Base; Air Ship — Aerial Transport |
| Rain | One | Rain Generator — Source of Energy; Crystal Crab — Ground Transport |
| Rain | Two | Ice Cannon — Shooter; Ice Tower — Defense; Acid barricade — Defense |
| Rain | Three | Man o' War — Aerial Attack; Man o' War Pool — Aerial Attack Base; Cloud Floater — Aerial Transport |
| Thunder | One | Thunder Generator — Source of Energy; Bulf — Ground Transport; Arc Spire — Defense |
| Thunder | Two | Thunder Cannon — Shooter; Bulwark — Defense |
| Thunder | Three | Vander Tower — Shooter |

역할은 각각 지상 수송 / 포대 / 방어 / 공중 공격 / 공중 공격 기지 / 공중 수송 / 에너지 공급이다.
각 유닛 이름은 해당 상세 페이지 링크이며 끝에는 Unit Information 읽는 법과 다른 유닛·원소 안내 링크가 있다.
개별 유닛 상세 페이지도 [전체 본문](../sources/in-game-help.md)에 포함하지만 이번 녹화에서 각각 열지는 않았다.

## 10. 사제의 건설 능력 — The Priest's Powers of Construction

녹화 03:55~03:58 (`g2360`), 원문 `vesselPriestHelp`.

High Priest로 Temple / Workshop / Altar를 지으며 멀티플레이에서는 Outpost도 짓는다.
자기 첫 섬을 포함한 큰 섬에서 건설할 수 있다.
사제를 우클릭해 알맞은 항목을 선택하면 건물이 커서에 붙어 배치할 준비가 된다.
사제가 건설 위치로 걸어갈 수 있어야 하며 배치하면 그 자리로 가서 공사를 시작한다.
무력화된 사제는 건물을 짓지 못한다.

사제는 Golem처럼 Transport 역할도 하여 주문을 얻어 사용하거나 Storm Crystal을 운반한다.
발밑 다리가 위험해지면 더 안전한 곳으로 움직이려고 한다.
새 지식을 위한 사제 희생 링크로 이어진다.

본문 링크: Temple / Workshop / Altar / Multiplayer / Outpost / Immobilize /
Golem / Transport / Spells / Storm Crystal / Bridge / Sacrifice.

## 11. 포획과 희생 — How To Capture and Sacrifice

녹화 04:00~04:11, 원문 `sacrificeOutline`.

인용문: “열에 들뜬 공포. 날을 세운 뼈. 피 한 방울. 비명. 신음.”

전투에서 자기 Transport로 적 High Priest를 포획해 Altar에서 희생한다.
멀티플레이에서는 새 Knowledge를 얻고, 캠페인에서는 미션 승리를 위한 행동이다.

전체 절차:

1. 사제를 무력화한다.
2. 적 사제를 자기 Altar로 데려간다.
3. 자기 사제를 Altar로 보낸다.
4. 희생을 수행한다.

자기 사제가 무력화되거나 죽으면 희생을 수행할 수 없다고 강조한다.
마지막에는 사제의 건설 능력을 더 알아보라는 링크가 있다.
본문 링크: Transports / Altar / Knowledge / Multiplayer / Immobilize / Bring Enemy Priest /
Send Your Own Priest / Perform Sacrifice / Construction.
네 단계의 각각의 상세 페이지 역시 원본 [전체 본문](../sources/in-game-help.md)에 포함했다.
이번에는 이 개요를 읽었으며 실제 포획·의식 행동은 시연하지 않았다.

## 보존 범위와 재현

읽기용 전체 문서는 모든 본문·인용문·유닛·주문 설명·채팅·멀티플레이·전투 옵션을 원본 순서로 싣는다.
빈 앵커는 별칭으로, 중복 본문은 별도 절로 남겼다.
스크롤 표본을 OCR로 추측한 내용 대신 녹화와 바이트 일치하는 아카이브 텍스트를 사용했다.

```powershell
python extracted/record-play-20261001-2354/document_help.py
```

이 로컬 분석 스크립트는 원본 아카이브를 읽어 전체 본문 문서·UTF-8 원문 JSON·검사 결과를 만들며 게임을 실행하지 않는다.
원문 JSON의 `lines`를 이어 붙여 CP1252로 인코딩하면 120,888바이트 원본과 SHA-256이 일치한다.
High Priest의 실행 시점 머리 정보와 Back / OK는 영상에서 별도로 기록했다.
