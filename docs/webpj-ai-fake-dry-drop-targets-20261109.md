# webpj AI 가짜 부족 좌표(RawAiFakeDrySpots)·배치 표적 점수(RawAiDropTargets) 이식 — WB04-E172

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백일흔두 번째 부분이다. 공급 시설/현재 부족 거리와 원본 양방향 탐색으로 AI 가짜 부족 좌표를 갱신하는 `o/RawAiFakeDrySpots`(패치 `00415fe0`)와, AI 건물 배치의 실제 표적 점수와 Gun/장애물 후보 평가를 옮긴 `o/RawAiDropTargets`(패치 `00412770`·`004128f0`·`00412950`·`00412a70`·`0048ece0`)를 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 두 파일의 .cpp/.h 와 시험의 변경 없음을 `git diff --numstat` 으로 확인). AI 계열(33개 파일) 열한 번째 단위이다. 앞 단위의 `RawAiDrySpots`(E170)·`aiPriestPointValid`(E171)·`aiPointDistance`(E163)를 쓴다.

## 1. 옮긴 것

`www/core/raw-ai-fake-dry-spots.js`(DOM 없음)

- `AI_FAKE_DRY_LOG` — `004105b0` 의 두 원래 알림(`discarded`/`added`). `new RawAiFakeDrySpots(memory, pool, globals, hooks)` — `globals = { manaRadius(), mapLimit(), debug() }`(현재 반경은 제곱 반경이 아님), `hooks = { score(token, point, box), center, candidate, add, remove, seed, mark, log }`.
- `extend(token)` — fake TYPE 0 이면 무동작, 개수가 용량 이상이면 다른 모든 하위보다 먼저 `remove(0)`. 공급 시설이 없으면 신전이 없을 때 끝, 있으면 신전의 raw 좌표를 추가(중복 실패 무시)하고 Added 로그. 공급 시설이 있으면 진입 시점의 count/포인터를 유지해 각 시설 raw 좌표의 부족 거리 점수가 strict 최고인 시설/index 를 고르고, 양수이면 선택 시설 중심과 선택 Dry Spot 좌표 − 1 로 양방향 탐색(출발은 중심 Y 를 X/Y 에 두 번 쓴 원본 인자)한다. 후보가 유효하고 중심과의 거리가 반경의 0.7 배(float)보다 크면 추가하고 현재 전략(+0x2d4 == 4)이면 전략 4 상태를 미정으로 되돌리며 +0xaf0/+0xaf4 를 갱신하고 디버그 29 이면 표식을 만든다(아니면 Discarded). 마지막으로 현재 가짜 좌표가 유효하지 않으면 실제 Seed 로 보충하고 추가된 경우에만 원래 후보 좌표로 Added 를 로그한다.

`www/core/raw-ai-drop-targets.js`

- `new RawAiDropTargets(memory, pool, types, globals, hooks)` — `types` 는 현재 188 TYPE({ flags2, group })의 같은 배열, `globals = { altarType(), directionalType() }`. 출력 SID 는 `{ value }` 상자.
- `score(token, target)` — 가산점(기본 10/0x4000000 이면 5, 0x200000 은 +5 또는 +200, 0x200 +6, group 6 +7, 현재 제단 +8, 운반 genus 포획 사제 +400) → hpPerSec 를 더해 ftol(하위 DWORD) → 본체 두 목록의 첫 일치마다 +100 → 집중 대상은 HP(getter 전 저장)가 signed 최대 HP/2 이상이면 +50 → owner BYTE 가 적 owner DWORD 와 다르면 signed 절반(0 방향). `gun`/`ground` 는 검색 결과를 점수 계산 전에 출력에 쓰고(0 도 기록) 비영만 점수화, `blockers` 는 모든 등장의 점수 합(DWORD 감김)과 방향 블로커의 점수 뒤 두 축 −1 facing 필터, 양수 최고 갱신 때 목록 첫 SID 출력, `facing` 은 flags2 0x100000 제외 → 실제 중심 → 네 방향 일치.

## 2. 검증

`tests/ai-fake-dry-spots.test.mjs`(13건)·`tests/ai-drop-targets.test.mjs`(15건). 두 몸체 모두 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 손으로 쓴 입력/기대값으로 직접 단언한다). 같은 입력과 단언을 옮겼다.

- 가짜 부족 좌표: fake TYPE 0 의 무효과, full 제거의 선행(cursor/가짜 좌표 불변), 신전 raw 좌표와 중복 Added, 진입 cache 와 첫 최고점수, 중심 효과 뒤 현재 Dry Spot pointer 와 선택 index 보존, 0.7 배 strict/NaN 반경 폐기, 유효성 경계의 무로그, 음수 공급 count 의 Seed 보충, Add false 에도 좌표/전략 4 갱신과 표식/로그 순서, 표식이 가짜 좌표를 무효화한 뒤 Seed 와 원래 후보 로그, 후보 뒤 현재 TYPE/전략/b00 재조회, 누락 하위의 앞선 쓰기 보존, 생성자 거부.
- 배치 표적: 모든 가산 종류와 결합, ftol/NaN, 포획 400점(운반 genus 한정·현재 rate), 목록 첫 일치/현재 backing, HP 선저장과 signed 절반, owner 전체 DWORD 비교, DWORD 감김, 물리 SID 0 진단, Gun/지상 결과의 선기록, 블로커 총점/첫 항목 출력/출력 보존, 방향 필터와 현재 전역 TYPE, 홀수 방향/세로 동률, 생성자와 가짜 본체/표 밖 TYPE 거부.

최초 실패와 교정: 새 모듈과 시험은 28건 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 부족 거리(`00412640`)·중심 조회·양방향 탐색(E163)·표식(`00410540`)·검색(`0046b6f0`/`0046b540`/`00469590`)과 TYPE+48 hpPerSec·최대 HP·포획 내용물 조회는 후속 AI 세션 연결 단위이며 지금은 명시 hook 이다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

AI 드롭 계열 나머지(`RawAiDropEvaluation`·`RawAiDropScan`·`RawAiDropSelection`·`RawAiDropActions`)·`RawAiDeployment` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
