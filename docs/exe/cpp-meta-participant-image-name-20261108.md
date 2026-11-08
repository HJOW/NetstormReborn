# 1145 — CS 참가자 이미지 변환과 공통 이름 정제

제품1145 / 표시·파일·제품버전11.01이다. 이전 단위는 `8d45566d / 20261108-113 CP`다.

20h 생성42b580이 요구하는46d790 Image 변환을 현재 ClientChallengeGlobals의 같은 가변 할당 owner에 준비했다. 원문은 기존 imageBits가 비영이면 Image.cpp40의 복귀형 진단을 내지만 선행 Destroy/Clear를 호출하지 않는다. ConvertImage도 같은20BYTE 헤더에서 이 순서를 보존한다. 입력 bitmap 토큰/원래 폭/높이를 공개하고 기존 RawImage.Convert의 같은 crop/RLE 알고리즘으로 변환한 뒤 shapeSize→실제 shape 할당/포인터→DWORD/잔여BYTE 복사→crop 폭/높이→현재 bitmap 반납→bitmap 포인터0 순서로 진행한다. 기존 shape/bitmap의 소유 토큰을 먼저 해제하거나 미작성 shapeSize를 임의로0으로 바꾸지 않는다. 기존 비영 포인터가 덮이는 비정상 재생성의 미반납 배열도 현재 owner 수명까지 남는다.

14×14/owner0/ignorePosition의 실제 지형 bitmap은 기존 Session.BuildEditorIslandChoiceImage에 있다. 이번 단위는 생성42b580의 이 bitmap 호출/전체20h 등록까지 완료한 것이 아니라, 그 다음 실제 CS Image 저장 하위를 준비한 것이다. host vector와 실제 토큰 배열 사이의 보관 복사/추가 할당은32비트 포인터를 자르지 않는 기존 이식 경계다. 원문의 OS 고갈 발생 시점이나 모든 미정/훼손 메모리 자체 일치를 보장하지 않는다. 오류 때 이미 공개된 header/할당의 부분 효과를 되돌리지 않는다.

42b0e0 이름 정제는 기존 RawChatChannels.NameChanged에 들어 있던 실제 금지문자/NUL 제자리 삭제 본체를 공통 SanitizePlayerName으로 분리했다. 금지문자 삭제 시 같은 index 재검사, 현재 NUL까지 memmove, 뒤 stale BYTE 보존, 빈 이름의 Cheat<subscriber 하위BYTE> 결과를 유지한다. 기존 삭제 루프부터 NUL 쓰기까지의 정규화 토큰이 이동 전과 같음을 정적으로 비교했다. Chat은 동일한 NameChanged 관찰 후 원래40BYTE를 전달하며 Add의+27 NUL/flags/두 Refresh 순서도 유지한다. CS 생성은+26부터 남은 실제 기록 물리를 전달할 수 있어 원문 strcpy 뒤 임의40BYTE 잘라내기를 추가하지 않는다. 물리 NUL 부재/빈 이름 대체 공간 부족은 명시 호스트 오류다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/자료조회/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 46d790/46e5a0/42b580/42b0e0 C/PE·선행 Clear 없음·bitmap/shape/header/크기/free 순서·같은 RawImage 변환·기존 정제 삭제 루프 토큰/Chat 관찰·BYTE 대체를 대조했다.

Netstorm.exe 12,982,784바이트/SHA-256 `56f4a49117d4a8cc13f012b6699e13c6101ad4e74fe19190bcea09f96dfc2e85` / R.exe 669,696바이트/SHA-256 `4824c99d07d4d625ead541f5a7c7ef292e8f418e9411171ddb2029f2609f2d5f`

새 C++/MP/게임/통신 실행 없이 컴파일과 정적 대조만 했다. 마지막 전체 디컴파일은2026-11-03/HJOW-Athlon, R전용은2026-11-07/HJOW-Athlon이며 생성 원문은 변경하지 않았다. 원시 근거/빌드/해시 감사는 Git 제외 `extracted/cpp-resume-1145/`다.

다음은 나머지 메타 CS 수신20/21/22/25h·참가자 생성의 실제 World/Gump/Rig/입장 알림 연결·카메라 caller·HTML 하강/참가 후속·1095 송신 상태 호출이다. 1a/1b checking 부모·helpWord 캐시 수명과 기존 잔여도 유지하며 전체 목표는 진행 중이다.
