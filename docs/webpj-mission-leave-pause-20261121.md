# 안내 창이 열린 채 미션을 떠나도 다음 미션의 시계가 멈추지 않게 수정 (WB04-E1132)

장시간 e2e(`NETSTORM_LONG_SECONDS`)를 후반 미션만 8분씩 돌리다가(`NETSTORM_LONG_ONLY` 새 옵션) 마지막 미션 `bc9ssun_bars76` 에서 **게임 시계가 480초 동안 한 번도 흐르지 않는** 결함을 찾았다. 세 번 모두 같은 모양으로 재현됐다.

## 증상과 원인

- 실패 메시지의 상태: `before == after`(1489.94), `paused: true`, 열린 창 없음, 입력 큐 비어 있음, 메뉴 정지 깊이 0.
- 시계 정지 호출 기록(e2e 에 임시로 `pushGamePause`/`popGamePause` 를 감싸 호출 위치를 남김)에서, 직전 미션(`FinalConfrontation`) 끝에서 `pauseMissionFrameForDialog`(미션 관문이 안내 창 때문에 거는 자기 정지, 호출자 `netstorm.js` `onSection`)의 정지 한 번이 **풀리지 않은 채** 미션을 떠난 것이 보였다. 떠나는 동안 `popGamePause` 가 한 번도 없었다.
- 미션 관문은 안내 창이 모두 닫히고 몇 프레임이 지나야(`RESUME_FRAMES`) 자기 정지를 푼다. 그 전에 `_leaveCampaignMission()` 이 Guide 를 닫으면 관문 상태(`scriptPaused`)가 버려지고, 시계(`GameClock`)의 정지 깊이만 1 로 남는다. 게임 시계 객체는 미션을 넘어 하나이므로 다음 미션은 깊이 1 로 시작하고, 새 미션이 거는 정지·재개가 짝을 이뤄도 깊이가 0 이 되지 않아 시계가 영영 멈춘다.
- 원본에서는 State 정리가 `ResumeGame` 효과로 "정지 중이면 한 단계 풀기"를 하는데(`client-state-world-cleanup.js` 의 `E.ResumeGame`), 웹의 미션 떠나기는 그 정리를 거치지 않았다.
- 사용자 경로: 안내 창이 시계를 멈춘 상태에서 그 창의 단추나 다른 입구로 미션을 떠나면 다음 미션이 멈춘 채 시작했다. 기존 e2e 는 떠나기 전에 창을 닫거나 시작 몇 초만 보아 놓쳤다.

## 수정

| 위치 | 내용 |
| --- | --- |
| `www/netstorm.js` `_leaveCampaignMission()` | Guide 를 닫기 전에 현재 미션 관문의 `scriptPaused` 가 1 이면 `popGamePause()` 로 한 단계만 풀고 플래그를 0 으로 돌린다. 재시작·MissionBegin·스커미시 시작이 모두 이 함수를 거치므로 한 곳으로 충분하다. |
| `e2e/campaign-long-run.spec.js` | `NETSTORM_LONG_ONLY`(쉼표로 구분한 미션 이름)로 미션을 골라 오래 돌릴 수 있다. 실패 메시지에 시계 정지 상태·창·입력 큐 머리와 최근 정지 호출 기록(호출 위치)을 담는다. |

## 시험

- `tests/mission-leave-pause.test.mjs`(Node 3건): 관문 정지가 있으면 한 단계만 풀고 플래그를 지우며, 없거나 Guide 가 없거나 떠날 미션이 없으면 시계를 건드리지 않는다. 수정 줄을 지우는 변형에서 첫 시험이 실패함을 확인했다.
- `e2e/mission-leave-pause.spec.js`(Chromium): Tutorial1 첫 안내 창이 열린 채 떠나면 시계 정지가 풀리고(수정 전에는 `paused: true` 가 남았다), 다시 시작해 창을 닫으면 4초 동안 게임 시각이 2초 넘게 흐른다.
- 후반 4개 미션(`FinalConfrontation`·`Vicious`·`EnemyTerritory`·`bc9ssun_bars76`)을 8분씩 이어 돌리는 장시간 e2e 의 수정 전 3회 실패와 수정 뒤 결과는 LEFT_JOBS 0.1 절에 적었다.

## 부록: 수정 뒤 중반 13개 미션 장시간 확인 (WB04-E1133)

수정 뒤 캠페인 중반 13개 미션(`TheWarBegins`·`MasterOfWhirligigs`·`SaveTheIsland`·`FragileFortune`·`ThunderingPower`·`DissolvedAlliance`·`TheNoose`·`RainVsRain`·`RunForIt`·`GuardMyBack`·`Surrounded`·`BreakingThrough`·`ToTheRescue`)을 한 페이지에서 7분(420초)씩 이어 돌렸다(`NETSTORM_LONG_SECONDS=420 NETSTORM_LONG_ONLY=…`, 1.7시간). 모든 미션에서 게임 시계가 흘렀고 페이지 오류·콘솔 오류가 없었다. 앞 미션의 정지 깊이가 남아 다음 미션이 멈추는 증상은 다시 나타나지 않았다. 코드 변경 없이 확인 결과만 기록한 단위이다.

## 남은 것

- 같은 모양의 누수 후보: 메뉴 정지(`_menuPauseDepth`)는 `_releaseCampaignPause` 가 이미 풀고, 탭 숨김 정지(`_hiddenGamePause`)는 가시성 복귀가 푼다. 그 밖의 `pushGamePause` 호출자(데모 종료 등 `onSection` 외)가 떠나기 경로에서 남는지는 정지 호출 기록 도구로 더 긴 실행에서 다시 본다.
- 장시간 e2e 는 사용자 입력이 없는 수동 진행이다. 몽키(`NETSTORM_MONKEY_SECONDS`)를 후반 미션에서 오래 돌려 보는 것은 남아 있다.
