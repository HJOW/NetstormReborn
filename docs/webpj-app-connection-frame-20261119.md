# 앱 로컬 연결 프레임 (WB04-E895, 2026-11-19)

기준선 `225553df`의 `ClientMain.cpp`와 `UberGump::Tick`·`PollMissionPackets`를 대조했다. 앱 프레임의 시각 고정 뒤 같은 현재 World의 로컬 State Poll, 내부 패킷 Pump, Guide 실행을 차례로 호출한다. Poll 뒤에는 World를 다시 읽어 기존 World가 교체된 경우 이전 Session의 패킷을 진행하지 않는다. 수신 콜백 실행 중에는 해당 State와 Pump 호출을 건너뛴다.

실제 Tutorial1의 `BridgeTheGap` 요청과 앱 공통 저장소에서 State7·State8을 준비한 뒤, 테스트가 Session의 Poll이나 Pump를 직접 부르지 않고 앱 프레임만 실행해 내부 로그인 Ready에 도달했다. 매 프레임 State → 패킷 → Guide 호출 순서도 관찰했다. World 교체와 수신 중 재진입은 별도 검사에서 확인했다.

Node v26.13.0에서 실제 Session 적재·로그인 등 연관 검사 126건과 앱 시계·Client State·로컬 State 검사 23건이 실패·취소·건너뜀 없이 통과했다. 대응표 1,160행의 구조 오류는 0이다.

이 단위는 이미 설치된 World의 로컬 연결에 한정된다. 지연 Fort 큐 소비와 실제 브라우저 장치의 World 생성, 로그인 완료 후 Tutorial·입력·렌더러 설치 및 Kernel 프레임은 남아 있다. 로그인 타이머의 원본 입력 뒤 분리 호출도 아직 앱에 연결하지 않았으며 현재 Session의 `frameLoginTimer` 설정에 따른 기존 Pump 동작을 유지한다. 따라서 브라우저에서 미션을 시작하거나 플레이할 수 있다는 뜻은 아니다. 원본 게임은 실행하지 않았다.
