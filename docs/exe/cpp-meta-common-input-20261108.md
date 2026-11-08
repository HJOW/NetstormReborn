# 1138 — 메타 ESC·공통 키와 실제 게임 메뉴 호출 연결

제품1138 / 표시·파일·제품버전11.01이다. 이전 단위는 `bcf8a663 / 20261108-106 CP`다.

현재 실제 입력 Session 조회기를 추가해 GameWorld 또는 준비된 MetaMap을 매 호출 시점에 빌린다. 준비된 MetaMap에서만 기존 메뉴 SP 회전·추적 참조·모드/Player 바인더의 설치 관문을 확장했다. loaded/known/Native State·접속 성공을 쓰거나 별도 Player/SP/난수/메뉴 월드를 만들지 않는다. 같은 장기 SP 검산 주소와 실제 메뉴 serial을 사용하며 기존 cleanup에서 메뉴 상태를 Session보다 먼저 반납한다.

메타 ESC는 일반 정지 뒤의 마우스 처리로 보내지 않는다. 같은 Select(0,0,0)의 enabled→현재 Guide demo→다중 정리→기존 선택의 프로세스/가상 통지→강조0을 먼저 실행한다. 실제 커서 깊이0이면 Native Cancel→공통 popup Kill만 실행하고 아니면 현재 serial 조회→메뉴 생성 또는 Kill→serial0→공통 popup Kill 순서를 유지한다. enabled0/demo는 선택 정리만 막으며 후속 커서/메뉴 판정을 막지 않는다.

원문4cc210의 같은 RawGameMenuBar/RawGameMenus를 메타에 연결했다. Game/View/Options/About·조건부 하위와 SP 회전/검산+3은 기존 몸체다. 폭/위치는005ca99c를 공유한 현재 MapViewRect.left에서 읽고 폭 적용 뒤 다시 조회한다. 메타 Players/fancy Player List는 기존 원문처럼 작성 전에0을 반환한다. 메타 전용 Game의 Main Menu/View의 Refresh Zones 행을 새 임시 메뉴로 바꾸지 않았다. Refresh Zones(c6)의 실제 소비자는 후속 잔여다.

공통 Gump/모달 뒤 메타의 키도 원문 커서 옵션→활성 커서→생산 단축→선택 키→마우스 순서를 사용한다. H/F4/F5/F6는 같은 일반 명령 큐, N/Ctrl+F5는 같은 NextUnitSelection, P/R는 실제 사제 참여 조회/현재 selected·장기 priestSelections 카운터를 사용한다. 카메라는 같은 Session의433ba0/4950d0→433aa0→실제497960 commit이며 별도 임시 위치나 복제된 World를 만들지 않는다. 각 메뉴/명령의 known/metamap/정지 관문은 유지하므로 메타 H/ViewHome이 임의로 홈 이동을 하지는 않는다.

F9/Shift+F9는 메타에서도 원문 Player List/Pause 명령을 큐 뒤에 예약한 뒤 각 소비자의 모드 관문에 맡긴다. View 메뉴의 Refresh Zones(c6)와 F9의 실제 f0을 혼동하지 않는다. 공통 F6 경로에서는 건설 기록 키 함수가 false를 반환해도 무조건 return하던 누락을 고쳐 일반 F6가 하위 ShowTechnology 예약까지 진행하도록 했다. Ctrl/special/정지/수정키 거부는 기존 각 원문 부모가 맡는다. 이는 캠페인의 일반 F6에도 적용되는 실제 caller 교정이다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/자료조회/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 컴파일/정적 이식을 실제 멀티플레이 검증으로 쓰지 않는다.

마지막 전체 정밀 디컴파일2026-11-03/HJOW-Athlon·R전용2026-11-07/HJOW-Athlon, 생성 원문 변경0이다. 이 근거는10.78이며10.72 전체 동등으로 확대하지 않는다.

다음은 메타 Refresh Zones(c6)·나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오UI다. BattleDesc HTML 주소·helpWord 캐시 수명과 기존 강제 활성 커서/희생/marker 경계도 유지한다. 986 후보는 미적용이고 전체 목표는 진행 중이다.

Netstorm.exe 12,933,120바이트/SHA-256 `fabc5568b05ec2768ffc04a055ce08efd8bdad6f3518f36e2242fd73fb4d20c4` / R.exe 669,696바이트/SHA-256 `9078d237d386bfb38fc27218455cf8f343c9e45a91ce824885a52507c6b93b6e`다. 보고서는 `cpppj/meta-common-input-20261108.json`, 원문/기계어·빌드·보호 기준은 `extracted/cpp-resume-1138/`(Git 제외)다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
