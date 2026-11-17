# Session Login 타이머와 현재 Player 장부 (WB04-E709)

`RawCampaignSessionLogin.inc`의 `RawMissionLoginTimer`를 같은 Client의 로그인 기한·Begin·스크립트 중단·요청 phase·시계와 실제 Player 9기록에 연결했다. State13은 연결이 열려 있을 때 같은 타이머를 시작한다. 서버 방송·사제 상태 조회·연결 활동·선택된 UI 경계는 이미 존재하는 실제 부모를 사용한다.

미션 또는 현재 Player 미확정 시 재접속 저장은 cpppj `RawSavedStateWrite.cpp`의 파일 접근 전 관문대로 자연 반환한다. 비미션 저장과 일반 전투 초기화는 후속 스커미시·웹 저장 단계의 명시 오류다. 실제 Session owner에서 현재 Player 집계와 예약, 연결 상실 시 공유 상태의 중단 순서, 빈 월드의 미션 로그인 완료 관문, State13 타이머 시작을 검사했다. 최초 State13 집중 검사는 시험 Fort의 읽기 전용 표식을 빠뜨려 기존 진단에서 실패했고 표식을 갖춘 뒤 관련 47건이 통과했다.

서버/클라이언트 로그인 수신기, State7·Begin/Pump, 실제 내부 로그인 왕복, 외곽 Session 연결과 기준선 뒤 32커밋의 전체 판정은 미완료다. 게임 창·실제 플레이·사제 포획/희생·자연 승리·공중 3종 전용 행동은 실행하지 않았다.

Node v26.13.0 전체 회귀 6,156건 통과(실패·취소·건너뜀 0). E706의 Node 22 통과 이후 전체 회귀는 3/8회다. 로그는 Git 제외 `extracted/webpj-e709-node26.log`다.
