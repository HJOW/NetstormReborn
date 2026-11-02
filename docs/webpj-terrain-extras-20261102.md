# webpj 절벽 fringe·소유자 색 변환표·작은 받침 (WB03-Y, 2026-11-02)

2026-11-02, **HJOW-Athlon**(마지막 전체 정밀 디컴파일 호스트 HJOW-Athlon/2026-10-23 과 같은 PC 이지만 이번 단위는 디컴파일 소스를 읽지 않았다). 근거는 cpppj 의 복원 소스 `o/RawTerrainStamp.cpp`(절벽 `Create`)·`o/RawFrameChoose.cpp`·`o/RawTerrainContext.cpp`(제외 플래그)·`o/RawIslandPostPop.cpp`(작은 받침)·`o/RawFarmCluster.cpp`·`o/RawFortRandomPlacement.cpp`(edgeFarm 이 미뤄진 이유)·`client/OwnerColorMaps.cpp`·`client/OwnerColorTableState.cpp`(소유자 색표)와 그 근거 문서 [다리 저장 프레임과 지면 생성](exe/terrain-and-bridges.md)·[초기 지형 도장 감사](exe/cpp-initial-terrain-stamp-audit-20261013.md)·[noIsland 최초 등록과 받침 소유자 전파](exe/cpp-islandpostpop-reconstruction.md)·[edgeFarm 군집](exe/cpp-farm-cluster-20261014.md)·[공통 소유자 색표](exe/cpp-banner-option-runtime-20261028.md), 그리고 cpppj 가 **원본 실행 파일(10.78)을 제한 x86 으로 실행해 얻어 둔 fixture**(`cpppj/tests/fixtures/terrainstamp-x86.tsv`·`owner-hsl-colors-x86.tsv`·`owner-capture-x86.tsv`)이다. 모두 읽기만 했다. 사용자 요청("AGENTS.md, LEFT_JOBS.webpj.md 를 읽고 webpj 다음 작업 진행해 줘")으로 [인계](../LEFT_JOBS.webpj.md)의 '다음' 첫 항목 **정적 맵 화면(M2) 3단계(c)** 를 했다. 화면에는 아직 연결하지 않았다. `cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md`·`README.md`(이전 단위에서 받은 '공통 파일은 나중에 몰아서 갱신' 지시를 이어 지킴)·`.gitattributes`·공유 명칭 JSON·`docs/formats/` 는 수정하지 않았다. 원본 게임·analyzeManager·브라우저는 실행하지 않았다(화면이 바뀌는 변경이 아니다).

## 1. 범위와 결과

(c) 항목 4개 가운데 **3개를 끝냈고 `edgeFarm` 은 미뤘다**(6절).

| 대상 | 결과 |
|---|---|
| 절벽 `fringe` | 지면 칸의 `isle` 클러스터에 `fringe` 플래그(비트 1)가 있으면 그 칸의 **y + 4칸** 아래에 `fringe` 타입 오브젝트를 만든다. 후보는 지면 클러스터의 글자 쌍으로 `fringe` 타입 프레임 코드에서 찾고(정확히 → 정규화 → (방향, A) → 방향만) 전체에서 난수 한 번으로 고르며, 제외 플래그와 겹치면 **난수를 다시 소비해** 다시 고른다. `createFringeStamper` 를 `buildTerrainFrames` 의 `onStamp` 훅에 붙이면 지면 하나를 고른 직후 절벽을 고르는 원본의 난수 순서가 그대로 이어진다 |
| 소유자 색 변환표 | 소유자 색 번호 1~8 에 따라 그림의 색 번호 256개를 바꾸는 9 × 256바이트 표. 일반 띠 표(228~245), 명도 전용 표(지면 97 `isle` · 받침 윗면 94 `island` · 111 · 141 · 137), 등록 모음 `createOwnerColorTables`(배너 옵션 `useBanners`) |
| 작은 받침 | 저장 파일에는 3 × 3 의 `noIsland` 9칸만 있고 윗면(`island`)·아래 바위(`islandStalag`)는 읽을 때 만들어진다. `noIsland` 를 원본이 읽는 순서로 모아(`collectNoIslandObjects`) 칸마다 서쪽·북쪽 글자로 글자를 정하고(`F C G / B A D / I E H`) 글자 F 칸마다 받침을 찾는다(`findSupports`: 왼쪽 위 칸·기준 = 오른쪽 아래 칸·소유자·완전 여부) |
| 원본 맵 | `.fort` **465개 모두**: 지면 칸 1,909,275개 중 `fringe` 플래그 칸 **158,686개(AB 27,203 · AE 25,459 · EI 49,532 · HN 27,374 · IO 29,118)가 모두 절벽을 만들었고** 후보가 없는 칸은 없다. `noIsland` 칸 176,093개 → 받침 19,568개(불완전 10개) |

## 2. 원본 규칙(cpppj·문서 근거)

### 절벽 `fringe` (원본 Terrainbuilder `004bfe50`, `RawTerrainStamp::Create`)

* 지면 하나를 만든 직후, 그 지면 타입이 섬 genus(`isle`)이면 고른 클러스터의 코드(방향 글자·변형 글자·플래그)를 읽는다. 플래그 비트 1 이 없으면 아무것도 하지 않고 난수도 쓰지 않는다.
* 있으면 `ChooseTerrainFrame(fringe 타입, 코드, theme −1)` — 후보 전체에서 `Next(후보 수)` 한 번. 고른 후보의 플래그를 부호 있는 char 로 읽어 32비트로 확장한 값이 **제외 플래그**(`excludedFlags`)와 겹치면 후보를 버리고 난수를 다시 소비한다(겹치지 않을 때까지). 후보가 없으면(−1) 후보 배열 바로 앞 바이트를 플래그로 읽는데 cpppj 는 이를 명시 입력(`fringePrefixFlags`, 기본 0)으로 두며 겹치지 않아 그대로 끝나고 절벽은 만들지 않는다.
* 제외 플래그는 생성 문맥이 정한다(`004c0390`, `RawTerrainContext`): 전투 맥락 0x10(`unlit` 클러스터 제외), 맥락 소유자가 0 이면 0x18(`lit`·`unlit` 모두 제외). 클러스터 플래그 비트: fringe 1 · suck 2 · rim 4 · lit 8 · unlit 0x10 · cracked 0x20 · hard 0x40.
* 만들어진 절벽의 프레임은 고른 클러스터 번호, 위치는 지면과 같은 x 와 **y + 4.0**(칸), 소유자는 없다(0). 지역 도장(신전을 세워 소유자가 정해질 때 다시 칠함)은 절벽을 다시 만들지 않는다 — 절벽은 첫 도장이 정한 클러스터로만 정해진다.
* `fringe.type` 의 클러스터는 `EA01`~`EA39`(일부 `lit`/`unlit` 쌍)와 `IA`·`BK`·`BE`·`EK`·`HA`·`DJ`… 등이다. 지면의 `EI` 는 (방향, A) 단계로 `EA…` 전체(39개)가, `HN` 은 `HA…`(6개)가 후보이다.

### 소유자 색 변환표 (원본 `43b950` 항등 9표 · `43ba00` 일반 띠 · `43bbe0`/`43b750` 명도 전용 · `43bc40` 배너 옵션 · `43be30` 최초 등록)

* 후보표는 원본 실행 파일 `005319e8`~`00531b07` 의 DWORD 72개이다. 소유자 색 번호 c(1~8)의 명도 후보 8개가 `[c × 8, c × 8 + 8)` 이고 낮은 바이트만 색 번호이다. 변환표 0 행(중립)은 항상 항등이다. 소유자 색 번호의 의미: 1 파랑 · 2 빨강 · 3 흰색 · 4 초록 · 5 보라 · 6 노랑 · 7 연파랑 · 8 주황(기본값은 소유자 번호 그대로이고 미션 머리가 덮어쓴다).
* **일반 띠 표**: 228~245 의 18칸만 바꾼다. 228~237 은 밝기 0, 238~245 는 역순 7~0 의 여덟 명도이며 다리 타입은 소유자 2 의 238~245 를 밝기 4, 소유자 7 을 밝기 3 으로 고정한다.
* **명도 전용 표**: 지정한 소스 색 번호마다 팔레트의 HSL 명도 `(max + min) ÷ 2`(채널 × 1/255, double)에 소유자별 보정값을 더한 목표에 가장 가까운 명도의 후보를 고른다(첫 후보 거리는 99 로 자르고 같은 거리는 앞 후보). 지면 소스는 타입 94·97 이 같은 33개(44~64, 3, 177, 200, 203, 224, 220~223, 114, 201, 226)이고, 보정값은 **타입 97(`isle`) 만 소유자 1~8 에 −0.1 · −0.17 · 0.15 · 0 · 0 · 0.05 · −0.2 · 0**, 141 은 −0.15, 94·111·137 은 0 이다.
* **등록(`43be30`)과 배너 옵션(`43bc40`)**: 일반 띠 표는 타입 82·155·158·164·135·133·156·73·170·146·144·126 에, 명도 전용 표는 111·141·137 에 만든다. 옵션 `useBanners`(F7 "Hide Island Ownership")가 0 이면 지면 97·받침 윗면 94 의 명도 전용 표도 만들어지고(섬 테두리가 소유자 색으로 바뀜), 0 이 아니면 두 표가 없어 지면은 소유자 색으로 바뀌지 않는다. (옵션의 기본값은 그리기 단계의 옵션 해석에서 정한다 — `createOwnerColorTables` 의 기본은 0 이다.)
* **`edgeFarm`(타입 121)은 이 표가 없다.** 옛 dotnetpj 문서는 `isle` 과 `edgeFarm` 에 같은 변환표를 만든다고 적었지만 cpppj 의 원본 실행 검증을 거친 등록 목록(`OwnerColorTableTypes`)에는 121 이 없다. 그림 단계에서 원본 캡처로 다시 확인한다.

### 작은 받침 (원본 noIsland postPop `004423b0` · 서/북 글자 조회 `00442350` · 받침 소유자 전파 `0044bd20`)

* 받침 칸 `noIsland`(타입 157)의 클러스터는 모두 그림 없음(`null.gif`)이고 글자 A·F·C·G·B·D·I·E·H 마다 `P` 변형 번호 1 이 있다. `island`(94)는 발자국 3 × 3, 클러스터 `P01`~`P08`(소유자 색별 `playcolr.gif`)과 `P09`(받침 없음)이며 `islandStalag`(95)는 아래 바위이다. 둘 다 `dontSave` 라 저장되지 않는다.
* 요새를 읽으며 `noIsland` 칸 하나를 만들 때마다: 서쪽 칸 (x−1, y)·북쪽 칸 (x, y−1) 에 **이미 만든** `noIsland` 칸이 있으면 그 글자를 읽는다. 서쪽 글자가 C→G · F→C · A→D · B→A · E→H · I→E 이면 그 글자를, 아니면 북쪽 글자가 F→B · B→I 이면 그 글자를, 그 밖에는 F 를 자기 글자로 삼고 `P` 변형 번호 1 프레임으로 저장된 프레임을 덮는다. 행 우선 배치에서 `F C G / B A D / I E H` 가 된다.
* 글자 F 칸에서 받침 윗면을 만든다: 소유자는 그 칸의 소유자, 위치는 `(x + 폭 − 1, y + 높이 − 1)` = **받침의 오른쪽 아래 칸 (x + 2, y + 2)**. 아래 바위는 윗면의 postPop 이 같은 좌표에 만든다. 글자 H 칸(오른쪽 아래)이 만들어지면 3 × 3 칸·윗면·바위의 소유자를 왼쪽 위 칸(F)의 소유자로 맞춘다(저장 게임을 읽는 중이 아닐 때). 칸이 모자라면 원본은 진단 문자열만 남기고 계속한다.
* **읽는 순서**: Chaff 의 청크(월드 y 바깥·x 안쪽) → 영역 0~19 의 청크 레코드(영역 안 청크 순서) → 청크 안은 저장 순서. 이웃이 아직 만들어지지 않았으면 그 글자는 없는 것으로 본다.

## 3. 구현

| 파일 | 내용 |
|---|---|
| `www/assets/terrain-fringe.js`(신규) | `FRINGE_FLAG`·`FRINGE_Y_OFFSET`·`EXCLUDE_UNLIT`·`EXCLUDE_LIT_AND_UNLIT`·`fringeCandidates`(검색 단계·제외 전/후 후보)·`chooseFringeFrame`(난수 반복)·`createFringeStamper`(`cells`: Int32Array(65536) 지면 칸 → 절벽 클러스터 번호, `onStamp` 훅, `count`, `positionOf`) |
| `www/assets/owner-colors.js`(신규) | `OWNER_COLOR_CANDIDATES`·`OWNER_COLOR_TYPES`·`specialOwnerColorSources`·`buildStandardOwnerColorMaps`·`paletteLightness`·`buildSpecialOwnerColorMaps`·`createOwnerColorTables`·`applyOwnerColor` |
| `www/assets/island-supports.js`(신규) | `supportLetter`·`collectNoIslandObjects`·`findSupports`·`SUPPORT_ROWS`·`SUPPORT_SIZE`·타입 번호 상수 |
| `www/assets/terrain-tiles.js`(수정) | `buildTerrainFrames` 에 옵션 두 개: `onStamp(x, y, frame, random)`(첫 도장에서 칸의 프레임을 고른 직후 부르는 훅)와 `stampVariants`(이미 만들어 둔 첫 도장 난수표 99개 — 원본은 이 표를 전역에 한 번만 만든다). 난수표는 지면 칸이 하나도 없으면 CRT 를 소비하지 않도록 첫 칸에서 만든다(앞 단위는 항상 만들었다) |
| `tools/make-terrain-fringe-expected.py`(신규) | 독립 기대값: `fringe.type`·`isle.type` 텍스트를 줄 단위 정규식으로 직접 읽어 글자 쌍 256가지의 후보·제외 후 후보와 fringe 플래그 클러스터 56개를 적는다. `tests/fixtures/terrain-fringe-expected.tsv`(314줄) |
| `tools/make-island-supports-expected.py`(신규) | 독립 기대값: `tools/fort.py`(Python 판독기)와 영역 배치 독립 자료로 465개 `.fort` 를 읽어 원본 글자 규칙을 따로 구현해 칸 수·받침 수·불완전·겹침·받침 목록 해시·소유자별 수를 적는다. `tests/fixtures/island-supports-expected.tsv`(465줄, 약 6초) |
| `tests/terrain-fringe.test.mjs`(9개)·`tests/owner-colors.test.mjs`(7개)·`tests/island-supports.test.mjs`(9개) | 25개 검사(5절) |

사용 예: `const fringe = createFringeStamper(isleFrames, fringeFrames); const tiles = buildTerrainFrames(mask, isleFrames, { themeOf, onStamp: fringe.onStamp });` → `fringe.cells[y * 256 + x]` 가 그 지면 칸 아래(y + 4)의 절벽 클러스터 번호(없으면 −1). `findSupports(collectNoIslandObjects(layoutFort(parseFort(...), pieceCodes)))` → `supports[]`(왼쪽 위 `x, y`·기준 `anchorX, anchorY`·`owner`·`complete`). `createOwnerColorTables(palette.logical, { useBanners: 0 }).get(97)[소유자 색 번호]` 가 256바이트 변환표.

## 4. 원본과 다르게 둔 점·가정

1. **절벽의 제외 플래그는 호출자가 정한다**(기본 전투 맥락 0x10). 어느 맥락(전투 / 소유자 0)이 미션 읽기에 쓰이는지는 그림·세션 단계에서 확정한다. 실제 맵에 나오는 fringe 플래그 글자 쌍 다섯 가지는 두 값 모두 허용 후보가 비지 않는다(AB 1 · 1, AE 1 · 1, EI 24 · 9, HN 4 · 2, IO 4 · 2 — 0x10 · 0x18 제외 뒤 후보 수; 검사로 고정).
2. **난수로 고르는 절벽 변형은 원본과 같다고 보장하지 않는다.** 원본은 맵 적재 전의 모든 난수 소비를 거친 전역 난수 상태를 쓴다. 이 구현의 기본값은 난수 모드 0 의 시작 상태(`seedGameRandoms()`)이다. 같은 난수 상태를 주면 원본 실행 결과와 같다는 것은 fixture 재생으로 확인했다(5절).
3. 후보가 모두 제외 플래그와 겹치는 경우 원본은 끝없이 반복한다 — 이 구현은 `RangeError` 로 알린다(원본 465개에는 없음). 후보가 없을 때 원본이 읽는 앞 바이트 플래그는 `prefixFlags` 입력(기본 0)이다.
4. 3 × 3 그림 자리표시자(`J` 방향 글자)가 없는 프레임 표는 원본이 `−1 + 1` 로 계산을 이어 가지만 이 구현은 자료 오류로 거부한다(fixture 의 빈 표 줄 4개를 건너뛰는 이유; 거부는 검사한다).
5. **작은 받침은 윗면·바위 오브젝트를 만들지 않고 목록만 정한다.** 윗면의 프레임(소유자 색 번호 1~8 → 클러스터 0~7, 소유자 없음/비전투 → 8)과 소유자 색 번호의 해석(미션 머리의 덮어쓰기)은 그림 단계에서 한다. 같은 칸에 `noIsland` 가 겹친 사용자 제작 맵 5개(`MasterMind`·`rebelhq`·`sw_m1`·`Tester`·`WOF1`)는 불완전한 받침 10개가 나오며 원본도 진단만 남기고 계속하는 경우로 보고 같은 규칙으로 계산한 값을 그대로 적었다(공식 맵 32개는 모두 완전).
6. 소유자 색 변환표의 팔레트 입력은 호출자가 준 논리 팔레트이다. 원본은 최초 ColorSort 객체가 만든 명도를 쓰고 이후 화면 팔레트가 바뀌어도 자동으로 갱신하지 않는다([ColorSort 입력 수명](exe/cpp-palette-colorsort-owner-20261024.md)). 어느 시점의 팔레트를 줄지는 그림 단계의 계약이다. 53비트 기본 출력만 쓴다(자바스크립트 double 과 같고 64비트 관측은 기대값으로 섞지 않는다).
7. 10.78 패치판만 옮겼다(CD판은 타입 번호·절벽·받침 규칙이 다르다).

## 5. 검증(HJOW-Athlon, Node v24.21.0)

* `npm --prefix webpj test`: **250개 중 248개 통과**(신규 25). 실패 2개는 이 PC 의 `fonts/License.txt` 줄바꿈(CRLF) 때문에 자산 비교 2개가 실패하는 기존 현상이다(이전 단위 기록과 같음, 내용 차이 아님).
* **원본 실행 fixture 재생(가장 강한 근거)**: ① 첫 도장 `Stamp` 패치 180줄 중 **144줄**(장면 2 — 도장하지 않는 같은 종류 이웃 칸 — 는 마스크로 표현할 수 없어 36줄 제외)에서 칸마다 만든 지면·절벽 슬롯(타입 · 프레임 · 지역 · x/y 부동소수 비트 — 절벽 y 는 `y + 4.0`)과 **게임 난수 최종 상태 · CRT 난수 최종 상태 · 난수표 99개**가 원본 실행 결과와 같다(제외 플래그 0 · 2 · 0x80000000 의 부호 확장 포함). ② 프레임 선택 `Frame` 패치 600줄 중 596줄(안쪽 19 · 가장자리 577; 3 × 3 자리표시자 없는 표의 안쪽 4줄은 거부를 확인)에서 이웃 비트 → 방향 글자 → 프레임 · 두 난수 상태 · 난수표가 같다(월드 가장자리 칸 포함). 앞 단위 WB03-X 의 첫 도장 규칙을 원본 실행 결과로 다시 확인한 셈이다.
* **소유자 색 변환표**: 명도 전용 표 5종(111 · 141 · 137 · 94 · 97) × 팔레트 3벌 15줄과 일반 띠 표 18줄의 9 × 256바이트가 원본 실행 fixture(`owner-hsl-colors-x86.tsv` 53비트 출력, `owner-capture-x86.tsv` M 줄)와 **바이트 단위로 같고** 일반 표 줄의 후보 72개는 내장 후보표와 같다. 64비트 출력은 53비트와 다른 줄이 있어 기대값으로 섞지 않았다.
* **절벽 후보**: 글자 쌍 256가지의 검색 단계(정확히 · 정규화 · (방향, A) · 방향만 · 없음)·후보 번호·0x10 / 0x18 제외 뒤 후보가 독립 기대값(`fringe.type` 직접 읽기)과 같고, `isle.type` 의 fringe 플래그 클러스터 56개(주석 처리된 5개 제외)의 허용 후보 수가 같다. 원본 `.fort` 465개 전체에서 `buildTerrainFrames` + 절벽 생성기의 결과가 fringe 플래그 칸에만 절벽이 있고 독립 기대 후보 안에서만 고른다(플래그 칸 158,686개 전부 절벽이 있음).
* **작은 받침**: 465개 모두 칸 수 · 받침 수 · 불완전한 받침 수 · 겹친 오브젝트 수 · 받침 목록(`x,y,소유자,빠진 칸 수`) SHA-256 · 소유자별 받침 수가 독립 구현과 줄 단위로 같다(합계 칸 176,093 · 받침 19,568 · 불완전 10). 공식 맵 32개는 모두 완전하고 칸 수 = 9 × 받침 수이다(Save the Island! 26개 · 234칸, The War Begins! 13개 · 117칸 — 문서의 값과 같음).
* **변이 검사** 3개 모듈(+ `terrain-tiles.js` 훅) **120곳**(사본 `extracted/` 에서 한 곳씩 바꿔 신규 검사 파일을 돌림; 작업자 3개 병렬, 약 3분). 첫 실행은 114곳 검출이었다. 놓친 6곳 가운데 3곳은 검사를 보강해 검출했다 — 처음 두 일반 표 건너뛰기의 두 번째 경계(`skipFirstTwo` 가 앞 하나만 건너뛰어도 통과) · 같은 칸에 겹친 둘째 오브젝트가 글자를 덮어쓰는 경우 · 칸 하나만 빠진 받침이 완전으로 판정되는 경우. 나머지 3곳은 결과가 바뀌지 않는 동등 변이다 — 서쪽 칸 글자의 `null` 검사 제거(`Object.hasOwn(표, null)` 이 거짓) · 위쪽 가장자리 `y > 0` 대신 `y >= 0`(음수 칸 번호는 지도에 없음) · 월드 밖 `x + dx < 256` 검사 제거(월드 밖 칸 번호가 다음 줄 x = 0 칸을 가리켜도 그곳의 글자는 F · B · I 뿐이라 요구 글자 C · G · A · D · E · H 와 맞을 수 없음). 최종 117 / 120 검출.
* 확인하지 않은 것: 브라우저에서의 동작(화면에 연결 전), 원본 실행 중 지면 프레임 · 원본 화면과의 대조, 난수로 고르는 변형이 원본과 같은지(난수 상태가 같으면 같음), 제외 플래그가 미션 읽기에서 어느 값인지, 소유자 색 변환표의 `useBanners` 기본값, CD판(10.72), `edgeFarm`.

## 6. `edgeFarm` 이 아직인 이유와 필요한 입력

`edgeFarm`(흰 가장자리 장식, 타입 121)은 지면 생성 직후 영역마다 군집 함수(`0040e560`)가 `영역 청크 수 × 20` 개를 목표로 심는다. 군집 위치(별도 지역 난수: 시드 = 영역 첫 `Territory` 레코드의 바이트 + 3 × 77, 30000 번 연속 실패까지 반복, 중심 시도 뒤 3 × 3 주변 시도)는 순수 계산이지만 **각 시도의 성공 여부(`0040db90`)가 월드 상태에 달려 있다**: ① 칸의 `spots` 표(65,536바이트, 하위 6비트가 0 이 아닌 칸만 시도 — 오브젝트의 점유로 `SquidSpatial` 이 유지하는 현재 표) ② 같은 칸에 이미 `edgeFarm` 이 있는지 찾는 `finder`(오브젝트 해시) ③ 배치 가능 검사(`MayPlace` 파이프라인) ④ 현재 청크 영역과 요청 영역 일치 ⑤ `isle` 지면 오브젝트를 찾아 **지면을 `edgeFarm` 으로 바꾸는**(Retype, 프레임 유지) 대각선 제약(`MayReplace`) 검사. 이 입력은 WB04(공유 월드 코어: SID 풀 · 공간 · 오브젝트 해시)가 있어야 같은 상태에서 계산된다. 근사(좌표 기반 선택)로 대체하지 않고 WB04 뒤에 군집 순수 부분을 `place` 콜백과 함께 옮긴다. 이번 단위의 `buildTerrainFrames` 가 돌려주는 칸마다 프레임이 그 군집이 바꾸는 `isle` 프레임의 출처이다(바뀐 칸은 같은 번호의 `edgeFarm` 프레임).

## 7. 다음

M2 4단계 **그리기와 카메라**: 타입별 셰이프 프레임(타입 표 · 레코드, 번호 = 70 + 로딩 순서, 프레임 = 클러스터 번호 + 레이어 × 클러스터 수) · 소유자 색표(이 단위의 `createOwnerColorTables`) · 그림자 · 깊이 정렬([그리기 정렬](exe/cpp-draw-sort-reconstruction.md)) · 칸 16 × 11 픽셀 변환, 원본 캡처(`screenShots/`) 대조. 지면(`buildTerrainFrames`) · 절벽(`fringe.cells`, 위치 y + 4) · 받침(`supports`: 윗면 · 바위 프레임은 소유자 색 번호로) · 신전의 `theme` 로 만든 `themeOf` 를 한 장면에 모은다. `edgeFarm` 은 WB04 뒤.
