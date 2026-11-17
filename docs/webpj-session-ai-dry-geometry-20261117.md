# Session AI 부족 좌표 기하 (WB04-E662)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectAiMovement 2985..2986에서 `RawAiDryGeometry`를 같은 Session의 지도·Pool·World에 연결했다. 디버그 표식 TYPE과 로그 종류, 탐색 범위는 사용할 때 현재 값을 읽으며, 객체 상태 WORD 기록 뒤 현재 다리만 표면에 통지한다.

신규 검사는 선행/중복 설치, 동일 Pool·실제 지도 조회와 객체 WORD 기록 및 반납 관문을 확인했다. 기존 cpppj 계약을 이식한 독립 부품 검사 포함 집중 36건이 통과했다. 이 부품의 원본 실행 fixture는 없으며 실제 AI 배치·자연 플레이 및 전체 Load는 실행하지 않았다. `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,083건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e662-node22.log`·`extracted/webpj-e662-node26.log`다.
