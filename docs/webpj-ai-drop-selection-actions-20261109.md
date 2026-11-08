# webpj AI 배치 TYPE 선택(RawAiDropSelection)·배치 행동(RawAiDropActions) 이식 — WB04-E174

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백일흔네 번째 부분이다. AI 가 다음에 놓을 건물 TYPE 을 고르는 `o/RawAiDropSelection`(패치 `00413100` 선택/회전/직전 후보 재사용·`00425830` 방향 TYPE 판정)과, 그 선택·스캔·비용·기한·사제 관문을 실제 후보 배치로 연결하는 `o/RawAiDropActions`(패치 `004162e0` 전체 배치 행동·`00410580` 생산 조각)를 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 두 파일의 .cpp/.h 와 시험의 변경 없음을 `git diff --numstat` 으로 확인). AI 계열(33개 파일) 열세 번째 단위로 AI 드롭 계열(Targets → Evaluation → Scan → Selection → Actions)을 마무리한다.

## 1. 옮긴 것

`www/core/raw-ai-drop-selection.js`(DOM 없음)

- `AI_DROP_SELECTION_LOG`(hated·retried·scanning)와 `new RawAiDropSelection(memory, pool, types, directionalTypes, random, now, hooks)` — `directionalTypes` 는 현재 네 방향 TYPE 전역(기본 131/129/140/142)의 같은 배열, `random` 은 SP/시설/스캔과 같은 공용 `GameRandom`, `now()` 는 현재 시각, `hooks = { technology, count, blocker, nth, mayPlace, remove, log }`.
- `select(token)` — 진행 위치 초기화(+0xaf8/+0xafc = 10, +0x3ec = 0) → 현재 owner 기술 부모/개수(최초 값만 사용, 0 이면 TYPE 0·재선택 표식만 쓰고 끝) → 능력 BYTE 0x80 이면 공용 난수 100 으로 직전 표적 반복 여부(표적이 직전 표적 목록에 있으면 80 미만, 목록이 비면 30 미만) → 블로커 우선(현재 TYPE 가 블로커가 아닐 때 블로커 조각이 없으면 +0x404 만 지움) → 반복이면 같은 index 에서 압축된 현재 후보를 승인 재검사·거부 제거(재시도 로그), 아니면 기술 개수+2 번까지 회전(방향 TYPE 이고 +0x3e4 < 6 이면 2 증가, 아니면 signed 나머지 순환으로 다음 TYPE, 블로커 우선 TYPE 를 찾으면 멈춤) → 후보 count 0 이면 최고점수 0, 운반 genus 와 쿨다운이 안 지난 공급 TYPE 는 선택을 지우고, 선택이 남으면 스캔 로그. `directional(type)` 은 현재 네 DWORD 를 전체 번호로 비교한다.

`www/core/raw-ai-drop-actions.js`

- `AI_DROP_ACTION_LOG`(일곱 원래 알림)와 `new RawAiDropActions(memory, pool, types, random, globals, hooks)` — `globals = { now(), mapLimit(), checking() }`, `hooks = { product, select, scan, canAfford, mayPlace, deploy, fake, seed, remove, assertion, log }`. 알림은 `{ action, type, direction, x, y }`(Partial 외에는 알림 전에 TYPE getter 범위 검사).
- `run(token)` — 현재 TYPE 의 생산 조회 실패는 알림만 → TYPE 가 없거나 재선택 표식이면 선택 → 스캔 미완료면 스캔(완료 시 표식 + 현재 TYPE/방향 알림) → 기한 두 개(unordered 미도달)·지갑(signed 1200)·최초 시도(+0x400)·생산 조각(pending bit 만 제외)·사제 탐색 진행(signed)·유효 목표 좌표 관문 → 스캔 미완료 알림 → 후보가 없으면(checking 진단 뒤 현재 TYPE flags2 0x800 이면 fake + 재선택, 아니면 seed) 있으면 비용 확인 뒤 성공/표적을 지우고 공용 난수로 후보 하나씩 승인(거부 BadChoice, 승인 Laying/Target 알림 뒤 실제 배치와 성공·표적·집중(+0x248 하위 BYTE 0x40) 쓰기)하며 실패 후보도 실제 압축으로 제거하고 압축 뒤 현재 성공 표식으로만 중단한다.

## 2. 검증

`tests/ai-drop-selection.test.mjs`(16건)·`tests/ai-drop-actions.test.mjs`(19건). 두 몸체 모두 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 손으로 쓴 입력/기대값으로 직접 단언한다). 같은 입력과 단언을 옮겼고 E173 의 `RawAiDropScan.remove`(실제 30후보 압축)를 같은 저장소에서 사용했다. 선택 쪽은 기술 0개 접두, 최초 기술/개수 캐시, 방향 전역 전체 DWORD 비교와 회전(홀수/음수), ordinal signed 나머지, 난수 100 의 BYTE 조건, 압축 뒤 같은 index 재검사, 80/30 경계, 재사용 승인 쿼리, 블로커 우선/현재 블로커/부재/회전 예산, 최고점수 0 조건, 운반체와 공급 쿨다운(less/unordered), nth 누락 prefix. 행동 쪽은 TYPE 0 경계, 능력 상실, 스캔 snapshot/완료, 두 기한/NaN 시각, signed 지갑, 최초 시도/Partial, 생산 조각 pending, 사제 진행/목표, 진단 뒤 fake/seed, 비용 거부 상태 보존, 난수 소비/압축, 집중 BYTE, 중단 조건, 누락 효과 보존, 로그 꺼짐의 물리 표적 조회.

최초 실패와 교정: 새 모듈과 시험은 35건 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 기술 내용물 조회(`004acc60`·`004acb80`·`004acac0`)·비용(`00410280`)·배치(`004108f0`)·전체 배치 검사·로그(`004105b0`) 연결은 후속 AI 세션 연결 단위이다. `RawAiDropActions` 의 `scan`/`remove`/`fake`/`seed` hook 은 각각 E173 `RawAiDropScan.scan`/`remove`, E172 `RawAiFakeDrySpots.extend`, E170 `RawAiDrySpots.seed` 에 연결한다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiDeployment`·`RawAiResourceChoice`·`RawAiBridge*`·`RawAiCollectors`·`RawAiFactoryProduction`·`RawAiCarrierActions`·`RawAiPreparation`·`RawAiStrategySetup`·`RawAiWorldRegistration`·`RawAiObjectNotice`·`RawAiDamage` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
