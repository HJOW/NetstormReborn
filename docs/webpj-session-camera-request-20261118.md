# WB04-E775 Session 카메라 요청과 적용 경계

cpppj `RawCampaignSession.cpp`의 `RequestCameraCenter`, `CommitRequestedCamera`, `CameraGlobals`를 같은 Session과 Client의 카메라·MetaRebuild·WorldRedraw 전역에 연결했다. 34h 수신도 공개 요청 경로를 사용한다. 요청은 목표 픽셀과 재구성 번호만 바꾸며 현재 Renderer와 snapshot은 보존한다. 적용은 호출자가 제공한 실제 497960 장치에 같은 카메라와 다시 그리기 상자 및 논리 크기를 전달하고, 장치가 없으면 명시적으로 실패한다.

TheWarBegins 실제 Session에서 목표 (1284, 879), 재구성 번호의 DWORD 감김, 적용 전 현재 화면 보존, 장치 호출과 명시 적용, 반납 뒤 접근 및 미연결 장치 오류를 검증했다. 기존 34h 수신 검사도 통과했다. 검사 장치는 현재 픽셀을 직접 적용하는 독립 관찰 장치이므로 실제 GUI의 커서·dirty·표시 장치 전체가 연결되었다는 뜻은 아니다. ResetCamera·Home·사제 조회·입자 뷰 및 GameWorld/브라우저 적용은 남는다. 제한된 미션 행동과 세 공중 유닛의 전용 행동은 실행하지 않았다.

인수인계 0.3절 집계에서 E769의 ClientChatPacketReceiver 상태 변경이 빠져 있어 대응표·`tools/port_inventory.py`의 실제 partial 40 / todo 109와 42줄 이동으로 정정했다. 남은 단위 총수와 줄 수는 변하지 않았다.

Node v26.13.0 전체 회귀 6,278건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E771의 Node v22 전체 회귀 뒤 네 번째 v26 회귀다.
