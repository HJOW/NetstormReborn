# 미션 프레임에 죽은 Gump 정리 단계 연결 (WB04-E1134)

몽키 시험(`NETSTORM_MONKEY_MISSION`, 후반 미션 선택 옵션을 이번에 추가)을 Complete Victory 의 Vicious 에 200초 돌리다가 약 196초에 `RangeError: Gump 원본 개수 한도 초과` 로 게임이 멈추는 결함을 찾았다.

## 증상과 원인

- 스택은 `WarningMessages.gameTimeStatus → MessageBoard.show → … → GumpCreate.construct` 이다. 등록 Gump 수가 원본 한도(999)에 닿아 새 안내를 만들지 못한 것이다.
- 등록 Gump 수 변화를 조작 종류별로 측정하니 손을 대지 않아도 10초에 80개 넘게 늘었고, 등록표의 192~226개가 `WarningNode`(안내, 종류 16 Game Time)였다. Kill 큐 길이도 400을 넘었다.
- 사용자 경로: **T 키로 게임 시간 표시를 켜면** 원본(`00439b43..64`)처럼 매 프레임 0.1초 기한의 Game Time 안내를 요청한다. 기한이 지난 안내는 Idle 이 Kill 큐에 넣지만, 큐를 비워 등록표에서 빼는 단계가 웹 미션 프레임에 없었다. 켠 채 2~3분 두면 한도에 닿았다.
- 원본 메인 루프(`ClientMain.cpp` 608, 원본 `439b13 → 4662b0`)는 Gump Idle(`464170`) 바로 앞에서 죽은 Gump 정리(`CleanupGumps`)를 **매 프레임 한 번** 한다. 웹 `main-loop.js` 단계 표에는 `cleanupGumps` 가 적혀 있었지만 `netstorm.js` 의 미션 프레임이 호출하지 않았다. 다른 정리(창 닫기·메뉴)는 각자 `cleanup()` 을 불러 평소에는 가려져 있었다.

## 수정

| 위치 | 내용 |
| --- | --- |
| `www/netstorm.js` `_frame` | 미션 표시 중 `_clientGump.idle()` 바로 앞에서 `_clientGump.lifetime().cleanup()` 을 매 프레임 호출한다. |
| `e2e/monkey.spec.js` | `NETSTORM_MONKEY_MISSION='묶음:미션,…'` 로 몽키를 돌릴 미션을 고른다(정하면 그 미션들만 돌고 모든 미션 진행 저장이 켜진다). 정하지 않으면 기존 3조합이다. |

## 시험

- `e2e/mission-gump-cleanup.spec.js`(Chromium): The War Begins! 에서 T 키를 켜고 3초·12초 시점의 안내 Gump 수·등록 Gump 수·Kill 큐 길이를 비교한다. 수정 뒤에는 평형(십여 개)에 머물러 증가가 4 이하이고, 끈 뒤에는 안내가 0·Kill 큐가 0이다. 수정 줄을 빼는 변형에서는 12초 동안 안내 Gump 가 35개 늘어 실패한다.
- 같은 몽키 조작열(120초)의 등록 Gump 수는 수정 전 416, 수정 뒤 14 였다.
- netstorm.js 를 가져오는 Node 시험 27개 파일(311건)이 그대로 통과했다.

## 함께 고친 오래된 e2e 기대값

전체 e2e(`npx playwright test`, 91 통과·8 건너뜀)를 한 번 돌리자 이번 변경과 무관한(수정 줄을 빼도 같은 결과) 실패 둘이 있어 현재 동작에 맞췄다. e2e 는 평소 전체를 돌리지 않아 앞선 단위에서 못 잡았다.

| 시험 | 원인 | 조치 |
| --- | --- | --- |
| `e2e/client-clock.spec.js` | `20261120-46` 이 소유자 표식 기본값을 0으로 바꾸면서(PE 초기값 1) 시작 때 소유자 표식 옵션 적용이 재그리기 값을 한 번 더 올려 3이 됨 | 기대값 2 → 3, 이유 주석 |
| `e2e/first-run.spec.js` | 설정 스냅샷에 스커미시 설정(`skirmish*` 16개 키 기본값)이 항상 포함됨 | 비교에서 `skirmish*` 키 제외 |

## 남은 것

- 원본 `CleanupGumps` 는 정리 뒤 HUD 소유자 정리(`RetireHudOwners`)와 다시 만들기 표시도 같이 한다. 웹은 `_campaignProduction.hud.refresh()` 가 이를 맡고 있어 이번에는 Gump 수명 정리만 넣었다. HUD 쪽 대응 확인은 별도 점검 대상이다.
- 후반 미션 몽키 시험은 Vicious 외 FinalConfrontation·Surrounded·bc9ssun_bars76 도 돌려 확인한다(LEFT_JOBS 0.1 절 참고).
