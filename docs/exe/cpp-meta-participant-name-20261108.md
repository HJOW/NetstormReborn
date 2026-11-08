# 1142 — 메타 참가자 길드 표시와 이름표 갱신

제품1142 / 표시·파일·제품버전11.01이다. 이전 단위는 `4b78690d / 20261108-110 CP`다.

88h 수신4b4680은 메타 모드/phase/known/송신자 관문 없이 현재 CS에서 packet+4 subscriber와 첫 일치하는 참가자를 찾는다. subscriber0도 배제하지 않는다. 같은 기록의 flags+64 bit1이 꺼져 있으면 입력 이름과 표시 장치를 읽지 않는다. active면 packet+8의 첫 NUL까지 같은 기록+8에 BYTE별로 복사하고42b490→47d8c0(기록+0,ff)→47cab0 순서로 진행한다. +8은 길드 표시 필드이며 +26의 정제된 참가자 이름과 다르다. 길드를 참가자 이름에 덮어쓰거나 새 Chat Add를 합성하지 않았다.

42b490은 +8 문자열을 보호 PE의 `~a`/`:`로 감싸고 빈 길드면 접두사를 비운다. flags800의 `~[IfortGump.L1]` 경고를 선택한 뒤 실제 번역→Core+4 rank+1 캡처→기존0042b210 기술 합계→현재+26 이름 순서로 서식화해 같은 기록+d8에 쓴다. 서식은 `~j%s%s~w%s\n~yLevel ~w%d ~yRank ~w%d\n`이며 원문 ASM의42b522 push가42b210의 ret0 뒤에도 남는 rank 인자를 확인했다. Ghidra가 누락한 다섯 번째 printf 인자까지 전달한다. 생성20h가 다른 길드 접두사를 사용할 수 있도록 공통 서식 자식과42b490 부모를 구분했다.

등록은 원문 빈 observer0040b380/version1/policy0/PE0052ec40의 고정98BYTE를 사용한다. Client/ClientRendererFrame에 지정 참가자/마스크를 전달하는 얇은 진입점을 추가해 기존 NativeMetaDisplay.UpdateMetaParticipant와 같은 CS/Pool/Gump/카메라/Renderer를 빌린다. 전체 Poll로 대체하지 않으므로 다른 참가자·존·카메라의 추가 갱신을 만들지 않는다. TYPE/기술 합계는 기존 Runtime.Weight/RawPlayerNotification의 같은 실제 TYPE 표를 사용한다.

미발견 NULL 역참조, 입력 NUL 부재/실제 CS 물리 초과는 명시 호스트 오류로 거부한다. 원문 범위 밖 메모리 훼손/스택 배치/전체 formatter 자체 일치를 확정하지 않는다. 앞선 길드 복사 등의 부분 효과는 되돌리지 않는다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/자료조회/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 4b4680/42b490/42b210 C/PE·고정98BYTE·별도+8 길드/+26 이름·다섯 printf 인자·기존 TYPE 합계·특정 참가자 Frame 장치의 호출 순서를 대조했다.

Netstorm.exe 12,971,520바이트/SHA-256 `1566d88f4e25c94fa9fc8617a4c2437dc81ba83f05c4553f73b56b805949e4a0` / R.exe 669,696바이트/SHA-256 `278e81b31926b49facef649feb056dcd478377da0065eb8813c7a0886647458d`

최초 빌드는 OriginalText.h 선언 include 누락으로4진단이 발생해 헤더를 추가한 뒤 최종 컴파일을 확인했다. 새 C++/MP/게임/네트워크 실행 없이 컴파일과 정적 비교만 했다. 마지막 전체 디컴파일은2026-11-03/HJOW-Athlon, R전용은2026-11-07/HJOW-Athlon이며 생성 원문은 변경하지 않았다. 원시 근거/빌드/해시 대조는 Git 제외 `extracted/cpp-resume-1142/`다.

다음은 나머지 메타 CS 수신20/21/22/25/26h·참가자 생성·카메라 caller·HTML 하강/참가 후속·1095 송신 상태 호출이다. 특히20h는 CS 필드만 저장하면 끝나는 처리가 아니며 실제 섬 이미지/World 객체/이름 Gump/Rig/자기 Core 선택까지 남아 있다. 1a/1b checking 부모, helpWord 캐시 수명과 다른 기존 잔여를 완료 처리하지 않았다. 전체 목표는 진행 중이다.
