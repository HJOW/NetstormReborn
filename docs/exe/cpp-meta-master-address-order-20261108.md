# 1158 — M3.3a 참가 master 주소와 TCP 재사용 순서

2026-11-08 / HJOW-Athlon. 직전 감사1157/커밋809ef16a, 제품1156에서1158로 변경. 표시·파일·제품 버전11.01.

State33의 ConnectMaster는 원래 주소의 NUL을 먼저 찾고 없으면 거부했다. 원문4b6dd0은 현재 CS 참가자+1a0의 주소만 넘기고,4e2620은 현재9의 준비 값이 비영이면 주소 해석4e0cd0보다 먼저 성공 반환한다. 기존 준비가 음수인 경우도 같은 비영 조건이다. 따라서 앞단 NUL 읽기는 현재 원문 분기보다 빨랐다.

이제 같은 현재 포트/참가자 기록을 읽고 주소 필드의44BYTE 경계만 ConnectPeerText에 넘긴다. 주소 사본/사전 NUL 검사/새 접속 상태를 만들지 않는다. 실제 같은 TCP는 ReuseConnect를 먼저 실행하고, 새 접속이 필요할 때만 NUL/프로토콜/주소를 해석한다. State33의 master 변경 분기와 기존 master의 재접속 분기가 같은 helper를 사용한다. 현재 CS가 없는 물리 오류를 가짜 성공으로 바꾸지 않는다.

원문 C/PE와 실제 MetaConnectionRuntime→TCP 연결, 읽기 순서/같은 pointer·44BYTE 경계를 정적으로 대조했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0, 보호2800파일 불변이다. 새 C++ 단위/MP/패킷/게임/통신/전체/ZIP 실행0이며 실제 접속 검증은 하지 않았다. 보조 C/ASM·빌드는 Git 제외 `extracted/cpp-resume-1158/`에 있다.

Netstorm.exe 13,169,664바이트/SHA-256 `ab184250379d8db2a42f0b40d434c42ed1b3463313e25e312e188914f97314e0` / R.exe 670,208바이트/SHA-256 `c526c36cb3e2425f31a952304b614a9de0d5a9ba6dde0b9db2f10f6f3f03b729`

M3.3은 아직 미완료다. 다음 M3.3b는 State19 준비·Descend/참가의 World/카메라/HTML 후속을 마감한다. UberMeta의 CS->listsInitialized와 FortCoreDataLoadContext 진단 두 곳은 현재 clog만 쓰므로 원문 식/파일/줄을 확인해 같은 Client AssertDisplay에 연결할 구체적 후속이다. checking 목록의 두 부모를 다시 미완료로 세는 것은 아니다.3차 잔여는M3~M8의6묶음이다.

마지막 전체 디컴파일2026-11-03/HJOW-Athlon,R전용2026-11-07/동일 호스트이며 생성 원문은 수정하지 않았다.
