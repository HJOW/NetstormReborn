# webpj 냉각 연기(RawSmokeEffect) 이식 — WB04-E191

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백아흔한 번째 부분이다. 원본 `Smoke.cpp` 의 연기 입자 효과(TYPE 21, 냉각되는 건물/포구에서 피어오르는 연기)를 맡는 `o/RawSmokeEffect` 를 옮겼다. 하나의 cpppj 파일에 생성/소멸(`RawSmokeLifetime`: 패치 `004a8140` 생성자·`004a86e0` 냉각 wrapper·공통 소멸)과 프레임/출력(`RawSmokeFrame`: `004a8240` 방출·`004a82b0` 프레임·`004a85a0` 출력), 공유 색 번호(`00487cd0(77,88)`)가 함께 있어 웹도 한 모듈로 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스·시험·fixture 모두 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋 `e6d638a0`(소스·시험·fixture 같은 커밋).
- 공유 장치: E187 `createFieryTailParticle`(같은 24바이트 입자)과 같은 `{ count, readable }` 색 표 형식, E129 `RawRenderRegistry`·`createRenderProcessFields`, WB04-C1 `GameRandom`, WB04-A `SidPool`.

## 1. 옮긴 것

`www/core/raw-smoke-effect.js`(DOM 없음)

- `buildSmokePalette()` — 양끝 포함 77..88 의 팔레트 번호 12개(RGB 근사 색을 만들지 않는다).
- `createSmokeFields()`·`createSmokeRequest()` — 128바이트 객체의 의미 필드(`capacity`/`total`/`hasStorage`·시계·`due`/`windX`/`windY`·`hotspotX`/`hotspotY`/`remaining`·`maximumLife` 100/`velocity` −1/`spread` 9/`period` 5/`noise` 0.5)와 생성 요청 기본값이다. `remaining` 이 −1 이면 기한마다 무한 방출한다.
- `RawSmokeLifetime` — `initialize(fields, token, request)` 는 공통 등록(TYPE 21)/부착 → 입력 값(원문 저장 순서) → 깊이 −1(32비트 감김) → 4800바이트 확보·200입자 → 방출 기한 0·바람·방출 수 8 → 공유 색이 없을 때만 8바이트 확보·생성·12색 순서다. 생성자가 쓰지 않는 delta/scaledDelta 는 보존한다. `initializeCooling(fields, token, parent, [x, y])` 는 기본 생성을 끝낸 뒤 방출 수 10·간격 3 을 덮어쓴다. `destroy` 는 입자(저장소가 있을 때) → 등록/base → flags bit 0 객체 순서이며 숫자 카운터와 공유 색은 남긴다.
- `RawSmokeFrame` — `emit(fields, x, y, life)` 는 꼬리 먼저, 가득 차면 사용 범위의 수명 ≤ 0 슬롯(NaN 재사용 안 함)을 얻은 경우에만 속도 난수 두 개(x → y)를 쓴다. `step(fields)` 는 최초 호출에서 시각/영역만 맞추고 끝나며, 이후 전체 delta 한 번(clamp/시간 조각 없음) → 방출(남은 수 > 0 이거나 무한 방출 기한, 위치 난수 y → x, 다음 기한 `0.75×간격 + 난수(0, 0.25×간격) + 경과`) → 사용 꼬리(분열로 같은 프레임에 늘어난 새 꼬리 포함)의 수명 감소·분열(수명 > 2 일 때만 난수 `(0,3) < delta`, 분열 뒤 수명 ×0.8)·바람 이동·정수 좌표 ±1 영역 → 활성 입자가 없으면 `kill(0)`, 아니면 이전/새 dirty 다. `draw(fields)` 는 좌표를 먼저 정수화한 뒤 카메라를 빼고, 색 표 DWORD(색수 × min(수명, 최대 수명) × float(1/최대 수명), 인접 DWORD 는 공급된 것만)로 가로 2픽셀 선을 낸다.
- 정수화는 원본 `__ftol` 처럼 signed64 절삭 뒤 하위 DWORD 이고 비유한/범위 밖은 0 이다.

## 2. 검증

`tests/smoke-effect.test.mjs`(6건). cpppj `SmokeEffectTests.cpp` 의 `SmokeScene`/`ReplaySmoke` 를 1:1 로 옮겼다(같은 장면 풀을 행 사이에 재사용).

- `smoke-effect-x86.tsv`(10.78, 893입력, SHA-256 `902cc800…`): 프레임/출력 841·방출 실패/재사용 28·냉각 생성/소멸 8·현재 SHP 냉각 wrapper 8·기본 생성자 8행. 입력 11열을 복원해 112바이트 의미 필드·전체 입자·색 표 메모리(색수 + 인접 64 DWORD)·난수 상태·dirty·선·생성 행의 반납 뒤 112바이트·외부 요청 순서(N 확보·B 부착·H SHP 조회·K 삭제·F 반납) 8열을 문자열 그대로 대조한다.
- 계약(웹 추가): 출력/삭제·확보 경계 누락과 다른 판본 거부, 공유 색이 없는데 생성 장치가 없으면 입자 확보 뒤 멈춤, 저장소 없는 효과의 무동작(난수 미소비), 색 표 인접 DWORD 미공급 거부.

최초 실패와 교정: 모듈은 fixture 893입력을 첫 실행에 모두 통과했다. 내가 쓴 계약 시험이 최대 수명 100·수명 100 이면 인접 DWORD(12번째)를 읽는다고 잘못 가정해 1건 실패했다(`float(1/100)` 이 내림이라 실제 번호는 11). 역수가 정확한 최대 수명 4 로 시험을 고쳤다(모듈 변경 없음).

## 3. 경계와 남은 것

- `SmokeVisualSystem`(Host/Kernel 부착·실제 입자 소유 월드·냉각 wrapper 호출자와 현재 SHP 포구 조회)·실제 색 표(공유 색 객체 owner)·dirty/선 래스터 연결은 후속 단위다(cpppj `RawCampaignParticles` 가 소비자). 이 단위는 몸체와 요청까지만 옮겼다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "입자 표시 마무리": `*VisualSystem` 8종·`RenderProcessSystem`. `RawBurningFrame` 은 cpppj 시험(`BurningFrameTests.cpp`)이 보호 자산으로 실제 캡처(`client/RawShapeToBuffer`·`client/BurningFrameCapture`·`client/SquidRenderer` 그림 헤더, 모두 PORT_MAP todo)와 `client/Renderer` 의 dirty 병합(웹 `render/dirty-regions.js`)을 실행하므로, 그 client 캡처 부품을 먼저 옮긴 뒤 진행한다.
