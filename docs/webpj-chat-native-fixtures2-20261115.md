# webpj 채팅 대상·정리·입력줄 원본 관측 연결 (WB04-E475, 2026-11-15)

3.8절 이월 목록의 채팅 계열 두 번째 묶음이다. 이미 이식한 `ChatRecipients`·`RawChatChannels`·`HudChatGeometry`의 입력줄 부분을 원본 10.78 독립 관측 fixture 다섯 개 **992행**에 대조했다. 모듈 코드는 고치지 않았고(주석 오기만 정정) 다섯 모두 **첫 실행에서 전체 일치**했다.

| fixture | 행 | 대상 | 비교한 것 |
| --- | --- | --- | --- |
| `chat-recipients` | 350 | `checkChatRecipients` | 반환·지역 60바이트·조회/번역 호출 순서, 모드별 번역 변경·`Unknown`→`EVERYONE` 치환·선택 중단 |
| `chat-recipient-identity`(E455 항목 `chat-recipient-identity-runtime` 재사용) | 48 | 같은 함수 + 실제 `RawPlayerIdentity` | 이름 길이(40바이트 절단)·전투/에디터 모드가 다른 입력의 반환·지역·호출 순서 |
| `chat-state-clear` | 216 | `RawChatChannels.resetOccupied` | 108입력×두 정밀도(027f/037f)의 채팅 기록 전체 4800바이트와 쓰기 수(12DWORD 배수) |
| `chat-input-toggle` | 216 | `HudChatGeometry.inputVisible/toggleInputVisible` + `consumeChatInputToggle` | 콘솔 플래그·현재 Chat·요청·반환·가상 Push/Attach 순서(Push 안의 재진입 변경 포함) |
| `chat-input-key` | 162 | `dispatchChatInputKey` | Guide/Gump/모달 관문 뒤 Enter 의 표시·열기 호출 순서 |

## 한 일

- `tests/chat-native-fixtures2.test.mjs` 신설: cpppj `ChatRecipientsTests`·`ChatStateClearTests`·`ChatInputTests`의 입력 구성(앞 열만 읽음)과 직렬화를 그대로 옮겼고 관측 열은 비교에만 썼다. 상태 정리 시험에는 cpppj의 두 번째 시험(빈 행의 stale 값과 양쪽 가드 바이트 보존, `clear` 마스크 정리가 전체 정리를 대신하지 못함)과 입력줄 시험의 실패 callback 요청 보존/성공 한 번만 소비도 함께 옮겼다.
- manifest에 네 항목(`chat-recipients`·`chat-state-clear`·`chat-input-toggle`·`chat-input-key`, `unit WB04-E475`)을 연결했다. `chat-recipient-identity` fixture(48행)는 E455가 이미 `chat-recipient-identity-runtime` 항목으로 `ClientChatRuntime`에 연결했으므로 같은 항목에 이 시험을 소비자로, `chat-recipients.js`·`raw-player-identity.js`를 모듈로 추가했다(경로 중복 금지). 또한 PORT_MAP 세 행(`ChatRecipients`·`RawChatChannels`·`HudChatGeometry`) 메모에 연결 사실을 덧붙였으며 `chat-recipients.js`·`hud-chat-geometry.js`와 `hud-chat-geometry.test.mjs`의 "cpppj에도 직접 시험이 없다"는 오기를 정정했다(E465 감사가 찾은 같은 오기).

## 남은 것

채팅 계열은 `chat-window`·`chat-window-base`·`chat-completion`·`chat-callbacks`·`chat-input-recipients`·`trace-chat`·`hud-chat-geometry`(3,961행 기하)가 남았다. `HudChatGeometry`의 기하(부착/크기) 부분은 이 단위에서 다루지 않았다.
