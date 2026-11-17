# Session AI 부족 좌표 갱신 (WB04-E663)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectAiMovement 2987..2988에서 `RawAiFakeDrySpots`를 같은 Session의 부족 거리 평가·지도 기하·좌표 목록·공용 난수와 연결했다. 로그 종류와 공급 범위는 호출할 때 현재 값을 읽는다.

신규 검사는 선행/중복 설치와 동일 Pool 및 빈 AI 본체의 난수 보존을 확인했다. 기존 cpppj 계약을 이식한 독립 부품 검사 포함 집중 38건이 통과했다. 이 부품의 원본 실행 fixture는 없으며 실제 AI 배치·자연 플레이 및 전체 Load는 실행하지 않았다. `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,084건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e663-node22.log`·`extracted/webpj-e663-node26.log`다.
