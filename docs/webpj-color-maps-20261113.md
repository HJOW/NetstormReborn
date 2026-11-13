# webpj 색 변환표 소단위 SpriteColorMaps·PreviewColorMaps·ProductionColorMaps (WB04-E373, 2026-11-13, 5-A 두 번째 단위)

5-A 의 색표 소단위 세 개 — **SpriteColorMaps**(40줄)·**PreviewColorMaps**(39줄)·**ProductionColorMaps**(29줄) — 를 `core/sprite-color-maps.js`·`core/preview-color-maps.js`·`core/production-color-maps.js` 로 옮기고, 웹에 이미 있던 `assets/palette.js` 의 `buildShadeMaps`(= cpppj `PaletteShade`, E-WB03-M1 에서 만든 GUI 명암 표)를 같은 원본 관측으로 대조해 PORT_MAP 의 `PaletteShade` partial 을 닫았다. 네 파일 모두 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). 이 표들은 원본 전체 ColorSort `0043c010` 이 만드는 9표의 일부이며, 나머지(`OwnerColorMaps` 소유자 표, 첫 표 0과 HLS 명도 표 8, 소유자 8행 `ownerHighlight`/`ownerSolid` 파일 캐시)는 다음 단위 `ClientColorSortInput`(최초 논리 RGB 캡처·9표 `Rebuild`)과 `RawColorMapCache`(adapt)가 맡는다.

## 1. 규칙 (모두 논리 팔레트 256색 R/G/B/예약 DWORD 입력, 정규화 double 1/255, 거리 비교는 엄격 `<` 후 최소 거리만 float32 저장)

- **`buildSpriteColorMaps(logical)` → `{ bright, dead, shadow }`:** `bright`(556238)·`dead`(556438)는 `buildShadeMaps` 의 밝음(1.5배·최대 1, 후보 229~245 제외)·어두움(float 0.6 승격 double 0.6000000238418579 배) 표를 그대로 쓴다(L1 거리). `shadow`(557738)는 소스 RGB 에 같은 어두움 계수를 곱한 목표와 모든 후보의 **blue→green→red 제곱 거리** 합을 엄격 비교해 고르고, 끝에서 `228..245` 를 항등으로 덮은 뒤 `254`·`255` 를 명시 대입한다(원본 `0043c010` 꼬리).
- **`buildPreviewColorMaps(logical, red)` → `{ destroy, illegal, labelShadow }`:** `destroy` 는 모든 소스 번호를 현재 기본 빨강 번호의 하위 BYTE 로, `labelShadow` 는 모두 0 으로 채운다. `illegal` 은 소스 빨강만 2.1배(최대 1)한 목표와 후보의 blue→green→red 제곱 거리로 고른다.
- **`buildProductionColorMaps(logical)` → `{ disabled, underlay }`:** `underlay`(556338, 선행/밑그림)는 원본 memset 의 0, `disabled`(556038)는 불법 표와 같은 계산(빨강 2.1배)이다. 두 모듈의 같은 계산은 시험에서 `illegal === disabled` 관계로 함께 확인한다.
- 입력이 256색이 아니면 거부(`RangeError`)하고 결과 객체는 동결한다(웹 적응; C++ 은 `std::span<…,256>` 컴파일 시점 검사). 엄격 비교의 동률은 앞 번호를 유지하지만 float32 로 저장한 최소가 실제 거리보다 크게 반올림되면(`d < fround(d)`) 다음 후보가 다시 최소가 되어 뒤 번호까지 간다(원본 `PaletteShade` 주석의 "동일 RGB도 저장 반올림에 따라 뒤 번호").

## 2. 웹 범위

- 10.72(CD) 분기는 없다. 세 모듈은 계산만 하며 `ColorSort` 9표 저장소·파일 캐시(`!color.dat`)·소비자(`SpriteDraw`·`PreviewDraw`·생산 아이콘)는 5-A 후속 항목이다. DOM 을 쓰지 않는다.

## 3. 검사 (`tests/color-maps.test.mjs` 6건)

- **원본 독립 관측(10.78):** `sprite-colors-x86.tsv`·`preview-colors-x86.tsv`·`production-colors-x86.tsv` 각 3행(실제 GIFCLOUD 파일 팔레트·혼합 RGB·중복 회색 입력의 논리 팔레트 256색, 미리보기는 현재 기본 빨강 포함)의 모든 표 256바이트가 **첫 실행에서 일치**했다(밝음·건설·그림자 / 파괴·불법·글자 그림자 / 비활성·선행). 밝음·건설 표는 웹의 기존 `buildShadeMaps` 가 이 관측을 처음으로 직접 대조받았다(기존 시험은 기본 팔레트를 원본 캐시와만 비교).
- **웹 계약:** 합성 팔레트(모든 색이 서로 다른 RGB)로 입력 불변·결과 동결·모든 표 256바이트·그림자 표의 항등 꼬리·파괴 표의 빨강 하위 BYTE(0·255·256·0x1bd·−1)·글자 그림자/선행 표 0·불법 = 비활성, 256색이 아닌 입력 거부, 검정 단색에서의 정확한 0 동률(첫 후보 유지)과 단색 0x336699 에서의 float 반올림에 따른 마지막 번호 선택, 예약 BYTE(최상위)가 거리에 쓰이지 않음.

## 4. 변형 확인과 남은 경계

- 세 모듈의 연산자·상수 자동 변형 45+39+38 = 122개 중 **13개가 남았고 모두 동등**하다: 반복문 범위를 한 칸 넘기는 `<` → `<=` 아홉 건(typed array 범위 밖 쓰기는 무시되고 범위 밖 읽기는 NaN 거리라 비교가 거짓)·`Float64Array(COLORS * 3)` 크기 3→4 세 건(더 큰 배열)·그림자 표의 정규화 계수 `1/255` 변형(그림자 표는 목표와 후보를 같은 비율로 키우므로 거리 순서가 같고 계수가 2의 거듭제곱 배면 float 반올림도 같음 — 불법·비활성 표는 `min(1, …)` 때문에 이 변형이 잡힌다). 초기 최소 거리 `Math.fround(1e6)` 도 정규 RGB 의 최대 거리(3 미만)보다 크기만 하면 같아 변형할 수 없다.
- 시험이 먼저 실패한 곳은 시험 쪽이었다(단색 팔레트에서 "동률은 앞 번호" 라고 적은 기대 — 원본의 float 저장 반올림 때문에 d 가 float 으로 정확히 표현되지 않는 단색은 마지막 번호까지 간다). 모듈은 관측 대조에서 바뀌지 않았다.
- 다음 5-A 항목: **ClientColorSortInput**(116줄; 최초 논리 RGB 캡처·`Rebuild` 로 9표(첫 표 0·HLS 명도 표 8·항등 3 포함)·소유자 표 `OwnerColorTableState` 소유) — 이 단위의 세 표 계산을 합성하고 `RawColorSortPrimary`(E205)에 연결한다. 그 뒤 `RawColorMapCache`/`ClientColorMapFile`(adapt: 웹은 파일 없이 계산 생성).
