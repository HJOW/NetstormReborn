# webpj 오벨리스크 보상·Deck 추첨(RawGemReward·RawDeckDraw) 이식 — WB04-E105

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백다섯 번째 부분이다. 오벨리스크(묻힌 보상)의 현재 QA 보상 또는 실제 가중 Deck 추첨(461f00 선택·4620c0 공개)과, 그 아래의 352바이트 Deck 공유 초기화·추가·열기/닫기·벽시각 난수 추첨(44f200~44f510)을 cpppj 기준선 `1f00a25a` 의 `o/RawGemReward`, `o/RawDeckDraw` 에서 옮겼다. 선행은 이미 이식된 `GameRandom`(`random.js`)·`SquidPostPopList`(`squid-postpop-state.js`)이다. `RawProductionDeck`(Player 소유 Deck 수명)는 `RawPlayerWorld` 선행이라 이번에 옮기지 않는다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-deck-draw.js` — `PRODUCTION_DECK_BYTES`, `DeckAssertion`, `RawDeckDraw(raw, global, hooks)`

- Deck 은 5바이트 항목(type, chance, power, 255, 선택 횟수) 최대 66개와 끝의 다섯 DWORD(+0x14c 개수·+0x150 합계·+0x154 남은 선택·+0x158 seed·+0x15c 열림)이다.
- `RawDeckDraw.initialize(raw)`(44f200): 다섯 DWORD 만 0 으로 만든다(항목과 미초기화 꼬리 보존). `RawDeckDraw.append(raw, type, chance, assertion)`(44f290): 같은 type 이 있으면 진단(`DuplicateType`)을 부르고도 계속 추가하며, 가득 차거나 개수가 음수이면 진단(`Capacity`) 뒤 거부한다. 새 항목은 power 1·BYTE 로 줄인 type/chance·255·선택 0 이고 type 이 255 초과/chance 가 BYTE 에 안 맞으면 진단(`TypeTruncated`/`ChanceTruncated`)을 부르며 합계에 chance(DWORD 비트)를 더하고 개수를 올린다.
- `open(seed)`(44f250): seed 가 0 이면 같은 게임 난수를 다시 한 번 소비(0..9999)해 seed 를 쓰고 열림 bit0 을 켠다. `close()`(44f280): 열림 bit0 만 지운다(다른 열림 비트와 seed 는 보존).
- `draw(power)`(44f510): 열려 있지 않으면 진단(0xe5), 현재 합계(power 가 양수인 항목의 chance 합)가 저장된 합계와 다르면 로그(`mismatch`) 뒤 합계를 다시 쓴다. 호출 시점 벽시각으로 만든 별도 난수(Open 에 저장한 seed 는 쓰지 않음)로 [0, 합계) 값을 뽑아 power 가 양수인 항목의 chance 를 차례로 빼며 음수가 되는 항목을 고르고 그 항목의 선택 횟수와 남은 선택 DWORD 를 올린 뒤 type 을 돌려준다(없으면 0). power 가 `{ value }` 이면 선택 항목이 상품 TYPE 일 때 0, 아니면 -1(선택 없음은 0)을 쓴다.
- `RawDeckDraw.wallSeed(seconds)`(4bcab0): 시각 × 1013 의 signed64 절삭 하위 DWORD `low` 로 `low×33 + (low>>4 부호 확장)`(DWORD 감김)이다. 비유한/범위 밖은 low 0.

`www/core/raw-gem-reward.js` — `GemRewardState`, `RawGemReward(pool, types, random, excluded, state, hooks)`

- `choose(unused = 5)`(461f00): 전달된 5 를 읽지 않고 현재 TYPE 표 전체로 스택 Deck 을 만든다(초기화 전 물리 미정 바이트는 `fill` 훅이 공급). 현재 TYPE 개수(signed, 반복 끝마다 다시 읽음)만큼 돌며 flags2 에 0x100(보상 후보)이 있고 제외 목록(BYTE 항목, 전체 TYPE 인자와 비교)에 없는 TYPE 의 현재 레벨로 가중치 0→20, 1→30, 2→50, 3→100 을 정해 Deck 에 추가한다(레벨 98/99 는 제외, 그 밖은 진단 0xf6 뒤 건너뜀). Deck 개수가 0 이 아니면 열기(seed 는 같은 게임 난수 0..9999)→추첨→닫기로 TYPE 을 골라 0 이 아니면 반환하고, 빈 Deck 이거나 실패이면 진단(0x104) 뒤 0 을 반환한다.
- `reveal(buried)`(4620c0): 오벨리스크 TYPE 이 현재 오벨리스크 타입이 아니거나 삭제 비트(bit0)이면 진단(0x10a)을 부르고 계속한다. 슬롯의 보상 TYPE(WORD +12)이 0 이거나 보상 후보가 아니면 `choose` 로 다시 고른다. 생성(`create(type, 0)`) 뒤 원 오벨리스크의 **현재** 좌표(+14/+18)로 배치(`pop`)하고 원 오벨리스크를 삭제(`destroy(buried, 0)`)한 뒤 생성 결과를 반환한다. 범위 밖 TYPE 은 읽기 전에 거부한다.
- 도달한 훅이 없으면 오류이다(`fill·deck·assertion·deckAssertion·create·pop·destroy`).

## 2. 검증

- `tests/gem-reward.test.mjs`(2건):
  1. `gem-reward-x86.tsv` 1250행(10.78 전용 단일 판본, 두 정밀도 열 동일). 입력 14 DWORD(연산 0 선택/1 공개/2 직접 Draw·TYPE 변이·제외 비트·seed·벽시각 비트·부모 TYPE/상태/보상 TYPE·진단 변이·직접 Deck 모양[항목 수/chance 불일치/가득 참/음수 개수 등]·seed/열림·optional power)로 결과(선택 TYPE/생성 SID/Draw TYPE 또는 범위 실패 `fault` 32행), 외부 효과 순서(I 진단/W 벽시각/M 합계 불일치 로그/C 생성/P 배치/D·E 삭제), 게임 난수 상태, 직접 Deck 352바이트 hex, 부모와 생성 슬롯 raw, power, TYPE 개수가 같다.
  2. 계약: 시각 seed 변환(0·NaN·무한대·범위 밖은 0, 1초는 `1013×33 + 63`), 열기/닫기 비트와 0 seed 의 난수 소비, 추가 진단(중복·chance 잘림·가득 참), Deck 크기·판본/TYPE 표 크기 거부, 미연결 훅.

최초 실패와 교정: 새 모듈은 위 fixture 1250행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 Player 소유 Deck 의 수명(`RawProductionDeck`: 할당·Player+68 부착·반납 증빙)은 `RawPlayerWorld` 이식 뒤에 한다. 오벨리스크 생성(4af530)·배치·삭제는 훅 경계이며 실제 연결은 해당 단위에서 한다.

## 4. 다음 시작점

`RawPathProcess`(RunFrame·도착·Halt; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체(`RawObjectConversion`·`RawForeignRitualReward`·`RawWorldGeyser` 등)이다.
