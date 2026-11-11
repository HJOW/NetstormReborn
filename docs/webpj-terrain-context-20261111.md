# webpj 초기 지형 context(RawTerrainContext) — WB04-E290 (2026-11-11, 4-B)

3.1절 4-B 의 **RawTerrainContext**(partial 171줄)를 `core/raw-terrain-context.js` 로 옮겼다. 원본 패치 `004c0390`(constructor)·`004c09a0`(MarkEdges)·`004c0ad0`(NormalizeAfterStamp)이며 기준선(`1f00a25a`) 뒤 `RawTerrainContext.cpp/.h` 에는 변경이 없다(3.6절 ①). 이 단위가 3.6절 ③ 의 `Edition()`/`ArchipelagoMode()` 접근자를 반영한다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다. 후속(RawTerrainGrowth·RawTerrainStamp·RawRegionPaint)이 이 객체의 `map`·배열·군도 상자를 그대로 쓴다.

## 1. 규칙

- 지도: 원본 X 열 우선 두 65,536BYTE 배열 `codes`(인덱스 = x·256+y)와 `regions`, 문자별 256 DWORD `typeMapping`·`aliases`. 렌더용 Y 행 자료는 호출자가 명시적으로 전치한다.
- 생성: 모든 칸을 `.` 와 `noRegion` 하위 BYTE 로 채운다. `mapping['.']=0`, `['x']=islandType`, `['X']=edgeType`, `['o']=0`, `['=']=islandType`. `aliases['x','X','o','=']='x'`. `map = { codes, regions, typeMapping, aliases, bounds, makePredictable: 1, lastSid: { value: 0 }, excludedFlags }` 이고 `excludedFlags` 는 전투면 0x10, 아니면 0, **owner 가 0 이면** 어떤 경우든 0x18 이다.
- `setBounds(bounds)`: 청크 값을 보관하고 칸 bounds(`map.bounds`)를 `<< 4` 로 먼저 int32 감김 저장한다(같은 배열 객체를 제자리에서 갱신). 패치는 **입력 right > 16** 이면 RightChunkLimit, 보관한 bottom > 16 이면 BottomChunkLimit 를 순서대로 보고한 뒤 계속하고, 마지막에 보관 청크의 right/bottom 만 16 으로 줄인다(16 이하는 보고 없음, 칸 bounds 는 제한하지 않음).
- `markEdges(code, edge)`: Y 외부/X 내부로 `[bounds0,bounds2) × [bounds1,bounds3)` 반열린 순회, `code` 와 signed 문자가 같은 칸마다 북부터 시계방향 여덟 이웃을 본다. 이웃이 `bounds` 밖(양 끝 칸 **포함** 판정: `nx > bounds[2]` 만 밖)이거나 이웃 문자의 alias 가 현재 칸 문자와 다르거나 지역 BYTE 가 다르면 현재 칸을 `edge` 로 바꾸고 그 칸의 나머지 이웃은 읽지 않는다(앞에서 바뀐 문자는 alias 로 후행 판정에 반영됨).
- `normalizeAfterStamp()`: 같은 반열린 범위에서 `=`/`o`/`X` 를 `x` 로 바꾼다. 지역 BYTE 와 다른 문자는 보존한다.
- 거부(자료 손상으로 진단): 지도 밖(0..255 바깥) 좌표, signed 음수 문자 주소(≥0x80), 필수 진단 훅 누락, 다른 판본(CD 의 "보고 없이 줄이는" 경계는 옮기지 않음), uint32/int32 가 아닌 owner/region/bounds/전역/`markEdges` 입력.
- `chunks()` 는 보관 청크의 사본, `owner`/`region`/`edition` 은 생성 때 값, `sharedArchipelagoMode()` 는 군도 root 와 성장이 같이 읽는 uint32 `{ value }` 상자(기본 0)이다.

## 2. 검사 (`tests/terrain-context.test.mjs` 9건)

- `terraincontext-x86.tsv` **originals 112행**(Ctor 72·Edge 20·Normalize 20): 모든 고유 입력(owner·전투·noRegion·청크 네 값·region·타입 두 개)을 같은 입력으로 재생해 makePredictable·lastSid·excludedFlags·region·청크·칸 bounds·타입 대응표 256·alias 256·배열 균일성·X 열 우선 16×16 창(code/region)·assertion 횟수가 같다. CD 112행은 웹 목표 밖.
- 계약: 배열 공유·거부(훅/판본/입력)·bounds 제자리 갱신과 int32 감김·assertion 순서와 16 경계·청크만 제한·반열린 순회와 양 끝 포함 이웃 판정·첫 불일치 break·지도 끝의 바깥 이웃 거부(x 와 y 각각)·signed 문자(0x80) 선택과 대응표 주소 거부·정규화 범위/지역 보존·owner(0xffffffff)/region(음수) 보존·군도 상자 uint32 감김.
- 변형 확인 68개로 시작해 14개가 살아남았고 계약 4건 추가 뒤 **모두 잡았다**. 단 `cellBounds[3]` 를 `this.#chunks.bottom << 4` 로 바꾼 변형은 **동등 변형**이다(그 시점에 보관 bottom 은 입력 bottom 과 같아 청크 제한은 그 뒤에 적용됨).
- 첫 실행의 실패 두 건은 모두 시험 기대 오류였다: `0x08000001<<4` 는 int32 로 `-2147483632`, 이웃 시험은 북쪽 이웃이 bounds 밖이라 첫 칸에서 edge 로 끝나지 않도록 안쪽 칸을 써야 한다.

## 3. 남은 경계

- 모듈은 `assets/terrain-mask.js` 의 `NEIGHBOR_CELLS` 만 가져온다(같은 여덟 방향 표). 기존 타입 모델(terrain-mask·terrain-fringe)은 개발용 미리보기에 계속 쓰이며 이 raw 몸체와 별도이다.
- 군도 root(`ArchipelagoMode`을 켜는 쪽)와 성장 소비자는 후속 단위이다: 다음은 **RawTerrainGrowth**(306줄)이다. 3.1절 4-B 는 21/31 이다.
