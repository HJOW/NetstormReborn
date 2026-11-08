# webpj 썬 바리케이드 재생성(RawFenceRebuild) 이식 — WB04-E81

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 여든한 번째 부분이다. 썬 바리케이드(Fence)의 연결 재생성 제어(`o/RawFenceRebuild`, 00457460)를 cpppj 기준선 `1f00a25a` 에서 옮겼다. 같은 fixture(`fence-create-x86.tsv`)의 표식(M)/포스필드(S) 생성 행은 중심 계산기(`RawGunTarget`)가 선행이라 `RawFenceCreate` 단위로 남기고, 이번에는 재생성(R) 166행만 쓴다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 배치는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-fence-rebuild.js` — `RawFenceRebuild(pool, hooks)`

- `rebuild(parent)`: 부모 extra 비트 0·3(9)이 있으면 진입에서 반환한다(진입 때만 검사). 없으면 네 방향 0·2·4·6 을 순서대로: 동맹 최근접 이웃(`neighbor`) 조회, 없으면 다음 방향 → 부모와 이웃의 **현재** 좌표 float 4개로 사거리 판정(`withinRange`) → 사거리 밖이거나 이웃의 **현재** extra 가 0x21 비트를 가지면 다음 방향 → 내부 표식 생성(`markers`) → 표식 효과 뒤의 현재 소유자 둘과 현재 좌표로 포스 필드 생성(`shields(좌표4, 부모 소유자, 이웃 소유자, 부모 SID, 이웃 SID)`).
- 좌표 사본을 보관하지 않고 각 호출 직전에 현재 raw 를 읽는다(NaN payload 보존). 선택하지 않은 경계는 요구하지 않고 선택한 경계의 누락만 거부하며, CD 판본은 거부한다.

## 2. 검증

- `tests/fence-rebuild.test.mjs`(2건): `fence-create-x86.tsv` 재생성(R) 166행 — 부모 extra·이웃 존재 비트·사거리 비트·이웃 extra 네 BYTE·외부 효과 변이(좌표 변경·이웃 extra 변경·소유자/좌표 변경·부모 extra 변경)에서 부모+네 이웃(SID 50..54)의 전체 50바이트 raw 와 호출 순서(Neighbor/Range/Markers/Shields의 float 비트·소유자·SID)가 같다. 계약: 부모 extra 로 막히면 하위 효과를 요구하지 않고, 선택한 경계의 누락은 거부하며, 이웃이 없으면 사거리/표식/포스필드를 요구하지 않고 CD 풀은 거부한다.

최초 실패와 교정: 새 모듈은 위 fixture 166행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 동맹 최근접 검색(`00455fb0`)·사거리 판정(`00455f00`)·표식 생성(`00456460`)·포스필드 생성(`00455b20` mode 1)의 실제 연결은 해당 단위에서 한다(`raw-fence-line.js` 는 E74 의 mode 0 삭제 쪽). `RawFenceCreate`(표식 M 106·포스필드 S 306)는 `RawGunTarget` 이식 뒤에 같은 fixture 로 이어서 대조한다.

## 4. 다음 시작점

`RawGunTarget`→`RawFenceCreate`(표식·포스필드 생성), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawShapeToBuffer`(+ 캡처 계열), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
