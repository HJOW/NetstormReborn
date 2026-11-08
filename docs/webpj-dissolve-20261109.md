# webpj Dissolve(분해) 프레임(RawDissolveFrame)·수명(RawDissolveLifetime) 이식 — WB04-E189

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백여든아홉 번째 부분이다. 애시드 포획(RawAcidZap, E48)이 대상 건물을 두 폴 사이로 녹여 없애는 120바이트 Dissolve 효과의 준비·프레임·출력을 맡는 `o/RawDissolveFrame`(패치 `00450460` 캡처 뒤 초기화·`004500a0` 준비·`0044fe40` 프레임·`0044ffc0` 출력)과, 생성(`00450510`/`004881d0`)·반납(`0044fd80`/`00423aa0`) 순서를 맡는 `o/RawDissolveLifetime` 을 옮겼다.

- cpppj 기준선 `1f00a25a`. 두 몸체·시험·fixture 모두 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋은 모두 `5d34fc2d`(소스·시험·fixture 같은 커밋).
- 공유 장치: 입자/캡처/등록 배치와 반납 대상은 E188 `raw-shrapnel-frame.js`(`createShrapnelFrameFields`·`shrapnelFloatFromBits`)·`raw-shrapnel-lifetime.js`(`ShrapnelRelease`)를 그대로 쓴다(cpppj `RawDissolveFrame.h` 가 `RawShrapnelFrame.h` 를, `RawDissolveLifetime.h` 가 `RawShrapnelLifetime.h` 를 포함하는 것과 같다). 그 밖에 E129 `RawRenderRegistry`, WB04-C1 `GameRandom`, WB04-A `SidPool`.

## 1. 옮긴 것

`www/core/raw-dissolve-frame.js`(DOM 없음)

- `createDissolveFrameFields()` — `{ common, opaqueCount, clockStarted, particlesAllocated }`. `common` 은 파편과 같은 의미 필드(중력은 쓰지 않음), `opaqueCount` 는 +0x70 불투명 픽셀 수, `clockStarted` 는 +0x74 첫 프레임 시계 시작, `particlesAllocated` 는 `new 0` 도 비영 포인터라 반납해야 하는 원본 의미를 용량 0 과 구분한다.
- `new RawDissolveFrame(pool, random, state, hooks)` — 게임 난수 하나만 쓴다(CRT 없음). `hooks` 는 파편과 같은 `{ reserve, release, invalidate, pixel, removeForm }` 이며 모두 필수다.
- 정수화: 원본 CRT 의 signed64 절삭 뒤 하위 DWORD 이고, 비유한 값이나 ±2^63 밖은 예외 대신 0(원본 `_ftol` 불확정 반환 경계)이다. 파편(E188)은 같은 경우에 거부하므로 두 모듈의 정수화 함수는 공유하지 않는다.
- `initialize(fields, first, second, xOriented)` — 현재 부모 더미의 좌표 × 배율(넓은 값)과 signed BYTE 깊이로 영역을 정하고, 캡처 기준점을 뺀 원점으로 `prepare` 를 부른다(저장한 float 사각형이 아니라 넓은 곱에서 뺀다).
- `prepare(fields, originX, originY, first, second, xOriented)` — 두 폴 위치에서 폴 보정(x float 20.8·y double 36.3)과 float 저장 위치를 원문대로 지켜 양쪽 간격·가로 오프셋을 구한다. 불투명 픽셀마다 **열 바깥/행 안쪽** 순서로 게임 난수 두 번(속도 잡음 `00455930(1,1.3)` → 방향 흔들림 `004558f0(−w,w)`)을 소비해 왼쪽/오른쪽(세로면 위/아래) 폴로 향하는 속도·수명과 가로 성분을 만든다. 투명 픽셀은 난수를 소비하지 않는다. 이전 입자 확보가 있으면 먼저 반납한다.
- `step(fields)` — 첫 호출에서만 시계를 현재 시각으로 맞춰 delta 0 을 적용하고(생성 시각과 무관), 용량 전체의 진입 양수 수명 입자를 무중력으로 이동해 정수 좌표 영역(+1)을 모은다. 진입 양수 입자가 없으면 dirty 없이 `removeForm` 만 부른다.
- `draw(fields)` — 사용 꼬리의 양수 수명 입자만 캡처 색으로 픽셀 요청을 낸다(x 만 카메라 감산 뒤 float 저장).

`www/core/raw-dissolve-lifetime.js`

- `new RawDissolveLifetime(pool, registry, hooks)` — `hooks = { reserve, create, pop, attach, depend, capture, prepare }` 모두 필수, `registry.pool` 이 같은 풀이어야 한다.
- `reserveObject()` — 원본 120바이트 요청. 실패는 false 이며 원본의 null 역참조는 재현하지 않는다.
- `initialize(fields, token, source, first, second, xOriented, dummy)` — parent 0 등록(process 18) → 필드 초기화(입자 확보 false) → 더미 생성(TYPE 110/flags 2, `dummy.value`) → 생성 callback 뒤 현재 source 좌표로 Pop(flags 16) → 부착 → Form 검사 → 상호 감시 → 캡처 그림·불투명 수·시계 시작 초기화 → 캡처 → 준비(`RawDissolveFrame.initialize`) 순서다. 최종 깊이는 준비가 현재 부모에서 읽는다.
- `destroy(fields, flags, release)` — 캡처 그림(있을 때) → 입자(실제 확보했을 때, 용량 0 이어도) → 공통 등록 제거/base → flags bit 0 이면 객체 순서이며 `ShrapnelRelease` 값을 쓴다.

## 2. 검증

`tests/dissolve-frame.test.mjs`(4건)·`tests/dissolve-lifetime.test.mjs`(2건). cpppj `DissolveFrameTests.cpp`·`DissolveLifetimeTests.cpp` 의 Replay/본문을 1:1 로 옮겼다.

- `dissolve-frame-x86.tsv`(10.78, 252입력, SHA-256 `988f0b60…`): 직접 준비 44행(원점 29/31·폴 61/62)·캡처 뒤 초기화 44행·프레임/출력 164행. 호출 전 84바이트·캡처·용량 전체 입자·부모/두 폴 150바이트·전역 28바이트·게임 난수를 입력으로 복원하고, 호출 뒤 84바이트·입자·난수·요청 순서(확보/반납/dirty/소진/픽셀)·68×48 행 padding 화면(시험 쪽 clip)·캡처 원본 불변을 허용오차 없이 비트 단위로 대조한다. cpppj fixture 폴더에 같은 내용의 64비트 관측 사본이 따로 있으나(바이트 동일 확인) cpppj 시험도 쓰지 않으므로 등록하지 않았다.
- `dissolve-lifetime-x86.tsv`(10.78, 288입력, SHA-256 `0a0ce44a…`): 방향 × 표시 목록 용량/구멍 × source 깊이 × callback 중 변경(더미 생성 중 source 좌표/깊이, 부착의 다른 부모 61, 캡처 중 깊이) × flags × 불투명/완전 투명 캡처. 같은 `RawDissolveFrame.initialize` 를 준비 경계로 연결해 생성 직후 100바이트·입자·난수·목록·생성 요청 순서와, 반납 뒤 100바이트(해제한 포인터/용량 표현 두 칸만 제외)·목록·반납 순서를 대조한다. 완전 투명 캡처도 입자 반납 요청이 나오는 것을 확인한다.
- cpppj 계약: 경계 미연결·다른 판본/풀 거부, 빈 캡처·꼬리 초과·색 주소 토큰 0/그림 밖 거부(난수 미소비), 무한대 카메라의 dirty 정수화 0, 확보 실패는 등록 전에 멈춤, 반납 경계 누락 거부.

최초 실패와 교정: 두 모듈 모두 fixture 전 행(252+288입력)과 계약 시험을 첫 실행에 통과했고 교정이 필요한 실패가 없었다.

## 3. 경계와 남은 것

- `DissolveVisualSystem`(Host/Kernel 부착·실제 입자 소유 월드)·실제 캡처·더미 생성/Pop/상호 감시의 월드 연결·픽셀 장치 연결, 그리고 애시드 포획(E48 `raw-acid-zap.js`)에서 이 효과를 실제로 만드는 호출 연결은 후속 단위다. 이 단위는 몸체와 요청까지만 옮겼다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "입자 표시 마무리": `RawEvilRayFrame`/`RawEvilRayLifetime` → `RawSmokeEffect` → `RawBurningFrame` → `*VisualSystem` 8종·`RenderProcessSystem`.
