# Session AI 사제 행동 (WB04-E665)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectAiMovement 3005..3008에서 `RawAiPriestActions`를 같은 Session의 탐색·이동·생산·회수·현재 월드에 연결했다. 실제 사제 프레임과 제단의 현재 상태를 읽고, 시각과 지도 제한은 호출할 때 다시 조회한다.

신규 검사는 선행/중복 설치, 동일 Pool 및 사제 없는 본체의 난수 보존을 확인했다. 기존 cpppj 계약을 이식한 독립 부품 검사 포함 집중 48건이 통과했다. 이 부품의 원본 실행 fixture는 없으며 자연 사제 행동·희생·실제 AI 플레이 및 전체 Load는 실행하지 않았다. `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,086건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e665-node22.log`·`extracted/webpj-e665-node26.log`다.
