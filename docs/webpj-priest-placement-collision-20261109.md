# webpj 사제 배치 미리보기와 충돌(RawPriestPlacementPreview·RawPriestPlacementCollision) 이식 — WB04-E157

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백쉰일곱 번째 부분이다. 사제 배치 계열(접두 E155, 기하·픽셀 모양 E156 다음)의 로컬 미리보기와 후보 충돌을 옮긴 `o/RawPriestPlacementPreview`·`o/RawPriestPlacementCollision`(cpppj 기준선 `1f00a25a`, 두 파일과 시험·fixture 모두 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. 둘 다 사제의 기존 여섯 값 인자(`type, x, y, flags, owner, mode`)를 E152 일반 미리보기·E153 일반 충돌 계산기에 연결하는 어댑터이며 두 계층은 같은 12×12 차단 배열과 같은 클라이언트/무시 전역을 공유한다.

## 1. 옮긴 것

`www/core/raw-priest-placement-preview.js`

- `new RawPriestPlacementPreview(pool, types, spots, surfaceMap, state, hooks)` — `hooks = { bounds(priestQuery) → 정수 사각형, inspectCollisions(priestQuery, localOwner) → bool }` 둘 다 필수(비어 있으면 생성 시 거부). 콜백에는 일반 요청을 원래 사제 여섯 값 인자로 되돌려 전달한다. `inspect(query, localOwner)` 는 첫 인자를 타입 번호로 하여 일반 미리보기에 넘기며 로컬 비교를 다시 하지 않는다.
- `makePriestPlacementPreviewHooks(pool, preview, hooks)` — 사제 접두의 `inspectGeometry` 만 실제 미리보기로 바꾸고 기존 모양 조회는 유지한다. 풀이 다르면 거부한다.

`www/core/raw-priest-placement-collision.js`

- `new RawPriestPlacementCollision(pool, hash, types, state, preview, hooks)` — `hooks = { inspectRegions(priestQuery, scan), inspectCandidateTerrain(priestQuery, sid) }` 둘 다 필수. `inspectCandidate(query, localOwner, sid)` 는 매 후보마다 현재 사제 genus(`0x200000`)·유한 좌표 계약을 확인하고 E153 의 공통 무시/표시/거부 본문을 쓴다. `inspectArea` 는 실제 finder 의 `begin`/`next` 를 쓰고 첫 거부 뒤에는 후속 후보를 읽지 않으며, `inspect` 는 지역 경계가 거부 이후 `scan` 을 다시 요청하거나 성공을 덮어쓰지 못하도록 거부 상태를 유지한다.
- `makePriestPlacementCollisionHooks(pool, collision, hooks)` — 미리보기의 `inspectCollisions` 만 실제 후보 검사/탐색기로 연결한다. 무시 함수는 E153 `placementIgnoreValue`(0049ade0)를 그대로 쓴다.

두 모듈 모두 DOM 을 쓰지 않으며 10.72(CD)의 bridge 비트/전역 마스크/extra 위치/선행 linear spot 읽기 차이는 범위 밖이다.

## 2. 검증

`tests/priest-placement-preview.test.mjs`(3건):

1. `priestpreview-x86.tsv` 의 `originals` 1620행(전체 4860행 중 10.78) — 실제 사제 배치 접두와 미리보기를 사각형 12종×지도 5종×전역/소유자 9종×조회 중 변화 3종으로 실행한 허용 결과, 표시 차단 전역, 편집기/동맹/관계 표 전역, 표면 타입 genus, 144바이트 차단 배열(부분 쓰기 포함), 표면 슬롯 raw, spot·표면 지도 Adler-32, 모양/충돌 경계 호출 기록(`B:`/`R:`/`C:`/`H:`)이 모두 같다. 허용/거부/지도 가장자리 부분 배열이 모두 관찰되었는지 센다.
2. 필수 경계·지도 크기·열 12칸 초과·정수 최댓값 산술·y=254 부분 배열·음수 관계 인덱스·다른 풀 거부.
3. 미리보기의 차단 표시(9칸)가 실제 배치 허용 결과와 독립인지 사제 생성(`RawPriestSpawn`)까지 연결해 확인(충돌 경계가 참을 돌려 한 번에 채택, 생성 SID 15000, WORD `0x8101` 복사).

`tests/priest-placement-collision.test.mjs`(5건):

1. `priestcollision-x86.tsv` 의 `originals` 3840행(전체 11520행 중 10.78) — 원본 무시 함수 반환(I: 비트값 전체)과 한 후보 인라인 구간(C: 후보 결과·144바이트 미리보기 배열·후보 raw 슬롯 전체)이 같다. 두 종류와 통과/거부가 모두 관찰되었는지 센다.
2. 실제 finder 의 buried 제외·전역 무시 마스크와 첫 거부 이후 탐색 중단(다음 링크를 잘못된 값으로 두어 더 읽지 않음을 확인).
3. 후보 충돌 거부를 접두·미리보기·finder·사제 생성자에 연결해 첫 후보(blocker 와 충돌)를 거부하고 다음 위치(21,21)에서 생성(지역 경계 2회, 생성 SID 15001).
4. 외부 경계 누락·사제 genus 없음·비유한 좌표·다른 풀 거부.
5. 충돌에서 무시한 후보도 지형 경계를 받고 finder 가 미리 저장한 next 를 사용(지형 경계가 첫 후보의 링크를 끊고 전역을 바꿔도 두 번째 후보가 새 전역으로 검사됨).

최초 실패와 교정: 새 모듈 둘은 fixture 5460행과 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 사제 배치 계열의 남은 층은 `RawPriestPlacementTerrain`(후보 지형·지역 배열·모양 종료 지면)이다. C++ 의 비패턴 모양/미리보기 범위 → finder → 사제 생성자 통합 시험(`priest_geometry_preview_collision_spawn_factory`)은 지형 이식 뒤 그 단위에서 함께 닫는다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawPriestPlacementTerrain`(+ 사제 배치 통합 시험), `RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
