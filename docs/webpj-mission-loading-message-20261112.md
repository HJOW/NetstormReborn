# webpj 미션 시작 진행 안내 — MissionLoadingMessage (WB04-E339, 2026-11-12, 4-D B 구획)

3.1절 4-D B 구획 첫 항목의 네 번째 파일인 client **MissionLoadingMessage**(기준선 `.h` 35 + `.cpp` 44 = 79줄, 기준선 뒤 변경 없음)를 `core/mission-loading-message.js` 로 옮겼다. 원본 `cf570` 이 미션 시작 때 "Starting Mission..." 안내를 HTML 부모(transient)로 띄우거나 이미 떠 있는 부모를 갱신하는 몸체이다.

## 1. 규칙

- **buildMissionLoadingMessageBody(formatted, cancelText):** 이미 확장 printf 를 거친 C 문자열(첫 NUL 앞)에 줄바꿈과 기본 Cancel 단추 메타만 붙인다. 본문이 비었거나 마지막이 `\n` 이 아니면 `\n` 을 붙이고, 본문이 `~.` 로 시작하지 않고 대소문자 구별 `$Button` 부분 문자열도 없을 때만 `$ButtonInstant=` + 번역된 Cancel(첫 NUL 앞) + `,LeaveBattle,3` 을 붙인다. **Cancel 번역은 그 분기에서만 조회**한다(`~.`/`$Button` 본문은 번역 연결 없이도 동작). 원본 임시 버퍼는 NUL 포함 1024BYTE 라 원문이든 `\n` 을 붙인 뒤든 단추를 붙인 뒤든 1024BYTE 이상이면 호스트 `RangeError` 로 거부한다. 임의 스타일/따옴표 escape 는 만들지 않는다.
- **showMissionLoadingMessage(state, formatted, hooks):** 본문 조립 → 현재 transient 검색(`findSerial(state.transientSerial)`) →
  - 있으면 기존 부모 갱신(깊이 +1 → `metadata(token, { text })` 가 같은 문자열에서 메타를 지움 → 메타가 바꾼 문자열의 첫 NUL 앞을 `body(token, …)` 에 전달 → **효과 뒤 현재 깊이를 다시 조회해** −1; 32비트 감김)하고 `state.transientSerial = serial(부모)`, 그 값을 반환.
  - 없으면 `create(본문, 0, 400, 30)` 으로 새로 만들고(create 가 `state.createdSerial` 을 바꿀 수 있음) **그 뒤의 현재** `state.createdSerial` 부모를 찾아 있으면 같은 갱신을 한 번 더 한 다음 `state.transientSerial` 에 create 의 반환 serial 을 쓰고 그것을 반환한다(둘을 서로 바꾸지 않는다).
- `state` 는 `{ transientSerial(005cab34), createdSerial(005cab38) }` 속성을 읽고 쓰는 객체이다. C++ 의 소유 상태판(`MissionLoadingMessageState`)과 Client 비연속 전역을 빌리는 뷰(`MissionLoadingMessageView`)를 JS 하나로 합쳤다 — 뷰가 필요하면 같은 이름의 getter/setter 속성을 가진 객체를 넘기면 값 사본 없이 현재 값을 읽고 쓴다. `updateDepth(token)` 은 부모 `+21bc` 의 살아 있는 DWORD 를 `{ value }` 상자로, `std::string&` 는 `{ text }` 상자로 옮겼다.
- 글자 하나가 한 바이트인 원문 바이트 문자열을 다룬다(번역의 UTF-16→표시 부호화 변환은 호출자 경계). 선택된 경로에 필요한 효과만 요구하며 누락은 도달한 위치에서 거부한다(기존 부모 경로는 `create` 를 요구하지 않고, 신규 경로는 `serial` 을 요구하지 않는다).

## 2. 웹 범위

- 기준선 뒤 cpppj 변경 없음. PORT_MAP: `MissionLoadingMessage.cpp` port(base `1f00a25a`). 실제 HTML 부모(`HtmlWindowGump`·`004cf270` 생성기)는 UI 단위(5-A·6)에서 연결하며, 앱 수명 연결(Init 이 시작 안내를 띄움)은 B 구획 `Mission`·4-D 조립 때 한다(PORT_MAP session 0).
- cpppj 시험 `MissionLoadingMessage_StartingMissionDialogMetadata`(본문을 실제 DialogPage 파서에 넣어 instant Cancel 단추 하나와 `LeaveBattle`/`3` 동작이 나오는지 확인)는 `DialogScript`(PORT_MAP todo)가 필요해 그 이식 때 옮긴다. 본문 문자열 자체는 이 단위 시험이 `Starting Mission...\n$ButtonInstant=Cancel,LeaveBattle,3` 으로 확인한다.

## 3. 검사 (`tests/mission-loading-message.test.mjs` 10건)

- **원본 독립 관측:** `mission-loading-message-x86.tsv` 1540행(본문·Cancel 번역 × 기존/새 부모 프로필 × 깊이 × 호출 도중 현재 필드 변경 5가지)의 조립 본문·하위 효과 순서·최종 transient/created serial·두 부모의 깊이/serial 을 전부 대조한다. **첫 실제 실행에서 1540행이 모두 일치했다.** manifest 에 `mission-loading-message` 항목을 추가했다.
- **cpppj 시험 이식:** `MissionLoadingMessage_Native1540` · `MissionLoadingMessage_ConditionalCallbacksAndBounds`(조회하지 않는 Cancel/생성 경계, 1023BYTE 허용·1024BYTE 거부, 효과 누락 시 상태 무변경).
- **웹 계약:** 실제 시작 안내 한 줄의 모양, 줄바꿈 규칙(빈 본문·`\n` 종결), `$Button` 대소문자 구별, 본문/Cancel 의 첫 NUL 절단, `~.` 는 맨 앞·점까지 있어야 단추를 막음(`~Body`·`~` 는 단추 추가), 붙인 뒤 1024BYTE 경계(987글자 허용·988글자 거부), 기존 부모 깊이 +1/−1 과 메타가 지운 본문 전달·효과 뒤 현재 깊이 기준 감소·32비트 감김, 접근자 속성 상태 객체, 신규 경로의 `L5→C0,400,30→L70` 순서와 created 재조회, 메타가 NUL 을 남긴 본문의 절단, 효과 누락의 도달 위치 거부.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 33개를 돌려 첫 실행에서 3개가 남았다(메타가 남긴 NUL 절단·`~.` 접두 검사를 `~` 단독으로 약화·`need()` 방어). 앞 둘은 시험 보강(NUL 을 남기는 메타, `~Body`·`~` 본문)으로 잡았다. 변형 목록의 이스케이프 오류로 적용되지 않은 `endsWith('\n')` 변형 한 개도 목록을 고쳐 다시 돌려 잡았다. **남은 동등 변형은 1개**: `need()` 의 `typeof effect !== 'function'` 방어(제거해도 `undefined(...)` 호출이 같은 종류 `TypeError` 로 실패).
- 동등으로 판단해 변형 목록에서 뺀 것: 원문 첫 검사 `checkSize(first)` 는 뒤의 `checkSize(body)`(본문 ≥ 원문)가 같은 입력을 거부하므로 결과가 같고, 갱신 뒤 깊이를 다시 조회하는 상자는 같은 노드를 가리키는 시험 상자에서는 구별되지 않는다(단 효과 뒤 현재 값을 읽는 동작은 깊이 감김·효과가 깊이를 바꾸는 시험으로 확인).
- 남은 경계: 실제 HTML 부모/생성기 연결, 표시 부호화 경계(UTF-16 번역→바이트), DialogPage 파서 연동 시험.
