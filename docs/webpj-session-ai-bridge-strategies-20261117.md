# Session AI 다리 전략 (WB04-E667)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectAiRuntime 2925..2936에서 `RawAiBridgePlanning`과 `RawAiBridgeStrategies`를 같은 Session의 후보·난수·기술·현재 그래프·경로에 연결했다. 간헐천 목록과 재시도, 진단 및 로그도 기존 세션 장치를 사용한다.

신규 검사는 선행/중복 설치, 동일 Pool, 디버그 초기값과 반전된 빈 사각형을 확인했다. 기존 원본 PE 관측과 cpppj 계약을 이식한 계획·전략 검사 포함 집중 42건이 통과했다. 실제 AI 다리 건설·자연 플레이 및 전체 Load는 실행하지 않았다. `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,088건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e667-node22.log`·`extracted/webpj-e667-node26.log`다.
