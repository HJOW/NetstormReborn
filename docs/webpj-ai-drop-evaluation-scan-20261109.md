# webpj AI 배치 평가(RawAiDropEvaluation)·배치 스캔(RawAiDropScan) 이식 — WB04-E173

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백일흔세 번째 부분이다. AI 가 건물을 놓을 후보 좌표를 평가·승인하는 `o/RawAiDropEvaluation`(패치 `00412640` 부족 거리·`004126c0` 공급 점수·`00412ab0` 시설별 평가 분배·`00412b90` 후보 승인)과, 표면 지도/다리 J·K 주변 후보에서 30개 선택 목록을 만드는 `o/RawAiDropScan`(패치 `00412ce0` 다리 보정·`00412e10` 스캔·`0040f390` 제거)을 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 두 파일의 .cpp/.h 와 시험의 변경 없음을 `git diff --numstat` 으로 확인). AI 계열(33개 파일) 열두 번째 단위이다. 앞 단위의 `RawAiDropTargets`(E172)·`RawAiDrySpots`(E170)를 쓴다.

## 1. 옮긴 것

`www/core/raw-ai-drop-evaluation.js`(DOM 없음)

- `new RawAiDropEvaluation(memory, pool, types, targets, globals, hooks)` — `targets` 는 E172 `RawAiDropTargets`(같은 풀), `globals = { manaRadius(), probeType() }`(매 평가 위치에서 현재 값), `hooks = { collect, mayPlace, energy, dry, unsupported, dryLog }`.
- `dryDistance(token, point, box)` — 현재 개수/포인터에서 매 기록을 재조회하며 정확한 0 거리는 제외하고 NaN 은 건너뛰고 `1000 − Manhattan` 의 첫 최고(비교는 축소 전 값, 저장은 float)와 index 를 구한다. `supplyScore` — 점수가 `1000 − 2 × 반경` 보다 크고 현재 공급 C 문자열의 길이가 0 이 아니며 현재 +0xad8 보다 작을 때만 `ftol(저장 float 점수) / (길이 + 1)`. `evaluate` — flags2 0x800 공급 → 0x4000000 블로커 → group 3 지상 → group 0/1 Gun → group 6 무진단 0, 그 밖은 현재 +0xb00 이면 진단 뒤 0.
- `consider`/`considerStored` — 전체 배치 → 평가 → 양수 동률도 허용하는 개선 판정 → 실제 에너지 승인(DWORD) → 승인되면 최고 Y → X → 점수, 아니면 실제 부족 좌표를 추가하고 성공 시 현재 +0xb00 이면 로그. `considerStored` 는 실제 스캔의 점수 포인터(+0x3e8)를 매 읽기/쓰기 위치에서 직접 쓴다.

`www/core/raw-ai-drop-scan.js`

- `new RawAiDropScan(memory, pool, types, frames, spots, surfaces, alliances, random, globals, hooks)` — 월드 지도(spot 65536·표면 WORD 65536·방향별 동맹 81)와 원본 공용 `GameRandom`, `globals = { bridgeType(), bounds() }`, `hooks = { free, consider, assertion }`.
- `adjustBridge(surface, cell)` — 표면이 현재 다리 TYPE 이면 프레임 코드 side J 는 상/하, K 는 좌/우를 전체 BYTE 0 세 칸으로 검사해 기준 축을 먼저 쓰고(J 의 위쪽/K 의 왼쪽 우선) 다른 축에 공용 난수 3 을 더한다(실패에서는 난수 무소비). `scan(token, type, direction)` — x 를 먼저 증가시키고 inclusive 우측/하단 소진 뒤 좌상단으로 되돌리며(false) flags2 0x800 은 예산 500/그 밖 1000 에서 칸마다 −1, 표면이 있으면 −5, 다리 후보 −125, 일반 후보(동맹 행 `표면 owner*9 + owner`·같은 owner 의 전체 발자국 −10)를 승인하고, 점수(+0x3e8)를 strict 로 올리면 30개 목록을 재시작/동률이면 추가/30개가 가득하면 공용 난수로 한 칸 교체한다. `remove` — count 가 1 미만이면 진단 뒤에도 계속해 count 를 먼저 줄이고 좌표 → 표적 순으로 안정 압축하며 꼬리/점수/진행은 보존한다.

## 2. 검증

`tests/ai-drop-evaluation.test.mjs`(17건)·`tests/ai-drop-scan.test.mjs`(17건). 두 몸체 모두 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 손으로 쓴 입력/기대값으로 직접 단언한다). 같은 입력과 단언을 옮겼다: 평가 쪽은 빈/음수 count, 0 거리·NaN·거리 1000, 축소 전 비교/float 저장, 반경 경계/NaN 반경, C 문자열 길이 한도, 분배 우선순위와 출력 보존, 미지원 group 진단, 지상 결과 선기록, 전체 배치 인자, 현재 owner 재조회, 양수 동률/음수 개선, 비채택 시 대상 효과 보존, 부족 좌표 추가/로그 중복, 선택 하위 누락 prefix, 저장 점수의 현재 읽기/쓰기. 스캔 쪽은 J/K 다리 보정(우선/대체/난수 무소비/선형 행 이웃/비다리 TYPE), 커서 선증가/소진 복귀, 예산 차감(일반·공급·다리), 동맹 방향 행과 자기 발자국, 관문, 호출별 snapshot, 목록 재시작/동률 추가/30개 교체, 거부 하위의 현재 점수/count, 후보 하위 누락 시 진행/난수 보존, 제거의 안정 압축과 진단 뒤 계속, 생성자 거부.

최초 실패와 교정: 새 모듈과 시험은 34건 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 공급 문자열(`004730c0`)·에너지 승인(`004734d0`)·부족 좌표 로그(`004105b0`)·전체 배치 검사(E152~E154 `RawCanonPlacement`)·후보 승인(`00412b90` = 이 모듈의 `considerStored`)을 스캔의 `consider` hook 에 연결하는 일은 후속 AI 세션 연결 단위이다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

AI 드롭 계열 나머지(`RawAiDropSelection`·`RawAiDropActions`)·`RawAiDeployment`·`RawAiResourceChoice` 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
