# webpj 아처 TYPE 자료(RawArcherTypeData) 이식 — WB04-E127

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백스물일곱 번째 부분이다. 타입 초기화(.type 콜백)가 아처/크로스보우 소비자에게 공급하는 방향 수(TYPE+110)와 물리 프레임 수(TYPE+114)를 읽는 `o/RawArcherTypeData`(cpppj 기준선 `1f00a25a`)를 옮겼다. 선행은 기존 웹 타입 정의 파서(`assets/type-table.js` 의 `parseTypeDefinition`)와 `FrameCodeTable` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-archer-type-data.js` — `readRawArcherTypeData(definition)`

- 방향 수(TYPE+110)는 초기값 0 을 유지하며 `orientations` 속성(대소문자 무시, 같은 속성이 여러 번이면 마지막 값)이 명시된 경우에만 숫자 AST 의 값을 float 폭으로 줄여 0 쪽으로 절삭한다. 속성이 있는데 숫자가 아니면(예: 문자열) 원본 계약 오류로 `TypeError`, 값이 비유한이거나 int32 범위(float 로 줄인 뒤)를 벗어나면 `RangeError` 이다. 누락 속성 0 으로 바꾸지 않는다.
- 물리 프레임 수(TYPE+114)는 `.type` 클러스터 개수이다.
- 반환 `{ directions, frameCount }` 가 소비자의 `animationTypes[type]`(`RawArcherAnimation.rotate`·`RawCrossbowAnimation.frame`)이다.

## 2. 검증

- `tests/archer-type-data.test.mjs`(2건):
  1. `archertype-x86.tsv` 의 `originals` 22행(`originalCD` 22행은 10.72 라 제외) — 타입 초기화 접두(prefix)와 보호 35클러스터 AST 의 callback/후처리 정상 반환에서 원본 현재 TYPE 의미 필드(방향 수·프레임 수·기본/gump/help/base 프레임·글자 종류 수·SHP 프레임 수 35), 4바이트 프레임 코드 배열 전체(side/variant/number/flags), 글자 A~P 의 첫 번호 16개와 개수 16개가 같다. 기존 웹 타입 정의 파서와 `FrameCodeTable` 이 같은 입력에서 같은 값을 낸다.
  2. 계약: 문자열 `orientations`/범위 밖 `1e30` 거부, 속성이 없으면 방향 0·프레임 수는 클러스터 수, 마지막 속성이 이기고 소수는 0 쪽으로 절삭.

최초 실패와 교정: 새 모듈은 22행 fixture 를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 보호 TYPE/SHP 자산에서 실제 `sunArcher`(방향 0, 물리 프레임 35, group 0, default P32)를 읽는 통합 시험(`ArcherTypeData_ActualAssetsGunSkipsRotateAndStartsFire`)은 `RawGunProcess`·Host 연결 이식 뒤에 확인한다.

## 4. 다음 시작점

`RawGunProcess`(+`MakeGunTargetHooks`, Animation/Missile Host 연결), `RawLocalGeyser`, `RawGunImpact`(SquidFrame/Host 장면 필요), `RawWindAviaryProcess`·`RawWindFlyer`, 비행체(`RawFlyerProcess`·`RawFlyerFrame`·`RawFlyerTarget`), 번개 수명/프레임/화면, `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
