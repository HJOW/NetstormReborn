# 채팅 창 조립 1단계: 만들기·F3 표시·로그 출력 (WB04-E1169)

미션 화면 아래의 채팅 영역(F3)은 구분선만 오르내리고 안은 검은 띠였다([E1159](webpj-chat-toggle-f3-20261121.md)). 이식돼 있던 `ChatWindow`(원문 0043f820/00431ee0)와 `HudChatGeometry`를 실제 Client Gump 트리에 조립해 **창이 만들어지고, F3로 보이며, 로그 줄이 색과 함께 그려지게** 했다. E1168의 글자 입력창·모달 기반 위에서 같은 방식으로 조립했다.

## 구현

| 영역 | 내용 |
| --- | --- |
| 조립 | [client-chat-window.js](../webpj/www/ui/client-chat-window.js)의 `ClientChatWindow`(cpppj `RawMenuChat.inc`의 `BindChat`·`AllocateChat`·`ChatRead`). root(0x13c `HudChatNode`)·입력줄 Console(0x128)·로그 ScrollingText(0xd8)·도우미 돌 버튼(0x170)·배경(0xb8)을 실제 Gump 저장소에 만들고, 원본 `ChatWindow` 훅 약 50개를 실제 부품에 묶는다: 노드 조회, 입력창(`ClientTextEntryControls`, Console은 `allocate(true)`), 스크롤 글자(`ClientScrollingText`), 돌 버튼(`ClientStoneControls`), 배경, 도우미 메뉴(`ClientMenuGump` 작성기), 점 명령 등록(`ClientDotCommandGlobals`, clear/quit), 완성 목록용 프레임 높이(TYPE 클러스터 수·SQUID 헤더 +0x1a). |
| 전역 표 | 원본 `ChatRead`: 사건 11개(0x02000115·0x0200002c·0x020000bf·0x02000088·0x02000038·0x02000027·0x02000101·0x020000a6·0x02000105·0x02000106·0x0200006d), 이름 팔레트 5개(도우미 갈색 9·배경 32·입력줄 배경 30·입력줄 글자 31·로그 글자 33), 창 상태(현재 root·등록·완성 준비), TYPE 수, 그 밖은 앱 장치(전투 모드 0x594fbc·메타 0x594fc8·메뉴 위치 0x5cae94/98·부모 루트 0x563e38). |
| 공유 로그 | 로그 자식은 Client 장기 상태 `globals.log`를 `0x548758` 토큰의 128줄 버퍼로 **빌려** 쓴다(`ClientStyleTextBuffers.constructBorrowed` 추가). 웹은 미션마다 스타일 저장소를 새로 만들어, 미션을 떠날 때 버퍼를 반납하고 `ClientChatWindowGlobals.resetLog()`로 로그 상태를 새로 바꾼다(처음에는 둘째 미션에서 `StyleTextBuffer 새 저장소 필요`로 시작 직후 터졌고 e2e가 잡았다). 원본은 한 Client 동안 로그가 이어지지만 웹은 미션마다 새로 시작한다. |
| 스크롤 글자 노출 | `ClientScrollingText`에 `atBottom`·`requestBottom`·`requestRelativeScroll`·`setScrollable`·`replaceBuffer`를 더했다(채팅 창이 로그 끝 판정·바닥 요청·버퍼 교체에 쓴다). |
| HUD | [client-uber-hud.js](../webpj/www/ui/client-uber-hud.js): `devices.chat`가 있으면 Uber 루트가 `createChat`·`type`(TYPE 범위만 검사)·`translate`·`help`·`helpWord`·`refreshTechnology`를 받아 채팅 자식을 만들고, `HudDock`의 `chatVisible`/`chatInput`이 실제 창의 도우미·입력줄 표시를 바꾼다(원본 `SetHelperVisible`/`SetInputVisible`). 생산 UI 조립(`ui.chat()`)이 창을 만들고 [netstorm.js](../webpj/www/netstorm.js)가 Client 장기 채팅 전역과 현재 값 조회 장치를 넘긴다. |
| Players 메뉴 | 채팅 창이 생기면서 Players 메뉴의 `whispered` 조회(채팅 창이 있으면 미연결 오류를 던지던 자리표시)가 터져 ESC 메뉴막대가 열리지 않았다(영향 e2e 묶음이 8건 실패로 잡았다). 원본 `ContainsRecipient`(0042e9e0)로 현재 입력줄 수신자 접두에 그 Player 이름이 있는지를 돌려주게 연결했다(`ClientChatWindow.containsRecipient`). |
| 입력 | 입력줄 키보드 경로(Enter로 열기·문자 입력·제출)와 수신/송신은 아직 연결하지 않았다. 도우미·입력줄은 구분선 전환의 표시 비트만 바뀐다. 제출(`sendChat`)은 로컬 되울림, 수신자(`recipient`)는 없음, `resolve`는 실패가 기본이다. |

## 시험

- `e2e/chat-window.spec.js`(Chromium, 스커미시 AI 3명) 두 건: ① 시작 때 채팅 root가 화면 아래(768 이후)에 숨어 있고 지도 영역이 전체 높이이며 도우미가 꺼져 있다. 숨긴 동안에도 로그가 쌓인다. F3를 누르면 지도 영역 아래쪽이 768→672로 줄고 채팅 영역(그림 폭 940 이상, 아래 768)이 올라오며 도우미 표시가 켜지고 첫 로그 줄 글자가 그려진다. 빨강 접두(`~r`)와 되돌림(`~n`)이 줄 안에서 색을 바꾼다. 다시 F3를 누르면 숨겨지고 로그는 남는다. ② 미션을 떠나면 창 상태가 0이 되고 다음 스커미시에서 창이 다시 만들어진다. 오류·경고 0(시험이 픽셀을 읽을 때 브라우저가 내는 Canvas2D 성능 안내만 제외).
- 연관 Node 시험 62건(대응표·fixture·스크롤 글자 위젯·ChatWindow 원본 관측 180행·Gump 루트)과 영향 e2e 묶음 25건(브라우저 입력 폴링·생산 정보·게임 메뉴막대·한국어 Players·한국어 게임 안·미니맵·미션 단축키·D 키·Players 동작·항목·Send X·채팅 창·HUD 배치)이 모두 통과했다(`extracted/webpj-e1169-e2e-2.log`, 3.5분). 처음 실행에서는 위 `whispered` 때문에 8건이 실패했고, HUD 배치 시험의 `chat: 0` 기대는 실제 창이 생겨 `chat > 0` 으로 고쳤다.
- 실제 화면(영어 4:3): 지도 아래에 회색 채팅 영역과 오른쪽 스크롤 칸이 나오고 `Our Priest has been Immobilized`·빨강 `Red` + `line two`가 그려진다.

## 남은 일

- **입력줄 키보드 경로:** 채팅 구분선이 보일 때 Enter로 입력줄을 열고(`dispatchChatInputKey`) 키를 입력창으로 보내며 Enter로 제출한다. E1168의 모달 키 경로와 같은 방식(큐에 올린 키를 Gump 입력으로만 전달)을 쓴다. 제출·수신자 이름 완성·도우미 메뉴 e2e는 이 단위에 포함하지 않았다.
- **로그 발생원:** 단독 미션의 알림 문구(예 `Our Priest has been Immobilized`)를 `appendLog`로 흘리는 연결(`MessageGump` 안내와 같은 원천).
- 원본 단독 미션 스크린샷에는 채팅 영역이 보이지 않아(기본 숨김) 픽셀 대조는 F3를 직접 눌러 캡처해야 한다.
- 지식(NSInfo) HUD 패널 공장은 이 단위 범위 밖이다.
- 기준선 뒤 cpppj 변경 없음(`RawMenuChat.inc`·`UberHud.inc`).
