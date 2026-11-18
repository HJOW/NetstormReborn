# WB04-E853 Session 연결 활동 패킷

기준선 `225553df`의 `RawCampaignSession::SendConnectionHeartbeat`를 이식했다. 같은 Session의 Login이 있을 때 원본과 같은 5바이트 `01 64 05 00 00`을 만들고, 서버 대상은 실제 Login 전송 9번에, 방송은 같은 Player 기록과 연결을 쓰는 `RawBattleBroadcast`에 전달한다. Login이 없거나 Session을 반납한 경우 거부한다.

실제 TheWarBegins 내부 로그인 뒤 서버 전송이 후속 Pump에서 64h 수신기로 들어가고 같은 Client 활동 표의 프레임이 갱신됨을 확인했다. 별도 Player 연결의 방송 수신과 접속 비트가 꺼졌을 때의 생략도 확인했다. 첫 집중 실행에서는 시험 장면에 실제 프레임 공급자가 없어 수신이 실패했으며 공급자를 추가한 뒤 통과했다. Node v26.13.0 전체 6,367건이 통과했다(실패·취소·건너뜀 0).

Client 프레임의 연결 감시·재접속 부모와 실제 브라우저 수명 결선은 남아 있다. 원본 게임 창은 실행하지 않았다.
