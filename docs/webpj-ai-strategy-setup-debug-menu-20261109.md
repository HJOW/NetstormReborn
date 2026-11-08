# webpj AI 전략 준비(RawAiStrategySetup)·AI 디버그 메뉴(RawAiDebugMenu) 이식 — WB04-E182

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백여든두 번째 부분이다. AI 의 첫 영역·다리·생산 준비와 전략 목표 선택을 맡는 `o/RawAiStrategySetup`(패치 `00414af0` 초기화·`004139f0` 목표 선택·`00410360`/`004103c0` 생산 조회·`00410420` 생산 등록·`0040fc60` 다리 시드·`0040fcb0` 다리 보충·`0040fb30`/`0040fbd0` 다리 추가/스캔)와, 선택적 AI 디버그 메뉴를 현재 Player/brain 과 controlled 메뉴 작성기에 연결하는 `client/RawAiDebugMenu`(원문 `413dd0`)를 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 두 몸체와 시험의 변경 없음을 `git diff --numstat` 으로 확인). AI 계열(E164 이후)의 마지막 몸체이며, 전략 준비는 독립 원본 관측 fixture 가 없는 cpppj 계약 단위이고 디버그 메뉴는 실제 PE 관측 60행을 재생한다.

## 1. 옮긴 것

`www/core/raw-ai-strategy-setup.js`(DOM 없음)

- `new RawAiStrategySetup(memory, player, preparation, finder, create, finderState, types, random, allies, globals, hooks)` — `player` 는 `RawPlayerWorld`(현재 Player 9기록 `records` 와 `technology(owner)` 기술 부모), `preparation` 은 E181 `RawAiPreparation`(공유 `00410300` 적 선택), `finder` 는 `RawCarrierPreDestroy`(`findContained` = 공유 `004ac9f0`), `create` 는 `RawContainedCreate`(공유 `004afa70`), `finderState` 는 `ContainedFinderState`, `types` 는 신전의 현재 BYTE TYPE 에서 `theme` 을 읽는 TYPE 표, `random` 은 같은 게임 공용 난수, `allies` 는 방향별 81 DWORD, `globals = { localOwner(), noRegion(), templeProduct(), bridgeType(), mapLimit(), now() }`, `hooks = { regionBounds, temple, destroy, assertion, demo }` 는 도달한 위치에서만 요구한다. 같은 PATCH 풀(player/preparation/finder/create)과 Player 9기록/동맹 81/TYPE 표를 생성 때 확인한다.
- `initialize(token)` — 적(+0x264)이 없거나 방향별 동맹이면 적을 재선택하고, 적/자기/적의 시작 지역이 준비되지 않았으면 아무것도 쓰지 않고 재시도를 허용한다. 준비되면 자기 청크 경계(+0x270..+0x27c)와 중심(+0x268/+0x26c, x87 배정도 중간 합 뒤 float)을 쓰고 기술 부모에 다리 재고를 시드(+0x238 개수)하며 현재 신전에 첫 신전 생산이 없으면 생산을 등록하고 신전 theme(+0x390)을 쓴 뒤 완료 표식(+0x258)을 쓴다(최초 월드 스캔 표식 +0x25c 는 별도).
- `selectGoal(token)` — 목표 1(경제 충족·지갑 > 비용+1200(signed)·공격 능력 0x4·적 그래프가 다름)/2(간헐천 미부착·능력 0x10)/4(지도 안 strict 특정 좌표)/3 으로 고르고, 목표가 바뀔 때만 대상/시작점/기한/탐색 상태 여섯 필드를 초기화한다.
- `findProduct`(mask 8 기술 부모 조회, 두 원본 함수는 실제 PE 에서 같음)·`addProduct`(중복 진단을 계속 진행시킨 뒤 원격 AI 는 현재 기술 부모 생산을 삭제·재등록하고 신전 생산 등록)·`seedBridges`/`refreshBridges`(기존 다리 스캔과 부족분 생성, 직선 강제도 매번 난수를 소비하고 serial 4배수마다 직선, 신전/기한/로컬 데모 관문 뒤 한 조각만 보충, 8칸 재고 초과는 물리 계약 오류).

`www/core/raw-ai-debug-menu.js`

- `openAiDebugMenu(owner, reads, hooks)` — `reads = { checking, player(owner), body(token), debug(address), command(address) }`(현재 물리/전역 조회), `hooks = { format, header, checked, begin, take, submenu }`(메뉴 작성 경계). checking 이 0·owner 가 signed 비양수·현재 Player+0xb0 brain 이 0 이면 효과 없이 돌아가고 owner 9 이상은 거부한다. 그 뒤 원문 순서로 초기화되지 않았으면 "NOT INITIALIZED" 머리, 돈/목표(표 밖 거부) 머리, "AI Reporting" 체크, 진단 7행(현재 로그 값 == 29), "AI Debug" Submenu, 파라미터 다섯 묶음과 "AI Parameters" Submenu 를 작성한다. 처음 계산한 Player 행을 callback 동안 보관하고 효과 뒤에도 같은 행의 현재 +0xb0 만 다시 읽으며, 파라미터 단계는 "AI Debug" 작성 뒤 한 번 읽은 brain 을 보관한다. 부분 실패는 되돌리지 않는다.

## 2. 검증

`tests/ai-strategy-setup.test.mjs`(11건)·`tests/ai-debug-menu.test.mjs`(3건).

- 전략 준비(cpppj 계약, 독립 fixture 없음): 지역이 없을 때 하위나 +0x258/+0x25c 를 건드리지 않고 재시도, 자기 경계 → 다리 → 현재 신전 → 첫 생산 → +0x258 의 실제 순서와 QWORD 기한, 원격 AI 의 mask 8 기술 부모와 mask 10 신전 생산과 기존 생산이 있으면 생략, 보류 중인 다리는 총수에만 포함, 직선 강제의 난수 소비와 serial signed 감김, 중복 진단 뒤 원격 내용물 삭제와 새 생산, signed 비용+1200 경계와 목표 우선순위 전수 조합, 같은 목표에서는 본체 보존·변경 때만 여섯 필드 초기화와 엄격한 좌표 경계, 신전 없음/충족/기한 전/데모의 한 번에 하나만 보충, 원격 기한 전 데모 미호출과 unordered 시각 대기, 생성자 거부.
- 디버그 메뉴: `ai-debug-menu-x86.tsv` 60행(원문 `413dd0`)을 입력만 설치해 재생한다. 열 = 입력 20개(checking·owner·present·mutation·필드 9개·진단 로그 전역 7개)와 원문 메뉴 효과 trace(머리/서식/체크/begin/take/submenu 순서와 전체 인자)이며, 효과 callback 이 Player+0xb0 또는 보관 본문을 바꾸는 독립 재진입 입력(mutation)에서의 현재 brain 재조회/보관 본문 수명과 단락 뒤 Player 조회 횟수를 함께 대조한다. cpppj 계약(첫 효과 중단 뒤 후속 메뉴 비합성·초기 단락의 미연결 GUI 비요구·목표 배열 밖 거부)과 owner 9 이상/미연결 하위/해제 brain 의 거부를 추가했다. cpppj 의 `AiDebugInput_*` 시험(명령 입력/이름/Client 전역)은 `ai-debug-command-input.js`(WB04-E27) 등 다른 몸체의 검사 범위라 옮기지 않았다.

최초 실패와 교정: 새 모듈과 시험 14건은 모두 첫 실행에 통과했다(디버그 메뉴 60행 포함).

## 3. 경계와 남은 것

- 실제 청크 경계(`00432210`)·신전 조회(`0046f260`)·삭제 가상 호출·원본 진단·데모 판정(`0040fcb0`)과 메뉴 작성기(`4b9390` 서식·`477bb0`/`475c00` 스택·Submenu)·현재 Player/brain 물리 연결은 후속 AI 세션 연결 단위이다. 두 몸체는 모두 hook 경계로만 연결되어 있다.
- AI 계열(E164~E182)의 모든 `o/RawAi*` 몸체와 `client/RawAiDebugMenu` 가 옮겨졌다. 실제 세션 조립(월드/그래프/신전/시각/난수 연결, `RawGeyserAiRegistration.bindPatch` 에 `RawAiWorldRegistration.geyser` 연결 등)은 후속 연결 단위이다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
