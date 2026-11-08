# webpj AI 수집자 행동(RawAiCarrierActions)·준비(RawAiPreparation) 이식 — WB04-E181

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백여든한 번째 부분이다. AI 수집자의 수집/포획/귀환/경로 취소 전체 행동과 그 아래의 내용물 genus 조회·다이스 접근점을 맡는 `o/RawAiCarrierActions`(패치 `00412130` 전체 행동·`004acb80` 내용물 genus·`004487e0` 다이스 접근점)와, 현재 목표·지역·지갑·초기 월드 등록 준비를 같은 AI 본체에 적용하는 `o/RawAiPreparation`(패치 `00416eb0` 준비 전체·`00410300` 적 선택·`00411a80` 부착 간헐천 수)을 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 두 몸체와 두 시험의 변경 없음을 `git diff --numstat` 으로 확인). 두 몸체 모두 독립 원본 관측 fixture 가 없고 cpppj 계약 시험이 원본 분기 순서를 손으로 쓴 입력/기대값으로 직접 단언한다.

## 1. 옮긴 것

`www/core/raw-ai-carrier-actions.js`(DOM 없음)

- `findContainedGenus(pool, state, types, parent, mask, genus, index)` — 현재 내용물의 kind WORD 로 TYPE genus 를 먼저 읽고(mask 탈락 후보라도 TYPE 범위를 거부, kind 는 BYTE 로 자르지 않음) 전체 WORD mask 와 모든 genus 비트, 일치 순번(DWORD 감소)을 검사해 첫 일치 SID(없으면 0)를 돌려준다. `daisApproachPoint(pool, types, checking, invalid, assertion, dais)` — `checking()` 이 비영이고 다이스가 물리 범위 밖이거나 genus 0x400000 이 아니면 진단을 호출하고(복귀하면 계속), 진단 뒤 현재 TYPE/좌표/공유 실패점 `invalid` 를 다시 읽어 유효한 다이스면 좌표 −3 의 float 쌍을, 아니면 현재 실패점을 돌려준다(free/dead 필터 없음).
- `new RawAiCarrierActions(memory, pool, types, globals, hooks)` — `globals = { now(), boss(), checking(), invalidGraph() }`(앞 셋은 생성자에서, 마지막은 도달할 때만 요구), `hooks` 는 최대 HP·포획 사제·경로·다이스 접근점·좌표 이동·신전 귀환·적 참여·취약·비행·그래프·크리스털·내려놓기·삭제·대상 이동·등록·수집·경로 대상·재분류·정지·진단 하위로 도달한 위치에서만 요구한다.
- `act(token)` — (1) 물리 적 SID 가 있으면 최대 HP prefix(결과가 버려져도 실행), (2) 포획 목록(+0x384)에서 포획 사제를 든 정지 운반자를 가장 먼저 귀환(다이스가 있으면 접근점 이동, 없으면 신전 귀환)하고 목록을 한 actor 로 축소(생각 기한/일반 수집자보다 우선), (3) 일반 수집자 목록(+0x2dc)이 비어 있거나 기한(+0x5a8, less/unordered 는 모두 미도달)이 아니면 종료, 아니면 순환 cursor(+0x6d0, signed 나머지, 쓰고 난 뒤 물리 접근 거부)로 한 수집자를 골라 free 는 모든 중복을 안정 제거하고 priest genus 는 진단하며, 경로가 없으면 포획 가능(능력·용량·적 참여·취약·기한·비행/그래프)이면 포획(크리스털 비우기/내려놓기·적 이동·등록), 아니면 포획 사제 귀환 또는 수집/귀환 판단 뒤 포획 목록에서 제거하고, 경로가 있으면 포획 대기 보호·간헐천 목표 경로 유지·재분류 뒤 정지한다.

`www/core/raw-ai-preparation.js`

- `new RawAiPreparation(memory, pool, players, allies, globals, hooks)` — `players` 는 9×180 BYTE Player 기록, `allies` 는 방향별 9×9 DWORD(대칭화하지 않음), `globals = { localOwner(), playerKnown(), noRegion(), invalidGraph(), now() }`, `hooks` 는 적 진단·사제·신전·최대 HP·사제 거리·그래프·지역 경계·분산 SP·로컬 SP 가산·데모·지식/차단 창·초기 스캔 하위이다.
- `selectEnemy(token)` — +0x264 를 비운 뒤 강제 대상(+0x240)이면 사제/동맹 확인 없이 선택, 아니면 Player 1..8 중 현재 사제가 있고 현재 owner 행에서 동맹이 아닌 첫 owner 를 선택한다. `countAttachedGeysers(token)` — +0x324 포인터와 +0x338 개수(원본의 +0x32c 불일치를 교정하지 않음)로 각 간헐천의 현재 그래프가 +0x298 과 같은 수를 센다.
- `update(token)` — 적 재선택 → 적 진단 → 데모 지식/차단 창 생각 기한(float rate + QWORD now) → 자기 신전 → 능력 0x100 이면 참여 적 사제 중 최대 HP 절반 미만(HP 선저장)인 대상을 같은 동맹의 모든 참여 사제 거리로 비교(먼저 가까운 자신을 발견해도 중단하지 않음)해 현재 owner 가 최근접이면 선택 → 적 신전/그래프/지역 경계 중심(x87 배정도 중간 합 뒤 float 저장) → 소유/적 사제 → 로컬 SP(FILD 정수 비교와 float 차액, 두 번째 읽기에서 현재 owner/지갑 재조회) → 최초 한 번 초기 월드 등록(+0x25c) → 부착 간헐천 수로 수집자 충족(+0x2cc)/간헐천 충족(+0x2d0, 12000 초과 지갑이면 1)을 쓴다.

## 2. 검증

`tests/ai-carrier-actions.test.mjs`(15건)·`tests/ai-preparation.test.mjs`(7건). 두 몸체 모두 독립 원본 관측 fixture 가 없어 cpppj 시험의 입력과 단언을 그대로 옮겼다.

- 수집자 행동: 물리 적 SID 의 최대 HP prefix 와 빈 일반 목록 종료, 포획 귀환의 우선순위와 이동 효과 뒤 큐 축소, less/unordered 기한의 순환 인덱스 불변, 순환 시작점과 free 안정 제거·cursor 비보정, 반환하는 free/priest 진단의 상태 재조회, 서버 포획의 첫 크리스털 삭제와 현재 적 이동·등록, 현재 boss 의 크리스털 조회 뒤 재판정과 비서버 내려놓기, 첫 비행 포획 운반자의 그래프 미요구와 기존 비행 운반자의 그래프 판단, 참여 효과 뒤 현재 적의 HP 판정과 포획 기한 NaN 단락, 포획 사제 귀환/수집 뒤 현재 큐 정리, 포획 대기 중 경로 보호·targetA 0 비정지, 등록 간헐천의 비행/같은 그래프 경로 유지와 다른 그래프 재분류·정지, 정지 하위 누락 시 앞선 효과 보존과 signed 음수 cursor, 내용물 genus 의 kind 전체 WORD/필터 순서, 다이스 접근점의 진단 효과 뒤 재조회, 생성자 거부.
- 준비: 강제 대상/첫 비동맹 선택(사제 조회 중 owner 변경 포함), 같은 최초 스캔 위치·그래프/범위·두 수량 판정과 QWORD 시각, 절반 HP 경계·전체 동맹 거리 방문·현재 적 조기 반환과 HP 선저장, FILD 정수 비교와 float 차액·두 번째 SP 읽기, +0x324/+0x338 불일치 조합과 하위 뒤 재조회, 선택되지 않은 누락 하위 미요구와 선택된 지점의 앞선 쓰기 보존, 생성자 거부.

최초 실패와 교정: 새 모듈과 시험 22건은 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 최대 HP(`004adcb0`)·포획 사제/경로(`004acb80`/`00411630`)·좌표 이동(`004117e0`)·신전 귀환(`004118a0`)·크리스털 조회/삭제(`004acc60`/`004ac9f0`)·수집/귀환 판단(`00411900`)·청크 경계(`00432210`)·분산 SP(`0040ec50`)·초기 월드 등록(`00415720`, E180 `RawAiWorldRegistration.fullScan`)·지식/차단 창 연결은 후속 AI 세션 연결 단위이다. 두 몸체는 모두 hook 경계로만 연결되어 있다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiStrategySetup`·`client/RawAiDebugMenu` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
