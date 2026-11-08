# webpj EvilRay 표시 시스템(EvilRayVisualSystem) 이식 — WB04-E198

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백아흔여덟 번째 부분이다. E190 에서 옮긴 EvilRay 몸체(process 10)를 E192 공통 표시 저장소 위에서 실제 Host/Kernel 에 붙이는 `o/EvilRayVisualSystem` 을 옮겼다. 구조는 E192 연기 표시 시스템과 같고(부모 0 이면 등록만 하고 Host 에 붙이지 않음), ProcessForm `notify` 가 수명 몸체의 Stop 으로 이어진다.

- cpppj 기준선 `1f00a25a`. 소스는 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋 `fbdcb4df`.
- **cpppj 전용 시험이 없다**(`EvilRayVisualSystem` 은 cpppj `client/RawCampaignParticles` 를 통해서만 쓰인다). 같은 구조인 `SmokeVisualSystemTests.cpp` 의 장면·관측을 EvilRay 에 맞게 옮긴 웹 조립 검사로 확인했다. EvilRay 계산은 E190 에서 실제 PE fixture 와 대조한 몸체다.
- 공유 장치: E192 `RenderProcessSystem`·`renderProcessRemovalBounds`, E190 `RawEvilRayFrame`/`RawEvilRayLifetime`, D5c `SquidProcessHost`.

## 1. 옮긴 것

`www/core/evil-ray-visual-system.js`(DOM 없음)

- `EvilRayVisualProcess`(SquidProcess 파생) — `fields`(EvilRay 의미 필드)·`ready`, `runFrame`/`onFormDestroy`/`notify(event)`(Stop mode, 나머지 두 인자는 원문처럼 무시)/`draw`/`destroy`(C++ 소멸자).
- `EvilRayVisualSystem(rendering, lifetime, frame, colors, hooks)` — 반납 관찰 누락·배율 불일치를 생성 때 거부하고, 프레임 장치(SHP 폭·음향 번호·음향·dirty·clip·타원·진단)는 cpppj 처럼 도달한 위치에서 요구한다. 출력의 공유 3색은 `colors.readable` 을 출력 때 읽는다.
- `add(parent)` — 중첩 거부 → 대기 객체 → `track` → `RawEvilRayLifetime.initialize`(등록 안에서 실제 client Form 부착, null/예외 실패면 즉시 소멸) → 생성 중 삭제 확인 → 준비 완료 → 부모 0 이면 시스템 보관. cpppj `Add` 처럼 생성 때는 배율을 검사하지 않고 프레임/출력에서 검사한다.
- 프레임/출력은 준비 완료 객체만 실행하고(kill 은 그 프로세스의 Host 삭제), Stop 은 준비 완료 뒤에만 수명 몸체로 넘긴다. Form 삭제 직전에는 공통 삭제 영역 dirty(장치가 없으면 그때 거부), 소멸은 입자 → 공통 등록/base → 객체이며 공유 색은 남긴다. `dispose()` 는 부모 0 객체를 직접 소멸하고 나머지를 Host 삭제(flags 0x10)한다.

## 2. 검증

`tests/evil-ray-visual-system.test.mjs`(4건, 웹 조립 검사).

- Form TYPE 10·50입자·source 전역, 첫 프레임 방출 1입자·시작 음향(번호 11)·dirty 2, 출력의 세 타원(색 2 → 1 → 0 순서), `notify(2)` 진단 뒤 미중단·`notify(3)` 중단, 2초 경과 프레임의 끝 음향(31)과 다음 프레임 자기 삭제·조회 제거·입자 → 객체 반납·삭제 깊이 0.
- 생성 중 프레임/출력/수집 무동작과 중첩 거부·입자 확보 실패의 객체만 반납·Host 부착 예외 정리 뒤 정상 생성.
- 부모 0 독립 소유(Form 0·Kernel 미부착·조회 가능), 부모 종속 삭제의 dirty 1건이 `renderProcessRemovalBounds` 와 같음, `dispose` 의 전체 반납.
- 반납 관찰 누락·배율 불일치 생성 거부, 생성 뒤 배율 불일치의 프레임/출력 거부.

최초 실패와 교정: 모듈과 시험 모두 첫 실행에 통과했다. 다만 내가 처음 쓴 네 번째 시험에 항상 참인 단언이 있어 통과 직후 의미 있는 검사(생성 뒤 배율 불일치의 프레임/출력 거부)로 고쳤다.

## 3. 경계와 남은 것

- cpppj 쪽에도 직접 시험이 없으므로 실제 소비자(`RawCampaignParticles`·EvilRay 발사/중단 호출) 이식 때 그 cpppj 시험으로 다시 대조한다. 실제 SHP 폭/음향 번호(현재 TYPE +70/+74)·위치 음향·타원 래스터·Session 월드 연결도 후속이다(PORT_MAP session 0 유지).
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절: 일반 `*VisualSystem` 은 Burning 만 남았고, 그것은 `RawBurningFrame` 과 client `RawShapeToBuffer`·`BurningFrameCapture`·`SquidRenderer` 그림 헤더가 선행이다. 그 밖의 3.1절 묶음(비행·이동 유닛, Dude 생성, 경로, 총·피해·삭제 수명, 다리, 자원·생산·건설 명령, 의식·신전, 그림·캡처, 공통 수명)에서 고른다.
