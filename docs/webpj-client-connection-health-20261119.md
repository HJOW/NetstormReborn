# WB04-E854 Client 연결 감시 프레임

기준선 `225553df`의 `UberGump::PollConnectionHealth`를 `pollClientConnectionHealth`로 이식했다. 같은 Client의 상태·연결·Player·활동 표와 현재 World를 사용해 기존 `pollConnectionHealth`를 호출하고, 선택된 재접속 부모 뒤 현재 World를 다시 조회해 같은 Session 로그인 타이머를 실행한다. 패킷 콜백 중 재진입은 감시 전에 거부한다. 안내와 재접속은 선택된 위치의 외부 장치이며 실제 LAN 재접속을 구현하지 않는다.

TheWarBegins의 실제 Session·로컬 로그인과 Client 장기 전역을 연결해 활동 방송이 먼저 나가고 프레임 타이머가 같은 로그인 기한을 정리하는 순서를 확인했다. 별도 검사에서는 안내 중 World가 교체되면 후속 타이머가 새 World를 조회하는 것과 콜백 재진입을 확인했다. 처음에는 시험의 기본 서버 권한을 클라이언트로 바꾸지 않아 지연 안내가 선택되지 않았고, 실제 분기 조건을 맞춘 뒤 통과했다. Node v26.13.0 전체 회귀 6,369건이 통과했다(실패·취소·건너뜀 0).

`UberReconnect.inc`는 재접속 부분만 있는 것으로 분류됐으나 `PollConnectionHealth`는 단독 미션의 타이머도 호출한다. 따라서 대응표 재기준선 분류를 `exclude-mp`에서 `port/partial`로 바로잡았다. 실제 브라우저 Client·경고 UI·프레임 루프 및 재접속 제외 경계의 최종 판정은 남는다. 원본 게임 창은 실행하지 않았다.
