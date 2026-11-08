# webpj AI 배치 실행(RawAiDeployment)·자원 선택(RawAiResourceChoice) 이식 — WB04-E175

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백일흔다섯 번째 부분이다. AI 가 생산 조각을 소비하고 대기 항목을 예약해 실제로 건설을 요청하는 `o/RawAiDeployment`(패치 `004106e0` 배치 전체·`00410280` 비용 확인·`004102b0` 비용 지불)와, 간헐천 선택·신전 귀환·포획 운반자 목록·끊긴 간헐천 재분류를 같은 본체에서 처리하는 `o/RawAiResourceChoice`(패치 `00410a30`·`00410980`·`004109d0`·`004112b0`·`00411350`·`00411900`)를 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 두 파일의 .cpp/.h 와 시험의 변경 없음을 `git diff --numstat` 으로 확인). AI 계열(33개 파일) 열네 번째 단위이다.

## 1. 옮긴 것

`www/core/raw-ai-deployment.js`(DOM 없음)

- `new RawAiDeployment(memory, pool, types, create, globals, hooks)` — `create` 는 같은 풀의 `RawContainedCreate`(대기/예측 내용물 생성), `globals = { authority(), moveLocalBuilder({ value }), now() }`, `hooks = { product, technology, destroy, reserveRegular, addRegular, request, localPlace, demo, assertion }`.
- `canAfford(token, type)` — 원본 ftol(cost)의 signed 하위 DWORD 와 현재 지갑 비교. `charge(token, type)` — ftol(−cost) 감김 가산 뒤 현재 데모이면 signed 1190 이하를 1190 으로(SP 는 쓰지 않음). `place(token, type, x, y, argument, direction)` — 생산 조각이 있으면 삭제 → 같은 기술 부모에 대기 내용물(flags 0x108, 부착 0x10) 생성 → Regular 예약(성공 시 현재 +0x224 rate 로 부착, 실패해도 계속) → 비용 지불. 없으면 이동/운반(0x70000) TYPE 는 진단 뒤 현재 flags2 를 다시 읽고 시설(0x4200)만 비용 지불. 이어서 이동 플래그를 켜고 권한이면 10인자 서버 요청(소유자 = 현재 +0x260), 아니면 예측 내용물을 만들어 로컬 5인자 배치로 전달한 뒤 이동 플래그를 끄고 flags2 0x800/0x4200 시설은 현재 시각 + 20초 QWORD 기한(+0x5b8/+0x5c0)을 쓴다.

`www/core/raw-ai-resource-choice.js`

- `new RawAiResourceChoice(memory, pool, types, invalidGraph, hooks)` — `hooks = { center, graph, crystals, target, home, captureLog, disconnectedLog, assertion }`. `homeCenter` — 현재 신전이 있으면 center hook, 없으면 +0x268/+0x26c 원문 float(NaN/부호 0 은 `xBits/yBits` 로 보존). `select` — 객체까지 거리 제곱을 signed 로 절삭(그래프는 거리 탈락 후보에서도 조회)해 65536 보다 strict 하게 작고 제외/필수 그래프(무효 sentinel 이면 조건 없음)를 통과하는 첫 최소 객체. `collectOrHome` — 신전 없음/무효 그래프면 무동작, 크리스털이 없고 능력 0x10 이면 신전 중심/actor 그래프로 목록에서 대상을 골라 이동, 없으면 귀환.
- `hasFlyingCaptureCarrier`/`rememberCaptureCarrier` — 현재 TYPE 의 flags2 0x20000 만 검사, 중복은 로그도 생략하고 full 은 추가만 생략한 채 로그(기한 불변). `reclassifyDisconnected` — +0x330 의 다른 그래프 간헐천을 모든 중복까지 안정 제거해 +0x348 에 순서대로 옮기며(대상 full 이면 추가만 생략) SID 0/비간헐천은 진단 뒤 무효 그래프로 계속한다.

## 2. 검증

`tests/ai-deployment.test.mjs`(9건)·`tests/ai-resource-choice.test.mjs`(13건). 두 몸체 모두 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 손으로 쓴 입력/기대값으로 직접 단언한다). 같은 입력과 단언을 옮겼다: 배치는 서버 확정 순서, new 실패 계속, 생산 없음의 무료/시설 비용, 진단 뒤 flags2 재조회, 비권한 예측 조각과 로컬 5인자, signed 비용 절삭/감김/데모 재조회, 누락 하위 prefix(이동 플래그 미복구), 하위 효과 뒤 현재 입력. 자원 선택은 fallback 원문, 거리 절삭 동률/65536 경계, 그래프 필터와 현재 sentinel, 그래프 효과 뒤 목록 재조회, 무효 ftol, 수집 관문/이동/귀환 순서, 능력/sentinel 재조회, 포획 대기, 끊긴 간헐천 안정 제거와 SID 0 진단, 공용 내용물 총수(WORD mask/DWORD kind).

최초 실패와 교정: 새 모듈과 시험은 22건 모두 첫 실행에 통과했다(`homeCenter` 의 NaN 원문 비트 보존을 위해 `xBits/yBits` 를 반환값에 추가했다).

## 3. 경계와 남은 것

- 실제 생산 조회(`004103c0`)·기술 부모(`0048fc90`)·Regular 부착·서버 건설 요청(`00444760`)·로컬 예측(`004433b0`)·그래프(`00462bc0`)·내용물 총수(`004acc60`)·이동/귀환(E167 `RawAiMovement`)·로그 연결은 후속 AI 세션 연결 단위이다. `RawAiDropActions.deploy` hook 은 이 모듈의 `place` 에, `RawAiDropActions.canAfford`/`RawAiBuildChoice` 의 비용 확인은 `canAfford` 에 연결한다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiBridge*`(Planning·Strategies·Candidates·Search·Path 와 fixture 가 있는 계열)·`RawAiCollectors`·`RawAiFactoryProduction`·`RawAiCarrierActions`·`RawAiPreparation`·`RawAiStrategySetup`·`RawAiWorldRegistration`·`RawAiObjectNotice`·`RawAiDamage` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
