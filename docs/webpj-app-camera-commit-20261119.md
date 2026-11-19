# 앱 미션의 즉시 카메라 적용 (WB04-E956)

기준선 `225553df`의 `CampaignClientServices`·`UberGumpCamera`를 따라 Session의 즉시 카메라 요청을 앱의 현재 `Screen`, `Renderer`, Client 카메라, Gump 루트와 현재 World에 연결했다. 기존 497960 본체를 사용하므로 DIB 스크롤과 커서 잠금, 현재 지도 영역, 두 카메라 view, 노출 변경 영역 및 전체 출력 요청의 원래 순서를 유지한다. 현재 Client 카메라와 다른 요청이나 소유자가 빠진 요청은 거부한다. Client Gump의 루트 재그리기는 기존 `GumpFrame`의 실제 표시·삭제·자식 관문과 연결했다.

실제 Screen과 Renderer에서 카메라 이동, DIB 스크롤, 노출 띠, 커서 깊이 복구, 현재·snapshot 일치를 검사했다. Tutorial1 Guide→State7→실제 내부 로그인 Ready 검사에서 가짜 카메라 적용을 제거하고 같은 앱 Renderer의 카메라 다시 그리기 요청을 확인했다. 최초 검사에서 앱 전역 `cursor`가 속성인 점을 함수로 잘못 접근해 실패했으며 수정 후 관련 34건과 Tutorial1 요청 검사 4건이 통과했다.

연결 범위는 즉시 적용이다. Gump의 앱 재그리기는 아직 전체 화면 갱신 예약만 하며 개별 Gump의 Renderer 사각형 합성은 남았다. 카메라 프레임 말미 적용·가장자리 스크롤, 소프트웨어 커서 장치, 월드 합성·Present 및 실제 미션 플레이는 아직 연결되지 않았다. E954 기준 Node v26 전체 회귀는 별도 고정 작업 트리에서 진행 중이며, E955 기준 Node v22 회귀는 그 뒤 실행해야 한다.
