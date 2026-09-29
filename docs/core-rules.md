# 게임 규칙 코어 (`Netstorm.Core`)

> 2026-09-29 구현. 문서화된 규칙(사용자 확인·원본 매뉴얼·exe 정적 분석)만 옮겼다. MonoGame에 의존하지 않으며 `tests/Netstorm.Core.Tests`에서 헤드리스로 검사한다.
> 원본 게임은 실행하지 않았다. 다리·이동·전투·AI 규칙은 아직 없다.

## 구성

| 파일 | 내용 | 근거 |
|---|---|---|
| `Rules/Element.cs` | 원소 Sun·Rain·Wind·Thunder, theme·요구 문자(w·r·t·s) 변환 | [elements-energy.md](gameplay/elements-energy.md) 1절, exe 원소 문자 표 `0x540ad8` |
| `Rules/EnergyRequirement.cs` | 유닛별 필요 에너지. `.type`의 `mana`가 있으면 그 값, 없으면 theme·level 기본값(원소 L1 = 원소 1, L2 = 원소 1 + Sun 1, L3 = 원소 2 + Sun 1, Sun = Sun × 레벨) | [energy-requirements.md](exe/energy-requirements.md), [PDF 대조표](sources/pdf-manual.md#유닛별-건설-에너지-energy-to-build), 사용자 확인(Bulf = Thunder 1) |
| `Rules/EnergySupply.cs` | 위치를 덮는 공급원 선택(자기·동맹, 중심 거리² ≤ 반지름²)과 요구 글자 배정(공급원 하나 = 에너지 1개) | `Mana.cpp` `004730c0`·`00473330`, 사용자 확인 충족 판정 |
| `Rules/Footprint.cs` | 발자국: 기준점 = **오른쪽 아래 칸**, 사각형 `(x − (foot_x − 1), y − (foot_y − 1))`~`(x, y)`, 중심 = 양 끝 평균 | exe `FUN_0049ae80`(아래 "발자국") |
| `Rules/BattleOptions.cs` | 전투 옵션 표(다리 칸·Unit Rate·Generator Range·시작 금액·보상·가이저)와 공급 반지름(14~30칸 제한), 튜토리얼 2 덮어쓰기 | [battle-options.md](exe/battle-options.md) |
| `Rules/ObjectKind.cs` | 템플·워크샵·아웃포스트·알타·사제·Generator·건물형 유닛·수송·비행체·다리·주문·가이저·결정 분류 | 타입 플래그, [island-ownership.md](gameplay/island-ownership.md) 용어 |
| `Rules/StormPower.cs` | 표시 색(≤1000 빨강, ≤2000 노랑), 결정 200, 회수 25%, 파괴 보상 | `Combatgump.cpp` `0043da10`, 매뉴얼, 컨텍스트 메뉴 캡처 |
| `Rules/ProductionTimers.cs` | 배치 후 재충전 간격 10/5/1초, 요새 모드 0.0001초, useProductTimers 30/15/8초 | [production-refresh.md](exe/production-refresh.md) |
| `Rules/ProductionDeck.cs` | 생산 창(덱): 템플 → 다리·골렘, 워크샵 등록(자기 원소, Sun Workshop 의 Generator 예외, 한 유닛은 한 워크샵에만, Level I/II/III = 2/3/4칸, 파괴 시 해제, 재건은 빈 상태) | [workshop-deck.md](gameplay/workshop-deck.md) |
| `Rules/IslandOwnership.cs` | 섬 상태(내 섬·빈 섬·남의 섬), 유닛·건물 위치 조건 | [island-ownership.md](gameplay/island-ownership.md) 규칙 2~6 |
| `Rules/BattleMap.cs` | `.fort` 오브젝트에서 소유권·공급원·점유 칸을 만들고, 유닛 배치를 위치 → 빈 자리 → Storm Power → 에너지 순서로 판정·실행 | 위 규칙 조합, 매뉴얼 "빈 자리 + SP + 에너지" |
| `Simulation/FixedTimestep.cs` | 고정 틱 누적기(기본 24Hz, 따라잡기 8틱 한도), 초 → 틱 올림 | [animation-timing.md](videos/animation-timing.md) 4절 |
| `Simulation/MsvcRandom.cs` | MSVC CRT `rand()` 선형 합동 난수 | 지형 분석의 `_rand` 재현과 같은 계수 |
| `Simulation/NetstormRandom.cs` | 게임 전역 정수 난수 (상태 × 0x10003 + 3, 시드 0 → 0x0BAD0BAD) | `FUN_004558c0`·`FUN_004558f0` ([bridge-pieces.md](exe/bridge-pieces.md) 3절) |
| `Bridges/BridgeLinks.cs` | 방향 글자 'A'~'P' ↔ 연결 비트, 회전 표, 반대 방향 | VA 0x52f910·0x531590 |
| `Bridges/BridgePatternCatalog.cs` | 다리 조각 모양 26개(가중치 합 287)와 누적 가중치 추첨 | VA 0x52f998, `Canondecoder.cpp` `004257c0` |
| `Bridges/BridgePiece.cs` | 모양 + 회전(1 = 시계 방향 90°) → 회전된 칸 목록. 원본 조작: 오른쪽 클릭 = 시계, C(반대 회전)면 반시계 | `00425c20`·`00425860`, 원본 실행 |
| `Bridges/BridgeGrid.cs` | 놓인 다리 칸의 연결망·배치 판정(겹침 불가, 섬 가장자리·내 다리 열린 끝에 이어짐)·10초 주기 붕괴(수명 7→0, 5 아래 금 감, 단단한 칸 제외) | `Bridge.cpp` `00422bc0`·`004227e0`·`00421c30`, `Rifttype.cpp` `0049b510` ([bridge-pieces.md](exe/bridge-pieces.md) 8절). 이어짐·붕괴 대상 조건은 근사 |
| `Rules/MissionStart.cs` | 미션 머리 값 → 시작 SP(myStartMoney, 없으면 전투 옵션)·시작 지식(myTech)·기술 허용(techAllowed: deny/allow/all 순서 적용)·denySalvage 등 | `Mission.cpp` `00482eb0`, `Totalmade.cpp` `004c23c0`~`004c2400`, 튜토리얼 1·2 원본 관찰 |
| `Bridges/BridgeCursor.cs` | 커서 → 들고 있는 조각의 왼쪽 위 칸: (⌊(x + 7) / 16⌋, ⌊y / 11⌋), 크기·회전 무관 | 원본 실행 측정 ([bridge-pieces.md](exe/bridge-pieces.md) 4절) |
| `Bridges/BridgeFrames.cs` | 칸 → bridge.type 프레임 (보통 / 금 감 +10 / 단단함 20) | `0049a940`, bridge.type 주석 |
| `Bridges/BridgeTray.cs` | 생산 창 다리 칸: 템플이 있으면 1초마다, Bridge Slots 칸까지, 5번째 추첨마다 한 칸 조각, 템플을 잃으면 비움 | `Combatgump.cpp` 매 프레임 처리 ([bridge-pieces.md](exe/bridge-pieces.md) 6절) |

## 발자국과 공급 범위 기하

* 원본 `FUN_0049ae80(type, rect, x, y)`는 타입의 `foot_x`(+0x1d4)·`foot_y`(+0x1d8)로 `(x − (foot_x − 1), y − (foot_y − 1))`~`(x, y)` 사각형을 만든다. 따라서 저장·배치 좌표는 발자국의 **오른쪽 아래 칸**이다. 이는 작은 받침에서 이미 확인한 "오른쪽 아래 칸 기준점"([terrain-and-bridges.md](exe/terrain-and-bridges.md))과 같다.
* `FUN_004730c0`은 배치 대상 사각형의 중심 `((x0 + x1) × 0.5, (y0 + y1) × 0.5)`과 크기 항 `((x1 − x0) × 0.7071)²`을 공급원 객체의 가상 함수(+0xA0)에 넘긴다. 공급원 쪽 판정 함수는 가상 호출이라 아직 찾지 못했다.
* 클론은 **공급원 발자국 중심과 배치 발자국 중심의 칸 거리² ≤ 반지름²**으로 판정한다. 크기 항이 판정을 넓히는지(예: 반지름 + 대상 반지름)는 **미확인**이다. 범위 경계 근처의 큰 유닛에서 원본과 1~2칸 차이가 날 수 있다.

## 게임 쪽 연결 — 맵 뷰어 배치 시험 모드

맵 뷰어에서 **P**를 누르면 플레이어 1로 워크샵 생산 유닛 27종을 놓아 볼 수 있다([실행 안내](map-viewer.md#배치-시험-모드)). 판정은 `BattleMap.CheckUnit` 하나로 한다. 결과로 발자국 칸(초록/빨강), 아군 공급원의 범위 원, 요구 에너지에 배정된 공급원까지의 선, 불가 이유, 원본 색 규칙의 Storm Power를 보여 준다.

검증(2026-09-29): Dissolved Alliance! 의 플레이어 1 섬 칸 (124,126)을 판정했다.
- Sail Skater(Wind 1 + 아무 1): 배치 가능. 이웃 Wind Generator 두 곳에 배정되었다.
- Bulf(Thunder 1): Thunder 공급원이 없어 에너지 부족.
- Thunder Cannon: 자리가 차 있어 불가.
- 스크린샷: `extracted/screens/placement-*.png`.

## 근사·미구현 (다음 분석 대상)

* **다리 연결·다리 끝**: 빈 섬 연결은 뷰어의 `C` 키 가정으로 대신한다. 다리 끝은 "발자국 둘레에 플레이어 다리 칸이 있는 섬 밖 위치"로 근사한다. 다리 조각 생성·회전은 구현했고(`Bridges/`), 배치 판정·연결·붕괴는 `Bridge.cpp` 분석 후 교체한다([bridge-pieces.md](exe/bridge-pieces.md) 8절). `Deck.cpp`는 다리와 무관한 지식·생산 덱이다.
* **섬 칸 판정**: 지면 미리보기의 본섬 마스크(영역 번호 ≥ 0)를 쓴다. 작은 받침·유닛 발판(`createsisland`)은 섬 밖으로 본다. 기준점 칸 하나만으로 섬을 판정하며, 발자국 전체가 섬 위여야 하는지는 미확인이다.
* **지식·등록 상태**: 뷰어는 모든 생산 유닛을 후보로 보여 준다. 미션 `myTech`와 `.fort` `Technology`·`Deck` 섹션으로 초기 지식·덱을 채우는 연결은 아직 없다.
* **동맹**: 기본은 같은 플레이어만 아군이다. 미션·멀티플레이 동맹 설정 연결은 남았다.
* **회수 금액**: 손상된 유닛의 감소 공식이 미확인이라 건강한 상태(25%)만 계산한다.
* **워크샵 생산 칸 수**: `GAME.HLP` 값(2/3/4)이다. 패치판 exe의 판정은 미확인이다.
