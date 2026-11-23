# webpj 입력 큐 명령 코드와 입력 루프 허용 목록 전수 대조 (WB04-E1267, 2026-11-23)

[LEFT_JOBS 점검](webpj-left-jobs-audit-20261123.md)(E1263)이 4D-3e 에 남긴 일이다. 입력 루프(`netstorm.js` `_runCampaignInputFrame`)는 큐 머리의 명령이 처리 목록(`unsupported` 판정식)에 없으면 앞 사건을 추월하지 않고 멈췄다가 3초(벽 시각) 뒤 `미구현 입력 사건을 버립니다` 오류를 남기고 버린다. 지금까지 E1083·E1150·E1225·E1227·E1231 에서 이런 빈칸을 하나씩 발견해 고쳤으므로, 큐에 올리는 곳을 한 번에 모두 세어 목록과 대조했다. 코드는 바꾸지 않았다.

## 방법

`www` 전체에서 입력 큐(`pushCommand`·`push`)에 명령을 올리는 곳을 모두 찾아(`grep`) 올리는 코드를 확인하고, 각 코드가 허용 목록(전용 비교·`TUTORIAL_NOTICE_COMMANDS`·`isGameMenuCommand`·선택/생산/카메라/회전 키 판정)에 드는지 `isGameMenuCommand` 를 직접 호출해 확인했다.

## 발행처 목록과 판정

| 발행처 | 올리는 코드 | 허용 |
| --- | --- | --- |
| `campaign-user-input.js`(`#queue`) | 0x02000044(선택한 목적지) | `WORLD_DESTINATION_SELECTED_COMMAND` 전용 비교 |
| `content-form-lifetime.js` 수입 알림(`localNotice`) | 0x020000b4(내 너겟 납품) | `TUTORIAL_NOTICE_COMMANDS` |
| `mission-guide-input.js` | 0x02000091(미션 첫 단계), 0x020000c5(전투 떠나기), 0x0200008d(미션 재생성, `netstorm.js` 의 `endDemo`) | `isGameMenuCommand` 참 |
| `game-menu-commands.js` ESC·Pause 키 | 0x020000f0·0x020000cf | `isGameMenuCommand` 참(Pause 는 `PAUSE_KEY_CODE` 분기도 있음) |
| 생산 단축키(`production-shortcuts.js`) | 0x020000f4(다리)·0x02000107(반복) | `PRODUCTION_BRIDGE_COMMAND`·`PRODUCTION_REPEAT_COMMAND` 전용 비교 |
| 대화상자 단추(`_queueDialogPlayerCommand`) | 송금·동맹·회수(0x0200005d `SALVAGE_COMMAND`)·지식(ShowTechnology·GetTechnology 등) | 각각 `isSendMoneyCommand`·`isGameMenuCommand`·전용 비교·`MISSION_F6` 계열 |
| 대화상자 명령 수명(`dialog-command-lifetime.js`·`dialog-exit-command.js`)·스크립트 명령(`script-command.js`) | 대본 버튼 명령의 표 코드 | 대본 82개 전수 대조(E1190)와 입력 큐 소비 감사(E1231)가 확인 |
| Help Back(`onBackToPopup`) | 직전 팝업 명령(F6 지식 창 등) | `isGameMenuCommand`·`MISSION_F6` |
| `MissionAbort`(`netstorm.js` `_runDialogCommand`) | 0x020000c5 계열 | `isGameMenuCommand` 참 |
| 개발자 큐(`campaign-client-services.js` `developerQueue`) | 31h 개발자 패널 명령 | 도달하지 않음(송신원이 개발 콘솔뿐, E1242) |
| 전투 종료 메뉴(`client-battle-completion.js` `queueMainMenu`) | 전투 종료 메뉴 명령 | 도달하지 않음(`missionPost` 관문, E1238) |
| 브라우저 키·마우스(`browser-key-events.js`·`browser-mouse-events.js`) | 글자·가상 키·버튼 | 선택·생산·카메라·회전·Pause·ESC 키 판정과 좌·우 버튼 |

대조 결과: 위 발행처가 올리는 코드는 모두 허용 목록에 들거나(전용 비교·`isGameMenuCommand`·`TUTORIAL_NOTICE_COMMANDS`) 웹 단독 미션에서 도달하지 않는다. 새로 막히는 코드는 찾지 못했다.

## 남는 위험과 한계

- 허용 목록에 든다는 것은 3초 정체·폐기를 하지 않는다는 뜻이지, 각 코드의 효과가 원문과 같다는 증거는 아니다. `isGameMenuCommand` 에 든 명령이 앞쪽 분기에서 소비되지 않으면 소비 없이 조용히 버려진다(오류가 아니다). 이 부분은 대본 명령 전수 감사(E1190·E1231)와 각 명령의 e2e(ESC 메뉴·송금·동맹·회수·지식)가 따로 확인한다.
- 대본 버튼 명령의 이름→코드 표는 이번에 다시 세지 않고 E1190·E1231 감사를 근거로 삼았다. 새 대본·명령을 더할 때는 허용 목록을 함께 점검한다.
- 판정식이 `netstorm.js` 안에 인라인이라 시험으로 고정하지 못한다. 새 발행처를 더할 때 이 문서의 표를 갱신한다.
