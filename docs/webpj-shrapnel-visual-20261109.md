# webpj 파편 표시 시스템(ShrapnelVisualSystem) 이식 — WB04-E197

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백아흔일곱 번째 부분이다. E188 에서 옮긴 픽셀 파편 몸체(process 16)를 E192 공통 표시 저장소 위에서 실제 Host/Kernel·더미 상호 감시에 붙이는 `o/ShrapnelVisualSystem` 을 옮겼다. 구조는 E193 분해 표시 시스템과 같고, 게임 난수와 별도 CRT 난수 두 스트림·세 가상 생성기(kind)를 쓴다.

- cpppj 기준선 `1f00a25a`. 소스는 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋 `fbdcb4df`.
- **cpppj 전용 시험이 없다**(`ShrapnelVisualSystem` 은 cpppj `client/RawCampaignParticles` 를 통해서만 쓰이고 `cpppj/tests` 에 이 시스템을 직접 검사하는 시험이 없음). 그래서 같은 구조인 `DissolveVisualSystemTests.cpp` 의 장면·관측을 파편에 맞게 옮긴 웹 조립 검사로 확인했다. 파편 계산 자체는 E188 에서 실제 PE fixture 1196입력과 대조한 몸체다.
- 공유 장치: E192 `RenderProcessSystem`·`renderProcessRemovalBounds`, E188 `RawShrapnelFrame`/`RawShrapnelLifetime`, D5c `SquidProcessHost`, D5b `SquidFormDispatch`, D5a `DependFormHost`, D4 `SquidFactory`.

## 1. 옮긴 것

`www/core/shrapnel-visual-system.js`(DOM 없음)

- `ShrapnelVisualProcess`(SquidProcess 파생) — `fields`(파편 의미 필드)·`dummy`(`{ value }` 상자)·`paired`·`ready`, `runFrame`/`onFormDestroy`/`draw`/`destroy`(C++ 소멸자).
- `ShrapnelVisualSystem(rendering, game, crt, frame, hooks)` — 세계 경계(`lifetime.reserve/create/pop/depend/capture`)·반납 관찰·`destroyUnpaired`·프레임 출력 경계(`reserve/release/invalidate/pixel`)와 두 난수를 실제 `RawShrapnelFrame` 생성으로 첫 등록 전에 확인하고 배율 불일치를 거부한다. 수명의 `attach` 는 대기 객체를 실제 Host 에 넘기고(null/예외 실패면 즉시 소멸), `depend` 는 상호 감시 쌍 뒤 `paired` 를 세우며, `prepare` 는 `RawShrapnelFrame.prepare(fields, kind)` 이고, 프레임 소진(`removeForm`)은 그 프로세스의 Host 삭제(flags 0)다.
- `add(source, kind)`(004604a0) — source 0·중첩 거부 → 배율 확인 → 104바이트 확보(실패면 null) → 대기 객체 → `track` → `RawShrapnelLifetime.initialize`(parent 0 등록 → 더미 생성/Pop → 부착 → 상호 감시 → 셋째 생성기 깊이 +1 → 중력 → 캡처 → 준비 → 최종 깊이) → 준비 완료. 실패 정리와 소멸 순서(그림 → 입자 → 등록/base → 객체, 쌍 전 실패 더미 정리)·Form 삭제 dirty·`dispose()` 는 E193 과 같다.

## 2. 검증

`tests/shrapnel-visual-system.test.mjs`(3건, 웹 조립 검사).

- 세 생성기(폭발·붕괴·셋째)마다 Form TYPE 16·부모=더미·상호 감시 쌍·중력 binary32(6·0.24·1.2)·최종 깊이 = 현재 source 깊이 5, 대상 삭제 뒤에도 유지, 첫 프레임 dirty 2·출력 픽셀 수 = 사용 꼬리의 양수 수명 입자 수, 10초 경과 프레임 뒤 다음 프레임 자기 삭제와 Form·더미 free/dead·조회 제거·그림 → 입자 → 객체 반납·삭제 깊이 0.
- 104바이트 확보 실패(null, 더미 미생성)·캡처 예외(Form/더미 정리)·Host 부착 예외(객체만 반납, 짝 없는 더미 정리, 이후 정상 생성).
- 경계 누락·CRT 난수 누락·배율 불일치·source 0 거부와 `dispose`.

최초 실패와 교정: 모듈은 첫 실행에 맞게 동작했다. 내 시험이 자연 소진에 1초(10배 시간 10)를 주었는데, 붕괴/셋째 생성기 수명은 최대 약 25 라 다 소진되지 않아 1건 실패했다. 경과 시간을 10초로 고쳤다(모듈 변경 없음).

## 3. 경계와 남은 것

- cpppj 쪽에도 이 시스템을 직접 검사하는 시험이 없으므로, 실제 소비자(`RawCampaignParticles`·건물 폭발/붕괴 호출) 이식 때 그 cpppj 시험(CampaignSession 계열)으로 다시 대조한다. 실제 캡처·더미 월드·픽셀 장치·Session 월드 연결도 후속이다(PORT_MAP session 0 유지).
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "입자 표시 마무리": `EvilRayVisualSystem`(cpppj 전용 시험 없음), 그리고 선행 부품이 필요한 `RawBurningFrame`→`BurningVisualSystem`.
