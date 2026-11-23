# 운반체 Carrying/Drop·폭탄 Detonate! 우클릭 메뉴 e2e (WB04-E1310, 2026-11-24)

LEFT_JOBS 6단계 "객체 우클릭 메뉴"의 남은 두 확인(Archer 계열 특수 행, 짐을 실은 운반체의 Carrying/Drop)을 실제 Chromium 으로 끝냈다. 새 시험 `e2e/mission-unit-menu-rows.spec.js`(2건, 약 20초)이고 **웹 코드는 바꾸지 않았다**(앱이 이미 맞게 동작했다). 이로써 대응표 `CampaignObjectMenuClient.cpp` 행을 partial → port 로 올렸다.

## 확인한 것

| 대상 | 메뉴 | 눌렀을 때 |
| --- | --- | --- |
| 너겟을 실은 내 Bulf(Priest Training 5, SID 15005) | 실제 우클릭으로 열린 운반체 메뉴에 `Carrying` 머리말 아래 **Drop 행**(명령 `0x03000076`, 인자 = Bulf SID, 활성 플래그)이 있고 Salvage·About·Player 행이 이어진다 | 명령이 입력 큐에 올라 Session 이 소비한다: Cast/Drop 성공 `0x030000b2`와 내 너겟 납품 `0x020000b4` 통지가 뒤따르고 너겟이 내용물(contained)에서 빠진다(상태 8 → 3). 큐가 비고 메뉴가 닫히며 오류가 없다 |
| 내 폭탄(`bombExplodeSmall`, TYPE 113: 원본 +a8 `00420b30`·+ac `0041e660` 'Archer 계열' 파생) | 비편집 모드에서 **Detonate! 행**(명령 `0x03000074`, 인자 = 폭탄 SID, 활성)이 있고 `Turn To Obelisk!`(`0x030000ce`)는 없다(편집 전용) | 명령이 입력 큐에 올라 소비되고 큐가 비며 메뉴가 닫힌다 |

## 알게 된 것

- 'Archer 계열'의 `+a8 00420b30` 은 이름과 달리 **폭탄 종류**(`bombExplodeSmall/Medium/Large`·`bombHeal`·`bombInvisible`·`bombParalyze`·`bombHardener`·`bombTreason`·`bombTwister` 계열·`bombGraviton`·`BOMBmeteor`·`bombMano` 계열·`bombLighting*`·`bombSpecialOne`, TYPE 113~120·168~183)의 작성 본체다. 미션의 `sunArcher`·`windArcher` 는 일반 Damageable 메뉴(`0044cbd0`)이다.
- 폭탄은 화면 적중 대상이 아니라(`world.pick` 이 못 찾음) 우클릭 적중 뒤와 같은 앱 객체 메뉴 열기 입구(`ui.openObject`)를 불러 열고, 운반체는 실제 마우스 우클릭으로 열었다. 폭탄·너겟은 실제 미션에 없어 Session 객체 생성 경로(`world().create/pop/setAssetOwner`·`createContained(parent, 154, 0, 0, 0x10, 0)`)로 만들었다.
- 메뉴 행은 Gump 노드의 `items[index].command`·`parameter`·`flags` 와 `bounds`·`offsets`·`heights` 로 찾아 실제 마우스로 누른다(`mission-build-placement.spec.js` 의 회수 행과 같은 방식, 메뉴 노드가 중첩이면 재귀 탐색).

## 남은 것

- 운반체가 **줍는** 앱 조작(Bulf 선택 → 너겟·사제를 눌러 줍기)과 Walker·Cast(주문을 가진 운반체) 행의 실제 확인은 남았다('사제·운반·경로 바인딩'의 줍기 항목).
- 편집 모드의 `Turn To Obelisk!`/`Turn To Spell!` 는 편집 우클릭 메뉴가 웹 제외라 확인하지 않는다.
