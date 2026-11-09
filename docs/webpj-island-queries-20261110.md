# webpj 섬 검색(RawIslandQueries) 이식 — WB04-E220

2026-11-10, HJOW-Athlon. 순서3 마감 전 "기준선 뒤 변경이 없는 `o/` 단위" 가운데 섬 목록(E161 `raw-island-runtime.js`)의 읽기 전용 검색 세 몸체(패치 0046f380·0046f3f0·0046f430)를 옮겼다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위(… RawIsland* …)"에 속하지만 소비자 연결 없이 따로 옮길 수 있고 원본 관측이 있어 먼저 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawIslandQueries.cpp/.h`)·시험(`IslandQueriesTests`)·fixture(`islandqueries-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 루트에서 확인). 선행 `RawIslandRuntime.cpp` 는 기준선 뒤 변경(+57/−23)이 있으나 이 단위는 그 getter(`count`·`exists`·`owner`)만 읽는다. 선행 모듈의 기준선 뒤 변경은 3.6절 판정 대기에 포함되어 있다.
- 근거 커밋: 소스·fixture `5b10ce82`.
- 원본 fixture: `islandqueries-x86.tsv` 의 10.78(originals) 4245행(originalCD 4245행은 웹 범위 밖). 입력 12칸(count·존재 패턴·존재값·현재 owner·원래 owner·지역·owner 인자·지역 인자·fort·special·chaff·추가 존재 위치)에서 세 검색의 반환, 전체 3204 BYTE 불변(Adler-32), 세 모드, revision 불변을 대조한다.
- 공유 장치: E161 `raw-island-runtime.js`(`RawIslandRuntime`·`createFortFinishOwnerState`·`createRawIslandStorage`).

## 1. 옮긴 것

`www/core/raw-island-queries.js`(DOM 없음, 10.78 전용)

- `ISLAND_NOT_FOUND`(127)·`RawIslandQueries(islands, modes)`: 같은 월드의 섬 목록(10.78)과 현재 fort 모드를 참조하며 별도 섬 벡터를 만들지 않는다.
- `findOwnerTerritory(owner, territory)`(0046f380): **시작할 때의 count 와 fort 모드를 한 번만 읽어** 반복 내내 쓴다(패치). 존재하는 기록만, 일반 모드는 signed 원래 owner BYTE(+13)와 현재 owner DWORD 가 모두 인자와 같아야 하고, fort 모드는 현재 owner 0 도 허용한다. 이어서 signed 지역 BYTE(+12)가 같은 첫 번호를 돌려준다. 없으면 127.
- `findTerritory(territory)`(0046f3f0): fort/special/chaff/owner 를 읽지 않고 존재 DWORD 가 비영인 첫 기록의 signed 지역 BYTE 를 비교한다.
- `countOwned(owner)`(0046f430): 존재하는 기록 중 현재 owner DWORD 가 같은 개수(owner 0 도 계수). count 를 매 반복 다시 읽고 삭제 뒤 줄지 않는 원본 정책을 유지한다.
- BYTE 필드는 MSVC signed char 처럼 32비트 부호 확장해 읽는다(지역 255 = −1). 음수(0xffffffff) count 는 빈 순회이고 물리 100기록을 넘는 count 는 섬 목록이 거부한다.

## 2. 검증

`tests/island-queries.test.mjs`(4건)

- 원본 관측 재생: 10.78 4245행 전부(FindOwnerTerritory·FindTerritory·CountOwned). 전체 3204 BYTE 사본 비교와 Adler, 모드, 공유 전역 불변을 확인한다.
- cpppj 계약 이식 1건: 실제 Add/Delete/Reset 과 현재 fort 가 다음 검색에 즉시 반영되고 물리 100기록 밖 접근이 거부된다.
- 웹 계약 2건: signed 부호 확장과 판본 거부, 시작 count/fort 고정(반복 중 바뀐 값을 쓰지 않음).

최초 실패와 교정: 모듈은 처음부터 4245행과 계약이 모두 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(cmp 일치).

- 실패한 변형: signed 부호 확장 제거(2건), 반복 중 fort 재조회(1건), fort 모드의 현재 owner 0 허용 제거(3건), `countOwned` 의 owner 0 제외(2건).
- 적용되지 않은 변형: 시작 count 재조회(sed 패턴 불일치). 시작 count 고정은 호출 전후 값이 바뀌어도 `exists` 가 물리 count 밖에서 0 을 돌려주는 실제 목록에서는 관측되지 않아 가짜 목록 시험으로만 확인한다.

## 3. 경계와 남은 것

- 실제 월드에 연결하지 않았다. 호출 지점(Fort 로드·AI·의식 등의 섬 조회)은 순서4 이후 몫이다(PORT_MAP session 0). CD 10.72 는 웹 범위 밖이다(CD 는 반복마다 count/fort 를 다시 읽는다).

## 4. 다음 시작점

WB04-E221 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 `o/` 단위(`RawStateWorldCleanup`·`SquidSpatial`·`RawTerrainGrowth`·`RawManaBoltRegular` 등)를 권한다.
