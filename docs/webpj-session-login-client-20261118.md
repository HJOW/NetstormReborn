# Session 클라이언트 Login 수신 몸체 (WB04-E711)

실제 서버 Login과 Player 참여 소유자가 준비된 뒤 `RawMissionClientPackets`를 같은 Player·SP·옵션·Core·Zone·상태 블록·시계에 부분 결선했다. 서버의 옵션 적용은 같은 클라이언트 수신기를 호출하고, 클라이언트가 받은 시작 seed는 서버가 쓰는 같은 자원 DWORD에 반영된다. 최상위 비트가 켜진 seed는 수신기의 signed 변환 뒤에도 unsigned 비트 그대로 저장하도록 교정했다.

실제 Tutorial 1 자산으로 구성한 Session owner에서 서버 선행/중복 관문, 0x29의 Core/Zone과 `0xdeadbeef` seed 공유, 서버를 통한 0x67 옵션 파생값, 로컬 Fort 수신의 같은 개정 전역을 확인했다. 원본 fixture 검사까지 집중 52건이 통과했다. 패킷은 부품 입구에서 호출했으며 내부 연결을 통한 실제 왕복은 아직 아니다.

일반 멀티플레이 원격 Fort와 메타 CS는 웹 제외 경계, 스커미시 Fort는 후속 단계다. 수신기 등록·State7·Begin/Pump·전체 실제 내부 왕복·외곽 Session 연결과 기준선 뒤 32커밋 전체 판정은 미완료다. 게임 창·실제 플레이·사제 포획/희생·자연 승리·공중 3종 전용 행동은 실행하지 않았다.

Node v26.13.0 전체 회귀 6,162건 통과(실패·취소·건너뜀 0). E706의 Node 22 통과 이후 전체 회귀는 5/8회다. 로그는 Git 제외 `extracted/webpj-e711-node26.log`다.
