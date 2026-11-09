# webpj 미니맵 셀 그림(RawMinimapRaster) 이식 — WB04-E204

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "그림·캡처" 묶음의 세 번째 부분이다. 미니맵 한 셀(2×2 픽셀)을 같은 월드의 지면 WORD 와 공간 해시 체인에서 그리는 원본 004808d0 을 옮기고, cpppj `MinimapRasterTests` 의 원본 4722행 전체 bitmap 을 재생했다.

- cpppj 기준선 `1f00a25a`. 소스·시험·fixture 모두 기준선 뒤 변경 없음. 근거 커밋 `560df210`(소스·시험·fixture 같음).
- 원본 fixture: `minimap-cell-x86.tsv` 4722행(두 x87 정밀도 관측, SHA-256 `9a40c0ed…`).
- 공유 장치:
  - WB04-A `core/sid-pool.js`(물리 슬롯 `physical`·읽기 사본 `slot`)
  - WB04-B `core/squid-hash.js`(`cell(level, x, y)` = cpppj `SquidHash::Cell`, 지면 WORD = 0단계 머리)
  - WB04-D2 `core/metadisplay-dirty.js`(같은 128×128 bitmap 의 소유자, 아직 연결 전)

## 1. 옮긴 것

`www/core/raw-minimap-raster.js`(DOM 없음, 10.78 전용)

- `createMinimapRasterState(options)`: 셀 그림이 매 호출 읽는 현재 전역(cpppj `MinimapRasterState`)이다.
  - 두 지면 TYPE(isle 97·noIsland 157)과 Challenge 표식 TYPE(battleIsland 109·playerIsland 108)
  - 모드·색 억제·억제 시 색으로 쓰는 프레임 DWORD
  - 검정·두 중립 지면·Challenge 초록·소유자 1..8 미니맵 전용 색
- `RawMinimapRaster(pool, hash, types, state).drawCell(bitmap, cellX, cellY)`(004808d0)
  1. 셀의 2×2 를 검정 전역으로 초기화한다(색 억제와 무관).
  2. 지면 4×4 를 Y 바깥/X 안쪽으로 읽는다. 0단계 머리만 보고 next 를 따라가지 않으며, 두 지면 TYPE 만 소유자 색(없으면 중립 색)으로 한 점을 쓴다.
  3. 해시 2단계 셀 체인(Challenge 표식 없음) → 1단계 2×2(X 바깥/Y 안쪽) → 0단계 4×4(X 바깥/Y 안쪽) 체인을 차례로 덮는다.
  4. 체인 효과:
     - 일반 모드: TYPE flags2&6 이면 좌표를 먼저 절삭하고, &4 일 때 소유자 색(없으면 246)으로 한 점을 쓴다.
     - Challenge 모드: 1·0단계 체인의 두 표식 TYPE 만 signed /2 뒤 bit0 을 지운 짝수 정렬 2×2 를 그린다(playerIsland 는 소유자 5 칸 색).
  - 색 억제면 소유자/표식 색 대신 현재 프레임 하위 BYTE 를 쓴다.
  - 좌표는 float → signed64 하위 DWORD 절삭이며, 비유한·±2^63 밖은 0 이다(JS `value | 0`).
  - 체인 next 는 효과 뒤 현재 슬롯 +4 WORD 를 읽는다.
- 거부: bitmap 크기·셀·픽셀 범위 밖과 TYPE 범위 밖은 `RangeError`, 손상된 next 순환(용량 초과 방문)은 `Error` 다. 원본의 메모리 손상 대신 명시 거부한다.

## 2. 검증

`tests/minimap-raster.test.mjs`(2건)

- 원본 4722행: 행마다 같은 SidPool(24000, client)·SquidHash·TYPE flags2 표에 다음 입력만 설치하고 셀을 그린다.
  - 슬롯 입력(sid:type:owner:flags2:xBits:yBits:next:state:extra, 나머지 바이트 0xA5)
  - 해시 머리(level:x:y:head)
  - 셀·모드·억제·프레임·색 DWORD 12개
  - 비교: 원문이 바꾼 위치의 관측 픽셀(offset:BYTE)을 펼친 기대 bitmap 과 미수정 주변(0xA5)까지 포함한 전체 128×128 bitmap.
  - 행 형식 감사: 완료 1·스택 차이 0·명령 수 양수·bitmap SHA 64자리.
- 웹 계약: 판본/빈 TYPE 표 거부, bitmap 크기·셀 범위 거부, 일반 점의 소유자 색, 색 억제 전역을 다음 호출에서 읽음, 월드 밖 절삭의 픽셀 범위 오류, NaN 좌표 0 절삭, next 순환·TYPE 범위 밖 거부.

최초 실패와 교정: 모듈은 첫 실행에 원본 4722행을 통과했다. 웹 계약 검사 초안의 손계산 기대값(일반 점 위치)을 잘못 적어 실행 전에 고쳤다. 비교가 실제로 작동하는지 확인하려고 소유자 없음 기본 색(246→245)·playerIsland 색 칸(owners[4]→[3])·2단계 체인의 Challenge 표식 여부를 각각 바꾸면 원본 대조가 모두 실패했고, 확인 뒤 되돌렸다.

## 3. 경계와 남은 것

- 전체 Refresh(행 우선으로 셀을 부르는 소비자)·`RawMetadisplayDirtyState` 의 같은 bitmap 연결·실제 미니맵 화면 표시(client `MinimapGeometry`·`RawMinimapControls`, J/K 영역)는 순서5 표시 연결 몫이다(PORT_MAP session 0).
- 실제 색 전역(검정·중립·소유자 색 검색 결과)과 Challenge 모드는 미니맵 소비자 이식 때 연결한다. Challenge 는 단독 경로에서 쓰이지 않는 메타맵 모드이지만 원본 분기를 그대로 옮겼다.

## 4. 다음 시작점

WB04-E205 는 "그림·캡처" 묶음의 RawColorSortPrimary(`ColorSortPrimaryTests` 80행, E77 `raw-color-sort-startup.js` 뒤)를 권한다.
