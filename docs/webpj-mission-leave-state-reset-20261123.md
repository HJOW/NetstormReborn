# webpj 미션 이탈 때 State 정리 효과 (WB04-E1268, 2026-11-23)

[LEFT_JOBS 점검](webpj-left-jobs-audit-20261123.md)(E1263)이 4D-5b 에 남긴 "이탈 때 원문만 하는 효과 네 가지"를 확인하고, 필요한 둘을 `_releaseCampaignWorld` 에 옮겼다.

## 배경

원문 `CleanupClientStateWorld`(`ClientStateWorldCleanup.cpp`)는 미션·전투를 떠날 때 연결·정지·소리·커서·시간 동기화·채팅 채널·선택·Player 연결·Kernel·Gump 등을 차례로 정리한다. 웹 앱은 이 부모를 직접 부르지 않고(앱에 없는 장치 `interpretStateOptions`·`writeSongEndBits`·`killGumps`·`replaceUber`·`leaveMetamap`·`retireRuntime` 이 먼저 필요하다, E1223) 미션 이탈 `_leaveCampaignMission`→`_releaseCampaignWorld` 가 효과를 하나씩 옮겨 왔다(초기 캡처 기한 E1121, 건물 숨김 E1230, 정지 깊이 E1132, 장면 음악 E1119·E1258).

## 네 효과의 판정

| 효과 | 원문 | 판정 |
| --- | --- | --- |
| 채팅 채널 정리 | `Clear445140` → `ChatChannels::ResetOccupied`(점유 기록의 이름·플래그·ID 48바이트를 0 으로) | **옮김.** 이전에는 미션을 떠나도 점유 기록이 다음 미션까지 남았다 |
| 시간 동기화 세 DWORD | `Clear4c1f20` → `ResetClientClockSynchronization` | **옮김.** 이전에는 로그인 중 1 이 되는 응답 방식(`replyMode`)이 미션을 떠난 뒤에도 1 로 남았다(탐침: 시작 전 `[0,0,0]`, 미션 중 `[0,1,0]`, 이탈 뒤 `[0,1,0]`) |
| `MapRevision+1` | State 정리의 `0x5949d4` 직접 쓰기 | 영향 없음. 이 값은 Native 렌더러 초기화(`initializeNativeRenderer`, `client-camera-globals.js:69`)가 새 미션 시작 때 올리고 지역 투표가 같은 값을 비교하므로 이탈 때 올리지 않아도 매 미션 값이 달라진다 |
| 이탈 시점 선택 해제 통지(`ClearCursor` 효과) | 선택 정리 뒤 커서를 기본으로 | 영향 없음. 웹은 이탈 때 미션 UI·사용자 입력·Session 을 모두 반납하고 `_applyCursor` 로 커서를 기본으로 되돌린다 |

## 바꾼 것

- `www/netstorm.js`: `_releaseCampaignWorld` 에 `this._clientChatChannels?.resetOccupied()` 와 `resetClientClockSynchronization(this._clientLocal)` 을 더하고 `client-state-storage-cleanup.js` 에서 가져온다. 월드가 있었을 때만 실행한다.

## 검증

- 새 e2e `e2e/mission-leave-state-reset.spec.js`: 메뉴 처음 `[0 기록, [0,0,0]]` → 스커미시 시작 뒤 응답 방식 1 → 점유 기록 하나를 넣고 `_leaveCampaignMission()` → 기록 0·동기화 `[0,0,0]`·기록 48바이트 전부 0, 오류 없음. 정리 호출 두 줄을 빼 보면 실패한다(변형 확인, 원복).
- 연속 시작·이탈 관련 e2e 19건 통과(`all-missions-start` 31개 미션 연속·`game-menu-bar`·`mission-leave-pause`·`chat-window`·`skirmish-start`·새 e2e, 로그 `extracted/webpj-e1268-e2e.log`).
- 대응표: ClientState 계열 다섯 행(`ClientStateWorldGlobals`·`ClientStateWorldCleanup`·`ClientStateRuntimeRetirement`·`ClientStateStorageCleanup`·`ClientUserSelectionCleanup`)을 partial→port 로 올렸다(메모의 "영향 확인 필요(미확인)"를 위 판정으로 해소).

## 남은 것

- 원본 실행으로 확인한 것이 아니라 cpppj 복원 코드와의 대조다. 채팅 채널 정리는 실제 채팅(점유 기록이 생기는 경로)을 통한 자연 확인이 아니라 점유 기록을 직접 넣은 확인이다.
