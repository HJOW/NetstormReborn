# webpj AI 사제 행동(RawAiPriestActions) 이식 — WB04-E171

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백일흔한 번째 부분이다. 사제 AI 의 의식/건설/탐색/회수/수집·귀환 전체 행동(패치 `00411cd0`)과 그 헬퍼(`0041d7a0` 좌표 보정·`0040e800` 좌표 유효성·`00491b30` 의식 구간·`00448a90` 제단 argument 비교)를 옮긴 `o/RawAiPriestActions`(cpppj 기준선 `1f00a25a`, 기준선 뒤 이 파일과 시험의 변경 없음을 `git diff --numstat` 으로 확인)이다. AI 계열(33개 파일) 아홉 번째 단위이며 `RawAiFakeDrySpots`·`RawAiDropActions` 가 `aiPriestPointValid` 를 쓴다.

## 1. 옮긴 것

`www/core/raw-ai-priest-actions.js`(DOM 없음)

- `snapAiPriestPoint(point)` — 각 축에 float 0.99999 를 더한 뒤 중간 float 저장 없이 signed ftol 하위 DWORD 를 float 으로 저장(ceil/반올림과 다름). `aiPriestPointValid(point, mapLimit)` — 두 float 모두 0 보다 크고 mapLimit 을 float 로 저장한 값보다 작아야 하며 NaN 은 false. `aiPriestInRitual(pool, frames, priest)` — 현재 물리 frame(+36)의 프레임 코드 side 를 signed char 로 읽은 방향이 10..14(상태 BYTE 와 무관). `aiAltarContainsEnemy(pool, dais, enemy, altarType, find)` — 다이스 위치의 제단 argument WORD 와 적 owner 전체 DWORD 비교(찾은 SID 0 도 읽음).
- `new RawAiPriestActions(memory, pool, globals, hooks)` — `globals = { now(), mapLimit(), daisType() }` 는 각 원본 읽기/기한 쓰기 위치에서 현재 값을 다시 읽고, `hooks` 는 scan·vulnerable·busy·path·maxHitPoints·ritual·altarEnemy·target·distance·select·home·collect·deploy·begin·point·enemyLowFunds·salvage 를 도달한 위치에서만 요구한다.
- `act(token, priest)` — 탐색 접두(진행 중이면 `scan(0)` 만) → 사제 0/생각 기한(+0x5a8, unordered 미도달)/상처/예약 → 경로가 있으면 최대 HP 조회만 → 건설 기한(+0x5c0) → 다이스/제단(+0x2a4 와 능력 0x400)의 의식(본체 +0x29c 의 현재 사제)/적 제단(희생 요청 두 SID 재조회)/거리 2 초과 이동(2 와 NaN 은 접근 안 함) → 다음 건설 TYPE 선택 → 지갑 400 미만이면 HP 선저장·현재 800h 능력으로 수집/귀환 → 목표 유효성과 TYPE 0(유효 목표만 지움, 미정이면 귀환/수집) → 현재 위치 보정 좌표가 목표와 같으면 20초 기한 뒤 raw 좌표로 배치하고 목표를 지움 → 탐색 기한(+0x5c8) → 같은 TYPE 완료 탐색의 최적 좌표 소비·보정·이동 또는 새 탐색(그 시점의 여덟 인자 snapshot)을 시작 → 다이스 회수(건물 중 signed HP 최대 첫 SID 또는 신전, 회수 기한이 정확히 0 일 때만 대상 이동·회수 요청·10초 기한·재탐색·`scan(1)`) → 탐색 기한 현재 시각 + 3초. 각 관문 뒤 본체 필드를 다시 읽고 선택된 하위 누락은 앞선 효과를 보존하며 명시 실패한다.

## 2. 검증

`tests/ai-priest-actions.test.mjs`(21건). 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 손으로 쓴 입력/기대값으로 직접 단언한다). 같은 입력과 단언을 옮겼다: 탐색 접두/생각 관문(NaN·상처·예약·이동 중 최대 HP·건설 기한), 의식/제단/거리 경계, 저자금·HP 선저장·800h, TYPE 0, 배치 기한과 누락 하위 prefix 보존, 탐색 기한 ordered, 완료 탐색 소비/보정/이동과 새 탐색 snapshot·현재 시각 + 3, 선택 하위의 탐색 진행 변경, 회수 대상/요청 순서/신전·자금 대체/회수 기한 0 비교(과거·NaN 도 차단, ±0 만 허용)/누락 하위, 0.99999 보정과 mapLimit float 저장, 제단 argument WORD, 의식 프레임 코드, 생성자와 가짜 본체 거부.

최초 실패와 교정: 새 모듈과 시험은 21건 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 하위(`00411900` 수집/귀환·`004108f0` 배치·`0040f660` 탐색 초기화(E169 `initialize`)·`004116d0` 대상 이동(E167 `toTarget`)·`004117e0`(E167 `toPoint`)·`00411b40`(E167 `select`)·`004118a0`(E167 `home`)·`0044c420` 회수 요청 등)는 명시 hook 이며 실제 세션 연결(AI 프레임)에서 위 단위들의 메서드로 연결한다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

AI 드롭 계열(`RawAiDropTargets`·`RawAiDropEvaluation`·`RawAiDropScan`·`RawAiDropSelection`·`RawAiDropActions`)·`RawAiFakeDrySpots`·`RawAiDeployment` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
