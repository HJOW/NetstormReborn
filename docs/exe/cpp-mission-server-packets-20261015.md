# 실제 Player 송신과 중앙섬 metadata 복원

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. 원본10.78의 `0048f9f0`/`004a6b40`/`004b3bc0`과 실제 등록표를 다시 읽었다. 원본 게임/GUI 실행0이며 보호 원본·AGENTS·dotnetpj를 변경하지 않았다.

`RawMissionServerPackets::SendPlayer`는 같은 Player9 원문의 이름을 실제 `SanitizeMissionPlayerName`으로 정제하고0x2b packet을 작성한다. 현재 Player 번호→이름 정제→길이BYTE/이름→subscriber→접속flags→현재 주소→flags/동맹/순서/예측WORD/부가BYTE 순서다. metamap 진단은 두 번호 조회 위치에서 각각 실행한다. 주소 하위가 현재 Player를 바꾸면 뒤 필드를 다시 읽는다. 전체100BYTE 예약은 원본처럼0xff이며 실제 observer `0040a590`의 **59+nameLen**으로 wire 길이를 확정한다.

metadata는 실제 **0x72**다. descriptor 주소의 간격을 암산한 예비 메모의0x70과 구분하며 원본 등록표 `0040b110`/`004b3bc0`를 확인했다. `$Random`의 NUL 포함8BYTE를 +10에 쓰고 현재 archipelago의 낮은BYTE를+5, 길이DWORD를+6에 쓴다. 아직 중앙섬이 생성되지 않았으면 현재 server를 진단하고 같은 로컬 수신 몸체를 먼저 실행한 뒤 현재 localOwner와 target을 비교한다. 자기 target이면 실제 송신을 생략한다. 다른 target은 원본 observer의 **10+길이WORD=18BYTE**를 송신한다.

미션 Zone의 `0x82000004`에는 중앙섬100비트가 없으므로 `ReceiveMetadata`는 원본의 `No Central Island.` 로그 뒤 자연 반환한다. 100비트가 있어도 이름 첫 글자가 `$`가 아니면 원문대로 생성하지 않는다. `$`로 시작하는 실제 Archipelago 생성은 명시 미복원 오류다. 이를 빈 성공/임의 지형 생성으로 대체하지 않는다.

등록 helper는 실제0x72/version1/정책0과 현재 수신기를 설치한다. 수신 observer 인자0은 자연 반환한다. 현재 공용 observer API에는 packet span이 없어 인자1로 길이를 쓸 수 없으므로 그 호출을 명시 거부한다. 송신 writer는 같은 최종 길이를 실제 packet을 가진 위치에서 작성한다.

[`decomp_mission_server_packets.py`](../../tools/decomp_mission_server_packets.py)는 전체 정밀 함수표의 검토한 writer/Player번호/이름 정제/header/최종길이/metadata 몸체만 실행한다. 주소 객체·서식·네트워크 송신과 CRT 문자 검색/이동/서식은 명시 외부 대체다. 실제 기계어 쓰기는 같은 Player9, 정제 문자표와 한정 스택만 허용한다. 정상 반환/스택/보존 레지스터4개를 검사했다.

새 **187고유입력/정상 반환**을 보존했다. Player는 owner1/8, 빈/금지문자/31글자 이름, 접속flags와 상위DWORD, server/metamap을 교차한다. metadata는 자기/다른/음수target, 중앙섬 생성표시, server진단과 현재 archipelago 하위BYTE를 포함한다. [`MissionServerPacketsTests.cpp`](../../cpppj/tests/MissionServerPacketsTests.cpp)는 실제 전체1,620BYTE Player, 현재 전역, 선언된 모든 wire BYTE와 외부 진단/주소/로그/송신 순서를 대조한다. 내부 이름 정제 진입은 외부 trace에서만 제외하고 전체 Player 결과와 native 진입 통계를 별도 보존한다. 주소/송신을 실제 소켓 실행으로 계산하지 않는다.

최종 Release 경고0·오류0, 독립187행 및 선택된 누락/등록 검사의2개를 포함한 **집중52개 실패0·전체 CTest1/1·내부1,290개 실패0·203.58초**다. 로그는 `extracted/goal-login-packets-final-{build,focused}.log`, `goal-login-packets-{ctest,tests}.log`다. 근거/현재 소스 SHA는 [`mission-server-packets.json`](../../cpppj/mission-server-packets.json)에 보존한다. 실제 로그인 Session 조립, 일반 중앙섬 생성/네트워크/GUI 자연 완주는 후속이다.
