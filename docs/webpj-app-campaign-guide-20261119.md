# 앱의 캠페인 Guide 수명 (WB04-E883)

앱 자산과 Client 장기 저장소가 준비되면 `ClientCampaignGuide`가 같은 설정·Gameplay·Player·월드 미션 모드와 미션 제한 표를 보유한다. 기존 `beginGuideMission`·`closeGuideMission`과 실제 Script/Runtime 생성·Tutorial 종료·E882 해제를 결합해 기존 미션 교체와 앱 종료의 반납 순서를 보존한다. 없는 스크립트 또는 지원하지 않는 유형은 실패 시 등록 설정을 정리한다. Session에는 같은 256칸 제한 표를 빌려준다.

실제 앱 Client의 Tutorial1 생성·Tutorial2 교체에서 같은 Session을 유지하며 제한 표의 허용 변경이 생산 조회에 반영되는 것을 검사했다. 없는 미션 뒤 정상 미션을 다시 만들고, Chromium에서도 실제 공개 자산의 Guide 생성·반납을 확인했다. 이 단계는 메뉴의 `MissionBegin`/`StartDemo`, 원본 Guide 단계 실행, 로그인 및 실제 게임 화면을 아직 연결하지 않았다.

Node v26 집중·연관 검사 74건과 Chromium 2건이 통과했다. 게임 창은 실행하지 않았다.

정해진 주기에 따라 Node v22.23.3 전체 회귀 6,429건이 실패·취소·건너뜀 없이 통과했다.
