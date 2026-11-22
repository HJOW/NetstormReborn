# Players 하위 메뉴 항목의 도달성 판정 (WB04-E1167)

게임 메뉴막대 ▸ Players ▸ (Player 이름)의 하위 메뉴(원본 Player 메뉴 `004c9700`, 웹 `core/raw-player-menu.js`)에 남은 것으로 적혀 있던 `Ignore chat`·`Whisper To`·`View Player Info`·`Send X` 중 어느 것이 단독 웹 미션에서 실제로 나오는지 실제 스커미시 화면의 항목 목록으로 확인했다.

## 소스 판정

`RawPlayerMenu.open`(cpppj `client/RawPlayerMenu.cpp`를 옮긴 `core/raw-player-menu.js` 246~278행)의 분기를 단독 미션 값(`missionPost = true`, 앱의 `0x594fa4`; 스커미시·캠페인·데모 모두 같음)으로 따라간다.

| 항목 | 만드는 조건 | 단독 미션 |
| --- | --- | --- |
| `View Player Info`(0x0200009b) | 메타맵(`meta()`)일 때만, 그 안에서 `meta() \|\| !mission()` | 메타맵이 아니므로 **나오지 않는다** |
| `Ignore chat`(0x020000ee/ef) | `!mission() \|\| 대상 == 나` 안에서 `대상 != 나` | 미션이면 `대상 == 나`인 경우에만 바깥 조건을 통과하는데 안쪽 조건이 그 경우를 막으므로 **나오지 않는다** |
| `Whisper To`(0x02000027/0x02000101) | 위와 같은 `대상 != 나` 안 | **나오지 않는다** |
| `Send X`(0x02000100) | `Send Money` 하위의 네 번째 행(미션에서 다섯 번째 All 만 Tell 0x02000095 로 바뀜) | **나온다** |

세 항목(채팅 두 개·정보 보기)은 멀티플레이어·메타맵 전용이라 웹 범위(단독 미션)에서는 도달할 수 없다. `You` 행에서는 원본 순서대로 `ignored`·`whispered` 조회가 먼저 불리지만(앱 `netstorm.js` Player 메뉴 장치가 현재 채팅 창 없음으로 0을 돌려준다) 그 값으로 항목을 만들지 않는다. 멀티플레이 UI는 웹 범위에서 제외했다(LEFT_JOBS.webpj.md 1절).

## 실제 화면 확인

`e2e/player-menu-items.spec.js`(Chromium, 영어 4:3, 스커미시 AI 3명)가 ESC ▸ Players를 열어 네 행(`You`·`Computer #1~#3`)의 하위 메뉴 노드 항목(명령 번호·인자·자식 메뉴)을 읽는다. 하위 메뉴 작성기의 라벨 기록은 Player 메뉴를 거치지 않으므로 노드의 명령 번호로 판정한다.

- 어느 행에도 0x020000ee·0x020000ef·0x02000027·0x02000101·0x0200009b 가 없다(자기 자신 포함).
- `You` 행은 `Find Priest!`(0x020000b8, 인자 1) 하나뿐이다.
- AI 세 행은 `Find Priest!`(인자 owner 2~4)와 `Send Money` 하위(자식 메뉴)를 가진다. `Send Money` 하위는 비어 있지 않은 항목이 다섯 개(300/700/1000/X/All)이고 `Send X`(0x02000100)의 인자가 각 AI 의 owner(2~4)이다.
- 콘솔·페이지 오류 0.

## 결론과 남은 일

- 3.1절 6단계 '게임 메뉴'와 0.1절 '다음 작업 1'에서 `Ignore chat`·`Whisper To`·`View Player Info`를 "웹 범위 밖(단독 미션 도달 불가)"으로 판정하고 남은 일에서 뺀다. 이 항목들은 멀티플레이 UI를 구현할 때에만 의미가 있다.
- **남은 것은 `Send X` 숫자 입력 창뿐이다.** 원본은 편집용 이름 입력과 같은 모달 대화창(`SaveFortNameDialog::ShowMoney`, 장식 틀 200×100·숫자 입력 한 줄·`OK`/`Cancel` 두 버튼)을 쓴다. 웹 앱에는 아직 미션 화면의 글자 입력창(`ClientTextEntryControls`)·모달 등록(`SetModal`)·키보드를 글자 입력창으로 보내는 경로가 없어 이 단위에서 만들 수 없었다. 같은 부품이 채팅 창 조립(`ui/chat-window.js`, 0.1절 다음 작업 3)에도 필요하므로 두 일을 한 기반 위에서 잇는다.

## 시험

- `e2e/player-menu-items.spec.js` 1건 통과(34초). 코드 변경은 없다(판정과 시험 단위).
- 기준선 뒤 cpppj 변경 없음(`RawPlayerMenu.cpp`).
