# 게임 규칙 코어 (`Netstorm.Core`)

> 2026-09-29 정적 규칙 구현, 2026-09-30 게임 세션(고정 틱 루프·명령·플레이어 상태)으로 묶음.
> 사용자 확인·원본 매뉴얼·exe 분석을 기준으로 하며, 아직 부족한 전투 자료는 문서에 근사를 명시하고 구현한다.
> MonoGame에 의존하지 않으며 `tests/Netstorm.Core.Tests`에서 헤드리스로 검사한다. 원본 게임은 실행하지 않았다.
> 사제의 가이저 왕복 수집과 튜토리얼 1 진행 조건을 구현했다. 2026-10-01 포대 전투·체력·파괴·사제 기절을 추가했다.
> 전투에는 명시적인 근사가 있다([확정 근거·추정값·검증](gameplay/combat.md)).
> Whirlibase·Whirligig의 출격·이동·공격·귀환을 추가했다([비행체 계약](gameplay/flyers.md)). 다른 수송 유닛 이동·Rain/Wind 공중 공격·전략 AI는 아직 없다.

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
| `Rules/BattleMap.cs` | `.fort` 오브젝트에서 소유권·공급원·점유 칸(겹침 수를 세어 회수해도 다른 오브젝트 칸은 유지)을 만들고, 유닛 배치(`CheckUnit`)·사제 건물 건설(`CheckBuilding`)을 위치 → 빈 자리 → Storm Power → 에너지 순서로 판정·실행. 저장 오브젝트 번호(`InitialObjects`)를 노출 | 위 규칙 조합, 매뉴얼 "빈 자리 + SP + 에너지" |
| `Simulation/FixedTimestep.cs` | 고정 틱 누적기(기본 24Hz, 따라잡기 8틱 한도), 초 → 틱 올림 | [animation-timing.md](videos/animation-timing.md) 4절 |
| `Simulation/MsvcRandom.cs` | MSVC CRT `rand()` 선형 합동 난수 | 지형 분석의 `_rand` 재현과 같은 계수 |
| `Simulation/NetstormRandom.cs` | 게임 전역 정수 난수 (상태 × 0x10003 + 3, 시드 0 → 0x0BAD0BAD) | `FUN_004558c0`·`FUN_004558f0` ([bridge-pieces.md](exe/bridge-pieces.md) 3절) |
| `Bridges/BridgeLinks.cs` | 방향 글자 'A'~'P' ↔ 연결 비트, 회전 표, 반대 방향 | VA 0x52f910·0x531590 |
| `Bridges/BridgePatternCatalog.cs` | 다리 조각 모양 26개(가중치 합 287)와 누적 가중치 추첨 | VA 0x52f998, `Canondecoder.cpp` `004257c0` |
| `Bridges/BridgePiece.cs` | 모양 + 회전(1 = 시계 방향 90°) → 회전된 칸 목록. 원본 조작: 오른쪽 클릭 = 시계, C(반대 회전)면 반시계 | `00425c20`·`00425860`, 원본 실행 |
| `Bridges/BridgeGrid.cs` | 놓인 다리 칸의 연결망·배치 판정(겹침 불가, 섬 가장자리·내 다리 열린 끝에 이어짐)·10초 주기 칸 단위 붕괴(끝 칸만 구동자, 접합 칸 무리는 함께, 섬에서 떨어진 5칸 미만 조각은 즉시, 수명 7→0, 5 아래 금 감)·조각 품질별 시작 상태(금 감 = 수명 4)·건물형 유닛이 없어질 때 주변 ±2칸 한 단계 약화(`WeakenAround`) | `Bridge.cpp` `00422bc0`·`004227e0`·`00421c30`, `Rifttype.cpp` `0049b510`, `Construction.cpp` `00442c80`, 공통 제거 처리 `0044b9e0` ([bridge-pieces.md](exe/bridge-pieces.md) 8절). 이어짐 판정과 칸 처리 순서는 근사 |
| `Bridges/BridgeAnchors.cs` | 다리를 시작할 수 없는 섬 칸: 가장자리 초목(edgeFarm) 칸 + dropBlocking 타입 오브젝트 발자국 (사용자 확인 규칙 "초목이 있는 가장자리에서는 다리를 시작할 수 없다") | edgefarm.type `dropBlocking`, `Squid.cpp` `004b02d0` 스폿 비트 0x10, `Rifttype.cpp` `0049b510` ([bridge-pieces.md](exe/bridge-pieces.md) 8.4절) |
| `Rules/MissionStart.cs` | 미션 머리 값 → 시작 SP(myStartMoney, 없으면 전투 옵션)·시작 지식(myTech)·기술 허용 표(techAllowed: deny/allow/all 순서 적용, **실행 중 `Set`·`SetAll`로 바뀜**)·denySalvage 시작 값 등. 머리 값은 시작 상태이고 튜토리얼 단계 처리가 실행 중에 바꾼다 | `Mission.cpp` `00482eb0`, `Totalmade.cpp` `004c23c0`~`004c2400`, [mission-header-flags.md](exe/mission-header-flags.md), 튜토리얼 1·2 원본 관찰 |
| `Bridges/BridgeCursor.cs` | 커서 → 들고 있는 조각의 왼쪽 위 칸: (⌊(x + 7) / 16⌋, ⌊y / 11⌋), 크기·회전 무관 | 원본 실행 측정 ([bridge-pieces.md](exe/bridge-pieces.md) 4절) |
| `Bridges/BridgeFrames.cs` | 칸 → bridge.type 프레임 (보통 / 금 감 +10 / 단단함 20) | `0049a940`, bridge.type 주석 |
| `Bridges/BridgeTray.cs` | 생산 창 다리 칸: 템플이 있으면 1초마다, Bridge Slots 칸까지, 5번째 추첨마다 한 칸 조각, 템플을 잃으면 비움, 새 조각은 6초 동안 금 간 품질(`QualityAt`) | `Combatgump.cpp` 매 프레임 처리 ([bridge-pieces.md](exe/bridge-pieces.md) 6절) |
| `Bridges/BridgeReach.cs` | 다리 연결망이 닿는 섬 영역 계산. "내 다리 연결망 하나가 내 섬과 빈 섬에 함께 닿으면 그 빈 섬은 연결됨"(근사) | [island-ownership.md](gameplay/island-ownership.md) 규칙 4. 다른 빈 섬을 거치는 연쇄 연결은 미확인 |
| `Simulation/BattleSession.cs` (+ `.Commands.cs`) | **게임 세션**: 고정 틱 루프, 플레이어 상태, 엔티티, 명령 실행, 판정, 이벤트, 검사합 ([아래](#게임-세션-battlesession)) | 규칙 코어 전체 |
| `Simulation/BattleSessionFactory.cs` | 맵(.fort)·지면 미리보기·미션 시작 조건에서 세션 조립 (섬 칸, 다리 시작 불가 칸, 저장 다리) | 뷰어에 있던 초기화를 Core로 옮김 |
| `Simulation/GameCommands.cs` | 명령: 유닛 배치·건물 건설·지식 등록·회수·다리 조각 집기/되돌리기/놓기·오브젝트 선택·가이저 수집·화면 복귀 | |
| `Simulation/TutorialStages.cs` | **튜토리얼 단계 처리**: 튜토리얼 1 A~G, 튜토리얼 2 A~I 조건·안내 이벤트 | [priest-construction.md](exe/priest-construction.md), [mission-header-flags.md](exe/mission-header-flags.md) 3.5절 |
| `Simulation/BattleSession.Harvest.cs`·`MovementRate.cs`·`TutorialGeysers.cs` | 사제의 섬·다리 경로 탐색과 반복 왕복 수집, 타입별 `speed`, 저장 가이저가 없는 튜토리얼 1의 연습 받침 생성 | [priest-construction.md](exe/priest-construction.md) |
| `Simulation/GameEntity.cs`·`PlayerState.cs`·`SessionEvents.cs` | 오브젝트(.type 구동), 플레이어 상태(SP·덱·다리 칸·기술 표), 이벤트·실패 이유 | |
| `Rules/KnowledgeCatalog.cs` | 지식 창 카드 행: SUN·WIND·RAIN·THUN. 행, `.type` group 순서, 골렘 제외, `.fort` Technology 지식(목록 플래그 4) | [show-technology.md](exe/show-technology.md), 2026-09-30 녹화 |
| `Audio/MusicDirector.cs` | 배경음악 선택: 메뉴 ser22, 전투 원소 곡 순환(wind → rain → thunder → sun, 첫 곡 난수, 천둥 곡 thunderCrack), 내 희생 의식 음악과 곡 끝 복귀, 30초 이하 곡 180초 재확인, 결과 음악 잠금 | exe `FUN_00469fc0`·`00469f00`·`00469f60`·`00469db0`, [music.md](exe/music.md), 녹음 대조 |
| `Simulation/ConstructionTimes.cs` | 사제 건물 건설 시간: 템플 16초·워크샵 10초(관찰값, 이동 포함), 그 밖 10초(임시) | 튜토리얼 2 사용자 조작 관찰([screens/README.md](screens/README.md) 1.7절) |

## 발자국과 공급 범위 기하

* 원본 `FUN_0049ae80(type, rect, x, y)`는 타입의 `foot_x`(+0x1d4)·`foot_y`(+0x1d8)로 `(x − (foot_x − 1), y − (foot_y − 1))`~`(x, y)` 사각형을 만든다. 따라서 저장·배치 좌표는 발자국의 **오른쪽 아래 칸**이다. 이는 작은 받침에서 이미 확인한 "오른쪽 아래 칸 기준점"([terrain-and-bridges.md](exe/terrain-and-bridges.md))과 같다.
* `FUN_004730c0`은 배치 대상 사각형의 중심 `((x0 + x1) × 0.5, (y0 + y1) × 0.5)`과 크기 항 `((x1 − x0) × 0.7071)²`을 공급원 객체의 가상 함수(+0xA0)에 넘긴다. 공급원 쪽 판정 함수는 가상 호출이라 아직 찾지 못했다.
* 클론은 **공급원 발자국 중심과 배치 발자국 중심의 칸 거리² ≤ 반지름²**으로 판정한다. 크기 항이 판정을 넓히는지(예: 반지름 + 대상 반지름)는 **미확인**이다. 범위 경계 근처의 큰 유닛에서 원본과 1~2칸 차이가 날 수 있다.

## 게임 세션 (`BattleSession`)

지금까지의 정적 규칙(에너지·소유권·생산 창·다리)을 **하나의 고정 틱 루프**로 묶은 것이다. 화면·입력과 무관해서 헤드리스로 테스트하고,
멀티플레이·리플레이를 염두에 두고 "상태는 명령으로만 바뀐다"는 구조로 만들었다.

* **시간**: 24Hz 고정 틱(`FixedTimestep`). 화면은 `Advance(흐른 초)`를 부르고(밀린 시간은 한 번에 8틱까지만 따라잡는다), 테스트는 `RunTicks(n)`으로 정확히 진행한다.
  게임 시각 = 틱 ÷ 24. 다리 조각 채우기(1초)·붕괴(10초)·건설·재충전은 모두 틱으로 센다.
* **틱 순서(고정)**: 명령 실행(넣은 순서) → 사제 수집·이동 → 건설 완료 처리 → 튜토리얼 단계 처리 → 플레이어 번호 순 다리 칸 채우기 → 다리 붕괴.
* **명령** (`Submit`): `PlaceUnitCommand`(생산 창 유닛 배치)·`ConstructBuildingCommand`(사제 건물 건설)·`RegisterKnowledgeCommand`(워크샵 등록)·`SalvageCommand`(회수)·
  `PickBridgePieceCommand`·`ReturnBridgePieceCommand`·`PlaceBridgeCommand`(다리 조각; 회전은 화면이 관리해 놓을 때 값으로 보낸다)·
  `SelectEntityCommand`(오브젝트 선택/해제 — 튜토리얼 2 단계 C·F가 읽음)·`HarvestGeyserCommand`·`ReturnHomeCommand`.
  거부된 명령은 `CommandRejected` 이벤트(실패 이유 `CommandFailure` 포함)로 알린다. 화면은 `DrainEvents()`로 알림을 받는다.
* **판정(상태를 바꾸지 않음)**: `CheckUnit`·`CheckBuilding`·`CheckBridge`, 재충전 남은 시간 `SecondsUntilReady`, 건설 진행률 `ConstructionProgress`.
* **수집 경제**: 가이저를 지정하면 소유한 사제가 섬과 자기 다리 칸을 따라 가이저·완공 신전을 왕복한다. 신전에 결정 하나를 전달할 때마다 200 SP가 들어온다. 다리 연결이 바뀌면 경로를 다시 찾고 갈 수 없으면 작업을 멈춘다. 이동 속도는 각 유닛 `.type`의 `speed`를 읽는다. 현재 실제 경로 이동은 사제 수집에만 적용한다.
* **공중 공격**: Whirlibase는 별도 Whirligig를 생성하고 출발점 사거리·목표당 최대 3대·수송 제외 규칙으로 공격시킨다. 1분 출격 후 귀환·보급과 파괴 후 재생성을 구현했다. 정확한 시간·피해·이동 단위는 [추정표](gameplay/flyers.md)를 따른다. Rain/Wind 공격체는 후속이다.
* **비행형 수송(후속)**: 비행형 이동유닛은 출발할 때 떠오르고 이동 후 목적지에서 착륙한다(사용자 확인). 수송의 이륙·이동·착륙 상태와 시간·경로는 아직 없다. 공중 공격체와는 별도 구현 대상이다.
* **건설**: 비용은 시작할 때 나가고(원본 `00442c80` → `00442b50`의 배치 시 차감과 부합), 건설 시간이 지나야 규칙 효과가 생긴다 — **템플**: 섬 소유(빈 섬 → 내 섬)·에너지 공급원 등록·생산 창의 다리 조각/골렘 공급 시작,
  **워크샵**: 지식 등록 가능. 완공에 섬 소유 색이 바뀌는 것은 튜토리얼 2 관찰과 같다. 건설 중인 템플도 "플레이어당 1기" 판정에 센다.
* **회수**: 비용의 25%를 돌려받는다(튜토리얼 2: 300 → 75). 템플을 회수하면 섬이 빈 섬이 되고 다리 조각·골렘이 사라지며, 워크샵을 회수하면 그 워크샵의 등록이 사라진다. 사제·가이저·지형은 회수할 수 없다. 건물형 유닛(`maxHitPoints` 가 있고 이동체가 아닌 타입)이 없어지면 중심 ±2칸의 다리가 한 단계 약해진다(보통 → 금 감, 금 감 → 무너짐, 단단함 그대로). 원본 공통 제거 처리가 제거 이유를 보지 않아 회수에도 적용했다(원본 화면 미확인, [bridge-pieces.md](exe/bridge-pieces.md) 8.5절).
* **배치 뒤 재충전**: Unit Rate 표(10/5/1초, 기본 Fast 1초)만큼 그 유닛을 덱에서 다시 쓸 수 없다 (튜토리얼 2 관찰 1.1~1.2초와 부합).
* **미션 시작 조건**: 사람 플레이어(기본 1)에게 시작 SP·시작 지식·기술 허용 표를 적용하고, 튜토리얼 2는 전투 옵션(Short 14칸·Fast)을 덮어쓴다. `denySalvage`는 세션의 변하는 상태 `DenySalvage`로 시작한다.
* **생산 규칙 켜기/끄기** (`EnforceProductionRules`): 켜면 기술 허용 표·덱 등록·재충전·회수 금지를 명령에 적용한다. 끄면 규칙 조건(섬·자리·비용·에너지)만 본다(맵 뷰어 시험 모드).
* **결정론**: `Checksum()`(FNV-1a)이 틱·전역 난수·플레이어·오브젝트·다리 칸을 요약한다. 같은 시작·같은 명령열은 같은 값이다(테스트로 확인, 락스텝·리플레이 검증용 기반).
  플레이어 상태·오브젝트·다리 칸은 정렬된 순서로만 순회하고, 난수는 하나(`NetstormRandom`)를 모든 플레이어의 다리 칸이 번호 순으로 공유한다(원본도 전역 난수 하나를 공유).

### 튜토리얼 단계 처리 (`TutorialStages`)

미션 머리의 `techAllowed`(기술 허용 표)와 `denySalvage`는 **시작 값**이다. 원본의 튜토리얼 단계 처리(튜토리얼 2 = `FUN_004c3bb0`)가 실행 중에 바꾼다:
단계 B에서 sunFactory 허용, 단계 H에서 회수 금지 해제 ([근거](exe/mission-header-flags.md)). 세션이 이를 `TutorialStages`로 재현한다:

* 세션을 만들면 첫 단계 안내 `TutorialTell "A."` 이벤트가 나온다. 단계마다 그 단계의 스크립트 섹션 이름(`"B."` …)을 `TutorialTell` 이벤트로 알리고, 화면이 본문을 안내 창으로 띄운다(창은 아직 없다).
* 조건은 세션 상태만 본다: 지은 수(`PlayerState.Made`·`MadeWithFlags`, 누적 — 파괴·회수로 줄지 않는다), 워크샵 등록 여부, 선택한 오브젝트(`SelectedEntityId`), 이번 틱의 회수 이벤트, 타이머(단계 F 4초·H 2초·C의 `NotVortex` 2초 — exe 상수 값).
* 표·회수 금지·전투 옵션(Short·Fast)을 바꾸는 것도 단계 처리 몫이다: 단계 A 옵션 덮어쓰기, 단계 B `Tech.Set("sunFactory", true)`, 단계 H `DenySalvage = false`. 팩토리는 시작 시점에도 옵션을 덮어쓴다(단계 A 첫 프레임과 같은 결과).
* 튜토리얼 1은 F4 또는 첫 다리(A), 다리 8·19칸(B·C), 가이저 연결(D), 200·600 SP(E·F)로 G까지 진행한다. 원본은 다리의 **누적 제작 수**를 보지만 현재 클론은 살아 있는 내 다리 칸 수를 센다. 원본 위치 생성식도 아직 복원하지 못해 연습 가이저 받침을 결정적으로 만든다.
* `RunsTutorial = false`로 끌 수 있고, 구현되지 않은 튜토리얼(3~6)과 튜토리얼이 아닌 미션은 `Tutorial == null`이다. 단계·타이머·지은 수·선택·기술 표·회수 금지·사제 운반 상태는 `Checksum()`에 들어간다.
* 원본이 기술 허용 표를 확인하는 곳(메뉴 항목·덱)에 맞춰 세션도 Construct 판정(`CheckBuilding`)·지식 등록·덱 배치에서 표를 확인한다.
* 근사: 단계를 넘긴 뒤 원본이 열 번 세는 동안 다음 단계 처리를 멈추는 잠금(`+0x84`, 안내 창이 뜨고 닫힐 때까지로 추정)은 "다음 틱부터 검사"로 대신한다. 건물은 완공 시점에, 유닛은 놓는 시점에 지은 수로 센다(원본의 출생 콜백 시점은 스크립트 문구로 추정).

### 근사한 부분 (원본 확인 전)

* 건설 시간 = 관찰한 "클릭부터 완공"(사제 이동 포함) 값. 사제의 **건설 장소까지 이동**·정확한 `constructionRate` 계산·건설 자리에 서 있어야 하는지는 판정하지 않는다. 비용 차감은 배치 시점의 원본 경로를 정적으로 확인했다([분석](exe/priest-construction.md)).
* 수집 경로는 칸 단위·네 방향이며 가이저/신전 발자국의 인접 칸을 목표로 한다. 원본의 곡선 이동·다른 유닛과의 충돌·정확한 `speed` 시간 단위, 생성 가이저의 위치는 미확인이다.
* 유닛(생산 창 → 배치)은 건설 지연 없이 곧바로 완성으로 본다.
* 빈 섬 연결 = `BridgeReach`의 근사(위 표). 다리 끝 = 발자국 둘레의 내 다리 칸.
* 맵에 처음부터 있던 워크샵은 레벨 1, 등록 목록은 비어 있다(`.fort`의 `Deck`·`Technology` 섹션은 아직 연결하지 않았다).
* 세션 이벤트의 한국어 문구는 개발용이다(12단계 다국어 전).

## 게임 쪽 연결 — 맵 뷰어 배치 시험 모드

맵 뷰어에서 **P**를 누르면 플레이어 1로 워크샵 생산 유닛과 건물을 세션 명령으로 놓아 볼 수 있다([실행 안내](map-viewer.md#배치-시험-모드)). 판정은 세션의 `CheckUnit`·`CheckBuilding`(내부는 `BattleMap`) 하나로 한다. 결과로 발자국 칸(초록/빨강), 아군 공급원의 범위 원, 요구 에너지에 배정된 공급원까지의 선, 불가 이유, 원본 색 규칙의 Storm Power를 보여 준다.

검증(2026-09-29): Dissolved Alliance! 의 플레이어 1 섬 칸 (124,126)을 판정했다.
- Sail Skater(Wind 1 + 아무 1): 배치 가능. 이웃 Wind Generator 두 곳에 배정되었다.
- Bulf(Thunder 1): Thunder 공급원이 없어 에너지 부족.
- Thunder Cannon: 자리가 차 있어 불가.
- 스크린샷: `extracted/screens/placement-*.png`.

## 근사·미구현 (다음 분석 대상)

* **다리 연결·다리 끝**: 빈 섬 연결은 세션이 `BridgeReach`로 계산한다(근사). 다리 끝은 "발자국 둘레에 플레이어 다리 칸이 있는 섬 밖 위치"로 근사한다. 다리 조각 생성·회전·배치 판정·붕괴는 구현했고(`Bridges/`), 붕괴 시작 대기 조건과 영역 소유 이어짐은 `Bridge.cpp` 분석 후 교체한다([bridge-pieces.md](exe/bridge-pieces.md) 8절). `Deck.cpp`는 다리와 무관한 지식·생산 덱이다.
* **섬 칸 판정**: 지면 미리보기의 본섬 마스크(영역 번호 ≥ 0)를 쓴다. 작은 받침·유닛 발판(`createsisland`)은 섬 밖으로 본다. 기준점 칸 하나만으로 섬을 판정하며, 발자국 전체가 섬 위여야 하는지는 미확인이다.
* **지식·등록 상태**: 세션은 미션 `myTech`로 시작 지식을 채운다. `.fort`의 `Technology`·`Deck` 섹션으로 초기 지식·덱을 채우는 연결은 아직 없다(맵만 연 뷰어는 생산 규칙을 꺼서 모든 유닛을 놓을 수 있다).
* **동맹**: 기본은 같은 플레이어만 아군이다. 미션·멀티플레이 동맹 설정 연결은 남았다.
* **회수 금액**: 손상된 유닛의 감소 공식이 미확인이라 건강한 상태(25%)만 계산한다.
* **워크샵 생산 칸 수**: `GAME.HLP` 값(2/3/4)이다. 패치판 exe의 판정은 미확인이다.
