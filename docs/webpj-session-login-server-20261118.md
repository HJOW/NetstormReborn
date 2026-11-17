# Session 서버 Login과 공유 전역 (WB04-E710)

실제 Player 참여 부모가 준비된 Session에 `RawMissionLogin` 서버를 부분 조립했다. 서버는 같은 Player9·권한·Fort 수·Core·로그인 기한과 자원 seed를 현재 시점에 사용한다. 기존 설정·Fort·메타데이터 writer와 mutable 송신, SP 지정 및 Core 복원 경계를 연결했다. 0052ed54의 0x0c 생성 헤더와 00544c24 길이 WORD도 `GameAuxState`가 장기 소유한다.

실제 Tutorial 1 자산으로 만든 Session의 Player 참여 부모에서 Begin·Ready·Reply 및 디버그 burst의 예약/현재 seed/송신 경계를 확인했다. 비미션 Core 복원에서 JS 불리언을 원문의 0/1로 전달하는 관문도 확인했다. 송신은 관찰 장치로 확인했으며 내부 연결의 실제 패킷 왕복은 아니다. 관련 집중 검사 52건이 통과했다.

서버 객체는 실제 참여 부모가 없거나 중복 생성이면 거부한다. 클라이언트 옵션 수신기와 현재 Session Load 설치가 없으면 해당 선택 위치에서 중단한다. 원격 Fort는 일반 멀티플레이 제외, 원격 동기화는 4D-3d, 전투섬·비미션 재접속 정리는 후속 스커미시/저장 단계다. 서버 수신기 등록·클라이언트 수신기·Begin/Pump·실제 내부 왕복·외곽 Session 연결과 기준선 뒤 32커밋 전체 판정은 미완료다. 게임 창·실제 플레이·사제 포획/희생·자연 승리·공중 3종 전용 행동은 실행하지 않았다.

Node v26.13.0 전체 회귀 6,159건 통과(실패·취소·건너뜀 0). E706의 Node 22 통과 이후 전체 회귀는 4/8회다. 로그는 Git 제외 `extracted/webpj-e710-node26.log`다.
