# webpj 일반 배치 최종 관계(RawCanonPlacementRelations) 이식 — WB04-E150

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백쉰 번째 부분이다. 일반 타입과 특수 패턴 배치(MayPlace) 후반의 현재 표면 소유 관계, 특수 섬 지역의 지면 취소, 최종 소유자 거부, 표시 차단 전역을 옮긴 `o/RawCanonPlacementRelations`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. E148 지형의 `finishRelations` 훅이 부르는 최종 경계이며 E149 권한 helper 의 관계 표를 그대로 쓴다.

## 1. 옮긴 것

`www/core/raw-canon-placement-relations.js`

- `createCanonPlacementRelationsState()` — `{ bypassGroundPermission, islandCount }`(지면/권한만 우회하는 전역 `0059ab30`, 10.78 IslandList `00568b60` 의 부호 있는 개수 DWORD)를 원본 초기값(0)으로 만든다.
- `new RawCanonPlacementRelations(pool, types, surfaces, islands, state, placement, permission)` — `surfaces` 는 현재 표면 지도(65536칸 uint16 SID, 지형 후보의 고정 섬 지도와 별개), `islands` 는 `{ restricted, exists }` 특수 지역 표, `placement` 는 표시 차단 전역(`blockedRelation`)을 가진 객체, `permission` 은 E149 권한 helper(같은 풀)이다. 자료·크기·풀이 어긋나면 거부한다.
- `inspect(query, terrain, priorRejection = 0)` — 지형 누적(E148 `finish`) 뒤의 원본 후반이다.
  1. 표시 차단 genus(`0x34000`, 단 `0x200000` 제외)이면 현재 표면의 유효성·소유자·`related` 관계로 내부 거부(`relationRejected`)를 정하고 표시 차단 전역에 쓰되, `0x4200` 이면 0 으로 되돌린다(다리 표시 genus 는 소유자가 있을 때만 관계를 본다).
  2. 지면이 완성이고 특수 genus(`0x200`)이면 지역 배열 첫 BYTE(부호 확장)가 sentinel 이 아니고 제한된 지역이면 지면 허용을 취소한다(없는 지역·개수 밖 지역은 거부).
  3. `0x200000` 은 즉시 허용(표면 좌표를 읽지 않음). 4. 다리 genus 가 아니고 유효 표면의 소유자가 있어 동맹이 아니며 `0x20a000` 이 아니면 최종 소유자 거부(`foreignRejected=1`)이다. `priorRejection` 은 원본 함수 진입에서 0 인 지역 변수이며 `foreignRejected` 의 초기값이다.
  5. 허용 = (지면/권한 우회 또는 권한과 지면 허용) 이고 내부 거부와 소유자 거부가 없음.
- `surfaceAt(x, y)` — 원래 좌표에 `0.9999f` 를 더해 0 방향 절삭한 뒤 현재 표면 지도 WORD(지도 밖은 0, 비유한 좌표는 거부)를 읽는다.
- `makeCanonRelationsTerrainHooks(pool, relations, hooks)` — 기존 후보/주변 정책을 유지하고 `finishRelations` 만 실제 구현으로 바꾼다. 풀이 다르면 거부한다.

이 모듈은 DOM 을 쓰지 않으며 10.72(CD)의 소유자 BYTE 위치(`+32`)·이전 거부 저장·특수 지역 분기는 범위 밖이다.

## 2. 검증

`tests/canon-placement-relations.test.mjs`(3건):

1. `canonrelations-x86.tsv` 의 `originals` 2307행(전체 6921행 중 10.78) — 타입 genus/플래그, 이전 누적(지면 완성·받침 전용·다리 겹침·권한), 지면/권한 우회, 현재 표면·요청 소유자(부호 BYTE), 편집기/동맹/관계 표 값, 표면 SID·좌표, 지역 배열·sentinel·제한 값, 이전 표시 차단·이전 거부 입력으로 실제 PE 에서 실행한 결과 — 최종 허용·권한·지면 허용·표시 차단 전역·내부 거부·소유자 거부·이전 거부, 그리고 128슬롯 raw·표면 지도·관계 표 Adler-32(자료 불변성) — 가 모두 같다. 실제 E148 지형의 `begin`/`finish` 를 거쳐 지면 누적 뒤 관계 모듈에 도달하며, 허용과 거부 행이 모두 관찰된다. `originalCD`·`original1037` 행은 사용하지 않았다.
2. 필수 자료·풀·크기 거부, 없는 지역/개수 밖 지역/sentinel 이 아닌 지역의 지연 읽기 거부, 항상 허용 genus 가 비유한 좌표를 읽지 않음(직접 조회는 거부), 표시 차단 초기화 마스크, 표 밖 소유자 진단, 지도 밖 좌표.
3. 지형 훅 어댑터가 최종 관계 경계만 바꾸고 다른 훅을 유지함.

최초 실패와 교정: 새 모듈은 2307행 fixture 와 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- C++ 의 일반/패턴 완전 파이프라인 통합 시험(`canon_relations_normal_complete_pipeline`·`pattern_complete_pipeline`)은 충돌/미리보기/주변/배치 접두/Player 기준점 연결이 모두 이식된 뒤 `RawCanonPlacementPipeline` 단위의 통합 시험에서 다룬다.
- `placement.blockedRelation` 을 가진 `PriestPlacementState` 의 실제 소유 모듈은 배치 접두(`RawCanonPlacement`) 단위가 정한다. 이 모듈은 숫자 필드 하나만 쓴다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawCanonPlacementSurrounding`·`Collision`·`Preview`·`Pipeline`·`RawCanonPlacement`, `RawPriestPlacement*`, `RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
