# 첫 캠페인 진입과 AI 다리 탐색 (WB04-E1012)

원본 캠페인 목록의 Struggle For Freedom 첫 미션 TheWarBegins를 기존 Guide·World 경로에 연결했다. 브라우저 실행 중 적 AI의 다리 탐색에서 누락된 caller 8바이트 입력을 기존 웹 CurrentAllocationBytes에 연결했다. JavaScript의 새 Uint8Array는 0으로 초기화되므로 원본 Windows의 미초기화 스택 바이트와 동일하다고 주장하지 않는다.

Chromium에서 실제 목록 선택·브리핑 종료 뒤 게임 시계 12초 동안 AI를 포함한 월드가 페이지 오류 없이 실행됐다. AI 다리 경로·검색·전략과 앱 검사 Node 37건이 통과했다. 미션 승리와 후속 미션, 생산 HUD와 장시간 AI 동작은 미검증이다.
