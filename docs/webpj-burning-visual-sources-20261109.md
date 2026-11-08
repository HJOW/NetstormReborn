# webpj Burning 표시(RawBurningVisual)·연소점 선택(RawBurningSources)·연소점 개수(RawBurningSourceCount) 이식 — WB04-E183

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백여든세 번째 부분이다. Burning 모드 0/1 생성자의 표시 등록·150입자 초기화와 실제 입자 추가를 맡는 `o/RawBurningVisual`(생성자 상위 몸체와 `00423e80` 공통 `Particle.New`), 현재 HP 와 캡처 마스크에서 고정 연소점 행을 선택하는 `o/RawBurningSources`(패치 `00424110`), 연소점 목표 개수의 원문 DWORD 감김과 signed IDIV 를 보관한 `o/RawBurningSourceCount`(원문 `4241a2`)를 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 세 몸체와 시험/fixture 의 변경 없음을 `git diff --numstat` 으로 확인). E76 `raw-burning-victim.js`(세 부모의 공통 앞단)와 E129 `RawRenderRegistry`, E1/D4 의 `readRiftHitPoints`/`currentRiftTypeNumber`, `GameAuxState` 위에 얹어 처음으로 연소 계열의 실제 PE 관측 fixture 네 개(128+326+18+2,826행)를 재생한다.

## 1. 옮긴 것

`www/core/raw-burning-visual.js`(DOM 없음)

- `createBurningVisualFields()` — 원본 24바이트 입자(`{ x, y, velocityX, velocityY, life, source }`)와 의미 필드(registration·capacity/total·particles·lastTime/elapsed·palette/sound·stage·image·victim·mode·rows·selected·off)를 모은 객체를 만든다. 미초기화 delta 40/44 와 DWORD 74 의 값을 만들지 않는다.
- `new RawBurningVisual(pool, registry, state, hooks)` — `registry` 는 같은 풀의 `RawRenderRegistry`, `state = { processType: 14, palette }` 는 월드가 소유하는 공유 색 표, `hooks = { reserve, createPalette, gradient, buildPalette }`(색 표 생성은 도달할 때만 요구). `initialize(fields, token, victim, mode)` 는 공통 표시 등록 → 의미 필드 초기화 → 150입자 zero(3600바이트 확보) → 공유 색 표가 없으면 첫 색 표(8바이트·주황 → 밝은 색 8색 띠, 보정 2.4)를 만들며, 색 표 생성이 미복원이면 이름 없는 빈 색 표로 완료 처리하지 않고 거부한다. `append(fields, particle)` 는 여유 칸이 있으면 개수를 늘려 거기에, 가득 찼으면 처음 만료된(ordered float life ≤ 0, NaN 은 재사용하지 않음) 칸에 6필드를 쓰고 성공 여부를 돌려준다.

`www/core/raw-burning-source-count.js`

- `burningSourceCount(hitPoints, maximum, width, height)` — 현재 HP 는 getter 전 저장값이다. 최대 HP×2 의 하위 DWORD 를 signed /4 한 half 로 분모 = 최대 − half(감김), 분자 = HP×5(감김)를 signed 나눗셈해 목표 = 6 − 몫(감김)이고 행 수 = (너비 + 높이)(감김)/5 로 `max(1, min(목표, 행 수))` 를 돌려준다. 분모가 0 이면 `BurningSourceCountError('zero')`, 분자가 INT_MIN 이고 분모가 −1 이면 `'overflow'` 이다(원본 IDIV 예외를 구별).

`www/core/raw-burning-sources.js`

- `new RawBurningSources(pool, types, visual, random, state)` — `state = { weakenedMana, manaType: 158, daisType: 165, sharedWeakenedMana?(), sharedManaType?(), sharedDaisType?(), victim: { checking(), robust?, assertion? } }`. shared* getter 가 있으면 독립 값보다 우선해 실제 GameAux 의 사제 1/4 HP·사제/다이스 TYPE 현재 주소를 매 호출 다시 읽는다.
- `select(fields)` — 피해 부모(`runBurningVictimParent`, Sources 입구)가 조기 반환하면 아무것도 하지 않고, 현재 HP 를 최대 HP getter/진단보다 먼저 저장한 뒤 최대 HP·이미지 크기로 목표 행을 구한다. 진단 복귀 뒤 같은 선택 슬롯의 현재 TYPE flags2 0x400 이면 목표를 1 로 강제한다. 목표가 선택 개수와 같으면 반환, 높이 3 미만이면 선택 개수만 갱신, 선택 개수가 작으면 최대 300번의 시도로 난수(x → y 순, 아래 픽셀이 투명하면 rowTaken 에 도달하지 않음)로 행을 골라 6..9개의 대각선 점을 `append` 하며(성공한 점마다 rowTaken 표식, 4개 이상 성공이면 선택 +1), 300번째 시도 뒤 남은 횟수가 0 이면 마지막 행을 제외한 y/x 순서의 최초 7개 불투명 점 폴백을 실행한다. 선택 개수가 크면 첫 source 점의 행 표식(1 이어야 함)을 지우고 같은 행의 source 표식/life 를 0 으로 바꾸되 total/selected 는 보존한다. 불투명 픽셀 뒤 실제 선택된 잘못된 행은 계속 보호한다(`RangeError('Burning sources 새 행 인덱스 오류')`).

## 2. 검증

`tests/burning-visual.test.mjs`(3건)·`tests/burning-sources.test.mjs`(5건)·`tests/burning-source-count.test.mjs`(4건). 실제 PE 관측 fixture 를 입력만 설치해 재생한다(CD 행은 웹 범위 밖).

- `burningprocess-constructor-x86.tsv` 10.78 128행: 피해 SID·모드·초기 표시 목록·깊이 BYTE 입력으로 warm-palette 생성자를 한 번 호출해 registration 필드·시각/이미지/모드 등 27개 의미 값, 표시 슬롯, 메모리/부착 호출 순서, 150입자 24바이트의 Adler-32 와 합을 대조한다.
- `burningsources-x86.tsv` 10.78 326행(전체 본체·입자·행 표식·난수 상태)을 독립 값 설정과, 반대 fallback 으로 연결한 공유 현재 위크 마나 getter 의 두 방식으로 모두 재생한다. `burning-sources-corner-x86.tsv` 10.78 18행은 작은 투명 캡처에서 10.78 이 투명 픽셀에서 논리 행 끝 후보를 읽지 않는 순서를, cpppj 계약은 불투명 뒤 잘못된 행의 보호를 검사한다.
- `burning-source-count-current-x86.tsv` 10.78 2,826행(두 판본 합 5,652행): 높이 2 로 입자·난수·픽셀을 실행하지 않고 부모 robust 앞단 호출 횟수, 최대 HP getter/정수식/IDIV 예외, 진단 복귀 뒤 선택/전역(사제 TYPE·다이스 TYPE·위크 마나·HP 저장)을 원문과 대조한다. cpppj 계약(장기 owner/현재 다이스 예외 구별, HP 진단이 자기 연결을 반납해도 현재 getter 복귀와 이전 HP 저장 보존)과 순수 IDIV 계산(zero/overflow/클램프)도 함께 검사한다. 시험의 weak 포인터 수명 단언은 호스트 GC 특유라 옮기지 않았다.

최초 실패와 교정: 모듈은 모든 fixture(3,298행 + 18행)를 첫 실행에 통과했다. 내가 쓴 순수 IDIV 시험의 입력(HP INT_MIN)과 기대값(목표 4)이 처음에 틀려 시험만 고쳤다.

## 3. 경계와 남은 것

- 실제 Step/캡처/팔레트/표시(`RawBurningFrame`·`RawBurningLifetimeDraw`·`BurningVisualSystem`), 연소 입자 이동/표시, 실제 피해 월드 연결은 후속 단위이다. `RawBurningVisual.initialize` 의 첫 색 표(`createPalette`/`gradient`/`buildPalette`)는 hook 경계로만 연결되어 있다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawBurningFrame`·`RawBurningLifetimeDraw`·`RawBurningVisual` 의 후속 입자 프레임/표시, `RawPacketDispatch`→`RawConstructionCommand`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
