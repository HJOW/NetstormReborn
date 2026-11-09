# webpj Fort 소유자 후처리(RawFortFinishOwner) 이식 — WB04-E221

2026-11-10, HJOW-Athlon. Fort 적재의 소유자 후처리(0040e1a0 Finish)와 단일 객체 임의 배치(0040ded0 AddRandom), 호출별 벽시계 local seed(004bcab0)를 옮겼다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위(Fort 적재·지형/섬 …)"에 속하지만, 기준선 뒤 변경이 없고 원본 관측이 있어 섬 목록(E161)·섬 검색(E220) 다음으로 옮겼다. 섬 population(`RawIslandPopulation`)이 이 모듈의 `placeRandom` 을 요구한다.

- cpppj 기준선 `1f00a25a`. 소스(`RawFortFinishOwner.cpp/.h`)·시험(`FortFinishOwnerTests`)·fixture(`fortfinishowner-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 루트에서 확인).
- 근거 커밋: 소스 `a4b63270`, fixture `ada85427`.
- 원본 fixture: `fortfinishowner-x86.tsv` 의 10.78(originals) 719행(originalCD 719행은 웹 범위 밖). 모드 F(Finish 전체)·B(AddRandom 배치)·S(시각 seed)의 16칸 입력(소유자·모드 3종·지역·목록 프로필·벽시계 비트·사각형 프로필·성공 시도/주기·local seed·타입·수량·force·변이)에서 호출 순서(지역 `R`·시각 `T`·배치 `P`·진단 `D`)·반환·local PRNG 상태·거주 구역 목록·현재 거주 구역 TYPE 을 대조한다.
- 공유 장치: E161 `raw-island-runtime.js` 의 `createFortFinishOwnerState`, D5b `random.js` 의 `GameRandom`, E2 `squid-postpop-state.js` 의 `SquidPostPopList`.

## 1. 옮긴 것

`www/core/raw-fort-finish-owner.js`(DOM 없음, 10.78 전용)

- `RawFortFinishOwner(pool, types, residences, state, hooks)`: 188칸 TYPE 표와 `hooks = { regionBounds, wallSeconds, place, invalidType }` 네 경계를 모두 구성 때 요구한다.
- `finish(owner)`(0040e1a0): 시작할 때 chaff/special 이면 자연 반환(소유자 범위 조회보다 앞섬). 시작 지역을 한 번 정한다(fort 이면 0, 아니면 Player 첫 지역, invalidRegion 이면 반환; **반복 중 모드/Player 가 바뀌어도 다시 정하지 않는다**). 반복마다 현재 거주 구역 목록을 끝까지 읽어(count·SID·현재 TYPE·owner; free/dead/void 를 추가 제외하지 않음) genus 0x40000000 이고 owner 가 같은 항목이 있으면 반환한다. 없으면 `regionBounds(region)` → 호출별 `wallSeconds()` 로 만든 local seed → 현재 거주 구역 TYPE 으로 `placeRandom(수량 1)` 을 반복한다(1000회 실패 뒤에도 계속).
- `placeRandom(random, rectangle, region, quantity, type, force)`(0040ded0): 타입 0/−1 은 `invalidType()` 진단 후 0. 사각형은 by-value snapshot, 높이 다음 폭 순서로 local PRNG 를 소비해 단정도 좌표를 만들고(limit<1 이면 소비하지 않음) 성공(`place` 가 비0)마다 수량을 줄이며 0 이 되면 멈춘다(수량 0 도 첫 시도 실행, 감김, 최대 1000회). 마지막 결과를 돌려준다.
- `RawFortFinishOwner.timeSeed(wallSeconds)`(004bcab0): 시각×1013 의 CRT 절삭 하위 DWORD 의 SAR 4 + 33배(DWORD 감김). 전역 게임 난수를 바꾸지 않는다.
- `bindPlayerRecords(players)`: 신전/섬 목록이 함께 갱신하는 같은 9 Player 기록(0xb4×9)의 +0x78 첫 지역 DWORD 를 현재 원문에서 읽고 `state.playerRegions` 사본은 쓰지 않는다(Player 기록은 쓰지 않음).
- `makeFortFinishOwnerHooks(pool, finish, hooks)`: 같은 풀의 `finish` 를 Fort 적재 훅의 필수 `finishOwner` 로 연결한다(다른 풀이면 거부).

## 2. 검증

`tests/fort-finish-owner.test.mjs`(4건)

- 원본 관측 재생 2건: 10.78 719행 전부, 같은 719행을 공유 Player 기록 모드(현재 +0x78 DWORD 만 지역으로 읽고 state 사본은 999 로 분리)로 다시 전부 재생한다. 행마다 새 풀/목록/상태를 만든다.
- cpppj 계약 이식 2건: 로더 훅 연결(같은 풀의 거주 구역이 있으면 지역/시각/배치 없음, 다른 풀 거부, 초기 모드 반환이 소유자 범위 조회보다 앞섬), by-value 사각형 snapshot·수량(2회)·실제 경계 누락/손상된 현재 목록(count>저장소) 거부. 웹에서 타입 0/−1 진단·Player 기록 크기·판본 거부를 같은 시험에 더했다.

최초 실패와 교정: 모듈은 처음부터 719행 ×2 와 계약이 모두 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(cmp 일치).

- 실패한 변형: 시각 seed 계수 33→31(2건), 시각 배율 1013→1000(2건), 시도 횟수 1000→999(2건), 수량 종료 조건 변경(3건), fort 시작 지역 0 제거(2건), owner 비교 반전(1건 — 첫 행에서 바로 무한 반복 대신 조기 실패).

## 3. 경계와 남은 것

- 실제 월드에 연결하지 않았다. `finishOwner` 호출 지점(Fort 객체 적재 `fort-object-load.js` 의 훅)과 실제 지역 사각형(`regionBounds`)·벽시계·가상 배치(`place` → 실제 Pop/postPop)는 순서4 몫이다(PORT_MAP session 0). CD 10.72 는 웹 범위 밖이다(CD 는 Player 기록 간격 0xac·첫 지역 +0x70).
- 다음 단위인 `RawIslandPopulation`(섬 배치 뒤 거주 구역·초기 객체)은 `RawIslandBuilder`(미이식)와 `ChunkMap` 에 의존한다.

## 4. 다음 시작점

WB04-E222 는 3.1절에서 고른다. 기준선 뒤 변경이 없는 `o/` 단위(`RawIslandBuilder`·`RawIslandLifecycle`·`RawFortMetaLoad`·`RawFortBadgesGlobals`·`RawFortPaletteTransition`·`RawStateWorldCleanup`·`SquidSpatial`·`RawTerrainGrowth`·`RawManaBoltRegular` 등; 의존하는 `o/` 헤더가 모두 이식된 것)을 권한다.
