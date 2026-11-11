# webpj Fort 월드 적재 부모(RawFortWorldLoad) — WB04-E286 (2026-11-11, 4-B)

3.1절 4-B 의 **RawFortWorldLoad**(120줄)를 `core/raw-fort-world-load.js` 로 옮겼다. 원본 `004b5f70`(Load)·`004bd660`(ReadTerritory)이며 기준선(`1f00a25a`) 뒤 `RawFortWorldLoad.cpp/.h` 에는 변경이 없다(3.6절 ①). 같은 항목의 client **RawFortWorldLoadBinding**(40줄)은 `RawSurfaceWorld`·`RawNormalTerrainWorldBinding` 을 요구해 그 두 단위 뒤로 이월한다(3.1절에 별도 항목으로 분리). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다. 선행: E222(RawIslandBuilder)·E161(RawIslandRuntime)·E285(단일 배치/군집)·E218/E276/E279(FortObjectLoad)·E278(FortMoneyLoad)·E226(FortMetaLoad).

## 1. 규칙

- **`load(alternate)`(004b5f70):** builder(owner 1, 청크 경계 1..15, 실제 world 주소)를 만들고 현재 `localOwner` 의 Territory 120BYTE 를 읽어(현재 pos 에서 복사, pos += 120) `placeAll(0, 20)` 으로 영역 20개를 놓은 뒤 현재 `noRegion` 으로 정상 지형을 생성한다. `alternate == 0` 이면 저장 Fort 후처리를 objects → money → badges → technology 순으로 부르되 **매 단계 직전에 현재 `localOwner` 와 그 owner 의 저장 Template 를 다시 조회**한다(앞 child 가 owner 를 바꿔도 다음 child 는 새 owner 를 씀). `alternate != 0` 이면 현재 섬 0 이 존재할 때만 `populateIsland(state, 0)` 을 요청하고, 마지막은 항상 `rebuildGraph(false)` 이다.
- **`readTerritory(templateSlot, output120)`(004bd660):** Template 의 첫 DWORD 가 0 이면 진단 `MissingTerritoryTemplate` 뒤 계속 → Territory stream 의 현재 pos 에서 120BYTE 복사(물리 120BYTE 가 없으면 거부하고 pos 불변), pos += 120, signed pos > end 이면 진단 `TerritoryPastEnd`. 출력은 정확히 120BYTE 여야 한다.
- **`makeFortWorldChildrenHooks`:** 같은 풀의 `FortObjectLoad.load(fort, signed owner, 0, 20)` 와 `RawFortMetaLoad` 의 Badges/Technology 선택 읽기(`selectMeta` → `loadBadgesSelected`/`loadTechnologySelected` 의 `metaStream`)를 훅에 연결한다. Money·섹션 선택·TypeNames·UI 는 호출자 경계로 남긴다.
- 10.78 전용이라 CD 의 Territory 깊이 증감(`loadingDepthChanged`)과 깊이 상자는 옮기지 않았다. `puzzlePiece`/`localOwner`/`noRegion` 은 현재 DWORD 를 읽는 `{ value }` 상자이며 child 직전마다 다시 읽는다.

## 2. 검사 (`tests/fort-world-load.test.mjs` 5건)

- `fortworldload-x86.tsv` **originals 267행**(전체 534 중 patch): 새 저장소에서 시작해 현재 Builder · 실제 3204BYTE 섬 목록 · 16×16 청크(12BYTE: 섬·방향 글자·record 6·순번) · localOwner/noRegion/깊이 · Territory stream 의 pos/end 가 실행 뒤 모두 같고, 단계 순서(진단 `A:T`/`A:E` · 생성 `G` · 후처리 `O`/`M`/`B`/`T` · 섬 채움 `I` · 그래프 `R`)와 진단 횟수가 같다. 변이 9종(Territory 조회·Generate·각 child 가 owner/noRegion/섬 존재를 바꿈)이 매 단계 직전 owner 재조회를 구별한다. **첫 실행에 모두 통과했다.**
- 손 시험: 다른 풀·필수 child 누락·표 크기·물리 120BYTE 미만 Territory 거부(pos 불변)와 출력 크기, Canon 진단이 지역 Builder 에서 중단하면 생성/후처리 없음(cpppj `CanonConsumers_FortLoaderBeforeWorldGeneration`).
- 계약: Territory 조회 칸은 현재 localOwner 이고 20번째 영역 레코드(오프셋 114)까지 배치한다(fixture 의 레코드는 앞쪽 영역에만 있음), child 어댑터의 풀 불일치 거부와 선택 → 읽기 순서(Objects 는 signed owner, 영역 0..20).
- 변형 확인 32개 중 처음에 3개가 살아남았다(Territory 조회 owner 하드코딩, PlaceAll 끝 19, 출력 크기 검사). 위 계약 두 개로 모두 잡았다.

## 3. 남은 경계

- client `RawFortWorldLoadBinding`(`MakeFortWorldLoadBinding`: 정상 지형·저장 객체·SP·Meta·Graph 연결)은 `RawSurfaceWorld`(4-B 목록 끝)·`RawNormalTerrainWorldBinding`(미착수) 뒤에 한다. cpppj `FortWorldLoadBindingTests`·`NormalTerrainWorldBindingTests` 의 월드 연결 시험도 그때 소비한다.
- 실제 `ChunkMap`(partial)은 웹 `ChunkMap`(territory.js)의 `at/xLength/yLength` 로 바로 쓰였다. 이 단위의 청크 직렬화(12BYTE)가 Builder 가 쓰는 항목 필드(island·side·record·ordinal)와 맞는 것을 fixture 가 확인한다.
- 3.1절 4-B 는 17/31 이다(분리 항목 추가로 분모 +1). 다음은 **RawCampaignWorldLoad**(144줄)이다.
