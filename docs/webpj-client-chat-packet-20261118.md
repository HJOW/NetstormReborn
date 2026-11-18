# WB04-E769 Client 기본 채팅 수신기

기준 `225553df`의 `ClientChatPacketReceiver.cpp`와 `RawChatRelay.cpp`를 따라 장기 Client 기본 표에 41h 수신기와 송신 관찰기(version 1·policy 0·가변 길이)를 연결했다. 같은 Client의 현재 전투·서버·subscriber·연결 주소와 Chat 마스크를 읽고, 수신 시 현재 Chat 창이 없으면 메뉴를 조회하지 않는다. 창이 있으면 그 시점의 메뉴 수신 장치에 같은 패킷을 전달하며, 선택된 UI가 없으면 오류를 전파한다. 원격 subscriber로의 LAN 채팅 중계는 기존 웹 제외 경계를 유지한다.

기본 등록표에 세션 수신기를 일시 등록했다가 반납하면 원래 41h 수신기가 복구되는지 확인했다. 실제 원본 중계 fixture 132행과 송신 길이·마스크·현재 Chat 창 관문·UI 선택·오류를 포함한 집중 검사 27건과 기존 69h 실제 Session 집중 검사가 통과했다. 이번에는 Client 계약 시험 장치를 사용했으며 브라우저 Chat UI와 실제 GameWorld·Native canonical 등록은 아직 연결하지 않았다.

Node v26.13.0 전체 회귀 6,269건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 검사 오류는 0건이며 E761의 Node v22 전체 검사 뒤 여섯 번째 v26 전체 회귀다.
