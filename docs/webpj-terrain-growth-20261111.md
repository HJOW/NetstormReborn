# webpj 초기 지형 성장(RawTerrainGrowth) — WB04-E292 (2026-11-11, 4-B)

3.1절 4-B 의 **RawTerrainGrowth**(306줄)를 `core/raw-terrain-growth.js` 로 옮겼다. 원본 패치 `004c10a0`(일반 분기)과 군도 root 가 공유하는 helper(`004c0e20`·`004c13f0`·`004c0800`·`004c0b30`·`004c0db0`·`004c0fe0`·`004c0cf0`·`004c0c30`·`004c0930`·`004c0780`)이며 기준선(`1f00a25a`) 뒤 `RawTerrainGrowth.cpp/.h` 에는 변경이 없다(3.6절 ①). 선행: E290(RawTerrainContext)·E291(ChunkMap mode). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다. 후속 RawTerrainStamp 가 이 성장이 만든 `codes`/`regions` 배열을 읽어 지면 객체를 만든다.

## 1. 규칙

- 생성: `new RawTerrainGrowth(context, chunks, globals, { checking?, assertion? })`. 지도/청크/전역은 이 객체보다 오래 살고 사본을 만들지 않는다. `checking` 은 원문 `005e4794` 의 전체 DWORD 를 호출마다 읽는 함수(RawCheckingFlag 의 웹 대체), `assertion(표현식, 파일, 행)` 은 원문 진단 뒤 정상 복귀한다. 시드는 모두 `{ value }`(uint32) 상자이다.
- `runNormal()`: ① Y 청크 행 → X 청크 순서로 통로를 먼저 구성한다 — 청크 섬 BYTE(signed)가 noRegion 이거나(생성 요청 지역이 noRegion 이 아니면) 요청 지역과 다르면 건너뛰고, 섬별 시드 = 방문 수 × `0x10e3` + record[3], 중앙 4×4(`record[3]` 하위 3비트·`9999 - record[3]` 하위 3비트 오프셋), 연결 방향(`connectionMask(side)`: 북1·동2·남4·서8) 통로, 같은 섬의 두 번째 방문이면 중앙 10×10, 밀도 = `max(20, 난수30 + 50 - 채운 칸×100/256)`, 목표 = 밀도×256/100 → ② 모든 비 noRegion 청크마다 방문 수를 올리고 청크 칸 `GrowRectangle` → ③ `fixGaps(2)`.
- `growRectangle(island, bounds, target, seed)`: 폭 = (right-left)×16-1(원문 bottom 은 쓰지 않음, 비양수 폭은 `nextRange` 가 low 반환), 칸마다 X/Y 난수와 limit 1 호출을 모두 소비한 뒤 현재 문자/지역이 맞으면 Y 행 먼저 3×3 `Put`, 성공이 있으면 연속 실패 1000 으로 되돌린다. 군도 DWORD 가 비영이면 지역 비교와 청크 지역 조회를 건너뛴다.
- `nextRange`(폭 <1 이면 시드 불변), `scatter`(첫 시드×0x5d, 거부된 칸은 count 를 줄이지 않음, 큰 좌표는 float32→ftol 뒤 0 방향 /16), `firstChunkSeed`(signed 비교·진단 복귀 뒤 현재 right/bottom 재조회·실패 시 `0x2333`), `removeSparseCell`(직교2·대각1 가중, 합 ≤ threshold 이면 제거), `removeSparse`(임계 4, 즉시 제거·변화 없을 때까지 반복), `clearPlacement`(float32 칸 경계·ftol 하위 DWORD), `fixGaps`(13이웃 같은 지역 연속 5·홀수 방향).
- 진단: 패치만 `isValid()`(청크 mode 0, `Chunkmap.cpp` 0xb7)·범위(0xb8)·첫 시드 실패(checking 켜짐, `Terrainbuilder.cpp` 0x25f)를 보고하고, 장치가 없으면 숨기지 않고 `초기 지형 시드 진단 장치 누락` 으로 거부한다. 정상 입력은 장치를 요구하지 않는다. 지도 밖 접근·음수 `scatter` count·int32/시드 상자가 아닌 입력도 거부한다.
- CD(10.72)의 noRegion 진입 캐시와 무진단 경계는 옮기지 않았다.

## 2. 검사 (`tests/terrain-growth.test.mjs` 23건)

- 고정 관측 세 파일의 **originals 행** 전체(전체 65,536BYTE code/region 두 배열을 RLE 로 풀어 바이트 단위 대조 + caller 시드 + 진단 기록):
  `terraingrowth-x86.tsv` 54행(RunNormal) · `terrain-seed-x86.tsv` First 111행·Scatter 78행(진단 복귀 효과 포함) · `terrain-arch-helpers-x86.tsv` Grow 72행·Cell 771행·Sparse 12행·Clear 12행. **첫 실행에서 모두 통과**했다. CD 행은 웹 목표 밖.
- 계약 10건: 같은 context 배열 갱신과 stamp 필드(mapping·lastSid·makePredictable) 유지, 살아 있는 checking 의 호출마다 재조회와 선택 진단 장치 누락 거부(연결이 없어도 정상 조회는 성공), 일반 분기는 첫 시드를 조회하지 않음, 입력 종류 거부, 범위 난수의 비양수 폭/폭 1/감김 폭/시드 0/합 감김, 지도 저장 범위의 각 축 경계(0·255 허용, -1·256 거부), 청크 크기 경계(x == xLength, y == yLength)의 진단과 Y 외부 순서의 첫 칸,
  폭 -1 로 고정한 성장 중심에서 Put 의 양 끝 포함 bounds·signed 지역 비교, 연속 실패 한계의 성공 시 재설정(5×5 씨앗에서 289칸 = 17×17 포화), RemoveSparse 결과가 임계 이하 칸이 없는 고정점임(4가지 밀도의 무작위 무늬),
  빈틈 보정의 연속 5·홀수 방향·지역 변경 시 재시작·signed 지역 쓰기·bounds 안쪽 칸만 대상, 일반 성장의 섬 번호 0..99 범위 거부와 noRegion signed 비교.
- 변형 확인 130개 중 처음에 28개가 살아남았고, 계약 10건을 더한 뒤 **11개만 남았는데 모두 동등 변형**이다: ① 큰 좌표의 float32 반올림(`floatChunk`·`clearPlacement` 의 fround/`|0`)은 지도 밖(256 이상)이라 반올림 유무와 관계없이 같은 거부로 끝난다, ② `removeSparse` 가 한 칸 제거 뒤 같은 pass 의 나머지를 건너뛰어도(단락) 제거는 이웃 가중치를 줄이기만 하므로 변화 없을 때까지 반복한 고정점은 같다,
  ③ `fixCell` 의 13번째 방향(12, 짝수라 채울 수 없음)·초기 지역(첫 읽기 전에 항상 현재 값으로 덮임)·쓰기 지역(`region` 과 `current` 는 그 지점에서 같음), ④ `fixGaps` 의 네 직교 이웃 관문은 채움에 직교 x 이웃이 최소 둘 필요하므로(연속 5가 홀수 방향에서 끝남) 하나씩 빠져도 결과가 같다.

## 3. 남은 경계

- 군도 root(`ArchipelagoMode` 을 켜고 `scatter`/`fixGaps(1)`/`clearPlacement` 를 부르는 쪽)는 후속 단위(군도 지형)이다. 이 모듈은 그 helper 와 일반 분기를 모두 갖췄다.
- 3.1절 4-B 는 23/31 이다. 다음은 **RawTerrainBitmap**(95줄)이다.
