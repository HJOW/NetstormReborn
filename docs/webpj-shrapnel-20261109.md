# webpj 픽셀 파편 프레임(RawShrapnelFrame)·수명(RawShrapnelLifetime) 이식 — WB04-E188

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백여든여덟 번째 부분이다. 원본 `Flyingshrapnel.cpp` 의 픽셀 파편 효과(건물 캡처 그림을 픽셀 묶음으로 쪼개 날리는 폭발/붕괴 표시)에서 묶음 생성·중력/수명 프레임·현재 색 픽셀 출력을 맡는 `o/RawShrapnelFrame`(패치 `00460000` 준비·세 가상 묶음 생성기·`004600a0` 분할·`0045fd60` 프레임·`0045fef0` 출력)과, 부모 없는 표시 등록 → 더미 생성/Pop/부착 → 캡처/준비 생성 순서와 공통 반납 순서를 맡는 `o/RawShrapnelLifetime`(`004604a0` wrapper·세 가상 생성자·소멸)을 옮겼다.

- cpppj 기준선 `1f00a25a`. 두 몸체·시험·fixture 모두 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋: 프레임 `392ab168`(소스·시험·fixture 같은 커밋), 수명 `954d1942`.
- 공유 장치: E129 `RawRenderRegistry`·`createRenderProcessFields`, WB04-C1 `GameRandom`·`CrtRandom`, WB04-A `SidPool`. 새 공유 장치는 만들지 않았다.

## 1. 옮긴 것

`www/core/raw-shrapnel-frame.js`(DOM 없음)

- `ShrapnelKind`(`Explosion` 0·`Collapse` 1·`Variant2` 2) — 원본 세 가상 픽셀 생성기 번호이며 수명 생성자의 중력 표 첨자다. 셋째의 의미 이름은 cpppj 와 같이 추측하지 않는다.
- `createShrapnelFrameFields()`·`createShrapnelParticle()` — 104바이트 객체의 의미 필드(`registration`·용량 전체 `particles`·사용 꼬리 `total`·`lastTime`/`elapsed`·`delta`/`scaledDelta`·`first`·`image`(캡처 `{ width, height, hotspotX, hotspotY, pixels }`)·`gravity`(기본 6))와 24바이트 입자 `{ x, y, velocityX, velocityY, life, pixel }`(pixel 은 0 미사용, 나머지는 같은 캡처의 offset+1 색 주소 토큰)이다.
- `new RawShrapnelFrame(pool, game, crt, state, hooks)` — 게임 난수(폭발 속도)와 CRT 난수(묶음 크기·수명)를 따로 빌리고 합치지 않는다. `state` 는 불꽃 꼬리와 같은 `{ now, pixelScaleX, pixelScaleY, rate, cameraX, cameraY }`, `hooks = { reserve, release, invalidate, pixel, removeForm }` 는 모두 필수다.
- `prepare(fields, kind)` — 부모 raw 좌표 × 배율(넓은 값)로 영역을 정하고 캡처 기준점을 빼 절삭한 원점에서, 불투명(255 아님) 픽셀 수(최소 1)의 용량을 확보한 뒤 작업 복사본을 행/열 순서로 훑어 아직 고르지 않은 픽셀마다 가상 생성기를 부른다. 폭발은 게임 난수(`Random(20,14)` = low 20 + 0..6) → CRT 순서, 붕괴/셋째는 CRT 만 쓴다. 분할 `004600a0` 은 열/행 한도(3..5)를 **반복 조건마다 다시 뽑고**(마지막 실패 검사에서도 소비) 첫 행/열(좌표 0)은 원문의 `> 0` 비교로 제외한다. 이전 입자 저장소가 있으면 먼저 반납하고, 작업 복사본은 끝에 반납한다. 캡처 원본은 바꾸지 않는다.
- `step(fields)` — 간격 제한 없이 한 시간 조각(10배 시간)으로 delta/scaledDelta/가속도를 먼저 float 로 저장하고, 용량 전체(사용 꼬리가 아님)의 진입 양수 수명 입자만 수명·속도·좌표를 갱신한다. y 갱신과 영역(±3)에는 저장 전의 넓은 새 속도/좌표를 쓴다. 진입 양수 입자가 하나도 없으면 dirty 없이 `removeForm` 만 부른다.
- `draw(fields)` — 사용 꼬리의 양수 수명 입자만 캡처 색으로 픽셀 요청을 낸다. x 는 절삭 뒤 카메라를 뺀 값을 float 로 저장한 다음 절삭하고, y 는 넓은 값 그대로 절삭한다. 색 주소 토큰이 0 이거나 그림 밖이면 픽셀을 내지 않고 거부한다.

`www/core/raw-shrapnel-lifetime.js`

- `ShrapnelRelease`(`Source`·`Particles`·`Object`)·`new RawShrapnelLifetime(pool, registry, hooks)` — `hooks = { reserve, create, pop, attach, depend, capture, prepare }` 모두 필수, `registry.pool` 이 같은 풀이어야 한다.
- `reserveObject()` — wrapper 의 104바이트 요청. 실패하면 생성자/등록/더미 생성에 들어가지 않는다.
- `initialize(fields, token, source, kind, dummy)` — parent 0 등록(process 16, 초기 깊이는 SID 0) → 필드 초기화 → 더미 생성(TYPE 110, flags 2, 결과는 즉시 `dummy.value`) → 현재 source 좌표를 **생성 callback 뒤 다시 읽어** Pop(flags 16) → 부착(process 16, flags 16) → Form 번호 검사 → 상호 감시(`004b1510`) → 셋째 생성기만 깊이 +1(32비트 감김) → 캡처 그림 비움 → 중력(binary32 `0x40c00000`/`0x3e75c28f`/`0x3f99999a`) → 캡처 → 준비 → 최종 깊이는 그 시점 source 의 signed BYTE 순서다.
- `destroy(fields, flags, release)` — 캡처 그림 → 입자 → 공통 등록 제거/base → flags bit 0 이면 객체 순서이며, 있는 저장소만 반납한다.

## 2. 검증

`tests/shrapnel-frame.test.mjs`(3건)·`tests/shrapnel-lifetime.test.mjs`(2건). cpppj `ShrapnelFrameTests.cpp`·`ShrapnelLifetimeTests.cpp` 의 Replay/본문을 1:1 로 옮겼다.

- `shrapnel-frame-x86.tsv`(10.78, 980입력, SHA-256 `6c7f82e6…`): 준비 198행과 프레임/출력 782행. 호출 전 76바이트 의미 상태·캡처 그림·용량 전체 입자·부모 50바이트·전역 28바이트·두 난수 상태를 입력으로 복원하고, 호출 뒤 의미 상태·용량 전체 입자·게임/CRT 난수·요청 순서(확보/반납/dirty/소진/픽셀)·68×48 행 padding 화면(시험 쪽 clip)·캡처 원본 불변을 허용오차 없이 비트 단위로 대조한다.
- `shrapnel-lifetime-x86.tsv`(10.78, 216입력, SHA-256 `68b71fd2…`): 세 생성기 × 표시 목록 용량/구멍 × source 깊이 × callback 중 변경(더미 생성 중 source 좌표/깊이, 부착의 다른 부모 61, 캡처 중 깊이) × flags. 같은 `RawShrapnelFrame.prepare` 를 준비 경계로 연결해 생성 직후 92바이트·입자·두 난수·목록·생성 요청 순서와, 반납 뒤 92바이트(해제한 포인터/용량 표현 두 칸만 제외, cpppj 시험과 같음)·목록·반납 순서를 대조한다.
- cpppj 계약: 경계 미연결·다른 판본/풀 거부, 빈/불일치 캡처·꼬리 초과·부모 0/용량 밖·생성기 범위 밖 거부(난수 미소비), 색 주소 토큰 0/그림 밖 거부, wrapper 확보 실패는 등록 전에 멈춤, 반납 경계 누락 거부.

최초 실패와 교정: 두 모듈 모두 fixture 전 행(980+216입력)과 계약 시험을 첫 실행에 통과했고 교정이 필요한 실패가 없었다.

## 3. 경계와 남은 것

- `ShrapnelVisualSystem`(Host/Kernel 부착·실제 입자 소유 월드)·실제 캡처(`capture`, 원본 plain/mapped 그림)·더미 생성/Pop/상호 감시의 실제 월드 연결·픽셀 장치(clip/pitch) 연결은 후속 단위다. 이 단위는 몸체와 요청까지만 옮겼다.
- `RawDissolveLifetime` 이 이 모듈의 수명 몸체를 쓰므로(cpppj `RawDissolveLifetime.h` 가 `RawShrapnelLifetime.h` 포함) 다음 Dissolve 단위는 이 모듈 위에 얹는다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "입자 표시 마무리": `RawDissolveFrame`/`RawDissolveLifetime`(이 모듈 재사용) → `RawEvilRayFrame`/`RawEvilRayLifetime` → `RawSmokeEffect` → `RawBurningFrame` → `*VisualSystem` 8종·`RenderProcessSystem`.
