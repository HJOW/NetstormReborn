# Session Login 설정·메타데이터·Fort 송신 (WB04-E705)

`RawCampaignSessionLogin.inc`의 옵션 뒤 생성 순서에 맞춰 현재 Session의 Player·Template·Zone·전투 옵션과 같은 전송을 사용하는 `RawMissionServerPackets`, `RawMissionLoginPackets`, `RawMissionSettingsPacket`을 조립했다. 설정 7DWORD는 같은 장기 배열을 매번 읽고 쓴다. Player와 Fort/메타데이터 writer는 현재 mutable 송신을 선택하고 설정 writer는 읽기 전용 송신을 선택한다. Ready 진단은 관찰 장치를 켠 경우 같은 위치에서 기록한다.

실제 Tutorial 1 Fort manager에서 생성한 이미지가 0x2a 예약에 들어가는 것을 현재 이미지 writer와 대조하고, watcher·예측수·초기 선언 길이와 송신 대상을 확인했다. Player 이름 정제·0x2b/0x72 송신, 현재 설정의 송수신 및 없는 Fort의 무효과도 검사했다. 송신 경계는 관찰 장치로 확인했으며 내부 연결을 연 실제 패킷 왕복은 아니다. 원본 관측 fixture를 포함한 관련 집중 검사 23건이 통과했다.

중앙섬 생성은 현재 명시 오류이며 12차 스커미시에서 실제 구현한다. 서버/클라이언트 수신기·상태 전이·Begin/Pump·외곽 Session 연결과 행 base 이후 32커밋의 전체 판정은 미완료다. 게임 창·플레이·공중 3종 전용 행동은 실행하지 않았다.

Node v26.13.0 전체 회귀 6,144건 통과(실패·취소·건너뜀 0). E698의 Node 22 통과 이후 전체 회귀는 7/8회다. 로그는 Git 제외 `extracted/webpj-e705-node26.log`다.
