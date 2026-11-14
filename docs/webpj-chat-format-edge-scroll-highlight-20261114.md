# webpj Chat 서식·가장자리 스크롤·사용자 강조 소형 3단위 (WB04-E442, 2026-11-14)

RawMenuControls 선행 **RawChatFormat 88줄·RawEdgeScroll 63줄·RawUserHighlight 66줄(기준선 `1f00a25a`의 .cpp/.h 줄 수, 합계 217줄, todo→port)**을 새 모듈 [`www/ui/raw-chat-format.js`](../webpj/www/ui/raw-chat-format.js)·[`www/core/raw-edge-scroll.js`](../webpj/www/core/raw-edge-scroll.js)·[`www/core/raw-user-highlight.js`](../webpj/www/core/raw-user-highlight.js)로 이식했다. 기준선 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 손 계산 바이트/값으로 한 것이며 원본 직접 관측 수치가 아니다.

## 기준선 뒤 변경 (3.6절)

세 단위 모두 기준선 뒤 변경이 없다.

## 구현

- **`RawChatFormat`(원문 0042ddf0 back / 0042def0 front, 슬롯 제거 0042d160)**: `stripChatFormatSlots`는 `~0`~`~6` 두 바이트를 순방향 제자리 제거하며 제거한 위치를 다시 검사하고(연속 `~0~1`도 제거) NUL까지 앞으로 복사하되 새 NUL 뒤의 원래 꼬리는 지우지 않는다(물리 범위/NUL 없음은 RangeError). `applyChatFormat`은 현재 manager가 0이면 `assertion`(back 157/front 184) 뒤 계속하고, 범용 정규식 대신 고정 패턴 `(~[^~ CR TAB LF space]+)+`(VT/FF·비ASCII는 스타일 글자)의 첫 부분 검색/greedy 연속 그룹을 실행해 일치가 없으면 `illegalFormat(입력 첫 NUL 앞)` 뒤 0을 돌려준다. 일치하면 최대 80바이트를 공통 캡처에 쓰고 NUL → 슬롯 제거 → 현재 manager 복사(back 0x14/60, front 0x50/500) → 그 manager의 강제 NUL(back 0x4f, front 0x243) → `ID+chatFormat`/`chatFormat2` Config 쓰기 순서이며 항상 0이다. 캡처는 81바이트 이상이어야 한다.
- **`RawEdgeScroll`(원문 004d65ac..004d67ad)**: `processEdgeScroll`은 초기화 비트 4(`005cafe8`, 다른 비트 보존)를 켜고 시각을 0으로 지운 뒤, 꺼짐/테두리/대화상자/왼쪽 버튼 눌림이면 방향 0으로 반환하고, 포인터가 가장자리 2픽셀(`x<2`·`y<2`는 메뉴가 없을 때만·`≥크기-1`)이면 방향을 정한다. 방향이 없거나 팝업이 막으면 시각을 0으로 지우고 반환하며, 아니면 최초 시각을 현재 시각으로 잡고 `ftol(30×경과+2)`를 속도로 제한(`min`, float32 저장)한 양으로 팝업을 닫은 뒤 Shift가 눌려 있으면 큰 축만 남기고(같으면 X 삭제) `request(dx, dy)`한다. `requestCameraDelta`는 현재 view에 이동량만 더해 목표에만 쓰고(`높이/2+현재 Y+dy−높이/2`, 32비트 감김) `metaRebuild`를 1 올린다.
- **`RawUserHighlight`(원문 004d5bb0/004d5480)**: `resetMouse`는 마우스 TYPE만 0으로 쓰고, `highlight(모드)`는 같은 모드면 아무 효과 없이 반환하고 변경 때만 모드 쓰기 → 마우스 TYPE 초기화 → 효과(1~3 커서 3, 4 커서 2, 5는 역투영 전에 선택 SID를 보관해 역투영한 두 DWORD로 이동 강조, 그 밖은 커서 1)를 실행한다. 선택 SID는 바꾸지 않는다.
- 웹 적응 표기: 참조로 받는 현재 전역은 제자리에서 바꾸는 `state` 객체/`{ value }` 상자, `std::function` 기본 초기화(`stripSlots`)는 지정하지 않으면 기본 구현(명시 `null`은 거부), `logic_error`→`Error`, `out_of_range`→`RangeError`.

## 검사와 발견

새 시험 파일 3개, **21건 통과**: `raw-chat-format.test.mjs`(7), `raw-edge-scroll.test.mjs`(9), `raw-user-highlight.test.mjs`(5). 슬롯 제거와 캡처는 바이트 배열을 손으로 계산해 꼬리 보존까지 대조했고, 가장자리 스크롤은 경계 포인터(1/2·1022/1023)와 경과 0/0.5/1/2초의 가속을 값으로 계산했다.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 3건):** 슬롯 제거 뒤 캡처에는 'x,NUL' 뒤로 옛 바이트가 그대로 남는다는 점(두 곳), 시각이 있는 상태를 시험하려면 초기화 비트(4)가 켜져 있어야 한다는 점을 바로잡았다.

**변형 확인:** 연산자/정수 상수 변형 167개(chat-format 80·edge-scroll 66·user-highlight 21) 중 159개를 잡았다. 처음 생존 17개 중 9개는 보강으로 잡았다: 최하위 비트만 있는 flags의 초기화, 경과 1/2초 가속 계수, 슬롯 제거로 빈 캡처, 입력 선두 NUL·NUL 뒤 입력, 정확히 81바이트 캡처, 길이 2 배열의 `~1`(NUL 없음 오류). 남은 8개는 동등 변형이다: 배열 끝을 한 칸 더 읽는 변형(`undefined`가 같은 결과), 곧바로 덮어쓰는 `[length]` 쓰기, 한 그룹의 스타일 글자를 하나 더 건너뛰는 `cursor += 2`(끝 일치가 같음), `높이/2`가 목표 Y에서 상쇄됨.

## 남은 경계

- Chat 명령(format/back/front 등록)·입력 처리가 이 부품을 부르는 연결, 가장자리 스크롤이 읽는 실제 화면/포인터/메뉴 serial, 사용자 선택의 커서/역투영/이동 강조 장치는 별도 단위다. 앱은 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
