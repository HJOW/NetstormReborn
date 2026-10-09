# 게임 규칙 코어 (`Netstorm.Core`)

> 2026-09-29 정적 규칙 구현, 2026-09-30 게임 세션(고정 틱 루프·명령·플레이어 상태)으로 묶음.
> 사용자 확인·원본 매뉴얼·exe 분석을 기준으로 하며, 아직 부족한 전투 자료는 문서에 근사를 명시하고 구현한다.
> MonoGame에 의존하지 않으며 `dotnetpj/tests/Netstorm.Core.Tests`에서 헤드리스로 검사한다. 원본 게임은 실행하지 않았다.
> 사제의 가이저 왕복 수집과 튜토리얼 1 진행 조건을 구현했다. 2026-10-01 포대 전투·체력·파괴·사제 기절, 수송 사제 포획·제단 의식·미션 승패 이벤트를 추가했다.
> 전투에는 명시적인 근사가 있다([확정 근거·추정값·검증](gameplay/combat.md)).
> Whirlibase·Whirligig의 출격·이동·공격·귀환을 추가했다([비행체 계약](gameplay/flyers.md)). [캠페인 1-1](gameplay/campaign-one.md)·[1-2](gameplay/campaign-two.md)에 한정한 임시 방어 AI·골렘 수확·일반 이동/정지·워크샵 업그레이드 명령을 추가했다. 원본 전략 복원, 다른 공중 공격 유닛과 비행 수송의 이륙·착륙 연출은 후속이다.
> 2026-10-07: 다리 추첨·난수·전체 패턴·프레임 검색·설정을 C++의 원본 x86 기대값에 연결했다. 설정·타입·요새·SHP 자료 계층 정정과 검증 범위는 [dotnetpj 복원 기록](dotnet-reconstruction-20261007.md) 참조. 전투·AI·붕괴 스캔의 근사는 유지한다.
> 2026-10-09: 다리의 열린 방향·방문 목록·수명 계산을 x86 기대값으로 검증했다. 후속으로 공유 `SidPool`과 `BridgeDecayScan`을 세션에 연결해 틱마다 SID 구간을 처리한다. `SurfaceGraph`·`SpotRules`·`DrawOrder`의 월드 연결은 아직 남았고 그래프 크기는 칸 수 근사다. 원본의 전체 생성 순서·동적 받침·투사체 SID는 미복원이므로 절대 스캔 위상은 차이 날 수 있다. [계산 복원](dotnet-reconstruction-20261009.md)·[SID 세션 연결](dotnet-sid-session-20261009.md) 참조.

> 2026-10-09 최근 변경 점검: 화면 밖 선택/체력 상자의 투영 순서를 수정했다. Release 경고/오류 0·검사 **888개 통과**, TEST01·전투·메뉴 클론 창 검사 통과. `.chfnt` 판독기는 18개 캐시 검증 완료이며 원본 영어 UI 연결은 후속이다. [점검과 제한](dotnet-review-20261009.md).

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
| `Bridges/BridgeGrid.cs` | 놓인 다리 칸의 연결망·배치 판정(겹침 불가, 섬 가장자리·내 다리 열린 끝에 이어짐)·SID 구간을 틱마다 나누는 10초 주기 칸 단위 붕괴(끝 칸만 구동자, 접합 칸 무리는 함께, 섬에서 떨어진 5칸 미만 조각은 즉시, 수명 7→0, 5 아래 금 감)·조각 품질별 시작 상태(금 감 = 수명 4)·건물형 유닛이 없어질 때 주변 ±2칸 한 단계 약화(`WeakenAround`) | `Bridge.cpp` `00422bc0`·`004227e0`·`00421c30`, `Rifttype.cpp` `0049b510`, `Construction.cpp` `00442c80`, 공통 제거 처리 `0044b9e0` ([bridge-pieces.md](exe/bridge-pieces.md) 8절). 한 칸 처리는 원본 기대값과 일치(2026-10-09). 이어짐 판정·그래프 크기·전체 원본 생성 순서는 근사 |
| `Bridges/BridgeSurfaceRules.cs`, `Bridges/SurfaceFinder.cs` | 표면 연결 판정·열린 방향·붕괴 방문 목록, 표면 번호 지도의 오브젝트 단위 이웃 탐색 (월드와 분리한 계산) | `00441e40`·`00421770`·`004217f0`·`004218b0`, `004b23e0` 외. 원본 x86 기대값 9,210개 |
| `Bridges/BridgeDecayRules.cs` | 수명 감소·금 간/보통 프레임 전환·약화/복구·destroy 재정의·스캔의 한 칸 처리, 붕괴 스캔 커서(`BridgeDecayScan`) | `00421c30`·`00421b60`·`00421bd0`·`00421db0`·`00421e60`·`004220f0`·`004227e0`·`00422bc0`. 원본 x86 기대값 15,004개 |
| `Bridges/SurfaceGraph.cs` | 표면 그래프 표(무리 번호와 표면 수): 할당·반납·감소·flood·등록·삭제 준비·전체 재구성. 세션에는 미연결 | `Graph.cpp` `00463330` 외. 원본 x86 기대값 2,560개 |
| `Simulation/SidPool.cs` | 오브젝트 번호 할당기(영역·FIFO·예약 꼬리·예측 커서). 저장 오브젝트·noIsland·다리·생산·건설·비행체가 공유하는 서버 풀 | `Squid.cpp` `004af1d0` 외, 120000슬롯 초기화. 원본 x86 기대값 3,168개. [연결 기록](dotnet-sid-session-20261009.md) |
| `Rules/SpotRules.cs`·`Rules/SpotMap.cs` | 칸별 점유 비트(발자국 안쪽 비트 8)와 공간 해시 단계, 256×256 점유 지도의 발자국 OR 등록·AND 해제(패치 중복 조기반환·CD 계속, 행 우선·표면/매몰/포함 필터). 세션·배치 판정에는 미연결 | `004afd30`·`004ace40`·`004b02d0`·`004afe50`. 원본 x86 기대값 3,821개 + 지도 단위 검사 7개 |
| `Display/DrawOrder.cs` | 그리기 순서 비교(깊이 내림차순 → y → x)와 안정 정렬. 저장 오브젝트의 초기 키(`FortMapViewer._sorted`)에 연결. 이동 후/새 객체·지면·다리의 전체 정렬은 후속 | `00497900`. 원본 x86 기대값 261개. 클론 창 ui·test01 스모크 통과 |
| `Display/DisplayBounds.cs` | 화면 투영(trunc(x × 16 + 0.5) − 카메라)·Q16 hotspot·표시 폭·높이 + 1 의 경계, 표시 영역 자르기·선택 확장(패치 위 15·CD 위 9)·그림자 프레임 번호. 체력 막대·선택 괄호 상자(`FortMapViewer.EntityFrameBox`)에 연결 | `00498ff0`·`004990d0`. 원본 x86 기대값 368개 + 1배 84행의 카메라 이동 전/후 168개 대조. `BoundsInView`는 투영 뒤 카메라/확대를 적용하며 그림 위치는 기존 VFX 기준 유지. 클론 창 ui·combat·test01 스모크 통과 |
| `Bridges/BridgeAnchors.cs` | 다리를 시작할 수 없는 섬 칸: 가장자리 초목(edgeFarm) 칸 + dropBlocking 타입 오브젝트 발자국. 2026-10-07 타입 후처리를 연결하여 포대·궁수 등 모든 emplacement도 이 비트를 가진다 | edgefarm.type `dropBlocking`, 타입 후처리 `0049b0d0`, `Squid.cpp` `004b02d0` 스폿 비트 0x10, `Rifttype.cpp` `0049b510` ([bridge-pieces.md](exe/bridge-pieces.md) 8.4절) |
| `Rules/MissionStart.cs` | 미션 머리 값 → 시작 SP(myStartMoney, 없으면 전투 옵션)·시작 지식(myTech)·기술 허용 표(techAllowed: deny/allow/all 순서 적용, **실행 중 `Set`·`SetAll`로 바뀜**)·denySalvage 시작 값 등. 머리 값은 시작 상태이고 튜토리얼 단계 처리가 실행 중에 바꾼다. **AI 플레이어**는 번호가 붙은 `aiNStartMoney`·`aiNTech`·`aiNAllyList`·`aiNColor`를 쓰고, 플레이어 2는 번호 없는 구식 키(`aiStartMoney` 등)를 대신 쓸 수 있으며, 시작 SP가 없으면 0이다(`AiStormPower`·`AiKnowledgeFor`) | `Mission.cpp` `00482eb0`, `Totalmade.cpp` `004c23c0`~`004c2400`, 시작 조건 읽기 `004c2b20`→`004c2d40`, [mission-header-flags.md](exe/mission-header-flags.md), 튜토리얼 1·2 원본 관찰. 네 미션의 플레이어 8명을 cpppj `--dump-world`와 대조([2026-10-10 기록](dotnet-reconstruction-20261010.md)) |
| `Rules/GameCursor.cs` | 원본 커서 모양 18가지(값 = 원본 커서 번호 1~18)와 번호 → 커서 그룹 → 그림 리소스 번호 표(`GameCursors`). 쓰임이 확인된 것은 화살표·배치·템플·금지·이동 다섯 가지이고 나머지 13개는 그림만 연결했다 | 원본 표 `005423a8`, `originals/Netstorm.exe` 의 `RT_GROUP_CURSOR`. 상황별 선택(UserInput `004d62b0`)은 미복원 |
| `Bridges/BridgeCursor.cs` | 커서 → 들고 있는 조각의 왼쪽 위 칸: (⌊(x + 7) / 16⌋, ⌊y / 11⌋), 크기·회전 무관 | 원본 실행 측정 ([bridge-pieces.md](exe/bridge-pieces.md) 4절) |
| `Bridges/BridgeFrames.cs` | 칸 → bridge.type 프레임 (보통 / 금 감 +10 / 단단함 20) | `0049a940`, bridge.type 주석 |
| `Bridges/BridgeTray.cs` | 생산 창 다리 칸(자리 고정: 집은 조각은 놓을 때까지 칸에 남고 새 조각은 빈 칸에 — [clone-deck.md](screens/clone-deck.md)): 템플이 있으면 1초마다, Bridge Slots 칸까지, 5번째 추첨마다 한 칸 조각, 템플을 잃으면 비움, 새 조각은 6초 동안 금 간 품질(`QualityAt`) | `Combatgump.cpp` 매 프레임 처리 ([bridge-pieces.md](exe/bridge-pieces.md) 6절) |
| `Bridges/BridgeReach.cs` | 다리 연결망이 닿는 섬 영역 계산. "내 다리 연결망 하나가 내 섬과 빈 섬에 함께 닿으면 그 빈 섬은 연결됨"(근사) | [island-ownership.md](gameplay/island-ownership.md) 규칙 4. 다른 빈 섬을 거치는 연쇄 연결은 미확인 |
| `Simulation/BattleSession.cs` (+ `.Commands.cs`) | **게임 세션**: 고정 틱 루프, 플레이어 상태, 엔티티, 명령 실행, 판정, 이벤트, 검사합 ([아래](#게임-세션-battlesession)) | 규칙 코어 전체 |
| `Simulation/BattleSessionFactory.cs` | 맵(.fort)·지면 미리보기·미션 시작 조건에서 세션 조립 (섬 칸, 다리 시작 불가 칸, 저장 다리) | 뷰어에 있던 초기화를 Core로 옮김 |
| `Simulation/GameCommands.cs` | 명령: 유닛 배치·건물 건설·지식 등록·회수·다리 조각 집기/되돌리기/놓기·오브젝트 선택·가이저 수집·화면 복귀·사제 포획/운반/내려놓기·알타 이동 | |
| `Simulation/TutorialStages.cs` | **튜토리얼 단계 처리**: 튜토리얼 1 A~G, 튜토리얼 2 A~I 조건·안내 이벤트 | [priest-construction.md](exe/priest-construction.md), [mission-header-flags.md](exe/mission-header-flags.md) 3.5절 |
| `Simulation/BattleSession.Harvest.cs`·`MovementRate.cs`·`TutorialGeysers.cs` | 사제의 섬·다리 경로 탐색과 반복 왕복 수집, 타입별 `speed`, 저장 가이저가 없는 튜토리얼 1의 연습 받침 생성 | [priest-construction.md](exe/priest-construction.md) |
| `Simulation/BattleSession.Movement.cs`·`BattleSession.Support.cs` | 수확·수송의 공통 경로 상태·재탐색·대기/재개, 지상 낙하·허공 사제 기절과 발판 복귀 | [이동 경로 구현·추정표](gameplay/movement-pathing.md) |
| `Simulation/BattleSession.Sacrifice.cs` | 타입 속도에 따른 수송·사제 이동, 기절 사제 포획·제단 운반·구출, 다섯 룬 의식, 제단 파괴·승패 이벤트 | [희생 의식 구현·근거](gameplay/sacrifice.md), [music.md](exe/music.md) |
| `Simulation/GameEntity.cs`·`PlayerState.cs`·`SessionEvents.cs` | 오브젝트(.type 구동), 플레이어 상태(SP·덱·다리 칸·기술 표), 이벤트·실패 이유 | |
| `Rules/KnowledgeCatalog.cs` | 지식 창 카드 행: SUN·WIND·RAIN·THUN. 행, `.type` group 순서, 골렘 제외, `.fort` Technology 지식(목록 플래그 4) | [show-technology.md](exe/show-technology.md), 2026-09-30 녹화 |
| `Audio/MusicDirector.cs` | 배경음악 선택: 메뉴 ser22, 전투 원소 곡 순환(wind → rain → thunder → sun, 첫 곡 난수, 천둥 곡 thunderCrack), 내 희생 의식 음악과 곡 끝 복귀, 30초 이하 곡 180초 재확인, 결과 음악 잠금 | exe `FUN_00469fc0`·`00469f00`·`00469f60`·`00469db0`, [music.md](exe/music.md), 녹음 대조 |
| `Simulation/ConstructionTimes.cs` | 사제가 현장에 **도착한 뒤** 건물이 완성되기까지 10초(모든 건물 공통, `constructionRate` 10) | 2026-10-03 원본 자동 분석(워크샵 걷기 제외 10~11초), `.type constructionRate`, 웹 팬게임 ([건설 흐름](gameplay/priest-construction-flow.md)) |
| `Simulation/BattleSession.Construction.cs` | 사제 건설 흐름: 설치 때 비용 차감·공사장 → 사제 이동 → 도착 때 건설 시작, 도착 전 중단 시 환불 취소, Construct 메뉴 제한(`GetBuildingRestriction`: 템플·알타 1기·기술·워크샵 지식) | [건설 흐름](gameplay/priest-construction-flow.md) |

## 발자국과 공급 범위 기하

* 원본 `FUN_0049ae80(type, rect, x, y)`는 타입의 `foot_x`(+0x1d4)·`foot_y`(+0x1d8)로 `(x − (foot_x − 1), y − (foot_y − 1))`~`(x, y)` 사각형을 만든다. 따라서 저장·배치 좌표는 발자국의 **오른쪽 아래 칸**이다. 이는 작은 받침에서 이미 확인한 "오른쪽 아래 칸 기준점"([terrain-and-bridges.md](exe/terrain-and-bridges.md))과 같다.
* `FUN_004730c0`은 배치 대상 사각형의 중심 `((x0 + x1) × 0.5, (y0 + y1) × 0.5)`과 크기 항 `((x1 − x0) × 0.7071)²`을 공급원 객체의 가상 함수(+0xA0)에 넘긴다. 공급원 쪽 판정 함수는 가상 호출이라 아직 찾지 못했다.
* 클론은 **공급원 발자국 중심과 배치 발자국 중심의 칸 거리² ≤ 반지름²**으로 판정한다. 크기 항이 판정을 넓히는지(예: 반지름 + 대상 반지름)는 **미확인**이다. 범위 경계 근처의 큰 유닛에서 원본과 1~2칸 차이가 날 수 있다.

## 게임 세션 (`BattleSession`)

지금까지의 정적 규칙(에너지·소유권·생산 창·다리)을 **하나의 고정 틱 루프**로 묶은 것이다. 화면·입력과 무관해서 헤드리스로 테스트하고,
멀티플레이·리플레이를 염두에 두고 "상태는 명령으로만 바뀐다"는 구조로 만들었다.

* **시간**: 24Hz 고정 틱(`FixedTimestep`). 화면은 `Advance(흐른 초)`를 부르고(밀린 시간은 한 번에 8틱까지만 따라잡는다), 테스트는 `RunTicks(n)`으로 정확히 진행한다.
  게임 시각 = 틱 ÷ 24. 다리 조각 채우기(1초)·붕괴(10초)·건설·재충전은 모두 틱으로 센다.
* **틱 순서(고정)**: 명령 실행(넣은 순서) → AI → 수집 → 수송·사제 이동(건설 현장 도착 포함) → 도착 전 공사장 점검 → **생산 자원 운송·실체화** → 건설 완료 → 전투 → 제단 의식 → 튜토리얼 단계 → 플레이어 번호 순 다리 칸 채우기 → 다리 붕괴 → **지상 낙하·허공 사제 복귀** → 미션 승패 이벤트.
* **명령** (`Submit`): `PlaceUnitCommand`(생산 창 유닛 배치)·`ConstructBuildingCommand`(사제 건물 건설 — 맡을 사제 번호 선택, 0 이면 첫 자유 사제)·`RegisterKnowledgeCommand`(워크샵 등록)·`SalvageCommand`(회수)·
  `PickBridgePieceCommand`·`ReturnBridgePieceCommand`·`PlaceBridgeCommand`(다리 조각; 회전은 화면이 관리해 놓을 때 값으로 보낸다)·
  `SelectEntityCommand`(오브젝트 선택/해제 — 튜토리얼 2 단계 C·F가 읽음)·`HarvestGeyserCommand`·`ReturnHomeCommand`·`CapturePriestCommand`·`DeliverPriestCommand`·`DropPriestCommand`·`MovePriestToAltarCommand`.
  거부된 명령은 `CommandRejected` 이벤트(실패 이유 `CommandFailure` 포함)로 알린다. 화면은 `DrainEvents()`로 알림을 받는다.
* **판정(상태를 바꾸지 않음)**: `CheckUnit`·`CheckBuilding`(사제를 지정하면 사제가 그 자리 둘레까지 걸어갈 수 있는지도 본다)·`CheckBridge`, Construct 메뉴 줄의 제한 `GetBuildingRestriction`, 재충전 남은 시간 `SecondsUntilReady`, 건설 진행률 `ConstructionProgress`(사제 도착 전 0).
* **수집 경제**: 가이저를 지정하면 소유한 사제가 섬과 자기 다리 칸을 따라 가이저·완공 신전을 왕복한다. 신전에 결정 하나를 전달할 때마다 200 SP가 들어온다. 지형이 바뀌면 경로를 다시 찾고, 길이 없으면 목표를 유지해 기다리다가 복구 후 재개한다. 기절·포획·목표 제거 시 취소한다. 이동 속도는 각 유닛 `.type`의 `speed`를 읽으며 수확·수송·사제 이동은 같은 이동 상태 코드를 쓴다.
* **지지와 낙하**: 발밑 섬·받침·다리가 사라지면 지상 보행 유닛을 그 틱 끝에 제거한다. 사제는 현재 HP·위치·결정을 유지해 허공에서 기절하고 점유를 비운다. 비행 수송으로 포획하거나 다리를 복구할 수 있으며, HP가 절반 이상이면 발판 복구로 회복한다. `createsisland` 받침은 건물 생존·완공 상태를 따라 생성/소멸하고 지형 버전을 갱신한다. 세부 미확인 정책은 [이동 경로 문서](gameplay/movement-pathing.md#34-낙하-규칙-구현과-임시-정책)에 남겼다.
* **공중 공격**: Whirlibase는 별도 Whirligig를 생성하고 출발점 사거리·목표당 최대 3대·수송 제외 규칙으로 공격시킨다. 1분 출격 후 귀환·보급과 파괴 후 재생성을 구현했다. 정확한 시간·피해·이동 단위는 [추정표](gameplay/flyers.md)를 따른다. 다른 공중 공격체는 후속이다.
* **수송·희생 의식**: 수송 유닛별 `.type speed`로 이동하고, 기절한 적 사제를 싣고 알타에 내려놓는다. 내 사제가 알타 옆에 도착하면 다섯 룬 의식이 시작되고, 대상 팀 사제가 제거되면 미션 이벤트를 한 번 알린다. 비행 수송의 이륙·착륙 연출과 일부 시간·체력 임계값은 추정이다 ([계약·근거](gameplay/sacrifice.md)).
* **건설**([건설 흐름·근거](gameplay/priest-construction-flow.md)): 설치 명령에서 **비용을 바로 차감**하고(원본 `00442c80` → `00442b50`의 배치 시 차감과 부합) 그 자리에 공사장(`AwaitingBuilder`)을 놓는다. 맡은 사제가 현장 둘레까지 **걸어가 도착해야 건설 시간(10초)이 시작**되고, 끝나야 규칙 효과가 생긴다 — **템플**: 섬 소유(빈 섬 → 내 섬)·에너지 공급원 등록·생산 창의 다리 조각/골렘 공급 시작,
  **워크샵**: 지식 등록 가능. 완공에 섬 소유 색이 바뀌는 것은 튜토리얼 2 관찰과 같다. 도착 전에 사제가 다른 명령(이동·정지·수확·희생)을 받거나 기절·포획·사망하면 공사장은 **비용 전액 환불**과 함께 사라진다(건설이 시작된 뒤에는 사제가 떠나도 이어진다). 사제가 걸어갈 길이 없는 자리는 지을 수 없다.
  건설 중이거나 사제를 기다리는 템플·알타도 "플레이어당 1기" 판정에 센다. 우클릭 Construct 메뉴는 이미 템플이 있으면 Temple 줄을, 기술이 막히거나 그 원소의 지식(발전기 제외)이 없으면 워크샵 줄을 어둡게 한다.
* **회수**: 비용의 25%를 돌려받는다(튜토리얼 2: 300 → 75). 템플을 회수하면 섬이 빈 섬이 되고 다리 조각·골렘이 사라지며, 워크샵을 회수하면 그 워크샵의 등록이 사라진다. 사제·가이저·지형은 회수할 수 없다. 건물형 유닛(`maxHitPoints` 가 있고 이동체가 아닌 타입)이 없어지면 중심 ±2칸의 다리가 한 단계 약해진다(보통 → 금 감, 금 감 → 무너짐, 단단함 그대로). 원본 공통 제거 처리가 제거 이유를 보지 않아 회수에도 적용했다(원본 화면 미확인, [bridge-pieces.md](exe/bridge-pieces.md) 8.5절).
* **배치 뒤 재충전**: Unit Rate 표(10/5/1초, 기본 Fast 1초)만큼 그 유닛을 덱에서 다시 쓸 수 없다 (튜토리얼 2 관찰 1.1~1.2초와 부합).
* **유닛 생산**([운송·실체화 규칙](gameplay/unit-production-flow.md)): 배치 때 비용·자리를 예약하고, 덱 등록에 사용한 워크샵(골렘은 템플)의 스톰 파워와 필요한 원소 공급원의 에너지가 모두 도착한 뒤 실체화한다. 활성화 전에는 이동·수집·전투·공급·받침 효과가 없다. 줄기는 길이 끊기면 현재 위치에서 공중 직선으로 전환한다. 목적지 발판까지 사라지면 전액 환불한다. 맵의 기존 유닛과 생산 규칙을 끈 개발용 배치는 즉시 완성 상태다.
* **미션 시작 조건**: 사람 플레이어(기본 1)에게 시작 SP·시작 지식·기술 허용 표를 적용하고, 튜토리얼 2는 전투 옵션(Short 14칸·Fast)을 덮어쓴다. `denySalvage`는 세션의 변하는 상태 `DenySalvage`로 시작한다. AI 플레이어는 미션의 `aiNStartMoney`(없으면 0)와 `aiNTech`로 시작한다 — 2026-10-10 이전에는 공개 캠페인 미션(1-1·1-2)에만 적용했다. 맵에 저장된 `Money`는 미션 시작 값에 쓰지 않는다(미션 없이 맵만 열 때만 쓴다).
* **저장 오브젝트의 소유자**: 세션은 `FortObject.LoadOwner`를 쓴다. 저장 소유자 바이트가 0이거나 8보다 크면 1로 바꾸고(원본 `004bdc60`), geyser·buried·island 타입은 0(중립)이다. 소유자를 저장하지 않는 타입은 0으로 둔다(원본은 호출자가 준 영역 소유자를 쓰며, 이 경로는 미복원이다). 네 미션 2,036개 오브젝트가 cpppj 월드의 소유자와 같다.
* **생산 규칙 켜기/끄기** (`EnforceProductionRules`): 켜면 기술 허용 표·덱 등록·재충전·회수 금지를 명령에 적용한다. 끄면 규칙 조건(섬·자리·비용·에너지)만 본다(맵 뷰어 시험 모드).
* **결정론**: `Checksum()`(FNV-1a)이 틱·전역 난수·플레이어·오브젝트·다리 칸을 요약한다. 같은 시작·같은 명령열은 같은 값이다(테스트로 확인, 락스텝·리플레이 검증용 기반).
  플레이어 상태·오브젝트·다리 칸은 정렬된 순서로만 순회하고, 난수는 하나(`NetstormRandom`)를 모든 플레이어의 다리 칸이 번호 순으로 공유한다(원본도 전역 난수 하나를 공유).

### 튜토리얼 단계 처리 (`TutorialStages`)

미션 머리의 `techAllowed`(기술 허용 표)와 `denySalvage`는 **시작 값**이다. 원본의 튜토리얼 단계 처리(튜토리얼 2 = `FUN_004c3bb0`)가 실행 중에 바꾼다:
단계 B에서 sunFactory 허용, 단계 H에서 회수 금지 해제 ([근거](exe/mission-header-flags.md)). 세션이 이를 `TutorialStages`로 재현한다:

* 세션을 만들면 첫 단계 안내 `TutorialTell "A."` 이벤트가 나온다. 단계마다 그 단계의 스크립트 섹션 이름(`"B."` …)을 `TutorialTell` 이벤트로 알리고, 화면이 본문을 안내 창으로 띄운다. `MissionTell`도 대응 미션 스크립트 섹션이 있으면 같은 안내 창으로 연결된다.
* 조건은 세션 상태만 본다: 지은 수(`PlayerState.Made`·`MadeWithFlags`, 누적 — 파괴·회수로 줄지 않는다), 워크샵 등록 여부, 선택한 오브젝트(`SelectedEntityId`), 이번 틱의 회수 이벤트, 타이머(단계 F 4초·H 2초·C의 `NotVortex` 2초 — exe 상수 값).
* 표·회수 금지·전투 옵션(Short·Fast)을 바꾸는 것도 단계 처리 몫이다: 단계 A 옵션 덮어쓰기, 단계 B `Tech.Set("sunFactory", true)`, 단계 H `DenySalvage = false`. 팩토리는 시작 시점에도 옵션을 덮어쓴다(단계 A 첫 프레임과 같은 결과).
* 튜토리얼 1은 F4 또는 첫 다리(A), 다리 8·19칸(B·C), 가이저 연결(D), 200·600 SP(E·F)로 G까지 진행한다. 단계 B·C 는 살아 있는 칸이 아니라 놓은 칸의 누적 수(`FUN_004c2500(4)`, 정밀 디컴파일 004c3a20 확인)를 본다. 명령 경로로 놓은 칸마다 `PlayerState.Made` 에 다리 칸을 넣고 `TutorialStages` 가 그 합으로 판정한다. 원본 위치 생성식도 아직 복원하지 못해 연습 가이저 받침을 결정적으로 만든다.
* `RunsTutorial = false`로 끌 수 있고, 구현되지 않은 튜토리얼(3~6)과 튜토리얼이 아닌 미션은 `Tutorial == null`이다. 단계·타이머·지은 수·선택·기술 표·회수 금지·사제 운반 상태는 `Checksum()`에 들어간다.
* 원본이 기술 허용 표를 확인하는 곳(메뉴 항목·덱)에 맞춰 세션도 Construct 판정(`CheckBuilding`)·지식 등록·덱 배치에서 표를 확인한다.
* 근사: 단계를 넘긴 뒤 원본이 열 번 세는 동안 다음 단계 처리를 멈추는 잠금(`+0x84`, 안내 창이 뜨고 닫힐 때까지로 추정)은 "다음 틱부터 검사"로 대신한다. 건물은 완공 시점에, 유닛은 자원 도착 후 실체화 완료 시점에 지은 수로 센다(원본의 정확한 출생 콜백 시점은 미확인).

### 근사한 부분 (원본 확인 전)

* 건설 시간 = 사제 도착 뒤 10초(워크샵 관찰 10~11초, `constructionRate` 10, 웹 팬게임 10초). 정확한 `constructionRate` 계산식은 미분석이라 모든 건물에 같은 값을 쓴다. 건설이 시작된 뒤 사제가 떠나도 건설이 이어지는지, 건설 중 사제를 새로 부릴 수 있는지는 미확인이다(팬게임 가정). 비용 차감은 배치 시점의 원본 경로를 정적으로 확인했다([분석](exe/priest-construction.md)).
* 이동 경로는 섬 위 8방향(대각선 우선)·다리 위 4방향이며 가이저/신전 발자국의 인접 칸을 목표로 한다. 대각선 걸음은 √2칸 길이다. 세션 위치는 칸 단위이고 화면이 칸 사이를 보간해 그린다([이동 3.6절](gameplay/movement-pathing.md#36-클론-반영-부드러운-8방향-이동방향-그림그림-모양-클릭-2026-10-03)). 다른 유닛과의 충돌·생성 가이저의 위치는 미확인이다.
* 유닛(생산 창 → 배치)은 건설 지연 없이 곧바로 완성으로 본다.
* 알타의 "14.5초"는 캠페인 1-5 영상에서 클릭부터 완공까지 잰 값(사제 이동 포함)이다. 이제 이동을 따로 모델링하므로 도착 뒤 10초로 바꿨다. 알타 1기 제한은 웹 팬게임 규칙이며 원본에서 직접 확인하지 못했다. 제단 의식의 룬 주기·희생·소멸 지연과 이벤트 보정은 [sacrifice.md](gameplay/sacrifice.md)에 근거와 함께 적었다.
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
* **동맹**: 미션 동맹 목록은 연결했다. 멀티플레이 동맹 협상은 구현하지 않았다.
* **회수 금액**: 손상된 유닛의 감소 공식이 미확인이라 건강한 상태(25%)만 계산한다.
* **워크샵 생산 칸 수**: `GAME.HLP` 값(2/3/4)이다. 패치판 exe의 판정은 미확인이다.
* **이동 경로·생산 에너지 줄기 후속**: 끊긴 길 재탐색·대기·복구 재개와 낙하·허공 사제 복귀는 구현했다. 금 간 칸 가중치·충돌 회피는 후속이다(일반 이동 명령은 2026-10-03 구현). 생산 Stream of Power·원소 에너지 운송·공중 전환·도착 후 실체화는 **2026-10-04 구현 완료**했다. 원본 입자·정확한 시간·취소 규칙의 대조는 후속이다 — [생산 흐름](gameplay/unit-production-flow.md).
