# webpj 운반 후보 선택(RawCarrierChoice) 이식 — WB04-E102

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백두 번째 부분이다. 골렘(Carrier)의 귀환·간헐천 후보 목록 수집(428ea0/429420)과 실제 경로 시각에 따른 후보 평가(4280a0)를 cpppj 기준선 `1f00a25a` 의 `o/RawCarrierChoice` 에서 옮겼다. E90 의 `raw-carrier-candidate-sort.js`(원본 비교기/CRT qsort), E98 의 `raw-path-runtime.js`(임시 경로·Prepare), E100 의 `raw-board-target.js`(일반 접근점)와 `squid-postpop-state.js`(`SquidPostPopList`)가 선행이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-carrier-choice.js` — `CarrierChoiceState`, `RawCarrierChoice(pool, types, players, returns, geysers, now, checking, board, state, hooks)`

- `evaluate(candidates, anchor, mover)`(4280a0): mover 가 유효하면 이동 속도(`movement`)를 한 번 읽고 AI 가 있거나 비행(flags2 0x20000)이면 경로 탐색을 생략한다(처음에 고정, 이후 타입이 바뀌어도 유지). 첫 순회는 후보 목록을 원래 ring 으로 한 번씩 돌며(현재 count 를 매번 다시 읽음, 용량에 닿으면 중단) surface 후보는 진단(0xace)을 부른 뒤 다시 검사해 여전히 surface 면 건너뛰고(seen 증가 없음), 아니면 기준점에서 후보 발자국으로의 두 접근점 사이 거리/속도(넓은 거리, `_ftol` 하위 WORD)를 후보 슬롯 +8 의 임시 WORD 로 쓴다(할당·타입 필터 없음). 이어 목록 앞 count 개를 원본 CRT qsort 와 비교기로 정렬한다. 두 번째 순회는 임시 WORD 가 지금까지의 최소(float, 초기 10000)보다 작고 경로 비교가 켜졌으며 탐색 생략이 아닐 때만 새 임시 경로(count·시각 배열 0)를 만들어 BFS(cardinal TYPE 이면 flags 1)를 부르고, 경로가 있으면 현재 시각으로 Prepare 해 `마지막 시각 − 현재 시각`을 새 점수로 하며 currentMetric 에 `속도×점수`(float 비트)를 기록한다. 엄격한 최소값만 채택하며, 마지막에 검사 전역이 켜지고 본 수가 현재 count 와 다르면 진단(0xb03)을 부르고 currentMetric 을 baseline DWORD 로 복사한다(float 변환 없음). 후보의 발자국·위치는 첫 발자국 효과 전에 보관한다.
- `chooseReturn(parent)`(428ea0): 현재 Player+0x7c 의 집 신전(유효하면)을 선두에 넣고 별도 outposts 목록을 nonzero 수만큼 ring 순회하며 수집한다(용량 500 상한·중복 보존) → `evaluate(후보, parent, parent)`.
- `chooseGeyser(parent)`(429420): 먼저 귀환점을 고르고 물리 범위 안일 때만 Graph 조회(462bc0; 반환값은 쓰지 않지만 생략하지 않음) 뒤 현재 geysers 목록(여기서는 nonzero 수가 아니라 index 와 count 를 비교, 조기 종료 없음)을 수집해 그 점에 대해 `evaluate` 한다.
- 도달한 훅이 없으면 오류이다(`movement·footprint·search·validReturn·validGeyser·graphFor·assertion`). 독립 관측의 배열/풀 기준 주소는 선택 훅 `sortAddresses()` 가 정렬 비교기에 공급한다(게임 경로에서는 비움).

## 2. 검증

- `tests/carrier-choice.test.mjs`(2건):
  1. `carrier-choice-x86.tsv` 721행(10.78 전용 단일 판본). 입력 19 DWORD(연산 0 평가/1 귀환 선택/2 간헐천 선택·원본 목록 구멍/중복/역순 모양·count/capacity·Player·AI·병기·속도·경로 비교·검사 전역·변이 11종·집 신전·유효 마스크·경로 모양·주소 기준·baseline·큰 좌표·mover)로 외부 효과 순서(R 발자국/B 임시 BFS 인자/V·G 유효성/Q Graph/! 진단)·상태 11 DWORD(선택·currentMetric·baseline·comparePaths·두 목록 count·집·발자국/탐색/유효성 호출 수·전체 풀 24000슬롯의 Adler-32)·정렬된 후보 배열 hex 가 같다. NaN 경로 시각·큰 좌표·중복/구멍 목록·진단 변이를 포함한다.
  2. 계약: 빈 목록도 baseline 을 currentMetric 에 복사하고 선택되지 않은 큰 하위를 요구하지 않으며, CD 판본과 Player 표 크기 오류를 거부한다.

최초 실패와 교정: 새 모듈은 위 fixture 721행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 발자국(4ade70)·BFS(488af0)·소유권/Graph/상태 유효성(4274b0/427580)·Graph 조회(462bc0)·이동 속도는 훅 경계이고, 실제 연결은 `RawCarrierTarget`(E100)의 `chooseReturn` 훅과 경로 이식(`RawPathStart` 등) 뒤에 한다.

## 4. 다음 시작점

`RawPathProcess`(RunFrame·도착; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`·`RawPathRetarget`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
