# webpj WB04-E1333 — Tutorial3 단계 F~H(적 사제 무력화·골렘 포획·완료 창·Tutorial4 해제) L3 교차검증 + `INJECT_SHIFT.right` 교정 + Node 26 전체 회귀

- **작성일 / 단위:** 2026-11-25 / WB04-E1333 (커밋 `20261125-01 WB`, `webpj/www/` 무변경)
- **비교 기준선:** `cpppj 225553df`, 오라클 실행 파일 `extracted/webpj-oracle/225553df/Netstorm.exe` (SHA-256 `5b52d96d2e53f4674a40991b78ff64c3c7e2ad8275a0a99b22d18d532b77eb46`), 고정 프레임 `--fixed-frame-ms 25`, 1024×768 영어(`WelcomeGiven=1;tellTips=0;autoDemo=0`).
- **전 단위:** [단계 D~E→F 기록(WB04-E1332)](webpj-l3-tutorial3-stage-def-20261124.md) · **Tutorial3 완료 근거**

## 1. 단계 F~H 진행 및 `right` 입력 시프트 교정

1. **적 사제 무력화 (`3500~4162` 프레임):**
   - `3500`: `click screen:40,748` (미니맵으로 적 섬 하단 이동, `camera 1568,1112`) → `3520`: `click screen:40,272` (`Sun Disc Thrower` 집기) → `3526`: `click screen:616,532` (적 사제 `(138,153)` 인접 `(137,151)` 배치).
   - `3620` 체크포인트 `f3620` (`Sun Disc Thrower #2` 건설 마나볼트·`300` SP 표시). 완공 후 자동 사격으로 적 사제가 무력화되며 `4160` 프레임에 `mission_gate 3 71` 전이와 함께 `'Capturing the Enemy Priest'` 안내 창이 열린다(`4160` 체크포인트 `g4160` 화면·덤프 → `4162` `click-if OK`).
2. **골렘의 `Drop Storm Crystal` · 다리 이동 · 적 사제 포획 (`4174~6200` 프레임):**
   - `4174`: `click screen:40,710` (아군 본섬 상단 복귀, `camera 1600,650`) → `4180`: `right screen:252,288` (이동 중인 골렘 `OBJ 15042` 우클릭 메뉴 `Golem Level I / Carrying: Storm Crystal / Drop Storm Crystal`, `4185` 화면 체크포인트 `g4185`).
   - **L3 대조 교정 (`e2e/l3-common.js`):** 초기 비교에서 `g4185` 만 팝업 상자 영역(`[246..426, 187..404]`) 에서 12,247픽셀(`dy=+4px`) 어긋났다. 원인 추적 결과 골렘(`flags2 & 0x30100`)은 `RawObjectMenuOpen.open()` 이 클릭 좌표가 아닌 객체 현재 좌표(`position(object)`)를 투영해 메뉴를 띄우며, 웹 마우스 우클릭 역시 좌클릭과 동일하게 `queueBrowserMouseEvent` 입력 큐를 타므로(`netstorm.js:3714,3738`) `INJECT_SHIFT.right` 를 `1` → `-1` 로 맞춰 `4180` 프레임에 동일 소비되게 교정했다. 교정 뒤 `g4185` 도 0픽셀 일치를 확인했고 기존 `l3-tutorial1.spec.js`·`l3-tutorial2.spec.js` 회귀 없음도 함께 확인했다.
   - `4195`: `click screen:303,305` (`Drop Storm Crystal`) → `4205`: `click screen:254,290` (빈손 골렘 선택) → `4215`: `click screen:472,719` (남쪽 다리로 이동 명령).
   - `5100`: `click screen:40,748` (`camera 1632,1024`, `5120` 체크포인트 `g5120` 화면·덤프: 다리 위 골렘 `(130,125)` 과 보호막 속 적 사제 `(138,153)`) → `5130`: `click screen:438,330` (골렘 선택, `5140` 화면 `g5140`).
   - `5150`: `click screen:568,653` (`RawCursorPosition.screenToWorld` 공식 기준 적 사제 `(138,153)` 타일 중앙) → 골렘이 이동해 사제를 포획, `6200` 체크포인트 `h6200` 에서 `mission_gate 3 72`·`'Mission Accomplished!'` 완료 창이 열린다(`Leave Tutorials` · `Next Tutorial`).
3. **진행 저장 및 다음 미션 해제 확인 (`inspect`):**
   - `h6200` 체크포인트 직후 `localStorage['netstorm.progress']` 에 `DoneTutorial3 === '1'` 기록을 확인하고, `'Next Tutorial'` 클릭 뒤 `window.netstorm._lastCampaignMission.name === 'Tutorial4'` 와 브리핑(`_clientGuide.current()`) 전환까지 E2E로 통과했다.

## 2. 오라클·L3·Node 26 전체 회귀

- **단계표·오라클:** `webpj/tests/fixtures/cpppj/candidate-225553df/tutorial3-l3/steps.json` 을 6,201프레임으로 확장하고 `node webpj/tools/l3-collect.mjs tutorial3-l3` 로 `f3620`, `g4160`, `g4185`, `g5120`, `g5140`, `h6200` 화면 6곳과 `g4160`, `g5120`, `h6200` 덤프 3곳을 수집했다. 기존 `a100..f3460` `.tsv` 17개는 무작위 `sp 1` XOR 키 한 줄(`1 1`)만 달라 복원했다.
- **Playwright L3 검증:**
  - `Set-Location webpj; npx playwright test e2e/l3-tutorial3.spec.js --reporter=line` → **1 passed (5.9m)** (화면 **31곳** `a100..h6200` 0픽셀 차이, Session 덤프 **20곳** 허용 행 `sp 1`·`player:*` 외 차이 0, `DoneTutorial3=1`·`Tutorial4` 전환 통과).
  - `Set-Location webpj; npx playwright test e2e/l3-tutorial1.spec.js e2e/l3-tutorial2.spec.js --reporter=line` → **2 passed (5.3m)** (`INJECT_SHIFT.right = -1` 회귀 없음).
- **manifest:** `webpj/tests/fixtures/manifest.json` 에 신규 9항목(1,012 → **1,021항목**)과 `steps`/`metadata` 해시를 갱신하고 `fixture-manifest.mjs --write --only` 를 통과했다.
- **Node 26.13.0 전체 회귀 (4/4 주기):** E1329 이후 완료 단위 4회 도달(E1330~E1333)에 따라 `npm --prefix webpj test` 를 실행해 **6,847/6,847 통과**, 실패·취소·건너뜀·미구현 0 (`402.4초`) 을 확인했다. (`fixture-manifest.test.mjs`·`bundle-fresh.test.mjs`·`port-map.test.mjs` 포함)

## 3. 남은 일

- **다음 단위:** Tutorial4(단계 A~B 고급 전투·썬 디스크 발사기와 썬 캐논 → 단계 C 안내 창) L3 교차검증.
