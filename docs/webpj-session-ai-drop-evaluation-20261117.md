# Session AI 배치 평가 (WB04-E659)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectAiMovement 2976..2978에서 `RawAiDropEvaluation`을 같은 Session의 AI 본체·배치 표적·에너지 공급·부족 좌표 및 현재 반경/TYPE에 연결했다. 실제 게임 배치 파이프라인은 후속 설치 단계에서 구성하므로 선택된 경로에서 미설치 오류를 유지한다.

신규 검사는 선행/중복 설치, 같은 Pool의 AI 본체와 빈 부족 좌표에서 인덱스 보존·점수 0을 확인했다. 기존 cpppj 계약을 이식한 독립 부품 검사 포함 집중 38건이 통과했다. 원본 실행 fixture는 이 부품에 없으며 실제 AI 배치·자연 플레이 및 전체 Load는 실행하지 않았다. `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,080건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e659-node22.log`·`extracted/webpj-e659-node26.log`다.
