# WB04-E768 Client 기본 기술 목록 수신

기준 `225553df`의 `ClientCampaignTransport.cpp` 285~286행과 `RawCampaignSession.cpp` 4652~4653행을 따라 장기 Client 기본 표에 69h 기술 목록의 관찰기·수신기를 등록하고 Session의 실제 생산 기술 표시를 공개했다. 타입 사용표는 같은 GameAux의 256바이트를 사용한다. 원본처럼 빈 목록은 현재 Session을 조회하지 않고, 표시할 항목마다 현재 Session과 전체 Runtime을 선택해 정상·예외 복귀까지 pin한 뒤 반납한다. 수신기는 이전 Session을 보관하지 않는다.

Client 전송 집중 검사 21건과 실제 TheWarBegins Session의 기술 표시 검사 두 건이 통과했다. 기본 69h 분배에서 실제 Session 생산 부모로 연결하고 Battle snapshot에 해당 기술이 한 번만 기록되는지 확인했다. 이 통합의 Runtime owner는 시험 장치이며 브라우저 GameWorld 연결을 완료한 것은 아니다. 최초 snapshot 검사에서 현재 미션 조회 장치가 빠져 실패했으므로 실제 Fort 저장 계약을 추가해 재검사했다. 채팅 41h·연결 종료 2번과 Native 고정 수신기는 남는다.

Node v26.13.0 전체 회귀 6,268건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 검사 오류는 0건이며 E761의 Node v22 전체 검사 뒤 다섯 번째 v26 전체 회귀다.
