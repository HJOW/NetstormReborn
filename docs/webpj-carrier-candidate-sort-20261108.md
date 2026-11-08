# webpj 운반 후보 비교기·CRT qsort(RawCarrierCandidateSort) 이식 — WB04-E90

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 아흔 번째 부분이다. 골렘(Carrier) 후보 평가기가 쓰는 원본 비교기(`o/RawCarrierCandidateSort`, 428020)와 CRT qsort(4e6010/4e5fa0)의 동률 순서를 cpppj 기준선 `1f00a25a` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-carrier-candidate-sort.js` — `RawCarrierCandidateSort(pool, { poolBase, arrayBase })`

- `compare(firstAddress, secondAddress)`: 428020 은 요소의 SID 값이 아니라 qsort 가 전달한 **요소 주소**를 SID 처럼 계산한다. 인자를 역참조하지 않고 `주소×50 + 풀 시작 주소` 를 32비트로 감아 만든 두 주소가 풀의 `[시작+50, 시작+용량×50)` 안인지 본 뒤(둘 다 밖이면 0, 첫째만 밖이면 1, 둘째만 밖이면 −1), 모두 안이면 현재 raw 의 +8 위치 unsigned WORD 차이를 돌려준다(슬롯 중간을 가리키는 감김 주소도 나눗셈으로 정렬하지 않고 슬롯 경계를 넘어 읽으며, 실제 확보 범위 밖 읽기는 거부).
- `sort(candidates, onCompare)`: 폭 4 의 CRT qsort 이다. 8개 이하 구간은 엄격한 `>` 로 최대 위치를 고른 뒤 끝과 교환하는 선택 정렬(동률도 회전), 큰 구간은 세 위치의 중앙 pivot·pivot 앞/뒤 전진 조건·pivot 교환 뒤 재배치·pivot 오른쪽/왼쪽 동률 구간 제외(`goto` 둘을 구조화한 같은 분기)를 거쳐 30쌍 스택에 큰 구간을 두고 작은 구간부터 처리한다. 후보 값(SID)은 읽지 않으며 비교기는 교환 뒤 현재 위치의 요소 주소(`arrayBase + 4i`)를 다시 받는다.
- 웹에는 호스트 포인터가 없으므로 원본 32비트 주소는 명시 입력이다(`poolBase` 풀 시작, `arrayBase` 정렬 배열 시작; 비원본 기본값은 임의 상수). `onCompare` 는 비교 호출의 요소 주소 쌍을 관찰하는 선택 훅이다.

## 2. 검증

- `tests/carrier-candidate-sort.test.mjs`(2건): `carrier-candidate-sort-x86.tsv` 513행 — 직접 비교(주소 둘의 유효/무효/감김 조합)와 정렬(개수 0..500, 풀 패턴 0/1 의 전체 24000×50 raw, 값 패턴: 오름/내림/SID 범위 밖 큰 값)의 반환값, 정렬 뒤 배열, 비교 호출 주소 열(쌍의 순서), 배열 Adler-32 가 같고 raw 와 정렬 범위 밖 요소는 변하지 않는다. 계약: 범위 밖 주소 비교의 0/±1, 빈/단일 배열, CD 풀 거부.

최초 실패와 교정: 새 모듈은 위 fixture 513행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 후보 평가기(`RawCarrierChoice` 류)가 이 정렬을 부르는 조립과, 호스트가 아니라 풀 안 SID 를 가리키는 실제 주소 체계(웹에서는 의미 있는 값이 아닌 명시 주소)의 연결은 해당 단위에서 한다.

## 4. 다음 시작점

`RawCarrierRegular`·`RawCarrierSteal`·`RawCarrierDrop`(Carrier 계열), `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
