# Session Login의 공유 시계 동기화 (WB04-E706)

`RawCampaignSessionLogin.inc`의 시계 생성자 연결을 현재 `ClientLocalStateGlobals`의 같은 시계 전역과 70개 표본에 이식했다. 별도의 시계 사본을 만들지 않는다. 송신과 응답은 현재 Player 번호로 다시 선택하지 않고 연결 등록소가 보유한 고정 토큰의 가상 송신을 사용한다. 응답 주소가 같은 연결의 현재 주소와 달라지면 원본처럼 거부한다.

실제 Session owner·연결 등록소·패킷 분배기에서 서버 초기화, 표본 보존, 시간 응답, Commander 표본 송신과 주소 변경 거부를 확인했다. 기존 원본 계약과 Poll 검사까지 집중 68건이 통과했다. 이 부품을 만드는 것만으로 시계 수신기를 등록하거나 로그인 접속을 시작하지 않는다. `Begin`의 서버 초기화, `StateHooks`와 Pump 결선, 실제 로그인 왕복 및 고정 Native 수신기는 후속이다. 게임 창·플레이·공중 3종 전용 행동은 실행하지 않았다.

Node v26.13.0 전체 회귀와 그 종료 뒤 순차 실행한 Node v22.23.3 전체 회귀가 각각 6,146건 통과했다(실패·취소·건너뜀 0). 다음 Node 22 검사까지 완료된 전체 회귀는 0/8회다. 로그는 Git 제외 `extracted/webpj-e706-node26.log`와 `extracted/webpj-e706-node22.log`다.
