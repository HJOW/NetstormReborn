# webpj 일반 배치 지형(RawCanonPlacementTerrain) 이식 — WB04-E148

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백마흔여덟 번째 부분이다. 일반 타입과 특수 패턴 배치의 후보 지형 누적(다리 겹침·섬 지역 쓰기), 144바이트 지역 배열, 모양 종료의 지면 완성 판정, 최종 지면/권한 누적을 옮긴 `o/RawCanonPlacementTerrain`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. E146 기하 모듈의 `beginRegions`/`beginShape`/`endShape`/`finishRegions` 훅에 그대로 연결되는 Canon 배치 계열의 두 번째 층이다.

## 1. 옮긴 것

`www/core/raw-canon-placement-terrain.js`

- `createCanonPlacementTerrainState()` — `{ regions(144바이트, 열 간격 12), emptyRegion(sentinel DWORD, 기본 127), noIslandType, bridgeOverlap, noIslandOnly(참), groundComplete(참), permission, canPlaceGround }` 를 원본 초기값으로 만든다.
- `new RawCanonPlacementTerrain(pool, types, frames, islands, state, hooks)` — 188칸 타입 표, 타입별 프레임 코드 표, 지도 칸마다 지역 SID(uint16)를 담은 65536칸 표를 받는다. 훅 `candidatePermission(query, sid)`·`shapePermission(query, cell)`·`finishRelations(query, state)` 은 모두 필수이다(권한/최종 관계의 실제 정책은 후속 단위가 공급한다).
- `begin(query)` — 누적 상태를 초기화하고 그룹 10 이거나 권한 genus(`0x10202000`)가 있으면서 다리 genus 가 아니면 권한을 먼저 허용한다. 유효 모양이 없으면 이전 지역 배열은 지우지 않는다.
- `beginShape(query, cell)` — 절삭 좌표−발자국+1 을 배열 원점으로 잡고, 지면 종류 마스크(`flags1 & 6`)를 후보 순회 동안 고정하며, 배열을 sentinel 하위 BYTE 로 채운다. 비유한 좌표는 거부한다.
- `candidate(query, sid)` — 충돌에서 무시한 후보도 받는다. 다리 위 배치(발자국 폭≠높이면 거부)는 다리 겹침을 세우고, 섬 후보는 받침 전용 여부를 갱신한 뒤 프레임 코드의 지면 플래그가 마스크와 맞을 때만 8×8 안의 배열을 열 바깥/행 안쪽 순서로 지역 BYTE 로 덮어쓰고, 권한이 없으면 후보 권한 조회를 요청한다.
- `endShape(query)` — 첫 BYTE 의 부호 확장과 sentinel 을 비교해 지면 완성 여부를 거짓으로 내리거나(후속 모양이 되돌리지 않음) 현재 발자국 전체가 첫 BYTE 와 같은지 검사하고, 권한이 없으면 주변 권한 조회를 요청한다.
- `finish(query)` — 지면 허용(다리 genus 또는 받침 전용 또는 지면 완성 또는 다리 겹침)을 계산하고, 받침 전용 타입이 섬과 겹치면서 지면이 불완전하면 권한을 철회한 뒤 필수 최종 관계 경계의 판정을 돌려준다.
- `regionAt(x, y)` — 지도 밖/빈 칸은 127, 다리 genus 의 섬은 현재 sentinel, 그 밖의 섬은 raw +8 WORD 이다. 지역 genus 가 아닌 대상은 10.78 에서 거부한다.
- `makeCanonTerrainGeometryHooks(pool, terrain, query)` — 원래 질의(소유자/모드 포함)를 값으로 캡처해 E146 기하 훅 묶음으로 만든다. `makeCanonTerrainCollisionHooks(pool, terrain, hooks)` — 기존 훅에 `inspectCandidateTerrain` 만 덧붙인다. 풀이 다르면 거부한다.

이 모듈은 DOM 을 쓰지 않으며 10.72(CD)의 BYTE 프레임과 지역 getter 차이는 범위 밖이다.

## 2. 검증

`tests/canon-placement-terrain.test.mjs`(3건):

1. `canonterrain-x86.tsv` 의 `originals` 3740행(전체 11220행 중 10.78) — 시작(B)·후보 지형(C)·모양 종료(E)·최종 지면 누적(F)·지역 getter(R)를 소유 genus/그룹/플래그/발자국, 후보 genus/발자국/프레임 지면 플래그/좌표/지역값/섬 지도 유무, 배열 씨앗 6종, sentinel, 초기 지면 완성의 조합으로 실제 PE 에서 실행한 결과 — 반환값·다리 겹침·받침 전용·지면 완성·권한·지면 허용·144바이트 지역 배열·후보 raw 슬롯 전체 — 가 모두 같다. `originalCD`·`original1037` 행은 사용하지 않았다.
2. 무권한 모양에서만 주변 권한 조회를 하고(두 모양에 한 번), 빈 디코더(기본 프레임 −1)는 모양 준비 없이 시작과 최종 경계만 받으며 이전 배열(33)을 유지한다. 실제 E146 기하 모듈을 거친다.
3. 필수 훅 누락·지역 지도 크기 오류·비활성 순회의 모양 종료·풀 불일치·발자국 13·비유한 좌표·표 밖 타입 거부, 다리 genus 의 지면 허용과 권한 거부, 지도 밖·빈 칸 127, 지역 genus 아닌 섬 거부.

최초 실패와 교정: 새 모듈은 3740행 fixture 와 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- C++ 의 실제 받침 decoder/미리보기/finder 연결 시험(`canon_terrain_pattern_preview_finder_and_permission_lifetime`)은 충돌/미리보기/배치 접두 모듈 이식 뒤 그 단위의 통합 시험에서 다룬다.
- 후보 권한(`candidatePermission`)·주변 권한(`shapePermission`)·최종 관계(`finishRelations`)의 실제 정책은 `RawCanonPlacementPermission`·`RawCanonPlacementRelations`·`RawCanonPlacementSurrounding` 단위가 공급한다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawCanonPlacementPermission`·`Relations`·`Surrounding`·`Collision`·`Preview`·`Pipeline`·`RawCanonPlacement`, `RawPriestPlacement*`, `RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
