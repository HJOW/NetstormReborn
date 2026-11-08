# webpj 사제 배치 접두(RawPriestPlacement) 이식 — WB04-E155

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백쉰다섯 번째 부분이다. 사제(Priest) 생성이 자리를 고를 때 부르는 배치 검사(MayPlace)의 접두 — 전역 초기화, 강제/즉시 허용, 부호 확장한 소유자의 로컬 비교, 모양 여백, 지도 경계 — 를 옮긴 `o/RawPriestPlacement`이다. E152 일반 배치 접두(`RawCanonPlacement`)에 위임하는 얇은 어댑터이며, 사제 배치 계열(접두→기하→모양→미리보기→충돌→지형)의 첫 부분이다. cpppj 기준선 `1f00a25a` 에서 `.cpp`·시험·fixture 는 변하지 않았고, `RawPriestPlacement.h` 는 `a84509c9`(20261107-48 CP)에서 강제 허용 공유 getter(`sharedForcePlacement`) 두 줄이 추가되어 그 판을 옮겼다(`git diff --numstat` 으로 확인, PORT_MAP 기준 `a84509c9`).

## 1. 옮긴 것

`www/core/raw-priest-placement.js`

- `new RawPriestPlacement(pool, types, state, hooks).mayPlace(query)` — `query = { type, x, y, flags, owner, mode }`(사제 생성의 여섯 배치 인자, 호출 때 값으로 복사). 사제 genus(`0x200000`)를 가진 자산 타입(70 이상)만 받고 좌표는 유한해야 한다. `hooks = { shape(type, argument, flags) → 사각형, inspectGeometry(query, localOwner) → bool }` 둘 다 필수. 판정은 E152 `RawCanonPlacement` 에 위임하되 첫 인자(argument)는 타입 번호 자신이고, 후반 경계에는 호출 때 복사한 사제 요청을 그대로 넘긴다(호출자가 요청 객체를 바꾸어도 원본 스택 값 인자와 로컬 비교 결과를 유지). `state` 는 일반 배치와 같은 `createPlacementState()` 구조이다.
- `makePriestSpawnPlacementHooks(pool, placement, hooks)` — 사제 생성(`RawPriestSpawn`)의 `mayPlace` 경계만 위 접두로 연결하고 다른 생성 효과는 유지한다. 풀이 다르면 거부한다.

이 모듈은 DOM 을 쓰지 않으며 10.72(CD)의 중간 단정도 저장 여백은 범위 밖이다.

## 2. 검증

`tests/priest-placement.test.mjs`(3건):

1. `priestplacement-x86.tsv` 의 `originals` 1008행(전체 3024행 중 10.78) — 모양 9종×좌표 16종×전역/소유자 7종을 실제 PE 의 사제 배치 접두로 실행한 허용 결과, 전역 차단값, 강제 허용·로컬 플레이어 전역, 타입 genus, 모양/후반 경계 호출 기록(`H:`/`G:`)이 모두 같다. 모양 조회 중 전역 변경과 후반 경계의 전역 변경도 공급하며, 허용/거부/모양 조회가 실제로 일어났는지 센다.
2. 누락 모양/후반 경계·타입 표 크기 오류·비자산 타입(255)·사제 genus 가 없는 타입·비유한 좌표(경계 호출 전 거부)·비유한 모양 사각형 거부, 다른 풀의 생성 훅 연결 거부, 모양 조회 중 요청 객체와 로컬 플레이어를 바꿔도 값 인자와 로컬 비교 결과가 유지됨.
3. 실제 `RawPriestSpawn` 과 연결해 지도 공간이 있는 자리 탐색에서 첫 후보를 거부하고 두 번째 후보(21,21)를 채택하며 모양 조회·기하 경계가 각각 두 번 불리고 차단 전역이 0 으로 남는다. 생성 효과(`create`/`setOwner`/`pop`)는 시험 기록 경계이다.

최초 실패와 교정: 새 모듈은 1008행 fixture 와 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 사제 배치 계열의 나머지(`RawPriestPlacementGeometry`·`Shape`·`Preview`·`Collision`·`Terrain`)와 전체 통합 시험은 다음 단위들이다. 이 접두의 `inspectGeometry` 는 그 단위들이 연결한다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawPriestPlacementGeometry`·`Shape`·`Preview`·`Collision`·`Terrain`, `RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
