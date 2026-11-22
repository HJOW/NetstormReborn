# Send X 숫자 입력 창: 입력창·모달·키보드 기반을 앱 미션에 연결 (WB04-E1168)

Players ▸ (Player) ▸ Send Money ▸ **Send X**는 원본에서 숫자를 받는 모달 대화창(`SaveFortNameDialog::ShowMoney`, 원문 004d3860/004d3b60, 제목 `Send X Storm Power`)을 연다. 웹에는 이 대화창이 없어 명령만 소비하고 경고를 남겼다. 이 단위에서 대화창과, 그것이 요구하는 **미션 화면의 글자 입력창·모달 등록·키보드→입력창 경로**를 앱에 처음 연결했다. 같은 기반이 채팅 창(`ui/chat-window.js`) 조립에도 쓰인다.

## 구현

| 영역 | 내용 |
| --- | --- |
| 원본 대화창 이식 | [save-fort-name-dialog.js](../webpj/www/ui/save-fort-name-dialog.js)의 `SaveFortNameDialog.showMoney`/`buttons`/`handle`(cpppj `SaveFortNameDialog.cpp`의 ShowMoney·Buttons·Handle). 호출 순서(번역은 제목→OK→Cancel), 장식 틀 200×100, 입력 폭 `baseWidth×0.8` 절삭·높이 18, 입력 상한 10(실제 입력은 아홉 문자), 입력 금지 문자(1..255 중 ASCII 숫자 아닌 모든 바이트), 테두리(폭+8·높이+4, 위치 −4·−2), 제목(fit→push→정렬 4→방식 2 재부착), 두 버튼(합 폭+10, 가운데), 높이 +4·폭 `max(200, 제목+32)`, 화면 가운데 부착, 깊이 `depth−76`, 모달 등록, OK 단추·Enter 확정(callback 비영이면 Kill)·Cancel·그 밖은 기본 Help 위임. float 산술은 단계마다 `Math.fround`. 편집용 이름 입력(Save as·설명·HTML `$Input`·Scenario)은 웹 범위 밖이라 옮기지 않았다. |
| 앱 조립 | [client-money-dialog.js](../webpj/www/ui/client-money-dialog.js)의 `ClientMoneyDialog`가 기존 부품을 한 대화창으로 묶는다: 장식 틀(`ClientGumpDecoration`, 파생 노드·가상 처리기 선택을 이 단위에서 추가), `ClientTextEntryControls`(숫자 입력창)·`ClientStoneControls`(OK/Cancel)·`ClientTextGump`(제목)·배경(테두리). 입력창은 출력 시점의 표면으로 매번 `createClientTextOutput`을 만들어 그린다. 모달은 `setModal`(원문 00466600: dialog 표 등록, 이전 모달이 있으면 그 하위 최소 깊이 앞으로 맞춤). 확정은 원본 004c9350 `confirmSendMoney`(양수·잔액·`canAfford` 뒤 이체, 잘못된 금액도 닫음)를 실제 Session 송금 장치(`moneyInputHooks()`)로 부른다. |
| 앱 연결 | [netstorm.js](../webpj/www/netstorm.js): `_dispatchCampaignSendMoney`의 `numberDialog`가 대화창을 연다(이미 열려 있으면 같은 창). `_campaignInputDialogOpen()`이 모달 창 토큰을 돌려주고, 열려 있는 동안 ① `_onKeyDown`/`_onKeyUp`이 모든 키(문자·Backspace·Enter·방향·Home/End/Delete)를 입력 큐에 올린다(게임 단축키·TAB·숫자 화면 저장으로 새지 않음), ② 입력 프레임이 큐의 모든 사건을 Gump 입력(`route`)에만 보내고 월드·단축키로는 보내지 않는다(바깥 클릭도 월드로 새지 않음). 마우스 OK/Cancel은 기존 포인터 경로가 큐에 올린다. |
| 버튼 소리 | Gump 버튼 본체(`ButtonGump`)는 인자 없이 소리를 요청하는데 앱 장치는 이름을 요구해 클릭 처리 중 `소리 이름이 비었거나 NUL을 포함합니다`로 터지고 눌림이 남았다. 원본 UberGump의 `button.wav`를 채웠다(`client-money-dialog.js`·`client-production-menu.js`의 스크롤 컨트롤). 생산 메뉴의 스크롤 화살표에도 같은 잠복 결함이 있었다. |
| 한국어 | 제목 `Send X Storm Power`의 한국어 번역(`스톰 파워 보내기`)을 `translations/xlat.korean`에 더했다(OK·Cancel은 기존 `확인`·`취소`). 돌 버튼 라벨은 게임 표시 부호화(선두 0x1f 네 바이트)로 넣고, 입력창이 쓰던 **혼합 글자(원본 바이트+확장 글리프) 출력·측정**을 `createMixedGumpTextDrawer`(`client-text-entry-draw.js`)·`createMixedGumpTextMeasure`(`client-text-entry-controls.js`)로 분리해 돌 버튼도 같은 경로로 그리고 잰다. 처음에는 `fonts.encode`(UTF-16 단위)를 라벨에 넣어 한국어 모드에서 글자가 깨졌다. |
| 대응표 | `tools/port-map.mjs`의 `edit-fort-save`에서 `SaveFortNameDialog`·`RawMenuSaveFort`를 빼 `campaign-send-money-dialog`(port) 규칙으로 옮기고 두 행을 partial로 채웠다(편집 전용 가지는 이식 대상이 아님). |

## 시험

- `tests/save-fort-name-dialog.test.mjs`(7건): 가짜 장치로 호출 순서·번역 순서·할당 크기·필드(vtable 0x00517ca0·command 68·대상·취소 가능)·입력 한도·금지 문자 245개·모든 배치 좌표(화면 640×480 → (220,188), 제목이 넓을 때 폭 332)·depth/modal·확정/취소/위임·필수 장치 누락 거부. 첫 실행에서 기대값 한 곳을 고쳤다(깊이는 `AddDepth` 변화량으로 `현재 깊이−76`을 넘김).
- `e2e/send-money-dialog.spec.js`(Chromium, 스커미시 AI 3명, 영어 4:3): Send X 명령(대상 2)으로 창이 열리고 화면 가운데(412,332)-(612,436)에 놓이며 처음 입력이 선택된 `0`이다. 이미 열려 있으면 두 번째 창이 생기지 않는다. 키보드로 `5q0-0 `을 쓰면 `500`(문자·기호·공백 무시), Backspace 뒤 숫자를 계속 넣으면 아홉 문자(`500123456`)에서 멈춘다. 비운 채 Enter는 송금 없이 닫고, `500`+Enter는 로컬 SP −500·AI 지갑 +500, OK 단추(마우스)로 300, Cancel 단추는 보내지 않고 닫는다. 모달이 열린 동안 바깥 클릭이 창을 닫거나 월드로 새지 않고 입력 큐가 비어 있다. 콘솔 오류·경고 0.
- 실제 화면(영어): 석판 틀·모서리·제목·입력줄·OK/Cancel이 원본 형태로 나온다.
- `e2e/send-money-dialog.spec.js` 두 번째 시험(한국어 스커미시): OK/Cancel 라벨이 `확인`·`취소`로 부호화(선두 0x1f)되고 폭·높이가 측정된다. `tests/save-fort-name-dialog.test.mjs`의 8번째 시험: 창이 번역 장치에 넘기는 세 문구(제목·OK·Cancel)가 모두 한국어 번역표에 있다.
- 연관 Node 시험 110건(Gump 루트·장식·돌 버튼·입력창·메뉴·게임 메뉴·키 이벤트·입력 폴링·대응표)과 번역표를 읽는 시험 154건(한국어 번역·미션·세션 적재)을 돌렸다.
- 영향 e2e 묶음 19건(브라우저 입력 폴링 7·생산 정보·게임 메뉴막대 4·한국어 Players·미니맵·미션 단축키·D 키·Players 동작·Players 항목·Send X) 모두 통과했고(`extracted/webpj-e1168-e2e.log`, 3.2분), 번역표를 바꾼 뒤 한국어 e2e(`korean-ingame`·`korean-font`)와 Send X 두 건을 다시 돌려 4/4 통과했다.

## 한계와 남은 일

- IME(한글 조합)로 입력하는 경로는 이 창에 필요 없다(숫자만 받는다). 채팅 창이 같은 기반을 쓸 때 IME 요소 활성(`_activateClientTextInput`)과 붙여넣기를 연결한다.
- 한국어 번역 문구(`스톰 파워 보내기`)는 개발자 검수 대상이다.
- 원본 화면 캡처와의 픽셀 대조는 하지 않았다(원본 스크린샷에 이 창이 없음).
- 기준선 뒤 cpppj 변경 없음(`SaveFortNameDialog.cpp`·`RawMenuSaveFort.inc`).
