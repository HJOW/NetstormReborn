# Session 운반체 이동 요청과 수신 (WB04-E652)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectMotion 3940..3960에서 `RawCarrierMove`를 기존 Session의 선택 수명·현재 Graph·경로 프로세스·정기 이벤트·운반 후보·귀환/간헐천·희생 관문·경로 시작과 내부 전송에 연결했다. 현재 종속 Form이 Path 프로세스가 아니면 명시적으로 거부하며 요청은 실제 수신을 직접 실행하지 않는다. 포획·희생의 자연 재현은 하지 않았다.

신규 Session 검사는 회수 설치 선행/중복 관문과 같은 Pool·실제 18바이트 이동 요청의 헤더/대상 및 전송을 확인한다. 원본 10.78 `carrier-move`의 982입력 요청/수신 대조를 포함한 집중 17건이 통과했다. `ConnectAiMovement`·경로 송신 합성·BridgeCarriers·수신기 등록 및 전체 Load는 후속이며 `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,073건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e652-node22.log`·`extracted/webpj-e652-node26.log`다.
