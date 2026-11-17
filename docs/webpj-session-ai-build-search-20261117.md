# Session AI 배치 탐색 (WB04-E656)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectAiMovement 2969에서 `RawAiBuildSearch`를 같은 Session의 AI 본체·188 TYPE·spot·벽시각 및 현재 배치 파이프라인에 연결했다. 실제 게임 배치 파이프라인은 후속 설치 단계에서 구성하므로 그 전에 검사에 도달하면 기존 미설치 오류를 유지한다.

신규 검사는 AI 운반 행동 선행/중복 관문과 실제 AI 본체의 탐색 TYPE·500 stride 핸들 쓰기를 확인했다. 기존 cpppj 계약을 이식한 독립 부품 검사를 포함해 집중 32건이 통과했다. 실제 AI 배치와 전체 Load는 실행하지 않았고 `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,077건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e656-node22.log`·`extracted/webpj-e656-node26.log`다.
