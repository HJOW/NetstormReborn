# webpj AI 건설 선택(RawAiBuildChoice)·AI 이동(RawAiMovement) 이식 — WB04-E167

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백예순일곱 번째 부분이다. 사제 AI 의 다음 건설 TYPE 선택(`00411b40` 와 `00411a40`·`00411ad0`·`004709d0`·`00470a30`)을 옮긴 `o/RawAiBuildChoice`와, AI 의 경로 조회/정지·대상/좌표 이동·신전 복귀(`00411630`~`004118a0`, 발자국 거리 `0041d840`)를 옮긴 `o/RawAiMovement`이다(cpppj 기준선 `1f00a25a`, 기준선 뒤 두 파일의 .cpp/.h 와 시험의 변경 없음을 `git diff --numstat` 으로 확인). AI 계열(33개 파일) 다섯 번째 단위이며 E164 의 `RawAiBrainMemory` 위에서 동작한다.

## 1. 옮긴 것

`www/core/raw-ai-build-choice.js`(DOM 없음)

- `new RawAiBuildChoice(memory, types, globals, hooks)` — `types` 는 현재 188 TYPE({ cost })의 같은 배열(복사 없이 매 비용 조회에서 현재 값을 읽는다), `globals = { localOwner(), playerKnown(), collectorType(), daisType(), workshops[4], temple(remainder) }`, `hooks = { captured, dead, canAfford, wallet, storedSp, assertion }`. 도달한 위치에서만 하위를 요구하고 없으면 그 위치에서 오류이며 앞선 효과는 유지된다.
- `reserveForCollectors(token)` — `ftol(현재 수집자 cost) × +0x234` 의 하위 DWORD 곱(감김)을 signed 로 보아 현재 지갑 이하인지 본다. `enemyLowFunds(token)` — 적이 로컬이 아니면 원격 지갑, 로컬이면 playerKnown 일 때 분산 SP(미확정 0.0f)를 ftol 한 signed 값이 1000 미만인지 본다(NaN 은 0).
- `select(token)` — 포획 다이스 관문(단락 순서: +0x248 bit 0x200 → +0x2a4 없음 → 포획 → 미사망 → 구매 가능 → 신전 있음 또는 적 자금 부족) → 신전 없고 +0x390 ≠ 3 이면 원소 신전 TYPE(비용 ≤ 지갑일 때만) → 후보 워크샾이 지갑/수집자 여유 비용을 모두 통과하면 다섯 theme 수량(TYPE 0 포함) 복사본을 선호 순서대로 감소시켜 처음 음수가 되는 선호 TYPE. 실제 본체 수량표는 쓰지 않으며 각 관문 뒤 본체 필드와 다이스/적 owner 를 다시 읽는다. `workshopType`(범위 밖 진단 0x1a)·`templeType`(signed `% 5`, 진단 0x47)도 같다.

`www/core/raw-ai-movement.js`

- `aiDistanceToFootprint(point, bounds)` — x 차이만 float 에 저장하고 y 는 배정도이며 내부 점의 음수 거리/unordered(x 선택)를 보존한다.
- `new RawAiMovement(memory, pool, boss, board, packet, hooks)` — `boss()` 는 현재 권한(원본 위치마다 재조회), `board` 는 `RawBoardTarget`, `packet` 은 사용자 이동과 같은 `CarrierMoveState` 헤더. `movementForm`/`stop`(actor 0 무동작)·`toTarget`(비서버: 접근점 → 현재 헤더 18바이트를 채널 9 로 송신, 서버 희생은 의식만, 송신 뒤 권한 변경은 대상 접근 분기를 따름, 대상 → 출발 이름을 디버그 조회 전에 읽고 +0xb00 이 켜질 때만 로그)·`toPoint`(인자 0 실제 경로 시작, 실패하면 이름/로그 생략, 로그 좌표는 y 먼저 ftol·무효는 0)·`home`(신전이 있고 거리 2 이상 또는 unordered 이면 효과 뒤 다시 읽은 신전으로 이동).

## 2. 검증

`tests/ai-build-choice.test.mjs`(14건)·`tests/ai-movement.test.mjs`(9건). 두 몸체 모두 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 손으로 쓴 입력/기대값으로 직접 단언한다). 같은 입력과 단언을 옮겼다.

- 건설 선택: 수집자 cost 의 곱셈 전 절삭과 감김·현재 전역 TYPE, 적 자금(로컬 SP 절삭/미확정 0/원격 signed 지갑/NaN), 다이스 우선과 단락·효과 뒤 현재 적/다이스 재조회, 신전 없는 다이스와 원소 신전 비용 경계, 0..3/4·5/진단과 signed `% 5`, 무신전 theme 3 과 NULL 후보, 지갑/여유 비용 관문 분리, 중복 선호의 복사 수량 감소와 실제 수량 보존, 다섯 번째 수량(TYPE 0)과 음수 감김, 수량 복사 순서와 손상된 임시 인덱스 거부, 자금 하위 누락의 앞선 효과 보존, 생성자 거부.
- 이동: 현재 TYPE/순번 0/remove 인자와 정지 0, 서버 실패/희생/성공의 이름 읽기 순서, 비서버의 18바이트(현재 헤더 byte0 = protocol & 0x7f, 길이/부모/좌표/희생) 송신, 송신 뒤 권한 재조회, 시작 인자의 나머지 0 과 ftol 로그 좌표, 거리 경계 1.999/2/NaN 과 효과 뒤 현재 신전 재조회, 발자국 거리, 누락 하위의 앞선 효과 보존, 생성자/목표 SID 물리 검사.

최초 실패와 교정: 새 모듈과 시험은 23건 모두 첫 실행에 통과했다(시험 작성 중 `RawAiBuildChoice` 무신전 후보 시험의 기대값을 원본 C++ 시험의 `Throws` 로 바로잡았다 — 수집자 TYPE 200 조회는 188 밖 거부).

## 3. 경계와 남은 것

- 실제 사제 포획(`00492340`)·사망(`004922e0`)·구매 가능(`00410280`)·지갑(`00413c20`)·분산 SP·경로(`00488fe0`/`004b1a90`/`004894e0`/`00489080`)·의식(`00494eb0`)·이름/로그 조회는 후속 AI 세션 연결 단위이며 지금은 명시 hook 이다. `RawPathStart`(경로 시작 몸체)는 아직 `todo` 이다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiBrainInitialUpdate`(`00414710`), `RawAiDropSelection`·`RawAiDeployment`·`RawAiResourceChoice` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
