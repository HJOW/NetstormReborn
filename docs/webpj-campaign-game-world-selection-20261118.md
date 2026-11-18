# WB04-E793 World 선택 전역과 삭제 효과

지연 `CampaignGameWorld`가 `ClientGameplayGlobals.selection`의 같은 선택 DWORD를 Session 삭제 조회에 전달하고, 삭제의 `clearSelection` 효과를 같은 `RawUserSelection.clear()`에 연결한다. 다른 파생 삭제 효과는 기존 서비스로 전달하며 장치가 없으면 오류를 보존한다. Session 선택 처리기는 같은 전역의 활성화·다중 선택 배열을 빌린다. World의 `resetSelection`은 `select(0,0,0)`을 쓰고 `clearSelection`은 원본처럼 기존 선택 슬롯을 재조회하지 않는다. 현재 객체 보기에서 선택 번호가 사라지면 원본처럼 전체 DWORD를 0으로 지운다.

TheWarBegins 실제 로그인에서 같은 사제의 선택·초기화와 유지/부재 선택 번호를 확인했다. 실제 Session 표면 효과 경계에 `clearSelection` 이벤트를 주입해 같은 선택 전역과 강조 콜백까지 도달하는 것도 확인했다. 이는 실제 객체 삭제 전체 과정의 검증이 아니며, 전체 강조·운반 입력 조립, 화면 이벤트·프레임·브라우저 앱 연결은 남는다. 포획·희생·승패는 실행하지 않았다.

검증: Node v26.13.0 전체 6,293건 통과(실패·취소·건너뜀 0), 대응표 1,160행 오류 0. 마지막 Node v22 전체 검사는 E787이며 이번 단위까지 이후 v26 전체 검사 6/8회다.
