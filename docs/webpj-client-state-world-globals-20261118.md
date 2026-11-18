# WB04-E810 State 장기 전역 DWORD 앞단

기준선 `225553df`의 `ClientStateWorldGlobals` 중 연결 활동 프레임 22 DWORD, Client Local/Gameplay DWORD, Player 소유자 조회와 원문 모드의 읽기·쓰기 경계를 실제 장기 소유자에 부분 연결했다. 읽기 전용 주소는 쓰기를 거부하고 bool 저장은 정상 0/1만 허용한다. Fort의 Player 별칭과 Battle의 Bridge 별칭을 같은 원문 쓰기에서 갱신한다. 바인딩 시 실제 표면·권한·GameAux 정체성도 확인한다. 기준선 이후 해당 cpppj 파일 변경은 없다.

실제 `ClientLocalStateGlobals`·`ClientGameplayGlobals`·`ClientConnectionActivityGlobals`·`RawSurfaceStorage`에서 현재 값 재조회, 22번째 프레임과 인접 Zone 보존, DWORD 상위 비트, bool 거부 및 모드 별칭을 확인했다. Audio·Template·TYPE·Pool과 GameAux의 나머지 직접 접근, WORD/double 및 실제 Client/State 부모 연결은 후속이다. 이 부분 바인딩은 미이식 주소를 명시 거부한다.

검증: 집중 검사 통과, 대응표 1,160행 오류 0, Node v26.13.0 전체 6,301건 통과(실패·취소·건너뜀 0). E803 Node v22 전체 검사 뒤 v26 회귀 7/8회다.
