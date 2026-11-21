# F3 채팅 창 숨김/표시: 구분선 전환 연결 (WB04-E1159)

원본 도움말의 `F3 Hide and show the Chat Window` 를 점검했다. 하단 채팅 구분선의 전환은 되고, 채팅 창 내용물은 아직 없다.

## 점검 결과

- F3(`togglebottomgump`, 명령 `0x02000088`)을 누르면 입력 프레임의 Gump 전달(`gump.input().route`)이 Uber 루트 도킹(`HudDock`)에서 채팅 구분선을 올리고 내린다. 지도 영역(`mapView`)의 아래쪽이 768→672 로 줄었다가 다시 768 로 돌아간다. 전환 로직 자체는 E1082(HUD 틀)에서 이미 이식돼 있었다.
- 문제는 Gump 가 처리하고도 반환값이 0 이라 같은 사건이 게임 메뉴 명령 분배로 한 번 더 내려와, "게임 메뉴 명령 0x2000088 은(는) 아직 연결되지 않아 실행하지 않았습니다" 경고가 눌릴 때마다 났다는 것이다.
- 수정(`www/netstorm.js`): `0x02000088` 을 연결된 메뉴 명령 집합에 더하고(View 메뉴 항목도 활성), 메뉴 명령 분배에서는 구분선을 다시 옮기지 않고 지도 영역이 바뀐 것만 다시 그린다. (처음에는 분배에서 한 번 더 전환했다가 같은 키에 두 번 전환되어 제자리로 돌아가는 것을 시험으로 찾아 뺐다.)

## 한계 (채팅 창 공장)

- 올라온 채팅 영역(96픽셀)은 검은 띠이며 원본의 배경·본문 기록·입력줄은 아직 없다. 이식된 부품(`HudChatGeometry`·`ClientChatRuntime`·텍스트 입력 컨트롤·`raw-chat-receive` 등)을 Gump 트리에 채팅 창으로 조립하는 단계가 남아 있다.
- 채팅 영역 아래로 지도의 일부 픽셀이 겹쳐 그려지는 가장자리 흔적이 있다(지도 그리기 영역과 구분선 사이 정렬).

## 시험

- `e2e/mission-hotkeys.spec.js`: F3 → 지도 아래쪽이 줄고, 다시 F3 → 원래 아래쪽, 그리고 연결되지 않은 명령 경고가 없어야 통과한다. 연결을 빼면 실패한다.

## 채팅 창 조립 설계 메모 (E1161, 다음 단위용)

`ui/chat-window.js`(`ChatWindow`, 원문 0043f820/00431ee0)는 이식돼 있고 노드 생성·부착·로그 추가·입력 처리·도우미 메뉴까지 가지고 있다. 앱에 올리려면 호스트 훅 약 50개를 실제 부품에 연결해야 한다.

| 훅 묶음 | 필요한 것 | 앱에 있는 대응 부품(후보) |
| --- | --- | --- |
| 노드 접근 `chat`·`base`·`console`·`text`·`buffer` | `ChatWindow` 루트(0x13c)·Console(0x128)·ScrollingText(0xd8)·Helper(0x170)·Background(0xb8) 노드 클래스를 같은 Gump 저장소(`root.node(token)`)에 등록 | `ui/text-entry-gump.js`(`TextEntryGumpNode`·`ConsoleGumpNode`), `ui/scrolling-text-gump-widgets.js`, `ui/client-text-entry-controls.js`(편집기·버퍼·Slider 조립) |
| 생성 `allocate`·`constructBase`·`constructHelper`·`constructBackground`·`constructScrolling`·`console` | Gump 토큰 할당(`create.entries`)과 각 생성자 | `ClientTextEntryControls.allocate()/construct()`, `ClientScrollControls` |
| 편집 `entry`(`constructConsole`·`copyText`·`setText`)·`replaceBuffer`·`sharedBuffer`·`bufferAppendCallback`·`setSlider`·`setScrollable`·`requestScroll`·`relativeScroll` | 한 줄 입력과 로그 버퍼 | `ui/text-entry-gump.js`·`text-entry-storage.js`·`client-scroll-controls.js` |
| 부착·표시 `attach`·`geometry`·`requestBottom`·`frameHeight`·`frameCount` | `HudChatGeometry`(이식됨)와 HUD 의 `chatVisible`/`chatInput` | `ui/hud-chat-geometry.js`, `ui/client-uber-hud.js` 의 stub(`chatVisible: () => {}`) 교체 |
| 메뉴 `menu*`·`help`·`title`·`translate` | 도우미 `>` 메뉴(수신자 선택) | `ui/chat-recipients.js`, 기존 메뉴 작성기 |
| 송신·수신 `sendChat`·`resolve`·`recipient`·`log`·`appendLog` | 단독 미션에서는 로컬 기록만 의미가 있다 | `ui/raw-chat-submit.js`, `ui/raw-chat-receive.js`, `core/client-chat-runtime.js`(Client 장기 owner 와 연결) |

권장 순서: (1) `ClientTextEntryControls`/`ClientScrollControls` 로 Console·ScrollingText 노드를 만드는 훅을 먼저 묶고 `ChatWindow.create()` 한 번만 통과시킨다(Node 시험). (2) HUD 의 `chatVisible/chatInput` 을 실제 창의 표시 비트로 교체한다. (3) 단독 미션의 알림 문구(예: `Our Priest has been Immobilized`)를 `appendLog` 로 흘린다. (4) F3 e2e 에 로그 한 줄과 입력줄 표시 확인을 더한다. 원본 단독 미션 스크린샷에는 채팅 영역이 보이지 않으므로(기본은 숨김) 비교 자료는 직접 F3 를 눌러 캡처해야 한다.
