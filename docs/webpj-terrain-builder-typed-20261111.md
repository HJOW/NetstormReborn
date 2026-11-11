# webpj 본섬 마스크 TerrainBuilder(타입 모델) 완료 확인 — WB04-E289 (2026-11-11, 4-B)

3.1절 4-B 의 **TerrainBuilder**(partial 246줄)는 코드를 더 옮길 것이 없어 **완료 확인**만 했다. 기준선(`1f00a25a`) 뒤 `TerrainBuilder.cpp/.h` 에는 변경이 없다(3.6절 ①). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 함수별 대조

| cpppj `o/TerrainBuilder` | 웹 | 비고 |
| --- | --- | --- |
| 생성자 `TerrainBuilder(chunks, islands)`(004c10a0: 전체 연결 통로 → 청크별 성장 → 두 차례 빈틈 보정) | `buildIslandMask(map, islands, trace)`(terrain-mask.js, WB03-W) | 65536칸 마스크(y×256+x, 허공 255). 465맵 독립 대조(WB03-W) |
| `At(x, y)` | 내부 `cellAt`(월드 밖은 허공) | 마스크 반환값을 직접 색인 |
| `Mask()` | `buildIslandMask` 반환 `Uint8Array` | 독립 마스크 대조용 |
| `Next(state, limit)`(004bca20) | `LocalRandom.next`(terrain-mask.js) | 32비트 오버플로 정수 난수 |
| `Connection(side)`·`Orientation(mask)` | `connectionMask`·`orientationLetter` | 방향 문자 ↔ 연결 비트 |
| `Normalize(cardinal, diagonal)`(0041cd20) | terrain-tiles.js 의 방향 정규화(77행 근처) | 양쪽 직선 이웃이 받쳐 주는 대각선만 남김 |
| 사설 `Put`·`Fill`(004c0930) | `putCell`·`fillMaskGaps`·내부 채우기 | x 바깥·y 안쪽 순서 |
| `kWorldCells`·`kEmptyTerrain`·`kNeighborCells` | `WORLD_CELLS`·`EMPTY_TERRAIN`·`NEIGHBOR_CELLS` | 256, 255, 북부터 시계 방향 여덟 이웃 |

## 2. 범위 밖: GroundCell · GroundGrid · CellPoint

같은 헤더의 `GroundCell`/`GroundGrid`/`CellPoint`(지상 이동 격자와 8방향 최단 경로 `Path`)는 본섬 마스크가 아니라 옛 `client/GameWorld` 모델의 부품이다(헤더가 "원본 탐색 함수 자체를 복원한 것으로 주장하지 않는다"고 적음). 이 부분은 3.1절 4-B 의 **GameWorld·GameWorldInput**(옛 `--dump-world` 어댑터 이식 여부 판정) 항목에서 함께 판정한다(그 항목에 적었다). `CellPoint` 를 `boundary` 목록에 쓰는 `RawRegionPaint` 는 자기 항목에서 필요한 최소 구조만 옮긴다.

## 3. 검증

- `tests/terrain-mask.test.mjs`·`terrain-tiles.test.mjs` 등 기존 검사: 보호 Fort 465개의 본섬 마스크(독립 Python 표), 난수/연결 비트/방향 문자/정규화 입력 전수.
- PORT_MAP 상태만 `partial` → `port`. 코드 변경이 없어 지도/manifest 검사만 다시 돌렸다.

## 4. 남은 경계

- 3.1절 4-B 는 20/31 이다. 다음은 **RawTerrainContext**(partial 171줄, 3.6절 ③ 군도 접근자)이다.
