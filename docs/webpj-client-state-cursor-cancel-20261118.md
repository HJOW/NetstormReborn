# WB04-E809 State 커서 취소 경계

기준선 `225553df`의 `CancelClientStateCursor`를 이식했다. Gameplay에 Session 대여가 있고 실제 커서가 설치됐으면 Session의 State 전용 취소를 호출한다. 첫 메뉴 또는 설치 전에는 호출자가 제공한 같은 Client의 초기 커서 취소를 호출하고, 선택된 장치가 없으면 거부한다. Session에 커서 설치 여부와 독립적인 State 전용 취소를 공개해 일반 취소의 Prepare를 추가하지 않는다. 기준선 이후 해당 cpppj 파일 변경은 없다.

실제 Gameplay·TheWarBegins Session에서 Session 없는 첫 메뉴와 커서 설치 전의 초기 장치 호출, 실제 사제 메뉴가 취득한 워크샾 미리보기의 State 취소를 확인했다. 설치 전 Session 취소와 초기 장치 누락도 거부한다. 전체 State 부모의 다른 효과와 실제 Client 장치 결선은 남았다.

검증: 실제 Session 집중 검사 통과, 대응표 1,160행 오류 0, Node v26.13.0 전체 6,300건 통과(실패·취소·건너뜀 0). E803 Node v22 전체 검사 뒤 v26 회귀 6/8회다.
