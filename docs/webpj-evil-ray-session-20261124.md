# EvilRay 실제 Session 대조 — WB04-E1314

2026-11-24, HJOW-Athlon. 5-A E198의 실제 Host/Kernel·입자 월드 연결을 확인했다. 제품 로직 변경 없이 기존 구현이 C++ 통합 검사의 기대값을 재현한다.

- 비교 대상: `cpppj/tests/CampaignObjectMenuTests.cpp`의 `EvilRayCurrent_ActualTeleportSelectsConstructorAndItsCurrentTypeSource`, `EvilRayCurrent_ActualVisualFrameNotifyPixelsAndSelfDeletionUseSameHost`, `EvilRayCurrent_ParentZeroRemainsRenderedWithoutKernelAndOwnerPersists`. 현행 검사 파일 커밋 `bb78d4f66`; `EvilRayVisualSystem.cpp`·`RawCampaignParticles.cpp`는 웹 기준선 `225553df` 이후 변경이 없다.
- 새 검사: `webpj/tests/campaign-evil-ray-session.test.mjs`. 실제 `RawCampaignSession`과 TheWarBegins의 보호 자산을 읽는다. 전체 게임·AI Kernel·공격·포획은 실행하지 않고 선택한 표시 효과의 프레임만 호출한다.
- 생성: 실제 Teleport postPop이 선택한 생성자와 현재 EvilRay TYPE을 구분한다. TYPE 58→59·source 변경은 다음 생성에만 반영되고, 같은 Host의 Form·Kernel·표시 등록 수와 50개 입자 초기값이 C++와 같다.
- 표시·반납: 현재 SHP 폭으로 방출 위치를 계산하고 첫 입자 수명 13·위치·음향을 대조한다. 실제 Session 출력의 전체 1024×768 픽셀을 세 타원의 합성과 비교한다. 입력 clip보다 실제 고정 clip을 사용하는 경로, 중복 Stop 진단, 수명 만료와 Form·Kernel·표시 토큰·SID 반납을 확인한다.
- 부모 0: Kernel과 Form 없이 표시 목록에만 등록하며 미사용 source와 셰이프를 조회하지 않고 출력 표면을 보존한다.

검증: Node 26.13.0 집중 3/3 통과(`webpj/extracted/webpj-e1314-focused.log`). 독립 자식 프로세스의 모듈 로드 훅으로 `sharedSource`를 0에 고정하는 변형은 3/3 검출했다(`webpj/extracted/webpj-e1314-mutation.log`). 디스크의 제품 소스는 변형하지 않았다. 초기 검사 준비 과정에서 느슨한 Fort 경로와 필수 음향 장치 누락이 실패해 기존 검사와 같은 TAFF 조회·명시 장치로 고쳤다.

E198은 완료다. E201 Burning과 섬 투하 지지·입자 대조는 별도 단위로 남긴다.
