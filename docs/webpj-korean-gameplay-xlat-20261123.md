# 미션 화면의 번역 누락 15개 보강과 도움말 없음 안내 번역 (WB04-E1249)

[다음 작업 3](../LEFT_JOBS.webpj.md#next)의 "나머지 번역"을 이번에는 **번역 호출이 있는데 번역표에 항목이 없는 문구**를 체계적으로 찾아 닫았다. E1118·E1166 의 수집은 `translate('리터럴')` 호출을 기준으로 해서 문구가 배열·상수에 들어 있는 경우를 놓쳤다.

## 찾은 방법

1. cpppj 한국어 번역표(`assets/lang/korean/xlat.korean`, 513개)에는 있고 웹 번역표(`webpj/translations/xlat.korean`, 410개)에는 없는 원문 303개를 뽑아, 웹 소스(`webpj/www/**/*.js`)에 같은 문자열 리터럴이 있는 58개로 좁혔다.
2. 웹 소스에서 Gump 서식(`~색`·`~[아이콘]`·`<c>`·`%s`)을 가진 영어 리터럴 중 웹 번역표에 없는 83개를 따로 뽑았다(cpppj 표에도 없는 문구를 찾기 위해).
3. 각 후보의 **소비 코드를 읽어** 번역 호출을 거치는지, 단독 미션에서 화면에 나오는지 판정했다.

## 보강한 15개 (번역표에 추가)

| 문구 | 소비 코드(번역 호출) | 한국어 |
| --- | --- | --- |
| 커서 배치 경고 5개: `Priest can't get here.`·`Must be placed over constructing artifact`·`Cannot place here`·`Energy not satisfied`·`Only on Controlled Island` | `raw-cursor-lifecycle.js` 의 `WARNINGS[index]` → `h.translate` | 사제가 이곳에 올 수 없습니다. 등(cpppj 표와 같음) |
| 생산 힌트 2개: `~0~w~ENot enough Storm Power to build this`·`~0~w~ENone in inventory` | `production-hint-gump.js` `hooks.translate(…)` | `~0~w~E이것을 건설할 스톰 파워가 부족합니다`·`~0~w~E보유한 것이 없습니다` |
| 객체 정보 2줄: `~yCapturable!~.\n`·`~yNot Connected!~.\n` | `raw-object-menu-info.js` `h.translate(…)` | `~y포획 가능!~.\n`·`~y연결되지 않음!~.\n` |
| `No help information available` | **원본 `QuickHelpWindow::Show` 는 치환 뒤 본문이 비면 `Translate`** — 웹 `renderHelpTopic` 은 번역하지 않았다(코드 수정, 아래) | 사용할 수 있는 도움말 정보가 없습니다 |
| Players 메뉴 Send 5줄: `Send %s300/700/1000/ X/ All~[IfortGump.3]` | `raw-player-menu.js` `format(labels[i], …)` → `translate` | `%s300~[IfortGump.3] 보내기` 등 |

- 번역은 cpppj 표의 같은 원문 번역을 **바이트 그대로** 복사했다(줄바꿈·서식 포함, 시험이 cpppj 표와 같은지 대조). cpppj 표에 `Send %s1000` 한 줄만 있고 `300`·`700`·`X`·`All` 은 없어(cpppj 쪽 번역표의 같은 빈칸) 같은 패턴으로 직접 적었다: `%s300~[IfortGump.3] 보내기`, `%s700…`, `%s X…`, `%s 전부~[IfortGump.3] 보내기`. 서식 함수로 만든 실제 줄은 `~r300~[IfortGump.3] 보내기`(금액이 모자라면 빨강)처럼 나온다.
- 코드 수정: `ui/help-topic.js` `renderHelpTopic` 가 `translate` 장치를 받아(없으면 영어 그대로) 본문이 비었을 때 `NO_HELP_TEXT` 를 번역하고, `ui/help-window.js` 가 자신의 번역 장치를 두 호출(번역 파일·영어 대체 파일)에 넘긴다.
- `webpj/www/data/d/xlat.korean`·`manifest.json` 은 `tools/prepare-assets.js` 로 재생성(`--check` 통과).

## 보강하지 않은 것(판정)

- **멀티플레이어 접속 문구**(`Connecting to Game Server %s`·`Reconnecting …`·`: ~iPending~.` 등, `mission-loading-window.js`·`raw-mission-login-timer.js`), `Status: ~iOnline~.`(비미션 Players 메뉴), `LAN Server`: 단독 미션에서 나오지 않는다(웹 범위 밖).
- **실제 TYPE 정의의 class 문자열 19종**(`Aerial Attack`·`Defense`·`Spell` 등)은 웹 번역표에 이미 모두 있다(실제 188개 TYPE 정의를 읽어 확인). `type-flags.js` 의 `archer`·`cannon` 등은 내부 그룹 이름이라 화면에 나오지 않는다.
- **원본도 번역 호출이 없는 고정 문구**: 지식 창 머리 `~3      The Furies of the World/Storm`(`RawKnowledgeMenu.cpp:71`)과 채팅 도우미 메뉴의 색·스타일 이름(`ChatWindow.cpp:176~178`의 고정 배열)은 cpppj 도 번역 호출 없이 쓴다. 같은 구조로 두었다.
- `%d Priest%s`: cpppj 번역 `사제 %d명%s` 가 복수 접미 `%s` 를 한국어에도 붙이는 결함이 있고 원본 대본 어디에도 `{global.priests}` 가 없어 화면에 나오지 않는다(E1246 판정 유지).
- `Sky Overview`: 원본이 번역 호출 없이 리터럴을 넣는다(E1246 판정 유지). `Type`: 도움말 주제 이름 접미사(번역하면 주제를 못 찾는다, E1118). `CHEAT Set Owner`·`ClientCheated(%s)` 등 개발자·진단 문구, `english`·`french` 등 언어 코드, `Failed`(대본 구역 이름 사건).

## 시험

- `tests/korean-gameplay-xlat.test.mjs` 5건(Node): **소스 파일에서 원문 배열·상수를 직접 읽어** 번역표와 대조(커서 경고 5, 생산 힌트 2, 객체 정보 줄바꿈 포함 2, Send 5줄 + 실제 서식 함수 결과, 도움말 없음 안내의 번역 호출 유무·본문이 있을 때는 호출 안 함). 번역표와 `help-topic.js` 변경을 모두 되돌리면 5건 모두 실패함을 확인하고 복원했다.
- `e2e/korean-ingame.spec.js` 새 시험(실제 Chromium, `?lang=ko`): 브라우저가 받은 번역표(`www/data/d/xlat.korean` + manifest)에서 15개 키가 기대한 한국어로 번역됨.
- 관련 Node 103건(help·korean·quick-help·external-link·port-map·fixture-manifest·raw-player-menu·raw-cursor-lifecycle) 통과, 도움말 e2e(`main-menu-help-type`·`mission-object-commands` 도움말 5건)와 한국어 e2e 모두 통과.

## 남은 일

- 번역 문구 자체의 어색함은 개발자 검수 범위이다. 위 15개 중 cpppj 표에 없던 Send 네 줄은 이 프로젝트가 새로 적은 번역이다.
- 미션 화면에서 이 문구들이 실제로 보이는 장면(커서 경고·생산 힌트 말풍선·객체 정보·Players 메뉴 Send)의 한국어 화면 확인은 하지 않았다. 번역 호출과 서식 결합(서식 함수 결과)까지만 시험했다.
- cpppj 쪽 번역표의 `Send %s300/700/ X/ All` 빈칸은 cpppj 작업(다른 세션)에서 채우도록 남긴다.
