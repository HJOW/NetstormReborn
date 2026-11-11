# webpj 영역 패턴 배치(Islandbuilder, 타입 모델) 완료 확인 — WB04-E288 (2026-11-11, 4-B)

3.1절 4-B 의 **Islandbuilder**(partial 117줄)는 코드를 더 옮길 것이 없어 **완료 확인**만 했다. 기준선(`1f00a25a`) 뒤 `Islandbuilder.cpp/.h` 에는 변경이 없다(3.6절 ①). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 함수별 대조

cpppj `o/Islandbuilder.h` 의 공개 항목 전체가 이미 `www/assets/territory.js`(WB03-V)에 있다.

| cpppj | 웹 | 비고 |
| --- | --- | --- |
| `kTerritoryRecordBytes`·`kTerritoryCount` | `TERRITORY_RECORD_BYTES`·`TERRITORY_COUNT`(territory.js 안) | 6, 20 |
| `IslandBuilder(map, islands, pieceFrames, owner, rect)` | `new IslandBuilder(map, islands, frameCodes, owner, rect = DEFAULT_CHUNK_RECT)` | 기본 사각형 1..15 |
| `Place(record, territory, ignorePosition)`(0046dba0) | `place(record, territory, ignorePosition)` | 위치 니블(하위 x·상위 y)·`islands.add(owner, territory)`·모양 번호/방향·`CanonDecoder` 칸마다 청크 항목(섬·record 6·순번·방향 글자). 웹은 지도 밖으로 당겨진 칸 수와 프레임 누락 수를 센다(원본 파일은 둘 다 0) |
| `PlaceAll(territories, start, end)`(0046dd70) | `placeAll(territories, start, end)` | `start == end` 거부(원본 assert), flags bit0 켜짐·bit1 꺼짐만 놓음 |
| `TerritoryChunks(map, islands, owner, territory)`(004be2c0) | `territoryChunks(map, islands, owner, territory)` | y 바깥·x 안쪽 순서 |

헤더 주석 "섬 지형 만들기(0046da70 이후), 전투용 섬 배치는 아직 옮기지 않았다"는 이 타입 모델 파일의 범위 밖 설명이다. 그 실제 원본 몸체(`Place`/`PlaceAll` 의 raw 풀 판)는 **`RawIslandBuilder`**(E222, `core/raw-island-builder.js`)이고 월드 부모는 E286/E287 이 사용한다. 이 타입 모델은 소비자(웹의 개발용 지형 미리보기 `layoutFort`, `tests/territory.test.mjs`)가 쓴다.

## 2. 검증

- `tests/territory.test.mjs`(기존): 원본 영역 패턴 표·방향 회전·프레임 번호 검색·`IslandBuilder` 배치·`territoryChunks` 순서를 보호 Fort 465개와 독립 기대표(Python 도구가 만든 표; `www/` 구현의 출력으로 다시 만들지 않음)로 대조한다.
- PORT_MAP 의 상태만 `partial` → `port` 로 올렸다(웹 모듈 `www/assets/territory.js`·`terrain-mask.js` 의 다른 단위와 공유하므로 같은 칸 유지). 코드 변경이 없어 전체 검사는 직전 단위(E287)와 같고, 지도/manifest 검사만 다시 돌렸다.

## 3. 남은 경계

- 3.1절 4-B 는 19/31 이다. 다음은 **TerrainBuilder**(partial 246줄)이다.
