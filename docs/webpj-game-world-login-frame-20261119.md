# WB04-E855 실제 GameWorld 로그인 프레임 통합

TheWarBegins의 실제 `CampaignGameWorld` State7 → State8 → 내부 로그인 검사를 확장했다. 같은 Client의 연결 등록소·활동 표·Player 기록과 프레임 시각을 World에 제공하고 `frameLoginTimer`를 켰다. 실제 로그인 Ready 뒤 Pump가 기한을 남기는지 확인하고, `pollClientConnectionHealth`가 같은 World의 활동 기한과 로그인 타이머를 처리해 진행 창 serial을 닫는지 검사했다.

집중 검사 1건과 Node v26.13.0 전체 회귀 6,369건이 통과했다(실패·취소·건너뜀 0). 메뉴와 진행 창은 관측 장치이며 실제 브라우저 Client 프레임, 화면과 자연 플레이 검사는 남는다. 원본 게임 창은 실행하지 않았다.
