# webpj 방향키 카메라 이동 (WB04-E1291, 2026-11-24)

사용자 요구(2026-11-23, [LEFT_JOBS.md 공통](../LEFT_JOBS.md#user-requests-20261123))를 cpppj 1328(`d2bcefeb4`, [계약](exe/cpp-arrow-camera-1328-20261123.md))의 입력 계약대로 웹에 반영했다. 방향키 화면 이동은 원본에 없는 추가 요구이다.

## 계약 (cpppj `UberGump::Event`·`UserInput::Event`)

- 게임 화면에서 ←↑→↓ **누름 사건마다** 논리 화면 32픽셀을 옮긴다(위·왼쪽 음수). 누르고 있을 때는 브라우저 키 자동 반복 사건마다 한 번씩 더 옮기고 별도 프레임 타이머·키 보관은 없다. 같은 프레임의 여러 사건은 현재 카메라 **목표**에 누적하며 지도 경계 제한은 기존 Present가 적용한다. FPS·출력 배율은 이동량에 영향이 없다.
- 입력 UI 우선: 대화창·도움말·메뉴·팝업·채팅 입력줄이 있으면 세계로 보내지 않는다. 활성 조각의 ←/→는 기존 `RawCampaignCursor.HandleInput`(회전 ∓2)이 먼저 소비하고, ↑/↓는 조각을 들어도 카메라 이동이다. 정지(Shift-F9)·키 뗌은 기존 세계 입력 관문이 거부한다. 입력 UI가 소비하지 못한 방향키도 세계 꼬리로 다시 보내지 않는다.
- 채팅 입력줄이 숨겨져 있으면 방향키는 카메라가 먼저이다(원본 ChatWindow는 숨긴 입력줄에서도 ↑/↓를 로그 스크롤로 소비했으나 11.01은 우선순위를 바꾼다).

## 웹 변경

- `www/ui/uber-gump-camera.js`: `requestKeyDelta(dx, dy)` — 기준점을 현재 적용 위치가 아니라 **목표**(`target().pixels`)로 두고 기존 `requestCameraDelta`를 호출해 같은 프레임의 여러 요청이 쌓이게 한다(`metaRebuild`도 요청마다 증가).
- `www/core/campaign-user-input.js`: `ARROW_CAMERA_STEP`(32)·`ARROW_CAMERA_STEPS`(키 번호별 방향)와, `event()`에서 조각 회전(`cursor.handleInput`) 뒤 남은 방향키를 `services.cameraKey(dx, dy)`로 보내는 분기를 더했다. 정지·키 뗌은 `event()` 앞쪽 관문이 그대로 거부한다. 장치가 없으면 조용히 건너뛰지 않고 오류이다.
- `www/netstorm.js`:
  - 방향키 네 개를 항상 입력 큐에 넣는다(이전에는 조각을 든 ←/→만 큐에 들어갔고 ↑/↓는 큐에 들어와도 미구현 사건으로 3초 뒤 console 오류와 함께 버려졌다).
  - `_runCampaignInputFrame`에 방향키 분기를 더했다: Guide 안내 창(`tutorial.event`)이 먼저, 그다음 새 `_campaignArrowKeyBlocked()`(대화창·도움말 `_mainMenu.dialog`, 팝업 `blockingPopups()`, 게임 메뉴막대, 지식 창)이면 Gump로만 보내고 세계로는 보내지 않으며, 아니면 `user.event`로 보낸다. 채팅 입력줄과 Send X 입력 창이 열린 경우는 입력 루프 앞쪽의 기존 분기가 입력줄·모달로 보낸다(←/→ 커서 이동 보존). `cursorRotate` 변수는 이 분기로 대체해 지웠다.
  - `CampaignUserInput` 서비스에 `cameraKey`를 연결했다(`_campaignCameraDevice().requestKeyDelta`).

## 검증

- Node 시험: `tests/campaign-user-input-left.test.mjs`(조각이 없으면 네 방향 모두 카메라, 조각을 든 ←/→는 회전만, ↑/↓는 카메라, 뗌·정지 거부, 수정키가 섞인 코드, 장치 없음 오류)와 `tests/uber-gump-camera.test.mjs`(같은 프레임 세 요청의 목표 누적·`metaRebuild` 증가·이전 값에 이어 더하기) 추가, 두 파일 모두 통과.
- 새 e2e `e2e/arrow-camera.spec.js`(Chromium 4:3·16:9 각 1건 통과, The War Begins!): 네 방향 ±32 후 원래 자리 복귀, 누르고 있을 때 32의 배수만큼 자동 반복(세로는 그대로)·뗌에서 정지, Shift-F9 정지 중 불변, ESC 메뉴막대가 열려 있을 때 네 키 각각 불변, 채팅 입력줄(Enter)이 보일 때 네 키 각각 불변, 조각(Q)을 든 채 →는 +2·←는 −2 회전하고 카메라는 그대로이며 ↓는 조각을 들어도 +32 카메라(회전 없음).
- 변형 확인: 카메라 이동 분기를 끄면 e2e의 첫 '오른쪽'에서 실패하고, 입력 UI 관문(`_campaignArrowKeyBlocked`)을 끄면 '열린 메뉴 중 ArrowLeft'에서 실패한다. 처음에는 네 방향을 한꺼번에 누르고 합계만 비교해 상쇄되어 이 변형을 놓쳤고, 키마다 확인하도록 e2e를 고쳤다.
- 관련 e2e 22건(`mission-rotation-key`·`mission-camera-alt`·`mission-camera-keys`·`mission-hotkeys`·`chat-window`·`game-menu-bar`·`mission-unit-menu`·`send-money-dialog`) 통과, 입력·카메라 Node 시험 통과.

## 판정·남은 것

- cpppj 1328의 `ProductPresentation` 외 두 파일(`UberGump.cpp`·`UserInput.cpp`)은 이 단위에서 ② 웹 재반영을 마쳤고 대응표 base를 `d2bcefeb`로 올렸다.
- 물리 키 반복 속도·실제 DPI·`event.repeat`의 브라우저별 차이는 확인하지 못했다(Chromium의 `keyboard.down` 반복만 사용). 채팅 입력줄이 숨겨졌을 때의 ↑/↓ 로그 스크롤은 11.01 규칙대로 카메라가 우선이라 웹 채팅 창에서도 로그 스크롤에 쓰이지 않는다.
