# WB04-E935: Client Gump 공통 idle 분배

cpppj `RawButtonControls`가 같은 `GumpFrame`의 idle 표에서 실제 파생 처리기를 선택하는 경계를 Client Gump 소유자에 연결했다. 파생 노드별 idle 처리기는 같은 주소·등록·수명으로 보유하고 실제 반납에서 제거한다. 기본 노드의 idle은 빈 본문이며 미연결 파생이 실제 idle로 선택되면 명시 오류다.

실제 Client 등록표와 idle 표에서 앞 처리기가 뒤 슬롯을 비웠을 때 현재 표를 다시 읽는 순서, 재등록·삭제 및 반납 뒤 접근을 검사했다. Gump 수명과 원본형 TextEntry 관련 Node 검사 49건이 통과했다.

현재 공통 idle을 호출할 수 있지만 브라우저 메인 루프 자동 호출과 TextEntry 실제 공장·점멸 연결은 아직 남았다.
