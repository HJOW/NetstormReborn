# webpj 사제 배치 지형(RawPriestPlacementTerrain) 이식 — WB04-E158

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백쉰여덟 번째 부분이다. 일반 사제 배치의 후보별 다리·섬·지역 배열과 모양 종료 효과를 옮긴 `o/RawPriestPlacementTerrain`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. 원본 `0049bb1a` 이후의 사제 지형 효과이며 일반 타입 지형(E148)과 달리 권한 훅이 없고 처음부터 권한이 허용되며 최종 판정이 항상 참이다. 이 단위로 사제 배치 계열(접두 E155, 기하/모양 E156, 미리보기/충돌 E157)이 닫힌다.

## 1. 옮긴 것

`www/core/raw-priest-placement-terrain.js`

- `createPriestPlacementTerrainState()` — `{ regions(144바이트, 열 간격 12, 후보 쓰기는 8×8), emptyRegion(기본 127), noIslandType, bridgeOverlap, noIslandOnly(참), groundComplete(참), permission(참), canPlaceGround }`.
- `new RawPriestPlacementTerrain(pool, types, frames, islands, state)` — `islands` 는 지형 SID 지도(65536칸 uint16, 미리보기의 표면 SID 지도와 구분). 사제 genus(`0x200000`)이고 다리(4)가 아니며 10.78 의 특수 지역(`0x200`)이 아닌 타입, 발자국 1..12 만 받는다(위반은 거부).
- `begin(query)` — 누적 상태 초기화(권한 참, 지역 배열은 유지). `beginShape(query, x, y)` — 절삭한 좌표−발자국+1 을 배열 원점으로 잡고 지면 마스크(`flags1 & 6`)를 고정하며 배열을 sentinel 하위 BYTE 로 채운다. `candidate(query, sid)` — 충돌을 통과/무시한 모든 후보에 다리 겹침(폭≠높이면 거부)과 섬 지역 쓰기(8×8, 열 바깥/행 안쪽 순서)를 적용. `endShape(query)` — 첫 signed BYTE 와 sentinel 비교 및 발자국 전체 동일성으로 지면 완성 여부를 누적(되돌리지 않음).
- `finish(query)` — 지면 허용(받침 전용·지면 완성·다리 겹침)을 계산하고 받침 전용 타입이 섬과 겹치면서 지면이 불완전하면 권한을 철회하되 반환은 항상 참(사제의 최종 지면 우회). `regionAt(x, y)` — 지도 밖/빈 칸 127, 다리 sentinel, 섬 raw +8 WORD(지역 genus 가 아니면 10.78 거부).
- `makePriestTerrainGeometryHooks(pool, terrain)` — 지역 초기화/모양 시작/모양 종료/최종 계산을 E156 사제 기하의 네 경계에 연결. `makePriestTerrainCollisionHooks(pool, terrain, hooks)` — 후보 지형 효과만 연결. 풀이 다르면 거부한다.

이 모듈은 DOM 을 쓰지 않으며 10.72(CD)의 BYTE 프레임과 지역 getter 차이는 범위 밖이다.

## 2. 검증

`tests/priest-placement-terrain.test.mjs`(4건):

1. `priestterrain-x86.tsv` 의 `originals` 2240행(전체 6720행 중 10.78) — 후보 지형(C)·모양 종료(E)·최종 판정(F)·지역 조회(R)를 사제 타입 flags1/후보 genus/프레임 지면 플래그/좌표/지역값/섬 지도 유무/배열 씨앗 6종/sentinel/초기 지면 완성 조합으로 실제 PE 에서 실행한 결과(반환값·다리 겹침·받침 전용·지면 완성·권한·지면 허용)와 144바이트 지역 배열, 후보 raw 슬롯 전체가 모두 같다. 네 종류가 모두 관찰되었는지 센다.
2. 실제 사제 기하/finder 가 충돌에서 무시한 섬·다리도 지형 효과를 받고(지역 9 기록, 받침 전용 해제, 다리 겹침 발생·해제) 허용 지면이 없어도 최종 사제 우회와 권한 철회가 분리되어 보존됨, 유효 모양이 없으면 배열을 초기화하지 않음.
3. 잘못된 순회 순서·자료 크기·타입 계약(특수 지역 genus)·비유한 좌표·다리 위 발자국 폭/높이 불일치·후보 프레임 오류·지역 genus 오류·다른 풀 거부, 한 모양에서 거짓이 된 지면 완성의 끈끈함.
4. **통합(E156 이월)** 사제 비패턴 모양/미리보기 범위 → 실제 finder(blocker 충돌) → 사제 지형 → 사제 생성자를 모두 실제 모듈(기하·픽셀 모양·접두·미리보기·충돌·지형·생성)로 연결: 첫 후보 거부 뒤 (21,21) 에서 채택, 지역 시작 2회·모양 종료 1회·최종 1회, 생성 SID 15001, WORD `0x8101`, 미리보기 16칸 표시, 지면 완성/허용 거짓.

최초 실패와 교정: 새 모듈은 2240행 fixture 와 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 사제 배치 계열(E155~E158)은 이 단위로 닫혔다. 사제 배치와 연결된 서버 건설/생성 경로(`RawPriestSpawn` 실제 팩토리 연결, `RawPacketDispatch`→`RawConstructionCommand`)는 별도 단위이다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
