# Session AI 배치 행동 (WB04-E664)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectAiMovement 2989..3004에서 `RawAiDropActions`를 같은 Session의 생산 조각·배치 선택·후보 스캔·비용 판정·부족 좌표·공용 난수에 연결했다. 현재 시각과 좌표 제한, 진단 플래그 및 원본 배치 로그를 사용한다.

신규 검사는 선행/중복 설치와 동일 Pool 및 현재 생산 비용 위임을 확인했다. 기존 cpppj 계약을 이식한 독립 부품 검사 포함 집중 45건이 통과했다. 이 부품의 원본 실행 fixture는 없으며 실제 AI 배치·자연 플레이 및 전체 Load는 실행하지 않았다. `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,085건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e664-node22.log`·`extracted/webpj-e664-node26.log`다.
