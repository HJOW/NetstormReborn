# webpj 일반 배치 접두와 미리보기(RawCanonPlacement·RawCanonPlacementPreview) 이식 — WB04-E152

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백쉰두 번째 부분이다. 일반 타입과 특수 패턴 배치(MayPlace `0049b510`)의 접두 — 차단값 초기화, 강제/즉시 허용, 부호 확장한 소유자의 로컬 비교, 모양 여백, 지도 경계 — 와 그 뒤의 로컬 미리보기(12×12 차단 배열·표면 지도·소유 관계)를 옮긴 `o/RawCanonPlacement`·`o/RawCanonPlacementPreview`(cpppj 기준선 `1f00a25a`. `RawCanonPlacementPreview`·두 시험·fixture 는 기준선 뒤 변경이 없고, `RawCanonPlacement.cpp` 는 `a84509c9`(20261107-48 CP)에서 강제 허용 관문이 공유 getter(`ForcePlacement()`)를 읽도록 한 줄 바뀌어 그 판을 옮겼다. `git diff --numstat` 으로 확인)이다. 미리보기 fixture 가 접두를 거쳐 재생되므로 한 단위로 묶었다. E146 기하·E147 픽셀 모양·E148 지형·E149 권한·E150 관계·E151 주변 순회 위에 이어지는 Canon 배치 계열의 일곱·여덟 번째 층이다.

## 1. 옮긴 것

`www/core/raw-canon-placement.js` (접두)

- `createPlacementState()` — 배치 검사가 공유하는 전역 `{ forcePlacement(0055a488), localPlayer(00540c70), blockedRelation(0059ab2c), sharedForcePlacement(실제 Client 의 값을 빌리는 getter 또는 null) }` 를 원본 초기값으로 만든다. `currentForcePlacement(state)` 는 즉시 허용 관문에서 현재 전체 DWORD 를 읽는다. 사제 배치도 같은 구조를 쓴다.
- `new RawCanonPlacement(pool, types, state, hooks).mayPlace(query)` — `query = { type, argument, x, y, flags, owner, mode }`(값으로 복사). 타입 70 이상·유한 좌표를 요구하고, 차단값을 먼저 0 으로 지운 뒤 강제 허용 전역이거나 타입 genus 에 즉시 허용 비트(`0x02000000`)가 있으면 참이다. 아니면 요청 소유자의 하위 BYTE 를 부호 확장해 `localPlayer` 와 비교(모양 조회 전에 값으로 캡처)하고, `hooks.shape(type, argument, flags)` 의 사각형으로 지도 여백 `trunc(max((오른쪽−왼쪽)×0.0625, float((아래−위)×0.09090909361839294)))` 를 구해 `x`·`y`·`256−x`·`256−y` 중 하나가 여백보다 작으면 거짓, 아니면 `hooks.inspectGeometry(query, localOwner)` 가 결정한다.
- `makeCanonShapePlacementHooks(pool, shape, hooks)` — 공통 모양 경계만 E147 픽셀 getter(전체 패턴, 명시 프레임 없음)로 교체한다.

`www/core/raw-canon-placement-preview.js` (미리보기)

- `createCanonPlacementPreviewState()` — `{ blocked(144바이트, x 가 바깥인 12×12), editor, useAlliances, alliances(81칸) }`.
- `new RawCanonPlacementPreview(pool, types, spots, surfaceMap, state, hooks).inspect(query, localOwner)` — 비로컬 요청은 배열을 지우거나 모양을 조회하지 않고 곧바로 충돌 경계(`hooks.inspectCollisions`)로 간다. 로컬 요청은 배열을 지우고 모양 범위(`hooks.bounds`)를 조회해 왼쪽/위를 1 로 올린 뒤 오른쪽 아래+1 에서 왼쪽 위−1 까지 y 를 먼저 줄이고 각 행에서 x 를 줄이며 칸을 검사한다. 10.78 은 지도 밖이거나 y 가 254 이상이면 배열을 부분만 쓴 채 즉시 거짓이다. 칸이 막혔으면(spot `0x10`, 지도 밖, 표면 없음, 표면 genus 2 이면서 소유 관계 없음) `blocked[x번호×12+y번호]=1` 이다. 소유 관계는 편집기이거나 같은 소유자이거나 방향 동맹 표(표면 소유자×9+부호 확장한 요청자, 음수/표 밖은 진단)이다.
- `makeCanonPlacementPreviewHooks(pool, preview, hooks)` — 접두의 `inspectGeometry` 만 미리보기로 연결. `makeCanonGeometryPreviewHooks(pool, geometry, hooks)` — 모양 범위만 E146 기하 모듈로 연결(별도 패턴 인자와 방향 비트 보존, 명시 프레임 없음). 풀이 다르면 거부한다.

두 모듈 모두 DOM 을 쓰지 않으며 10.72(CD)의 중간 단정도 저장 여백, 선행 linear spot 읽기, y=254 조기 거부 부재는 범위 밖이다.

## 2. 검증

`tests/canon-placement.test.mjs`(3건):

1. `canonplacement-x86.tsv` 의 `originals` 1120행(전체 3360행 중 10.78) — 모양 10종×좌표 16종×전역/소유자 7종을 실제 PE 의 접두로 실행한 허용 결과, 차단 전역, 강제 허용·로컬 플레이어 전역, 타입 genus, 모양/후반 경계 호출 기록(`H:`/`G:`)이 모두 같다. 모양 조회 중 전역 변경과 후반 경계의 전역 변경도 공급한다. 허용/거부/모양 조회가 모두 실제로 실행되었는지 센다.
2. 일반 타입 처리, 요청 객체와 로컬 비교의 값 캡처, 차단값 초기화, 강제 허용·즉시 허용 genus, 타입 69/255·비유한 좌표·누락 경계·표 크기 거부, 공유 강제 허용 getter.
3. 실제 복원 다리 패턴과 SHP 를 접두에 연결해 argument 와 방향 전달, 지도 가장자리 거부, 다른 풀 거부.

`tests/canon-placement-preview.test.mjs`(3건):

1. `canonpreview-x86.tsv` 의 `originals` 1620행(전체 4860행 중 10.78) — 사각형 12종×지도 5종×전역/소유자 9종×조회 중 변화 3종을 접두를 거쳐 실제 PE 로 실행한 허용 결과, 표시 차단 전역, 편집기/동맹/관계 표 전역, 표면 타입 genus, 144바이트 차단 배열(부분 쓰기 포함), 표면 슬롯 raw, spot·표면 지도 Adler-32, 모양/충돌 경계 호출 기록이 모두 같다.
2. 실제 E146 기하 모듈의 범위(`17,18,20,21`)를 연결하고 36칸이 표시된 뒤에도 후반 거부가 독립이며, 비로컬 요청은 배열을 건드리지 않고 충돌 경계만 부른다.
3. 필수 경계·지도 크기·열 12칸 초과·정수 최댓값 산술·y=254 부분 배열·음수 관계 인덱스 거부.

최초 실패와 교정: 두 새 모듈은 fixture 2740행과 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 충돌(`RawCanonPlacementCollision`)·파이프라인(`RawCanonPlacementPipeline`) 단위가 남았고, 충돌이 들어오면 이 문서의 접두+미리보기+기하+충돌 통합 시험(C++ `canon_collision_pattern_preview_geometry_stops_at_second_shape` 등)을 함께 옮긴다.
- 사제 배치(`RawPriestPlacement*`)는 같은 `createPlacementState()` 를 쓰는 별도 단위다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawCanonPlacementCollision`·`Pipeline`, `RawPriestPlacement*`, `RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
