# webpj AI 다리 검색(RawAiBridgeSearch)·네 다리 전략(RawAiBridgeStrategies) 이식 — WB04-E178

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백일흔여덟 번째 부분이다. AI 다리 탐색의 실제 모양·점유·끝점 평가와 호출당 8개 후보 검색을 맡는 `o/RawAiBridgeSearch`(패치 `0040f400` 모양·`0040f880` 점유·`0040f910` 끝점 점수·`004144b0` 검색)와, 네 다리 전략의 공통 관문·목록 이동·현재 목표·확장 순서를 맡는 `o/RawAiBridgeStrategies`(패치 `00417bd0` 입구와 네 case)를 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 `RawAiBridgeSearch.cpp/.h`·`RawAiBridgeStrategies.cpp/.h`·두 시험·관련 fixture 의 변경 없음을 `git diff --numstat` 으로 확인). AI 다리 계열(Candidates → Planning → Path → Search → Strategies)의 마지막 두 단위이며 이 계열은 이 단위로 모두 옮겼다.

## 1. 옮긴 것

`www/core/raw-ai-bridge-search.js`(DOM 없음)

- `new RawAiBridgeSearch(memory, frames, spots, bridge, random)` — `memory` 는 E164 `RawAiBrainMemory`, `frames` 는 현재 TYPE 번호 순서의 프레임 코드 배열 표(각 항목은 `{ side, variant, number, flags }` 배열로 `CanonDecoder` 가 읽는 형태), `spots` 는 같은 65536 BYTE 점유 지도, `bridge` 는 005411a0 의 현재 전체 DWORD 번호 getter, `random` 은 `GameRandom` 과 같은 `next(limit)` 를 가진 같은 공용 난수이다. 별도 월드 사본/난수 스트림을 만들지 않는다.
- `shape(pattern, direction, raw)` — 실제 다리 패턴 26개를 (1, 1) 기준으로 `CanonDecoder` 로 펼쳐 8×8 BYTE grid 에 방향 글자를 쓰고(좌표 1..6 밖은 거부), 같은 칸의 J(북→남)/K(동→서) 끝점 중 이웃이 비어 있는 것만 12 BYTE(x, y, 방향)로 기록해 +0xb8 개수를 갱신한다. caller 의 grid/미사용 끝점 여백은 지우지 않는다(0xbc BYTE 전체).
- `clear(placement)` — 실제 decoder 순서의 모든 칸에서 y 절삭 → x 절삭 → flat spot(y×256+x)을 검사해 첫 점유 칸에서 즉시 false, 빈 decoder 는 true 이다(평면 65536 밖은 거부하되 x 가 255 를 넘으면 다음 행을 읽는다).
- `score(placement, targetX, targetY, shape, excluded, scratch)` — 연결에 쓴 끝점을 제외하고 끝점마다 X 는 배정도로 유지·Y 만 float 로 먼저 저장해 두 축 절댓값 합(INT_MIN 감김)을 구하고 방향이 다르면 금지(65536)/벌점(+4, 최소 6)을 적용하며 strict 최저점에서만 임시 XY 를 float 로 갱신한다(NaN 비트 보존용 `xBits/yBits` 는 새 값을 쓸 때 지움).
- `search(token, scratch)` — 호출당 후보×재고×회전 8개를 처리하고 예산이 소진되면 false(+0x6c4/+0x6c8/+0x6cc 진행 유지), 네 회전이 끝나면 true 이다. 같은 모양의 모든 일치 끝점을 검사하고 strict 최저 또는 동점의 난수(100 중 50 미만)에서 +0x6a8..+0x6c0 결과를 갱신한다.

`www/core/raw-ai-bridge-strategies.js`

- `new RawAiBridgeStrategies(memory, pool, types, random, globals, hooks)` — `types` 는 현재 188 TYPE 의 `{ flags2 }` 표, `globals = { now(), mapLimit(), invalidGraph(), checking(), disabled(), geyserLog() }` 는 각 원본 조회 위치에서 현재 값을 읽는 getter(`now`·`mapLimit`·`geyserLog` 는 생성자에서, `invalidGraph`·`checking`·`disabled` 는 도달할 때만 요구), `hooks = { technology, refresh, graph, center, connected, delay, home, reclassify, select, randomStart, advance, extend, rectangle, assertion, log }` 는 이미 옮긴 실제 하위와 장치 경계로 도달한 위치에서만 요구한다(없으면 그 위치에서 거부하고 앞선 효과는 취소하지 않음). 간헐천 로그 사건은 `AI_BRIDGE_STRATEGY_LOG { requeued: 0, abandoned: 1, trying: 2 }` 로 `log` 하위에 전달한다.
- `update(token)` — 공통 관문(신전 +0x294·graph +0x298 이 무효 graph 와 다름·+0x248 의 0x1000 비트·checking 이면서 disabled 가 아님)을 통과하면 현재 owner 의 기술 부모로 재고를 보충한 뒤 현재 목표(+0x2d4)의 1..4 를 정확히 한 번 처리한다. 1 신전(기한 +0x5a8, 살아 있는 다른 graph 의 신전으로 진행하거나 대상을 +0x2a8 로 되돌림), 2 간헐천(연결 판정 → 미연결 목록 안정 압축 제거 + 연결 목록 추가, 재시도 기한이 되면 포기 목록 첫 항목 재등록, 연결되지 않은 유효 대상은 중심을 +0.99999f 거의 올림해 진행/포기/새 대상 선택), 3 확장(기한 +0x5b0, 난수로 고른 주변 3칸 경계 또는 자기 영역에서 무작위 시작점 → ±8 무작위 목표로 연장, 결과가 시작점과 다를 때만 잔여 횟수 감소), 4 좌표(명시 좌표 +0xaf0/+0xaf4 가 유효하지 않으면 진단 뒤 inclusive 3×3 빈 사각형과 진행이 성공하면 유지, 아니면 −10000 으로 지움). 목록은 원본 네이티브 0040e9b0 처럼 가득 차면 추가를 생략하고 중복을 유지하며, 제거는 안정 압축하되 미사용 꼬리를 보존한다.

## 2. 검증

`tests/ai-bridge-search.test.mjs`(8건)·`tests/ai-bridge-strategies.test.mjs`(7건). 실제 PE 관측 fixture 를 입력만 설치해 재생한다.

- `ai-bridge-shape-x86.tsv` 208행(26개 패턴×4회전, 호출 전 0xbc 채움 BYTE 에서 grid/끝점 여백을 지우지 않는 전체 버퍼), `ai-bridge-clear-x86.tsv` 416행(실제 decoder 순서·비대칭 점유 지도·소수 좌표), `ai-bridge-score-x86.tsv` 640행(x87 의 X 보존/Y float 저장·signed 감김·금지/벌점·미갱신 결과), `ai-bridge-search-x86.tsv` 72장면×4호출(288, 본체 0xb08/공용 난수/임시 XY/완료값 전부), `ai-bridge-strategies-x86.tsv` 198행(네 case 전체/공통 관문/NaN 기한·좌표/재고 뒤 목표 재조회/중복 목록/기한/좌표를 한 번의 `update` 로 호출해 본체·네 목록의 미사용 꼬리·난수·하위 호출 순서 전부).
- cpppj 계약: E176 후보·E177 경로와 함께 실제 초기화 → 24칸 후보 → 8개 검색 → 대기 → 결과 반환을 수행하는 조립 시험(예산 중단 동안 상태 3 을 유지한 뒤에만 완료), 현재 TYPE/패턴/모양 범위 거부와 가짜 본체/모양 크기 거부, 방향 번호 8 이상 거부, checking 이 거짓이면 disabled 를 읽지 않고 재고 효과 뒤 현재 목표/좌표를 다시 씀, graph 하위가 대상을 바꾸면 변경 뒤 현재 SID 의 중심을 다시 읽음, 연장 결과가 시작점과 같으면 잔여 횟수 유지, 기한 NaN 이면 네 전략 모두 재고 갱신까지만 수행, 미연결 하위/생성자 거부.

최초 실패와 교정: 새 모듈과 시험 15건은 모두 첫 실행에 통과했다(fixture 5개 합계 1,534행, 검색 72장면은 288호출). 보고된 작은 정정은 시험의 점유 범위 시험이 `throws(a && b)` 로 합쳐져 있던 것을 x 평면 줄 넘김(참)과 y 300 거부로 나눈 것뿐이다.

## 3. 경계와 남은 것

- 실제 월드/그래프/신전/시각/난수 세션 연결(전략 하위 `graph`·`center`·`connected`·`home`·`select` 등을 세션 소유 모듈로 연결)은 후속 AI 세션 연결 단위이다. 이 계열의 후보·계획·경로·검색·전략 모듈은 모두 hook 경계로만 연결되어 있다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiCollectors`·`RawAiFactoryProduction`·`RawAiCarrierActions`·`RawAiPreparation`·`RawAiStrategySetup`·`RawAiWorldRegistration`·`RawAiObjectNotice`·`RawAiDamage` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
