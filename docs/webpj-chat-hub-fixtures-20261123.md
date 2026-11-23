# webpj 채팅 허브 원본 관측 두 개 연결 (WB04-E1265, 2026-11-23)

[LEFT_JOBS 점검](webpj-left-jobs-audit-20261123.md)(E1263)이 5-C "허브 fixture" 항목에서 찾은 빈칸이다. cpppj `ChatWindowClientTests.cpp` 가 읽는 원본 10.78 관측 두 개가 `webpj/tests/fixtures/manifest.json` 에 없고 웹 시험이 읽지 않았다. 둘 다 manifest 에 올리고 새 시험 `tests/chat-hub-native-fixtures.test.mjs` 가 읽는다.

## chat-input-recipients (720행)

- 원문: 입력줄 대상 포함 검사 `ChatContainsRecipient`(0042e9e0)와 실제 Client 의 현재 Identity 조회(`BindClientChatRecipients`)·공통 ChatWindow 입력의 접합. 열: 모드(0 일반·1 전투·2 편집)·manager 현재값·입력줄 내용(16진)·대상(16진)·Subscriber·반환.
- `contains` 432행: 입력줄 내용과 대상 이름을 주고 포함 여부를 본다. `subscriber` 288행: 식별 기록(0xa5 로 채우고 초기화 뒤 Subscriber 한 DWORD 만 씀)을 실제 조회해 그 이름으로 포함 검사한다(조회가 실패하면 입력 복사를 먼저 하지 않는 사슬).
- 웹 대응: `runtimeScene()`(`client-chat-runtime-support.mjs`, 원문 `ChatIdentityInputs` 와 같은 Player 9행·채팅 기록 입력)에 `bindClientChatRecipients` 로 대상 검사·조회를 묶고, 가벼운 `ChatWindow`(`chat-window.js` `containsRecipient`)에 Console 복사 장치만 공급했다. 모드는 `surface.ownerMode.battle`·`gameplay.world.editor`, manager 현재값은 `chatManagerGlobals().current().value` 로 넣는다.
- 결과: 720행 모두 일치. 변형 확인: `containsRecipient` 가 접두 유효 검사 결과를 무시하게 하면, `bindClientChatRecipients` 가 현재 manager 값 대신 0 을 넘기게 하면 각각 이 시험이 실패한다(원복 확인).

## chat-callbacks (29행)

- 원문: 채팅 창 callback 세 개 — `.clear`(0042f680) 10행, 로그 append(0042f590) 15행, `.quit`(0042e520) 4행. 모든 행의 반환은 0 이다.
- 웹 적응 판정: `.clear` 는 `ClientChatWindow.invokeChatCommand` 가 로그를 비우고 맨 아래로 가며 실제 입력줄 `.clear` 를 e2e(`e2e/chat-window.spec.js`)가 확인한다. 원문 행이 다루는 "callback 도중 현재 창 교체" 변형(currentNext·textNext 등)은 웹의 로그 교체가 append callback 을 거치지 않아 대응이 없다. 로그 파일 쓰기(file valid·write 호출)와 `.quit` 의 종료 큐 예약(RequestQuitApp)은 브라우저에 대응이 없어 웹 제외다(웹 `.quit` 은 콘솔 안내만 하고 0 을 돌려준다).
- 시험이 확인하는 것: 열 이름·행 수(10/15/4)·모든 반환 0, 웹 `invokeChatCommand(0x42e520)` 가 0 을 돌려주고 원문 세 callback 밖의 값은 오류라는 점. 줄 단위 동등이 아니라 적응 판정의 기록이다.

## 후속

- 채팅 로그를 미션 끝에 초기화하는 적응(E1170)을 유지할지는 사용자 결정 대기다(LEFT_JOBS 5-C 메뉴·버튼 컨트롤).
