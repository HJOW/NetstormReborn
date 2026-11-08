# webpj 사제 배치 기하와 픽셀 모양(RawPriestPlacementGeometry·RawPriestPlacementShape) 이식 — WB04-E156

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백쉰여섯 번째 부분이다. 사제 배치 계열(접두 E155 다음)의 기하와 픽셀 모양 어댑터를 옮긴 `o/RawPriestPlacementGeometry`·`o/RawPriestPlacementShape`(cpppj 기준선 `1f00a25a`, 두 파일과 시험·fixture 모두 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. 둘 다 사제 배치의 비패턴 계약(사제 genus·패턴 타입 전역 아님)을 검사한 뒤 E146 `RawCanonPlacementGeometry`·E147 `RawCanonPixelShape` 에 위임하는 얇은 어댑터이다.

## 1. 옮긴 것

`www/core/raw-priest-placement-geometry.js`

- `new RawPriestPlacementGeometry(pool, types, frames, state, hooks)` — `hooks = { beginRegions(query), endShape(query, frame, snappedX, snappedY) → bool, finishRegions(query) → bool, beginShape?(query, frame, snappedX, snappedY) }` 앞의 셋은 필수(미복원 지형 판단을 임의로 허용하지 않기 위해). `query = { type, x, y, flags, owner, mode }` 는 사제 genus(`0x200000`)이며 현재 패턴 타입 전역이 아니어야 한다.
- `bounds(query)` — 첫 인자는 타입 번호, 명시 프레임 없음. 실제 비패턴 생성과 현재 발자국으로 미리보기 범위를 계산한다(frame −1 도 계산). `inspect(query, scan)` — 요청을 값으로 캡처해 일반 모양 순회에 위임한다: 생성 뒤 `beginRegions` → 유효 칸마다 [양수 발자국 확인 → (선택) `beginShape` → `scan(finder 사각형)` → `endShape`] → 모두 끝나면 `finishRegions`. `scan`/`endShape` 가 거짓이면 즉시 거짓이고 진행/후처리를 부르지 않는다. 일반 자산의 0 발자국 계산과 구별해 사제의 양수 발자국 계약(1 미만 거부)을 유지한다.
- `RawPriestPlacementGeometry.collisionArea(x, y, footX, footY)` — 0049b8c0..0049b95a 의 finder 인자 산술이며 발자국 1 미만은 거부한다.
- `makePriestGeometryPreviewHooks(pool, geometry, hooks)` — 미리보기 `bounds` 만 실제 비패턴 decoder 에 연결. `makePriestGeometryCollisionHooks(pool, geometry, hooks)` — 충돌 검사기의 모양 순회 `inspectRegions` 를 실제 decoder 로 연결(후보별 지형 경계는 유지). 풀이 다르면 거부한다.

`www/core/raw-priest-placement-shape.js`

- `new RawPriestPlacementShape(pool, types, frames, shapes, hotspots, geometryState, state).measure(type, argument, direction, explicitFrame)` — 사제 genus 이고 패턴 전역이 아닌 타입만 받고(비패턴 계약), 나머지는 E147 일반 픽셀 getter 가 계산한다. 원본처럼 decoder 좌표는 항상 (0,0).
- `makePriestShapePlacementHooks(pool, shape, hooks)` — 배치 접두의 `shape` 경계만 실제 getter 에 연결한다. 배치 경로는 명시 프레임을 켜지 않고 타입 번호를 첫 인자로 그대로 전달한다.

두 모듈 모두 DOM 을 쓰지 않으며 10.72(CD)의 짝수 회전·중간 단정도 저장·직접 SHP 조회는 범위 밖이다.

## 2. 검증

`tests/priest-placement-geometry.test.mjs`(4건):

1. `priestgeometry-x86.tsv` 의 `originals` 2304행(전체 4352행 중 10.78) — 실제 비패턴 decoder(프레임 코드 개수·기본 프레임·인자·부호 DWORD 회전·좌표·명시 프레임·발자국)의 생성 직후 상태(프레임·유효·좌표 비트·미리보기 범위)와 모양별 finder 사각형, 한 칸 진행 뒤 상태가 모두 같다. 유효/빈 칸이 모두 관찰되었는지 센다. 이 행들은 E136 타입 디코더와 E146 사각형 산술을 홀수 회전·큰 방향 값까지 독립 관측으로 다시 검증한다.
2. 지역 경계 호출 순서(`begin → scan → end → finish`)와 현재 발자국/기본 프레임 변경에 대한 값 캡처(사각형 `18:20:21:22`, 범위 `17:19:20:21`), 빈 모양의 `begin/finish`, 첫 거부 뒤 후속 생략.
3. 선택 경계 `beginShape` 는 양수 발자국에서만 불리고 0 발자국은 거부.
4. 필수 경계·표 크기·비사제/패턴 전역·비유한 좌표·누락 scan·다른 풀·0 발자국 거부, 어댑터가 기존 훅을 유지.

`tests/priest-placement-shape.test.mjs`(3건):

1. `priestshape-x86.tsv` 의 `originals` 4032행(전체 8064행 중 10.78) — 사제 비패턴 픽셀 getter 를 인자·방향·명시 프레임·프레임 코드 개수·기본 프레임·타입 flags1·SHP 머리·핫스폿·배율 입력으로 실행한 범위와 기준점 보정의 단정도 비트 여섯 개가 모두 같다.
2. 실제 SHP 크기가 지도 여백 거부를 결정하고(큰 프레임은 거부, 작은 프레임은 하위 경계로 진행) 명시 프레임/빈 범위(기준점 = 현재 배율)를 현재 값으로 읽는다.
3. 패치의 frameCheck(논리 프레임 수·이중 프레임 플래그·해제 표식)와 SHP 없음·표 밖 프레임·명시 프레임·패턴 전역·비사제·비유한 배율·누락 자료·다른 풀 거부.

최초 실패와 교정: 새 모듈 둘은 fixture 6336행과 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- C++ 의 비패턴 모양/미리보기 범위 → finder → 사제 생성자 통합 시험(`priest_geometry_preview_collision_spawn_factory`)은 사제 미리보기·충돌·지형(`RawPriestPlacementPreview`·`Collision`·`Terrain`) 이식 뒤 통합 단위에서 다룬다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawPriestPlacementPreview`·`Collision`·`Terrain`(+ 사제 배치 통합 시험), `RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
