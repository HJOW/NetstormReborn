# webpj 청크 지도(ChunkMap) 원본 물리 순서·mode·reset — WB04-E291 (2026-11-11, 4-B)

3.1절 4-B 의 **ChunkMap**(partial 159줄)을 `assets/territory.js` 의 `ChunkMap` 확장으로 마쳤다. 원본 `00432180`(항목 찾기)·`00432360`(reset)·`004323c0`(생성자)이며 기준선(`1f00a25a`) 뒤 `ChunkMap.cpp/.h` 에는 변경이 없다(3.6절 ①). 순서 변경: 목록의 RawTerrainGrowth 가 청크 mode 를 읽어야 하므로(패치 첫 시드의 `isValid()` 진단) 이 단위를 앞당겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 함수별 대조

| cpppj | 웹 | 비고 |
| --- | --- | --- |
| `kNoIsland`·`kChunkMapSide`·`kCellsPerChunk`·`ChunkEntry` | `NO_ISLAND`·`WORLD_CHUNKS_PER_SIDE`·`CHUNK_CELLS`·항목 `{ island, side, record[6], ordinal }` | 이미 있음(WB03-V). 방향 글자는 문자열(`''`=0) |
| `DefaultChunkRect`·`IslandList`(`Add`/`Exists`/`Matches`) | `DEFAULT_CHUNK_RECT`·`IslandList` | 이미 있음 |
| `ChunkMap(x, y)` | `new ChunkMap(x = 16, y = 16)` | 1..16 범위 밖 거부(이미 있음) |
| `At(x, y)`(clamp) | `at(x, y)` | **저장 위치를 원본 물리 순서 `entries[x * 16 + y]` 로 바꿈**(이전 `y * 16 + x`; 소비자는 모두 `at()` 로만 접근해 영향 없음) |
| `HasIsland`·`Bounds`·`XLength`·`YLength` | `hasIsland`·`bounds`·`xLength`·`yLength` | 이미 있음 |
| `Mode()` | `mode` getter/setter | **새로**: 원문 첫 DWORD, 기본 1, uint32 로 감김, 수 아닌 값·NaN·Infinity 거부 |
| `Reset()`(00432360) | `reset()` | **새로**: mode 를 보존하고(0 도 0 으로 복원), 현재 가로·세로 안의 항목만 y 바깥·x 안쪽으로 같은 객체에서 섬 0x7f·방향 0·record 0·순번 0 으로 되돌린다. 현재 지도 밖의 고정 256칸 물리 항목은 그대로 둔다 |
| `Entries()` | `entries` | 고정 256칸 배열(원본과 같은 물리 순서) |

## 2. 검사 (`tests/chunk-map.test.mjs` 4건)

- 물리 순서 `entries[x * 16 + y]`(256개 서로 다른 객체)와 가장자리 당김, 작은 지도(4×3)의 당김 위치.
- mode 기본값·uint32 감김(`0x1_0000_0003`→3, -1→`0xffffffff`)·거부(문자열/NaN/Infinity)·독립 객체.
- reset: 모든 활성 항목을 같은 객체에서 비움(256칸 전부 더럽힌 뒤), mode `0x89abcdef`/0 보존, 반복 reset.
- 작은 지도 reset 의 바깥 물리 항목 보존(stale 값 그대로) 4×3 안만 비워짐.
- 기존 `tests/territory.test.mjs` 19건(보호 Fort 465개의 영역 배치 대조)은 순서 변경 뒤에도 그대로 통과한다.
- 독립 원본 관측 파일은 없다: 기준은 원본 `ChunkMap.cpp` 와 cpppj `RawSurfaceStorageTests`(partial reset 의 padding 보존·mode 보존)의 계약 문장이다.
- 변형 확인 16개 중 **2개가 살아남았고 둘 다 동등 변형**이다: reset 의 가로/세로 반복 한계를 16 으로 늘린 변형은 `at()` 이 현재 크기로 당겨서 마지막 활성 열/행을 같은 값으로 한 번 더 지우는 것뿐이라 결과가 같다.

## 3. 남은 경계

- 소비자 `RawTerrainGrowth`(다음 단위)는 `mode`(패치 `isValid()` 진단)·`at()`·`xLength`/`yLength` 를 읽는다. 실제 월드 수명에서 청크 지도를 `RawSurfaceStorage` 의 한 구성원으로 두는 일은 4-B 의 RawSurfaceWorld 단위에서 한다.
- 3.1절 4-B 는 22/31 이다. 다음은 **RawTerrainGrowth**(306줄)이다.
