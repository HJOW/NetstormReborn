# webpj 입자 색 띠(ParticleGradient)·번개 프레임/출력(RawLightningFrame)·번개 수명(RawLightningLifetime) 이식 — WB04-E186

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백여든여섯 번째 부분이다. 원본 `Colorsort.cpp` 의 RGB 팔레트 색 띠와 `Particle.cpp` 의 색 표 병합을 맡는 `o/ParticleGradient`(패치 `00439de0`/`0043ad20` 색 띠·`00487d20` 병합·`00457ea0` 불꽃 꼬리 표·`00458480` 번개 표), 번개 빔의 시간 조각·흔들림·굵기 선·소진 요청을 맡는 `o/RawLightningFrame`(패치 `00472310` 프레임·`004724f0` 출력), 빔의 생성/경로 소유와 공통 표시 반납 순서를 맡는 `o/RawLightningLifetime`(wrapper `00472700`/`00472690`/`00472770`·생성자 `00471b20`/`00471e70`)을 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 세 몸체와 시험/fixture 의 변경 없음을 `git diff --numstat` 으로 확인). E117 `raw-lightning-path.js`·E129 `RawRenderRegistry` 위에 얹어 번개 계열의 실제 PE 관측 fixture 세 개(48+250+110행)와, E183 `RawBurningVisual.append` 를 보강하는 `burningparticle`(10.78 282행)을 재생한다.

## 1. 옮긴 것

`www/core/particle-gradient.js`(DOM 없음)

- `buildParticleGradient(palette, from, to, colors, width = 2.4)` — RGBX DWORD 256개 팔레트에서 시작/끝 RGB(double 세 개)에 가장 가까운 색(B/G/R 제곱 순서의 합, 동률이면 먼저 나온 색)을 찾아 평행 이동 → 축 투영(R/B/G 내적 ÷ 제곱 길이)과 B/G/R 거리 제곱 저장 → 원본 CRT 의 세 번 정렬(거리·원통 안팎/축 위치·선택 표식, `004e6010`/`004e5fa0` 의 피벗/교환 순서)과 원통 후보에서의 가장 가까운 위치 선택(중복 선택도 표식 1 유지)으로 색 띠를 만든다. `colors` 2..256, `width` 유한 양수, 반올림 색 수 256 미만, 영길이 축 거부. 결과는 `{ entries(256), from, to, count, selected, threshold }` 이다.
- `mergeParticleGradients(gradients)` — 최대 3개 색 띠에서 각 색 띠의 요청 색 수만큼 앞 항목 번호를 차례로 합친다(실제 선택 수가 아닌 요청 수 사용). `buildFieryTailPalette`(어두운 빨강 → 주황 8색 + 주황 → 밝은색 3색)·`buildLightningPalette`(파랑 → 흰색 8색 + 흰색 → 128/255 회색 3색)는 PE 의 RGB double 비트 상수로 두 색 띠를 병합한 11개 색 번호이다.

`www/core/raw-lightning-frame.js`

- `createLightningFrameFields()`·`new RawLightningFrame(pool, random, path, state, hooks)` — `state = { now, pixelScaleX, pixelScaleY, rate, cameraX, cameraY }`, `hooks = { invalidate, line, sound, removeForm }` 는 모두 필수이다. `step(fields)` 는 최초에는 Seed 와 선택 파일명 소리만, 그 뒤에는 시간 조각(최대 0.05초, 시간 후퇴/0 간격도 원문 그대로)마다 경로 진행 → 용량 전체(사용 꼬리가 아님) 수명 감소·흔들림(난수 y → x 순)·영역 누적 → 모든 입자가 죽으면 `removeForm` 뒤 즉시 반환, 아니면 이전/새 영역 dirty 요청을 한다. `draw(fields)` 는 첫 생존 입자 다음부터 사용 꼬리까지 살아 있는 점을 이전 점에 연결해 중심선(색 195)과 굵기에 따라 보조선(색 85)을 요청한다(카메라 차이는 float 로 먼저 저장, 보조선의 ±1 은 그 값에서 넓게 계산).

`www/core/raw-lightning-lifetime.js`

- `createLightning`(dead/void 표적은 raw 주소 SID 로 구별, 생존 표적만 생성)·`createLightningAtScreen`(카메라 signed DWORD 감김, 파일명 `shortZap.wav`)·`new RawLightningLifetime(pool, registry, path, state, hooks)` 의 `initialize`(등록/부착 → 120입자 zero → 공유 색 → 깊이 −25 → 경로 64바이트 확보 → 부착이 바꿀 수 있는 현재 부모의 경로 → 흔들림 = 거리 × float 0.025 → 파일명)·`initializePoints`(직접 좌표 경로, life 가 0 일 때만 기본값 보존)·`destroy`(경로 → 입자 → 공통 등록 제거 → flags bit 0 객체 순서, 공유 팔레트는 반납하지 않음).

## 2. 검증

`tests/particle-gradient.test.mjs`(3건)·`tests/lightning-frame.test.mjs`(3건)·`tests/lightning-lifetime.test.mjs`(3건)과 `tests/burning-visual.test.mjs` 의 추가 1건.

- `particle-gradient-x86.tsv` 기본 027f 24입력: 입력마다 실제 COL 팔레트 256색과 두 색 띠 A/B 의 전체 256항목 의미 48바이트(RGB 차 double 세 개·색 번호·거리·축 위치·선택 표식)와 머리(시작/끝/선택 수/임계 비트), 1/2/3색 띠 병합 번호를 부동소수점 허용오차 없이 DWORD/QWORD 전체로 대조한다(비교 정밀도 037f 행은 중복 가산하지 않음). `fierytail-lifetime-x86.tsv` 의 10.78 cold 기본 생성자 3행으로 불꽃 꼬리 표의 두 RGB 끝점 순서를 검증한다(나머지 행은 불꽃 꼬리 이식 단위의 몫).
- `lightning-frame-x86.tsv` 250행(프레임 + 출력 220, 출력만 30): 최초/연속 소리·시간 후퇴/긴 간격·전체 용량 수명·흔들림·소진과 굵기 경계·포인터 방향·카메라 float 저장/절삭·보조선 위치를 의미 64바이트·경로 64바이트·용량 전체 입자·난수 상태·출력 요청(D/L/S/K) 바이트 단위로 대조한다.
- `lightning-lifetime-x86.tsv` 기본 027f 55입력: 생성 직후 의미 88바이트·경로·120입자 zero·번개 공유 색 표 11개(`buildLightningPalette` 가 실제 PE 와 같은 번호)·표시 슬롯·확보/부착 요청 순서와 반납 뒤 상태·순서(경로 → 입자 → 등록 제거 → 객체)를 대조한다(부착 중 깊이/현재 부모 변경 재진입 포함).
- `burningparticle-x86.tsv` 10.78 282행: 여유 칸 확장, ordered float 수명 ≤ 0 의 첫 만료 칸 재사용, NaN/무한대/음수 0 수명 경계(E183 `RawBurningVisual.append` 의 추가 검증).
- cpppj 계약: 색 띠 개수/폭/비유한 RGB/후보 수 범위/영길이 축/병합 개수 거부, 번개 프레임의 경계 미연결·입자 저장소/부모 범위 거부, wrapper 의 dead/void 표적과 화면 카메라 감김, 직접 좌표 생성의 life 보존, 생성자의 경계/다른 풀 거부.

최초 실패와 교정: 모듈은 모든 fixture(48+250+110행과 282행)를 첫 실행에 통과했다. 내가 쓴 번개 시험에서 경로 상태 기본값(scale 16/11, 타입 148/149/144)을 잘못 넣어 수명 시험 한 건이 처음에 실패했고 시험 입력만 고쳤다(번개 프레임 시험의 상태는 `step` 에 쓰이지 않아 영향이 없었다).

## 3. 경계와 남은 것

- `LightningVisualSystem`(Host/Kernel 부착, 실제 입자 소유 월드)·번개 발사/피해 로직·실제 팔레트(`BuildLightningPalette` 호출 위치)/Surface/소리 연결은 후속 단위이다. `RawFieryTailFrame`/`RawFieryTailLifetime` 은 이 단위가 `buildFieryTailPalette` 를 먼저 옮겨 두었다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`LightningVisualSystem`·`RawFieryTailFrame`/`RawFieryTailLifetime`·`FieryTailVisualSystem`, `RawBurningFrame`(캡처 자산 필요)·`BurningVisualSystem`·`RenderProcessSystem`, `RawFlyerProcess`→`RawWindFlyer`, `RawPacketDispatch`→`RawConstructionCommand`, Missile Host 팩토리, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
