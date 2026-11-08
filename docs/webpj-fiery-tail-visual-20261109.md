# webpj 불꽃 꼬리 표시 시스템(FieryTailVisualSystem) 이식 — WB04-E195

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백아흔다섯 번째 부분이다. E187 에서 옮긴 불꽃 꼬리(process 11)·건설 마나(process 12) 몸체를 E192 공통 표시 저장소 위에서 실제 Host/Kernel 에 붙이는 `o/FieryTailVisualSystem` 을 옮겼다. cpppj 시험은 E194 건설 표시 시스템과 같은 저장소·Kernel 을 함께 쓴다.

- cpppj 기준선 `1f00a25a`. 소스·시험 모두 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋 `fbdcb4df`(소스·`FieryTailVisualSystemTests.cpp` 같은 커밋).
- 공유 장치: E192 `RenderProcessSystem`·`renderProcessRemovalBounds`, E194 `ConstructionVisualSystem`, E187 `RawFieryTailFrame`/`RawFieryTailLifetime`, WB03-Z `render/draw-sort.js`(시험 조립), D5c `SquidProcessHost`.

## 1. 옮긴 것

`www/core/fiery-tail-visual-system.js`(DOM 없음)

- `FieryTailVisualProcess`(SquidProcess 파생) — `fields`(불꽃 꼬리 프레임 필드)·`ready`, `runFrame`/`onFormDestroy`/`draw`/`destroy`(C++ 소멸자).
- `FieryTailVisualSystem(rendering, random, lifetime, frame, palette, hooks, variant = Fiery)` — E187 수명 몸체를 같은 저장소의 registry 로 만들고, 끝점(`endpoint`, 원본 00497220 의 현재 부모 화면 좌표)·반납 관찰 누락과 배율 불일치를 거부하며 프레임 출력 경계(invalidate/line)를 실제 몸체 생성으로 확인한다.
- `add(parent)` — 부모 0·중첩 거부 → 대기 객체 → `track` → `RawFieryTailLifetime.initialize`(등록 안에서 실제 client Form 부착, null/예외 실패면 즉시 소멸) → 생성 중 삭제 확인 → 준비 완료. 실패 정리는 E192~E194 와 같다.
- 프레임은 준비 완료 객체만 같은 난수/시계로 실행한다(이 효과는 스스로 소진하지 않는다). 출력은 호출 시점의 부모 끝점을 읽어 그대로 넘긴다(배율/카메라를 다시 더하지 않음). Form 삭제 직전에는 공통 삭제 영역 dirty 를 낸다. 소멸은 입자 → 공통 등록/base → 객체이며 공유 색 표는 남긴다. `dispose()` 는 Host 삭제(flags 0x10).

## 2. 검증

`tests/fiery-tail-visual-system.test.mjs`(4건). cpppj `FieryTailVisualSystemTests.cpp` 의 4건을 옮겼다(다섯째 `RenderProcessSystem_RemovalBoundsNative027f` 는 E192 에서 이미 재생).

- 건설 표시 2개와 불꽃 꼬리가 토큰 1·2·3·한 Kernel 을 공유, Form TYPE 11·120입자·깊이 −25, 첫 프레임 10입자, 공통 수집 3항목에서 불꽃 꼬리가 깊이 −25 로 마지막·출력 순서 C, C 다음 선 10개(색은 색수 11 다음 명시 인접 DWORD, 끝점 326/230), 건설 삭제 뒤 슬롯 0 재사용과 새 토큰, 시간 조각 진행(기존 > 10, 새 것 10), 시스템 정리 순서(불꽃 꼬리 → 건설)와 목록 확보 160 한 번·공유 색 77 유지·반납 순서. 부착 뒤 확보 실패·생성 중 재진입(프레임/출력/수집 무동작, 중첩 거부)·dead 부모 부착 실패의 잔재 없음. 부모 종속 삭제의 등록/Form 정리와 dirty 1건이 `renderProcessRemovalBounds` 와 같음. 오래된 토큰 출력 무동작과 배율 불일치 거부.
- 첫 시험의 수집·정렬·출력은 cpppj 가 client `DrawVisibleScene`(DrawSubmission, PORT_MAP todo)으로 한 번에 하는 부분이다. 웹은 그 단위가 아직 없어 공통 수집 → `render/draw-sort.js` 원본 정렬 → `drawToken` 으로 같은 관측을 시험 쪽에서 조립했다. DrawSubmission 자체의 이식/대조는 그 client 단위에서 한다.

최초 실패와 교정: 모듈과 시험 모두 첫 실행에 통과했고 교정이 필요한 실패가 없었다.

## 3. 경계와 남은 것

- 실제 Session/Client 월드 생성·건설 마나(Streak) 변형의 실제 호출자·끝점 장치(00497220)·선 래스터·client 수집/정렬 출력(DrawSubmission)은 후속 단위다(PORT_MAP session 0 유지).
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "입자 표시 마무리": `LightningVisualSystem`(cpppj 시험이 이 시스템을 함께 씀) → `ShrapnelVisualSystem`·`EvilRayVisualSystem`, 그리고 선행 부품이 필요한 `RawBurningFrame`→`BurningVisualSystem`.
