# 1144 — 메타 참가자 접속/이탈 메시지 준비

제품1144 / 표시·파일·제품버전11.01이다. 이전 단위는 `80abc346 / 20261108-112 CP`다.

20h 참가자 생성42b580과21h 제거42bc10을 연결하기 전에 필요한 공통004d0600의 종류3/5를 복원했다. 최초에는 현재 메타 플래그에 따라48f480 또는42bbb0 이름 주소를 빌린다. 주소를 문자열 사본으로 먼저 바꾸지 않고 번역/음향 뒤 서식 시점에 같은 필드의 첫 NUL까지 읽는다.

종류3은 최초 이름 선택 뒤 현재 메타 플래그를 다시 읽는다. 메타면 `~E~w%s is present`를 번역/서식/Style로 처리한다. 메타가 아니면 현재00594fbc가 비영이고 현재 owner1..8일 때만 원문의8색 이름/접속 문구를 처리한다. 종류5는 공통 이름 조회 뒤 newWarnMessage.wav(0,0,0,0,0)를 재생하고 현재 owner1..8의 연결 해제 문구를 서식화한다. 종류5에는 두 번째 메타 모드 관문이 없다. 두 종류의4번 색은g이며 지연 안내 종류2의u와 구별했다. 무전투/범위 밖 owner로 Style이 생략되더라도 두 종류 모두 공통10초 기한→Idle→Fit(0,0)을 실행한다. 종류3은 경고음을 재생하지 않는다.

Uber의 두 메타 이름 조회 미연결 오류를 같은 ClientChallengeGlobals.ParticipantNameBytes의 현재 기록40BYTE/범위 진단/active 또는 빈 NUL 주소에 연결했다. 모드도 GameWorld가 있는지로 추정하지 않고 현재 SurfaceStorage.ownerMode.challenge를 읽으며 준비된 MetaMap의 실제 Session과 같은 전역을 사용한다. 일반 이름 조회의 기존 경로는 유지했다. WarningMessages의 같은20칸/현재 root 검색/파생 TextGump/수명/StyleText가 새 두 종류도 생성하도록 연결했다.

SetNativeMetaParticipantStatus는 매 호출 현재 UI의 실제 WarningMessages에 kind/participant/parameter0/빈 문자열로 생성 또는 제거를 위임한다. 별도 serial 표나 대체 팝업/가짜 성공을 만들지 않았다. 이 진입점은 다음20h/21h 연결을 위한 준비이며 이번 단위에서 해당 패킷이 이미 등록됐다고 주장하지 않는다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/자료조회/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 4d0600/4d3090/42bbb0 C/PE·최초 이름 주소/현재 메타 재조회·일반 전투/1..8 색 분기·5만 음향·공통10초/Idle/Fit·같은 실제 CS/Gump/20칸 표를 대조했다.

Netstorm.exe 12,982,784바이트/SHA-256 `92a354f4e008f38e605f97189b622c444c4e9c0e0738d96f41f1ff02162d6505` / R.exe 669,696바이트/SHA-256 `0d5f900969e5b197b482d4980cfb6e0e3bb8333a87bcfcb244d51a902b350349`

최초 빌드의 비공개 UI 직접 참조3진단을 기존 친구 Frame 위임으로 고친 뒤 최종 빌드를 확인했다. 새 C++/MP/게임/통신 실행 없이 컴파일과 정적 대조만 했다. 마지막 전체 디컴파일은2026-11-03/HJOW-Athlon, R전용은2026-11-07/HJOW-Athlon이며 생성 원문은 변경하지 않았다. 원시 근거/빌드/해시 감사는 Git 제외 `extracted/cpp-resume-1144/`다.

다음은 나머지 메타 CS 수신20/21/22/25h·참가자 생성·카메라 caller·HTML 하강/참가 후속·1095 송신 상태 호출이다. 1a/1b checking 부모·helpWord 캐시 수명과 기존 잔여도 유지하며 전체 목표는 진행 중이다.
