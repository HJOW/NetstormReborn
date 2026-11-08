# webpj 일반 배치 권한(RawCanonPlacementPermission) 이식 — WB04-E149

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백마흔아홉 번째 부분이다. 일반 타입과 특수 패턴 배치의 후보 권한 함수(`00462cb0`)와 방향 관계 helper(`004629e0`)를 옮긴 `o/RawCanonPlacementPermission`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. E148 지형 계층의 `candidatePermission` 훅에 실제 권한 helper 를 연결하는 Canon 배치 계열의 세 번째 층이다.

## 1. 옮긴 것

`www/core/raw-canon-placement-permission.js`

- `createCanonPlacementPermissionState()` — `{ graphReady, editor, useAlliances, allowOtherOwners, alliances(Uint32Array 81칸) }` 를 원본 초기값(모두 0)으로 만든다.
- `new RawCanonPlacementPermission(pool, state, playerAnchor)` — `playerAnchor(owner, type, x, y, xBits, yBits)` 는 Player 작업장/그래프 locator 로 필수 경계이며 DWORD 를 돌려준다. `xBits`/`yBits` 는 NaN 페이로드까지 같은 원본 DWORD 비트이다(JS 수는 NaN 비트를 보존하지 못해 추가한 인자).
- `candidate(sid, owner)` — 그래프 비활성이면 0, 요청자 0 이면 1, 후보 raw 소유자(BYTE `+34`)가 관계가 없고 중립이 아니며 다른 소유자 허용이 꺼져 있으면 0, 아니면 Player 조회 결과(DWORD 비트 그대로)를 돌려준다. 전역과 raw 는 호출 때마다 다시 읽는다.
- `related(currentOwner, owner)` — 편집기이거나 동일 소유자이면 참, 방향 동맹을 쓰지 않으면 거짓, 아니면 관계 표를 읽는다. `alliance(currentOwner, owner)` — 관계 표 직접 읽기(인덱스 `현재×9+요청자`는 DWORD 로 감기고 표 밖은 진단)로 편집기/동일 소유자/활성 조건을 붙이지 않는다.
- `surrounding(sid, owner)` — 주변 순회의 바깥 관계 검사(중립에도 적용, 다른 소유자 허용은 마지막)를 먼저 하고 통과하면 `candidate` 이다.
- `makeCanonPermissionTerrainHooks(pool, permission, hooks)` — 지형 훅의 후보 권한만 실제 helper 에 연결한다. 요청 소유자는 하위 BYTE 를 부호 확장한 DWORD 로 전달하고 질의의 원래 값은 유지하며, 풀이 다르면 거부한다.

이 모듈은 DOM 을 쓰지 않으며 10.72(CD)의 소유자 BYTE 위치(`+32`) 차이는 범위 밖이다.

## 2. 검증

`tests/canon-placement-permission.test.mjs`(4건):

1. `canonpermission-x86.tsv` 의 `originals` 5850행(전체 16890행 중 10.78) — 후보 권한(C)과 방향 관계 helper(R)를 그래프 준비/편집기/방향 동맹/다른 소유자 허용 전역, 현재 소유자·요청자, 정방향/역방향 관계 표 값, Player 조회 응답, 후보 SID/타입/좌표(부호 있는 0·NaN 포함), 조회 중 raw·전역 변경 입력으로 실제 PE 에서 실행한 결과 — 반환 DWORD·Player 조회 횟수와 인자 비트 네 개·최종 raw 슬롯 전체·편집기 값·관계 표 Adler-32 — 가 모두 같다. `originalCD`·`original1037` 행은 사용하지 않았다.
2. 요청 DWORD(`0xabff`)를 유지하고 후보 helper 에는 부호 있는 BYTE 소유자(`0xffffffff`)를 전달하며 NaN 페이로드(`0x7fc12345`)와 부호 있는 0 좌표를 보존하고, Player 조회 중 raw 소유자가 바뀌면 다음 호출이 현재 raw 를 다시 읽어 거부한다. 그래프 비활성과 요청자 0 은 잘못된 SID 도 읽지 않는다.
3. 주변 순회의 바깥 관계 검사가 중립에도 적용되고 다른 소유자 허용·동일 소유자·그래프 비활성 순서를 지킨다.
4. Player 경계 누락·관계 표 크기 오류·다른 풀·표 밖 소유자 거부, DWORD 감김(1×9+`0xffffffff`=8번 칸), 편집기 우선.

최초 실패와 교정: 새 모듈의 5850행 fixture 는 첫 실행에 통과했다. 시험 작성 중 `alliance(1, 0xffffffff)` 를 표 밖 거부로 잘못 기대했으나 DWORD 감김으로 8번 칸을 읽는 것이 원본 계약이어서 시험을 교정했다.

## 3. 경계와 남은 것

- C++ 의 실제 3×3 decoder/미리보기/finder/지형 통합 시험(`canon_permission_pattern_terrain_integration`)은 충돌/미리보기/배치 접두 모듈 이식 뒤 그 단위의 통합 시험에서 다룬다.
- 실제 Player 작업장 조회(`playerAnchor`)는 `RawPlayerPlacementAnchor`(E57)가 공급하며 이 모듈과의 연결은 배치 파이프라인 단위에서 한다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawCanonPlacementRelations`·`Surrounding`·`Collision`·`Preview`·`Pipeline`·`RawCanonPlacement`, `RawPriestPlacement*`, `RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
