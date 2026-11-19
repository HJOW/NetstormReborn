# 앱 카메라 Renderer 초기화 (WB04-E898, 2026-11-19)

기준선 `225553df`와 현행 cpppj의 `ClientMain::InitializeNativeRendererView` 및 `CampaignClientServices`를 대조했다. 앱 시작에서 논리 화면 크기가 결정된 뒤, 그리고 같은 Client의 State7 로그인에서 이미 이식된 `ClientCameraGlobals.initializeNativeRenderer`를 호출한다. 매번 현재 논리 화면 크기와 `worldCells`를 읽고 같은 `worldRedraw`·`mapRevision` 상자를 사용한다. 테스트가 별도 상자로 카메라를 선행 초기화하지 않아도 된다.

실제 Tutorial1 `BridgeTheGap`의 State7→8 내부 로그인에서 앱 장치가 1024×768의 카메라 한계 3072×2048, `worldRedraw=1`, `mapRevision=1`을 기록하고 Ready까지 진행했다. 진행 창·팔레트·카메라 이동·소리는 아직 검사 장치다. 독립 검사에서는 원본 카메라 fixture의 초기 행, 4:3·와이드와 세계 크기 변경, revision 감김, 화면 오류를 확인했다. Node 관련 검사 18건·추가 19건과 실제 로그인 1건, Chromium 앱 시작·재시작 1건이 통과했다. 원본 게임 창은 실행하지 않았다.

이 작업은 논리 카메라 초기화만 연결한다. Renderer 자체의 dirty 영역·월드 그리기와 카메라 적용, UI 모드·진행 창·팔레트, 화면 크기 변경 중 월드 처리 및 브라우저 미션 진입은 남아 있다.
