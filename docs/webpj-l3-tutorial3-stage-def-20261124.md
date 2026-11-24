# webpj WB04-E1332 — Tutorial3 단계 D~E(`Wind Generator` 생산·적 섬 에너지 확장·`Sun Disc Thrower` 배치)→단계 F(`Incapacitating the Enemy Priest`) L3 교차검증 + Node 22 전체 회귀

- **작성일 / 단위:** 2026-11-24 / WB04-E1332 (커밋 `20261124-99 WB`, `webpj/www/` 무변경)
- **비교 기준선:** `cpppj 225553df`, 오라클 실행 파일 `extracted/webpj-oracle/225553df/Netstorm.exe` (SHA-256 `5b52d96d2e53f4674a40991b78ff64c3c7e2ad8275a0a99b22d18d532b77eb46`), 고정 프레임 `--fixed-frame-ms 25`, 1024×768 영어(`WelcomeGiven=1;tellTips=0;autoDemo=0`).
- **전 단위:** [단계 C→D 기록(WB04-E1331)](webpj-l3-tutorial3-stage-cd-20261124.md)

## 1. 단계 D~E → 단계 F 진행 요약

1. **Wind Generator 생산 추가 (`2370~2475` 프레임):**
   - `2370`: `right screen:480,355` (Sun Workshop) → `2380`: `click screen:560,380` (`Put Knowledge into Production >`, `2384` 화면 체크포인트 `d2384`) → `2390`: `click screen:760,380` (`Wind Generator` 선택).
   - 좌측 생산 패널 4번째 슬롯(`screen:40,364`)에 `Wind Generator` 가 등록되며 `mission_gate 3 69` 로 전이해 `2415` 프레임에 `'Extending Energy, Part 1'` 안내 창이 열린다 (`2435` 체크포인트 `e2435` 화면·덤프).
   - `2445`: `click-if MORE` → `'Extending Energy, Part 2'`(`2460` 화면 `e2460`) → `2475`: `click-if OK` 로 단계 E 자유 진행.
2. **에너지 중계 및 적 섬 위 Sun Disc Thrower 건설 (`2490~3460` 프레임):**
   - `2490~2496`: 아군 Sun Temple(`(125,92)`) 에너지 범위(반경 22칸, `484`) 안의 남단 해안 `(129,110)`(`screen:520,455`)에 첫 번째 `Wind Generator` 배치 (`2510` 체크포인트 `e2510` 화면·덤프: `400` 비용·마나 볼트 비행).
   - `2800~2806`: 완공된 첫 `Wind Generator` 의 바람 에너지 범위 안인 적 섬 북단 `(134,131)`(`screen:616,708`) 에 두 번째 `Wind Generator` 배치 (`3000` 체크포인트 `e3000` 화면·덤프: 두 기 모두 완공·적 섬 에너지 확보).
   - `3150~3156`: 두 번째 `Wind Generator` 의 에너지 범위 안에서 겹치지 않는 적 섬 빈 칸 `(132,134)`(`screen:584,741`) 에 첫 `Sun Disc Thrower` 배치 (`3240` 체크포인트 `e3240`: `300` 비용·다리를 따라 적 섬으로 비행하는 마나 볼트).
   - `3360` 프레임에 `Sun Disc Thrower` 완공으로 `mission_gate 3 70` 전이, `3380` 프레임 `'Incapacitating the Enemy Priest'` 안내 창 열림 (`3420` 체크포인트 `f3420` 화면·덤프 → `3440` `click-if OK` → `3460` `f3460` 화면·덤프).

## 2. 오라클·L3·Node 22 전체 회귀

- **단계표·오라클:** `webpj/tests/fixtures/cpppj/candidate-225553df/tutorial3-l3/steps.json` 을 3,461프레임으로 확장하고 `node webpj/tools/l3-collect.mjs tutorial3-l3` 로 `d2384`, `e2435`, `e2460`, `e2510`, `e3000`, `e3240`, `f3420`, `f3460` 화면 8곳과 `e2435`, `e2510`, `e3000`, `f3420`, `f3460` 덤프 5곳을 수집했다. 기존 `a100..d2360` `.tsv` 는 실행마다 달라지는 무작위 `sp 1` XOR 키 한 줄만 차이 남을 확인한 뒤 복원했다.
- **Playwright L3 검증:**
  - `Set-Location webpj; npx playwright test e2e/l3-tutorial3.spec.js --reporter=line` → **1 passed (3.8m)**
  - 체크포인트 화면 **25곳**(`a100..f3460`) 모두 cpppj 오라클과 **0픽셀 차이**, Session 덤프 **17곳**(`a100..f3460`) 모두 허용 행(`sp 1`·`player:*`) 외 **차이 0**으로 통과했다.
  - 다리를 가로질러 적 섬으로 날아가는 마나 볼트 궤적·비용 부유 숫자(`400`·`300`)·제네레이터 완공·에너지 원 반경 판정·적 섬 발사기 배치/완공·단계 F 진입까지 웹 구현이 원본 10.78과 동일함을 확인했다.
- **manifest:** `webpj/tests/fixtures/manifest.json` 에 신규 13항목(999 → 1,012항목)과 `steps`/`metadata` 해시를 갱신했다.
- **Node 22.23.5 전체 회귀 (8/8 주기):** E1324 이후 완료 단위 8회 도달에 따라 `..\extracted\webpj-node22\node_modules\node-win-x64\bin\node.exe --test "tests/*.test.mjs"` 를 순차 실행해 **6,847/6,847 통과**, 실패·취소·건너뜀·미구현 0 (`469.0초`) 을 확인했다.

## 3. 남은 일

- **다음 단위:** Tutorial3 단계 F~H (적 사제 무력화·골렘 포획 → `'Mission Accomplished!'` 완료 창 및 Tutorial4 해제) L3 교차검증.
