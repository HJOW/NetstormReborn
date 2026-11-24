# `webpj` L3 교차검증 — `Tutorial4` 단계 C~E(`Sun Workshop` 업그레이드·`Sun Cannon` 으로 적 발사기 6기 파괴 → 단계 E1 진입)

- 작성 일시: 2026-11-25
- 작업 ID: `WB04-E1335`
- 선행 결과: `WB04-E1334` (`Tutorial4` 단계 A~B → 단계 C 진입 3,210프레임 검증)

## 1. 목적
- `Tutorial4`(`TacticalCombat`, `english`) 단계 C(`Upgrading Workshops, Part 1`) → `Sun Workshop` Level II 업그레이드 및 `Sun Cannon` 생산 등록(`단계 D`) → `Sun Cannon` 배치로 적 `Sun Disc Thrower` 6기 격파(`단계 E`) → 단계 E1(`Sacrificing Priests, Part 1`) 안내 창 진입까지의 전 과정을 `cpppj` 오라클(`candidate-225553df`)과 고정 시각 Chromium(`webpj`)에서 비트/덤프/화면 일치(L3)로 검증한다.

## 2. 핵심 설계·자원 원인 규명 및 조치
1. **단계 C~D SP 병목 해소(단계 B 발사기 2기 운용 → 단계 C 진입 시 `SP = 1,000`)**
   - `originals d/sunWalker.type` 및 `RiftTypeTable.cpp` 기준 `Golem` 생산 비용은 `400 SP`(`Sun Cannon`·`Wind Generator` 각 `400 SP`, `Sun Workshop` Level II 업그레이드 `800 SP`)이며, `Thrower`(`300 SP`) 해체 환급은 `+75 SP`, `Wind Generator` 해체 환급은 `+100 SP`다.
   - 기존 `Thrower` 3기 배치(`1560~1565`의 3번째 발사기)로는 단계 C 잔여 SP가 `700`에 그쳐 해체·업그레이드 뒤 `Sun Cannon(400 SP)`을 지을 SP가 부족하므로, `[1560, 1565]` 3번째 발사기 배치만 생략하고 사선 밖 2기(`(136,127)`·`(127,127)`)만 유지해 단계 C(`c3500`)에 **`SP = 1,000`** 으로 진입하도록 단계표([steps.json](file:///h:/Workspace/git/NetstormReborn/webpj/tests/fixtures/cpppj/candidate-225553df/tutorial4-l3/steps.json))를 정리했다(`1~1550` 프레임 체크포인트 `a100~b1550` 10곳은 변경 없음).
   - 단계 C(`SP 1,000`) → `Thrower` 2기 해체(`+150`, `1,150`) → `Sun Workshop` 업그레이드(`-800`, `350`) → 단계 D(`d4755`) → `Wind Generator #1(130,91)` 해체(`+100`, `450 SP`; `(136,110)` `#2`는 남쪽 전력 공급용으로 유지).
2. **외부 사거리·개방 다리 연결을 만족하는 `Sun Cannon` 단기 배치 `(131, 124)`**
   - `Sun Disc Thrower` 사거리는 `9`, `Sun Cannon` 사거리는 `20`이며, 아군 기존 다리 `(129,125) (NS)`·`(132,122..124)`와 맞물리는 개방 하늘 `(131, 124)`(`screen:552,576`, `[129..131, 122..124]`)에 `Sun Cannon` 1기(`400 SP`, 잔여 `50 SP`)를 배치하면 적 발사기 사거리 밖에서 무피해(`hp=600`)로 사격해 파편 연쇄 폭발로 6기(`15008/10/12/14/17/19`)를 모두 격파(`+900 SP` 보상 → **`SP = 950`**)한다.
   - `7200` 프레임(`e7200`)에서 `'Enemy Sun Disc Throwers Destroyed'`(`MORE`) 확인 후 `7215`에 `MORE` 클릭으로 `7240` 프레임(`e7240`)에 단계 E1 `'Sacrificing Priests, Part 1'`(`BACK`/`OK`, 잔여 `950 SP`) 안내 창에 진입한다.

## 3. 구현 및 검증 내용
1. **오라클 단계표·픽스처 갱신**: [steps.json](file:///h:/Workspace/git/NetstormReborn/webpj/tests/fixtures/cpppj/candidate-225553df/tutorial4-l3/steps.json) (7,241프레임, `fixedFrameMs=25`, `1024x768`)
   - 오라클 수집: `node webpj/tools/l3-collect.mjs tutorial4-l3` (`c3210.*` 삭제 후 `c3500`, `d4755`, `d5400`, `e7200`, `e7240` 추가, 총 스냅샷 18종 · 덤프 14종).
   - 이식 추적 목록: [manifest.json](file:///h:/Workspace/git/NetstormReborn/webpj/tests/fixtures/manifest.json) `1,055`항목 갱신(`node webpj/tools/fixture-manifest.mjs --write --only ...`).
2. **Playwright E2E 확장**: [l3-tutorial4.spec.js](file:///h:/Workspace/git/NetstormReborn/webpj/e2e/l3-tutorial4.spec.js)
   - `a100~e7240` 18개 화면 스냅샷과 14개 Session 덤프를 cpppj 오라클과 대조.

## 4. 실행 및 통과 근거
- E2E 검증: `npx --prefix webpj playwright test --config=webpj/playwright.config.js e2e/l3-tutorial4.spec.js --reporter=line` → **`1 passed (5.0m)`** (웹 코드 수정 없음, 18개 화면 `0` 픽셀 / 14개 덤프 허용 행 외 `0` 차이).
- 정적 검증: `fixture-manifest.test.mjs`, `bundle-fresh.test.mjs`, `port-map.test.mjs`, `node webpj/tools/port-map.mjs` 통과.
