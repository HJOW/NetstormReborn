# T 키 게임 시간 표시와 우측 상단 버튼 간격 (WB04-E1097)

미션 중 T 키가 오른쪽 위의 `Game Time h:mm:ss` 안내를 켜고 끈다. 원본 `UberGump`의 T/t 분배(`DispatchGameTimeKey`)와 메인 루프의 `ShowGameTimeMessage`(`00439b43..64`)를 연결했다. 이전에는 T가 입력 큐에서 소비되지 않았다.

## 구현

- **키.** T/t(`0x54`/`0x74`)를 문자 키 큐와 입력 프레임 허용 목록에 더하고 `dispatchGameTimeKey`가 정지 중이 아니면 표시 값(`showGameTime`, 원본 `00594fc0`)을 뒤집는다. 모달 창이 열려 있으면 처리하지 않는다.
- **안내.** Kernel 프레임 뒤 `ownerMode.battle`이고 표시 값이 켜져 있으면 매 프레임 `ClientWarnings.gameTimeStatus()`(종류 16, 플레이어 1)를 요청한다. 같은 안내가 있으면 새로 만들지 않고, 기한 0.1초가 지나면 지워져 다음 프레임에 새 시각으로 다시 만들어진다(원본 설계). 시각은 `readGameTimeSeconds`(CRT clock − 전투 시계 원점)다. 이 앱의 모든 캠페인 미션이 `battle` 모드이므로 미션 어디서나 동작한다.
- **우측 상단 버튼 간격.** 웹 전용 `☰ Menu` 버튼이 오른쪽 위 안내(Game Time·Game Paused!)를 가리던 문제를 고쳤다. `GameMenuButton.logicalInset()`이 버튼 폭+간격을 논리 픽셀로 환산하고, `WarningMessages.setView(…, inset)`가 안내를 오른쪽 끝에서 그만큼 안쪽에 띄운다(등록표의 화면 폭은 그대로, 기본 0이라 원문과 같다).

## 검증

- `tests/game-menu-button.test.mjs` 2건(배치·논리 폭 환산), `tests/warning-messages.test.mjs` 1건(inset). 
- 새 `e2e/game-time-key.spec.js`(Chromium): T가 `Game Time` 안내를 켜고(`Game Time : 0:00:02`가 버튼 왼쪽에 온전히 보임) 다시 누르면 끄며 오류가 없다. 일시정지 e2e 2건도 통과한다.
