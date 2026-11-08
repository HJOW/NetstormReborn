# webpj AI 런타임(RawAiRuntime) 이식 — WB04-E165

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백예순다섯 번째 부분이다. AI 한 명의 전체 행동 1회(`00417c70`)·개별 Tick(`00417d30`)·Player 1..8 자동 방문과 종료한 brain 의 소멸/반납(`00417de0`)을 옮긴 `o/RawAiRuntime`(cpppj 기준선 `1f00a25a`, 기준선 뒤 두 파일·시험·fixture 의 변경 없음을 `git diff --numstat` 으로 확인)이다. AI 계열(33개 파일) 세 번째 단위이며, 앞 단위(E164)의 `RawAiBrainMemory` 위에서 동작한다.

## 1. 옮긴 것

`www/core/raw-ai-runtime.js`(DOM 없음)

- `new RawAiRuntime(memory, players, globals, hooks)` — `memory` 는 E164 의 `RawAiBrainMemory`(실제 0xb08 본체의 유일한 소유자), `players` 는 실제 9×180 BYTE Player 기록(+0xb0 이 현재 brain 핸들)이며 복사하지 않고 빌린다. `globals = { battle(), timing(), dryLog() }` 는 각 원본 읽기 위치에서 현재 값을 다시 읽는 getter 이다(`battle` 은 server/boss 와 합치지 않은 현재 전투 모드). `hooks` 는 이 순서의 원본 하위(초기화·준비·목표 선택·수집자·시설·부족 좌표·알림·사제·배치·다리·운반자·경제·SP·벽시각·느린 프레임 로그·진단·소멸·해제)를 명시 함수로 받으며 도달한 위치에서만 요구하고 없으면 그 위치에서 오류이다.
- `actions(token)` — 살아 있는 정확한 0xb08 본체 확인 → 수집자 → 시설 → 부족 좌표(제거 TYPE 이 있으면 현재 로그 종류와 `제거 TYPE === 현재 +0x280` 비교를 알림) → 사제(현재 +0x29c 전체 DWORD) → 배치 → 다리 → 운반자 → 경제 → 지갑 정수(+0x2d8, int32)를 **float 으로 먼저 저장해** 현재 SP float 와 비교한다. 다르거나 NaN 이면 진단 `money == getMoney( playerId )`(줄 0x1048)을 보고하고 원본처럼 계속한다. 본체 필드는 각 원본 위치에서 다시 읽는다(앞선 hook 이 바꾼 값이 반영된다).
- `tick(token)` — +0x254 가 비제로면 false 만 반환한다. +0x258 이 0 이면 초기화 후 다시 읽어 여전히 0 이면 본체를 유지한 채 true 이다. 시작 벽시각 → 준비 → 목표 선택 → 행동 뒤 **현재** timing 전역이 비제로일 때만 끝 벽시각을 읽어 경과가 0.1 초(`00501960`, double)를 엄격히 넘을 때 느린 프레임을 알린다(행동이 timing 을 바꾸면 그 새 값을 따른다).
- `frame()` — 현재 전투일 때만 Player 1..8(0 번 중립 제외)의 현재 brain 을 방문한다. tick 이 false 이면 그 시점에 현재 +0xb0 을 다시 읽어 있으면 소멸 → 반납하고, 어느 경우든 +0xb0 만 0 으로 쓴다(flags 0x20 등 Player 의 다른 필드는 보존).

## 2. 검증

`tests/ai-runtime.test.mjs`(6건).

1. 실제 PE 관측 `ai-runtime-x86.tsv` 145행 전부(actions 18·tick 94·frame 33): 행마다 원본 실행기와 같은 입력(명시 하위 입력 10개 QWORD·본체 두 개·Player 9기록)을 설치하고 반복 호출한 뒤 정규화 본체(반납은 부재 0), Player 9기록, 해제 mask, Tick 반환, 도달한 하위와 인자/순서 trace(벽시각 호출 색인·double 경과 비트 포함)를 모두 비교한다.
2. cpppj 계약: 254 종료는 초기화/시계/행동 없이 현재 Player 본체 소멸 → 반납 → 0 쓰기만 수행하고 Player flags 와 0 번 Player 는 보존.
3. 추가 계약(같은 원본 분기): 본체 검증이 어떤 효과보다 먼저(해제/가짜/잘못된 크기), 누락 하위의 도달 위치 거부와 앞선 효과 보존(부족 좌표/느린 프레임 알림), timing 전역이 켜져 있을 때만 시계 재호출·경과 0.1 초 **엄격 초과**(0.1 은 알리지 않음), 비전투 프레임 무동작, 초기화 미완료 Tick 의 본체 유지, 생성자의 9 Player/소유자/현재 전역 getter 거부.

최초 실패와 교정: 새 모듈과 시험은 6건 모두 첫 실행에 통과했다(145행 포함).

## 3. 경계와 남은 것

- 이 모듈이 부르는 하위 몸체(초기화 `00414af0`·준비 `00416eb0`·목표 선택 `004139f0`·수집자/시설·부족 좌표·사제·배치·다리·운반자·경제)는 AI 계열의 후속 단위이며 지금은 명시 hook 으로만 연결된다. 느린 프레임 로그 장치(29 번)·벽시각(`00460d70`)·SP 조회(`0040ef30`)도 실제 세션 연결이 필요하다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiBrainInitialUpdate`(`00414710` 초기 갱신), AI 가짜 부족(`RawAiDrySpots`·`RawAiFakeDrySpots`)·`RawAiBuildSearch`·`RawAiWallet`·`RawAiEconomy` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
