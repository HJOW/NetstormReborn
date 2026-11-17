# Session AI 전체 프레임 (WB04-E668)

기준선 `225553df`의 `RawCampaignSession.cpp` 2937..2943을 같은 Session의 AI 초기화·준비·생산·사제·배치·다리·운반·경제와 Player, SP, 시계 및 진단 장치에 연결했다. 해당 cpppj 파일은 기준선 뒤 변경이 없다. 실행 시간 측정값은 원문의 Session 소유 초기값 1이며 부족 좌표 로그는 기존 Client 전역을 공유한다.

기존 원본 10.78 PE `ai-runtime-x86.tsv` 145행(SHA-256 `38016d7426a64fc8473b1ce97ba775e8c99c04530d73a00f732f1209ee9ecb05`)과 Runtime 부품 시험, 새 Session 검사로 설치 순서·로그·SID 범위·현재 타이밍·반납을 대조했다. 처음에는 편집 파이프의 인코딩 때문에 설치 함수가 추가되지 않아 검사에 실패했고 UTF-8 패치로 교정했다. 첫 전체 검사에서는 간접 검사를 fixture 직접 소비자로 등록한 오류가 나서 목록을 바로잡고 재검사했다. 실제 Session Load와 AI 자연 플레이는 검사하지 않았으며 `loaded=false`다. 다음은 ConnectMotion 경로 송신 합성과 BridgeCarriers다.

Node v22.23.3·v26.13.0 전체 각각 6,089건 통과(실패·취소·건너뜀 0). 전체 로그는 Git 제외 `extracted/webpj-e668-node22.log`·`extracted/webpj-e668-node26.log`다. v24는 사용자 지시에 따라 생략한다.
