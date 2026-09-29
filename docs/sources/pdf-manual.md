# 공식 매뉴얼 PDF — 조작법과 구버전 수치 대조

> 원문: [`originals/help/manual.pdf`](../../originals/help/manual.pdf), *NetStorm: Islands at War Manual (The Book of Nimbus)*. 사용자가 외부 사이트에서 확보한 107쪽 스캔 PDF다. 아래 `PDF 쪽`은 뷰어의 1부터 시작하는 순서이며, 인쇄된 쪽 번호는 따로 적었다. 이번에는 조작·메뉴·유닛 비용 관련 쪽을 선별해 읽었으며 전체 페이지를 전수 분석한 결과는 아니다.

## 자료의 적용 범위

기본 조작과 생산 절차는 PDF의 설명이 동봉 `GAME.HLP` 요약과 일치한다. 메인 메뉴와 컨텍스트 메뉴 그림도 보유 게임 화면의 기본 형태를 보여 준다. 따라서 마우스 입력, 스크롤, 다리 연결, 건설·생산 순서와 화면 구성의 자료로 활용한다. 보유 실행 파일은 후대 패치판이므로, 추가·변경된 단축키는 [패치 이력](patch-history.md#3-입력-키), 현재 메뉴 항목과 좌표는 [스크린샷](../screens/main-menu.md), 실제 비용·레벨은 보유 `.type`과 exe를 기준으로 한다.

PDF의 목차(PDF 10~11쪽, 인쇄 9~10쪽)는 조작 입문 21쪽, 화면 설명 31쪽, 유닛·주문 핸드북 61쪽, 명령 요약 95쪽을 가리킨다. 스캔 이미지가 본문인 페이지는 `pdftotext`로 문장이 추출되지 않으므로, 아래 근거는 페이지 그림을 직접 읽은 것이다.

## 조작·생산 절차

| 동작 | PDF 근거 | 분석 문서에 적용할 내용 |
|---|---|---|
| 좌클릭·우클릭 | PDF 22쪽(인쇄 21), 94쪽(인쇄 95) | 좌클릭으로 대상을 선택·집거나 들고 있는 것을 놓는다. 생산 창에서 유닛을 집어 전투 화면에 배치할 때도 쓴다. 우클릭은 대상 메뉴를 열거나 들고 있는 다리·방향성 유닛을 회전한다. |
| 화면 스크롤 | PDF 22~23쪽(인쇄 21~22), 94~95쪽(인쇄 95~96) | 전체화면 가장자리 스크롤, Alt(또는 휠 버튼)를 누른 채 커서 방향 스크롤, Shift를 더한 직선 스크롤. 창 모드의 직선 스크롤은 Alt+Shift로 설명한다. |
| Esc와 위치·선택 키 | PDF 23쪽(인쇄 22), 95쪽(인쇄 96) | Esc는 상단 메뉴 표시 전환과 들고 있는 다리 반환에 쓰인다. Shift+숫자는 카메라 위치 저장, 숫자는 복귀, Tab은 최근 유닛 순환, F1~F9는 도움말·화면/대상 조회 등에 쓰인다. |
| 건물 건설 | PDF 24쪽(인쇄 23) | 사제 우클릭 → Construct → Temple/Workshop 선택 → 섬 위의 유효 위치 좌클릭. |
| 워크샵 생산 등록·유닛 배치 | PDF 25쪽(인쇄 24) | 워크샵 우클릭 → Put Knowledge Into Production → 생산 칸 등록 → 생산 창의 유닛 좌클릭 → 전투 화면 배치. 배치에는 생산 등록, 충분한 Storm Power, 위치의 에너지가 필요하다. |
| 다리·발전기 | PDF 23쪽(인쇄 22), 26쪽(인쇄 25) | 다리는 연결 가능한 시작점에서 놓는다. Wind Generator도 최초 1기는 템플 에너지 범위 안에 지어야 하며, 이후 공급 범위를 연장할 수 있다. |

이 조작 설명은 [GAME.HLP 조작 요약](game-manual.md#4-조작-요약-행-404439)과 부합한다. 다만 `R`·`C` 회전/사제 선택, `D` 반복 배치, `Shift+F3` 섬 테마 등은 후대 패치에서 추가·변경됐다. 원판 키 표를 현재 패치판의 전체 키 계약으로 사용하지 않는다.

## 원판 비용과 보유 패치판의 차이

PDF의 `Cost in Storm Power`는 **매뉴얼 발행 당시의 값**이다. 아래 현재 값은 보유 아카이브의 `.type` `cost`를 직접 확인했다. 모든 비용을 전수 대조한 표가 아니며, 선택한 사례다. 더 넓은 HLP↔`.type` 표는 [게임 매뉴얼 요약](game-manual.md#7-유닛주문-핸드북-행-5721106)에 있다.

| 유닛 | PDF 쪽(인쇄 쪽) | PDF의 비용 | 현재 `.type` 비용 | 판정 |
|---|---:|---:|---:|---|
| Sun Disc Thrower (`sunarcher`) | 65 (64) | 200 SP | 300 SP | 변경 |
| Stone Tower (`sunblocker`) | 66 (65) | 300 SP | 400 SP | 변경 |
| Sun Barricade (`sunfence`) | 67 (66) | 600 SP | 300 SP | 변경 |
| Man o' War Pool (`rainaviary`) | 73 (74) | 400 SP | 600 SP | 변경 |
| Ice Tower (`rainblocker`) | 75 (76) | 400 SP | 800 SP | 변경 |
| Bulf (`bulf`) | 79 (80) | 600 SP | 500 SP | 변경 |
| Sun Cannon (`suncannon`) | 67 (66) | 400 SP | 400 SP | 동일 |
| Wind Generator (`windbattery`) | 69 (68) | 400 SP | 400 SP | 동일 |

선별한 비용은 기존 `GAME.HLP` 정리의 원판 비용과도 일치한다. 이 일치는 **확인한 항목에 한정**된다. 비용뿐 아니라 레벨·체력·에너지 요구량도 원판과 달라진 사례가 있으므로, 유닛 구현에서는 PDF 수치를 현재 값으로 복사하지 않는다.

## 유닛별 건설 에너지 (Energy to Build)

2026-09-29 유닛 핸드북(PDF 63~87쪽, 인쇄 62~86쪽)의 각 항목을 모두 읽었다. **스캔본에는 인쇄 71~72쪽이 없다**(PDF 71쪽 = 인쇄 70, PDF 72쪽 = 인쇄 73). 그래서 Wind Tower·Wind Workshop·Wind Temple 항목은 이 PDF로 확인하지 못했다.

PDF의 규칙은 한결같다.
- 원소 유닛: **L1 = 자기 원소 1**, **L2 = 자기 원소 1 + Sun 1**, **L3 = 자기 원소 2 + Sun 1**.
- Sun 유닛: **Sun × 레벨**.
- 예외: **Generator 3종·Outpost = Sun 1**(아무 원소).
- 건물(템플·워크샵·알타)·사제·기지가 만드는 비행체(Whirligig·Dust Devil·Man o' War): None.

같은 레벨이라도 유닛에 따라 필요 에너지가 다르다. 예를 들어 L1 Bulf는 Thunder 1이고 L1 Thunder Generator는 Sun 1이다. 사용자도 Bulf는 Thunder 공급원(Thunder Generator 또는 Thunder Temple)이 반드시 있어야 한다고 확인했다.

패치판 exe도 같은 규칙으로 요구값을 만든다([exe 분석](../exe/energy-requirements.md)). 다만 **유닛 레벨이 판본마다 바뀌었으므로** 현재 에너지는 오른쪽 열(패치판 `.type` 레벨 + 같은 규칙)을 따른다.

| 유닛 | PDF 쪽(인쇄) | PDF 레벨·에너지 | 패치판 `.type` level | 패치판 요구 에너지 |
|---|---:|---|---:|---|
| Golem | 64 (63) | L1, Sun 1 (400sp) | 1 | Sun 1 (아무 1). 패치판은 cost 없음 |
| Balloon | 64 (63) | L2, Sun 2 | 2 | Sun 2 |
| Sun Disc Thrower | 65 (64) | L1, Sun 1 | 1 | Sun 1 |
| Whirlibase | 65 (64) | L2, Sun 2 | 2 | Sun 2 |
| Stone Tower | 66 (65) | L1, Sun 1 | 1 | Sun 1 |
| Sun Cannon | 67 (66) | L1, Sun 1 | 1 | Sun 1 |
| Sun Barricade | 67 (66) | **L3, Sun 3** | **1** | **Sun 1** |
| Wind Generator | 69 (68) | L1, Sun 1 | 1 | Sun 1 (`mana = "s"`) |
| Sail Skater | 69 (68) | **L1, Wind 1** | **2** | **Wind 1 + Sun 1** |
| Air Ship | 70 (69) | L3, Wind 2 + Sun 1 | 3 | Wind 2 + Sun 1 |
| Devil Maker | 70 (69) | L3, Wind 2 + Sun 1 | 3 | Wind 2 + Sun 1 |
| Crossbow | 71 (70) | L2, Wind 1 + Sun 1 | 2 | Wind 1 + Sun 1 |
| Wind Tower | (스캔 누락) | — | 2 | Wind 1 + Sun 1 |
| Rain Generator | 72 (73) | L1, Sun 1 | 1 | Sun 1 (`mana = "s"`) |
| Cloud Floater | 72 (73) | L3, Rain 2 + Sun 1 | 3 | Rain 2 + Sun 1 |
| Ice Cannon | 73 (74) | L2, Rain 1 + Sun 1 | 2 | Rain 1 + Sun 1 |
| Man o' War Pool | 73 (74) | **L2, Rain 1 + Sun 1** | **3** | **Rain 2 + Sun 1** |
| Acid Barricade | 74 (75) | **L1, Rain 1** | **2** | **Rain 1 + Sun 1** |
| Ice Tower | 75 (76) | L2, Rain 1 + Sun 1 | 2 | Rain 1 + Sun 1 |
| Crystal Crab | 75 (76) | **L2, Rain 1 + Sun 1** | **1** | **Rain 1** |
| Thunder Generator | 78 (79) | L1, Sun 1 | 1 | Sun 1 (`mana = "s"`) |
| **Bulf** | 79 (80) | **L1, Thunder 1** | 1 | **Thunder 1** (사용자 확인) |
| Bulwark | 79 (80) | L2, Thunder 1 + Sun 1 | 2 | Thunder 1 + Sun 1 |
| Thunder Cannon | 80 (81) | L2, Thunder 1 + Sun 1 | 2 | Thunder 1 + Sun 1 |
| Vander Tower | 80 (81) | L3, Thunder 2 + Sun 1 | 3 | Thunder 2 + Sun 1 |
| Arc Spire | 81 (82) | **L2, Thunder 1 + Sun 1** | **1** | **Thunder 1** |
| Outpost | 85 (86) | L1, Sun 1 | — | Sun 1 (`mana = "s"`) |

굵은 글씨는 원판과 패치판의 레벨이 달라 에너지 구성도 달라진 유닛이다. 필요 에너지 판정(서로 다른 공급원, 범위 교집합, Sun = 아무 원소)은 [원소·에너지 규칙](../gameplay/elements-energy.md)에 있다. PDF 설명 문장에는 이 밖에도 "Workshop이 파괴되면 그 워크샵이 생산하던 것은 다시 지을 때까지 못 만든다", "Power Stream이 적절한 에너지와 합쳐지면 유닛이 즉시 생성된다"(워크샵 항목), "Temple은 자기 원소 에너지 1개를 자신을 중심으로 한 원에 공급한다"(템플 항목)가 있다. 모두 기존 규칙 문서와 일치한다.

## 화면 그림과 판본별 메뉴 항목

PDF 32쪽(인쇄 31)의 메인 메뉴 그림은 큰 NetStorm 제목과 그 아래 놓인 버튼이라는 기본 화면 구성이 보유 [패치판 메인 메뉴 캡처](../screens/main-menu.md)와 같다. PDF의 색상 손상은 스캔의 영향이며 해상도 차이도 있지만, 화면 자체는 같은 UI다(사용자 확인). PDF 설명의 기본 항목은 `Campaign`·`Multiplayer`·`Demo`·`Help`·`Credits`·`Options`·`Quit`의 **7개**다. `Intro`는 CD-ROM에 게임 CD가 있을 때만 나타난다고 별도로 적혀 있다. 보유 패치판 캡처에는 `Edit`가 추가된 **8개**가 보인다. PDF의 Campaign 네 묶음 설명과 패치판의 여섯 묶음도 판본별 항목 차이다.

PDF 45쪽(인쇄 44)의 Thunder Workshop 컨텍스트 메뉴 그림은 제목·`Owner`·`Alignment`·`Class` 정보, 명령 목록, 비용 표시와 돌무늬 메뉴 창의 구성이 보유 [워크샵 우클릭 캡처](../gameplay/workshop-deck.md#1-우클릭-컨텍스트-메뉴-사용자-확인-2026-09-28)와 같다. 글꼴만 다르며 실제 UI의 형태와 맞는다(사용자 확인). 그림의 `Upgrade costs 1000sp`·`Salvage gains 250sp`는 해당 구버전 Thunder Workshop에 적힌 값이므로 현재 다른 워크샵의 비용으로 일반화하지 않는다.

PDF 34~35쪽(인쇄 33~34)의 `Adjust Direct Draw`, `Cursor Snap to Grid` 같은 옵션은 현재 [Options 캡처](../screens/README.md#1-메인-메뉴-계열-창-제목-netstorm-main-menu)와 목록이 다르다. UI의 기본 형태를 읽을 때는 PDF도 근거로 쓰고, 현재 판본의 메뉴 항목·좌표·표시값은 보유 패치판 캡처를 우선한다.

## 추가 확인 범위

정확한 PDF 판본과 `GAME.HLP` 전체 본문의 동일성, 모든 유닛·주문 비용과 조작 키의 판본별 변화는 아직 전수 확인하지 않았다. 새 차이가 발견되면 이 문서에 PDF 쪽과 현재 `.type` 또는 exe의 근거를 함께 기록한다.
