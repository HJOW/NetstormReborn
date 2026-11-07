# webpj 대본 AddRandom·DestroyAt 부모 이식 — WB04-E30

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 서른 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawAddRandom`·`o/RawDestroyAt`(10.78)을 옮겼다. [E21](webpj-script-command-inputs-20261108.md)에서 옮긴 일반 입력 분배기(`dispatchAddRandomInput`/`dispatchDestroyAtInput`)가 부르는 실제 부모이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-add-random.js`

- `addRandomObjects(type, quantity, hooks)` [0040dfd0]: `wallSeconds()`로 시각 seed를 만들어(글로벌 `GameRandom`은 바꾸지 않고 새 지역 난수) 기본 범위 청크 1..15를 16배한 칸 `16..240` 사각형을 만들고, **seed 효과 다음에** `local()`로 현재 소유자를 읽어 `island(소유자, 0)`을 조회한 뒤 `place(난수, 사각형, 섬, 수량, 타입, 0)`을 호출한다. 마지막 배치 반환은 쓰지 않는다. 장치(`wallSeconds`→`local`→`island`→`place`)의 누락은 그 위치에서 거부하고 앞선 효과는 되돌리지 않는다.
- `timeSeed(wallSeconds)` [004bcab0→00460d70]: 초×1013을 64비트 절삭의 하위 32비트(`__ftol`; 비유한은 0)로 만들고 `(v>>4)`(SAR)`+v×33`을 32비트로 감는다. `RawFortFinishOwner`도 같은 함수를 쓴다(그 단위가 이 export를 재사용).
- `findFortTerritoryIsland(islands, fort, owner, region)` [0046f380]·`findAnyFortTerritoryIsland(islands, region)` [0046f3f0]: 3204바이트 섬 기록(개수 DWORD + 32바이트 항목: +4 존재, +8 현재 owner, +12 지역, +13 원래 owner)에서 일반은 원래/현재 owner가 모두 요청 owner와 같은 첫 항목, Fort는 현재 owner가 0이거나 요청 owner인 첫 항목(없으면 127), Any는 존재하는 첫 일치. cpppj에서는 `RawFortTerritorySave.cpp`에 있으나 그 파일의 저장 본체(`SnapshotFortTerritory`)는 Edit 전용이라 웹 제외(`exclude-edit`)이고, 이 두 검색만 미션 런타임(AddRandom)이 공유하므로 여기에 둔다.

`www/core/raw-destroy-at.js`

- `parseDestroyAtLocation(source, lookup)` [0044c540]: 첫 NUL 앞에서 맨 앞 `(`만 건너뛰고 앞 공백은 새로 지우지 않으며, 구분자 순서 `x`→`)`→`#`→(타입 이름 `/` 반경)→소유자(개행)로 읽는다. x/y/반경은 `configParseLong`(signed), 타입은 `lookup`의 signed 반환을 DWORD로, 소유자는 atol의 DWORD. `lookup` 누락은 토큰을 읽은 뒤 거부한다.
- `destroyObjectsAt(source, hooks)` [0044c790]: 권한이 없으면 진단 839 → 권한 재조회, 위치 해석 뒤 x/y를 `int→float32→ftol` 두 번으로 정수화한 점 `(x,y,x,y)`을 `begin`으로 검색하고, 반환된 현재 항목마다(0이거나 현재 풀 상한 이상이면 끝) 타입(0이면 전체)·분류 `0x7c77ef00`·소유자(0이면 전체)를 raw의 **늦은 읽기**로 판정해 맞으면 `destroy(sid, 0x400000)` 뒤 `next()`로 다음 항목을 조회한다. 반경은 해석만 하고 무시(원문 동일).

## 2. 검증 (`tests/add-random-destroy-at.test.mjs`, 5건)

원본 10.78 관측(`cpppj/tests/fixtures`를 직접 읽음) 1540행:

- `add-random-parent-x86.tsv` 720행: 시각 seed(난수 상태)·기본 범위 `16/16/240/240`(float 비트)·현재 소유자 섬·수량/타입/force·벽시각 효과가 소유자/Fort를 바꾸는 변형.
- `destroy-at-parse-x86.tsv` 16행: 토큰 구분자·따옴표·길이 제한·NUL·atol과 다섯 출력, 타입 조회 호출 기록.
- `destroy-at-parent-x86.tsv` 804행: 권한(없음/진단 뒤 복구)·점 정수화·필터 루프·장치 효과(분류/삭제)가 바꾸는 raw의 늦은 읽기·4슬롯 전체 raw 200바이트.

계약 검사: `timeSeed`의 0/NaN/무한/음수, 섬 검색 규칙과 손상된 큰 count의 접근 시 거부, 이미 실행된 벽시각 효과 뒤의 장치 누락, 권한 진단 한 번과 이후 장치 누락, 타입 조회 누락.

최초 실패와 교정: 5건이 처음부터 통과했다.

## 3. 경계

- 실제 월드 배치 몸체(`FortRandomPlace`/`PlaceRandom`, 0040ded0)와 삭제 가상(+10)·점 검색(`RawSquidFinder`, 이미 옮김)의 실제 연결은 `RawFortFinishOwner`·Damageable 소비자 단위에서 한다. 여기서는 훅으로만 주입했다.
- 대본 문자열 고리·큐 예약(`QueueScriptCommand`)은 대본 큐 단위에서 대조한다(`destroy-at-input`/`add-random-input` 일반 입력은 E21에서 이미 대조).
- CD 10.72는 범위 밖이다.

## 4. 다음 시작점

같은 대본 부모 `RawCreateAt`(`create-at-parent-x86.tsv` 580행, `CreateAtTests.cpp`)·`RawMakeAi`(`make-ai-parent` 770행·`make-ai-objects` 2592행, `MakeAiTests.cpp`)를 옮길 수 있다. 큰 묶음은 `RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체, `RawSquidPoolReset`, `RawBridgeConnect`, Player 나머지이다.
