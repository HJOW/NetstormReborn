# 1140 — Challenge 응답과 메타 상태28~33 실제 접합

제품1140 / 표시·파일·제품버전11.01이다. 이전 단위는 `8f3195f2 / 20261108-108 CP`다.

1094에서 라이브러리에만 있던1dh/State28~33 본체를 현재 MetaConnectionRuntime/Editor 장기 분배기에 연결했다. 보호 PE/원문 초기화는1dh→40b380 빈 관찰기→버전1/정책0→4b8ac0 수신 등록이다. 실제 고정13BYTE 응답을 같은 현재 Subscriber와 다섯144BYTE State 블록에 전달한다. 기존 본체의 로그→Subscriber→current 응답/28→버전 거부 시12/0→같은 Run 동기 호출 순서를 사용한다. sender/phase/known 필터·가짜 ACK·next로 바꾸는 우회는 없다. State12는 기존 공통 확정 직행을 유지한다.

RawLocalMissionState의 큰 부모 switch, Uber의 next/parent 분배, MetaConnectionRuntime의 실제 RunConnected 세 경계를 모두 연결했다. 일반20~27/검색34~38은 기존 Run을 사용한다. 새 요청이 없는 메타 next0에서도 원문 Poll의 metaRebuild/metaRefresh/NSInfo 예약을 소비한다. 상태 전환을 합성하지 않으며 기존 TakeAfterPoll의 한 번 처리 위치를 유지한다.

28의 실제 Tell은 현재 Config/미션 조회와 같은 HTML 공장을 공유하고 반환 serial0도 그대로 전달한다. 현재 모달은 실제 노드+2c의 serial을 읽으며 Gump token/등록칸/창 개수로 대신하지 않는다. 29의 transient 안내 닫기, 같은 CS 전체 표시,16h/TCP11 재요청·listener,33의 pending/1..9 닫기·master 조회/주소 재조회·TCP9·1fh 준비/7ch 로그인은 기존 실제 장치와 같은 연결표/owner를 사용한다. 29의 포트 읽기는 PE841c처럼 네 캐시/통지 초기화 전에 캡처하도록 맞췄다. 음수 비영 status·변경 분기의 master=-1 물리 오류 등 기존 원문 본체의 조건은 유지한다.

카메라 초기화는 실제 준비된 메타 Session의 기존 ResetCamera를 사용한다. 현재 ParticipantType(PE541208/기본108), 실제 Renderer+48/+4c 한계값, 현재 화면 크기를 공급하며256칸×16/11 등의 임의 전체 크기로 대신하지 않는다. 같은 로딩 깊이와 WorldRedraw→497f20→실제497960 commit을 실행한다. 원문에서 다른 참가자만 있고 자기 대상은 없는 미초기화 좌표 분기는 기존 명시 호스트 오류를 유지하며 성공 좌표를 합성하지 않는다.

State33에서 생성하는 게임 Peer의 전체 접속/종료 수신은 잔여다. 현재 메타 전송의 type2는 Root10/CS11 종료만 기존 본체에 연결돼 있고, GameServer9 실패/서버 Player 통지는 명시 미연결 오류다. type1도 실제 메타 전송에 아직 등록하지 않았다. 이번 상태 호출 접합을 게임 Peer 수명 전체 완료로 세지 않는다.

005318e8은 보호 PE의 초기6799다. 매번 기본6799로 Config를 다시 읽던 경계를 Client 수명의 단일 DWORD로 바꿨다. 최초/State 옵션 해석과 현재 설정 변경 callback에서 signed ReadInt 임시값을 통해 같은 비트를 쓰고, 누락 시 이전값을 보존한다. 메타29/33·일반 Zone/Serve/listener·Editor·Guide Fort 요청·서버 이전·LAN 대기실 게임 포트가 같은 owner를 읽는다. 별도 LAN 대기실 포트는 유지한다. Fort/Subscriber/설정 복귀 뒤에도 현재 포트를 읽도록 요청 writer에는 사본 대신 참조를 전달한다. 전체435220의 미복원 순서까지 완료로 주장하지 않는다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/자료조회/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 신규 MP 검사를 실행하지 않았으므로 컴파일/정적 접합을 동작/안정성 검증으로 쓰지 않는다.

마지막 전체 정밀 디컴파일2026-11-03/HJOW-Athlon·R전용2026-11-07/HJOW-Athlon, 생성 원문 변경0이다. 근거는10.78이며10.72 전체 동등으로 확대하지 않는다.

다음은 메타 게임 Peer의1/2 접속·종료 부모와 나머지 CS 수신/참가자 생성·카메라 caller·HTML 하강/참가 후속·1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오UI다. helpWord 캐시 수명과 기존 강제 활성 커서/희생/marker 경계는 유지한다. 986 후보는 미적용이고 전체 목표는 진행 중이다.

Netstorm.exe 12,961,792바이트/SHA-256 `b7a916b97eaf13519d261c03935705ad6e742e2e02e4108dde351d0e919e9f5f` / R.exe 669,696바이트/SHA-256 `2317fbc758b5a936da7f2cf2da31039b6be8478c3dd42e30de1c5c2a2a54ba92`다. 보고서는 `cpppj/meta-connected-registration-20261108.json`, 원문/기계어·빌드·보호 기준은 `extracted/cpp-resume-1140/`(Git 제외)다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
