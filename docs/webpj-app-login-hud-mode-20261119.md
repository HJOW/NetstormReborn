# 앱 로그인 HUD 모드 전달 (WB04-E951)

기준선 `225553df`의 `BindCampaignLoginStateClientServices`는 State7·13의 UI 모드를 현재 Client `HudModeCommand`와 함께 Gump 입력 트리에 전달한다. 앱 로그인 장치에 같은 호출을 연결했다. 매 호출 시 현재 Gump와 명령을 읽고, 모드·0·숨김 포함 플래그를 `GumpInput.route`에 전달한다. 생성 시 Gump를 미리 고정하지 않으며 누락·반납된 수명은 실제 선택 위치에서 거부한다.

실제 앱 Gump의 숨긴 자식과 입력 캡처에 바뀐 명령·모드가 전달되고 캡처가 해제되는 것을 확인했다. 실제 Tutorial1의 앱 State7→State8→로그인 Ready 검사에서도 기존 가짜 UI 모드 장치를 제거하고 같은 Gump에 모드 2와 3이 도달하는 것을 확인했다. 진행 창과 HUD 전체 생성은 아직 검사 장치 또는 미연결이며 미션 플레이 검증은 아니다.

검증: 실제 Tutorial1 로그인 1건과 앱 로그인·Gump·State 서비스 연관 22건이 Node v26에서 통과했다. E950 기준 격리 전체 회귀는 별도 작업 트리에서 실행 중이다.
