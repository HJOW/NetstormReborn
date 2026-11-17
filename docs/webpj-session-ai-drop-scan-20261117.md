# Session AI 배치 스캔 (WB04-E660)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectAiMovement 2979..2980에서 `RawAiDropScan`을 같은 Session의 AI 본체·Pool·TYPE·프레임·표면 지도·spot·동맹·난수 및 배치 탐색/평가에 연결했다. 원본 초기 스캔 범위 10,10..246,246은 같은 Session 소유 상태로 보존한다.

신규 검사는 선행/중복 설치, 동일 Pool·초기 범위 및 일반 표면에서 위치/난수 보존을 확인했다. 기존 cpppj 계약을 이식한 독립 부품 검사 포함 집중 39건이 통과했다. 이 부품의 원본 실행 fixture는 없으며 실제 AI 배치·자연 플레이 및 전체 Load는 실행하지 않았다. `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,081건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e660-node22.log`·`extracted/webpj-e660-node26.log`다.
