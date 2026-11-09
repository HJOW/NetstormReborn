# webpj Burning 표시 시스템(BurningVisualSystem) 이식 — WB04-E201

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 이백한 번째 부분이다. Burning(process 14) 몸체를 E192 공통 표시 저장소 위에서 실제 Host/Kernel 에 붙이는 `o/BurningVisualSystem` 을 옮겼다. 몸체는 생성·연소점 E183, 출력·소멸 E184, 프레임 E200, 캡처 E199 다.

구조는 E192 연기 표시 시스템과 같지만 다음이 다르다.

- 피해 부모 0 을 거부하므로 부모 0 보관 목록이 없다.
- Kernel 프레임은 487940 gate(표시 slot 과 현재 프레임 번호)를 거친다.
- 출력은 현재 dirty clip 으로 그리고, 배율은 프레임에서만 검사한다.
- ProcessForm `notify` 는 빈 ret12 다.

이로써 일반 Host 부착 `*VisualSystem` 8종(연기·분해·건설·불꽃 꼬리·빔·파편·EvilRay·Burning)과 E192 공통 표시 저장소가 모두 웹에 있다.

- cpppj 기준선 `1f00a25a`. 소스는 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋은 `961f6be5`.
- **cpppj 전용 시험이 없다.**
  - `BurningVisualSystem` 은 cpppj `client/RawCampaignParticles` 를 통해서만 쓰인다.
  - `BurningVisualTests.cpp` 는 E183 생성자 대조이고, `DamageableBurningSessionTests.cpp` 는 실제 Client Session 전체 장면이다.
  - 그래서 같은 구조인 `SmokeVisualSystemTests.cpp` 의 장면·관측을 Burning 에 맞게 옮긴 웹 조립 검사로 확인했다. 캡처는 실제 보호 SHP 를 쓴다.
- 공유 장치:
  - E192 `RenderProcessSystem`·`renderProcessRemovalBounds`
  - E183 `RawBurningVisual`/`RawBurningSources`
  - E184 `drawBurningVisual`/`destroyBurningVisual`/`renderProcessFrameDue`
  - E200 `RawBurningFrame`/`makeBurningFrameCapture`
  - E199 `RawShapeToBuffer`
  - D5c `SquidProcessHost`

## 1. 옮긴 것

`www/core/burning-visual-system.js`(DOM 없음)

- `BurningVisualProcess`(SquidProcess 파생):
  - 필드: `fields`(Burning 의미 필드), `frame`(생성자가 남긴 두 float), `ready`.
  - `runFrame`/`onFormDestroy`/`draw`/`destroy`(C++ 소멸자)는 시스템으로 넘긴다.
  - `notify()` 는 보호 VT 503730+1c 의 480260 이라 아무것도 하지 않는다. 기본 `SquidProcess.notify` 는 미복원 시 거부하므로 명시적으로 덮어썼다.
- `BurningVisualSystem(rendering, random, types, visualState, sourcesState, frameState, palette, frameNumber, hooks)`:
  - dirty·clip·픽셀·반납 관찰·소리 정지·10.78 풀·프레임 번호 getter 가 없으면 거부한다.
  - 같은 풀/캡처 조건은 실제 `RawBurningFrame` 생성으로 미리 확인한다.
  - 프레임이 쓰는 sink 는 억제 전역을 조회하지 않는다(`suppressed` false). flags 0 영역만 `hooks.invalidate` 로 넘긴다.
- `add(victim, mode)`:
  1. victim 0·중첩 생성은 거부한다.
  2. 대기 객체를 만들고 `track` 한 뒤 `RawBurningVisual.initialize` 를 부른다. 등록 안에서 실제 client Form 에 부착하며, 부착이 null/예외로 실패하면 즉시 소멸한다.
  3. 150입자/첫 색 표를 만들고 준비 완료로 둔다.
  4. 실패하면 부착 전 객체는 직접 소멸, 부착된 객체는 Host 삭제(flags 0x10)로 정리한다.
- 프레임: 준비 완료 객체만 487940 gate(slot < 7 또는 (slot XOR 프레임 번호)&3 = 0) 뒤 배율을 확인하고 `RawBurningFrame.step` 을 실행한다.
- 출력: 프레임 상태 사본에 현재 `clip()` 을 넣어 `drawBurningVisual` 을 부른다. 원문처럼 배율은 검사하지 않는다.
- Form 삭제 직전: 공통 삭제 영역을 dirty 로 넘긴다.
- 소멸: 이미지 → 행 → 소리 정지 → 입자 → 공통 등록 → 객체 순서로 반납한다. 공유 색 표는 남긴다.
- `dispose()`: 남은 객체를 Host 삭제(flags 0x10)로 정리한다.

## 2. 검증

`tests/burning-visual-system.test.mjs`(4건, 웹 조립 검사)

장면: 실제 10.78 TYPE 레코드, 내장 Form 생성자 주소, HP 10/20 의 Dude(TYPE 70) 피해 부모, 배율 16/11, 실제 ShapeToBuffer.

- 부착과 생성: Form TYPE 14, 150입자, 첫 색 표·색띠 1회. 빈 notify 를 확인한다.
- 단계 0: 실제 캡처가 직접 ShapeToBuffer 결과(11×17)와 같고, 행 표식은 (폭+높이)/5 개다.
- 단계 1: 첫 좌표(328, 233.75)·off·깊이 6 을 확인한다.
- visible 12프레임: 원본 폴백 연소점이 매 프레임 7개씩 쌓이고 selected 는 0 이다(작은 Dude 캡처라 300번 시도가 실패하는 E183 몸체 동작). 방출 입자와 dirty 를 확인한다.
- 출력과 삭제: 출력이 5점 단위이고 색 범위를 지킨다. Host 삭제의 삭제 dirty 는 `renderProcessRemovalBounds` 와 같고, 반납 순서와 소리 핸들 0 정지를 확인한다.
- 생성 중 처리:
  - 프레임/출력/수집 무동작, 중첩 거부를 확인한다.
  - 입자 확보 실패는 객체만 반납한다.
  - Host 부착 예외는 잔재를 남기지 않는다.
  - 두 번째 생성부터 공유 색을 재사용하고, `dispose` 를 확인한다.
- 487940 gate: 8개 중 slot 7 은 프레임 번호 0 에서 건너뛰고 3 에서 실행한다. 부모 종속 삭제는 준비된 8개마다 삭제 dirty 를 내고 객체 8개를 반납한다.
- 경계: dirty·clip·픽셀·반납·캡처·소리 정지 누락, 프레임 번호 getter 누락, 다른 풀 캡처, 피해 0 을 거부한다. 생성 뒤 배율이 어긋나면 프레임만 거부하고 출력은 허용한다.

최초 실패와 교정: 모듈은 첫 실행에 맞게 동작했다. 내 첫 시험이 "Dude 는 목표 1 이므로 selected 1·source 4개 이상"을 기대해 1건 실패했다. 원인은 11×17 캡처에서 300번 시도가 모두 실패해, 원본 폴백(selected 증가 없음)이 매 프레임 반복되는 것이다(E183 에서 원본 fixture 와 대조한 몸체 동작). 시험 기대를 이 원본 동작으로 고쳤다(모듈 변경 없음).

## 3. 경계와 남은 것

- cpppj 에도 직접 시험이 없다. 실제 소비자(`RawCampaignParticles`·피해 연소 생성 호출) 이식 때 그 cpppj Session 시험(`DamageableBurningSessionTests` 등)으로 E197·E198 과 함께 다시 대조한다.
- 실제 색띠 생성(`gradient`/`buildPalette`)·소리 정지 장치·Client 바람/카메라/viewport 전역·Session 월드 연결은 후속이다(PORT_MAP session 0 유지).
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

일반 `*VisualSystem` 은 모두 끝났다. 남은 입자/표시 client 단위는 순서5 표시 연결에서 옮긴다.

- `RawCampaignParticles`·`ConstructionVisualRenderer`·`ParticlePaletteMemory`(adapt)
- `ThunderProcess`(번개 팔레트 진행)·`SpellFrameVisibility`

순서3의 다음 후보는 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절에 있다. 특히 "그림·캡처" 묶음은 E199 `render/raw-shape-to-buffer.js` 로 원본 픽셀 대조(Geyser 459·ThunderCannon 333·SunAviary 384·Factory 200행)가 바로 가능해졌다.
