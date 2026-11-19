# 실제 Client의 Guide 해제 (WB04-E882)

기존 `CampaignGuideRelease`는 덕타입 검사에서 금지값을 일반 속성으로 사용했지만, 실제 `ClientGameplayGlobals`는 같은 원본 필드를 참조 상자를 돌려주는 메서드로 공개한다. 실제 Client에 연결하면 Tutorial 해제에서 읽기 전용 메서드에 대입해 예외가 발생했다. 현재 메서드가 돌려주는 상자의 값을 읽고 쓰도록 고쳤으며 기존 값형 독립 검사 계약도 유지한다.

실제 앱 공통 Client와 Session을 사용해 Tutorial Guide 해제 후 회수·승급 금지값, 미션 설정·번호·토큰과 AI 반납을 확인하고 같은 World와 Session은 유지되는 것을 검증했다. World가 없는 실제 Client에서도 Player 기록을 확인하고 지연 Fort 요청·전역을 정리한다. 원본 Guide 명령을 실제 앱 메뉴·프레임에 연결하는 작업과 전체 State 종료는 남았다.

Node v26 집중·연관 검사 49건이 통과했다. 최초 실제 Client 검사는 읽기 전용 메서드 대입 예외를 재현했고 수정 뒤 통과했다. 작업 단위 주기에 따라 이번에는 전체 회귀를 실행하지 않았다. 게임 창은 실행하지 않았다.
