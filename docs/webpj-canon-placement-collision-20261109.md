# webpj 일반 배치 충돌(RawCanonPlacementCollision) 이식 — WB04-E153

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백쉰세 번째 부분이다. 일반 타입 배치의 후보 무시 함수(`0049ade0`)와 한 후보 인라인 구간(`0049b9c4..0049bb1a`) — 클라이언트 SID 제외, 전역 무시 마스크, 로컬 발자국 표시, extra/mode 거부 — 를 실제 사각형 탐색기(`RawSquidFinder`)에 연결한 `o/RawCanonPlacementCollision`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. E146 기하·E148 지형·E152 접두/미리보기와 이어지는 Canon 배치 계열의 아홉 번째 층이며, 이 단위로 접두→미리보기→기하→충돌→지형 훅의 실제 연결 시험이 처음 닫힌다.

## 1. 옮긴 것

`www/core/raw-canon-placement-collision.js`

- `createCanonPlacementCollisionState()` — `{ clientMode(00540bc0), ignoredGenus(005325b4) }` 를 원본 초기값(0)으로 만든다.
- `placementIgnoreValue(placing, candidate, mode, ignoredGenus)` — 0049ade0 의 임의 타입 무시 함수이다. 반환은 비트값(DWORD)이며 bool 로 축소하지 않는다. 조기 분기 순서: ① mode 0 이고 배치 genus `0x20000` → 1 ② 후보 `0x02000000` → 1 ③ 후보 `0x120000` → 1 ④ 배치가 다리(4)이고 후보 `0x200000` → 1 ⑤ 후보 섬(2)이고 배치 타입 flags1 에 표면(`0x800`)이 없음 → 1 ⑥ 후보 `0x30006` 이고 배치 `0x208100` → 1 ⑦ 전역이 배치 genus 와 겹치면 `후보 & 0x218000` ⑧ mode 0 이고 배치 `0x30000` → 1 ⑨ 아니면 0.
- `new RawCanonPlacementCollision(pool, hash, types, state, preview, hooks)` — `hooks = { inspectRegions(query, scan), inspectCandidateTerrain(query, sid) }` 둘 다 필수. `inspectRegions` 는 모양 순회/지형/지역/관계 경계이며 모양마다 `scan(area)` 로 충돌 검사를 요청한다(`scan` 이 거짓이면 즉시 중단). `inspectCandidateTerrain` 은 충돌을 무시한 후보도 호출한 뒤 finder 의 미리 저장한 next 로 진행한다.
- `inspectCandidate(query, localOwner, sid)` — 현재 raw 타입을 먼저 읽고 클라이언트 SID 조건(예측 가능한 SID 5..14999)·무시 함수·로컬 발자국 표시(미리보기 배열 `[dx×12+dy+13]=1`, 후보 아래쪽 y 바깥·x 안쪽 순서, 넓은 차를 CRT 처럼 절삭, 후보 발자국/좌표 비정상은 로컬일 때만 거부)·`extra` 비트 0 과 mode 0 순서로 거부(거짓)를 정한다.
- `inspectArea(query, localOwner, area)` — 실제 finder 의 `begin`/`next` 를 쓰고 buried 만 finder 내부에서 제외하며 첫 거부 뒤에는 후속 후보를 읽지 않는다. `inspect(query, localOwner)` — 지역 경계가 거부 이후 `scan` 을 다시 요청하거나 성공을 덮어쓰지 못하도록 거부 상태를 유지한다.
- `makeCanonPlacementCollisionHooks(pool, collision, hooks)` — 미리보기의 `inspectCollisions` 만 연결. `makeCanonGeometryCollisionHooks(pool, geometry, regions, hooks)` — 요청의 소유자/모드를 캡처한 지역 정책(`regions(query)`)을 만들고 E146 기하의 패턴 순회/finder 범위를 연결한다(명시 프레임 없음). 풀이 다르거나 정책이 없으면 거부한다.

이 모듈은 DOM 을 쓰지 않으며 10.72(CD)의 bridge 비트/전역 마스크/extra 위치 차이는 범위 밖이다.

## 2. 검증

`tests/canon-placement-collision.test.mjs`(6건):

1. `canoncollision-x86.tsv` 의 `originals` 7560행(전체 22680행 중 10.78) — 일반 genus 9종×후보 7종×문맥 10종×좌표 6종×전역 2종으로 실제 PE 의 무시 함수와 한 후보 인라인 구간을 실행한 후보 결과(통과/거부), 144바이트 미리보기 배열, 후보 raw 슬롯 전체(불변성)가 같다. 예측 가능한 클라이언트 SID(4·5·14999·15000·15001)와 50 번 슬롯을 모두 쓰며, 통과·거부·표시가 모두 실제로 관찰되었는지 센다.
2. 무시 함수의 조기 분기 순서와 반환 비트값(0x8000 비트값, 반환 마스크 밖이면 0, 앞선 분기가 1 로 먼저 무시).
3. 실제 finder 의 buried 제외, 전역 무시 마스크, 첫 거부 이후 탐색 중단(다음 링크를 잘못된 값으로 두어 더 읽지 않음을 확인), 재요청 시 거부 상태 유지.
4. 무시한 후보의 지형 효과 뒤 현재 genus 와 미리 저장한 next 를 사용(지형 훅이 첫 후보의 링크를 끊고 전역을 바꿔도 두 번째 후보가 새 전역으로 검사됨).
5. 실제 E146 기하·E152 접두/미리보기·E148 지형 훅 형태를 연결한 통합: 두 번째 모양의 거부가 후속 효과를 생략(`begin → shape → terrain → end → shape`)하고 36칸이 표시된다.
6. 누락된 정책·잘못된 풀/타입 번호·비유한 좌표·정책 누락 거부.

최초 실패와 교정: 새 모듈은 7560행 fixture 와 통합 시험을 첫 실행에 통과했다. 직접 쓴 무시 함수 시험의 첫 기대값은 후보 `0x218000` 이 ⑥ 분기(`0x30006` 겹침)에서 먼저 1 로 무시된다는 점을 놓쳐 틀렸고, 분기 순서에 맞게 교정했다(모듈 오류가 아님).

## 3. 경계와 남은 것

- 이 단위로 접두/미리보기/기하/충돌의 실제 연결이 닫혔고, 남은 Canon 계열은 `RawCanonPlacementPipeline`(모든 모듈을 공통 전역에 연결하고 MayPlace 전체 정상 반환 fixture `canonmayplace` 로 대조)이다. 지형(E148) 훅과의 실제 통합은 파이프라인 단위에서 마저 닫는다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawCanonPlacementPipeline`, `RawPriestPlacement*`, `RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
