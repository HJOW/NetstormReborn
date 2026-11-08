# webpj 불꽃 꼬리 프레임(RawFieryTailFrame)·수명(RawFieryTailLifetime) 이식 — WB04-E187

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백여든일곱 번째 부분이다. 원본 `Fierytail.cpp` 의 11 불꽃과 12 건설 마나(Streak)가 같이 쓰는 입자 프레임·선 출력을 맡는 `o/RawFieryTailFrame`(패치 `004580f0` 프레임·`004583a0` 출력·`00487b40` 입자 추가·`00487e50` 색 조회)과, 두 생성자(`00457ea0` 불꽃·`00458680` 건설 마나)와 `004882c0` 공유 소멸 순서를 맡는 `o/RawFieryTailLifetime` 을 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 두 몸체와 시험/fixture 의 변경 없음을 `git diff --numstat` 으로 확인). E186 이 먼저 옮겨 둔 `buildFieryTailPalette`(불꽃 11색 표)와 E129 `RawRenderRegistry` 위에 얹어 실제 PE 관측 fixture 세 개(65+31+4 입력)를 재생한다.

## 1. 옮긴 것

`www/core/raw-fiery-tail-frame.js`(DOM 없음)

- `createFieryTailFrameFields()`·`createFieryTailParticle()` — 효과 객체의 의미 필드(`registration`·용량 전체 `particles`·사용 꼬리 `total`·`lastTime`/`elapsed`·`delta`/`scaledDelta`·`first`·`palette`·`maximumLife`(기본 4)·`previousX/Y`·`offset`)와 24바이트 입자 `{ x, y, velocityX, velocityY, life, spare }` 이다. 속도/마지막 DWORD 는 프레임이 바꾸지 않고 보존한다.
- `new RawFieryTailFrame(pool, random, state, palette, hooks)` — `state = { now, pixelScaleX, pixelScaleY, rate, cameraX, cameraY, diagnostic }`, `palette = { count, readable }`(색수와 읽을 수 있는 DWORD 목록, 진단 2 가 count 를 바꾸므로 호출자와 같은 객체를 공유), `hooks = { invalidate, line }` 은 모두 필수이다.
- `step(fields)` — 최초에는 영역·직전 좌표를 정하고 10입자(난수 y → x 순, offset 미적용)만 만들고 dirty/수명 감소는 없다. 이후에는 시간 조각(최대 0.05초 = 0.5×0.1, 시간 후퇴/0 간격도 원문 그대로, float 저장 순서 보존)마다 ① 2입자 방출(offset 적용, 빈 슬롯이 없어도 난수는 두 번 소비) ② 용량 전체(사용 꼬리가 아님)의 양수 수명 입자만 수명 감소와 ±10 영역 누적(죽는 조각에도 영역 포함) ③ 이전/새 영역 dirty(카메라 감산과 1픽셀 여유를 중간 float 저장 없이 `__ftol`)를 수행한다. 입자 추가는 사용 꼬리를 먼저 쓰고 꽉 찬 뒤에만 앞에서 수명 ≤ 0 인 슬롯을 찾으며 NaN 수명은 재사용하지 않는다.
- `draw(fields, endpointX, endpointY)` — 사용 꼬리까지 양수 수명 입자마다 부모 화면 끝점 + offset(32비트 감김)까지의 선 요청을 낸다. 선 색은 `float(색수) × min(수명, 최대 수명) × float(1/최대 수명)` 을 0 방향 절삭(음수는 0)한 번호의 색 표 DWORD 이며 색수 다음 인접 DWORD 도 공급된 것만 읽고, 공급되지 않았거나 분모가 비정상이면 가짜 색 없이 거부한다. 진단 2 는 색수 0x17655·최대 수명 0x315a 로 바꾼다.

`www/core/raw-fiery-tail-lifetime.js`

- `FieryTailVariant`(`Fiery`: 수명 4·offset 0·깊이 −25·process 11 / `Streak`: 수명 2·offset −8·깊이 −20·process 12)·`FieryTailRelease`(`Particles`·`Object`)·`new RawFieryTailLifetime(pool, registry, state, hooks, variant)` — `state = { palette, sharedPalette?, sharedProcessType? }`. `sharedPalette` 는 `{ value }` 상자이며 건설 마나가 빔과 같은 0055a758 owner 를 빌릴 때 그 값을 직접 읽고 쓴다(없으면 자체 `palette`). `sharedProcessType` 은 등록 전에 읽을 현재 0054109c/a0 값을 돌려주는 함수이다.
- `initialize(fields, token, parent)` — 현재 TYPE 를 등록/확보 콜백보다 먼저 읽어 공통 등록/부착 → 입자 비움 → 2880바이트 확보(120입자 zero) → 첫 공유 색 표(8바이트 확보 → 생성 → 실제 8+3색 구성) → 수명/palette/offset/깊이 순서로 쓰며 원본이 쓰지 않는 `delta`/`scaledDelta` 는 보존한다.
- `destroy(fields, flags, release)` — 입자 반납 → 공통 등록 제거/base → flags bit 0 이면 객체 반납 순서이며 공유 색 표는 반납하지 않는다.

## 2. 검증

`tests/fiery-tail-frame.test.mjs`(3건)·`tests/fiery-tail-lifetime.test.mjs`(5건).

- `fierytail-frame-x86.tsv` 기본 027f 65입력: 호출 전 68바이트 의미 상태·용량 전체 입자·부모 raw 50바이트·전역 40바이트·색 표 260바이트(색수 + 인접 DWORD 64개)를 외부 입력으로 복원해 `step` 뒤 `draw` 를 실행하고, 호출 뒤 의미 상태·용량 전체 입자·색수·난수 상태·dirty 요청·선 요청·부모 불변을 허용오차 없이 비트 단위로 대조한다(비교 정밀도 037f 65행은 중복 가산하지 않음).
- `fierytail-lifetime-x86.tsv` 기본 027f 31입력: 생성 직후 의미 88바이트·120입자 zero·공유 색 표 11개(`buildFieryTailPalette`)·표시 슬롯·확보/부착 요청 순서와, 반납 뒤 의미 상태·슬롯·순서(입자 → 등록 제거 → 객체)를 대조한다(warm/cold 색 표·표시 목록 구멍/확장·부착 중 깊이 변경/현재 부모 변경 재진입 포함).
- `streak-lifetime-x86.tsv` 기본 027f 4입력: 건설 마나 생성자의 의미 88바이트(수명 2·offset −8·깊이 −20)와 빔 공유 색 표만 빌리고 자체 palette(999)는 건드리지 않음을 대조한다.
- cpppj 계약: 인접 색 DWORD 미공급 거부, 현재 TYPE 전역을 등록/확보 콜백 전에 읽고 생성마다 다시 읽음, 두 TYPE 전역(`GameAuxState.fieryTailProcessType/streakProcessType` 초기값 11/12)이 일반 월드 준비 뒤에도 보존됨, 경계 미연결/다른 풀/첫 공유 색 경계 누락/반납 경계 누락 거부, 비어 있는 공유 상자에 직접 생성.

최초 실패와 교정: 모듈은 모든 fixture(65+31+4입력)를 첫 실행에 통과했고 내 시험에도 교정이 필요한 실패가 없었다.

## 3. 경계와 남은 것

- `FieryTailVisualSystem`(Host/Kernel 부착, 실제 입자 소유 월드, `fierytail-pixels-x86.tsv` 60행 소비)·실제 색 표(`buildFieryTailPalette` 호출 위치)/Surface/선 래스터 연결은 후속 단위이다. 이 단위는 몸체와 출력 요청까지만 옮겼다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`FieryTailVisualSystem`·`LightningVisualSystem`·`RawBurningFrame`(캡처 자산 필요)·`BurningVisualSystem`·`RenderProcessSystem`, `RawFlyerProcess`→`RawWindFlyer`, `RawPacketDispatch`→`RawConstructionCommand`, Missile Host 팩토리, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
