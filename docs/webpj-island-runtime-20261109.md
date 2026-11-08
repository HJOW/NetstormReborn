# webpj 섬 런타임(RawIslandRuntime) 이식 — WB04-E161

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백예순한 번째 부분이다. 실제 100개/32바이트 섬 목록(`IslandList 00568b60`)과 현재 owner 변경 효과(패치 `0046edf0..0046f320`)를 같은 pool/해시/등록 목록/Player 상태에서 실행하는 `o/RawIslandRuntime`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. `RawPriestArrival`(사제 도착의 신전/섬 조회)의 선행이다.

## 1. 옮긴 것

`www/core/raw-island-runtime.js`

- 저장소: 4바이트 count DWORD + 100개 × 32바이트 기록(`+0` 신전·`+4` 존재·`+8` 현재 owner DWORD·`+12` 지역 BYTE·`+13` 원래 owner BYTE·`+16..+28` 경계 float 네 개). `createRawIslandStorage()` 는 3204바이트 장기 배열을, `resetRawIslandRecords(records)`(`0046edf0`)는 세션 없이 100행의 지정 필드(DWORD 3·BYTE 2·float 4, padding 2바이트는 보존)와 마지막 count 만 쓰며 크기가 다르면 쓰기 전에 거부한다.
- `createFortFinishOwnerState()` — 섬/Fort 소유자 처리가 공유하는 모드 상태 `{ chaffMode, special, fort, playerRegions[9](127), invalidRegion(127), residenceType(89) }`. 이후 `RawFortFinishOwner` 이식이 같은 구조를 쓴다.
- `new RawIslandRuntime(pool, hash, records, modes, residences, worldRedraw, mapRevision, hooks, worldExtent = 256)` — `worldRedraw`(`0059a8b0`: owner 변경 때 증가)/`mapRevision`(패치 `005949d4`: 1 저장)은 실제 공유 DWORD 를 빌리는 `{ value }` 상자이고, `hooks = { setResidenceOwner(sid, owner), assertion('MissingIsland' | 'InvalidBounds') }` 둘 다 필수.
- getter/setter — `reset()`·`count`·`exists(i)`(signed 범위, 범위 밖 0)·`add(owner, territory)`(이전 count 반환)·`delete(i)`(기록만 초기화, count/번호 재사용 정책 불변)·`matches(i, owner, territory)`(일반 모드는 원래 owner BYTE, special/chaff 는 현재 owner)·`temple(i)`/`setTemple`·`owner(i)`·`bounds(i)`/`boundsBits(i)`(NaN 페이로드까지 같은 비트, 물리 100개 안에서 missing 진단 뒤에도 읽음)·`setBounds`/`setBoundsBits`·`bytes`·`pool`. 존재하지 않는 섬은 진단 뒤 현재 존재를 다시 읽어 계속한다.
- `setOwner(i, owner)` — Fort 이면서 special 이 아니면 즉시 반환. 아니면 owner 쓰기 → `worldRedraw` 증가(unsigned 감김) → `mapRevision=1` → 현재 경계 검사(진단) → 현재 등록 거주 구역 목록(매 반복 count/SID/type/좌표/표면 WORD 재조회, free/dead 필터 없음)에서 경계 안의 거주 구역에 표면 owner 전파 → Player 첫 지역(127 이면 이 섬 번호). `bindPlayerRecords(players)`(0xb4 간격×9, `+0x78` 첫 지역 DWORD)로 실제 Player BYTE 를 공유하면 `modes.playerRegions` 사본은 쓰지 않는다.

이 모듈은 DOM 을 쓰지 않으며 10.72(CD)의 조용한 getter/setter·경계 unordered 조건·Player 간격(`0xac`)은 범위 밖이다.

## 2. 검증

`tests/island-runtime.test.mjs`(6건):

1. `islandruntime-x86.tsv` 의 `originals` 1009행(전체 중 10.78) — Reset/Count/Exists/Add/Delete/OwnerGet·Set/TempleGet·Set/Match/GetBounds·SetBounds 를 입력 15개(count·섬 번호·존재·owner·원래 owner·지역·모드·경계 9종·callback 변화 7종)로 실제 PE 에서 실행한 결과 scalar, 전체 3204바이트 raw, 출력 경계 비트, 공유 전역 두 개, Player 첫 지역 아홉 개, 효과 순서(`A:B`/`A:E`/`O:`), 등록 목록 count/SID, 거주 구역 owner BYTE, 모드, 진단 횟수가 모두 같다. 열두 종류가 모두 실행되었는지 센다.
2. 같은 1009행을 실제 9 Player BYTE 를 공유하는 모드로 재생: 첫 지역 네 바이트 밖의 Player 메모리는 모두 `0xa5` 로 보존되고 모드 사본은 999 그대로이다.
3. 세션 없는 `resetRawIslandRecords` 가 기존 Reset 행 3개의 전체 3204바이트와 같고 전역/목록/효과를 건드리지 않는다.
4. 별도 장기 배열을 실행기가 계속 읽고 쓰며(padding 보존) 폭이 하나 모자란 입력은 부분 초기화 없이 거부.
5. 물리 100기록 경계·필수 효과·Player 기록 크기·물리 밖 경계 조회 거부.
6. 월드 증가 DWORD 와 지도 DWORD 가 서로 다른 소유자에서 분리 보존: 진단 복귀가 바꾼 현재 존재/전역을 다시 읽고, 가상 효과의 후속 변경(700/99)을 반환 후 사본으로 덮지 않으며, unsigned 감김은 월드 DWORD 에만 적용되고 지도 DWORD 는 1 을 직접 저장한다.

최초 실패와 교정: 새 모듈은 fixture 2018행(두 모드)과 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- `RawFortFinishOwner` 본체(전체 entry·시각 seed·현재 거주 구역 재조회·배치 재시도)는 별도 단위이며 이 문서의 `createFortFinishOwnerState()` 를 공유한다.
- 거주 구역 `setResidenceOwner`(가상 `+0x74`)와 월드/Player 전역의 실제 연결은 호출자(실제 Client 소유자)가 공급한다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawPriestArrival`(사제 +3c 중립 회복/내용물 부착·삭제와 운반체 도착 위임), `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
