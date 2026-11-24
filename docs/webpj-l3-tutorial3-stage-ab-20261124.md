# webpj L3: Tutorial3 단계 A→B 확장과 번개 프로세스(`ThunderProcess`) 연결 (WB04-E1329, 2026-11-24)

## 요약

1. **Tutorial3 (Capture The Priest) L3 대조 확장(240 → 1,700프레임)**: `tests/fixtures/cpppj/candidate-225553df/tutorial3-l3/steps.json`을 기존 시작 안내 창(240프레임)에서 **단계 A 전체(북서쪽 간헐천 직결·골렘 배치·완성·폭풍 크리스털 수확·신전 배달 SP 1600→1800) → 단계 B(`Capturing Enemy Priests, Part 1`/`Part 2` 안내 창 및 자유 진행)**까지 확장했다.
2. **실전 엔진 차이 발견 및 수정(`ThunderProcess` TYPE 55 미연결)**: 확장 L3 대조 중 9개 화면 체크포인트는 모두 0픽셀 일치했으나, `b900` 덤프에서 `pool`/`slot:0`의 `freeHead`가 cpppj `90` 대 webpj `89`로 정확히 1칸 차이 났다. 프레임별 추적으로 **프레임 404**에서 주변음(`StepAmbientSound`) 천둥 번개가 발생할 때 cpppj(`Client::StartThunderFlash`)가 창 모드에서도 `ThunderProcess`(TYPE 55, `flags 0x10`)를 실제 `ProcessHost`에 부착해 1프레임 뒤 자체 반납하면서 저번호 풀을 1칸 소비하는 반면, webpj는 빈 훅(`flash: () => {}`)으로 두어 부착하지 않았음을 확인했다.
3. **수정 및 재검증**: 이미 이식돼 있던 `www/core/thunder-process.js`를 `www/netstorm.js`(`_startThunderFlash`, `_clearThunderFlashes`)와 `www/audio/scene-music-driver.js`(`thunderFlash` 훅)에 원본과 동일하게 연결하고 번들을 다시 빌드한 뒤, Chromium `e2e/l3-tutorial3.spec.js`에서 **9개 화면 전부 0픽셀 차이·7개 덤프(`a100·a240·b340·b900·b1200·b1600·b1700`) 허용 밖 차이 0건**으로 통과했다.

## 입력 단계표 확장 (`tutorial3-l3/steps.json`, 1024×768 고정 25ms)

| 프레임 | 동작 | 인자 | 결과 |
| --- | --- | --- | --- |
| 100~240 | 기존 도입 구간 | `a100`, `a160`, `a240` | Capture The Priest! → Build Golems → 단계 A 자유 진행 |
| 250 / 256 / 264 | `click` → `right` → `click` | `60,44` → `272,202` | 2칸 직선 다리 가로 회전 후 `(113..114, 85)` 배치(내 섬 연결) |
| 310 / 316 / 324 | `click` → `right` → `click` | `60,44` → `232,202` | 리필된 3칸 직선 다리 가로 회전 후 `(110..112, 85)` 배치(북서쪽 간헐천 `(109, 85)` 직결) |
| 340 | `snapshot`·`session-dump` | `b340` | 5칸 직선 다리 완성(`bridges=5`) |
| 350 / 360 | `click` → `click` | `40,180` → `368,296` | Sun Walker(골렘) `(119, 95)` 배치(SP 2000 → 1600) |
| 404 | (내부 이벤트) | `StepAmbientSound` 천둥 | `ThunderProcess`(TYPE 55, SID 89, 부모 SID 5) 부착 → 405프레임 자체 종료(`freeHead 89→90`) |
| 900 | `snapshot`·`session-dump` | `b900` | 골렘 완성(`hp=50`, `freeHead=90`, `objectNext=15043`) |
| 920 / 940 | `click` → `click` | `360,290` → `186,185` | 골렘 선택 후 북서쪽 간헐천 수확 명령 |
| 1200 | `snapshot`·`session-dump` | `b1200` | 골렘이 폭풍 크리스털을 들고 다리 `(113.81, 85)` 횡단 중 |
| 1600 | `snapshot`·`session-dump` | `b1600` | 신전 배달 완료(SP 1600 → 1800), 단계 B `Capturing Enemy Priests, Part 1` 안내 창 |
| 1620 / 1640 | `click-if` / `snapshot` | `MORE` / `b1640` | `Capturing Enemy Priests, Part 2` 안내 창 |
| 1660 / 1700 | `click-if` / `snapshot`·`session-dump` | `OK` / `b1700` | 단계 B 자유 진행 재개(`freeHead=100`, `objectNext=15047`) |

## 코드 변경

- `webpj/www/netstorm.js`:
  - `ThunderProcess`, `THUNDER_PROCESS_TYPE` 연결(`_thunderProcesses` 목록, `_startThunderFlash(brightness)`, `_clearThunderFlashes()`).
  - 원본 `Client::StartThunderFlash(00470d40)`와 동일하게 `playerKnown`, `ownerMode.challenge`, `players[battleOwner * 180 + 0x9c]`의 기술 부모 SID를 읽어 `ProcessHost.attach(..., THUNDER_PROCESS_TYPE, parent, 0x10)`로 부착하고, 세션 정리(`_releaseCampaignWorld`)에서 `_clearThunderFlashes()`를 호출한다.
  - `_runAmbientFrame`의 `flash`, `_campaignChatClient`의 `requestThunderFlash`, `_beginSceneMusic`의 `thunderFlash`를 모두 `_startThunderFlash`에 연결했다.
- `webpj/www/audio/scene-music-driver.js`: `SceneMusicDriver` 생성자에 선택 인자 `thunderFlash = () => {}`를 추가해 `SceneMusic`의 `thunderFlash` 훅에 연결했다.
- `webpj/www/dist/netstorm.bundle.js`: 프로덕션 번들 재빌드(3,433,729바이트 / 3.27MiB, 한도 9.9MB 통과).
- `webpj/tests/scene-music-driver.test.mjs`: 천둥 곡(`thu22.mus`) 시작 시 `thunderFlash`가 효과음 앞에 호출되고 비 곡(`rain22.mus`)에서는 호출되지 않음을 단위 시험으로 추가했다.
- `webpj/e2e/l3-tutorial3.spec.js` 및 `webpj/tests/fixtures/manifest.json`: 신규 체크포인트 11개 오라클 파일 등록(총 986개 항목).

## 검증

- Chromium L3·회귀: `npx playwright test e2e/l3-tutorial3.spec.js --reporter=line` → 1/1 통과(2.1분, 9개 화면 0픽셀·7개 덤프 완전 일치), `e2e/session-services-wiring.spec.js`·`e2e/fixed-time.spec.js` → 5/5 통과(1.7분).
- 추적·번들·단위 검사: `port-map.mjs --check`(1,160행 오류 0, `todo 0 / partial 0`), `fixture-manifest.test.mjs`, `port-map.test.mjs`, `bundle-fresh.test.mjs`, `scene-music-driver.test.mjs` 통과.
- Node v26 전체 회귀(`npm --prefix webpj test`, Node `v26.13.0`, 4/4 주기): **6,847/6,847 통과**, 실패·취소·미실행 0(500.1초).

## 다음 단계

- Tutorial3 단계 B(남쪽 적 섬 다리 직결) → 단계 C(Sun Disc Thrower 생산 투입) → 단계 D(Wind Generator 생산 투입) → 단계 E~H(적 사제 포획) 확장, 또는 Tutorial4~6 L3 진입.
