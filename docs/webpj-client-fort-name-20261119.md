# Client Fort 이름과 제목 장치 (WB04-E885)

단독 미션에서 사용하는 `FortCurrentNameDevices` 경로에 실제 Client의 `ClientFortGlobals` 이름·dirty, `CampaignTemplateStorage`의 헤더·manager, `GameData` 경로 설정을 연결했다. 앱은 현재 Guide와 Fort 이름을 `refreshWindowTitle`에 전달해 실제 문서 제목을 갱신한다. 선택된 Snapshot·Notify 효과는 외부 실제 장치가 제공해야 하며, 웹 범위 밖 Fort 파일 삭제는 명시적으로 거부한다.

실제 앱 Client의 Tutorial1에서 `BridgeTheGap` 이름과 원본 설정 기반 미션 제목을 확인했다. 읽기 전용 실제 Template manager에 새 경로를 쓸 때 같은 manager와 선택된 Notify·Snapshot 순서도 확인했다. Chromium에서 이름과 제목을 별도로 대조했다. 아직 Guide Init의 Fort 요청과 실제 Snapshot·Notify 수명은 연결하지 않았다. E884의 원본 관측 나머지 836행도 후속이다.

Node v26 집중·연관 검사 81건과 Chromium 2건이 통과했다. 게임 창은 실행하지 않았다.

정해진 주기에 따라 Node v26.13.0 전체 회귀 6,434건이 실패·취소·건너뜀 없이 통과했다.
