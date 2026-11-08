# webpj AI 배치 탐색(RawAiBuildSearch) 이식 — WB04-E169

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백예순아홉 번째 부분이다. 사제 AI 가 건물을 놓을 자리를 찾는 실제 574 배치 탐색 상태(패치 `0040f660` 초기화·`0040f700` 스캔·`0040f130` 빈 발자국)를 옮긴 `o/RawAiBuildSearch`(cpppj 기준선 `1f00a25a`, 기준선 뒤 이 파일과 시험의 변경 없음을 `git diff --numstat` 으로 확인)이다. AI 계열(33개 파일) 일곱 번째 단위이며 뒤의 AI 드롭 계열(`RawAiDropTargets` 등)이 이 탐색 경계(`AiBuildSearchBounds`)를 쓴다.

## 1. 옮긴 것

`www/core/raw-ai-build-search.js`(DOM 없음)

- `new RawAiBuildSearch(memory, types, spots, typeBase, hooks)` — `memory` 는 E164 `RawAiBrainMemory`, `types` 는 현재 188 TYPE({ footX, footY })의 같은 배열, `spots` 는 월드의 같은 65536 BYTE(256 × 256, 행 우선), `typeBase` 는 호스트 포인터 절단 없이 500 stride 를 보존하는 opaque32 TYPE 기준(`0049a840` 의 TYPE 레코드 포인터 산술을 같은 실제 표 번호로 대응), `hooks = { wall(), mayPlace(query) }`. 탐색 상태는 별도 좌표 목록 없이 같은 0xb08 본체의 +0x574..+0x5a7(34 BYTE)에만 쓴다.
- `initialize(token, type, bounds, avoid, owner)` — TYPE → 캐시 핸들(표 밖 TYPE 이면 이 쓰기 전에 거부, 먼저 쓴 TYPE 는 유지) → 회피 좌표 float 비트(NaN 페이로드는 `xBits/yBits` 로 보존) → owner → 최고 점수 0 → 미정 −10000.0f → 발자국을 더한 왼쪽/위와 오른쪽/아래의 signed ftol → 현재 행을 위쪽으로. 별도 clamp/inclusive 변환은 없다.
- `freeFootprint(type, x, y)` — `(spot & 0x3f) === 2` 를 x 바깥/y 안쪽 순서로 검사하며 0 이하 폭/높이는 칸 접근 없이 성공, 지도 밖 주소는 clamp 하지 않고 거부한다.
- `scan(token, budgeted)` — 0 은 전체 완료, 비영은 각 행 시작에서 벽시각이 deadline(호출 첫 시각 + float 0.03)보다 크거나 NaN 이면 중단하고 한 행은 끝까지 처리한다. 행 사이에서만 시계를 다시 읽고 행마다 현재 cursor/최대 Y 를 재조회하며 중단 시 미처리 행 cursor 를 보존해 다음 호출이 그 행부터 현재 점수를 유지해 재개한다. spot 을 통과한 칸에서만 현재 TYPE 발자국을 조회하고, 거리(x 차이만 float 저장, y 는 배정도)가 현재 점수보다 strict 하게 클 때만 전체 배치(`mayPlace`: 캐시 TYPE 핸들의 현재 역해석, 인자 0, 플래그 0, 현재 owner BYTE, 모드 1)를 묻고 허용되면 float 점수와 좌표를 저장한다. 최대 X 만 각 후보 효과 뒤 현재 값으로 다시 읽는다.

## 2. 검증

`tests/ai-build-search.test.mjs`(14건). 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 손으로 쓴 입력/기대값으로 직접 단언한다). 같은 입력과 단언을 옮겼다.

- 초기화(발자국을 더한 절삭·전체 필드·주변 34 BYTE 바깥 보존·NaN 회피 비트), 빈 발자국(3f 마스크·0 이하 폭/높이), TYPE 0/빈 범위의 시계 호출 수, strict 최대 거리와 질의 인자, 동률/거리 0/NaN 점수, spot 탈락의 지연 TYPE 접근, 행 예산 중단/재개와 deadline 동일/unordered, 행 안에서 시계 재조회 없음, 행 snapshot 과 cached TYPE/owner 현재 재조회, 최대 X 의 현재 재조회와 선택 하위 누락의 앞선 효과 보존, x 차이 float 저장/y 미축소/비교가 float 저장보다 먼저(2^51 경계), 초기화 실패의 앞선 쓰기 보존과 지도 밖 주소 거부, 생성자 거부.

최초 실패와 교정: 새 모듈과 시험은 14건 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 벽시각(`00460d70`)과 전체 배치 검사(`0049b510`, E152~E154 의 `RawCanonPlacement` 파이프라인)·월드 spot/TYPE 표 연결은 후속 AI 세션 연결 단위이며 지금은 명시 hook/배열이다. `mayPlace` 의 query 는 `RawCanonPlacement.mayPlace` 의 `{ type, argument, x, y, flags, owner, mode }` 와 같은 모양이다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

AI 드롭 계열(`RawAiDropTargets`·`RawAiDropEvaluation`·`RawAiDropScan`·`RawAiDropSelection`·`RawAiDropActions`) 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
