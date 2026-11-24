# `webpj` L3 교차검증 — `Tutorial4` 단계 A~B(고급 전투·Sun Disc Thrower vs Sun Cannon → 단계 C 진입)

- 작성 일시: 2026-11-25
- 작업 ID: `WB04-E1334`
- 선행 결과: `WB04-E1333` (`Tutorial3` 단계 F~H 완주 및 `DoneTutorial3=1` / `Tutorial4` 해제 검증)

## 1. 목적
- `Tutorial4`(`TacticalCombat`, `english`) 단계 A(생산 등록) → 단계 B(`Sun Disc Thrower` 로 적 `Sun Cannon` 2기 파괴) → 단계 C(`Upgrading Workshops, Part 1` 안내 창, `mission_gate 4 67`) 진입까지의 전 과정을 `cpppj` 오라클(`candidate-225553df`)과 고정 시각 Chromium(`webpj`)에서 비트/덤프/화면 일치(L3)로 검증한다.

## 2. 구현 및 검증 내용
1. **오라클 입력 단계표**: [steps.json](file:///h:/Workspace/git/NetstormReborn/webpj/tests/fixtures/cpppj/candidate-225553df/tutorial4-l3/steps.json) (3,211프레임, `fixedFrameMs=25`, `1024x768`)
   - **단계 A (`a100~a260`)**: 안내 창(`a100`, `a155`) 확인 후 Sun Workshop(`685,445`) 생산 메뉴(`a210`, `a220`)에서 `Wind Generator` 와 `Sun Disc Thrower` 를 각각 생산 등록해 단계 B 안내 창(`b290` `mission_gate 4 66`) 진입.
   - **단계 B 자유 진행 (`b345~b2750`)**: 남쪽 카메라 전환(`40,745`, `(1504,760)`) 뒤 아군 본섬 남단 `(130, 91)` 에 `Wind Generator #1` 배치(`b440`), 두 적 `Sun Cannon`(`(124, 135)`·`(139, 135)`) 정면 사선과 사거리를 비켜나는 중앙 회랑(`x=129..133`) 다리 연결(`b835`) 및 `(136, 110)` 에 무피해 `Wind Generator #2` 배치(`b1120`).
   - **적 `Sun Cannon` 2기 격파 및 단계 C 진입 (`c3210`)**: `(128..130, 129)`·`(125..127, 128..129)` 서쪽 연장 다리 위에 `Sun Disc Thrower` 3기(`(136, 127)`·`(127, 127)`·`(124, 129)`)를 배치해 교전(`b1550`~`b2750`) 끝에 양쪽 적 `Sun Cannon` 2기를 모두 파괴(`+400` SP 보상 포함 잔여 SP `700`)하고 단계 C `'Upgrading Workshops, Part 1'` 안내 창(`c3210`, `mission_gate 4 67`, `paused=1`)에 도달.
2. **Playwright E2E**: [l3-tutorial4.spec.js](file:///h:/Workspace/git/NetstormReborn/webpj/e2e/l3-tutorial4.spec.js) 추가
   - `e2e/l3-common.js` 로 14개 화면 스냅샷(`a100`, `a155`, `a210`, `a220`, `b290`, `b325`, `b345`, `b440`, `b835`, `b1120`, `b1550`, `b2100`, `b2750`, `c3210`) 과 10개 Session 덤프(`a100`, `b290`, `b345`, `b440`, `b835`, `b1120`, `b1550`, `b2100`, `b2750`, `c3210`) 를 대조한다.

## 3. 실행 및 통과 근거
- 오라클 수집: `node webpj/tools/l3-collect.mjs tutorial4-l3` → `frames 3211 snapshots 14종, dumps 10종` 생성.
- E2E 검증: `npx --prefix webpj playwright test --config=webpj/playwright.config.js e2e/l3-tutorial4.spec.js --reporter=line` → **`1 passed (3.1m)`** (웹 코드 수정 없음, `0` 픽셀 / `0` 비허용 덤프 차이).
