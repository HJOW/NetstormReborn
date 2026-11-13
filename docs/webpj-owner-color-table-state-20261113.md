# webpj 소유자 색표 수명 OwnerColorTableState·ClientColorSortInput (WB04-E374, 2026-11-13, 5-A 세 번째 단위)

5-A 의 `ClientColorSortInput`(114줄; 최초 논리 RGB 캡처·실제 `RawColorSortPrimary` 소유·기본 53비트 9표 `Rebuild`·소유자 색표 저장소 소유)과 그 선행 **OwnerColorTableState**(partial 122줄)를 `core/client-color-sort-input.js`·`core/owner-color-table-state.js` 로 옮겼다. 웹 `assets/owner-colors.js` 는 소유자 색 표 **계산**(일반 띠·명도 전용)만 있었고(WB03-Y), 이 단위가 **언제 무엇을 만들고 반납하는지** 정하는 현재 저장소와 배너 옵션 수명(43bc40 `SetBannerOption`, 43be30 `InitializeOwnerColorTables`)을 더한다. 두 cpppj 파일 모두 기준선 뒤 변경이 없다(base `1f00a25a`). `ClientColorSortInput` 은 **partial**로 둔다 — 캐시 파일 경로 `InitializeCache`·소유자 8행 출력 `OwnerHighlight`/`OwnerSolid` 는 웹에서 `RawColorMapCache`(adapt) 단위가 맡는다(다음 단위).

## 1. 규칙

- **`setBannerOption(globals, requested, types, hooks)` (43bc40):** `globals = { ready, enabled, redraw, tables }`(현재 DWORD 와 포인터 표를 복사 없이 읽고 쓴다). 준비 전이거나 같은 값이면 포인터/장치를 읽지 않는다. 그 밖에는 설정 DWORD 를 쓰고 다시 그리기 DWORD 를 +1(unsigned 감김) 한 뒤, **0 이면** 지면(97)·받침 윗면(94) 명도 전용 표 둘을 소스 목록과 함께 만들고(`special` 두 번, 오프셋 0.0), **0 이 아니면** 현재 배너 TYPE 다섯(`types.banner`)을 차례로 다시 읽어 존재하는 표만 반납(`release`)하고 0 으로 쓴다(TYPE 이 표 길이 이상이면 거부).
- **`initializeOwnerColorTables(globals, skip, types, hooks)` (43be30):** 준비 1·설정 `0xffffffff` 강제 뒤 이전 설정으로 `setBannerOption` → `skip`(59ab28)이 0 이면 일반 표 앞 둘(82·155) → 전용 111(0.0)·141(−0.15000000596046448) → 일반 중간 넷 → 전용 137 → 일반 나머지 여섯 순서로 등록한다. 호출 순서와 오프셋 비트·소스 목록은 원본 trace 와 같다.
- **`OwnerColorTableState`:** 장기 저장소 — `initialize(primary, typeCount, bridgeType, redraw, skip)` 는 최초 한 번만(primary 가 완성돼야 하고 `typeCount ≥ 171`), `setEnabled(value, redraw)` 는 준비 전이면 무시, `enabled`·`ready`·`tokens`(주소가 바뀌지 않는 살아 있는 TYPE별 토큰 배열; 등록 전 조회는 거부)·`color(type, 색번호 0..8)`·`colorToken(0x100000 + 타입×2304 + 색번호×256)`. 일반 표는 `buildStandardOwnerColorMaps`(다리 TYPE 특례 포함), 전용 표는 **primary 가 저장한 명도**(`lightness(i)`)를 `assets/owner-colors.js` 의 새 `buildSpecialOwnerColorMapsFromLightness` 에 넣어 만든다(RGB 재계산 아님; 기존 `buildSpecialOwnerColorMaps(logical,…)` 는 같은 함수를 부르는 독립 adapter 로 남김).
- **`ClientColorSortInput` (439de0·43c010):** `capture(logical)`(독립 adapter: raw primary 없음)·`captureWithPrimary(logical, allocationBytes)`(정확 0x3800 초기 물리로 `RawColorSortPrimary` 를 만들고 정상 construct 뒤에만 발행, 실패하면 `captured` 거짓·이전 소비자가 가진 raw 는 유지)·`primary`/`logical`/`maps` 는 미연결/미캡처/미생성을 거부(빈 현재 팔레트 성공 금지)·`ownerTables`(항상 같은 객체)·`rebuild(currentRed)`: [0] RGB 1.6배 L1 최근접(첫 표) · [1] 파괴 · [2] 불법 · [3] 항등 · [4] 밝음 · [5] 글자 그림자 · [6] 건설 · [7] 그림자 · [8] HLS 명도 최근접(`(명도+0.2)/3`), 첫 표의 228..245·254·255 와 명도 표의 254·255 항등 덮어쓰기. 64비트 확장 정밀도 출력과 섞지 않는다. `colorSortLogicalInput(fixed, independentCurrent)`: 실제 Client 는 고정 256색을 필수로, 독립 검사만 현재 입력으로 되돌아간다(길이 오류는 거부).
- 하위 표 계산은 E373 의 `buildSpriteColorMaps`·`buildPreviewColorMaps` 와 같은 정규화·제곱 거리·float32 최소 저장을 재사용한다.

## 2. 웹 범위

- 10.72(CD) 분기는 없다. `Screen.BindColorSortInput`(화면이 고정 입력 getter 를 받는 연결)·`InitializeCache`(파일 hit/miss 와 `!color.dat`)·소유자 8행 출력은 후속(5-A 의 `RawColorMapCache`/`ClientColorMapFile` adapt, `Screen`). 이 모듈은 DOM 을 쓰지 않는다.

## 3. 검사 (`tests/banner-option.test.mjs` 5건 · `tests/client-color-sort-input.test.mjs` 6건)

- **원본 독립 관측(10.78):** `banner-option-x86.tsv` **2,066행**(write 2,050·init 16)의 끝 ready/enabled/redraw DWORD·188칸 포인터 표·생성/반납 효과(오프셋 double 비트·소스 목록 포함) 순서가 **첫 실행에서 모두 일치**했다(연속 요청 사이에 포인터/설정을 다시 만들지 않음). `colorsort-fixed-input-x86.tsv` **6행**(기본 53비트 3행 + 확장 64비트 3행)에서 같은 최초 논리 RGB 로 `rebuild(현재 빨강)` 한 아홉 표 256바이트가 모두 일치했고(64비트 행은 명도 표 8 만 원본에서 다름을 확인), 화면 입력을 바꿔도 초기 입력 객체/값이 유지된다. 이 관측은 E373 의 세 색표 소단위 계산도 아홉 표 안에서 다시 검증한다.
- **웹 계약:** 등록 전 설정 무시·초기화 뒤 토큰 배열 주소 유지·켜기/끄기 전환에서 97·94 표 생성/반납과 다시 그리기 증가·같은 값 무동작·unsigned 감김·`skip`·최소 타입 수 171 경계·살아 있는 토큰 변경 반영·색 번호 8/9 경계·토큰 기준값과 정렬 오류 문구·배너 TYPE 범위; `ClientColorSortInput` 의 명시 capture 규칙·미캡처/미생성 거부·호출자 배열 변경 비전파·독립 fallback·primary 주소/전체 BYTE 유지(정확 0x3800, +52..55 padding)·전용 표가 저장 명도를 직접 읽음(RGB 재계산과 구별)·누락/잘못된 초기 물리 거부, 그리고 **색 0 이 검정이 아닌 합성 팔레트**에서 첫 표·명도 표·항등 표를 독립 구현과 비교(원본 관측의 색 0 은 모두 검정이라 0번 처리 누락이 가려짐).

## 4. 변형 확인과 남은 경계

- 두 모듈의 연산자·상수 자동 변형 106+79 = 185개를 적용했다. 1차에서 34개가 남아 위 계약 시험(기본 TYPE 값·171 경계·토큰 배열 길이·살아 있는 토큰·색 번호 경계·`maps.length`·비검정 팔레트)으로 대부분 잡았다. 남은 동등/도달 불가 변형은 다음과 같다: 임시 `DataView` 크기 16→17·`#bridgeType` 초기값(등록 때 덮어씀)·`tokens`/`maps` 길이를 0 으로 지우는 줄(처음 비어 있고 등록은 한 번뿐)·내부 표 훅의 TYPE 범위/소스·오프셋 계약/반납 토큰 검사(공개 경로가 항상 올바른 값만 넘겨 닿지 못함; 비공개 훅이라 직접 호출 불가)·typed array 범위를 한 칸 넘기는 `<` → `<=` 반복(범위 밖 쓰기는 무시되고 NaN 거리는 비교가 거짓)·`Float64Array(COLORS * 3)` 크기·`selected` 초기값(첫 후보에서 항상 덮어씀)·`maps[5]` 에 글자 그림자(전부 0)를 쓰는 줄의 인덱스(초기 0 과 같음)·`allocationBytes === null` 분기의 검증 호출(값을 바꾸지 않음).
- 시험이 먼저 실패한 곳은 모두 시험 쪽이었다(정규화를 `/255` 로 쓴 독립 구현은 원본의 `× (1/255)` 와 마지막 비트가 달라 동률이 갈림, 배너 TYPE 97·94 가 표 길이를 넘는 입력).
- 다음 5-A 항목: **RawColorMapCache + ClientColorMapFile**(adapt) — 웹은 파일 없이 `rebuild` 로 계산 생성하고, `PrepareColorMapCache` 의 소유자 8행 `ownerHighlight`(명도 0.8 초과면 소유자 색)/`ownerSolid` 출력을 만든다. 그 뒤 팔레트(`PaletteColors`·`PaletteFlash`·`OwnerColorTableState` 의 `Screen` 연결·`ParticlePaletteMemory`)·VFX·렌더러 순이다.
