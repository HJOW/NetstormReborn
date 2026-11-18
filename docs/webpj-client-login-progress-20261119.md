# WB04-E851 Client 로그인 진행 안내 장치

기준선 `225553df`의 `BindCampaignLoginStateClientServices` 중 로그인 진행 안내·진행 창 닫기·번역·프레임 벽시각·타이머 안내와 serial 닫기를 `bindCampaignLoginProgressServices`로 연결했다. 각 호출에서 현재 Client와 메뉴를 다시 조회하며 없는 창은 선택된 호출에서 진단한다. 10.72 판본은 연결을 변경하지 않는다.

단위 검사에서 창 교체·누락, 실제 `ClientLocalStateGlobals`의 serial 참조 및 삭제, 현재 시계, 원문 바이트 번역을 확인했다. 실제 TheWarBegins `CampaignGameWorld`의 State7 → State8 → 내부 로그인 시험에 이 장치를 제공하여 `MissionTransferring`, 미션 창 닫기와 타이머 창 닫기를 관측했다. 첫 실행에서 시험 메뉴에 타이머 창 닫기 장치를 빠뜨려 실패했으며 해당 장치를 추가한 뒤 통과했다. Node v26.13.0과 v22.23.3 전체 회귀는 각각 6,365건 통과했다(실패·취소·건너뜀 0).

여기서 메뉴와 창은 검사 장치다. 실제 브라우저 Client·UberGump 소유, 타이머의 Client 프레임 분리(`frameLoginTimer`), 나머지 로그인 서비스와 스커미시/재접속은 아직 연결하지 않았다. 원본 게임 창은 실행하지 않았다.
