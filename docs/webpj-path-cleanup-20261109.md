# webpj 경로 정리(RawPathCleanup) 이식 — WB04-E159

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백쉰아홉 번째 부분이다. 경로 프로세스 삭제 통지의 공간 접두 뒤 몸체(패치 `0048aa00`) — 건설 helper 의 공유 예약, 귀환 장소의 운반 nugget 삭제, 신전 의식 요청, 남은 helper 의 환불·삭제 — 와 귀환 장소/근처 타입 finder 두 조회를 옮긴 `o/RawPathCleanup`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. 이 모듈은 `RawCarrierArrival`·`RawPriestArrival` 의 선행(귀환 장소·근처 신전 조회)이다.

## 1. 옮긴 것

`www/core/raw-path-cleanup.js`

- `createPathCleanupState()` — `{ boss, checking, priestType(158), daisType(165), outpostType(126), nuggetType(154), helperEvent(0x1a2c), sharedNuggetType }`. 권한/타입/이벤트는 하위 효과 뒤에도 현재 값을 다시 읽는다. `currentNuggetType(state)` 는 공유 getter(실제 `005412c0`)가 있으면 그 값을 쓴다.
- `new RawPathCleanup(pool, hash, types, state, hooks)` — `hooks = { reserveShared(), addShared(sid, event, payload), contained(sid, mask, kind, index), graphFor(sid), ritual(temple, parent, flags), refund(type, owner), destroy(sid, flags) }` 일곱 모두 필수(조회는 기존 실제 모듈, 의식/삭제/예약은 실행 경계).
- `afterSpatial(runtime, parent)` — ① 경로 이미지의 helper 가 현재 건설 보조 부류(`0x404200`)이면 부모 좌표(거의올림)±2 사각형에서 찾아 예약 시도(실패해도 helper 필드를 0 으로 지움, 첫 일치에서 종료) ② 부모가 죽지 않았으면 귀환 장소(반지름 1)와 운반 nugget 이 있고 소유자가 같고 두 Graph 가 같고 권한이 있으면 nugget 삭제 ③ 부모 타입이 사제이면 거리 2 의 신전을 찾아(abstract 신전 제외) 의식 요청 ④ 권한이 있고 남은 helper 가 있으면 정리: 이미 죽은 것은 생략, 보조 부류가 아니거나 abstract 가 아니면 checking 일 때 assert 로 거부하고 끝, 아니면 환불 뒤 삭제. 경로 이미지 폭이 판본과 다르면 거부, 남은 helper 번호가 풀 용량 밖이면 거부한다.
- `returnPlaceAt(x, y, radius = 1)` — 좌표±radius 를 CRT 절삭(하위 DWORD)한 사각형에서 abstract/죽음을 제외한 귀환 장소(genus `0x200` 또는 outpost)의 첫 일치. `nearbyType(x, y, radius, type)` — 거의올림 좌표±radius 사각형에서 현재 타입 genus 로 finder 시작 단계를 고른 첫 같은 타입.
- `makePathCleanupHooks(pool, carrier, anchor, host, place, hooks)` — 기존 contained 조회(`RawCarrierPreDestroy.findContained`)·Graph(`graphFor`)·공유 예약(`addSharedRegular`)·환불(`refund`)을 같은 풀에 정확한 인자로 위임한다.

이 모듈은 DOM 을 쓰지 않으며 10.72(CD)의 귀환 nugget 분기는 범위 밖이다.

## 2. 검증

`tests/path-cleanup.test.mjs`(3건):

1. `pathcleanup-x86.tsv` 의 `originals` 866행(전체 중 10.78) — 본문 행 704개(건설 helper 예약·실패, nugget 삭제, 신전 의식, 환불/삭제, 실패 뒤 raw 결과를 profile 18종·권한·죽음·사제·예약 성공 여부·checking·하위 효과 뒤 권한 변경 3종·좌표 3종으로 실제 PE 에서 실행)에서 효과 기록 순서·남은 helper 값·최종 권한·슬롯 5..11 전체 raw(가상 표 주입 4바이트 제외)가 같고 경로 이미지의 helper 외 필드가 보존된다(assert 로 거부되는 16행 포함). 조회 행 162개(귀환 장소/근처 타입 finder)는 반환 SID 쌍과 슬롯 불변성이 같다. `originalCD`·`original1037` 행은 사용하지 않았다.
2. 필수 하위 경계·타입 표 크기·경로 이미지 폭 불일치·남은 helper 범위 오류 거부, 공유 nugget 타입 getter.
3. 어댑터가 기존 contained/Graph/공유 예약/환불을 정확한 인자로 위임하고 다른 풀을 거부한다.

최초 실패와 교정: 새 모듈은 866행 fixture 의 모든 대조를 첫 실행에 통과했다(마지막 건수 확인식만 세 PE 합계 기준이라 10.78 단독 건수 704/162 로 교정).

## 3. 경계와 남은 것

- C++ 의 실제 경로 프로세스 공간 접두 + 같은 Host/Kernel SharedRegular 예약 통합 시험(`rawPathCleanup_real_process_spatial_prefix_and_shared`)은 경로 프로세스/사제 postPop/보호막이 한 장면에서 연결되는 단위에서 다룬다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawCarrierArrival`·`RawIslandRuntime`→`RawPriestArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
