# WB04-E760 실제 로컬 Login 패킷 송신

기준선 `225553df`의 `RawCampaignSession::SendLocalPacket` 캠페인 경로를 `RawCampaignSession.sendLocalPacket(target, packet)`으로 연결했다. 설치 전과 실제 Login 전송이 없는 Session은 원본처럼 거부한다. 설치된 Login은 `RawCampaignSessionLoginFront.send`에서 같은 내부 전송의 읽기 전용 송신을 사용하며 패킷의 sender·길이·내용을 재작성하거나 수신을 동기 실행하지 않는다. Editor 전송은 웹 범위 밖이다.

실제 TheWarBegins Session을 내부 로그인하고 31h 명령을 공개 송신했다. Pump 전에는 패킷 수신 기록과 현재 설정이 바뀌지 않고, 송신 뒤 호출자 버퍼를 변경해도 Pump에서 원본 복사본을 sender 1로 받아 같은 Session의 설정을 변경하는 것을 확인했다. 설치 전·Login 없는 설치 Session·반납 후에도 각각 호출을 거부하는 집중 검사 1건이 통과했다. 실제 게임 창 실행은 없다.

기준선 `225553df` 이후 해당 `RawCampaignSession.cpp`와 `RawCampaignSessionLogin.inc` 제품 소스 변경은 없다. Login 대응표 행의 별도 base `a328ac1f` 뒤 32커밋 전체 판정은 남아 있다. 호출 경계를 연결한 것으로 UI·미션 프레임·브라우저 앱의 실제 사용은 아직 아니다.

Node v26.13.0 전체 회귀 6,259건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E753의 v22 검사 뒤 일곱 번째 v26 전체 회귀다. 다음 전체 회귀 뒤에는 Node v22를 순차 실행한다.
