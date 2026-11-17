# Session AI 배치 선택 (WB04-E661)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectAiMovement 2981..2984에서 `RawAiDropSelection`을 같은 Session의 기술 부모·내용물·배치 검사·후보 스캔·난수와 연결했다. 네 방향 TYPE은 사용할 때마다 현재 전역을 읽고, 선택 로그는 원본의 종류와 메시지를 사용한다.

신규 검사는 선행/중복 설치, 동일 Pool과 변경된 현재 방향 TYPE을 확인했다. 기존 cpppj 계약을 이식한 독립 부품 검사 포함 집중 39건이 통과했다. 이 부품의 원본 실행 fixture는 없으며 실제 AI 배치·자연 플레이 및 전체 Load는 실행하지 않았다. `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,082건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e661-node22.log`·`extracted/webpj-e661-node26.log`다.
