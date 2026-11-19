# 앱 미션 Renderer와 화면 장치 (WB04-E954)

기준선 `225553df`의 `CampaignClientServices`·`UberGump` 경계에 맞춰 앱의 논리 Screen과 같은 크기의 실제 `Renderer`를 만들고, `SquidRenderer`를 통해 미션 표면 변경을 같은 변경 영역 표에 전달한다. 미션 카메라 패킷은 도킹 영역이 아닌 현재 논리 Screen의 전체 크기를 조회한다. 화면 크기가 바뀌면 Screen과 Renderer를 함께 교체하며 기존 미션 콜백은 매번 현재 장치를 다시 읽는다. 전체 갱신 대기 여부와 부분 갱신 플래그는 기존 Renderer의 규칙을 사용한다.

실제 Screen 크기 변경 시 팔레트 보존과 Renderer 교체·같은 크기의 재사용을 확인했다. 실제 Renderer의 전체 갱신 관문, 부분 사각형과 플래그, 교체 뒤 이전 표와의 독립성 및 실제 Tutorial1 로그인까지 검사했다. 로그인 검사에서는 기존 가짜 표면 표시 장치를 제거했다. 기존 `CampaignGameWorld`의 보관된 viewport는 설치 시 크기이므로 활성 미션 중 크기 변경을 월드에 반영하는 작업도 남았다. Renderer의 월드 합성·Present와 실제 카메라 적용, 소리·입력 소비는 아직 연결되지 않았으므로 이 단위만으로 미션 화면을 표시하거나 플레이할 수는 없다.

검증: Node v26에서 앱 표시 장치·팔레트·요청 프레임 관련 집중 검사 14건이 통과했다. E950 기준 격리 Node v26 전체 회귀는 `extracted/webpj-e950-node26-isolated-corrected.log`에서 실행 중이다.
