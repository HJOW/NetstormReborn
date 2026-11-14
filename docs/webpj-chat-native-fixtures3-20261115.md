# webpj trace→Chat 전달·ChatWindow 기하 원본 관측 연결 (WB04-E476, 2026-11-15)

3.8절 이월 목록의 채팅 계열 세 번째 묶음이다. 이미 이식한 `TraceChat`(+`ChatWindow.appendLog`)과 `HudChatGeometry`의 ChatWindow 기하를 원본 10.78 독립 관측 fixture 두 개 **3,997행**에 대조했다. 모듈 코드는 고치지 않았고(주석 오기만 정정) 둘 다 **첫 실행에서 전체 일치**했다.

| fixture | 행 | 대상 | 비교한 것 |
| --- | --- | --- | --- |
| `trace-chat` | 37 | `forwardTraceToChat` + `ChatWindow.appendLog` | 현재 색/현재 Chat 관문, 부모/두 자식의 8DWORD 문맥(공급/부모/자식 alias·`none`)·현재 자식 변경·문맥 변경·하위 부분 중단과 호출 순서 |
| `hud-chat-geometry` | 3,960 | `HudChatGeometry.attach/setHelperVisible/setInputVisible` | 독립 1,980입력의 두 x87 정밀도에서 root(1)·Console/Text/helper/background(2..5) 다섯 노드의 플래그·기본/표시 크기 float 비트와 공개 가상/텍스트 요청(`baseAttach`/`push`/`attach`/`bottomIndex`/`bottom`/`challenge`) 순서 |

## 한 일

- `tests/chat-native-fixtures3.test.mjs` 신설: cpppj `TraceChatTests`·`HudChatGeometryTests`의 입력 구성(앞 열만 읽음)과 출력 직렬화를 그대로 옮겼다. 기하 시험은 root/네 자식 저장소와 명시 경계의 호출만 기록하고 4ba3d0 내부 Redraw 표식은 원본처럼 비교에서 제외한다(제품 모듈의 외부 요청 경계 안쪽).
- manifest에 두 항목(`unit WB04-E476`)을 연결했고 PORT_MAP 두 행(`TraceChat`·`HudChatGeometry`) 메모에 연결 사실을 덧붙였으며 `trace-chat.js`와 `trace-chat.test.mjs`의 "cpppj에도 직접 시험이 없다"는 오기를 정정했다.

## 결과

- 두 fixture 모두 첫 실행에서 전체 일치했다. 첫 시도에서 `trace-chat` 3행이 어긋났으나 시험 하네스가 문맥 인자 `none`(빈 문맥)을 자식 문맥으로 잘못 넘긴 것이며(cpppj는 빈 span) 하네스 교정 뒤 모두 일치했다. 모듈 결함은 없었다.

## 남은 것

채팅 계열은 `chat-window`(180행 생성/입력/도우미/수명)·`chat-window-base`·`chat-completion`·`chat-callbacks`·`chat-input-recipients`가 남았다. `ChatWindow` 생성/입력 시험은 `TextEntryGump` 시험과 같은 `EntryScene` 지원 틀을 쓰므로 텍스트 입력 계열(`text-entry-gump`·`text-entry-buffer`)과 함께 처리하는 편이 낫다.
