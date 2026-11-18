# WB04-E815 State·Quick Help 팝업 전역

기준선 `225553df`의 `QuickHelpPopupState` 명령 DWORD와 80바이트 인자를 장기 소유자로 이식하고 `ClientStateWorldGlobals`의 `005ca938` 직접 접근에 연결했다. null 표식은 인자 전체를 지우고 빈 바이트열은 첫 NUL만 쓴다. 길이 오류는 쓰기 전에 거부하며 현재 첫 NUL 앞 뷰를 반환한다. State가 명령을 0으로 써도 인자는 보존한다. 기준선 이후 관련 cpppj 소스 변경은 없다.

표식·오류·꼬리 보존 및 실제 State 바인딩의 공유 저장소를 집중 검사했다. 아직 실제 RawMenuControls·Quick Help 창 수명과 연결하지 않았고, 음향 장치와 전체 State 부모 결선도 남았다.

검증: 집중 검사 2건 통과, 대응표 1,160행 오류 0, Node v26.13.0 전체 6,302건 통과(실패·취소·건너뜀 0). E811 Node v22 전체 검사 뒤 v26 회귀 4/8회다.
