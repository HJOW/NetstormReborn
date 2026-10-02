# 우클릭 컨텍스트 메뉴와 워크샵 "덱" 등록

2026-10-02 수정: 생산 버튼과 자동 등록을 없애고 원본처럼 워크샵 우클릭으로만 등록한다. 생산 창 배치·상태 표시는 [생산 창 문서](../screens/clone-deck.md)에 있다. `UpgradeWorkshopCommand`는 소유권·완공·최대 단계·업그레이드 비용(건설 비용과 같음: Sun 800 SP, Wind·Rain·Thunder 각 1,000 SP)을 검사하고 2/3/4칸을 확장한다. 원본 컨텍스트 메뉴 레이아웃·업그레이드 시간 복원은 후속이다.

> 분석 상태: **사용자 확인 규칙 + 원본 캡처 관찰**. 실행 파일 속 메뉴·덱 처리 함수는 일부만 확인 (아래 4절).

## 1. 우클릭 컨텍스트 메뉴 (사용자 확인, 2026-09-28)

* 원본은 **마우스 오른쪽 버튼 클릭**을 지원한다. 게임 화면의 오브젝트(건물·유닛·사제 등)를 우클릭하면 그 오브젝트의 **컨텍스트 메뉴(정보 창)** 가 뜬다.
* 캡처 `The War Begins! - Rain Temple Context Menu.png` 는 Rain Temple 을, `The War Begins! - Sun Workshop Context Menu.png` 는 Sun Workshop 을 우클릭한 화면이다.
  `The War Begins! - High Priest Context Menu.png`·`Playing 3.png`(사제), `Playing 7.png`(골렘) 도 같은 메뉴다.
* [공식 PDF 매뉴얼](../sources/pdf-manual.md#화면-그림과-판본별-메뉴-항목)의 45쪽(인쇄 44)에 있는 Thunder Workshop 메뉴도 제목·소유자·원소·분류 정보와 명령 목록, 비용 표시가 같은 형태다. 글꼴만 다르며 실제 컨텍스트 메뉴 UI와 맞는다(사용자 확인). PDF 그림의 Thunder Workshop 수치를 현재 Sun Workshop 수치와 혼동하지 않는다.
* 메뉴 구성 (캡처 기준):
  * 머리: `<이름> Level I`, `Owner:`, `Alignment:`(원소), `Class:` — 값은 노란 글자
  * 명령 목록: 타입마다 다름. 하위 메뉴(`>`)는 오른쪽에 두 번째 창으로 열린다. 비용·환급액 뒤에는 Storm Power 아이콘

  | 대상 | 명령 |
  |---|---|
  | High Priest | Construct > (Temple >, Workshop >, Build Level 1 Altar for 500), View Netstorm Knowledge, About, Player > |
  | Rain Temple | View Netstorm Knowledge, Salvage gains 1250, About, Player > |
  | Sun Workshop | View Current Production >, **Put Knowledge into Production >**, Upgrade costs 800, Salvage gains 200, About, Player > |
  | Wind·Rain·Thunder Workshop | 위와 같은 구성. Upgrade costs **1000**(건설 비용과 같음), Salvage는 건설 비용의 25%인 250 |
  | Golem | Salvage gains 100, About, Player > |

**워크샵 비용 정리:** 워크샵 업그레이드 비용은 해당 워크샵의 건설 비용과 같다: **Sun Workshop만 800 SP**, Wind·Rain·Thunder Workshop은 각각 **1,000 SP**다(사용자 확인 2026-10-02, `.type`의 `cost`와 일치). 건설 비용과 업그레이드 비용이 다른 워크샵은 없다. 회수액은 건설 비용의 25%(Sun 200, 그 외 250 — 마지막 값은 구버전 PDF 그림의 Thunder 값과 일치하나 현재 패치판 메뉴 캡처는 Sun만 확인)다.

템플·워크샵·알타는 "유닛"이 아니며 사제의 `Construct` 메뉴로 짓는다. 덱 등록 대상은 그 밖의 유닛이다 ([island-ownership.md](island-ownership.md) 용어).

## 2. 워크샵에서 덱에 등록해야 생산·건설 가능 (사용자 확인, 2026-09-28)

* 유닛·건물을 생산하거나 건설하려면 **해당 원소(타입)의 워크샵을 우클릭**해서, 그 유닛을 화면 **왼쪽 사이드바의 "덱"에 등록**해야 한다.
  등록된 것만 사이드바에서 골라 배치(생산·건설)할 수 있다.
  [공식 PDF 매뉴얼](../sources/pdf-manual.md#조작생산-절차)의 튜토리얼(PDF 25쪽, 인쇄 24쪽)도 `Put Knowledge Into Production`으로 등록한 뒤 생산 창에서 유닛을 좌클릭해 놓는 순서를 설명한다. 이는 원판 조작의 근거이며, 보유 패치판의 실제 비용은 `.type`을 따른다.
* 워크샵 메뉴의 `Put Knowledge into Production >` 하위 창 **"Knowledge Available"** 이 등록할 수 있는 목록이다. 머리에 `Production Slots Available:` (남은 생산 칸 수로 보임) 가 있다.
  The War Begins! 의 Sun Workshop Level I 에서는 **Rain Generator, Sun Cannon, Whirlibase** 가 나왔다.
* `View Current Production >` 은 현재 덱에 등록된 목록을 보는 메뉴다. [네 번째 사용자 녹화](../videos/ui-controls-record-play-20261001.md)의 01:26.5에서 Current Production → Rain Generator / About를 확인했다. 01:22~01:25에는 Rain Generator 등록 뒤 생산 창 아이콘이 생기고 등록 후보에서 해당 항목이 빠진다.
* `Upgrade costs 800` 은 캡처에 나타난 워크샵 업그레이드 비용이다. `GAME.HLP`는 생산 칸을
  Level I 2개·II 3개·III 4개로 설명한다 ([도움말의 게임 규칙](help-manual.md)).
  패치 실행 파일의 실제 판정과 등록 가능한 유닛 목록 변화는 아직 확인하지 않았다.

### 덱의 출처와 사라지는 조건 (사용자 확인, 2026-09-29)

* **용어**: 화면 왼쪽 세로 패널 = **"덱"** = 원판 매뉴얼의 **Production window**. 위에서부터 Storm Power 숫자, 다리 조각 칸, 사제·유닛 아이콘, 미니맵. 이 문서에서 "사이드바"라고 쓴 것도 같은 패널이다.
* 덱의 항목은 공급한 건물에 묶여 있다.

  | 공급 건물 | 덱에 넣는 것 | 그 건물이 파괴되면 |
  |---|---|---|
  | **워크샵** | 우클릭 `Put Knowledge into Production >` 로 등록한 유닛 | 그 워크샵으로 등록한 유닛들이 **덱에서 사라진다** |
  | **템플** | **다리 조각**과 **골렘** (등록 절차 없이 자동) | 다리 조각과 골렘이 **덱에서 사라진다** |

* 원판 매뉴얼도 같다: 템플의 역할 "다리와 골렘을 만든다 — 템플이 파괴되면 이 기본 유닛을 만들 수 없다"(`GAME.txt` 88행), 워크샵 "파괴되면 그 워크샵이 생산하던 것을 다른 워크샵을 지을 때까지 만들 수 없다"(682행).
* **한 유닛은 한 워크샵에만 등록된다** (사용자 확인, 2026-09-29): 어느 워크샵에 이미 등록한 유닛은 다른 워크샵의 컨텍스트 메뉴 목록("Knowledge Available")에 **나타나지 않는다**. 따라서 같은 유닛이 두 워크샵에 동시에 등록되는 경우는 없다.
* **재건해도 자동 복구되지 않는다** (사용자 확인): 파괴된 워크샵을 다시 지으면, 그 워크샵을 우클릭해 유닛을 **다시 등록해야** 덱에 돌아온다. 파괴와 함께 등록이 풀리므로 그 유닛은 다른 워크샵에서도 다시 등록할 수 있는 것으로 보인다 (목록에서 빠지는 조건이 "현재 등록됨"이므로).
* 덱 유닛의 **배치 후 재충전 간격**은 `useProductTimers` 설정과 요새/전투 모드에 따라 달라진다. 보유 설정의 요새 모드에서는 약 0.0001초가 예약된다. 시간표와 정적 분석 근거는 [production-refresh.md](../exe/production-refresh.md)에 있다.
* 구현 메모:
  * 덱 항목마다 "공급 건물" 참조를 두고, 건물 파괴 이벤트에서 해당 항목을 제거하고 등록도 해제한다.
  * 워크샵의 등록 가능 목록 = 그 워크샵 원소의 획득 지식(Sun Workshop 은 다른 원소 Generator 포함) − **이미 어느 워크샵에든 등록된 유닛**.
  * 워크샵 재건은 빈 등록 상태로 시작한다.
  * 템플 재건 시 다리·골렘은 등록 절차 없이 다시 덱에 들어오는 것으로 보인다 (템플은 사제가 다시 지을 수 있음 — [island-ownership.md](island-ownership.md)). 미확인.

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
* 도움말에 적힌 생산 칸 수(Level I 2개·II 3개·III 4개)의 패치 실행 파일 판정,
  워크샵 업그레이드 비용과 목록 변화
* ~~워크샵 원소와 등록 가능한 유닛의 관계~~ → 사용자 확인(2026-09-28): 워크샵은 **자기 원소 유닛만** 등록, **Sun Workshop 은 예외로 다른 원소의 Generator 도** 등록 가능 (Sun 발전기가 없으므로). Sun Workshop 목록의 Rain Generator 가 그 예 — [elements-energy.md](elements-energy.md)
* 등록 목록에 나오는 유닛과 기술 획득·`techBit`·미션 `myTech` 의 관계
* PDF가 설명한 좌클릭 선택 → 섬 배치가 보유 패치판에서도 동일한지, 비용이 언제 차감되는지 확인
