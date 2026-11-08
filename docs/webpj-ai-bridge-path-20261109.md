# webpj AI 다리 경로(RawAiBridgePath) 이식 — WB04-E177

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백일흔일곱 번째 부분이다. AI 다리 탐색의 한 본체를 초기화하고 상태 0..5 로 한 단계씩 진행하며, 연결점 조각 소비·다리 배치·시작점 선택·timeout 을 처리하는 `o/RawAiBridgePath`(패치 `0040fde0` Initialize·`0040fd30` Consume·`004167a0` Step·`004173b0` Extend·`004174c0` Advance)를 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 `RawAiBridgePath.cpp/.h`·`AiBridgePathTests.cpp`·관련 fixture 의 변경 없음을 `git diff --numstat` 으로 확인). AI 다리 계열(Candidates → Planning → Path → Strategies → Search)의 세 번째 단위이며, E176 에서 미뤄 둔 `RawAiBridgeCandidates` 조립 시험 두 건을 여기서 함께 옮겼다.

## 1. 옮긴 것

`www/core/raw-ai-bridge-path.js`(DOM 없음)

- `new RawAiBridgePath(memory, pool, finder, globals, hooks)` — `memory` 는 E164 `RawAiBrainMemory`(본체 0xb08 바이트), `pool` 은 `SidPool`(PATCH 풀), `finder` 는 `ContainedFinderState`(E169 이전 `RawContainedFinder` 공용 상태), `globals = { now(), mapLimit(), bridgeType(), logKind(), startMarker(), endMarker(), targetMarker() }` 는 현재 전역을 매번 읽는 getter 이고, `hooks = { initialize, prepare, search, start, deploy, marker, technology, refresh, destroy, log }` 는 아직 옮기지 않았거나 다른 모듈이 소유한 하위 호출이다. 호출 위치에 도달했을 때만 필요하며(`need`) 없으면 거부하고 그 앞의 효과는 취소하지 않는다.
- `initialize(token, source, target)` — 소스/목표 float 네 개를 ftol(하위 DWORD)로 +0x638/+0x63c/+0x640/+0x644 에 쓰고, +0x690/+0x694 를 0, +0x6a0/+0x6a4 를 `0x628` 사용 가능 수 기준값, +0x698=2, +0x69c=1, +0x6a8=0x10000, +0x6c0/+0x6bc 를 −10000 의 float 비트(0xc61c4000), +0x6c4/+0x6cc 를 0, +0x6c8 을 `field(0x628) − 1` 로 쓰고 항상 true 를 돌려준다. NaN/무한대/음수 0 도 원문 비트·ftol 낮은 DWORD 로 처리한다.
- `consume(token, parent, pattern)` — `RawContainedFinder` 로 부모의 내용물 중 `flags & 0x140 == 0x40`(pending 제외)이고 WORD 패턴이 `pattern` DWORD 와 같은 첫 조각을 찾아 삭제한 뒤, 삭제 효과 이후의 현재 +0x600 float 과 현재 `now` double 을 더해 +0x630 QWORD 에 쓴다. 패턴은 DWORD 전체 비교이므로 65536 은 WORD 0 으로 잘리지 않는다.
- `step(token, source, target)` — 상태(+0x5f0) 0..5 switch. 0: 초기화, 1: 준비(`prepare` 하위), 2: 검색 중, 3: 검색 → 상태 증가, 4: 시작, 5: 완료만 true 를 돌려주고 나머지는 한 단계 증가 또는 유지한다. `source` 는 갱신되는 `{x,y}` 객체이다.
- `extend(token, source, target)` — 상태 5 가 아니면 입력 시작점을 그대로 돌려주고 아무 하위 효과도 만들지 않는다. 상태 5 이면 상태를 0 으로 되돌린 뒤 결과 좌표가 strict 0..`mapLimit` 안일 때만 `deploy` → 현재 owner 로 `technology`(기술 조회) → 현재 owner/패턴으로 `consume` → 다시 현재 owner 로 `technology` → `refresh`(부모 5)하고, 디버그 표식은 현재 로그 종류가 활성일 때 `refresh` 이후의 현재 끝/목표 표식을 놓는다. 무효 결과(경계·NaN)는 상태 0 복귀만 남고 배치·기술 조회를 생략한다.
- `advance(token, target)` — 시작점(+0x3d0/+0x3d4)이 −10000 이면 `start` 하위에서 첫 시작점을 받아 저장하고 표식을 놓되 timeout(+0x3d8)은 바꾸지 않고, 없으면 성공을 돌려준다. 이미 시작점이 있으면 상태를 확인해 대기 중인 유효 연결점은 성공 timeout 1 을 쓰고, 무효 결과에서만 signed 감소와 실패/출력 관문(+0xb00 이 0 이 아닐 때 `"Bridge timeout\n"` 로그)을 적용한다.

## 2. 검증

`tests/ai-bridge-path.test.mjs`(18건). 실제 PE 관측 fixture 를 입력만 설치해 재생한다.

- `ai-bridge-initialize-x86.tsv` 144입력(두 x87 정밀도 288반환)의 전체 0xb08 바이트(소스/목표 NaN/무한대/음수 0 비트 포함, 채움 BYTE 는 호출 전 본체 값).
- `ai-bridge-step-x86.tsv` 600입력의 호출 뒤 상태·완료 반환·반환 소스 x/y 비트·첫 하위 진입. 각 행은 독립 본체이며 기대 상태를 재계산하지 않는다.
- cpppj 계약 14건: 실제 초기화 연결 Step, 소비(pending/다른 TYPE·패턴 건너뜀·첫 일치·DWORD 전체 비교·삭제 뒤 현재 rate/시각), Extend(대기 시 무배치·배치 뒤 현재 owner/패턴 재조회·무효 결과의 효과 생략·디버그 표식 순서), Advance(시작점 없음/첫 시작점/대기 점/timeout signed 감소와 출력 관문), 미연결 하위에서 앞선 효과 보존, 생성자 거부.
- E176 `RawAiBridgeCandidates` 와 조립하는 두 건: 초기화 + 예산 Step(예산 중단 동안 상태 2 유지 후 검색 3), 시작점 + Advance(시작점 없음은 기존 timeout 유지).

최초 실패와 교정: 새 모듈과 시험 18건은 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 전략 연결(`RawAiBridgeStrategies`)·탐색(`RawAiBridgeSearch`: 검색·도형·점수·정리)은 이 계열의 다음 단위이며 `search`/`deploy`/`technology`/`refresh`/`marker`/`log` hook 을 실제 모듈로 연결한다. 실제 월드 지도/프레임 코드/그래프 연결은 후속 AI 세션 연결 단위이다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiBridgeSearch`(fixture `ai-bridge-search`·`shape`·`score`·`clear` 계열)·`RawAiBridgeStrategies`(fixture `strategies`)·`RawAiCollectors`·`RawAiFactoryProduction`·`RawAiCarrierActions`·`RawAiPreparation`·`RawAiStrategySetup`·`RawAiWorldRegistration`·`RawAiObjectNotice`·`RawAiDamage` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
