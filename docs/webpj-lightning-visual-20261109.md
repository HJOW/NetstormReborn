# webpj 빔 표시 시스템(LightningVisualSystem) 이식 — WB04-E196

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백아흔여섯 번째 부분이다. E117 빔 경로와 E186 빔 수명/프레임 몸체를 E192 공통 표시 저장소 위에서 실제 Host/Kernel 에 붙이는 `o/LightningVisualSystem`(process 19)을 옮겼다. cpppj 시험은 E195 불꽃 꼬리 표시 시스템과 같은 저장소·Kernel 을 함께 쓴다.

- cpppj 기준선 `1f00a25a`. 소스·시험 모두 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋 `fbdcb4df`(소스·`LightningVisualSystemTests.cpp` 같은 커밋).
- 공유 장치: E192 `RenderProcessSystem`·`renderProcessRemovalBounds`, E195 `FieryTailVisualSystem`, E117 `RawLightningPath`, E186 `RawLightningLifetime`/`RawLightningFrame`, D5c `SquidProcessHost`.

## 1. 옮긴 것

`www/core/lightning-visual-system.js`(DOM 없음)

- `LightningVisualProcess`(SquidProcess 파생) — `fields`(빔 프레임 필드)·`ready`, `runFrame`/`onFormDestroy`/`draw`/`destroy`(C++ 소멸자).
- `LightningVisualSystem(rendering, random, lifetime, frame, hooks)` — 경로 상태 `{ scaleX 16, scaleY 11, vanderType 148, tallType 149, manOWarType 144 }`·실제 `RawLightningPath`(중심 조회 `center`, 004adea0)·`RawLightningLifetime` 을 소유한다. 반납 관찰 누락·배율 불일치를 거부하고 프레임 출력 경계(invalidate/line/sound)를 실제 몸체 생성으로 확인한다. 배율 검사 때마다 프레임 배율을 경로 생성 배율에 반영한다.
- `add(parent, target, sound)`(00472700/00472690 → 00471b20)·`addAtScreen(parent, from, to, life, sound)`(00472770 → 00471e70, 정수 화면 양끝을 대기 객체 생성 뒤·등록 전에 float 로 변환) — 두 생성자가 같은 경로를 쓴다: 부모 0·중첩 거부 → 배율 확인 → 대기 객체 → `track` → 수명 생성(등록 안에서 실제 client Form 부착, null/예외 실패면 즉시 소멸) → 생성 중 삭제 확인 → 준비 완료. 실패 정리는 E192~E195 와 같다.
- 프레임은 준비 완료 객체만 같은 난수/시계로 실행하며 소진(`removeForm`)은 그 프로세스의 Host 삭제(flags 0)다. 출력은 이전 점/굵기 선이며 끝점 조회가 없다. Form 삭제 직전에는 공통 삭제 영역 dirty, 소멸은 경로 → 입자 → 공통 등록/base → 객체이며 공유 색 객체는 남긴다. `dispose()` 는 Host 삭제(flags 0x10).

## 2. 검증

`tests/lightning-visual-system.test.mjs`(4건). cpppj `LightningVisualSystemTests.cpp` 3건을 1:1 로 옮기고 웹 계약 1건을 더했다.

- 자연 수명과 파일명 소리: Form TYPE 19·120입자·경로 확보·공유 색 88·깊이 −25, 첫 프레임 Seed 1입자와 `shortZap.wav` 1회(부모 raw 좌표 20.5/21.25, 출력 선 없음), 다음 시간 조각의 색 195/85 선, 2.6초에 소진 → 자기 삭제·경로 → 입자 → 객체 반납·오래된 토큰 출력 없음.
- 120입자(2880바이트)/경로(64바이트) 확보 실패와 생성 중 재진입(프레임/출력/수집 무동작·난수 불변·중첩 거부)의 잔재 없음과 반납(입자 → 객체), 이후 정상 생성의 슬롯 0.
- 불꽃 꼬리와 토큰 1·2·한 Kernel·공통 수집 2항목 공유, 부모 종속 삭제의 두 표시 정리와 삭제 dirty 2건, 두 공유 색 유지.
- 웹 추가: `addAtScreen` 의 같은 Host/등록 수명, 부모 0·배율 불일치 거부(생성과 프레임), `dispose` 정리.

최초 실패와 교정: 모듈과 시험 모두 첫 실행에 통과했고 교정이 필요한 실패가 없었다.

## 3. 경계와 남은 것

- 실제 Session/Client 월드 생성·발사 쪽 호출(createLightning/createLightningAtScreen 의 create 경계 연결)·중심 조회 장치(004adea0)·파일명 소리 재생·선 래스터는 후속 단위다(PORT_MAP session 0 유지).
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "입자 표시 마무리": `ShrapnelVisualSystem`·`EvilRayVisualSystem`(cpppj 전용 시험 없음, 다른 시스템 시험 방식으로 조립 검사), 그리고 선행 부품이 필요한 `RawBurningFrame`→`BurningVisualSystem`.
