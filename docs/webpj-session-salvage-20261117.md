# Session 회수 명령 결선 (WB04-E651)

기준선 `225553df` `RawCampaignSession.cpp`의 ConnectMotion 3931..3939에서 `RawSalvageCommand`를 같은 현재 Session의 Player·회수 금지·checking·미션·권한·제단/다이스/너겟 TYPE과 연결했다. 회수 요청은 설치 시 보관한 내부 전송으로 패킷을 보내고, 수신의 환불과 가상 삭제는 같은 보상 장부와 World에 넘긴다. 실제 수신기 등록과 전체 Load는 후속이며 `loaded=false`다.

신규 Session 검사는 현재 회수 금지 값 변경에 따른 Tell/전송 선택과 실제 8바이트 요청, 요청 시 객체 보존 및 설치 순서를 확인했다. 기존 원본 회수 fixture 10.78 3,458행의 요청·송신·수신 대조를 포함한 집중 17건이 통과했다. 실제 미션과 자연 플레이는 실행하지 않았다.

Node v22.23.3·v26.13.0 전체 각각 6,072건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e651-node22.log`·`extracted/webpj-e651-node26.log`다.
