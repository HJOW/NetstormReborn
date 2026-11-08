# webpj EvilRay 프레임(RawEvilRayFrame)·수명(RawEvilRayLifetime) 이식 — WB04-E190

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백아흔 번째 부분이다. 원본 `Evilray.cpp` 의 EvilRay 입자 효과(process 10: source 위치에서 15 간격으로 원형 고리를 방출해 13 수명 동안 반경이 커지며 세로 속도 3 으로 이동하는 표시)에서 프레임·방출 기한·음향·표시 영역과 세 색 원형 타원 출력을 맡는 `o/RawEvilRayFrame`(패치 `00453290` 프레임·`00453540` 출력)과, 생성(`004530b0`)·중단 통지(`00453240`)·공통 소멸 순서와 단일 3색 띠를 맡는 `o/RawEvilRayLifetime` 을 옮겼다.

- cpppj 기준선 `1f00a25a`. 두 몸체·시험·fixture 모두 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋: 프레임 `d4652e85`, 수명 `8eebe34b`(각각 소스·시험·fixture 같은 커밋).
- 공유 장치: E186 `particle-gradient.js`(`buildParticleGradient`·`mergeParticleGradients`), E187 `createFieryTailParticle`(같은 24바이트 입자), E129 `RawRenderRegistry`·`createRenderProcessFields`, WB04-A `SidPool`.

## 1. 옮긴 것

`www/core/raw-evil-ray-lifetime.js`(DOM 없음)

- `createEvilRayFields()` — 공통 SystemProcess 와 파생 필드(`registration`·50입자 `particles`·`total`·`lastTime`/`elapsed`·`delta`/`scaledDelta`·`first`·`nextEmission`(float 방출 기한)·`source`/`stopping`(전체 DWORD))이다.
- `buildEvilRayPalette(palette)` — PE binary64 RGB 상수(`0x00508e60/68/58` → `0x00508e78/70/68`, 끝색 G 성분이 1 을 넘는 값도 원문대로)로 요청 3색 띠 하나를 만든다(폭 2.4, E186 정렬/선택/병합 재사용).
- `EvilRayRelease`(`Particles`·`Object`)·`new RawEvilRayLifetime(pool, registry, state, hooks)` — `state = { palette, processType(10), source(0), sharedProcessType?, sharedSource? }`. 공유 함수가 있으면 TYPE 는 **등록 전**, source 는 **등록/부착 뒤** 현재 값을 읽는다(원문 인자 위치). `hooks = { reserve, createPalette, buildPalette, assertion }` 는 도달한 위치에서만 요구한다(cpppj 와 같이 미연결이면 그때 오류이며 앞선 효과는 남는다).
- `initialize(fields, token, parent)` — TYPE → 공통 등록/부착 → 필드 초기화·현재 source → 1200바이트 확보·50입자 zero → 공유 색이 없을 때만 8바이트 확보·생성·3색 계산 순서다.
- `stop(fields, mode)` — mode 3 만 `stopping=1`. 다른 mode 는 `false`(0x4e) 진단 뒤 현재 값을 쓰지 않고 복귀, 이미 중단(0 아닌 전체 DWORD)은 `stopping == false`(0x48) 진단 뒤 다시 1 을 쓴다.
- `destroy(fields, flags, release)` — 입자(있을 때) → 공통 등록 제거/base → flags bit 0 이면 객체 순서이며 공유 색 객체는 반납하지 않는다.

`www/core/raw-evil-ray-frame.js`

- `new RawEvilRayFrame(pool, state, colors, hooks)` — `colors` 는 공유 3색을 출력 때 읽는 살아 있는 배열(또는 그 배열을 돌려주는 함수). `hooks = { shapeWidth, soundId, sound, invalidate, kill, clip, circle, assertion }` 는 도달한 곳에서만 요구한다.
- `step(fields)` — source 번호를 먼저 저장하고(0 이거나 용량 밖이면 `sourceSquid.isValid()` 진단, 정상 복귀 가능), 현재 signed 16비트 SHP 폭을 물은 **뒤** 현재 raw 좌표로 `x = 배율×좌표 − 폭/2(0 방향 정수 나눗셈)` 를 float 로 저장한다. 최초 시계 준비 → 전체 delta 한 번(clamp/시간 조각 없음) → 방출 기한(`nextEmission ≤ elapsed`)이고 중단 중이 아니면 source 음향 번호(0 이면 부모 head 진단 뒤 **저장한** 부모 번호의 음향 번호) → 음향 callback 뒤 현재 source 좌표로 시작 음향 → 끝 꼬리에만 입자 방출(세로 속도 3·수명 13, 죽은 슬롯 재사용 없음) → 기한 `elapsed + 15`. 이어 사용 꼬리의 양수 수명 입자를 이동하고 정수 좌표 ±8(int32 감김 뒤 float)로 영역을 모으며, 이번에 수명이 0 이하가 된 입자는 메타데이터 호출 **전에** 저장한 현재 부모 좌표로 끝 음향을 낸다. 이번 호출에 활성 입자가 없고 중단 중이면 `kill(0)` 만, 아니면 이전/새 영역 dirty 를 요청한다.
- `draw(fields)` — 꼬리가 용량보다 크면 `totalParticles <= maxParticles` 진단 → `clip([0,0,639,479])` → 사용 꼬리의 양수 수명 입자마다 반경 `max(1, ftol((13−수명)×8×(1/13)))` 의 세 타원(반경 −2/−1/0, 색 2/1/0 순서)을 요청한다. 용량 밖 입자는 진단/clip 뒤 명시 오류다.
- 정수화는 원본 `__ftol` 처럼 signed64 절삭 뒤 하위 DWORD 이고 비유한/±2^63 밖은 0 이다(E189 Dissolve 와 같은 규칙, 모듈별 사본).

## 2. 검증

`tests/evil-ray-frame.test.mjs`(3건)·`tests/evil-ray-lifetime.test.mjs`(3건). cpppj `EvilRayFrameTests.cpp`·`EvilRayLifetimeTests.cpp` 의 본문을 1:1 로 옮겼다.

- `evilray-frame-x86.tsv`(10.78, 기본 027f 18행 + 비교 037f 18행, SHA-256 `61b89ebb…`): 프레임 10·출력 8행. 객체 raw·용량 전체 입자·SID 0/50/51 물리 150바이트·전역 28바이트·매개변수(폭·음향 번호·진단 중 source 변경)를 입력으로 복원하고, 80바이트 의미 필드·전체 입자·요청 순서(폭 조회 M·음향 번호 T·음향 S·dirty D·삭제 K·clip C·타원 R·진단 A)를 문자열 그대로 대조한다. 비교 정밀도 행은 중복 가산하지 않는다.
- `evilray-lifetime-x86.tsv`(10.78, 생성 기본 12행 + 비교 12행, Stop 기본 5행 + 비교 5행, SHA-256 `cbd1c9b8…`): 두 팔레트(GIFCLOUD/BULF)·기존 공유 색 여부·부모 0/50·부착 중 TYPE/source/깊이 변경 조합의 생성 80바이트·공유 3색·등록 목록·생성/반납 요청 순서, 그리고 Stop 의 진단 인자와 결과를 대조한다.
- cpppj 계약: 부모 head 진단 뒤 저장한 부모 번호와 현재 TYPE 사용, 출력의 진단 → clip → 용량 밖 오류 순서, 정확히 0 이 된 수명의 끝 음향이 다음 호출의 빈 중단 kill 보다 앞섬, TYPE/source 읽기 위치, 미연결 장치 도달 때 1200·8바이트 확보 뒤 멈추고 등록·공유 색 0 이 남음.

최초 실패와 교정: 두 모듈은 첫 실행에 모든 fixture 행을 통과했다. 내 프레임 시험이 빈 입자 배열을 `''` 로 직렬화해 fixture 의 `-` 와 달라 1건 실패했고, cpppj `Hex("-")`(빈 배열)과 같게 시험 쪽 비교를 고쳤다(모듈 변경 없음).

## 3. 경계와 남은 것

- `EvilRayVisualSystem`(Host/Kernel 부착·실제 입자 소유 월드·Stop 호출자)·실제 SHP 폭/음향 번호(현재 TYPE +70/+74)·위치 음향·타원 래스터 연결은 후속 단위다. 이 단위는 몸체와 요청까지만 옮겼다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "입자 표시 마무리": `RawSmokeEffect` → `RawBurningFrame` → `*VisualSystem` 8종·`RenderProcessSystem`.
