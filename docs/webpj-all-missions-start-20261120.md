# 모든 캠페인 미션 연속 시작 확인과 캡처 기한 정리 (WB04-E1121)

캠페인 목록의 미션을 한 페이지 안에서 연달아 시작·이탈하며 오류를 모으는 조사에서, **두 번째 미션부터** 콘솔에 `미션 AI 원본 진단: !earlyCaptureTimer.isRunning()` 오류가 났다. 원인과 수정, 그리고 이를 지키는 새 Chromium 시험을 기록한다.

## 원인과 수정

- 미션 Zone 설치(`installLocalMissionZone`)는 초기 캡처 기한(`005c8618`, 로그인 대기 + 120초)을 쓰고, 이미 0 이 아니면 `!earlyCaptureTimer.isRunning()` 진단을 낸다(원본 `LocalMissionTransport.cpp:86`).
- 원본은 월드 정리 어댑터(`UberGump.cpp:540`, 원문 004b2df0)에서 Session·UI·Kernel 을 반납하기 **전에** 이 기한을 0 으로 쓴다. 웹의 `_releaseCampaignWorld()` 는 이 쓰기가 없어, 미션을 떠난 뒤 기한이 지나기 전(약 2분)에 다른 미션을 시작하면 진단이 났다. 월드가 있을 때 `captureEnd` 를 0 으로 쓰도록 고쳤다(월드가 없으면 건드리지 않는다).
- 진단은 오류 로그일 뿐 시작을 막지는 않았으나, 첫 미션 뒤 모든 미션 시작에서 콘솔 오류가 나던 것이 사라졌다.

## 시험

- `tests/netstorm-gameplay-services.test.mjs` 의 새 시험이 월드 반납 뒤 기한이 0 이 되고, 월드가 없을 때는 값을 그대로 둠을 확인한다.
- 새 `e2e/all-missions-start.spec.js`(Chromium)는 Early Missions 6 + Struggle For Freedom 6 + A Nation Rises 5 + Complete Victory 5 + Priest Training 9 = 31개 미션을 목록에서 하나씩 시작하고(진행 저장은 모두 완료로 열어 잠금을 푼다), 브리핑·안내 창을 마지막 단추로 닫은 뒤 게임 시계가 정지 없이 3초 넘게 흐르는지와 페이지 오류 0 을 확인한다. 미션 사이에는 `_leaveCampaignMission()` 으로 메인 메뉴로 돌아간다. 31개 모두 통과했고 약 13분 걸린다(수동 실행용 회귀).
  - 데모형 훈련 미션(bc1menu76 등)은 시작 몇 초 뒤 `NetStorm Demo` 안내 창이 떠서 시계를 멈추므로, 창이 없는 상태가 3초 이어질 때까지 계속 닫도록 했다.
- 수정 전 같은 조사에서 Tutorial1 이후 모든 미션이 오류 1건씩을 냈고, 수정 뒤 31개 모두 0건이다.

## 남은 경계

- 이 시험은 시작·초반 진행(몇 초)만 확인한다. 미션 목표 수행·승리·패배·자연 완주와 AI의 장시간 행동은 사용자 확인 범위이거나 별도 단위다.
- 게임 시계(`gameSeconds`)가 미션 사이에 이어지는(미션마다 0으로 돌아가지 않는) 점은 이번 조사에서 관찰만 했다. 원본의 미션 시계 초기화와 게임 시간 표시(T 키)의 기준 시각은 별도 확인이 필요하다.
