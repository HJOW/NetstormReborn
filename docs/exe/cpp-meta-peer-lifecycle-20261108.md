# 1141 — 메타 게임 Peer 종료와 공통 장애 저장 연결

제품1141 / 표시·파일·제품버전11.01이다. 이전 단위는 `23b16b54 / 20261108-109 CP`다.

1140에서 실제 State33/TCP9를 연결한 뒤 남긴 type1/2의 전체 호출을 다시 추적했다. 원문 type1은4e2f00의 비음수ID→현재 초기 버퍼→현재 가상+4 송신→현재 버퍼free/0이다. 실제 LocalTcpTransport 생성자는 이미 RegisterConnectPacketReceiver를 호출하고 같은 Client Registry의 SendConnectPacket을 등록한다. 기존 수신기가 있으면 유지하며 Meta도 같은 등록표/Registry를 빌린다. 별도 MetaConnectionRuntime.Register(1)이 없다는 사실만으로 미등록이라 적은1140 판단을 정정했다. 중복 등록·가짜 접속 ACK를 추가하지 않았다.

type2의 기존 ReceiveConnectionEnded는 같은 동기화 token 비교/0쓰기, 비서버 sender9 장애 저장 후 현재 server 재조회, 실제 Disconnect, 서버1..8의 flags 세 store→접속자별 방송→활동 프레임0, Root10/phase23의 다음20을 선택한다. 메타가 공급하던 두 명시 미연결 throw를 현재 실제 Session의 SaveServerFailure/BroadcastConnectionPlayer로 연결했다. 별도 World·Player·접속성공·상태 전환을 만들지 않는다.

서버 장애의8행 flags8 대상 정리와 같은004b54e0 저장을 메타에서도 실행하도록 writer 몸체를 Login에서 Session 공통 함수로 옮겼다. 일반 Login은 기존 파일 장치로 같은 함수에 위임한다. 실제 source/ticks는 옛 Login 참조가 빌리던 같은 ClientLocalStateGlobals.SourceType/GameTicks getter다. 이동 전23b16b54 본문과 이동 후 본문을 객체 한정자/명시 alias/주석/공백만 정규화해 토큰·순서 동일을 확인했다. SP 비교/진단값/잔여 횟수와 현재 잔액/Player/복구 슬롯·loginEnd를 바꾸지 않았다. 이 검사는 소스 바이트 대조이며 C++/MP 함수를 실행하지 않는다.

MakeCampaignClientServices가 이미 만든 같은 사용자 홈 FILE 장치를 일반 Login과 비로그인 메타 서비스가 공유한다. 메타 준비가 localLogin을 제거해도 이 장치는 남고, 실제 파일 열기는 원문 writer의 missionPost/known 자연 관문과 진단 뒤에만 수행한다. 메타에 게임 Login/타이머/수신기를 생성하지 않는다. 일반/Editor의 기존 미설치 오류와 자연 반환도 유지한다. 저장 실패의 앞선 flags/부분 효과를 되돌리지 않는다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/자료조회/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 최초 빌드는 Session 완전 정의 include 누락의4진단으로 종료1이었다. 실제 헤더 추가 뒤 최종 빌드0이며 두 로그를 보존했다. 실제 장애 파일 생성·종료/재접속·MP 동작을 확인했다고 쓰지 않는다.

마지막 전체 정밀 디컴파일2026-11-03/HJOW-Athlon·R전용2026-11-07/HJOW-Athlon, 생성 원문 변경0이다. 근거는10.78이며10.72 전체 동등으로 확대하지 않는다.

다음은 나머지 메타 CS 수신/참가자 생성·카메라 caller·HTML 하강/참가 후속·1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오UI다. helpWord 캐시 수명과 기존 강제 활성 커서/희생/marker 경계는 유지한다. 986 후보는 미적용이고 전체 목표는 진행 중이다.

Netstorm.exe 12,962,304바이트/SHA-256 `298f544b154dcf8a6c8814749bf2bcf1be0c173b0a614a0db5b8e7b7ebc7872e` / R.exe 669,696바이트/SHA-256 `c4c01a48def6f21c19fcf2deb5fa9c54b8730cbd85cb858599049c0bfaf841b0`다. 보고서는 `cpppj/meta-peer-lifecycle-20261108.json`, 원문/기계어·빌드·보호 기준은 `extracted/cpp-resume-1141/`(Git 제외)다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
