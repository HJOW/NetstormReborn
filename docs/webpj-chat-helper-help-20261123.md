# 채팅 도우미 단추의 우클릭 도움말 연결 (WB04-E1253)

RawMenuChat 행(E1224 감사)의 남은 표시 차이 가운데 "title/help 훅 no-op(우클릭 도움말 없음)"을 고쳤다. E1236 이 HUD 노드의 `SetNodeHelp`(문구·주제 이름 저장)와 우클릭 도움말 팝업을 연결했지만 채팅 창의 도우미 단추는 훅이 빈 함수로 남아 있었다.

## 원본과 웹

- 원본 `ChatWindow.cpp:51`: `title(helperNode, translate("Chat Helper"))`, `help(helperNode, "chathelper")`. `RawMenuChat.inc:33` 이 이 두 훅을 `owner.SetHelpText`·`owner.SetHelpWord`(= `SetNodeHelp`)에 연결한다.
- 웹: `ui/client-chat-window.js` 의 훅 `title`·`help` 를 `devices.setNodeHelp(token, text|word, false|true)` 로 연결(장치가 없으면 이전처럼 아무것도 안 함). `ui/client-production-assembly.js`·`ui/client-production-menu.js` 가 앱의 `setNodeHelp` 를 채팅 창 장치까지 전달한다. 도움말 문구는 번역한 문자열이라 한국어 모드에서는 `채팅 도우미`(번역표 기존 항목)이다.
- 우클릭 팝업의 Help 항목은 주제 `chathelper` 에 `Help` 를 붙인 `chathelperHelp` 를 연다(영어·한국어 도움말 파일에 주제 있음).

## 시험

- `e2e/mission-object-commands.spec.js` 새 시험(실제 Chromium, The War Begins!): F3 로 채팅 창을 보이고 Enter 로 입력줄을 열면 보이는 도우미 단추의 노드 도움말이 `{ title: 'Chat Helper', word: 'chathelper' }`, **실제 마우스 우클릭**이 팝업을 열고 Help 항목 명령이 `chathelperHelp` 도움말 창을 연다. 훅을 빈 함수로 되돌리면 팝업이 열리지 않아 실패함을 확인하고 복원했다.
- 관련 Node 50건(chat-window·chat-format-dialog·chat-channels·대응표·manifest)과 채팅 e2e 10건(`e2e/chat-window.spec.js`), HUD 우클릭 도움말 e2e 통과.

## RawMenuChat 의 남은 차이(행은 partial 유지)

- 채팅 로그를 미션 끝에 초기화한다(원본은 Client 수명 동안 유지, E1170 에서 웹은 미션마다 새로 시작하기로 한 문서화된 적응). 채팅 전역(`ClientChatWindowGlobals`)이 미션 시작 때 새로 만들어져 토큰·창 상태와 함께 묶여 있어 유지하려면 수명 분리가 필요하다.
- 알림 Gump serial 을 0 으로 고정하고 wake 가 no-op 이다(채팅 창이 숨겨진 동안 새 메시지가 오면 도우미 단추가 깜박이는 표시가 없다). 단독 미션에서 채팅 메시지가 오는 경로가 거의 없어 영향이 작다.
