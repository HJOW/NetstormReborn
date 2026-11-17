# Session AI 배치 표적 평가 (WB04-E658)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectAiMovement 2972..2975에서 `RawAiDropTargets`를 같은 Session의 Pool·TYPE·현재 제단/방향 TYPE, 지상·Gun 표적 검색, 장애물, 객체 중심·피해 속도·최대 HP 및 진단 장치에 연결했다. AI 배치 평가와 나머지 Runtime은 아직 설치하지 않았다.

신규 검사는 선행/중복 설치와 같은 풀의 현재 TYPE 피해 속성 조회를 확인했다. 기존 cpppj 계약을 이식한 독립 부품 검사 포함 집중 35건이 통과했다. 이 부품의 원본 실행 fixture는 없으며 실제 적 공격·포획·AI 플레이 및 전체 Load는 실행하지 않았다. `loaded=false`를 유지한다.

Node v22.23.3·v26.13.0 전체 각각 6,079건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e658-node22.log`·`extracted/webpj-e658-node26.log`다.
