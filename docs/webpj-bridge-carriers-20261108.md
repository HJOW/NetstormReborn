# webpj 다리 위 이동체 대피(RawBridgeCarriers) 이식 — WB04-E46

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 마흔여섯 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawBridgeCarriers`(10.78; 원본 4202f0 칸 위 이동체 확인과 427e00 대피 후보 선택)를 옮겼다. [E45](webpj-raw-bridge-decay-20261108.md)의 `RawBridgeDecay.applyLife`가 새 수명이 5 이하일 때 부르는 `carriers(sid)` 훅의 본체이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-bridge-carriers.js` — `RawBridgeCarriers`

- `new RawBridgeCarriers(pool, hash, types, boss, checking, priest, rainWalker, avoidLife, hooks)`: 현재 전역 네 개(권한·진단 DWORD·사제/rainWalker 타입·최소 수명 7)는 값 getter로 받아 조회 시점마다 읽고, 큰 이동/Graph/진단 효과만 훅(`graphFor`·`movementType`·`movementForm`·`start`·`valid`·`assertion`·`wontWalkOffEdge`·`scan`)으로 받는다. 누락된 훅은 도달한 지점에서만 오류이며 앞선 실제 변경은 남긴다.
  - `check(bridge)` [4202f0]: 다리 칸의 한 칸 사각형(좌표를 `_ftol`로 절삭)을 일반 finder로 훑어 사제/rainWalker 두 TYPE만 선택하고, 이동체 genus(0x30000)가 아니면 진단 0x2d8, 현재 이동 종류→이동 Form 조회 뒤 Form이 없으면 대피시킨다. 바깥 finder는 중첩 대피의 finder와 독립이다.
  - `retreat(parent)` [427e00]: 권한과 Graph(254=없음을 0으로, 그 밖 번호+1) 확인 → checking이면 5개 TYPE 비교 진단(0x893) → 중심을 `0.99999f`로 거의 올림해 반경 5 사각형을 일반 finder로 순회 → 표면이고 위험하지 않은(bridge genus·비영 수명·`avoidLife` 미만이 아닌) 후보 중 같은 Graph이고 거리 1 이상인 최소(동률은 앞 후보, float 저장) → `valid(선택점)`이고 `start`가 거부하면(checking일 때만) 진단 0x8b0.

## 2. 검증 (`tests/bridge-carriers.test.mjs`, 2건)

원본 10.78 독립 관측 `bridge-carrier-retreat-x86.tsv` 258행(연산 2종·권한·종류·이동 Form 유무·위치 시나리오 6종·수명·후보 flags/Graph·시작 결과·checking·후보 순서·행위자 좌표 비트)을 실제 raw 풀·4단계 해시에 4개 슬롯으로 설치해 재생하고, 사건 순서(`F:` 탐색 사각형·`T:`/`P:` 이동 조회·`M:` 이동 요청·`A:` 진단)와 4개 슬롯 전체 50바이트를 원문과 비교한다. 계약: 구성 오류 거부, 권한 없음 자연 반환, 권한 있고 Graph 훅 없음은 도달 지점에서 오류.

최초 실패와 교정: 모듈은 258행 전부를 첫 실행에 통과했다.

## 3. 경계

- 실제 월드의 이동 프로세스(`RawPathRuntime`: 이동 종류 조회·이동 Form·이동 시작), `RawAuthorityState`, 위험 수명 전역(`avoidLife` 최초 7) 연결은 월드 조립(WB05)에서 공급한다.
- `RawPathPoint`는 단순 `{x, y}` 값 객체로 대신했다(길 찾기 본체 `RawPathSearch`/`RawPathProcess`는 아직 이식하지 않음).

## 4. 다음 시작점

`RawIslandPostPop`의 섬/표면 접두 본체, Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState`·`RawPlayerGeyserBounds`·`RawPlayerPlacementAnchor`), 종류별 PostPop, `SquidDisplay`/`SquidFrame`이다.
