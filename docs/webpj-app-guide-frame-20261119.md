# 앱 Guide 프레임 연결 (WB04-E890, 2026-11-19)

기준선 `225553df`의 `ClientMain.cpp` 메인 루프 8번과 `UberGump::MissionFrame`은 시각 고정과 상태·패킷 처리 뒤, 입력보다 먼저 Guide를 실행한다. 현행 cpppj의 해당 호출 순서도 같다. 웹 앱은 이미 공통 게임 시각을 고정하므로 그 뒤 메뉴 포인터 갱신·그리기 전에 같은 Client Guide를 한 번 실행한다. 현재 미션이 없으면 Guide는 아무 효과가 없다. 브라우저 이벤트 자체는 비동기로 전달되며 원본 입력 큐와 전체 순서가 같다고 판정하지 않는다.

실제 Tutorial1 자산과 앱 소유 Guide를 사용해 닫히지 않은 창에서 대기, 창 닫힌 뒤 First, 다음 프레임의 BridgeTheGap Fort 요청, 지연 요청 중 단계 대기, Guide 반납을 확인했다. 집중 검사 3건과 State·Guide·오류 보고 연관 검사 21건이 통과했다. 원본 fixture를 바꾸지 않았다.

State Tick의 요청 소비, Native Poll과 로컬 패킷 처리, 실제 로그인 장치와 메뉴 명령, World Start 및 브라우저 미션 표시는 아직 연결되지 않았다. 이 단위는 실제 플레이 진입이나 L2 완료를 의미하지 않는다.
