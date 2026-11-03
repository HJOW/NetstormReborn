# 일반 입력의 세 고정 송금 checking

단위830의 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며 직전829는 `586cc711 / 20261104-11 CP`다. 현재 제품830/11.01이며 checking17=부모11+부분3+MP분류3이다. 완료부모31/완료부분3이다.

기존 함수별 인계의004d62b0 Guide/Console checking 세 지점이라는 분류는 잘못됐다. 정밀 C와 실제004d62b0~004da650 기계어의005e4794 참조는 고정1000/700/300 송금 AI 가지 세 곳이다. 셋 모두 현재 자금 허용→대상 Player+50의0x20→현재 checking 비영/missionPost0→진단→로컬 SP 차감→저장 대상/금액의 AI 지갑 가산 순서다. 연결 Player/패킷 가지는 검증하지 않는다.

| 금액/명령 | checking 읽기 | 진단 CALL | 실제 파일/줄 |
| --- | --- | --- | --- |
| 1000 /020000ec |004d859d|004d85be|`.\Userinput.cpp`,0x8c2(2242)|
| 700 /02000108 |004d86ec|004d870d|`.\Userinput.cpp`,0x8e9(2281)|
| 300 /02000120 |004d882c|004d884d|`.\Userinput.cpp`,0x913(2323)|

진단 식은 모두00515318의 `inSingle`이다. 고정 파일은00519090의 `.\Userinput.cpp`이며829 숫자창의 `.\Ui.cpp:0x61c`와 다르다. 같은 함수의 나머지7진단은 별도 무조건·메뉴·특수키 관문이며 세 checking 참조가 아니다. checking 범위를 닫더라도 전체 일반 입력/Console·다른 예외/키/미복원 UI를 완료했다고 쓰지 않는다.

829가 세 경로의 같은 Session checking 상수1을 현재 Client DWORD로 고쳤다.830에서는 SendMoneyHooks 끝에 위치별 assertionAt을 추가하고 숫자/세 고정 명령이 실제 원문 파일/줄을 전달하도록 연결한다. 기존 독립 assertion은 위치별 장치가 없는 caller의 fallback이며 All은 기존 무진단이다. 실제 Session은 위치별 보고를 같은 services.ai.assertion으로 보내고 누락 예외도 선택된 원문 위치로 기록한다. 기본 서비스의 전체 원문 진단 UI는 별도 호스트 경계다.

실제 caller는 UberGump 일반 명령의 IsSendMoneyCommand→DispatchSendMoneyInput→같은 Session.MoneyInputHooks다. release는 World 대여보다 먼저 반환하며 기존 Guide/Gump 소비와 Console/정지의 원문 순서를 바꾸지 않는다. 기존547의888행 fixture는 읽기만 하고 AI flags0x20/0x22의312행만 새 집중 검사로 선택한다. 원관측 backend는 **기존 Unicorn**이며830에서 새 CPU 원문312행을 관측한 것이 아니다. 선택 자료의 진단행은1개다. 연결 Player/패킷 가지는 장치에 도달하면 실패하도록 두며 기존888 전체 관측기/MP 검사를 다시 실행하지 않는다.

같은 실제 Client/Session 집중 검사는829의 실제 AI 설정/생성·숫자700 두 번/고정300에 고정700/1000/300의 현재 checking80000000/ffffffff/1을 추가한다. 각 실제 진단이 checking을0으로 바꾼 뒤에도 같은 SP/AI 지갑이 갱신되고 숫자/고정의 원문 파일/줄이 보존되는지 확인한다. 게임/AI/Kernel/공격/포획/희생/승리·GUI/통신 프레임은 실행하지 않는다. 원시는 `extracted/cpp-resume-830/`이며 최종 빌드(NetstormCpp/file/scenario money/HTML) 종료0·경고/오류0,집중5개(새1/기존 Session 확장) 실패0이다. 최초 빌드는 새 검사 주석의 줄바꿈 누락으로 packets 선언이 가려져C2065 3건/종료1이었고 해당 검사 줄바꿈만 교정했다. 최초 로그/기존 fixture를 보존했다. 기존2756개/fixture936개 변경0·새 fixture0이다. EXE SHA-256은 `f8ba3dde280a1ad533359ab70b7c31c145224ae27dc0bf98c6069e6a5fc76f01`다. [830 JSON](../../cpppj/userinput-money-checking-20261104.json)에 보호/근거 해시를 기록했다. 다음831은004b8cb0의 실제 서버 전환/reloadFile caller와 비멀티 도달 범위를 먼저 대조한다. 새 MP 검사는 계속 제외한다.
