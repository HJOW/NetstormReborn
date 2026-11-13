# webpj TileGroupCollection — 지면/다리 행 묶음 수집기 (WB04-E394, 2026-11-13, 5-A 스물두 번째 단위)

`ClientRendererFrame` 의 선행 부품인 client **TileGroupCollection**(cpppj `client/TileGroupCollection.cpp/.h`, 기준선 138줄, 10.78 00433410 ClientBoard 의 행 묶음 수집, 상태 todo)을 `render/tile-group-collection.js` 로 옮겨 port 로 올렸다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). CD 004884a0 경로(판본 49행 고정 상한·슬롯 +33 깊이)는 웹이 10.78 만 지원해 옮기지 않았다.

## 1. 규칙

- **`collect(view, rowLimit, covered)`:** 해시 0단계를 고른 뒤 월드 가시 사각형 네 좌표를 `float32 + 0.5`(double)의 CRT 64비트 절삭 하위 DWORD(signed)로 칸 번호로 바꾼다. NaN·무한대·±2^63 밖은 FISTP 의 정수 indefinite 하위 DWORD 0, 상단은 255 로 제한한다. 왼쪽/위는 한 칸 줄이고(`+0xffffffff` DWORD 덧셈) 오른쪽/아래는 한 칸 넓힌 뒤 각각 따로 0~255 로 제한한다. y 바깥·x 안쪽 순서로 **현재(0단계) 칸의 머리 SID 하나만** 읽는다. SID 0 이면 배경이 덮이지 않은 것이다(`covered.value = false`).
- **항목:** 슬롯 +14/+18 의 float x·y 바이트를 그대로 복사하고 +35 의 signed BYTE 를 깊이(WORD)로, kind 1·tail 0, 토큰 `objectTokenBase + SID × 50` 의 16바이트 항목을 연속 저장소에 쓴다. next/type/state/프레임 크기는 이 수집기의 필터가 아니다. 풀 범위 밖 SID 는 `pool.slot` 이 거부한다(그 앞의 부분 출력은 남는다).
- **한도:** 항목을 기록한 **뒤** 행 개수가 `rowLimit` 이상이면 오류(strict). 개수가 0 이 아닌 행만 헤더 `{ token: groupTokenBase + 번호 × 8, entries(연속 보기), count: { value } }` 를 만들고 헤더 수가 `floor(screenHeight / 11) + 6` 이상이면 오류이다(화면 높이 0 → 5개까지 허용). 오류 때는 해시가 0단계에 머물고 `covered` 가 바뀌지 않는다.
- **정상 반환:** 해시 선택 2단계, `covered.value` 기록, 헤더 목록 반환. 행 내부는 공통 정렬을 적용하지 않는다. 다음 `collect` 때 이전 헤더/항목은 지워진다(지난 프레임 토큰은 `group()` 이 거부).
- **`group(token)`·`object(token)`·`entries()`·`bindTileGroupCollection`:** 헤더 토큰은 정수·기준 이상·`(token − base) % 8 == 0`·현재 헤더 수 안이어야 하고, 객체 토큰은 기준 이상·stride 배수·SID 1 이상·`< capacity` 이다. `bindTileGroupCollection` 은 공통 수집 훅의 `groups` 와 분배 훅의 `group` 을 이 수집기로 잇는다.
- **상태 검증:** `{ screenHeight = 768, objectTokenBase = 0, groupTokenBase = 0x80000000 }` — 화면 높이는 0 이상 `INT_MAX − 66` 이하 정수, 두 토큰 기준은 32비트 부호 없는 정수이며 객체 영역(`용량 × 50`)과 묶음 영역(`256 × 8`)이 32비트를 넘거나 겹치면 거부한다(맞닿음은 허용).

## 2. 검사 (`tests/tile-group-collection.test.mjs` 8건)

- **원본 독립 관측(첫 실행 일치):** `tilegroup-x86.tsv` 의 **originals 353행**(가시 사각형 float 비트·행 한도·화면 높이·초기 해시 선택·희소 해시 칸·완성 슬롯 입력 → 헤더 수·배경 덮임·연속 항목 3072바이트·헤더 1024바이트·해시 선택 2)을 재생하고, 수집 전후 풀과 해시 0단계가 같고 모든 객체 토큰이 `object()` 로 같은 SID 가 되는지 함께 본다(manifest id `tilegroup`). CD·추가판 각 353행은 웹이 10.78 전용이라 쓰지 않는다.
- **cpppj 시험 이식:** `RowLimitsAndEditionRows`(패치 부분: strict 한도·실패 시 선택/배경/부분 출력·49개 행·화면 높이 0 의 상한), `TokenContractsAndLifetime`, `CommonSubmissionIntegration`(실제 raw SID/해시의 행 순서→공통 수집→CRT 정렬→kind 1 내부 sprite 제출, 행 내부 깊이는 다시 정렬하지 않음, UI 전용 dirty 는 수집하지 않음).
- **웹 계약 보강 4건:** 상태 검증 경계(높이 상한·토큰 정수/범위·영역 끝과 맞닿음/겹침), 토큰(기본 0x80000000 의 양수성·8 간격·8 의 배수가 아닌 기준·객체 토큰 각 거부 조건), NaN/무한대/큰 float32 좌표와 0.5 올림 경계, 맵 전체(65,536 항목) 용량과 헤더 상한 분모.
- 입력 보존 비교는 120만 바이트 배열의 원소 비교 대신 바이트 버퍼 비교를 쓴다(47초 → 0.5초).

## 3. 변형 확인

`tile-group-collection.js`(25~160행) 연산자·상수 변형 118회: 1차 생존 36건 → 시험 보강 후 생존 16건 → 기준이 8 로 나누어떨어지지 않는 토큰 시험을 `…02` 기준으로 조여 `(token − base)` 부호 변형도 잡아 최종 **103/118 잡음, 생존 15건 모두 동등/비관측**이다: ① 항목 tail 바이트 위치(`15`→`16`: 다음 항목이 덮어쓰고 값이 늘 0), ② float32 큰 값의 하위 DWORD 가 항상 0 이라 `2^63` 경계·범위 `&&`/`||`·`63`→`64` 변형이 결과를 바꾸지 않음(7건), ③ 기본 화면 높이 768→769(`floor(h/11)` 이 같음), ④ 토큰 기준 `>=`→`>`(영역 끝 검사가 같은 입력을 거부), ⑤ x·y 복사 폭 4→5(뒤의 token/y 쓰기가 덮어씀)와 `setUint32` 의 `>>>`→`>>`(같은 비트), ⑥ 객체 토큰 `<`→`<=`·`||`→`&&`(기준과 같은 토큰은 SID 0 으로 거부되고 비정수는 `% stride` 로 거부).

## 4. 남은 경계

- 앱 수명에 연결하지 않았다(`PORT_MAP` session 0). 실제 `ClientRendererFrame.Collect` 가 `Screen`·`RawMenuControls`·세션 월드와 함께 이 수집기를 묶는 것은 그 단위에서 한다(`ClientRendererFrame` 의 나머지 선행: `GumpCollection`·`ClientKencloudGlobals`·`RawCampaignCursorDisplay`·`RawCampaignFactoryDisplay`·`RawMenuControls`·`GameWorld`·`ClientGameplayGlobals`).
- `ObjectCollection_CommonSubmissionIntegration`(UI·지면·일반 객체의 세 수집기를 한 공통 목록에서 병합)은 `GumpCollection` 단위(E395)에서 닫는다.
