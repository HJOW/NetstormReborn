# webpj 지역 도장(RawRegionPaint) — WB04-E296 (2026-11-11, 4-B)

3.1절 4-B 의 **RawRegionPaint**(partial 231줄)를 `core/raw-region-paint.js` 로 옮겼다. 원본 패치 `00470900`(Paint)·`0046fe40`(flood)·`004703b0`(경계)이며 기준선(`1f00a25a`) 뒤 `RawRegionPaint.cpp/.h` 에는 변경이 없다(3.6절 ①). 기존 `terrain-tiles.js` 의 `createPaintVariants`/`paintCoreFrame` 은 안쪽 칸 프레임 모델이었고, 이번 raw 몸체는 소유자 가상 호출·직접 프레임 쓰기·경계 후보·영향 지역 기록까지 원본 386행 전체를 재생한다. 선행: RawRegionOwnership(E249)·RawCanonPlacementTerrain 의 `regionAt`·E294 RawFrameChoose. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawRegionPaint(pool, frames, islands, terrainType, edgeFarmType, random, state, ownership, { setOwner, boundaryAt, regionAt })`: pool 은 10.78 `SidPool`, frames 는 188칸 `FrameCodeTable`, islands 는 hash 0 표면 머리 `Uint16Array(65,536)`(인덱스 = y × 256 + x), random 은 `GameRandom`, state 는 `RegionPaintState`, ownership 은 `RegionOwnershipState`(영향 지역 기록 `affected`). 훅은 모두 필수이다.
- `RegionPaintState`: `bounds`(생성 때 -10000 네 개, 각 Paint 가 [257, 257, -1, -1]로 다시 씀), 마지막 유효 요청의 `owner`(전체 DWORD)·`theme`(int32), `revision`/`sharedWorldRedraw`(월드 다시 그리기 DWORD; 빌린 상자가 있으면 그 상자, 진입 값을 캐시하지 않고 호출 시점의 현재 값에 하나를 더함), 중복 없는 `boundary` 후보(삽입 순서), 서로 독립인 99 캐시 `coreVariants`/`boundaryVariants`(첫 사용에 시드 0 의 지역 난수로 생성, 게임 난수 소비 없음).
- `paint({ x, y, owner, theme })`: 후보 목록을 비우고 bounds 를 다시 쓴 뒤, 시작 칸의 표면이 지면 타입이면 요청을 기록하고 flood — 북·동·남·서(서쪽 tail loop) 순서를 역순 push 로 재현, 빈 표면은 후보에 넣고, 지면이 아니거나 이미 같은 소유자면 건너뛰고, 아니면 `setOwner`(가상 +0x74) → 현재 프레임 코드의 연결 비트 → `Select` → 프레임 DWORD(+36) 직접 쓰기(SetFrame 호출 없음) → 연결된 이웃 push. 그 뒤 후보를 삽입 순서로(처리 중 늘어난 후보 포함) `Boundary`: 빈 표면이면 edgeFarm finder(`boundaryAt`) → 지역 getter → 지역이 영향 기록에 없으면 정수 좌표의 비트를 float 로 재해석한 값을 기록, 이미 같은 소유자의 지면이거나 지면/edgeFarm 이 아니면 종료, 아니면 소유자·프레임 지정과 이웃 후보 추가. 마지막에 월드 다시 그리기 DWORD +1(시작 칸이 무효여도).
- `Select`: 본섬 안쪽(방향 A/A)은 요청 x/y 의 99 캐시 좌표와 raw float 좌표의 0 방향 절삭 residue 로 `J 시작 + 1 + 원소 × 36 + (변형 × 3 + py % 3) × 3 + px % 3`(원소: 0/-1→0, 1→1, 2→2, 그 밖→3), 그 밖은 `chooseTerrainFrame(…, 요청 원소)`. 프레임 코드는 항상 **지면 타입의 코드 표**에서 읽는다(edgeFarm 도 마찬가지).
- 거부: 비유한/int32 범위 밖 raw 좌표, 표 밖 현재 프레임 번호(코드 표 길이 이상), 정수가 아닌 요청, 연결 오류. `makeRegionPaintOwnershipHooks(pool, paint, hooks)` 는 다른 풀이면 거부하고 `paint` 훅만 더한 사본을 돌려준다. CD(10.72)의 소유자 flood 전용 분기는 옮기지 않았다.

## 2. 검사 (`tests/region-paint.test.mjs` 13건)

- `regionpaint-x86.tsv` **originals 386행**(scene 6 × 변형 3 × 소유자/원소/시작 좌표): 소유자 가상 호출/edgeFarm finder 호출 순서와 인자·슬롯의 소유자와 프레임 DWORD·후보 삽입 순서·영향 지역 기록의 float 비트·drawer 네 float/요청·월드 DWORD·게임 난수 상태. 지역 getter 는 실제 `RawCanonPlacementTerrain.regionAt` 를 훅에 연결했다. **첫 실행에서 모두 통과**했다.
- 계약 12건: 공유 월드 DWORD(호출 시점 현재 값에 더함, 빈 시작 칸도 증가, 감김), 초기 상태와 0xff 를 넘는 소유자 DWORD 보존, 무효 시작 칸의 후보 목록 비움, 지도 끝(255 정상·256 밖) 표면 조회, 표 밖 프레임 번호(코드 표 길이·256·65,536) 거부, 절삭 대 반올림과 비유한/범위 밖 좌표 거부, 16비트를 넘는 프레임 번호(타입 모델 `paintCoreFrame` 이 독립 기대값), 경계 후보의 edgeFarm → 다른 소유자의 지면 순서와 별개 경계 캐시, 지면 타입 표에서만 프레임 코드를 읽음, 정수 좌표 비트(음수의 NaN 비트는 `xBits`/`yBits` 로 보관), 음수 캐시 좌표 거부, 구성/요청 종류 거부와 wrapper 풀 확인.
- 변형 확인 93개 중 처음에 17개가 살아남았고 계약 보강 뒤 **2개만 남았는데 모두 동등 변형**이다: ① `flood` 가 `Select` 에 요청 좌표를 (x, y) 대신 (y, x)로 넘기는 변형 — 캐시 합계 `(표[x ÷ 3] + 표[y ÷ 3]) % 99` 가 대칭이라 같은 결과, ② `Select` 의 TYPE 상한 — 지면/edgeFarm 만 닿으므로 항상 188 미만.

## 3. 남은 경계

- 영향 지역 기록의 `x`/`y` 는 정수 좌표 비트를 float 로 재해석한 JS 숫자라 NaN 비트열은 보존되지 않는다. 비트가 필요한 소비자는 `xBits`/`yBits` 를 읽는다(원본 투표는 좌표를 쓰지 않는다).
- 실제 세션에서 이 도장을 투표 wrapper 에 연결하는 일(`makeRegionPaintOwnershipHooks`)과 소유자 가상 호출·edgeFarm finder 의 실제 몸체 연결은 4-D 이다.
- 3.1절 4-B 는 27/31 이다. 다음은 **RawSurfaceWorld**(806줄)이다.
