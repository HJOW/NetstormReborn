# 일시정지(Pause)와 경고·상태 안내 (WB04-E1089)

미션 중 Shift-F9와 게임 메뉴 Options ▸ "Pause - Shift-F9."가 게임 시계와 소리를 멈추고 화면 오른쪽 위에 `Game Paused!` 안내를 띄우며, 한 번 더 누르면 풀린다. 이를 위해 이식만 해 두었던 경고·상태 안내 처리기(`WarningMessages`)를 실제 Client Gump 루트에 처음 연결했다. 이전에는 Shift-F9가 입력 큐에서 비활성 명령으로 소비되고 Pause 항목은 비활성이었다.

## 구현

- **안내 처리기 연결.** [ClientWarnings](../webpj/www/ui/client-warnings.js)가 cpppj `UberGump`의 `warnings_`(`EnableWarnings` → `WarningMessages`)에 해당하는 소유자다. 안내 한 줄은 Gump 루트의 직계 자식인 글자 Gump 파생 노드이며(`ClientTextGump.allocateDerived`) 화면 오른쪽 끝에서 12픽셀 간격의 칸(최대 20칸)에 붙는다. 기한이 있는 안내는 벽 시각 idle이 지운다. `WarningMessages`에 `common.allocate(node)` 훅을 더해(없으면 원문처럼 `common.token()`) 루트에 파생 노드 객체를 등록해 토큰을 받는다.
- **원본 입력.** 메시지 생성자가 읽는 이름·모드·게임 시간 장치(`pauseContext`)와 안내 억제(`missionPost`일 때 종류 11·16만 통과), 경고음(`newWarnMessage.wav`)을 앱에서 채운다. 메타맵 참가자 이름은 웹 제외라 연결하지 않는다. 안내는 `ClientProductionPanel.warnings()`로 앱에 노출되고 미션 반납 때 같이 닫는다.
- **Pause 명령.** `_dispatchCampaignPause`가 이식해 둔 `GameMenuActions`의 Pause 분기(미션·편집기에서만; 정지 중이면 시계 재개·소리 복원, 아니면 커서 취소·시계 정지·음소거; 두 경우 모두 종류 11 안내를 만들거나 지움)를 실제 장치에 연결한다. 명령 `0x020000cf`가 입력 프레임의 메뉴 명령 분배와 Shift-F9(`queuePlayerListKey`)에서 들어온다.
- **음소거.** 원본 `SoundMusic::PushMute/PopMute`를 옮겨 `SoundPlayer.pushMute/popMute`(전체 음량을 최소로 낮추고 예약, 보류 깊이)와 `AudioSystem.pushMute/popMute`(음악 음량 노드를 0으로, 보류 중 단계 변경은 해제 때 적용)를 추가했다.
- **정리.** 앱이 올린 정지·음소거 깊이를 세어 두었다가 미션을 떠날 때(`_releaseCampaignWorld`) 남은 만큼 되돌린다. 정지한 채 Leave/Restart를 해도 다음 미션의 시계와 소리가 멈춘 채 남지 않는다.

## 검증

- 새 `tests/pause-mute.test.mjs` 2건(효과음·음악 음소거 깊이와 예약 복원), `tests/warning-messages.test.mjs` 1건(`common.allocate` 경로). 변형 12종(예약 제거·깊이 가드 제거·복원 제거·음악 0 설정 제거·보류 중 단계 반영 제거·allocate 무시 등)이 모두 시험 실패로 잡힌다.
- 새 `e2e/pause-menu.spec.js` 2건(Chromium): Shift-F9가 시계를 멈추고 `Game Paused!`(`~E~wGame Paused!`) 안내를 띄우며 정지 중 게임 시간이 흐르지 않음, 다시 누르면 풀림; Options 메뉴 Pause 항목이 같은 정지를 걸고 정지한 채 미션을 떠나면 시계·정지 깊이·음소거 깊이가 0으로 돌아옴. 안내는 화면 오른쪽 위에 그려진다.

## 남은 경계

- 웹 전용 우측 상단 `☰ Menu` 버튼이 오른쪽 위 첫 줄 아래쪽 일부를 가린다(안내 칸은 y=0부터 12픽셀씩 쌓인다). 안내가 많이 쌓이면 버튼 뒤로 가려질 수 있어 버튼 위치는 필요하면 조정한다.
- F7 배너·F3 채팅 전환·Players 하위 항목, T키(게임 시간 표시 `gameTimeStatus` 연결)는 남아 있다. 경고 안내 처리기는 이제 연결돼 있어 `add`(스크립트 경고)·`gameTimeStatus`·전투 통지는 같은 `ClientWarnings`로 이어 붙일 수 있다.
