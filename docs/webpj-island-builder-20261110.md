# webpj 섬 Builder(RawIslandBuilder) 이식 — WB04-E222

2026-11-10, HJOW-Athlon. 영역(Territory) 패턴을 현재 `ChunkMap` 과 3204 BYTE 섬 목록(E161 `RawIslandRuntime`)에 원본 순서로 놓는 Place(0046dba0)·PlaceAll(0046dd70)을 옮겼다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위(Fort 적재·지형/섬 …)"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 섬 목록·검색(E220)·Fort 소유자 후처리(E221)에 이어 옮겼다. 섬 population(`RawIslandPopulation`)이 이 Builder 를 요구한다.

- cpppj 기준선 `1f00a25a`. 소스(`RawIslandBuilder.cpp/.h`)·시험(`IslandBuilderTests`)·fixture(`islandbuilder-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 루트에서 확인).
- 근거 커밋: 소스 `2a8f937e`, fixture `5bdb5bed`.
- 원본 fixture: `islandbuilder-x86.tsv` 의 10.78(originals) 578행(Place 548·PlaceAll 30; originalCD 578행은 웹 범위 밖). 입력 14칸(월드/임시 지도·owner·territory·기존 count·builder 경계 4필드·ignorePosition·프레임 프로필·청크 16×16·start/end)과 6 BYTE 레코드(또는 20레코드), 출력은 반환 지역·대상 지도 256청크 12 BYTE 전체(섬 번호/방향 글자/레코드 6 BYTE/순번)·임시 지도일 때 월드 지도 불변·3204 BYTE 섬 목록 전체(count 밖 padding 포함)·진단 순서(B·E·F·R·M)와 횟수다.
- 공유 장치: WB03-V `assets/territory.js`(`ChunkMap`·`CanonDecoder`)·`territory-patterns.js`, E161 `raw-island-runtime.js`(`add`·`exists`·`setBounds`).

## 1. 옮긴 것

`www/core/raw-island-builder.js`(DOM 없음, 10.78 전용)

- `ISLAND_BUILDER_ASSERTION`(`invalidBounds`·`equalRange`·`missingFrame`·`chunkRange`·`boundsMissingIsland`)과 `RawIslandBuilder(target, world, islands, frames, puzzlePieceType, state, diagnostic)`: 188칸 프레임 표(`{ side, variant, number, flags }` 배열)·현재 puzzlePiece 타입 getter·`state = { bounds, owner, decoderDiagnostics }`·필수 진단 함수를 구성 때 확인한다.
- `place(record, territory, ignorePosition)`(0046dba0): builder 경계 네 필드 중 하나가 −1 이면 진단(InvalidBounds) 뒤 계속한다. 레코드 위치 nibble(하위 x·상위 y)을 경계 left/top 에 더해(DWORD 감김·float32 저장) 시작 좌표를 만들고, **월드 지도일 때만** `islands.add(owner, territory)` 로 지역 번호를 얻는다(임시 지도는 전달 지역 유지). 현재 puzzlePiece 타입의 프레임 표와 영역 패턴(하위 6비트)·방향(상위 2비트)으로 `CanonDecoder` 를 만들어 유효 칸마다 지도 항목(섬 번호 BYTE·레코드 6 BYTE·순번·방향 글자)을 쓴다(칸 좌표는 `_ftol` 하위 DWORD). 칸이 지도 밖이면 진단(ChunkRange) 뒤 `ChunkMap.at` 이 가장자리로 당긴다. 월드 지도이면 끝에서 섬 존재 진단(BoundsMissingIsland) 뒤 청크 경계를 16배 float 로 섬 목록에 저장한다. 지역 번호를 돌려준다.
- `placeAll(records, start, end)`(0046dd70): 물리 20레코드(120 BYTE) 밖/범위 밖 start·end 는 쓰기 전에 거부하고, start==end 이면 진단(EqualRange) 뒤 빈 순회, 각 레코드의 **현재** flag(bit0 켜짐·bit1 꺼짐)를 읽어 `[start, end)` 순서로 place 를 부른다(정렬 없음).
- `inWorld()`: 대상 지도와 world 지도의 객체 동일성만 비교한다(같은 크기의 사본은 월드가 아님). `pool` 은 섬 목록의 풀이다.

## 2. 검증

`tests/island-builder.test.mjs`(4건)

- 원본 관측 재생: 10.78 578행 전부. 행마다 새 풀/지도/섬 저장소를 만들고(청크 항목은 섬 127·방향 `.`·레코드 0xa5·순번 0x12345678, 섬 목록 전체 0xa5 + count) 위 출력 전부를 대조한다. 프레임 프로필 4종(정상·역순·빈 표·G 방향 누락)으로 진단/쓰기 교차 순서를 본다.
- cpppj 계약 이식 2건: 형식/필수 진단/물리 Terr 범위의 효과 전 거부와 정상 empty 범위, 실제 Builder 가 패턴을 쓰기 전에 현재 Canon 진단(0x325)을 읽고 중단하면 청크를 쓰지 않음.
- 웹 계약 1건: puzzlePiece 타입이 표 밖이면 거부하고 임시 지도는 전달 지역을 유지하며 섬 목록에 등록하지 않음. (패턴 번호 하위 6비트는 63 까지이고 표가 68개라 표 밖이 될 수 없음을 확인해 해당 가정을 시험에서 뺐다.)

최초 실패와 교정: 모듈은 처음부터 578행이 모두 통과했다. 첫 작성한 웹 계약 시험 한 가정(패턴 번호 63 이 표 밖)이 틀려서 실패했고 시험에서 뺐다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(cmp 일치).

- 실패한 변형: 청크 경계 16배→8배, 위치 nibble 교환, 방향 시프트 6→5, 플래그 검사에서 bit1 제외, 순번 시작 0→1, 월드 등록 territory 무시, 같은 범위 진단 제거(각 1~2건).

## 3. 경계와 남은 것

- 실제 월드에 연결하지 않았다. 호출 지점(`RawFortWorldLoad` 의 Territory 적재·일반 전투 재배치)은 순서4 몫이다(PORT_MAP session 0).
- `IslandBuilder_LiveWorldChunkRuntimeAndGrowthConsumer`(실제 월드에서 섬 성장 `RawTerrainGrowth` 소비)는 그 지형 성장 모듈(미이식)을 옮길 때 대조한다. CD 10.72(OddDirection·BoundsNotFound)는 웹 범위 밖이다.
- 웹 `ChunkMap` 의 방향 글자는 프레임 코드 숫자 그대로 저장한다(청크 항목의 문자열 `side` 와 섞이지 않게 직렬화 시 문자 코드로 변환).

## 4. 다음 시작점

WB04-E223 은 3.1절에서 고른다. `RawIslandPopulation`(Builder·Fort 소유자 후처리 뒤, `ChunkMap` 기록·`FortTerritoryIslandRecord` 가 필요), `RawIslandLifecycle`, `RawFortMetaLoad`, `RawFortBadgesGlobals`, `RawFortPaletteTransition` 등 기준선 뒤 변경이 없는 Fort/섬 단위를 권한다.
