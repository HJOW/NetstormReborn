# 생산 창·스톰 파워 도움말과 채팅 채널 이름의 한국어 번역 (WB04-E1246)

[다음 작업 3](../LEFT_JOBS.webpj.md#next)에 남아 있던 "번역 호출 자체가 없는 생산 창 도움말·미니맵 도움말·채팅 채널 이름의 고정 영어"를 원본·cpppj와 하나씩 대조해 닫았다. 결론: 번역 호출이 **이미 있는데 웹 번역표에 항목이 없어** 영어로 남은 것(생산 창·스톰 파워 도움말, 채팅 채널 이름)은 항목을 더해 고쳤고, 미니맵 도움말은 원본이 번역 호출을 하지 않아 **영어 유지**가 원본과 같다.

## 대조 결과

| 문구 | 원본·cpppj | 웹 코드 | 웹 번역표(변경 전) | 처리 |
| --- | --- | --- | --- | --- |
| `Storm Power Available` (스톰 파워 표시 도움말) | `RawProductionControls.cpp:179` 가 `hooks.translate(...)` 로 받아 `SetHelpText` | `client-production-assembly.js` 가 `devices.setNodeHelp(token, devices.translate(text), false)` (번역 거침) | 없음 | **항목 추가** `사용 가능한 스톰 파워` |
| `Production Window` (생산 창 격자·가운데·아래 배경 도움말) | `RawProductionControls.cpp:183~186` 세 곳 모두 `hooks.translate` | 같음 | 없음 | **항목 추가** `생산 창` |
| `EVERYONE`·`ALLIES`·`BATTLE` (채팅 채널 이름) | `ChatRecipients.cpp:41~42`·`RawChatSubmit.cpp` 가 여덟 키를 순서대로 번역해 입력의 수신자 이름과 비교 | `chat-recipients.js`·`raw-chat-submit.js`·`chat-name-completion.js` 가 같은 번역 호출(`byteString(translate(text))`, 표시 코덱으로 인코딩) | 없음 | **항목 추가** `모두`·`동맹`·`전투` |
| `OBSERVER` | 같은 여덟 키의 하나 | 같음 | 없음(cpppj 번역표에도 없음) | 그대로 둠(관전은 단독 미션에 없다) |
| `Sky Overview` (미니맵 도움말 제목) | `RawMinimapControls.cpp:67` 이 `SetHelpText(token, "Sky Overview")` 로 **번역 호출 없이** 리터럴을 넣고 `SetNodeHelp` 도 번역하지 않는다 | `raw-minimap-controls.js` 가 같은 리터럴을 직접 `setHelpText` | 없음 | **영어 유지**(원본과 같음). cpppj 번역표에는 항목이 있으나 코드가 쓰지 않는다 |

번역 문구는 cpppj 한국어 번역표(`assets/lang/korean/xlat.korean`)의 같은 원문 번역과 일치시켰다. 시험이 두 표의 번역이 어긋나지 않는지 대조한다. 변경 파일: `webpj/translations/xlat.korean`(5항목 추가, 405→410개), `webpj/www/data/d/xlat.korean`·`manifest.json`(`tools/prepare-assets.js` 로 재생성: 크기 26,579→26,770바이트, `--check` 통과).

## 다시 대조해 본 것(번역표 격차 전체)

웹 번역표(405개)와 cpppj 번역표(513개)를 대조하면 cpppj에만 있는 원문이 308개이다. 웹 소스에서 `translate(리터럴)`·`help(토큰, 리터럴)`·`setHelpText(토큰, 리터럴)` 로 직접 요청하는 영어 리터럴 34개 가운데 웹 표에 없던 것은 위 항목과 다음 셋뿐이었다.

- `%d Priest%s`: Fort 사제 수를 `global.priests` 설정에 쓰는 문구(`raw-campaign-session-install.js`, 원본 `RawCampaignSession.cpp:3466`). 원본 대본 어디에도 `{global.priests}` 가 없어 화면에 나오지 않으며, cpppj 번역 `사제 %d명%s` 는 복수 접미 `%s` 가 한국어에도 `s` 를 붙이는 결함이 있어 **넣지 않았다**(필요해지면 `사제 %d명` 으로 따로 정한다).
- `LAN Server`: 멀티플레이어(웹 범위 밖).
- `Type`: 도움말 주제 이름(`bridgeType`)을 만드는 접미사라 번역하면 주제를 못 찾는다(E1118 결정).

cpppj에만 있는 나머지(저장·편집기·서버 선택·치트·기술 오류·메모리 경고 등)는 웹이 해당 UI를 쓰지 않거나 번역 호출이 정적 리터럴이 아닌 경로(소스 기준 번역 누락 확인은 E1166)다. 이 단위는 그 전체를 옮기지 않았다.

## 시험

- `tests/korean-production-chat-xlat.test.mjs` 4건(Node): 다섯 항목이 웹·cpppj 표에서 같은 번역임; 생산 패널 소스의 `parts.help(토큰, '원문', …)` 호출 원문이 정확히 두 개이고 모두 번역표에 있음(새 도움말을 더하면 항목이 없을 때 실패); 조립부가 번역 장치를 거침; 세 채팅 소스의 `CHANNELS` 배열이 같은 여덟 칸이고 `OBSERVER` 는 두 표 모두 항목이 없음; 미니맵 도움말은 번역 없이 리터럴을 쓰고 표에도 없음. 번역표를 변경 전(HEAD) 내용으로 되돌리면 앞의 세 시험이 실패함을 확인하고 복원했다.
- `e2e/korean-ingame.spec.js` 새 시험(실제 Chromium, `?lang=ko`): 첫 캠페인 미션을 시작해 노드 도움말 제목이 스톰 파워 `사용 가능한 스톰 파워`(주제 `moneyGump`), 생산 창 격자·가운데·아래 `생산 창`(주제 `teleportView`), 미니맵 `Sky Overview`(주제 `minimapGump`)이고 다섯 노드 모두 우클릭 처리·팝업이 열리며, 채팅 채널 키가 `['전투','모두','동맹','OBSERVER']` 로 번역됨을 확인한다. 실제 마우스 우클릭은 생산 창 격자에서 팝업이 열리고 스크린샷(`korean-hud-help-popup.png`)을 남긴다. 같은 파일의 기존 시험도 통과했다.
- 전체 회귀: Node 26.13.0 전체 6,782건 모두 통과(실패 0, 374초, 로그 `extracted/webpj-e1246-node26.log`). 한국어·채팅 e2e 7개 파일 20건(korean-campaign·korean-font·korean-mission·korean-players-menu·korean-ingame·send-money-dialog·chat-window)도 모두 통과했다.

## 이번에 발견한 별도 문제(고치지 않음)

스톰 파워 표시(왼쪽 위 `3000` 칸, 노드 위치 (7,2) 크기 66×21)의 **실제 마우스 우클릭 경로가 노드에 닿지 않는다**: Uber 루트 입력 `route(0x20000, 40, 12)` 가 0(처리 노드 없음)을 돌려주고, 같은 방식으로 생산 창 격자 한가운데 (40, 80)은 격자 토큰을 돌려준다. E1236 은 이 노드의 우클릭 도움말을 `root.handle(token, …)` 직접 호출로만 확인했다(미니맵은 실제 클릭으로 확인). 원본이 이 칸을 실제 우클릭에서 도달 가능하게 하는지(텍스트 노드의 판정 플래그·`Attach` 모드 2)와 웹이 어디서 막는지는 아직 대조하지 않았다 → 인계 문서 다음 작업 1 ⑨.

## 남은 일

- 번역 문구의 어색함과 위 `사제 %d명%s` 같은 cpppj 번역표의 결함은 개발자 검수 범위이다.
- "한국어 미션의 실제 화면 확인"(다음 작업 3의 마지막 부분)은 이 단위에서 생산 HUD 도움말만 확인했다. 나머지 미션 화면(대본 본문 줄 나눔 등)은 남았다.
- 원본과의 화면 픽셀 대조는 하지 않았다.
