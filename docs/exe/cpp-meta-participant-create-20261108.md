# 1146 — 실제 메타 참가자 생성과20h 수신 연결

제품1146 / 표시·파일·제품버전11.01이다. 이전 단위는 `f49b3c24 / 20261108-114 CP`다.

4b4710은 현재 메타 모드가 아니면 packet을 읽지 않고 반환한다. CS0 진단이 복귀한 뒤 packet+5 subscriber가 현재 자기와 같을 때만 기존 CoreData.Select(participant BYTE,1)→MetaPending 증가→MetaRefresh 증가를 수행하고 전체42c180을 실행한다. sender/known/phase 관문이나 가짜 ACK/접속 성공을 추가하지 않았다. observer는 원문40a340/version1/policy0/가변ffff이며 송신 nameLenff의534 진단 뒤 현재 BYTE+35h로 WORD 길이를 쓴다.

42b580 생성자는 원문의 비정렬 BYTE/WORD/DWORD 필드 쓰기, 원본packet35h 이름의 기록26 strcpy, 실제 M<signed subscriber> 길드 읽기, 공통 이름 정제, 버전 WORD/언어 BYTE, 현재 Core 장부, 상태 안내, 실제 이미지/객체/이름표와 입장 효과까지 수행한다. 생성자의 길드 접두사는 설정값+`~y:`이며88h가 갱신하는 기록+8/`~a` 접두사와 구별하고+8을 새로 쓰지 않는다.

ConfigInterface.ReadStringBytes는 기존 UTF-8 설정값을 원문 BYTE 경계로 옮기고, 키가 없으면 목적을 읽지 않는다. 키가 있으면 strcmp처럼 첫 다른 BYTE에서 비교를 끝낸 뒤 같은 목적을 NUL까지 복사하고 변경 여부를 반환한다. 목적 버퍼 전체 strlen이나 단순 키 존재로 대체하지 않는다. 첫80BYTE scratch의42ca20이 true일 때 현재 기록의 subscriber를 다시 읽어42ca60의 별도80BYTE scratch 확인을 하고, 그 결과가 true일 때만 실제60BYTE 목적 ReadString을 실행한다. 미작성 지역 바이트는 기존 CurrentAllocationBytes 정책을 사용하며 원문 스택 자체 값의 일치와 구별한다.

48f190은 현재 missionPost/known/자기 전투를 읽고0..39 active/같은 전투 참가자의 subscriber/Core10BYTE를 같은 ClientCoreDataGlobals 장부에 쓴다. ASM의595370+slot*16은 장부 시작595360의slot+1행이다. 따라서 StoreMetadata에도slot+1을 전달하며0번 행으로 잘못 옮기지 않는다. 상태는 marker비0 또는 현재 자기 subscriber일 때만 같은 MessageBoard의5제거→3표시→localflag1→marker1 순서다.

이미지는 기존 Session의14×14/owner0/ignorePosition bitmap→같은 CS ConvertImage crop1→shape NULL 복귀형 진단이다. 실제 World.Create(현재108 TYPE,2)→SID 저장→QA kind→void 진단→raw40 bit0 제거→paintOwner→raw8 WORD0→4adca0의 실제1a DWORD0→현재 좌표 Pop으로 연결했다. 원문의4adca0 본문을 다시 확인했으며 low14만 지우는 연산으로 추정하지 않았다.

이름표는 기존d8 CreateMetaName/4bc060 공장과 selectable1→ReplaceText 빈문자열/0→scrollable0→실제464540 Reference/world2000h→serial/토큰 저장→현재루트 Attach를 사용한다. 일반a4 TextGump/가짜 이름표로 대체하지 않는다. 원본 packet23의bit40이 없을 때만 같은 Kernel/Host의 참가자 Rig 입장 함수를 실행한다. 기록 flags에서는 앞서40을 지웠으므로 그것으로 이 관문을 대체하지 않는다. 현재 자기 subscriber면 실제 IntroCamera(-12→0) 뒤 noShowChalIntro/Tell을 실행한다.

418200은 localflag가 설정된 때 같은 기록26 이름 주소를 받아, 현재 alert의 GetFocus/SetFocus를 먼저 수행한다. 그 뒤 현재 monitor가 비0일 때만 현재 내 Handle과 stricmp로 비교해 같은 이름이면 EVERYONE 문구, 아니면 참가자 이름 문구를 monitorMessage와 지역256BYTE에 서식화한다. monitorFormat/기존 SubmitMonitorChat의 같은 manager/임시 서식 교체 부모를 사용한다. PTR540ca8은 리터럴 EVERYONE이 아닌 현재 내 Handle이며, PathSpec 앞 추가 push는 이후 printf/Chat의 인자임을 ASM으로 확인했다.

42c180은 진입CS/참가자 포인터를 보유한 채 생성한다. 원래 같은 CS의 전투 좌석과Battle+4 ID를 사용하고, 표시 뒤 현재 같은 참가자.a4로 같은 캡처 CS의 버전 범위를 집계한다. 이를 위해 기존 버전 집계를 명시 CS 뷰로도 호출하게 했으며 현재 getter의 범위 진단은 CS 조회보다 앞에 유지했다. 좌표는 캡처 참가자와 기존 NativeMetaSeatPosition의 실제 프레임/포구 계산을 사용한다. 원본packet 이름/채널20h Chat→특정 참가자ff→NSInfo가 마지막이며, 자기 Chat의 조기 반환에서는 이름을 먼저 읽지 않는다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/자료조회/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 4b4710/40a340/42c180/42b580/42ca20/42ca60/48f190/418200/435b00/441300/4adca0 C/PE·현재 필드/캡처 CS·실제 Frame/Session/Config/Gump/Core/Kernel/Chat·slot+1 장부 행·호출/부분 쓰기 순서를 대조했다.

Netstorm.exe 13,037,568바이트/SHA-256 `96ca043d126e91a34426ceffb747cd335de313fc11283acbcbdd13f8e78ceb39` / R.exe 669,696바이트/SHA-256 `0c33f7db5f779058d337bd2febd0f5e5cc3c82a68bddc8f3ab3afae1765b233b`

새 C++/MP/게임/통신 실행 없이 컴파일과 정적 대조만 했다. 실제 참가/장면 안정성을 확인한 것으로 세지 않는다. 마지막 전체 디컴파일은2026-11-03/HJOW-Athlon, R전용은2026-11-07/HJOW-Athlon이며 생성 원문은 변경하지 않았다. 원시 근거/빌드/해시 감사는 Git 제외 `extracted/cpp-resume-1146/`다. 물리 범위 밖/전체 formatter·x87/원문 스택·OS 실패 자체 일치는 별도 경계이며 앞선 부분 효과를 되돌리지 않는다.

다음은 메타 CS 수신21/22/25h·1a/1b checking·하강/참가 후속 카메라·HTML·1095 송신 상태 호출이다. helpWord 캐시 수명과 기존 잔여도 유지하며 전체 목표는 진행 중이다.
