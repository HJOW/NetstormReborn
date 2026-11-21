# 운반체·Archer 유닛 우클릭 메뉴 연결 (WB04-E1138)

LEFT_JOBS "객체 우클릭 메뉴"의 남은 항목 중 실제 플레이에서 가장 자주 만나는 유닛(Bulf·Walker 같은 운반체, Archer 계열)의 우클릭을 앱에 연결했다.

## 증상

- Priest Training(bc5walkers76 등)에서 내 Bulf 를 실제 마우스로 우클릭해도 메뉴가 열리지 않았다. 사제·건물·신전·배터리·제단은 열렸다.
- 원인은 객체 메뉴 열기(`ClientObjectMenuOpen`)가 공통 열기(+ac `004adb30`)만 지원했기 때문이다. 운반체는 +ac `004268c0`(작성 +a8 `004288f0`), Archer 계열은 +ac `0041e660`(작성 +a8 `00420b30`)로 열기가 달랐다.

## 원본 근거

- cpppj `CampaignObjectMenuClient.cpp`: 열기 +ac 가 `4268c0`/`492080` 이면 `OpenSelectionMenu(sid, event, archer=false, common)`, `41e660` 이면 `archer=true` 로 같은 선택 관문을 먼저 지난 뒤(아직 선택되지 않았으면 Select) 공통 열기로 메뉴를 만든다.
- 웹에는 선택 관문(`RawUserSelection.carrierMenu/archerMenu`)과 작성 본체(`RawCarrierMenu`, `RawObjectMenuVariants` 의 `00420b30` Detonate!/Turn To Obelisk!)가 이미 있었고, 앱 입구만 없었다.

## 수정

| 위치 | 내용 |
| --- | --- |
| `www/ui/client-object-menu.js` | 세션의 운반체 메뉴(`makeCarrierMenu`)를 같은 작성기(헤더·항목·번역·Damageable 마무리)에 붙이고 `openCarrier(sid)` 를 연다. |
| `www/ui/client-object-menu-open.js` | 운반체(`4268c0`/`4288f0`)와 Archer(`41e660`/`420b30`)를 지원 대상에 추가하고, 각각 `world.openSelectionMenu(sid, event, archer, common)` 로 선택 관문을 지난 뒤 공통 열기를 부른다. |

- 열린 Bulf 메뉴: `Bulf Level I` / Owner / Alignment / Class: Ground Transport / `Salvage gains 125` / `About` / `Player ›`. 내용물을 싣고 있으면 `Carrying: …`·`Drop …` 항목이 앞에 붙는다(운반체 본체).

## 시험

- `tests/client-object-menu-open.test.mjs`(Node): Archer 는 `selection(archer=true) → fancy → summary → variant(420b30) → attach → sound`, 운반체는 `selection(false) → … → carrier → …` 순서이고, 건설 중 운반체는 거부된다. 이전의 "Archer 는 지원하지 않는다" 기대를 새 동작으로 바꿨다.
- `e2e/mission-unit-menu.spec.js`(Chromium): Priest Training 5 에서 내 Bulf 를 우클릭해 그 Bulf 의 객체 메뉴가 열리고, 빈 곳을 눌러 닫힌다. 운반체 입구를 빼면 열리지 않아 실패한다.

## 남은 것

- 내용물을 싣거나 주문을 가진 운반체의 `Carrying`/`Drop`/`Cast` 항목을 실제 플레이로 확인하지는 않았다(항목 작성은 본체 Node 시험이 다룬다).
- Archer 유닛(Detonate!)을 실제 미션에서 우클릭해 보는 확인은 하지 않았다(해당 유닛이 있는 미션 시작 상태가 없다). 열기·작성 순서는 Node 시험으로만 확인했다.
