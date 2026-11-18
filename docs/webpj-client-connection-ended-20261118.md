# WB04-E770 Client 기본 연결 종료 수신기

기준 `225553df`의 `ClientCampaignTransport.cpp` 139~148행과 `RawCampaignSession.cpp` 4267~4277행을 따라 Client 기본 2번 연결 종료 수신기를 현재 동기화·상태·연결·Player·활동 프레임에 연결했다. 이 등록은 기존 관찰기·버전·길이를 바꾸지 않는다. 서버 실패 저장과 Player 방송이 선택된 경우에만 현재 Session과 Runtime을 pin하고 정상·예외 복귀 시 반납한다.

Session에는 서버 표식 Player의 연결 비트 정리와 현재 접속 대상별 2bh Player 상태 방송을 연결했다. 미션 또는 Player 미확정 시 재접속 저장은 자연 반환하며, 웹 범위 밖 비미션 재접속 저장이 선택되면 명시 오류를 낸다. 방송은 같은 Registry의 mutable 송신과 현재 Player 기록을 사용하고 각 송신 뒤 다음 대상의 연결 상태를 다시 읽는다.

Client 전송과 연결 종료 집중 검사 33건 및 실제 TheWarBegins Session 검사 2건이 통과했다. 실제 Session을 현재 Runtime으로 공개하고 Client 기본 종료 통지에서 Player 상태 변경과 2bh 방송까지 확인했다. 최초 집중 검사에서 기존 기본 길이를 0으로 잘못 기대했으나 원래 가변 길이 0xffff로 교정했다. Runtime owner와 연결 장치는 시험 장치이며 실제 브라우저 GameWorld와 Native canonical 수신기 연결은 후속이다.

Node v26.13.0 전체 회귀 6,272건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 검사 오류는 0건이며 E761의 Node v22 전체 검사 뒤 일곱 번째 v26 전체 회귀다. 다음 코드 단위 전체 회귀가 여덟 번째이므로 v26 종료 후 Node v22를 순차 실행한다.
