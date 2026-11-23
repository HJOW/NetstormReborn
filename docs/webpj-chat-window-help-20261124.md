# webpj 채팅 창 본체 우클릭 도움말 e2e (WB04-E1295, 2026-11-24)

[LEFT_JOBS](../LEFT_JOBS.webpj.md) 5-C '메뉴·버튼 컨트롤'의 "채팅 창 'Chat Window' 실제 우클릭"을 닫았다. 코드는 바꾸지 않고 e2e만 더했다(기능은 E1236·E1253에서 이미 연결돼 있었다).

## 확인한 것

- 원문(cpppj `UberGump`의 채팅 노드 생성): Uber 루트의 채팅 노드에 `SetHelpText("Chat Window")`·`SetHelpWord("chatView")`를 걸어 우클릭하면 제목과 Help 항목이 든 팝업이 열린다(웹은 `www/ui/uber-root-gump.js:112`).
- 새 e2e `e2e/mission-object-commands.spec.js`의 '채팅 창 본체 우클릭 도움말이 열리고 Help 항목이 chatViewHelp 를 연다'(Chromium 4:3, The War Begins!):
  1. F3으로 채팅 창을 보이게 하고 Uber 루트 노드(`_campaignProduction.hud.node().chat`, 생산 패널 노드가 아님)의 도움말 문구·주제가 `Chat Window`·`chatView`인지 확인한다.
  2. 실제 마우스로 채팅 노드 중앙을 우클릭하면 팝업(`blockingPopups()`)이 열린다.
  3. 팝업의 Help 항목(F1 도움말 명령 `0x02000017`에 주제 토큰을 실은 것)을 입력 큐에 올리면 `chatViewHelp` 주제의 도움말 창이 열린다.
- 앞서 `ui.node().chat`(생산 패널 노드)로 읽다가 "Gump 토큰/저장소 없음" 오류가 나서 임시 탐침으로 Uber 루트 노드가 `hud.node()`임을 확인하고 고쳤다.

## 대응표

`RawButtonControls`(811줄)의 닫는 조건 둘 중 이 항목은 닫혔다. 남은 것은 예산 칸(스톰 파워 표시)의 원본 실행 확인이며, 대응표 메모에 E1236~E1253·E1295를 반영한다. 그 확인이 끝나면 partial→port로 올린다.
