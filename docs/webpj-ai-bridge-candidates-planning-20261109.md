# webpj AI 다리 후보(RawAiBridgeCandidates)·다리 계획(RawAiBridgePlanning) 이식 — WB04-E176

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백일흔여섯 번째 부분이다. AI 다리 탐색의 나선 24칸과 연결점 후보 6개를 준비하고 시작점을 고르는 `o/RawAiBridgeCandidates`(패치 `0040fe90` 후보 준비·`0040faa0` 열린 이웃·`00410b10` 시작점)와, 네 다리 전략의 연결 판정·무작위 시작점·빈 사각형·재시도 기한을 옮긴 `o/RawAiBridgePlanning`(패치 `00410d40`·`00411310`·`00411470`·`004119b0`)을 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 `RawAiBridge*` 전 파일의 변경 없음을 `git diff --numstat` 으로 확인). AI 다리 계열(Candidates → Planning → Strategies → Search → Path) 첫 두 단위이며, 처음으로 AI 계열에서 실제 PE 관측 fixture 6개(1,007행)를 재생한다.

## 1. 옮긴 것

`www/core/raw-ai-bridge-candidates.js`(DOM 없음)

- `new RawAiBridgeCandidates(memory, pool, types, frames, surfaces, spots, globals)` — `surfaces` 는 같은 hash0 의 65536 WORD(Uint16Array), `spots` 는 같은 점유 65536 BYTE, `frames` 는 현재 188 프레임 코드 표(`FrameCodeTable`), `globals = { bridge(), fencemark(), windArcher() }`(현재 다리 TYPE 의 전체 DWORD/graph getter 의 두 예외 BYTE).
- `prepare(token)` — 한 호출에 나선 24칸을 처리하고 예산이 소진되면 false(상태 유지), 경계(10..246 밖)/나선 24단계/단계 8 이상 + 후보 3개 이상이면 true. 같은 graph/owner 표면에서 4방향 이웃 중 빈 칸이 있고 북/동/남/서 연결비트(원본 `0052f910` A~P 표)가 다리 여부와 일치하며 그 이웃이 비어 있는 방향마다 최대 6개 후보(+0x648, 12바이트)를 저장하고(count 증가 선행·DWORD 주소 감김), 나선 회전은 signed 나머지, 변 길이는 signed 산술 시프트로 진행한다.
- `open(surface, x, y)` — 현재 코드의 연결/다리 구분에 맞는 빈 이웃이 있으면 같은 SID, 없으면 0. `start(token, target, bounds)` — 사각형을 X 바깥/Y 안쪽 순서로 훑어 현재 본체 graph/owner 가 같고 열린 표면 중 목표까지 두 축 절댓값 합(INT_MIN 감김 포함)이 엄격히 최소인 첫 칸을 고르며 없으면 `{ -10000, -10000 }`(지도 경계를 clamp 하지 않음). 원본의 friend `RawAiBridgePlanning` 에 대응해 `pool/surfaces/frames/bridgeType/field/setField/body/spot/graph/connections` 를 협력자 접근으로 제공한다(사본 없음).

`www/core/raw-ai-bridge-planning.js`

- `new RawAiBridgePlanning(candidates, random, now, graphFor)` — `graphFor` 는 같은 현재 `00462bc0`(E57 `RawPlayerPlacementAnchor.graphFor`)이며 필요한 연결 판정에서만 호출한다.
- `connected(token, geyser)` — graph 가 본체 +0x298 과 같을 때만(하위 뒤 본체와 실제 목표 좌표를 다시 읽음) 좌표의 가장자리 12칸에서 자기 소유 J(북/남)/K(서/동) 다리를 확인. `delayRetry(token)` — +0x350 count 전체 DWORD 가 0 이면 40초, 아니면 120초를 현재 QWORD 시각에 더해 +0x5d0 에 쓴다. `randomStart(token, bounds)` — 같은 graph/owner 로 고정하고 경계를 bottom → top → right → left 로 절삭해 최대 20번 y → x 공용 난수를 소비하며 첫 열린 지면을 선택(없으면 −10000 쌍). `rectangleFree(left, top, right, bottom)` — inclusive 사각형의 flat spot BYTE 가 모두 0 인지 X 바깥/Y 안쪽 순서로 검사.

## 2. 검증

`tests/ai-bridge-candidates.test.mjs`(4건)·`tests/ai-bridge-planning.test.mjs`(6건). 실제 PE 관측 fixture 를 입력만 설치해 재생한다.

- `ai-bridge-candidates-x86.tsv` 210장면/220반환(24칸 예산·최대 6개·연결비트·재개·전체 본체 0xb08 바이트), `ai-bridge-start-x86.tsv` 399행(같은 거리 동률/분수 경계/현재 전체 DWORD 비교와 본체 불변), `ai-bridge-planning-random-x86.tsv` 261행(XY/공용 난수/본체 불변), `ai-bridge-planning-connected-x86.tsv` 85행(실제 `graphFor` 를 연결한 12칸 판정), `ai-bridge-planning-rectangle-x86.tsv` 24행(단일 칸/반전/flat 행 경계), `ai-bridge-planning-delay-x86.tsv` 28행(40/120초와 큰 정밀도/무한 시각의 전체 본체).
- cpppj 계약: graph 하위가 현재 owner/좌표를 바꾸면 변경 후 필드로 판정하고 해제 토큰은 거부, 협력자 접근의 주변 보존/물리 밖 거부와 flat 행 경계 spot, 생성자/graph getter 타입 오류 거부.

최초 실패와 교정: 새 모듈과 시험은 10건 모두 첫 실행에 통과했다(fixture 1,007행 포함). cpppj `AiBridgeCandidatesTests.cpp` 의 `ComposesInitializationAndBudgetedStep`·`ComposesStartAndAdvance` 는 `RawAiBridgePath` 가 필요해 그 단위에서 옮긴다.

## 3. 경계와 남은 것

- 실제 전략 연결(`RawAiBridgeStrategies`)·탐색(`RawAiBridgeSearch`)·경로(`RawAiBridgePath`: Initialize/Step/Advance)는 이 계열의 다음 단위이며 `prepare`/`start` 를 Path 의 hook 으로 연결한다. 실제 월드 지도/프레임 코드/그래프 연결은 후속 AI 세션 연결 단위이다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiBridgePath`(fixture `initialize`·`clear`·`step` 계열)·`RawAiBridgeSearch`·`RawAiBridgeStrategies`·`RawAiCollectors`·`RawAiFactoryProduction`·`RawAiCarrierActions`·`RawAiPreparation`·`RawAiStrategySetup`·`RawAiWorldRegistration`·`RawAiObjectNotice`·`RawAiDamage` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
