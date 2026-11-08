# webpj 분해 표시 시스템(DissolveVisualSystem) 이식 — WB04-E193

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백아흔세 번째 부분이다. E189 에서 옮긴 애시드 포획 Dissolve 몸체를 E192 공통 표시 저장소 위에서 실제 Host/Kernel·더미 상호 감시에 붙이는 `o/DissolveVisualSystem`(process 18)을 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스·시험 모두 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋 `fbdcb4df`(소스·`DissolveVisualSystemTests.cpp` 같은 커밋).
- 공유 장치: E192 `RenderProcessSystem`·`renderProcessRemovalBounds`, E189 `RawDissolveFrame`/`RawDissolveLifetime`, E188 `ShrapnelRelease`, D5c `SquidProcess`/`SquidProcessHost`, D5b `SquidFormDispatch`, D5a `DependFormHost`, D4 `SquidFactory`.

## 1. 옮긴 것

`www/core/dissolve-visual-system.js`(DOM 없음)

- `DissolveVisualProcess`(SquidProcess 파생) — `fields`(분해 의미 필드)·`dummy`(`{ value }` 상자, 생성 직후 더미 SID)·`paired`(상호 감시 쌍 완료)·`ready` 를 갖고 `runFrame`/`onFormDestroy`/`draw`/`destroy` 를 시스템으로 넘긴다. C++ 소멸자는 웹에서 `destroy()` 이며 Kernel 제거·부착 실패·부착 전 생성 실패 때 불린다.
- `DissolveVisualSystem(rendering, game, frame, hooks)` — 세계 경계(`lifetime.reserve/create/pop/depend/capture`)·반납 관찰·`destroyUnpaired`·프레임 출력 경계(`reserve/release/invalidate/pixel`, 실제 `RawDissolveFrame` 생성으로 확인) 누락과 배율 불일치를 첫 등록 전에 거부한다. 수명의 `attach` 는 대기 객체를 실제 Host 에 넘기고(실패면 즉시 소멸 뒤 오류), `depend` 는 상호 감시 쌍을 만든 뒤 `paired` 를 세우며, `prepare` 는 `RawDissolveFrame.initialize` 이고, 프레임 소진(`removeForm`)은 그 프로세스의 Host 삭제(flags 0)다.
- `add(source, first, second, xOriented)` — source 0·중첩 생성 거부 → 배율 확인 → 120바이트 확보(실패면 null, 등록/더미 없음) → 대기 객체 → `track` → `RawDissolveLifetime.initialize`(parent 0 등록 → 더미 생성/Pop → 실제 Host 부착 → 상호 감시 → 캡처 → 준비) → 준비 완료 순서다. 실패하면 부착된 객체는 Host 삭제(flags 0x10), 부착 전 객체는 직접 소멸한다.
- 소멸은 캡처 그림 → 입자 → 공통 등록/base → 객체 반납과 조회 제거 뒤, 상호 감시 쌍을 만들기 전에 실패해 아직 살아 있는 더미만 `destroyUnpaired` 로 정리한다(쌍 완료 뒤의 더미 삭제는 실제 Form 삭제 사슬이 맡는다). Form 삭제 직전에는 공통 삭제 영역 dirty 를 낸다. `dispose()` 는 남은 분해를 Host 삭제(flags 0x10)로 정리한다.

## 2. 검증

`tests/dissolve-visual-system.test.mjs`(3건).

- cpppj `DissolveVisualSystemTests.cpp` 2건을 1:1 로 옮겼다(실제 SidPool·SquidFactory·SquidHash·SquidUnpop·RawSquidDestroy·SquidFormDispatch·Kernel·SquidProcessHost·DependFormHost 장면, 캡처만 3×4 명시 입력). 가로/세로 두 방향에서 Form TYPE 18·부모=더미·상호 감시 쌍, 대상 삭제 뒤에도 분해 유지, 첫 프레임 시계 시작/delta 0·12픽셀·dirty 2, 자연 소진 뒤 Form·더미 free/dead·조회 제거·그림 → 입자 → 객체 반납·삭제 깊이 0. 완전 투명 캡처의 반납 3회·120바이트 확보 실패의 null(더미 미생성)·캡처 예외 뒤 Form/더미 정리.
- 웹 추가 계약: 경계 누락·배율 불일치·source 0 거부와 `dispose`.

최초 실패와 교정: 모듈과 시험 모두 첫 실행에 통과했고 교정이 필요한 실패가 없었다.

## 3. 경계와 남은 것

- 애시드 포획(E48 `raw-acid-zap.js`)에서 이 시스템을 부르는 호출, 실제 캡처(현재 source 의 원본 그림)·더미 생성/Pop 의 월드 연결, 픽셀 장치 연결, Session/Client 월드에 시스템을 만드는 일은 순서4~5 몫이다(PORT_MAP session 0 유지).
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "입자 표시 마무리": `FieryTailVisualSystem`·`LightningVisualSystem`(cpppj 시험 있음, `ConstructionVisualSystem` 을 같이 씀)·`ShrapnelVisualSystem`·`EvilRayVisualSystem`·`ConstructionVisualSystem`, 그리고 선행 부품이 필요한 `RawBurningFrame`→`BurningVisualSystem`.
