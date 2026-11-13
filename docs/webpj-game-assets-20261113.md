# webpj GameAssets 이월분 — Find·FrameIndex·SHP 블록 수 검사 (WB04-E385, 2026-11-13, 5-A 열네 번째 단위)

4-A(E272)에서 이월한 client **GameAssets**(cpppj `client/GameAssets.cpp/.h`, 기준선 122줄)의 남은 부분을 `assets/game-assets.js` 로 옮겼다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). cpppj 의 GameAssets 는 `.type` 116개와 `_shapes.shp` 를 읽어 연결하는 집계인데, 웹은 이미 `TypeTable.fromArchive`(정의 읽기·번호 70부터의 표)·`ShapeDatabase`(SHP 블록)·`buildTypeRecords`(타입 레코드)가 같은 일을 한다. 이 단위는 아직 없던 이름 조회 층만 더한다. cpppj 에는 이 층만 직접 보는 시험이 없고(호출자가 `Find`/`FrameIndex` 를 도구·월드 코드에서 부름), 웹은 실제 10.78 자료와 계약 시험으로 대조한다.

## 1. 규칙

- **블록 수 검사:** SHP 블록 수가 로딩 순서(116)와 다르면 `Shape block count does not match selected edition (use --cd for CD data)` 로 거부한다(CD 자료를 10.78 로 읽는 실수 방지). 타입 정의 수도 로딩 순서와 맞아야 한다.
- **`find(name)`:** 파일 이름(`assetName`)을 먼저, 없으면 `.type` 안의 typename 으로 찾는다. ASCII 대소문자만 무시(비 ASCII 대문자는 그대로)하며 같은 이름이면 앞선 로딩 순서가 이긴다. 없으면 `Unknown game asset type: 이름`. 읽지 못한 정의(null)는 typename 검색에서 건너뛴다.
- **`frameIndex(type, cluster, layer)`:** `layer × 클러스터 수 + 클러스터 순번`. 중복 클러스터는 첫 일치. SHP 블록 번호/레이어/프레임 수를 벗어나면 `RangeError`, 없는 클러스터는 `Unknown shape cluster`. (size_t 곱셈 넘침 검사는 JS 안전 정수 범위로 바꿨다.)
- **`types()`·`typeNumber(type)`:** 원본 로딩 순서의 `{ assetName, definition, block }` 목록과 `70 + block` 타입 번호(cpppj 호출자의 `kFirstAssetTypeNumber + type.block`).

## 2. 웹 범위

- Native 로더 훅(`readType`/`readShapes`)·팔레트 읽기(`assets/palette.js`)는 웹에서 필요 없거나 이미 있다. `GameAssets` 는 앱 수명에 연결하지 않았다(소비자 `GameWorld`·`GumpBackgroundDraw` 등이 각 단위에서 부른다).

## 3. 검사 (`tests/game-assets.test.mjs` 5건)

- 실제 10.78 자료: 116 블록 = 로딩 순서 116, 정의 null 없음, 별칭 3개(`dude`→`Man`·`edgeFarm`→`efarm`·`anim`→`Flare`)를 typename 으로도 찾고 typename `flare` 를 두 타입이 쓰지만 파일 이름 `flare` 가 먼저 선택된다. 모든 타입의 `find(파일 이름)`·`find(typename)` 이 앞선 로딩 순서의 일치와 같고, 모든 클러스터가 첫 일치 프레임 번호를 내며 마지막 유효 레이어/그다음 레이어의 경계가 SHP 블록 프레임 수와 맞아떨어진다(`bridge` 60 클러스터 × 1 레이어 = 60 프레임).
- 계약: 블록 수·정의 수 불일치 거부, 정의가 없는 타입이 문자열 `undefined` 와 일치하지 않음, 비 ASCII 대문자(`É`≠`é`)·ASCII 대소문자 무시, 중복 클러스터 첫 일치, 레이어 −1/소수/NaN/2^60 거부.
- 모두 첫 실행에서 통과했다(fixture 가 없는 집계 층이라 manifest 변경 없음).

## 4. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 19개와 손으로 쓴 구조 변형 13개를 적용했다. 1차 생존 4개 — 정의 수 검사의 중복 방어, `definition?.name` 의 `'undefined'` 일치, `toLowerCase` 로 비 ASCII 까지 소문자화, `MAX_SAFE_INTEGER` 경계 — 중 앞의 셋은 시험을 더해 잡았다.
- 남은 동등 변형 1개: `layer * stride + stride > Number.MAX_SAFE_INTEGER` 의 `>`→`>=` (정확히 같아지는 안전 정수 경계는 입력으로 만들 수 없음).
- 다음 5-A 항목: 렌더러 프레임 본체의 남은 부분(`Renderer` partial 494·`ClientRendererFrame` 396(기준선 뒤 +138/−17, 3.6절 판정 필요)·`ScreenPresentation`·`Screen`), 타입별 Draw·미리보기(`SpriteDraw`·`PreviewDraw`·`PreviewCollection`·`RawCursorPreviews`), 입자 본체(`RawCampaignParticles`·`ParticlePaletteMemory`).
