# 미션 나가기·재시작과 미션 사이 전환 (WB04-E1085)

게임 메뉴의 Leave Mission·Restart Mission과 ABORT 확인 창·미션 완료 창의 단추(`LeaveBattle`·`MissionAbort`·`MissionRestart`)를 실제 단독 미션 수명에 연결했다. Leave Mission은 원본 `00463e40`의 미션 중단 분기대로 인자 0이면 ABORT 확인 창(메인 메뉴/미션 다시 시작/미션 계속)을 열고, 확인하면 Guide 미션을 닫고 HUD·월드·Session을 반납한 뒤 메인 메뉴 표면을 다시 그린다. Restart Mission(Reload)은 마지막으로 시작한 미션의 이름·모드로 같은 절차를 거쳐 새 월드로 다시 시작한다. 메인 메뉴에서 다른 미션을 시작하는 것도 같은 수명으로 동작해, 현재 미션이 있으면 먼저 떠난 뒤 시작한다(이전의 "현재 미션을 정리하기 전에는 새 미션을 시작할 수 없다" 거부를 대체).

## 구현

- `_leaveCampaignMission()`: 팝업 닫기 → `ClientGuide.close()` → `_releaseCampaignWorld()`(메뉴 버튼·HUD·월드 반납) → 열린 대화상자 정리 → `_ensureSurface()`·메인 메뉴 `dirty`·화면 갱신. 가이드를 닫고 월드를 반납하는 것만으로는 메인 메뉴 표면이 다시 그려지지 않던 문제를 표면 재준비로 해결했다.
- `_restartCampaignMission()`: `_beginCampaignFromDialog`가 기록한 `_lastCampaignMission`(이름·demo·mode)으로 떠난 뒤 `ClientGuide.begin`을 다시 호출한다.
- 메뉴 명령 소비: 중단 0x8c(`dispatchMissionAbort`), 재시작 0x8d, 전투 떠나기 0xc5를 연결하고, 월드를 반납하는 명령은 입력 반복을 끝낸다.
- HUD가 사라진 뒤 현재 루트로 남는 빈 Uber 루트는 다음 설치 전까지 HUD 모드 사건을 도킹하지 않고 기본 처리로 돌려보내며 지도 영역을 전체 화면으로 되돌린다. 이 처리가 없으면 두 번째 미션의 로딩 HUD 모드 사건이 지워진 자식 토큰을 읽어 `Gump 트리 토큰/저장소 없음` 오류가 났다.

## 검증

`e2e/game-menu-bar.spec.js`의 세 번째 검사가 Tutorial1에서 Leave Mission → ABORT 창의 단추 세 개 → 미션 계속(미션 유지) → 다시 Leave → 메인 메뉴(월드·Guide·메뉴 버튼 없음) → 같은 페이지에서 Tutorial2 시작 → Restart Mission(새 월드·같은 미션 이름)까지 페이지 오류 없이 확인한다. `client-uber-hud` 단위 시험에 빈 루트의 HUD 모드 사건 무시와 지도 영역 복귀를 더했고 변형 확인으로 잡힌다.

Node 단위 시험(`client-uber-hud` 4건·`raw-game-menus` 5건·`campaign-request-frame` 4건·`port-map` 2건)이 통과했다. `campaign-request-frame`은 예전 "현재 미션이 있으면 새 미션을 거부한다" 단언을 "먼저 떠나고 새 미션을 시작한다"로 바꿨다. Chromium 8개 파일 33건 중 32건이 통과했고 병렬 실행에서 한 번 실패한 한국어 Tutorial1(생산 항목 우클릭 메뉴가 열리지 않음)은 단독 재실행과 `--repeat-each=5` 두 시험 10회에서 모두 통과해 부하에 따른 타이밍 흔들림으로 판단했다(E1084의 첫 Tutorial 시험과 같은 계열이며 반복되면 조사한다).

## 남은 경계

세이브 파일·진행 상황 저장 시점, 미션 음악과 장면 전환 효과, 데모 모드(`Exit Demo`·demo 인자 보존)의 실제 시작 경로, 완료 창에서 다음 미션으로 이어지는 실제 완주는 확인하지 않았다.
