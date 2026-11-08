# 1147 — 실제 메타 참가자 제거와21h 수신 연결

제품1147 / 표시·파일·제품버전11.01이다. 이전 단위는 `eefded86 / 20261108-115 CP`다.

4b46d0은 현재 메타 모드가 아니면 packet을 읽지 않고 반환한다. CS0의State.cpp d04 진단 뒤 전체42c2a0에 현재 CS를 전달한다. observer40a390/version1/policy0/가변ffff는 송신 numPlayersff의586 진단이 복귀한 뒤 현재 BYTE+5로 WORD 길이를 쓴다. 수신 observer는 필드를 읽지 않는다. sender/phase/known 관문을 추가하지 않았다.

42c2a0은 진입CS를 보유한 채 현재 packet+4 BYTE 개수와packet5+index의 참가자 BYTE를 사용한다. 각42bc10 제거 후 현재 개수를 다시 읽으며 current CS가 바뀌어도 진입 기록을 다른 CS로 바꾸지 않는다. 개수가0이어도 마지막47cab0 NSInfo를 실행한다.

42bc10은 참가자1..39 범위를 먼저 진단하고 복귀해도 범위 밖이면 수준9 illegal 로그로 반환한다. 유효 범위에서만 현재 checking을 읽고 활성bit1이 없으면 Ch.cpp4a1을 진단한다. 그 뒤 실제 active를 다시 읽으므로 진단이 기록을 고친 경우 원문처럼 계속 진행한다. 비활성이면 같은 수준9 invalid 로그로 반환한다. 전투가 있으면 캡처 CS의 현재 좌석0→그Battle+4 ID 표시→같은 참가자의 현재 전투로 캡처 CS 버전 범위 갱신 순서다.

445110은 현재 subscriber가 같은 모든 Chat100기록을48BYTE0으로 지운다. 첫 일치에서 멈추거나 subscriber0을 배제하지 않고 추가 Refresh/이벤트도 만들지 않는다. 기존 RawChatChannels의 같은 장기100기록에 Forget을 추가해 연결했다. 기존 ResetOccupied의 비영 기록만 제거하는 동작과 구별한다.

42b980은 현재 flags64 BYTE의a0 비트가 모두0이면419460(아래 방향), 아니면419390(위 방향)의 실제 같은 IslandRig 검색/생성·목표/지연/kill을 실행한다. 같은 Client/Kernel/Host의 기존 Rig가 현재 CS 이미지를 확보한 뒤 Image 두 배열을 해제하고, 기록 전체115DWORD/460BYTE를0으로 쓴다. CS 이미지부터 지워 Rig가 잃게 하거나 World/Gump를 임의로 즉시 파괴하지 않는다. 원문에서 쓰지 않는 추가 인자1을 별도 수명 정책으로 해석하지 않았다. 마지막 기록 초기화는 ShapeSize와 이름표 serial/토큰도 포함한다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/자료조회/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 4b46d0/40a390/42c2a0/42bc10/42b980/445110 C/PE·현재BYTE/진입CS·진단 복귀 뒤 active 재조회·모든 subscriber 일치/0 포함·a0별 실제 Rig 이탈→Imagefree→115DWORD0·동일 Frame/Kernel/Chat 장치를 대조했다.

Netstorm.exe 13,052,416바이트/SHA-256 `0da6fae77419126c1e5717b6bebdbb1096c602f36372a9bb40edad66766292aa` / R.exe 669,696바이트/SHA-256 `bc98702fcd41b24b1b5afa70a490d0eed253c0be57b265a48aa968a41198bc17`

새 C++/MP/게임/통신 실행 없이 컴파일과 정적 대조만 했다. 실제 이탈 장면/안정성을 확인한 것으로 세지 않는다. 마지막 전체 디컴파일은2026-11-03/HJOW-Athlon, R전용은2026-11-07/HJOW-Athlon이며 생성 원문은 변경하지 않았다. 원시 근거/빌드/해시 감사는 Git 제외 `extracted/cpp-resume-1147/`다. 원문 범위 밖 메모리/OS 실패 자체 일치는 별도 경계이며 앞선 부분 효과를 되돌리지 않는다.

다음은 메타 CS 수신22/25h·1a/1b checking·하강/참가 후속 카메라·HTML·1095 송신 상태 호출이다. helpWord 캐시 수명과 기존 잔여도 유지하며 전체 목표는 진행 중이다.
