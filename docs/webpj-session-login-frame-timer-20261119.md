# WB04-E852 Session 로그인 프레임 타이머 공개

기준선 `225553df`의 `RawCampaignSession::RunConnectionLoginTimer`를 이식했다. 호출 시 살아 있는 같은 Session의 Login이 있으면 기존 `RawMissionLoginTimer.run(false)`를 실행하고 없으면 아무 일도 하지 않는다. 기한 0 판정은 원본처럼 타이머 본체가 처리한다.

실제 TheWarBegins Session의 내부 로그인에서 `frameLoginTimer`를 켜고 Ready까지 진행했다. Pump를 다시 호출해도 공유 로그인 기한이 남고, 명시 프레임 호출에서 기존 타이머가 기한을 지우고 진행 안내를 닫으며 다음 호출은 무효과임을 확인했다. Login 생성 전과 Session 반납 후 경계도 검사했다. 집중 검사 1건과 Node v26.13.0 전체 6,366건이 통과했다(실패·취소·건너뜀 0).

실제 Client 입력·연결 감시 뒤 프레임 순서와 브라우저 앱 호출은 아직 연결하지 않았다. 원본 게임 창은 실행하지 않았다.
