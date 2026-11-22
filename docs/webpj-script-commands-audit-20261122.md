# 대본 스크립트 명령 점검과 연결 누락 수정 (WB04-E1190)

영어 대본 82개(`\D\*.english`·`*.html`, 전역 `tell.english` 포함)가 쓰는 스크립트 명령(`<$이름,인자>`·`$Button/$Menu/$Timeout/$OnExit=…,이름,인자`)을 모두 모아 웹 앱(`_runScriptCommand`·`_runDialogCommand`)이 아는 것과 대조했다. 앞서 Fury War 의 `CheatAllTechnology`([E1189](webpj-fury-war-technology-20261122.md))를 이렇게 찾았고, 같은 점검으로 두 곳을 더 고쳤다.

## 이번에 고친 것

| 명령 | 쓰는 곳 | 증상 | 수정 |
| --- | --- | --- | --- |
| `<$ViewSpot,(139x88)>` | `runforit.english` [Init] | Run For It! 시작 카메라가 (139,88)이 아닌 다른 곳에서 시작(그 칸이 화면 밖으로 약 800픽셀) | `_runScriptCommand` 가 좌표를 맡겨 두었다가(Init 은 월드 생성 전에 열림) 미션 시작(`gate.started===1`) 뒤 첫 입력 프레임에 `world.centerCamera`(TAB·U 키와 같은 길)로 옮긴다. 시작 전에 옮기면 이어지는 홈 카메라 초기화가 덮어쓴다 |
| `Salvage,<sid>` 단추 | `tell.english` [SalvageVortex]·[SalvageInf] 의 `Salvage It` | 신전(flags2 0x200)·영향 건물(0x4000) 회수 경고 창에서 `Salvage It` 을 눌러도 아무 일 없음(대화창 명령 `salvage` 미연결) | `_queueDialogPlayerCommand` 가 `salvage` 를 회수 입력 번호(0x0200005d)로 입력 큐에 올려 `requestSalvage(sid)` 로 잇는다 |

시험: `e2e/view-spot.spec.js`(시작 직후 (139,88)이 지도 가운데 근처), `e2e/salvage-warning.spec.js`(경고 창 단추가 `requestSalvage(321)` 요청, 취소는 요청 없음). 두 수정 전에는 각각 화면 밖 좌표·요청 없음이었다.

## 점검 결과: 남은 미연결 명령

| 구분 | 명령 | 사용처 | 판정 |
| --- | --- | --- | --- |
| 단독 미션에 닿는 것 | `Config`·`Warning`·`Tell`·`TellTip`·`MissionBegin`·`MissionAbort`·`MissionRestart`·`LeaveBattle`·`StartDemo`·`DoNothing`·`ShowTechnology`·`SendAllBucks`·`ConfirmAlliance`·`CheatAllTechnology`·`ViewSpot`·`Salvage` | 미션·메뉴 Tell | 모두 연결(`ShowTechnology` 는 지식 창 명령으로 연결, E1088) |
| 개발·시험 전용 | `DumpSquids`(weptest.english 60회)·`Capture`(동영상 캡처 시작)·`adminsetzone`·`adminsetTag`·`adminbcast`·`adminmcast`·`ReloadTheams`·`RefreshPlayers`·`SetOwner`·`NewFort` | 개발자 도구·서버 관리·편집기 | 웹 범위 밖 |
| 멀티플레이·메타·계정 | `MenuJustClosed`·`MoveDialog`·`GoMultiplayer`·`LaunchRootServer`·`RequestMenu`·`ChalForceJoinBattle`·`ChalSailJoinBattle`·`ChalSailPos`·`ChalJoinBattleNoWarning`·`DecendToChalLevel`·`AccountSetup`·`RecheckCD`·`LeaveCheater`·`SaveGoMain`·`RequestStormpower` | 길드·존·전투 종료·서버 갱신 | 웹 범위 밖(멀티플레이 UI) |
| 플랫폼 | `QuitApp`·`ScreenModeChange`·`AcceptScreenModeChange`·`ScreenModeAbsChange`·`URL` | 종료·해상도·바깥 링크 | 웹 제외 또는 별도 처리(바깥 링크는 새 탭) |

점검 도구는 scratchpad 의 `cmds.mjs`(대본 전체에서 정규식으로 명령 이름 집계)이며 `GameData.findFiles('\D\*.english')`·`readText` 로 보호 TARC 안의 대본을 읽는다.

## 남은 점

- `MoveDialog,br`(Battle Over/VICTORY! 창을 오른쪽 아래로)는 `GoMultiplayer` 단추와 함께 쓰는 멀티플레이 전투 종료 창이다.
- AI 사제 희생 안내(`ai?priestdead`)가 실제로 뜨는 자연 플레이 흐름은 확인하지 않았다(E1189와 같은 명령 경로).
