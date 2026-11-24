# webpj WB04-E1330 — Tutorial3 단계 B(남쪽 적 섬 다리 직결)→단계 C('Build Sun Disc Throwers') L3 교차검증

- **작성일 / 단위:** 2026-11-24 / WB04-E1330 (커밋 `20261124-97 WB`, `webpj/www/` 무변경)
- **비교 기준선:** `cpppj 225553df`, 오라클 실행 파일 `extracted/webpj-oracle/225553df/Netstorm.exe` (SHA-256 `5b52d96d2e53f4674a40991b78ff64c3c7e2ad8275a0a99b22d18d532b77eb46`), 고정 프레임 `--fixed-frame-ms 25`, 1024×768 영어(`WelcomeGiven=1;tellTips=0;autoDemo=0`).
- **전 단위:** [단계 A→B 기록(WB04-E1329)](webpj-l3-tutorial3-stage-ab-20261124.md)

## 1. 단계 B → 단계 C 진행 설계 및 다리 연결 규칙 확인

1. **남쪽 적 섬 북단 지형 (`b1700.tsv`):**
   - 아군 섬 남쪽 다리 연결 시작점 `(125, 111)` 에서 남쪽 적 섬 북쪽 해안(`y=128..133`)까지 다리를 이어 `RawTutorialLater::Three` 의 `graphFor(enemy) === graphFor(own)` 조건(게이트 상태 `66 → 67`)을 만족해야 단계 C(`Build Sun Disc Throwers`)로 넘어간다.
   - 적 섬 북단 중앙(`x=126..130, y=131..133`)은 절벽 타일(`type=121`)이 전면에 있어 다리가 인접해도 그래프가 합쳐지지 않으며, 서쪽 돌출부 `(126, 130)` 또는 동쪽 내만·해안 `(131..134, 131)`·`(135, 130)` 의 `type=97` 섬 표면에 개방된 다리 끝이 닿아야 한다.
2. **다리 조각 개방 방향 및 8조각 연결 경로:**
   - 팔레트 `(20,44)` 의 1칸 다리(`BRIDGE_PATTERNS[0]`)는 기본 방향(`'J'`, 프레임 4)이 남북 개방이고 우클릭 1회(`'K'`)가 동서 개방이며, 4·5칸 다리도 각 셀의 방향 코드(`A`~`P`)와 2×2 제한 및 주변 표면 소유권(`RawCanonPlacementTerrain`/`RawCanonSurfaceWalk`)이 맞아야 놓인다.
   - E1329 종료 상태(`b1700`, `cameraX=1536, cameraY=738`)에서 다음 8조각(총 다리 수 `5 → 45`)으로 `(130,130)` 에서 `(135,130)` `type=97` 적 섬 표면에 직결했다:
     - `1710~1988`: `60,90` 6조각 하행(조각 2는 우클릭 1회 회전) → `1950` 프레임 중간 체크포인트 `b1950` 기록.
     - `2038~2054`: `20,90` 우클릭 3회 후 `(128,129)` 기준(`screen:520,686`) 배치 → `(130,130)` 동쪽 개방 끝 확보.
     - `2104~2120`: `20,90` 우클릭 3회 후 `(131,129)` 기준(`screen:568,686`) 배치 → `(131..134,130)` 가로 다리가 `(135,130)` 에 닿아 `mission_gate 3 67` 전환.
3. **단계 C 진입 (`2146~2200` 프레임):**
   - `2146` 프레임에 `Build Sun Disc Throwers` 안내 창이 열려(`mission_gate 3 67 0 1`) `2160` 프레임 체크포인트 `c2160` 을 기록하고, `2180` 프레임 `click-if OK` 뒤 `2200` 프레임 `c2200`(단계 C 자유 진행)까지 확장했다.

## 2. 오라클·검사·manifest 갱신

- **단계표·오라클:** `webpj/tests/fixtures/cpppj/candidate-225553df/tutorial3-l3/steps.json` 을 2,201프레임으로 확장하고 `node webpj/tools/l3-collect.mjs tutorial3-l3` 로 `b1950`, `c2160`, `c2200` 화면(`.png`)과 덤프(`.tsv`)를 수집했다. 기존 `a100..b1700` `.tsv` 는 실행마다 달라지는 무작위 `sp 1` XOR 키 한 줄만 차이 나고 나머지 행은 동일함을 확인한 뒤 기존 고정 해시 파일을 유지했다.
- **Playwright L3 검증:**
  - `Set-Location webpj; npx playwright test e2e/l3-tutorial3.spec.js --reporter=line` → **1 passed (2.4m)**
  - 체크포인트 화면 **12곳**(`a100`, `a160`, `a240`, `b340`, `b900`, `b1200`, `b1600`, `b1640`, `b1700`, `b1950`, `c2160`, `c2200`) 모두 cpppj 오라클과 **0픽셀 차이**, Session 덤프 **10곳** 모두 허용 행(`sp 1`·`player:*`) 외 **차이 0**으로 통과했다.
  - 웹 소스(`webpj/www/`) 변경 없이 통과해 다리 연결·그래프 전파·안내 창 게이트가 10.78 원본과 동일함을 확인했다.
- **manifest:** `webpj/tests/fixtures/manifest.json` 에 `cpppj-tutorial3-l3-{b1950,c2160,c2200}` 및 `-screen` 6항목(986 → 992항목)과 `steps`/`metadata` 해시를 갱신했다.

## 3. 남은 일

- **다음 단위:** Tutorial3 단계 C (`Sun Disc Thrower` 생산 → 단계 D `'Generators, Part 1'` 안내 창) L3 교차검증.
