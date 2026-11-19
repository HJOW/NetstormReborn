# 미션 Renderer의 Client Gump 제출 (WB04-E988)

실제 미션 Renderer가 현재 Client Gump 트리를 dirty마다 수집하고, 다른 표시 항목과 원본 깊이 순서로 정렬한 kind 2를 같은 DIB·pitch·clip의 Gump 픽셀 장치에 출력한다. 파생 출력기 누락은 공통 Client Gump 경계에서 명시적으로 드러난다.

실제 Tutorial1 로그인 후 Client 트리에 부착한 Gump의 출력기가 Renderer 수집·정렬·합성·Present를 거쳐 앱 색 번호 표면에 도달함을 확인했다. 관련 Node 검사 64건과 실제 Chromium의 캠페인 클릭·첫 안내 1건이 통과했다.

실제 HUD 공장과 배경·버튼의 출력기 등록, 월드 부착 Gump의 카메라 투영, 임시 Tell의 원본 Gump 통합 및 입력 소비는 남았다. E982 고정 Node 22 전체 검사는 실행 중이며 이후 대기 중인 E982 고정 Node 26 전체 검사를 순차 실행한다.
