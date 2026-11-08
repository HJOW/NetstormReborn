# 1143 — 메타 기하 배치 수신과 현재 카메라 한계

제품1143 / 표시·파일·제품버전11.01이다. 이전 단위는 `5e4d232f / 20261108-111 CP`다.

26h의4b45a0은 메타 모드/CS NULL/known/송신자/phase 관문 없이 packet+7 BYTE 폭과 packet+8 BYTE 높이+PE00513780 float를 현재 Renderer497f80에 전달한다. 상수 비트는0x411ba2e9, 값은9.727272987365723이다. BYTE 높이와 상수를 더한 중간값은 원문의 x87→DWORD 스택 저장처럼 float로 축소한 뒤 전달한다. 실제 Client.GetScreen의 논리 폭/높이와 같은 GameplayGlobals.Camera.UpdateTargetLimits를 사용한다. FSR 표시 목표 크기나 새로운 카메라/viewport/좌표로 대체하지 않았다.

이후42c780은 같은 장기 GeometryMode를0으로 먼저 쓴 뒤 packet+6 BYTE를 우측2비트 이동해 DWORD 개수를 얻는다. 2개 이하면 modLen/contact를 읽지 않고 반환한다. 3개 이상일 때만 현재 WORD packet+4 modLen+9 뒤 contact 포인터를 계산해 두 원점 DWORD를 순서대로 복사한다. 나머지 중 최대2DWORD를 각각 firstRadius/secondRadius에 쓰고 마지막에 mode1을 쓴다. 정확히3개면 둘째 반경의 stale 값을 보존하며4개를 넘는 contact도 더 읽지 않는다. 최초 좌표 캐시 initialized, ring/grid/width/height와 다른 배치 값은 그대로 둔다. float 값을 계산하는 대신 DWORD 비트 복사에 대응하는 bit_cast를 사용한다.

observer40a3e0은 수신일 때 아무 필드도 읽지 않는다. 송신이면 modLenffff의5eb 진단→현재 contactLenff의5ec 진단→현재 두 필드 재조회→WORD(contactLen+modLen+9) 쓰기 순서다. 진단이 정상 복귀하며 패킷 필드가 바뀌는 경우도 선행 사본을 사용하지 않는다. 원문의 version1/policy0/가변 길이ffff로 실제 메타 공유 분배기에 등록했다.

실제 읽기/쓰기 경계에서 패킷 부족은 호스트 오류로 거부한다. 선행 Renderer 한계, mode0/원점/첫 반경 쓰기는 되돌리지 않는다. x87 모든 제어어/예외/전체 픽셀 일치나 원문 메모리 범위 밖 동작을 보장하지 않는다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/자료조회/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 4b45a0/42c780/40a3e0/40a440/497f80 C/PE·513780 float 비트·mod 뒤 contact 위치·3DWORD의 둘째 반경 보존·mode0/부분 저장/mode1·같은 논리 Screen/카메라 장치를 대조했다.

Netstorm.exe 12,977,664바이트/SHA-256 `2da3ce01f5b56dff33d9200f4eabe9c16b5ccd5c76bc7573c46295a1ed3f03c6` / R.exe 669,696바이트/SHA-256 `0b6295ef7ebde11de5fb82641eafbf3380aef0c28fc0c0f552dab05d34fa7865`

새 C++/MP/게임/통신 실행 없이 컴파일과 정적 대조만 했다. 마지막 전체 디컴파일은2026-11-03/HJOW-Athlon, R전용은2026-11-07/HJOW-Athlon이며 생성 원문은 변경하지 않았다. 원시 근거/빌드/해시 감사는 Git 제외 `extracted/cpp-resume-1143/`다.

다음은 나머지 메타 CS 수신20/21/22/25h·참가자 생성·카메라 caller·HTML 하강/참가 후속·1095 송신 상태 호출이다. 1a/1b checking 부모·helpWord 캐시 수명과 기존 잔여도 유지하며 전체 목표는 진행 중이다.
