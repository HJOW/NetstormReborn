# webpj 초기 terrain frame 선택과 stamp(RawTerrainStamp) — WB04-E295 (2026-11-11, 4-B)

3.1절 4-B 의 **RawTerrainStamp**(partial 241줄)를 `core/raw-terrain-stamp.js` 로 옮겼다. 원본 패치 `004c04b0`(프레임 선택)·`004c0640`(생성)·`004bfe50`(stamp 순회)이며 기준선(`1f00a25a`) 뒤 `RawTerrainStamp.cpp/.h` 에는 변경이 없다(3.6절 ①). 기존 `terrain-fringe.js`/`terrain-tiles.js` 는 마스크로 표현되는 144줄만 다시 재생하던 typed 모델이었고, 이번 raw 몸체는 원본 780행 전체(같은 raw SID 풀·두 난수·99 캐시·외부 효과)를 재생한다. 선행: E290(RawTerrainContext map)·E294(RawFrameChoose NB23). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawTerrainStamp(pool, types, frames, map, random, crtBox, state, hooks)`: pool 은 10.78 `SidPool`, types/frames 는 188칸(`flags2`/`FrameCodeTable`), map 은 `RawTerrainContext.map`, random 은 `GameRandom`, crt 는 독립 CRT holdrand `{ value }`, state 는 전역 `TerrainStampState`(99 캐시 `variants`, 기본 본섬 97/경계 98/장식 102/noRegion 127/접두 flags). hooks 는 여섯 효과(`create(type, predictable)`→sid · `setOwner` · `pop(sid, x, y, flags)` · `setRegion` · `chunkRegion` · `assertion`)이며 하나라도 없으면 거부한다.
- `frameAt(x, y)`: 이웃 8개를 북부터 시계방향으로 보아 alias(signed char 로 읽음)와 지역 BYTE 가 같고 bounds(양 끝 포함) 안이면 직선(짝수, 비트 1·2·4·8)/대각선(홀수) mask 에 더한다. 캐시는 첫 요청에서만 CRT 99번(15비트, 앞 원소와 mod 4 가 같으면 +1)으로 만들고 이후 Client 수명 동안 재사용한다. `'x'` 이고 방향 글자가 모두 `A` 인 안쪽 칸은 `J` 구간 뒤 3×3 묶음(`변형 = (표[(표[x/3]+표[y/3]) % 99] + y/3) % 4`), 그 밖은 `chooseTerrainFrame(…, theme 0)` 이며 후보 없음(-1)은 0 이다.
- `stamp(code)`: Y 외부/X 내부 반열린 순회에서 alias 가 요청 문자와 같은 칸마다 `x`·`=` 는 본섬 타입, `X` 는 경계 타입으로 mapping 세 항목을 생성 직전에 다시 지정하고(타입 0 이면 건너뜀) `frameAt` → `create`(마지막 SID 갱신) → `setOwner(sid, 0)` → 프레임 DWORD 직접 쓰기(+36, `SetFrame` 부수 효과 없음) → 실제 TYPE 확인 → `Pop(x, y, 0x02000061)` → 지면 genus(flags2 & 2)면 지역(noRegion 이면 `MissingRegion` 진단 뒤 `chunkRegion(x/16, y/16)`)을 `setRegion` 하고, 프레임 코드의 flags 비트 0 이 켜져 있으면 fringe 를 만든다(`chooseTerrainFrame(…, -1)`; 후보 flags 가 제외 flags 와 겹치면 다시 난수 소비; makePredictable 이 0 이면 `UnpredictableFringe` 진단; Pop 은 `y + 4.0f`).
- 거부: 지도 밖 좌표(0..255 바깥, 정수 아님), signed 음수 문자(0x80 이상), mapping 타입 표 밖(-1, 188 이상), 표 밖 생성 타입·프레임·fringe 타입, 정수가 아닌 stamp code. CD(10.72)의 BYTE frame/전체 후보 선택은 옮기지 않았다.

## 2. 검사 (`tests/terrain-stamp.test.mjs` 11건)

- `terrainstamp-x86.tsv` **originals 780행**(Frame 600·Stamp 180) 전체: 프레임 번호·게임 난수·CRT 상태·99 캐시 전체·외부 효과(인자/순서)·생성한 모든 raw 슬롯(type/frame/owner/region/좌표)·마지막 SID·mapping·진단 수. **첫 실행에서 모두 통과**했다. 기대 값은 원본 PE 관측에서 읽고 입력 지도/코드 표/이웃 mask 만 만든다.
- 계약 9건: 캐시 재사용(같은 월드의 두 번째 호출과 새 실행기/새 장면에서 CRT 를 다시 소비하지 않음), 필수 효과/크기/종류 거부, 안쪽 칸 프레임이 타입 모델 `stampCoreFrame` 과 같음(9개 좌표; 관측 행이 한 좌표(50,53)만 써서 x/y 역할을 구별하지 못했다), `'x'` 만 안쪽 묶음이고 `'='` 는 후보 선택, DWORD alias 의 signed 해석(0xe9→-23), 지도 가장자리/0x80/mapping 타입 경계, bounds 안쪽만 생성(위·아래·오른쪽·왼쪽 밖 제외, left 가 bottom 보다 커도 시작 행은 top)과 지역 BYTE 의 signed 전달, 지역 없음의 진단→청크 조회→설정 순서와 좌표 `(x/16, y/16)`, makePredictable 의 factory 전달/0 진단, 표 밖 생성 타입·빈 프레임 표·표 밖 fringe 타입 거부, 16비트를 넘는 프레임 번호(65,600칸 표)가 DWORD 전체로 쓰임.
- 변형 확인 105개 중 처음에 25개가 살아남았고 계약 보강 뒤 **4개만 남았는데 모두 동등 변형**이다: ① 캐시 첫 원소(`variants[-1]` 는 `undefined % 4 = NaN` 이라 비교가 항상 거짓), ② 안쪽 합계의 음수 검사(값이 모두 음수가 아니라 닿지 않는 방어), ③ 프레임 범위 하한(`frame >= 0` 이 항상 참인 호출만 옴), ④ fringe 접두 flags(검색 실패 -1 에서 제외 flags 와 겹치면 원문도 무한 반복이라 관찰할 수 없음).

## 3. 남은 경계

- 실제 월드의 factory/Pop/지역 setter 를 hooks 로 연결하는 일(RawNormalTerrainWorldBinding)은 client 단위로 4-B 의 RawSurfaceWorld/바인딩 항목과 4-D 에서 한다. 이 시험은 hooks 의 효과를 원본 관측의 인자/순서로만 대조한다.
- 3.1절 4-B 는 26/31 이다. 다음은 **RawRegionPaint**(partial 231줄)이다.
