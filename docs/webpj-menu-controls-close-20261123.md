# RawMenuControls partial → port 판정 (WB04-E1252)

대응표 `cpppj/src/client/RawMenuControls.cpp` 행(1,605줄)에 남아 있던 차이(E1230 감사의 ③ Tell 모달 스택, ⑥ 제목 사건 4개·메뉴 request/closed 사건 미설정, 그리고 "미확인" 둘)를 도달성과 실제 동작으로 판정해 행을 `partial` 에서 `port` 로 닫았다. ①②④⑤ 는 E1235~E1240 에서 이미 해소됐다. 코드 변경은 없다(e2e 한 건 추가).

## ③ Tell 모달 스택 없음 → 결과가 같다

원본은 Gump 창을 `SetModal`/`FindModalLowest` 모달 스택에 쌓아 여러 모달이 동시에 존재할 수 있다. 웹은 메인 메뉴의 대화상자 하나(`mainMenu.dialog`)만 둔다. 두 창이 동시에 필요한 경로가 있는지 찾았다.

- 대본이 여는 Tell(미션 안내·결과 안내)은 모두 프레임 관문(`core/raw-mission-frame-gate.js`, 원문 `004c...` 의 같은 순서)을 거친다. `tellStage` 는 `dialogsClear()`(열린 창이 없음)일 때만 진행하고 창이 열려 있으면 잠금 프레임(`lockFrames`)을 되돌린다. `tellAt`·일시정지 해제도 `dialogsClear()` 를 요구한다. 웹의 `dialogsClear` 는 `!mainMenu.dialog`, 원본은 모달 스택이 빈 것으로 같은 의미이다.
- 사용자가 여는 창(ESC 메뉴의 Leave Mission → `ABORT` 확인 창, F1 도움말 등)은 열린 모달이 있으면 모달 입력이 막고(F1 은 E1235 에서 모달 관문 추가), 한 Tell 의 단추가 다음 Tell 을 여는 경우는 단추가 먼저 창을 닫은 뒤 여는 순서이다(쌓이지 않는다).
- **실제 확인**(`e2e/mission-priest-training-flow.spec.js` 새 시험, 실제 Chromium): ABORT 확인 창이 열려 있어도 게임 시계는 흐른다(1.5초에 1.6초 진행). 그 사이 사제를 죽여 실패를 판정시켜도 열린 창은 ABORT 하나 그대로이고(결과 안내가 덮어쓰지 않음), 확인 창을 닫는(`Continue Mission`) 순간 실패 안내(`Training Menu`·`Restart`·`Leave Missions`)가 이어서 열린다. 원본의 스택 순서(ABORT 아래, 결과 안내는 관문이 미룸)와 같은 관찰 결과이다.
- 송금·서식 입력 창이 모달인 웹 구현(E1168 등)은 원본이 이 창들을 모달로 두는 것과 같다.

따라서 단독 미션에서 스택이 필요한 순간이 없다.

## ⑥ 제목 사건 4개·메뉴 request/closed 사건 미설정 → 영향 없음

| 값 | 원본 초기값 | 웹 | 판정 |
| --- | --- | --- | --- |
| `titleState.refreshEvent` | `0x0200001b`(ChangeFontFace) | 0 | 웹 옵션 메뉴에 Font 항목이 없고 앱이 `ChangeFontFace` 를 연결하지 않아(`core/presentation-command-input.js` 는 모듈만 있음) 제목 위젯이 이 사건을 받는 경로가 없다 |
| `titleState.cursorEvent` | `0x0200003b` | 0 | 위젯은 `code !== cursorEvent` 일 때만 좌·우 단추 처리를 하는데 커서 사건은 단추 코드가 아니라 어느 쪽이든 처리되지 않는다 |
| `titleState.leftTagEvent`·`rightTagEvent` | `0x02000025`·`0x02000026` | 0 | 태그 플래그(`LEFT_TAG`·`RIGHT_TAG`)는 본문에 `~[T…]` 태그 조각이 있을 때만 서는데 웹의 제목 위젯 본문(메뉴 제목·도우미 제목 등)에는 태그가 없다 |
| `inputState.requestEvent` | `0x0200000d` | 0 | 동적 자식 항목(`DYNAMIC`)을 가진 메뉴를 쓰는 곳이 웹에 없다(`menu-gump-draw.js` 의 상수 정의뿐). 사건이 보내질 일이 없다 |
| `inputState.closedEvent` | `0x020000be`(MenuJustClosed) | 0 | 세로 메뉴(`DELETE_COMMAND` 0x200000 포함, `beginVertical` 의 0x280000)에서 명령 항목을 고르면 메뉴를 죽이고 이 사건을 부모에 올린다. 원본에서 이 사건을 받는 `UberGump.cpp:1343` 은 `RequestClosePopups` 와 UI 재구성 표시(`rebuild_`)를 하는데, 웹은 같은 선택 처리의 끝(`menu-gump-input.js` `if (flags & COMMAND) this.closePopups()`)이 팝업을 모두 닫고 재구성 표시가 필요 없다. 대화 단계의 `Tell("Blank")` 는 미션 중 이 메뉴에서는 없다 |

값을 원본 초기값으로 채우면 부모 Gump 가 받는 사건이 늘어 동작이 바뀔 위험만 있고 효과가 없어서 채우지 않았다. 나중에 Font 옵션·태그가 있는 제목·동적 자식 메뉴가 웹에 들어오면 `ClientMenuGump` 의 `inputState`(`parts.inputState`)와 제목 위젯 상태에 같은 값을 넣는다.

## 미확인 두 가지

- `EnableBackgroundGifs`(`raw-background-gif-controls.js` 미결선): 웹 메인 메뉴는 같은 배경을 `main-menu.js` 가 직접 그리며 M1 8장면 픽셀 차이가 0이다(E1243 에서 같은 근거로 RawUberRootControls 배경 GIF 자식을 웹 범위 밖으로 판정). 미션의 메시지 배경은 웹 HUD 가 title 을 0 으로 읽어 쓰지 않는다.
- 큰 Tell 의 둘째 크기 한계(원문 전체 화면 대 웹 지도 영역): 미션 중 안내 창을 왼쪽 HUD 를 뺀 지도 영역 가운데에 놓는 것은 원본 스크린샷(Briefing·Success 창)을 근거로 한 E1130 결정이다.

## 시험

- 새 e2e 1건(위 ③ 실제 확인). 기존 Death Bridge e2e 와 함께 통과.
- 대응표·manifest 시험(`tests/port-map.test.mjs`·`tests/fixture-manifest.test.mjs`)으로 행 형식과 상태 규칙을 확인했다.
- 전체 회귀: Node 22.23.3 전체 6,790건 모두 통과(실패 0, 552초, 로그 `extracted/webpj-e1252-node22.log`).

## 남은 일

- 이 판정은 소스와 웹 실제 동작 확인에 근거하며 원본 실행으로 스택 동작을 확인하지는 않았다.
