# webpj 세계 간헐천 생성(RawWorldGeyserBounds·RawWorldGeyser) 이식 — WB04-E107

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백일곱 번째 부분이다. 전체 세계 간헐천 생성 부모(00486440)와 그 아래의 원본 청크 범위(전체 00485bc0·분할 00485cc0)·옵션 표(간격 0041ca10·밀도 0041ca40)·Player 예산(0041c8e0)을 cpppj 기준선 `1f00a25a` 의 `o/RawWorldGeyserBounds`, `o/RawWorldGeyser` 에서 옮겼다. 선행은 이미 이식된 `GameRandom`(`random.js`)이다. 로컬 간헐천 생성(`RawLocalGeyser`)·실제 배치(`RawFortRandomPlacement`)·청크 지도(`ChunkMap`)는 아직 이식 전이라 훅으로 받는다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-world-geyser-bounds.js` — `worldGeyserSpacing`, `worldGeyserBudget`, `RawWorldGeyserBounds(state, hooks)`

- `worldGeyserSpacing(selection)`: 옵션 BYTE 0~3 을 간격 3/4/5/6 으로 읽고 표 밖은 호스트 오류이다. `worldGeyserBudget(battle, players, options)`: battle 일 때 Player 1~8 의 +0x2c 가 비영이고 숨김 BYTE(`options[63+owner-1]`)가 0 인 수 n 으로 `(12−n)×n` 을 반환한다(Player 9×0xb4 바이트·옵션 87바이트가 아니면 거부).
- `whole()`: 청크 범위 지역 0 을 16 배(DWORD 좌표를 `<< 4` 한 뒤 signed→float 저장)로 읽고, missionPost 이면 지역 1 을 읽어 1~255 로 보정한 점(현재 지도 한계 안의 사각형 점만 min/max 확장, 아니면 점 자체)으로 합친다. 아니면 현재 간격을 공유 여백 DWORD(`spacingMargin`)에 `간격−2` 로 쓴 뒤 사각형을 왼쪽·위로 (여백×16−16), 오른쪽·아래로 (여백×16) 넓힌다(각 좌표는 float 저장).
- `partition(cell, grid)`: 지역 0 을 한 번 조회한 뒤 missionPost 이거나 cell 이 grid² 를 넘으면 `whole()`(지역 0 재조회)이다. 아니면 같은 여백으로 넓히되 y 축은 float 저장 없이 배정도로 유지하고, 폭은 `|trunc(오른쪽−왼쪽)| / grid` 를 float 로 저장, 높이는 배정도로 유지해 cell 번호(왼쪽→오른쪽, 위→아래, 1 부터)의 열/행에 해당하는 구간 사각형을 돌려준다(0 이하 cell 은 전체 순회를 마친 다음의 열/행, 호스트 격자 한도 4096).
- `state = { missionPost, mapLimit, spacingMargin }`(모두 장기 전역의 live 속성), `hooks = { chunkBounds(region), spacing() }`.

`www/core/raw-world-geyser.js` — `timeSeed`, `worldGeyserDensity`, `RawWorldGeyser(edition, state, hooks)`

- `generate()`: 로그("Make Local Geysers\n") → 로컬 생성(00486360) → 전투이고 옵션 9 가 0 이 아니면 장식 → `predictableGeysers`(기본 1)가 0 이면 벽시각 시드로 난수 → 예산·밀도·간격으로 요청량 계산 → 옵션 18 에 따른 분할/전체 요청.
  - 장식(Local 뒤 현재 장식 타입 4개로 여섯 항목 표를 한 번 고정): 벽시각 시드 난수로 `옵션9 × 50`(반복마다 현재 옵션 9 를 다시 읽음)번 `표[난수(6)]` 타입을 고르고 Monument(네 번째 장식 타입)는 열 번째 초과 요청을 건너뛰며 전체 범위(`whole`)에 배치(`decorate`)한다.
  - 예산: 기본 예산 b, 밀도 d(−1/0/1/2)에서 보정 `(밀도 0.1/0.2 가산 + 1) × b` 를 ftol 절삭, 간격 s(3/4/6)에서 크기 가산(−0.2/−0.1/+0.1)을 더해 다시 절삭한 값을 /2/4 한 corner, edge = corner+1, center = edge/2.
  - 옵션 18 == 2: 아홉 칸(3×3 분할)을 차례로 중앙 center·네 모서리 corner·네 변 edge 로 요청한다(중앙은 허용 지형 0). 분할을 시작한 뒤 옵션이 바뀌어도 아홉 요청을 끝낸다.
  - 옵션 18 == 1: 밀도 getter 를 한 번 호출(결과 미사용)한 뒤 밀도 −1→b/2, 0→b, 1→(b/2)×3, 2→b<<1 로 수량을 정하고 half = 수량/2 로 전체 범위에 한 번(허공 인수=demo, 지형 인수=demo==0), 나머지(예산−half)를 허공만으로 한 번 더 요청한다.
- `worldGeyserDensity(selection)`: 옵션 0~3 을 밀도 −1/0/1/2 로 읽고 표 밖은 오류이다. `timeSeed(seconds)`: `(value>>4) + value×33`(value = 시각×1013 의 ftol 하위 DWORD, DWORD 감김; `RawFortFinishOwner::TimeSeed` 와 같은 식이며 해당 모듈 이식 때 재사용한다).
- `state = { battle(live), options(87바이트), decorationTypes(live 4개) }`, `hooks = { log, local, configInt, wallSeconds, budget, density, spacing, demo, whole, partition, decorate, populate }` 이며 도달한 하위가 없으면 앞선 효과를 유지한 채 오류이다.

## 2. 검증

- `tests/world-geyser.test.mjs`(4건):
  1. `world-geyser-bounds-x86.tsv` 820행(10.78 전용). 연산 W 전체/P 분할/B 예산·모드·크기 옵션·셀/격자·두 지역 청크 정수·변이 5종·Player 존재/숨김·battle 으로 16바이트 x87 사각형·공유 여백 DWORD·예산·청크/간격 호출 순서·모드·크기가 같다.
  2. 계약: 미선택 spacing 은 요구하지 않고(missionPost 일 때 지역 0→1 조회만) 분기 도달 시 앞선 청크 조회를 보존, 표 밖 간격/밀도·Player 표 크기·호스트 격자 범위 거부.
  3. `world-geyser-x86.tsv` 354행. 전투·Player 존재/숨김·간격/밀도/모드/장식 옵션·predictableGeysers·벽시각 비트·변이 10종·난수 소비로 로그/로컬/장식/요청 순서와 전체 인자(난수 상태·16바이트 사각형·수량·허용 인수)·옵션 87바이트·Player 1620바이트·장식 타입·demo 가 같다.
  4. 계약: 미선택 시각/배치 하위를 요구하지 않으며 선택된 하위 누락은 선행 효과(`L`, `LO`)를 남기고, 시각 시드 변환(0/NaN → 0, 1초, −1초)과 판본/옵션 크기 거부가 같다.

최초 실패와 교정: 새 모듈은 위 fixture 1174행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 로컬 간헐천 생성 부모(`RawLocalGeyser`, 00486360)·장식/간헐천 실제 배치(0040ded0·00486130)·청크 지도(00432210)·demo 플래그(00463b50)·벽시각은 훅 경계이다.

## 4. 다음 시작점

`RawLocalGeyser`(로컬 간헐천 생성), `RawPathProcess`(RunFrame·도착·Halt; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 분산 SP 정책(`ReadPlayerSpAmount`/`SetPlayerSpAmount`), 그 밖의 작은 몸체이다.
