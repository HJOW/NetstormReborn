# webpj 일반 배치 주변 표면 순회(RawCanonPlacementSurrounding) 이식 — WB04-E151

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백쉰한 번째 부분이다. 배치 디코더의 현재 모양에서 flag 8 표면 지도를 동적으로 순회하는 `RawCanonSurfaceWalk`(모양 탐색 생성자 `004b23e0`의 flag 8 지도 반복/필터)와 그 순회로 주변 권한을 누적하는 `RawCanonPlacementSurrounding`을 옮긴 `o/RawCanonPlacementSurrounding`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. E148 지형의 `shapePermission` 경계에 실제 순회를 연결하는 Canon 배치 계열의 다섯 번째 층이며, 선행은 E149 권한·E12 `bridgeConnects`·E39 `RawSquidNeighbors.direction` 이다.

## 1. 옮긴 것

`www/core/raw-canon-placement-surrounding.js`

- `RawCanonSurfaceWalk` — 모양 원천이 두 가지이며 같은 발자국/중심/최초 방문을 만든다.
  - `forPlacement(pool, types, frames, islands, spots, query, cell)` — 배치 질의 타입과 디코더의 현재 칸(프레임·원래 좌표). `frames` 는 타입별 `{ frames: 코드 배열 }`.
  - `forSquid(pool, types, codes, islands, spots, sid)` — 실제 Squid 슬롯의 타입·프레임 DWORD·float 좌표(`004b23e0(sid, 8)`). `codes` 는 타입별 프레임 코드 배열의 배열.
  - 발자국은 1..255 로 자른 뒤 내부 spot 비트(8)가 모두 켜져 있으면 곧바로 종료한다. 확장 사각형의 네 모서리는 건너뛰고, 모양 안쪽 후보·이미 반환한 후보(부호 WORD 중복 캐시, 용량 64 초과 거부)·표면 타입이 아니거나 접합(`bridgeConnects`)/죽음/내부 비트에 걸리는 후보는 거른다. 후보 번호가 32767 을 넘으면 거부한다.
  - `current` 는 생성 직후 첫 결과이며 `next()` 가 지도를 그 시점에 다시 읽는다(0 반환 뒤에는 종료 상태 유지). `state` 는 원본 커서(`x`·`y`·`left`·`top`·`right`·`bottom`)와 중복 캐시(`returned`)이다. 모양의 float 발자국/중심/프레임 번호는 생성 때 고정하고 지도/raw/코드 표는 `next` 마다 현재 값을 읽는다.
- `new RawCanonPlacementSurrounding(pool, types, frames, islands, spots, permission)` — `inspect(query, cell)` 은 현재 모양의 표면 후보 모두에 E149 `permission.surrounding`(바깥 관계 검사 + 후보 권한)을 적용해 하나라도 권한을 얻으면 참이다. 권한을 얻은 뒤에도 남은 후보를 계속 읽으며 요청 소유자는 하위 BYTE 만 부호 확장한다.
- `makeCanonSurroundingTerrainHooks(pool, surrounding, hooks)` — 지형 훅의 `shapePermission` 만 실제 순회로 바꾸고 다른 훅은 유지한다. 풀이 다르면 거부한다.

이 모듈은 DOM 을 쓰지 않으며 10.72(CD)의 BYTE 프레임/소유자 위치/루프 차이는 범위 밖이다.

## 2. 검증

`tests/canon-placement-surrounding.test.mjs`(4건):

1. `canonsurrounding-x86.tsv` 의 `originals` 524행(전체 1572행 중 10.78) — 모양 표면 finder 전체(W: 생성 직후부터 마지막 0 까지 모든 단계의 커서·중복 캐시 스냅샷)와 MayPlace 주변 구간(모양 종료의 최종 판정)을 모양 크기·좌표·프레임·타입 flags·표면 노드(소유자·좌표·프레임·상태)·섬 지도·spot 입력, 그래프/편집기/동맹/다른 소유자 허용 전역, Player 조회 응답, 조회 중 후보/지도/소유자/프레임 변경 4종으로 실제 PE 에서 실행한 결과 — 스냅샷 목록·최종 판정·Player 조회 인자 목록·128슬롯 raw·섬 지도·spot Adler-32 — 가 모두 같다. 비어 있지 않은 순회와 Player 조회 관찰이 실제로 실행되었는지도 센다. `originalCD`·`original1037` 행은 사용하지 않았다.
2. 실제 E57 Player 기준점 조회와 주변 표면을 통해 모양 종료의 권한 누적(`beginShape`→`endShape`→`finish`)까지 연결: 소유자가 같은 현재 섬은 통과하고, 중립으로 바꾸면 후보 권한은 작업장을 돌려주고 주변 권한은 바깥 관계에 막히며, 다른 소유자 허용 후에는 참, 그래프 비활성이면 거짓이다.
3. 실제 Squid 슬롯을 원천으로 한 순회(`forSquid`)가 같은 입력의 배치 원천 순회(`forPlacement`)와 같은 커서·캐시를 만든다. 연결이 실제로 일어나도록 연결 마스크가 0 인 `P` 코드 대신 서쪽으로 연결되는 `A` 코드를 쓴다.
4. 필수 자료·풀·프레임 코드·발자국 0·섬 지도 크기·지도 밖/비유한 좌표 거부, 후보가 없는 빈 모양은 권한을 얻지 못함.

최초 실패와 교정: 새 모듈은 524행 fixture 와 Player 연결을 첫 실행에 통과했다. `forSquid` 동등성 시험은 처음에 연결 마스크가 0 인 `P` 코드를 써서 이웃이 찾아지지 않아 실패했고(시험 입력의 오류), `A` 코드로 교정했다.

## 3. 경계와 남은 것

- C++ `RawBridgeDecay` 가 쓰는 실제 Squid 원천 생성자는 `forSquid` 로 옮겨 두었으나, 이미 이식된 JS 다리 붕괴(`raw-bridge-decay.js`)가 이 순회기를 쓰도록 연결하는 일은 하지 않았다(해당 단위의 fixture 로 이미 닫혀 있음). 필요하면 후속 단위에서 대조한다.
- 일반/패턴 완전 파이프라인(충돌·미리보기·배치 접두 포함)의 통합 시험은 `RawCanonPlacementPipeline` 단위에서 다룬다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawCanonPlacementCollision`·`Preview`·`Pipeline`·`RawCanonPlacement`, `RawPriestPlacement*`, `RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
