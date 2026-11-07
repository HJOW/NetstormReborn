# 1113 — PlayerInfo 전체 입력과 실제 슬롯 송신

제품1113 / 표시·파일·제품버전11.01이다.1112는`4d408c76 / 20261108-47 CP`로 커밋됐다. 원문0047a900 전체148줄과 다음47afa0까지의 기계어를 읽고 준비/시작·좌우 슬롯 변경·추방 확인을 같은 실제 파생 노드/CS·메뉴·두 송신 장치에 연결했다.

실제 ButtonGumpState의 현재 clickEvent를 읽는다. 자기 ready serial은 헤더→현재subscriber→현재button value/CS flags 조회→2비트 변경/실제CS저장→송신이다. start serial은 reserved0에서만1fh/12h를 보내고 송신 복귀 뒤 reserved1로 쓰며 다음 요청을 막는다. Native 송신 실패에도 이미 수행한 쓰기를 되돌리지 않는다.

텍스트 serial과 참가자+100/200/300/400을 구별한다. 왼쪽은 랭크 감소·특수·색 감소·추방, 오른쪽은 특수·색 증가·랭크 증가다.1107의 실제 다섯 슬롯 writer를 같은 BattleOptions에 적용한다. 색/랭크는 헤더 전에, 특수는 헤더/초기payload 뒤에 변경하고 자기 특수 값만 실제594fc4에 쓴다. 현재 CS subscriber/flags|4000h의1fh를 보낸 뒤 같은 전투 키와 현재87BYTE를1108의67h로 보낸다.4000h flags는 실제CS에 재저장하지 않는다.

추방은 실제 Fancy controlled 스택·현재 이름/번역·256BYTE 제목·Yes/No를 사용한다. command5429dc와 직접 참가자번호/0을 주며 Current→현재루트→원시 화면 좌표→일반 Attach 순서다. 기존 메뉴 강제 Close나 두 번째 부착은 추가하지 않았다.루트의 실제 추방 처리는1112를 공유한다.

PlayerInfo 원문에 없는 별도 메타/known·추가 권한 관문이나 전체 참가자 준비 판정을 넣지 않았다. 태그 분기는 serial/값이 맞지 않아도1을 반환하고 나머지는0이다. 실제 전체 함수의 분기/헤더 전후/포인터 재조회/부분 쓰기와 기계어를 정적 대조했다.

NetstormCpp·NetstormRoot Release/x64 빌드 종료0·경고/오류0·보호2800파일 불변이다.새fixture/C++/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 MP 검사는 사용자 지시로 제외한다.**BattleInfo 전체 입력/옵션·제외 목록 writer**와 CS 수신/생성·카메라Rig/HTML→1dh/28~33등록/1095송신 상태호출은 후속이다.세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어후속→Edit시나리오UI도 남아 전체 목표는 미완료다.

Netstorm.exe 12,620,800바이트/SHA-256`606b361b0301fa9c9837ed8d8a629db21f78b3396fedf35364214b150b254c33`, R.exe 669,696바이트/SHA-256`21a4c6f62eb34afc31863cafff97484842f1ffadaa89c6785db7f9e6d61a9e8f`다.보고서`cpppj/nsinfo-player-input-20261108.json`, 원문/기계어·빌드 기록`extracted/cpp-resume-1113/`는Git제외다.마지막 전체 정밀 디컴파일**2026-11-03/HJOW-Athlon**, R전용**2026-11-07/HJOW-Athlon**이며 재생성/생성소스 변경0이다.AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
