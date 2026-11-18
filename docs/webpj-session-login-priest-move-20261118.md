# WB04-E754 로그인된 Session의 사제 이동 요청

`RawCampaignSession.requestCarrierMove(parent, destination, sacrifice)`를 기존 같은 Session의 `RawCarrierMove.request`에 연결했다. 설치 전과 반납 후에는 거부하고, 입력 좌표는 C++ `RawPathPoint`의 float 인자처럼 단정도로 변환한 뒤 선택·연결·기존 경로 관문과 실제 전송에 전달한다. 요청이 서버 수신이나 이동을 직접 실행하지는 않는다.

기준은 `225553df`의 `RawCampaignSession.cpp` 5583행과 `CarrierMove()`의 설치 관문, `RawCarrierMove.cpp` 및 `cpppj/tests/CampaignSessionTests.cpp`의 `CampaignSession_CarrierMoveRequestThroughActualLocalTransportAndKernel`이다. 두 제품 소스는 기준선 이후 변경되지 않았고, 테스트 파일의 변경 11행 추가·7행 삭제는 해당 검사 밖이다. 새 원본 fixture나 게임 실행은 없다.

캠페인 1-1 TheWarBegins의 실제 Session을 내부 로그인한 후 선택되지 않은 로컬 사제의 이동을 요청했다. 전송 전에는 수신·경로가 없고 Pump 후 sender 1의 `40h` 18바이트, 사제 SID·목적지·희생 0과 같은 Host의 `RawPathProcess`를 확인했다. 경로의 저장 목적지 `(89.5, 107.5)`와 같은 float가 되는 미세한 double 차이 입력은 원본처럼 중복 송신하지 않았다. 설치 전·반납 후 호출도 거부한다.

최초 검사에서는 경로 내부의 저장 좌표가 요청 좌표 `(90,108)`와 같다고 가정해 실패했다. C++ 검사처럼 실제 저장 좌표를 읽어 중복 목적지에 사용하도록 바로잡았다. 이후 코드 검토에서 공개 인자의 float 변환 누락을 찾아 보정하고 미세한 double 차이의 중복 요청도 확인했다. 앞선 전체 회귀는 이 보정 때문에 중단하고 최종 코드를 대상으로 다시 실행했다. 관련 집중 검사 86건과 수정 후 이동 집중 검사 1건이 통과했다.

선택된 사제의 UI 해제·실제 프레임 실행에 따른 자연 도착은 아직 연결·검증하지 않았다(4D-3f·4D-4). 포획·희생·승리나 공중 3종 전용 행동도 검사하지 않았다. 앱의 미션 실행·입력·화면과 로그인 기준선 뒤 32커밋 판정은 남아 있다.

Node v26.13.0 최종 전체 회귀 6,249건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E753의 v22 검사 뒤 첫 번째 v26 전체 회귀다.
