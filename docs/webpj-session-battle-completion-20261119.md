# WB04-E843 실제 Session의 일반 전투 종료 조회

기준선 `225553df`의 `RawCampaignSession::BattleFinished`를 기존 전투 종료 관문에 연결했다. 같은 Session이 보유한 Player·방향별 동맹·전투 옵션과 현재 battle/missionPost/boss, LocalState의 scriptBreak·loginEnd를 빌린다. 서버 객체는 그 시점의 로그인 전송에서 9번 연결의 존재를 조회하고, 사제 생존은 같은 Session의 `RawMissionOutcome`을 사용한다. 선택된 조회 전까지 로그인 장치를 요구하지 않으며 Session 반납 후에는 접근을 거부한다.

실제 1-1 Fort의 내부 로그인 Session으로 미션 관문과 로그인 전 연결 누락을 검사했다. 로그인 후에는 일반 전투 관문을 강제로 선택하고 실제 Player 기록·동맹을 조정해 양방향 동맹과 사제 사망의 반환을 확인했다. 이 검사는 완성된 스커미시 실행이나 게임 승패 처리의 증거가 아니다. `RawBattleCompletion::Frame`, 실제 메뉴 결과·종료 수명과 일반 전투 설치는 아직 남았다. 원본 게임 창은 실행하지 않았다.

검증: 실제 Session 집중 검사 1건, Node v26.13.0 및 v22.23.3 전체 각각 6,348건 통과(실패·취소·건너뜀 0), 대응표 1,160행 오류 0. v22 회귀 뒤 v26 회귀 0/8회다.
