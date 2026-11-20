# Players 메뉴의 송금·동맹 동작 연결 (WB04-E1126)

스커미시·캠페인의 ESC ▸ Players ▸ (AI 이름) 하위 메뉴에는 `Find Priest!`·`Send Money ▸ (300/700/1000/X/All)`·`Request/Break Alliance` 가 있다. 이 중 `Find Priest!` 만 동작했고, **송금 항목을 누르면 입력 큐 머리에 명령이 영원히 남아 이후 모든 마우스·키 입력이 먹통**이 됐다. (앱 입력 루프는 미구현 명령을 앞질러 가지 않도록 첫 미인식 코드에서 멈춘다.) 동맹 명령은 "아직 연결되지 않았다" 로 버려졌다.

## 구현

| 영역 | 내용 |
| --- | --- |
| 송금 입력 | 이미 이식돼 있던 `core/send-money-input.js` 의 `dispatchSendMoneyInput` 을 앱 입력 루프에 연결했다(허용 코드 목록 + 분배 가지 `_dispatchCampaignSendMoney`). 뗌과 열린 창 뒤의 입력은 소비만 한다. |
| 세션 장치 | `RawCampaignSession.moneyInputHooks()` 를 추가했다(원본 `MoneyInputHooks`): 로컬 SP(암호화 저장소의 float)·잔액 판정(편집 모드는 생략)·Player 플래그 `+0x50`·로컬 SP 가산(`#addMoney` 와 같은 보상 상태)·AI 지갑 가산(`creditWallet`)·연결 Player 에게 11바이트 송금 packet·진단. |
| 모두 보내기 | `askAll` 은 `asksend(대상)` Tell 창을 연다. 그 창의 `$Button=예,SendAllBucks,{param.1}` 단추 이름을 명령 번호로 풀지 못해 아무 일도 하지 않았으므로, 단추 명령 이름 해석(`gameMenuScriptCode`)에 송금 이름표(`sendMoneyScriptCode`)를 넣고 안내 창 단추 처리(`_runDialogCommand`)가 송금·동맹 이름을 입력 큐에 올리게 했다(`_queueDialogPlayerCommand`). |
| 동맹 입력 | 원본 `HandleAllianceRequestInput` 을 `raw-player-participation.js` 의 `handleAllianceRequestInput` 으로 이식했다. 요청(fb)·해제(fa)·확인(7e)·무승부 선언(db)이 6바이트 packet `{1, 종류, 6, 0, 로컬 owner, 대상}` 을 서버(9)로 보낸다. 요청만 대상마다 60초에 세 번으로 제한하며 넷째부터는 `AllianceSpam(제한,색,이름)` 안내만 연다(무승부 대상은 항상 1). |

## 시험

- `tests/alliance-request-input.test.mjs`(5건): 네 명령 외 거부·뗌 소비·수정 비트 제거, 해제/확인/무승부의 제한 없음, 요청의 최초 아홉 시계 저장·창 안 세 번·넷째 `AllianceSpam`·61초 뒤 창 갱신, 대상 범위 밖(제한만 갱신)·제한 변경·이름 NUL 오류, 필수 장치 누락. 변형 5개를 모두 시험이 잡았다.
- `tests/game-menu-commands.test.mjs`: 송금 여섯 이름이 대소문자 무시로 원본 번호가 됨. 기존 고정 상수(`BY_NAME` 29→31, 미등록 이름 14→12)는 송금 이름 두 개가 이 표로 옮겨진 것에 맞게 고쳤다.
- `tests/netstorm-gameplay-services.test.mjs`: 안내 창 단추 이름 → 입력 큐 명령(대상 두 인자 동일, 다른 이름·큐 없음은 건드리지 않음).
- `e2e/player-menu-actions.spec.js`(Chromium, 스커미시 AI 3명): 고정 300 송금으로 로컬 SP 가 정확히 줄고 큐가 비며, 700·1000 도 합계 1,700 만큼 줄고, `모두 보내기` 확인 창의 첫 단추가 남은 SP 를 모두 보내고(5 미만), 동맹 요청 네 번이 모두 제한 상태에 반영되며 큐가 막히지 않고 해제 뒤에도 입력이 이어짐을 확인한다. 오류 0.

## 남은 경계

- **Send X(숫자 입력 창)**: 원본은 편집용 `SaveFortNameDialog` 와 같은 숫자 입력 창을 쓰는데 웹은 그 대화창을 이식하지 않았다(대응표 `exclude-edit`). 지금은 경고만 남기고 소비한다(큐는 막지 않는다). 웹 숫자 입력 창을 만들 때 `confirmSendMoney` 를 확인 단추에 연결한다.
- **Ignore chat·Whisper To·Player 정보 보기**: 채팅 창과 함께 연결해야 한다(Players 메뉴의 나머지 항목, 3.1절 6단계 '게임 메뉴').
- **입력 루프의 안전 장치**: 앱 입력 루프는 아직 인식하지 못하는 명령 코드를 만나면 큐를 건드리지 않고 멈춘다(원본 순서 보존 목적). 다른 미연결 명령이 큐에 오르면 같은 먹통이 날 수 있으므로, 새 UI 항목을 연결할 때 그 항목이 올리는 명령을 반드시 입력 루프에 넣는다(또는 경고 후 소비하도록 바꾼다). 이번 조사에서 Players 메뉴의 송금 항목이 이 문제였다.
