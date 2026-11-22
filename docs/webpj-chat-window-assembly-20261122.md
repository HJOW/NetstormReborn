# 채팅 창 조립: 만들기·F3 표시·로그 출력(E1169), 입력줄 키 경로(E1170), 제출·수신(E1172)

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
| 입력줄 키 경로(E1170) | **Enter**(채팅 입력줄이 보이지 않을 때)가 입력 큐를 거쳐 원본 `dispatchChatInputKey`(`OpenChatInput`)를 실행한다: 입력줄을 보이고(`SetInputVisible(1)`) 채팅 구분선 전환 사건(0x02000088, 둘째 인자 1)을 부모로 올려 영역이 올라온다. 입력줄이 보이는 동안 **문자 키**(한 글자·Backspace·Delete·Enter·좌우·Home·End)는 입력 큐에 올라 입력 프레임에서 Gump 입력(`route`)으로만 간다(게임 단축키·화면 위치 저장 숫자로 새지 않음). 마우스와 F 키·**ESC**는 평소 경로를 쓴다(입력줄이 보여도 ESC 는 게임 메뉴막대를 연다). 입력줄 Enter 는 `ChatWindow.handle` 의 제출 분기를 타고, 단독 미션은 현재 채팅 manager 가 없어(원본 `say` 는 manager 가 없으면 0) 제출한 줄이 비워지며, **본문이 비어 있으면 입력줄이 숨는다**(원본 `inputVisible(0)`). 도우미 `>`와 입력줄·캐럿이 실제 화면에 그려진다. |
| 아직 | 점 명령(`.clear`·`.quit`·`.format` 등) 해석, 수신자(`recipient`)·이름 완성·`resolve`, 실제 송신은 연결하지 않았다. 제출은 위 로컬 처리가 기본이다(`ClientChatWindow.#submitLocal`). 도우미 `>` 메뉴 열기는 e2e 로 확인하지 않았다. |

## 시험

- `e2e/chat-window.spec.js`(Chromium, 스커미시 AI 3명) 두 건: ① 시작 때 채팅 root가 화면 아래(768 이후)에 숨어 있고 지도 영역이 전체 높이이며 도우미가 꺼져 있다. 숨긴 동안에도 로그가 쌓인다. F3를 누르면 지도 영역 아래쪽이 768→672로 줄고 채팅 영역(그림 폭 940 이상, 아래 768)이 올라오며 도우미 표시가 켜지고 첫 로그 줄 글자가 그려진다. 빨강 접두(`~r`)와 되돌림(`~n`)이 줄 안에서 색을 바꾼다. 다시 F3를 누르면 숨겨지고 로그는 남는다. ② 미션을 떠나면 창 상태가 0이 되고 다음 스커미시에서 창이 다시 만들어진다. 오류·경고 0(시험이 픽셀을 읽을 때 브라우저가 내는 Canvas2D 성능 안내만 제외).
- 연관 Node 시험 62건(대응표·fixture·스크롤 글자 위젯·ChatWindow 원본 관측 180행·Gump 루트)과 영향 e2e 묶음 25건(브라우저 입력 폴링·생산 정보·게임 메뉴막대·한국어 Players·한국어 게임 안·미니맵·미션 단축키·D 키·Players 동작·항목·Send X·채팅 창·HUD 배치)이 모두 통과했다(`extracted/webpj-e1169-e2e-2.log`, 3.5분). 처음 실행에서는 위 `whispered` 때문에 8건이 실패했고, HUD 배치 시험의 `chat: 0` 기대는 실제 창이 생겨 `chat > 0` 으로 고쳤다.
- 실제 화면(영어 4:3): 지도 아래에 회색 채팅 영역과 오른쪽 스크롤 칸이 나오고 `Our Priest has been Immobilized`·빨강 `Red` + `line two`가 그려진다.

## 남은 일

- **입력줄 후속:** 점 명령 분배(`.clear`·`.format`·치트 발행 명령)·수신자 접두 해석(`submitChatInput`)·이름 완성(Tab)·도우미 `>` 메뉴 확인·붙여넣기(`paste`)가 남았다.
- **로그 발생원:** 단독 미션의 알림 문구(예 `Our Priest has been Immobilized`)를 `appendLog`로 흘리는 연결(`MessageGump` 안내와 같은 원천).
- 원본 단독 미션 스크린샷에는 채팅 영역이 보이지 않아(기본 숨김) 픽셀 대조는 F3를 직접 눌러 캡처해야 한다.
- 지식(NSInfo) HUD 패널 공장은 이 단위 범위 밖이다.
- 기준선 뒤 cpppj 변경 없음(`RawMenuChat.inc`·`UberHud.inc`).

## 시험(E1170)

- `e2e/chat-window.spec.js` 세 번째 시험(Chromium): 시작은 입력줄 숨김(지도 영역 768) → Enter 로 입력줄이 보이고 지도 영역이 672 로 줄어듦 → `qwa1 hello` 를 치면 입력줄 글자가 되고 입력 큐는 비며 카메라(화면 위치 저장 숫자 `1`)와 생산 단축키가 반응하지 않음 → Backspace 두 번으로 `qwa1 hel` → Enter 제출로 줄이 비워지고 입력줄은 남음 → 빈 Enter 로 숨음 → 다시 열고 ESC 가 게임 메뉴막대를 엶. 오류·경고 0.

## 제출·수신(E1172)

- **원본 파이프라인 연결:** `_campaignChatDevices`(`netstorm.js`)가 원본 `MakeChatContext`처럼 `core/client-chat-runtime.js`의 `bindClientChatRecipients`(수신자 해석·이름 완성 장치)·`bindClientChatMessage`(`say` 조합·41h 패킷 송신)·`bindClientChatSubmit`(대상 해석 → 점 명령 분배 → say)을 묶어 `ClientChatWindow`에 넘긴다. Client 수명에 채팅 manager 전역(`ClientChatManagerGlobals`)과 점 명령 전역(`ClientDotCommandGlobals`)을 만들고, 최초 한 번 issue 명령 네 개(`flash`·`flash`·`desc`·`cheatorama`)와 chat manager를 초기화한다. 점 명령 전역은 채팅 창의 `clear`/`quit` 등록과 제출 분배가 같은 표를 쓴다.
- **송신 경로:** 전투 모드(미션)이면 `sendChatPacket`이 패킷을 로컬 서버(9)로 보내고(`session.sendMutableLocalPacket`), 세션이 41h로 되돌리면 `services.chatReceive`(새로 연결) → `_receiveCampaignChat` → 원본 `dispatchChatPacket`이 무시·채널·관계·옵션 관문을 거쳐 `You> 본문` 줄을 로그에 더한다(발신자 아이콘 접두 포함). 전송 장치가 없을 때만 로컬 대체(`localReceive`)가 같은 수신으로 간다.
- **`ReadChatGlobal`:** 관계표(0x595200 이후 81칸)·숨김 Player(0x52f81e 이후)·구독자·전투 소유자·모드 세 개·채팅 창 상태·manager·메시지 색(이름 팔레트 33)·종류(0x41)를 앱 값으로 읽는다. 알림 Gump serial(0x5ca92c)은 아직 0이다.
- **점 명령:** 창 쪽 효과 `chatGeometryBackend`/`invokeChatCommand`/`printChat`을 `ClientChatWindow`에 더했다. `.clear`(0x42f680)는 로그를 비우고 맨 아래로 간다(`ClientScrollingText.replaceText`·`requestBottom`). `.quit`(0x42e520)는 게임 종료라 웹에서는 콘솔 안내만 한다. 서식·issue 명령은 분배기가 처리한다(`.flash` 섬광은 콘솔 안내뿐이며 Kencloud 연결은 남음).
- **표시 옵션 기본값 결정:** `ShowBattle`·`ShowEveryone`·`ShowAllies`·`ShowObserver`·`ShowWisper`·`ShowMyAllies`·`ShowMyEmenys`·`ShowMyNeutrals`·`ShowChatWindow`·`PlaySoundMessage` 열 개는 원본 `setup.cfg`·아카이브에 기본값이 없다. `Netstorm.exe`를 역분석(capstone)해 보니 키 문자열의 코드 참조는 수신 함수(0x42ed30, `ReadInt` 0x441270 호출)와 도우미 메뉴 토글뿐이고, `ReadInt`는 키가 없으면 0을 돌려주며 출력 변수를 건드리지 않고, 수신 함수의 열 개 지역 변수는 초기화하지 않는다. 즉 원본에서 값이 없으면 스택 잔여값이라 정의된 기본값이 없다. 웹은 모두 **켜짐(1)** 을 기본으로 두고 설정·`<$config>`로 바뀐 값이 있으면 그것을 쓴다(`_chatOption`).
- **버그 수정 두 건:** 메시지 조합의 이름 getter는 문자열이고 패킷의 현재 이름만 NUL 포함 바이트이며(처음엔 둘 다 바이트를 넘겨 `name.indexOf` 오류), 수신 발신자 해석(`findClientChatSenderOwner`)은 getter가 아니라 이름 값을 받는다.
- **시험:** `e2e/chat-window.spec.js` 네 번째 시험(Chromium): chat manager 초기화 → Enter·`hello team`·Enter 로 로그 한 줄(`lineCount` 1, 입력줄은 남고 입력은 비워짐, 첫 줄에 글자 픽셀) → 둘째 줄 → `.clear` 로 0줄 → 빈 Enter 로 입력줄 닫힘. 오류·경고 0. 영향 e2e 묶음 결과는 아래 인계/커밋 기록 참조.
