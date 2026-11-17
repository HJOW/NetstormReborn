# Session 일반 MakeAi (WB04-E673)

기준선 `225553df`의 `RawCampaignSession.cpp` 4846..4863을 같은 Session의 AI 기본 설정·분산 SP·Player·섬·소유자·카운터·로딩 깊이·World 삭제에 연결했다. 전투 밖에서는 아무 행동도 실행하지 않으며, owner 0 경로는 객체 이전 → AI 생성 → 마스크 삭제를 원문 순서로 수행한다. 새 할당 입력은 기존 웹 `currentAllocationBytes` 적응을 사용한다. 기준선 뒤 해당 cpppj 파일 변경은 없다.

원본 PE `make-ai-parent-x86.tsv` 770행(SHA-256 `f35293b716d73552a66754cf9ebab34303022c668f0f6e7625870f122413f81e`)과 `make-ai-objects-x86.tsv` 2,592행(SHA-256 `da9b17226e5ca08d3a104877fb623e560b8d32e63d59f8edb2c94ab0b0713cfa`) 및 새 부모 계약 검사로 현재 설정/SP, 객체 이전·깊이·생성·삭제 순서를 대조해 집중 13건 통과했다. 실제 Session Load·AI 생성·자연 플레이는 실행하지 않았고 `loaded=false`다. 나머지 AI 공개 전달은 후속이다.

Node v26.13.0 전체 6,098건 통과(실패·취소·건너뜀 0), 로그는 Git 제외 `extracted/webpj-e673-node26.log`다. Node 22는 E669 후 여덟 번째 전체 회귀에서 다시 실행한다.
