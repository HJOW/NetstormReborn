# 앱 Tutorial Init과 Fort 요청 연결 (WB04-E886, 2026-11-19)

앱이 소유한 `ClientCampaignGuide`에 Tutorial 초기화를 연결했다. 원본 `UberGump::InitializeMission`의 순서대로 같은 Client의 Template manager를 비우고 전투 옵션을 초기화하며 미션 설정·Subscriber를 갱신한다. Fort 선택은 실제 현재 이름과 창 제목을 거쳐 State7 블록 및 Zone 이미지를 작성하고 지연 큐에 예약한다. 기술 제한은 Session이 빌리는 같은 배열에 적용한다. Session 공유 서비스는 이 요청과 같은 전투 옵션을 전달한다.

검사에서는 실제 Tutorial1 Script와 BridgeTheGap Fort를 읽어 Init의 번호·플래그·State7·Zone·큐를 확인하고, 그 요청을 Session에 넘겨 로컬 로그인과 월드 적재까지 진행했다. Chromium에서는 앱의 Guide 초기화, 이름·제목·지연 요청 생성 및 해제를 확인했다. Tutorial Init, Fort 요청, 기술 제한, Guide 교체, 오류 보고와 기존 메뉴 오라클의 연관 검사 38건 및 실제 Guide 집중 검사 5건이 통과했다.

이 검사는 테스트가 지연 큐를 직접 꺼내 Session을 만드는 경로다. 앱의 메뉴 명령과 프레임 루프에서 지연 요청을 소비하고 월드·화면을 연결하는 작업은 남았다. 실제 Tutorial1 L2 장면 및 자연 플레이의 동일성도 아직 확인하지 않았다.
