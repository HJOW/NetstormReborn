# 우클릭 컨텍스트 메뉴와 워크샵 "덱" 등록

> 분석 상태: **사용자 확인 규칙 + 원본 캡처 관찰**. 실행 파일 속 메뉴·덱 처리 함수는 일부만 확인 (아래 4절).

## 1. 우클릭 컨텍스트 메뉴 (사용자 확인, 2026-09-28)

* 원본은 **마우스 오른쪽 버튼 클릭**을 지원한다. 게임 화면의 오브젝트(건물·유닛·사제 등)를 우클릭하면 그 오브젝트의 **컨텍스트 메뉴(정보 창)** 가 뜬다.
* 캡처 `The War Begins! - Rain Temple Context Menu.png` 는 Rain Temple 을, `The War Begins! - Sun Workshop Context Menu.png` 는 Sun Workshop 을 우클릭한 화면이다.
  `The War Begins! - High Priest Context Menu.png`·`Playing 3.png`(사제), `Playing 7.png`(골렘) 도 같은 메뉴다.
* 메뉴 구성 (캡처 기준):
  * 머리: `<이름> Level I`, `Owner:`, `Alignment:`(원소), `Class:` — 값은 노란 글자
  * 명령 목록: 타입마다 다름. 하위 메뉴(`>`)는 오른쪽에 두 번째 창으로 열린다. 비용·환급액 뒤에는 Storm Power 아이콘

  | 대상 | 명령 |
  |---|---|
  | High Priest | Construct > (Temple >, Workshop >, Build Level 1 Altar for 500), View Netstorm Knowledge, About, Player > |
  | Rain Temple | View Netstorm Knowledge, Salvage gains 1250, About, Player > |
  | Sun Workshop | View Current Production >, **Put Knowledge into Production >**, Upgrade costs 800, Salvage gains 200, About, Player > |
  | Golem | Salvage gains 100, About, Player > |

템플·워크샵·알타는 "유닛"이 아니며 사제의 `Construct` 메뉴로 짓는다. 덱 등록 대상은 그 밖의 유닛이다 ([island-ownership.md](island-ownership.md) 용어).

## 2. 워크샵에서 덱에 등록해야 생산·건설 가능 (사용자 확인, 2026-09-28)

* 유닛·건물을 생산하거나 건설하려면 **해당 원소(타입)의 워크샵을 우클릭**해서, 그 유닛을 화면 **왼쪽 사이드바의 "덱"에 등록**해야 한다.
  등록된 것만 사이드바에서 골라 배치(생산·건설)할 수 있다.
* 워크샵 메뉴의 `Put Knowledge into Production >` 하위 창 **"Knowledge Available"** 이 등록할 수 있는 목록이다. 머리에 `Production Slots Available:` (남은 생산 칸 수로 보임) 가 있다.
  The War Begins! 의 Sun Workshop Level I 에서는 **Rain Generator, Sun Cannon, Whirlibase** 가 나왔다.
* `View Current Production >` 은 현재 덱에 등록된 목록을 보는 메뉴로 보인다 (캡처 없음).
* `Upgrade costs 800` 은 워크샵 레벨 올리기로 보이며, 레벨에 따라 생산 칸 수나 목록이 달라지는지는 미확인이다.

### 캡처 대조 (The War Begins!)

| 캡처 | Storm Power | 사이드바 |
|---|---|---|
| `Sun Workshop Context Menu.png` (등록 전으로 보임) | 1400 | 다리 조각 6칸 + 사제만 |
| `Playing 1`~`8`, `Victory` 등 | 450~10650 | 다리 조각 + 사제 + **수정 모양·검은 포대·돔 아이콘 3개** |

→ 워크샵 목록의 3개(Rain Generator·Sun Cannon·Whirlibase)를 등록한 뒤 사이드바에 3개 아이콘이 생긴 것으로 해석된다. 아이콘 ↔ 타입 대응(수정 = Rain Generator `rainBattery`?, 검은 포대 = Sun Cannon, 돔 = Whirlibase)은 스프라이트 대조로 확인할 일이다.
`Playing 9`(다른 판, Storm Power 200) 에서는 같은 아이콘들이 붉게 칠해져 있어 비용 부족 표시로 보인다.

## 3. `.fort` `Deck` 섹션과의 관계 (추정)

* `.fort` 의 `Deck` 섹션([fort.md](../formats/fort.md) 3절)은 `[타입 번호][chance][power][numRemaining]` 항목 목록이다.
  `Deck.cpp` `0044f510` 이 `power` 가 양수인 항목의 `chance` 를 합산해 **가중 추첨**을 한다.
* 공식 미션 파일의 첫 항목이 `sunArcher`(chance 3~4) 처럼 유닛 타입이므로, 이 섹션이 사이드바 덱(등록된 생산 목록과 등장 확률)을 저장하는 것으로 추정한다.
  원본 463개 파일은 항목 수 28~48, `power` 1, `numRemaining` 255 로 채워져 있어, 미션 시작 시 덱과 워크샵 등록의 관계(전체 목록 중 등록된 것만 활성인지 등)는 추가 확인이 필요하다.
* 다리 조각도 사이드바 맨 위에 무작위로 채워지므로(`bridgeDrawRate`), 다리와 유닛이 같은 덱 추첨을 쓰는지도 확인한다.

## 4. 확인할 것 (4·8·9단계)

* exe: 우클릭 처리와 메뉴 구성(`Menugump.cpp`·`Metadisplay.cpp` 등 [모듈 맵](../exe/modules.md)), `Put Knowledge into Production` 의 동작, `Deck.cpp` 의 등록·추첨·`numRemaining` 감소
* 생산 칸 수(`Production Slots Available`)의 결정 규칙, 워크샵 레벨·업그레이드 효과
* ~~워크샵 원소와 등록 가능한 유닛의 관계~~ → 사용자 확인(2026-09-28): 워크샵은 **자기 원소 유닛만** 등록, **Sun Workshop 은 예외로 다른 원소의 Generator 도** 등록 가능 (Sun 발전기가 없으므로). Sun Workshop 목록의 Rain Generator 가 그 예 — [elements-energy.md](elements-energy.md)
* 등록 목록에 나오는 유닛과 기술 획득·`techBit`·미션 `myTech` 의 관계
* 사이드바에서 유닛을 골라 놓는 조작(좌클릭 선택 → 섬에 배치)과 비용 차감 시점
