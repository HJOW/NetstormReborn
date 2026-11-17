# Session Login Fort 파일 선택과 이미지 (WB04-E708)

`RawCampaignSessionLogin.inc`의 `SelectFile`·`FileHooks`·`Reload`와 State9 로그인 이미지 작성, State13 누락 Fort 읽기를 실제 Template 저장소와 전송에 연결했다. 선택기는 같은 파일 이름과 지연 읽기를 묶고, 기존 manager 이름 비교는 원문의 ASCII 대소문자 무시 규칙을 쓴다. 실제 파일 선택 시 지역 Template에서 네 고정 섹션과 현재 Terr 하나만 복사해 0x27에 담고 임시 manager를 반납한다. 독립 Session은 현재 manager의 이미지를 사용한다.

실제 Tutorial 1 Fort로 State9의 다섯 섹션과 나머지 빈 섹션, 전역 manager 보존, 선택·읽기·송신 순서 및 State13의 누락 manager 읽기를 확인했다. 전송은 관찰 장치로 확인했으므로 열린 내부 연결의 왕복은 아직 아니다. 최초 집중 검사는 원문의 State9 벽시각 조회를 예상 순서에서 누락해 실패했고, 실제 순서를 반영한 뒤 관련 39건이 통과했다.

로그인 서버/클라이언트 수신기, 타이머·State7·Begin/Pump, 외곽 Session 연결과 기준선 뒤 32커밋 전체 판정은 미완료다. 게임 창·플레이·공중 3종 전용 행동은 실행하지 않았다.

Node v26.13.0 전체 회귀 6,152건 통과(실패·취소·건너뜀 0). E706의 Node 22 통과 이후 전체 회귀는 2/8회다. 로그는 Git 제외 `extracted/webpj-e708-node26.log`다.
