# webpj 운반 대상 접근(RawBoardTarget·RawCarrierTarget) 이식 — WB04-E100

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백 번째 부분이다. 운반 명령의 일반 접근점(41d530)·사각형 변 중간점(41d400)(`RawBoardTarget`)과 운반체의 대상 접근(4894e0)·귀환 접근(4276a0)·간헐천 이동(427600)·집으로 귀환(428fd0)(`RawCarrierTarget`)을 cpppj 기준선 `1f00a25a` 에서 옮겼다. E86~E98 에서 쓴 `RawSquidNeighbors.direction` 이 선행이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-board-target.js` — `BoardTargetState`, `RawBoardTarget(state, assertion)`

- `side(rectangle, side, flags)`(41d400): 0/4(북/남)는 가로 중간점과 위/아래 변, 2/6(동/서)는 오른쪽/왼쪽 변과 세로 중간점을 돌려준다. flags bit2 이면 사각형을 복사해 왼쪽·위·아래를 1 만큼 float 로 넓히고 오른쪽은 배정도(x87 53비트)로 넓힌다. 그 밖의 방향은 원본 진단(0x1f9)을 부른 뒤 flags 를 float 로 읽은 값(signaling NaN 은 quiet 로)을 두 좌표로 돌려준다. 결과를 지도 범위로 clamp 하지 않는다.
- `nearest(rectangle, source, flags)`(41d530): 사각형 중심(float 저장)을 향한 출발점의 방향을 4 칸 돌려 가장 가까운 변을 고른다. flags bit1 이 있고 방향이 0(북)이면 출발점이 중심 왼쪽일 때 서(6), 아니면 동(2)이다(순서 없는 값도 동쪽). flags bit0 이 있으면 `side` 결과이고, 없으면 진단(0x262) 뒤 현재 실패점(`state.invalid`)을 돌려준다.

`www/core/raw-carrier-target.js` — `RawCarrierTarget(pool, types, players, boss, board, hooks)`

- `startTarget(parent, target, replaceStart, helper)`: 대상 발자국(`footprint`)을 구한 뒤 부모의 현재 위치(발자국 효과 뒤)에서 가장 가까운 접근점으로 경로를 시작하고, 실패(0)하면 같은 사각형·인자를 유지한 채 북/동/남/서(0/2/4/6) 변의 중간점으로 최대 네 번 재시도한다(부모 위치를 다시 읽지 않는다).
- `goReturn(parent, target)`: 부모 소유자에게 AI 가 있으면(0 이 아닌 Player+0xb0 포인터를 매번 직접 읽음) 0 이다. 아니면 발자국 효과 전에 부모 좌표를 보관하고 접근점 한 번만 시도한다. `goGeyser(parent, target)`: 같은 AI 검사 뒤 대상이 free(bit0)이거나 TYPE flags2 에 간헐천 비트(0x10000000)가 없으면 0, 아니면 접근점 한 번만 시도한다(dead/소유자/특정 TYPE 번호 필터를 추가하지 않는다). `goHome(parent)`: 현재 boss 일 때만 귀환 후보(`chooseReturn`)를 구해 후보가 있으면 `startTarget(parent, 후보, 0, 0)` 이다(선택 효과가 권한을 바꿔도 다시 검사하지 않음).
- 경로 시작 결과는 원본 EAX 의 Path 주소 숫자(0 이면 실패)이다. 도달한 훅이 없으면 오류이다(`footprint·start·chooseReturn`).

## 2. 검증

- `tests/carrier-target.test.mjs`(2건):
  1. `carrier-target-x86.tsv` 2245행(입력 22 DWORD — 연산 0 변 중간점/1 일반 접근점/2 대상 접근/3 귀환/4 간헐천/5 집으로/6 초기 실패점·사각형·좌표·flags·AI 소유자·상태·재시도 성공 순번·변이 4종 등)과 실제 CPU 로 53/64비트 x87 정밀도 측정한 기하 결과 `carrier-target-hardware.tsv` 2040행. 연산 0/1 의 1020행은 이 실제 CPU 열을 기대값으로 쓰고 두 정밀도가 같음도 확인한다. 외부 효과 순서(! 진단/R 발자국/S 경로 시작 8값/C 귀환 후보)·상태(반환 주소·직접 결과 두 DWORD·실패점 두 DWORD·boss·시도 횟수)·두 슬롯(부모 50·대상 60) 전체 raw 가 같다.
  2. 계약: 조기 반환이 필요 없는 하위를 요구하지 않고 AI 판정은 현재 Player 포인터를 직접 읽으며, 진단 미연결·누락 훅·판본/Player 표 크기 오류를 거부한다.

최초 실패와 교정: 새 모듈은 2245행 중 252행까지 통과했고 253행에서 실패했다. 진단 콜백이 실패점 y 에 signaling NaN(0x7f800001)을 쓰는 입력에서 원본은 복사한 비트를 그대로 두는데 JS Number 는 quiet 비트를 보존하지 못해 0x7fc00001 이 되었다. 제품 코드의 오류가 아니라 표현의 한계이므로 시험이 trace/state 의 십진 토큰을 같은 규칙(지수 전부 1·가수 있음·quiet 비트 없음이면 quiet 비트 설정)으로 양쪽 정규화해 NaN 계열 일치만 확인하도록 고쳤다. 나머지 전 행은 비트까지 같다.

## 3. 경계와 남은 것

- 대상 발자국 조회(4ade70)·경로 시작(`RawPathStart` 489080)·귀환 후보(428ea0)는 훅 경계이다. 실패점의 sNaN quiet 비트는 JS 에서 표현할 수 없으며 실제 게임의 실패점은 −10000 이라 영향이 없다.

## 4. 다음 시작점

`RawCarrierChoice`·`RawCarrierEscape`(운반체 선택/탈출), `RawPathProcess`(RunFrame·도착; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`·`RawPathRetarget`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
