# webpj AI 본체 생성/소멸(RawAiBrain·RawAiCreate) 이식 — WB04-E164

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백예순네 번째 부분이다. AI 한 명의 실제 본체(0xb08 바이트)와 그 목록 16개·0xae4 블록의 소유 메모리, 패치 `00416860`(생성자)·`00410100`(소멸자), 그리고 `00417e50` AiCreate 의 호출 순서를 옮긴 `o/RawAiBrain`·`o/RawAiCreate`(cpppj 기준선 `1f00a25a`, 기준선 뒤 두 파일과 헤더·시험·fixture 의 변경 없음을 `git diff --numstat` 으로 확인)이다. AI 계열(33개 파일) 두 번째 단위이며, 이후 초기 갱신(`RawAiBrainInitialUpdate`)·런타임(`RawAiRuntime`)·전략 몸체가 이 본체 위에서 동작한다.

## 1. 옮긴 것

`www/core/raw-ai-brain.js`(DOM 없음)

- `RawAiBrainMemory(firstToken, fill)` — 원본의 32비트 포인터 필드를 호스트 포인터로 역참조하지 않는, 실제 소유 메모리의 opaque32 핸들 등록부이다. `allocate(size)` 는 8바이트 정렬 크기만큼 다음 핸들(비제로)을 올리고 방금 만든 블록에 미초기화 byte(`fill`)를 공급하며 순서/크기를 기록한다. `free(0)` 은 CRT `free` 처럼 무동작이고 없는 핸들 해제는 거부하며 해제한 블록은 다시 조회할 수 없다. 크기 0 과 주소 공간 소진은 예외(NULL 성공을 만들지 않는다). `bytes`/`contains`/`allocations`/`frees` 로 관찰한다.
- `RawAiBrainConstructor(memory, player, participation, islands, allies, types, globals, hooks)` — 현재 Player 9기록(1620바이트)·참여 처리기·섬 런타임(E161)·방향별 동맹 81 DWORD·TYPE flags2 표(앞 48개)가 모두 같은 실제 세계의 저장소(같은 PATCH 풀)여야 한다. `globals = { localOwner(), noRegion(), invalidGraph() }` 는 매 읽기 지점에서 현재 값을 다시 읽는다(BYTE 로 보관된 `00540410` 을 두 쓰기 위치마다 재조회). `hooks = { priest(owner)→SID, regionAt(x, y)→지역, requiredEnergy(type)→문자열 }` 는 도달한 경계만 요구한다.
  - `construct(owner, settings, bytes = 0xb08)` — 실제 0xb08 블록 할당 → `initialize`. 목록 생성 도중 실패하면 이미 만든 목록을 역순으로, 본체를 마지막으로 해제한다.
  - `initialize(token, owner, settings)` — 600바이트 설정 복사, −10000.0f 필드, 17개 소유 블록(0x284 의 40바이트, 0x2dc.. 의 15개 `capacity×4` 목록과 `capacity/count` 필드, 0xae4 의 120바이트), 고정 필드/double 0/고정 배열 255 DWORD 0, 현재 그래프 BYTE 를 0x298/0x2ac 에 두 번 읽어 쓰기, Player 첫 지역이 '없음'이면 사제 위치의 지역을 찾아 존재하는 섬이면 섬 owner 와 Player 첫 지역에 쓰기, flags2 `0x40800` 인 첫 `ss`(대소문자 무시, 종료 문자 앞 2글자) 요구 에너지 TYPE 저장, 설정 bit 0 이 꺼졌을 때 0x370/0x37c 0, 방향별 동맹(`setAlliance` → 현재 로컬 owner 방향이 켜져 있으면 `addAlly`)이다. 쓰기는 원본 DWORD 만 쓰며 미초기화 주변 바이트를 지우지 않는다.
  - `destruct(token)` — `00410100`: 0xae4 가 있으면 먼저 해제하고 16개 목록 포인터를 역순으로 해제한다(원본은 capacity 필드가 0 이어도 포인터를 읽는다). 본체는 아직 살아 있다. `freeBody(token)` — 소멸한 본체만 해제한다(`participation.removeBrain` 의 `destructBrain` → `freeBrain` 연결).
- `RawAiCreate(players, hooks)` — `00417e50`. 600바이트 설정 확인 → 이미 Native 가 반납한 owner 는 거부 → 색(+0x250) 이 0 이 아니면 `changeColor` → 기존 brain 이 있으면 그대로 반환(갱신 재호출 없음) → NoTemple(+0x244)이면 첫 지역의 신전을 `0x400000` 으로 삭제하고 0 반환 → 큰 생성자/초기 갱신 두 경계를 생성 전에 확인 → `construct` → `attachBrain` → `initialUpdate` 순서이다. brain 을 합성하지 않으며 두 경계가 없으면 flags 0x20 도 켜지 않는다.

## 2. 검증

`tests/ai-brain.test.mjs`(8건).

1. 실제 PE 관측 `ai-brain-x86.tsv` 160행 전부(고유 입력 160개): 행마다 원본 실행기와 같은 입력(할당 순서별 미초기화 byte 패턴·600바이트 설정·동맹 초기 7/0 표·섬 4기록·사제 좌표 5.75/6.25·TYPE 표)을 준비하고 `initialize` 후 본체 0xb08 전체, Player 9기록, 방향별 동맹 324바이트, 섬 3204바이트, 할당 "핸들,크기,Adler-32" 순서, `destruct` 후 해제 순서, 사제/지역/에너지 호출 trace 를 비교한다. 소멸 뒤에도 본체가 같은 바이트로 살아 있고 `freeBody` 가 마지막에 해제함을 확인한다.
2. cpppj 계약 시험 3건: 실제 생성 소유자의 참여 전달(18 할당/18 해제·flags 0x20 비소거)과 초기 갱신 누락의 선행 거부·요구 에너지 누락 시 17+1 모두 정리, 두 번의 현재 그래프 BYTE 재조회(0x298=254, 0x2ac=17, 두 번째 읽기 시점에 0x298/0x2a8 이 이미 쓰여 있음), 현재 그래프 조회기 누락의 연결 거부(할당 0).
3. 추가 계약(cpppj 시험에 없는 같은 모듈의 원본 분기): 메모리 핸들 정렬·NULL 해제·이중/미등록 해제·주소 공간 소진·첫 핸들 0, 생성자의 수명(새 본체만 초기화, 소멸 전 해제 금지, 크기/owner/설정 거부, 초기화 실패 시 본체 정리), `RawAiCreate` 의 색 변경/기존 brain/NoTemple/생성/부착/초기 갱신 순서와 Native 반납 owner 의 선행 거부, 필수 전역/같은 풀/크기 연결 거부.

최초 실패와 교정: 새 모듈과 시험은 8건 모두 첫 실행에 통과했다(160행 포함).

## 3. 경계와 남은 것

- `initialUpdate`(`00414710`, `RawAiBrainInitialUpdate`)·`RawAiRuntime`(`00413ae0` 상위 AI 틱/해제 소유자)·전략 몸체와 실제 Player 색 변경/신전 삭제의 가상 호출은 AI 계열 후속 단위이다. 이 모듈은 그 경계를 명시 hook 으로만 받는다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiRuntime`(실제 PE 관측 145행), `RawAiBrainInitialUpdate`, AI 가짜 부족(`RawAiDrySpots`·`RawAiFakeDrySpots`)·`RawAiBuildSearch`·`RawAiWallet`·`RawAiEconomy` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
