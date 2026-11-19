# 앱 Guide 단계 반복기 (WB04-E887, 2026-11-19)

`ClientCampaignGuide`에 원본 `RawMissionGuide` 반복기를 결합했다. 실제 미션 상태, Guide 지연 큐, 같은 Client의 Native 다음 State, 현재 창, 같은 미션 World의 연결 상태를 조회한다. 단계 0은 원본 First를 수행하고 다음 실행의 단계 1은 실제 Tutorial Init과 Fort 요청을 수행한다. 초기화 전 안내 기한이 있으면 실제 게임 시계로 재지연한다. 설치 후 가상 안내는 같은 Tutorial 객체를 사용하며, 아직 설치되지 않은 World Start는 도달 시 명시적으로 오류를 낸다.

실제 Tutorial1의 창 열림, Native State 대기, 같은 미션의 World 연결 대기, 단계 0·1, Fort 지연 요청 및 해제를 검사했다. 집중 검사 6건과 단계 반복·Lifecycle·Init·요청·교체 및 오류 보고 연관 검사 43건이 통과했다. Chromium에서도 앱 소유 Guide를 두 번 실행해 실제 Fort 요청이 만들어지는 것을 확인했다.

현재 테스트가 반복기를 호출하며 앱 프레임과 메뉴 명령은 호출하지 않는다. 다음 State의 실제 World 설치·연결과 단계 3 이후의 UI·Start·프레임 수명은 별도 작업이다.
