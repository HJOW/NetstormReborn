# WB04-E814 State double 기한 비트

기준선 `225553df`의 State double 직접 쓰기 9주소를 연결했다. GameAux의 네 기한과 Client Local의 네 기한은 각각 같은 장기 double 물리에 64비트로 기록한다. 음악 기한은 실제 음향 장치의 `writeSongEndBits`를 해당 주소에 도달했을 때만 요구한다. `DoubleBox`와 GameAux의 double을 `Float64Array` 물리로 보관해 기존 숫자 접근과 직접 비트 쓰기가 같은 값을 공유한다. 기준선 이후 관련 cpppj 소스 변경은 없다.

실제 GameAux·Local 소유자에서 NaN 페이로드와 정상 double 값을 비교하고 미등록 주소 및 음향 장치 누락을 확인했다. 음악 장치 호출은 관찰 대역으로만 확인했으며 Web Audio 실제 소유자 연결은 8단계에 남았다. 메뉴 팝업과 State 부모의 실제 Client 수명도 미연결이다.

검증: State 집중 검사와 GameAux 12건·연결 전역 16건 통과, 대응표 1,160행 오류 0, 관련 fixture 추적 항목을 선택 갱신했다. Node v26.13.0 전체 6,301건 통과(실패·취소·건너뜀 0). E811 Node v22 전체 검사 뒤 v26 회귀 3/8회다.
