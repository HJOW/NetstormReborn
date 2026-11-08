# webpj 일반 배치 파이프라인(RawCanonPlacementPipeline) 이식 — WB04-E154

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백쉰네 번째 부분이다. 일반 타입과 특수 패턴 배치의 모든 복원 모듈(Player 기준점→권한→주변→관계→지형→기하→픽셀 모양→충돌→미리보기→접두)을 같은 현재 표면/공통 전역 상태에 연결하는 `o/RawCanonPlacementPipeline`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. E146~E153 의 Canon 배치 계열을 닫는 마지막 층이며, 전체 MayPlace 관측 fixture(`canonmayplace`)로 모든 모듈의 실제 연결을 한 번에 대조한다.

## 1. 옮긴 것

`www/core/raw-canon-placement-pipeline.js`

- `createCanonPlacementPipelineState()` — 배치(`placement`)·미리보기(`preview`)·충돌(`collision`)·지형(`terrain`)·권한(`permission`)·관계(`relations`)·Player 기준점(`anchor`)·기하(`geometry`: patternTypes 8칸 + 진단 장치)·픽셀 모양(`shape`: 배율 16×11) 상태를 원본 초기값으로 한 묶음 만든다.
- `new RawCanonPlacementPipeline(pool, hash, data, bookkeeping, additional, contained, state)` — `data = { types, frames, shapes, hotspots, spots, islands, regions }`. 모든 자료/장부/상태는 호출자가 소유해 이 객체보다 오래 살아야 한다. 현재 표면과 Player 기준점 지도는 공간 해시의 0단계 배열을 **직접 공유**한다(`hash.view(0)`). 모듈은 anchor → permission → surrounding → relations → terrain → geometry → shape → collision → preview → placement 순서로 만들고, 후보/주변/최종 정책은 모두 복원한 실제 모듈에 연결한다.
- `mayPlace(query)` — 공통 전역의 중복 표현(편집기·동맹 사용·관계 표·그래프 활성·noIsland 타입 번호)을 현재 값으로 맞춘 뒤 실제 접두부터 최종 반환까지 실행한다. `pool` 은 서버 건설 요청 어댑터가 같은 풀인지 확인하는 용도이다.

`www/core/squid-hash.js` — `view(level)` 를 추가했다. 단계 머리 배열의 살아 있는 참조이며(사본 아님), 원본의 표면 지도(0단계)처럼 같은 저장소를 직접 공유해야 하는 소비자(이 파이프라인)에만 쓴다. 쓰기는 기존 `setCell`/`setBucket`/`setAt` 으로만 한다. 기존 해시 시험은 그대로 통과한다.

이 모듈은 DOM 을 쓰지 않으며 10.72(CD) 분기는 범위 밖이다.

## 2. 검증

`tests/canon-placement-pipeline.test.mjs`(3건):

1. `canonmayplace-x86.tsv` 의 `originals` 144행(전체 432행 중 10.78) — 전체 MayPlace(진입 → 실제 helper → 정상 반환)를 모양(일반/패턴)·인자·방향·좌표·타입 genus/flags·장면 6종(기본·작업장 없음·다른 소유자 섬·섬 칸 빠짐·장애물·빈 지도)·소유자·로컬 플레이어·그래프/편집기/동맹/관계 값·지면 우회·제한 값·모드·강제 허용으로 실행해 허용 결과, 표시 차단 전역, 144바이트 미리보기 배열, 144바이트 지역 배열, 128슬롯 raw, 해시 네 단계 전체, 고정 섬 지도의 Adler-32 가 모두 같다. 허용/거부/패턴 장면이 모두 관찰되었는지 센다.
2. 현재 표면이 해시 0단계와 같은 저장소를 공유하고 공통 전역(편집기·동맹·관계 표·그래프·noIsland)을 진입마다 현재 값으로 맞춘다.
3. 자료/상태/해시 누락, 지도 크기 오류, 지역 표 누락을 거부한다.

`tests/canon-placement-integration.test.mjs`(4건) — 앞선 단위가 파이프라인 이식 뒤로 미룬 cpppj 통합 시험을 옮겼다.

- 지형(E148 이월): 실제 받침 디코더·미리보기·finder 를 통과한 아홉 모양에서 후보 권한 1회, 지면 종류가 맞지 않으면 후보 권한을 생략하고 모양마다 주변 경계(9회), 충돌 거부는 지형·모양 종료·최종 처리에 도달하지 않음.
- 권한(E149 이월): Player 조회 2회, 그래프 비활성이면 조회 없이 모양마다 주변 권한 경계(9회).
- 관계(E150 이월) 일반 자산·3×3 받침 완전 파이프라인: 픽셀 getter 부터 디코더·finder·Player 기준점·주변/지형·최종 관계까지 실제 구현으로 연결하고, 동맹/그래프/표면 소유자/지면 우회/편집기/특수 지역 제한 변화에 따른 허용·표시 차단·지면 허용(10.78 은 제한된 지역이 지면 허용을 취소)을 검사한다.

최초 실패와 교정: 새 모듈은 144행 fixture 와 통합 시험 4건을 첫 실행에 통과했다(E146~E153 의 모든 모듈이 실제 연결에서도 원본 관측과 일치). 직접 쓴 거부 시험의 도우미가 `null` 을 `??` 로 기본값에 되돌려 처음 한 번 실패했고(시험 도우미 오류), `in` 검사로 교정했다.

## 3. 경계와 남은 것

- Canon 배치 계열은 이 단위로 닫혔다. 서버 건설 요청 어댑터(`RawConstructionCommand`)와의 연결은 `RawPacketDispatch` 선행 단위가 필요하다.
- C++ `RawBridgeDecay` 가 쓰는 `RawCanonSurfaceWalk` 실제 Squid 원천 생성자(`forSquid`)와 이미 이식된 JS 다리 붕괴의 대조는 E151 문서의 경계 항목으로 남는다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawPriestPlacement*`(사제 배치: 같은 `createPlacementState()` 사용), `RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
