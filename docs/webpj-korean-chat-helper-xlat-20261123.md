# 채팅 도우미 메뉴의 한국어 번역 59개 (WB04-E1250)

E1249 의 2차 후보 수집(번역 호출이 있는 웹 파일의 영어 문장 리터럴 66개)에서 가장 큰 덩어리가 채팅 창의 **Chat Helper 메뉴**(`ui/chat-window.js` `#helperMenu`, 도우미 `>` 단추)였다. 원본 `ChatWindow::HelperMenu`(cpppj `client/ChatWindow.cpp:150~`)는 이 메뉴의 라벨을 모두 `tr()`(번역 호출)로 받지만 **cpppj 한국어 번역표에도 웹 번역표에도 항목이 없어** 한국어 모드에서도 영어였다. 이 단위가 고정 라벨 59개의 한국어를 번역표에 더했다.

## 번역 규칙과 범위

- 원본이 번역 호출로 받는 고정 라벨만 더했다: 제목(`Chat Helper:`), 서식(`Edit Back/Front Format`·`Format`), 효과 토글(`Allow Chat Sound`·`Allow Background/Foreground Colors`·`Allow Embossed Offset`·`Chat Effects `), 채널·플레이어·채팅 설정 토글과 묶음 이름(`Show General Channel` … `Chat Controls`), 색·아이콘 묶음 이름(`Text Colors`·`Background Colors`·`Icons`), `KeyWords`, 스타일 여덟(`~BBold` 등)과 `Styles`, 미리 정한 색 스무 개와 `Prefabricated Colors`, `Commands`·`Insert`·`Copy`·`Paste`.
- **서식 접두를 번역문에도 유지한다**: `~BBold` → `~B굵게`, `~rRed` → `~r빨강`(`~B`·`~r` 등은 글자 서식 코드이고 뒤가 라벨 글자이다). 끝 공백도 키·번역 모두 유지한다(`Chat Effects ` → `채팅 효과 `, 번역표 파서가 보존함을 확인).
- 용어는 기존 번역표와 맞췄다: 기존 `Please type your back format.` 이 `Back 서식을 입력하세요.` 로 Back/Front 를 영어로 두므로 `Back 서식 편집`·`Front 서식 편집`, Emboss·Offset 은 AGENTS.md 의 영어 발음 표기 규칙(워크샾·제네레이터 같은)을 따라 `엠보스`·`오프셋`. 번역 문구는 개발자 검수 대상이다.
- **번역하지 않은 것(원본 동작과 같음)**: ① `Colors 0 to 21`·`BGColors …`·`Icons 1 to 23` 같은 **범위 라벨** — 원본이 `prefix + 번호 + " to " + 번호` 를 이어 붙여 `tr` 에 넘기므로 번역표 키가 될 수 없고 영어로 남는다(한국어 e2e 가 실제 요청에서 확인). ② `Mimic`·`Sound`·`Icon`·`Emboss Offset` — 원본이 `item()` 으로 번역 없이 바로 넣는다. ③ `.front`·`.back`·`.format` — 번역 호출을 거치지만 입력줄에 삽입되는 명령 이름이라 영어 유지.

## 2차 후보 수집의 나머지 판정

| 후보 | 판정 |
| --- | --- |
| `core/skirmish-setup.js` 의 영어 문구(`Number of AI players`·`Banned units` 등) | 이 모듈이 **자체 언어별 문구 묶음**(`ENGLISH`·한국어 등)을 가지고 화면을 만든다(E1101). 번역표 대상이 아니다 |
| `netstorm.js` 의 `Click to start`·`Failed to load game data.`·`Game menu (ESC)`·`Reset all settings…` 등 | 앱 셸 문구(시작 전 화면·접근성 이름·`confirm()` 안내). 원본에 없는 웹 전용이며 Reset All 확인 창 문구는 사용자 결정 대기(다음 작업 1 ⑧) |
| `core/raw-object-menu-variants.js` 의 `Set as Cracked/Normal/Hard`·`Turn To Obelisk!`/`Spell!` | 편집기(Edit) 전용 객체 메뉴(`editor` 관문)로 웹 범위 밖 |
| `core/raw-mission-login-timer.js` 의 `Creating initial geysers`·`Moveing Camera`·`Seting Priest State` | 원본 로그 호출(`hooks.log`)에 쓰는 진행 로그 문자열이며 화면 번역 대상이 아니다 |
| `Segoe UI`·`Malgun Gothic`·`Quick Help Spec` 등 | 글꼴 이름·설정 키 |

## 시험

- `tests/korean-gameplay-xlat.test.mjs` 새 시험 1건(총 6건): `#helperMenu` 본문에서 `tr`·`translatedItem`·`sub`·`toggle`·`grouped`·스타일/색 배열의 리터럴을 직접 읽어(55개 이상) 모두 번역표에 있음을 확인하고 서식 접두·끝 공백·용어 규칙을 고정한다. 번역표를 되돌리면 실패함을 확인했다.
- `e2e/korean-ingame.spec.js` 새 시험(실제 Chromium, `?lang=ko`): 한국어 미션에서 도우미 `>` 를 눌러 메뉴를 열고 **실제 번역 요청**(번역표 클래스의 `translate` 에 기록 장치를 끼워 수집)에서 대표 18개의 번역을 확인하고, 범위 라벨을 뺀 모든 고정 라벨에 영어로 남은 것이 없음을 확인한다. 스크린샷(`korean-chat-helper-menu.png`)에서 `채팅 도우미:`·`서식 >`·`채팅 효과 >`·`채팅 설정 >`·`삽입 >`·`복사`·`붙여넣기` 가 갈무리11로 그려진다.
- 번역표 사본·manifest 는 `tools/prepare-assets.js` 로 재생성(`--check` 통과).
- 전체 회귀: Node 26.13.0 전체 6,788건 모두 통과(실패 0, 394초, 로그 `extracted/webpj-e1250-node26.log`).

## 남은 일

- 서브메뉴(서식·채팅 효과·글자색·스타일 등)를 열어 펼친 항목의 화면 확인은 하지 않았다(번역 요청과 최상위 메뉴 화면까지 확인).
- 이 메뉴 아래 팝업 하단이 화면 아래로 잘려 보이는 배치(채팅 창이 화면 맨 아래라 메뉴가 위로 열리지 않음)는 영어와 같은 원본 배치로 판단했고 이번에 바꾸지 않았다.
