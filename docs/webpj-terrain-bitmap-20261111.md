# webpj 지형 미리보기 bitmap(RawTerrainBitmap) — WB04-E293 (2026-11-11, 4-B)

3.1절 4-B 의 **RawTerrainBitmap**(95줄)을 `core/raw-terrain-bitmap.js` 로 옮겼다. 원본 `004c1860`(수직 fringe → 지면 texture 의 8비트 bitmap)이며 기준선(`1f00a25a`) 뒤 `RawTerrainBitmap.cpp/.h` 에는 변경이 없다(3.6절 ①). 선행: E290(RawTerrainContext)·E292(RawTerrainGrowth). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawTerrainBitmap(state, islandTypeBox, { green?, texture? })`, `render(codes, width, height, random)` → `{ width, height, pixels }`. 입력 `codes` 는 X 열 우선 65,536BYTE 지도(x × 256 + y), 출력은 행 우선 8비트 색 번호이고 빈 0 은 texture 의 투명 255 와 별개로 보존한다. `random` 은 호출자가 준 로컬 `GameRandom` 하나만 소비한다(공유 게임 난수를 건드리지 않음).
- `TerrainBitmapState`(`initialized` 005c8bb8·`classified` 255BYTE 005c8ab0·`fringeType` 005411f0 기본 102)는 Client 수명에 속한다. 최초 한 번만 분류 캐시를 0 으로 지우고 `'x'` 칸만 현재 녹색의 **하위 BYTE** 로 채운 뒤 초기화 표식을 1 로 쓴다. 그 뒤에는 팔레트/세션이 바뀌어도 다시 읽지 않는다. 녹색 조회가 없으면 캐시를 지운 뒤 거부한다(표식은 0 그대로).
- texture 는 `{ width, height, pixels(Uint8Array 40,000, 200 stride) }` 이고 폭/높이는 2..200 정수여야 한다(원문 나눗셈 보호). 반복 주기는 각각 폭-1/높이-1 이다. 1차 순회에서 fringe(현재 `fringeType`)를, 2차 순회에서 지면(현재 지면 TYPE 상자 값)을 이 순서로 조회한다.
- 1차: X 열마다 `y = height-1 .. 1` 에서 `'='` 칸을 보고 위 칸이 `'x'` 이면 로컬 난수 `next(3) + 3` 길이만큼 fringe 의 1 행부터 아래로 칠하고(출력 높이에서 끊음), 위 칸이 `'='` 도 아니면 120(해안 색), 위 칸이 `'='` 이면 그대로 둔다. y 0 은 제외된다.
- 2차: 행 먼저 훑으며 문자가 0x80 이상이면 거부하고(원문 signed 분류 범위), 분류가 켜진 칸만 지면 texture 로 덮는다.
- 크기는 0..256 정수, 문자 저장소는 정확히 65,536BYTE `Uint8Array`, texture 장치가 던지는 오류는 그대로 올린다(분류 접두와 난수 상태는 그 시점 그대로).

## 2. 검사 (`tests/terrain-bitmap.test.mjs` 7건)

- `island-bitmap-x86.tsv` **28행**(10.78; 두 x87 관측 포함): 실제 보호 texture(TYPE 102·97 SHP 0 번 프레임을 200 stride 40,000BYTE 에 `drawShape`)로 전체 픽셀·로컬 난수 최종 상태·분류 캐시·녹색 조회 횟수·texture 조회 순서가 같다. **첫 실행에서 통과**했다.
- `island-bitmap-parent-x86.tsv` **12행**: 청크 지도 → `RawTerrainContext`(E290) → `RawTerrainGrowth.runNormal`(E292) → `markEdges('x','=')` → bitmap(owner × 0x10e3 로컬 난수)을 원문 상위 `004c1cc0` 순서로 합성(합성 함수는 시험 안에만 있음: cpppj `EditorIslandBitmap` 은 Edit 범위)해 합성하지 않은 Native 전체 부모의 픽셀과 같다 — **E290~E292 세 모듈의 결합 검증을 겸한다**(첫 실행 통과).
- 계약: 작은 texture 의 반복 주기(폭-1/높이-1)·fringe 길이 난수 한계 3·샘플 행 주기·위 칸 분류(120)·최초 분류 유지·녹색 조회 누락/texture 누락/texture 실패 접두·비정상 texture(폭/높이 경계 1·2·200·201, 정수 아님, 일반 배열, 길이)·문자 0x7f/0x80·크기·입력 종류 거부.
- 변형 확인 67개 중 처음에 11개가 살아남았고, 계약 보강 뒤 **4개만 남았는데 모두 동등 변형**이다: 생성 장치 `null` 검사(없어도 `null.green` 접근이 같은 TypeError), 녹색 `& 0xff`(분류 캐시가 `Uint8Array` 라 저장 때 같은 하위 BYTE 만 남음), 길이 끊김 두 변형(`Uint8Array` 범위 밖 쓰기는 무시되어 결과 같음; C++ 에서는 버퍼 넘침 방지용).

## 3. 남은 경계

- Edit 범위의 `BuildEditorIslandBitmap` 상위 연결은 이식하지 않는다(시험 안의 합성만). 실제 Draw 부모의 투명 판정·팔레트 변환은 5-A 렌더러 단계 몫이다.
- 3.1절 4-B 는 24/31 이다. 다음은 **RawTerrainStamp**(partial 241줄)이다.
