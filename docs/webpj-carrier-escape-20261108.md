# webpj 운반체 회피(RawCarrierEscape) 이식 — WB04-E101

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백한 번째 부분이다. 운반체(Carrier)의 네 방향 회피와 주변 surface 후보 경로 시간 비교(429010)를 cpppj 기준선 `1f00a25a` 의 `o/RawCarrierEscape` 에서 옮겼다. E98 의 `raw-path-runtime.js`(임시 경로·Prepare)와 일반 finder(`raw-squid-finder.js`)·해시(`squid-hash.js`)가 선행이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-carrier-escape.js` — `CarrierEscapeState`, `RawCarrierEscape(pool, hash, types, spots, now, checking, cardinalTypes, state, hooks)`

- `run(parent, distance, event)`: 현재 이동 회피 깊이(경로가 있으면 그 깊이, 없으면 0)에 1을 더한 값이 3 을 넘거나, 거리 1 인 비행체(TYPE flags2 0x20000)이면 1000 이다. 깊이 진단은 검사 전역이 켜졌을 때 깊이가 음수이면 0x87b, 3 초과이면 0x87c 이며 진단 뒤 계속한다.
- 방향 회피(event 가 0x1da1a 가 아닐 때): `event/2`(event 가 −1 이면 난수(0, 5000)/1000)번 방향부터 4 번 signed 나머지로 회전하며, 방향표(17 DWORD; 첫 난수 방향 4 의 중복 서쪽도 원본 그대로)로 `방향×거리+출발점` 목적지를 float 로 저장해 만든다. spot bit4(0x10)로 막히지 않고 새 경로 시작(`start`)이 성공하면 그 경로에만 깊이(+1)를 쓰고 1001 을 돌려준다. 네 번 모두 실패해도 1001 이다.
- 주변 탐색(event 0x1da1a): 난수를 먼저 소비한 뒤, 출발점 정수 중심(41d7a0 snap 0.99999f 후 다시 ftol)의 ±5 영역을 실제 일반 finder(`begin(area, 0)`)로 훑어 surface 후보(flags1 0x800) 중 spot 에 막히지 않고 후보 위치에 운반체가 없는(`blocker`) 후보마다 임시 경로를 만든다. cardinal TYPE 4 종이면 BFS flags 1 로 `search` 를 부르고 경로가 있으면 현재 이동 속도(`movement`)로 현재 시각 기준 Prepare 를 해서 `마지막 꼭짓점 시각 − 현재 시각`이 지금까지의 최소(float 저장, 초기 10000)보다 작으면 선택한다. 선택점이 유효(`valid`)하면 새 경로 시작(깊이 기록)을 하고 1001 이다. 선택이 없으면 초기 선택점(−10000, −10000)을 그대로 유효 검사(`valid`)에 넘긴다.
- `Blocked`: 좌표에 단정도 0.9999 를 더해 ftol(signed64 하위 DWORD)한 두 정수로 감기는 선형 spot 주소(y×256+x)의 bit4 이며 보드로 clamp 하지 않고 65536 밖은 오류이다. 시각·검사 전역·경로 포인터는 외부 효과 뒤 현재 값을 다시 읽는다.
- 도달한 훅이 없으면 오류이다(`path·random·start·blocker·search·movement·valid·assertion`, `scan` 은 관측용 선택). 10.72(CD) 판본은 거부한다.

## 2. 검증

- `tests/carrier-escape.test.mjs`(2건):
  1. `carrier-escape-x86.tsv` 1807행(10.78 전용 단일 판본, 53/64비트 열 동일). 입력 20 값(거리 float·event·초기 깊이·경로 유무·검사 전역·genus·부모 TYPE·spot·시작 성공 순번·난수·후보 체인·후보 flags·운반체 마스크·경로 마스크/모드·변이 번호·부모 좌표·이동 속도·현재 시각 비트)으로 반환(1000/1001), 외부 효과 순서(P 경로/R 난수/S 새 경로 시작/F 탐색 영역/O 후보 운반체 조회/B 임시 BFS/A 진단), 상태(새 경로 깊이·기존 경로 삭제 여부·검사 전역·시각 비트), 부모와 두 후보 슬롯 전체 raw 가 같다.
  2. 계약: 단위 거리 비행체의 조기 반환이 미선택 경계를 요구하지 않고, 실제 시작 누락과 CD 판본 혼합·spot 크기 오류를 거부한다.

최초 실패와 교정: 새 모듈은 위 fixture 1807행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 경로 조회(427ce0)·난수(4558f0)·새 경로 시작(`RawPathStart` 489080; 기존 경로를 삭제할 수 있음)·후보 운반체 조회(4b2070)·임시 BFS(488af0)·이동 속도·좌표 유효성(40e800)은 훅 경계이다. `RawPathStart`/`RawPathProcess` 이식 뒤에 실제 연결한다.

## 4. 다음 시작점

`RawCarrierChoice`(운반체 선택), `RawPathProcess`(RunFrame·도착; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`·`RawPathRetarget`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
