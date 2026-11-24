# webpj WB04-E1331 — Tutorial3 단계 C(`Sun Disc Thrower` 생산)→단계 D(`Generators, Part 1/2`) L3 교차검증

- **작성일 / 단위:** 2026-11-24 / WB04-E1331 (커밋 `20261124-98 WB`, `webpj/www/` 무변경)
- **비교 기준선:** `cpppj 225553df`, 오라클 실행 파일 `extracted/webpj-oracle/225553df/Netstorm.exe` (SHA-256 `5b52d96d2e53f4674a40991b78ff64c3c7e2ad8275a0a99b22d18d532b77eb46`), 고정 프레임 `--fixed-frame-ms 25`, 1024×768 영어(`WelcomeGiven=1;tellTips=0;autoDemo=0`).
- **전 단위:** [단계 B→C 기록(WB04-E1330)](webpj-l3-tutorial3-stage-bc-20261124.md)

## 1. 단계 C → 단계 D 진행 요약

1. **Sun Workshop 생산 메뉴 입력 (`2210~2230` 프레임):**
   - `2210`: `right screen:480,355` — Sun Workshop (`OBJ 15003`, `type=85`) 우클릭 메뉴(`View Current Production >`, `Put Knowledge into Production >`, `Upgrade costs 800`, `Salvage gains 200` 등) 열기 (`2214` 프레임 화면 체크포인트 `c2214`). 참고로 북동쪽 작은 구조물(`OBJ 15001`, `type=89`, `screen:560,263`)은 Residence(`sunResidence`)다.
   - `2220`: `click screen:560,380` — `Put Knowledge into Production >` 클릭해 `Knowledge Available / Sun Disc Thrower` 하위 메뉴 열기 (`2224` 프레임 화면 체크포인트 `c2224`).
   - `2230`: `click screen:760,380` — `Sun Disc Thrower` 선택. 좌측 생산 패널 3번째 슬롯에 `Sun Disc Thrower` 가 등록되며 `mission_gate 3 68` 로 전이한다.
2. **단계 D 안내 창 전환 (`2255~2360` 프레임):**
   - `2255` 프레임에 `'Generators, Part 1'` 안내 창(`MORE` 단추 포함)이 열려 `2280` 프레임 체크포인트 `d2280`(화면 + 덤프) 을 기록했다.
   - `2300` 프레임 `click-if MORE` 로 `'Generators, Part 2'`(`Rain/Wind/Thunder Generator` 그림·설명 3종 및 `BACK`/`OK` 단추, `2320` 프레임 화면 체크포인트 `d2320`) 을 확인한 뒤 `2340` 프레임 `click-if OK` 로 닫고 `2360` 프레임 `d2360`(화면 + 덤프, 단계 D 자유 진행 재개) 까지 확장했다.

## 2. 오라클·검사·manifest 갱신

- **단계표·오라클:** `webpj/tests/fixtures/cpppj/candidate-225553df/tutorial3-l3/steps.json` 을 2,361프레임으로 확장하고 `node webpj/tools/l3-collect.mjs tutorial3-l3` 로 `c2214`, `c2224`, `d2280`, `d2320`, `d2360` 화면(`.png`)과 `d2280`, `d2360` 덤프(`.tsv`)를 수집했다. 기존 `a100..c2200` `.tsv` 는 실행마다 달라지는 무작위 `sp 1` XOR 키 한 줄만 다름을 확인한 뒤 기존 고정 해시 파일을 유지했다.
- **Playwright L3 검증:**
  - `Set-Location webpj; npx playwright test e2e/l3-tutorial3.spec.js --reporter=line` → **1 passed (2.7m)**
  - 체크포인트 화면 **17곳**(`a100..d2360`) 모두 cpppj 오라클과 **0픽셀 차이**, Session 덤프 **12곳**(`a100..d2360`) 모두 허용 행(`sp 1`·`player:*`) 외 **차이 0**으로 통과했다.
  - 웹 소스(`webpj/www/`) 변경 없이 통과해 Sun Workshop 우클릭 메뉴·하위 지식 생산 메뉴·`Sun Disc Thrower` 생산 슬롯 등록·단계 D `'Generators, Part 1/2'` 창과 아이콘 출력이 원본 10.78과 동일함을 확인했다.
- **manifest:** `webpj/tests/fixtures/manifest.json` 에 신규 7항목(992 → 999항목)과 `steps`/`metadata` 해시를 갱신했다.

## 3. 남은 일

- **다음 단위:** Tutorial3 단계 D~E (`Wind Generator` 생산·적 섬까지 에너지 확장·첫 `Sun Disc Thrower` 배치 → 단계 F `'Incapacitating the Enemy Priest'`) L3 교차검증.
