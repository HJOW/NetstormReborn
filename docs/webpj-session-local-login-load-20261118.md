# WB04-E716: 외곽 Session의 로컬 Login과 Load 수명

`RawCampaignSession`이 실제 Login 앞단을 소유하고 원본의 독립 시작, 외부 State7 후반 준비, 로컬 상태 Poll, 전송 Pump, 상태·수신 기록 조회와 완료 판정을 부분 연결했다. Login 생성·등록이 실패하면 해당 전송을 반납하고, Session 반납 시 Login도 같은 실제 소유자보다 먼저 반납한다. 현재 Client나 외부 전송의 전체 State 반납·수신 callback 대여 증빙은 별도 4D-5·4D-8에 남는다.

27h 서버가 월드 설치를 선택하면 같은 외곽 Session의 실제 `load()`를 호출한다. 현재 `RegisterCarrierCommands`·`RegisterConstructionCommands`·`RegisterFactoryResearchCommands`는 아직 조립되지 않았으므로 Load 뒤 해당 위치에서 명시 오류를 내고 서버 후반/클라이언트 응답/준비 성공을 합성하지 않는다. 해당 수신 등록은 4D-3e에서 연결한다.

실제 Tutorial 1 자료의 프로세스 내부 시험에서 State8→9→10, 27h 수신과 실제 Load, 명령 등록 미연결 오류 및 반납을 확인했다. 관련 집중 검사 43건이 통과했다. 공중 유닛 전용 생성·행동이나 사제 포획·희생·승리를 새로 검사하지 않았으며 실제 게임이나 브라우저 창은 실행하지 않았다.
