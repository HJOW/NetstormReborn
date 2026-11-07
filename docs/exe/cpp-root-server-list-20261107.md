# 1076 — rootservers 목록과 현재 체크섬

`root::ServerList`가 실제 문자열 표/체크섬·행 파싱/갱신과 rootservers.dat 읽기/사용자 홈 저장을 복원하고 메인 루프의 재로드에 연결한다. 제품 기준1076 / 11.01, EXE 12,366,336바이트, SHA-256 `f88b99a3ffaff34dd1a21de8427fdd4f57826f25525e685fb7a4cbf40387a57a`다. 직전 CP는 `b62f8f9a / 20261107-130 CP`다. **새R.exe 최종 owner/실제 Reload·Save·Run 호출은 아직 없다.** 컴파일·정적 대조를 파일/서버 실행 검증으로 계산하지 않는다.

## 실제 표와 체크섬

CRT004ba320→004715a0은 flags7(줄끝1/대소문자2/중복4),count0/capacity20/checksum0으로 현재004d0e7c 표를 만든다. Add는 중복 검사보다 먼저20→30→45처럼 논리 capacity를1.5배 늘린다. 같은 순서/첫 표기를 유지하고 전체 조회는 flag2일 때_stricmp다. 부분 조회는 **저장 행만 소문자로 바꾸고 query는 그대로** strstr하므로 query까지 자동 소문자로 바꾸지 않는다. Clear는 count/체크섬만0이며 flags와capacity는 보존한다.

체크섬은 각 비NULL C문자열의 **unsigned 바이트+1 합**이다. NUL/별도 행 구분자를 넣거나 CRC/정렬을 적용하지 않는다. RootDirectory의 현재004d0e8c에 해당하는 DWORD를 참조로 빌린다. 제공 R00475660의 별도 빈 localvector를 이 표의 내용으로 임의 교체하지 않는다.0F의47/3BYTE 호환 문제와 최종 수신은 여전히 후속이다.

## 실제 줄 읽기와 저장 경로

00471ec0은 설정의 DataPrefix와 별도로 고정 '..'/'d'를 같은 RawFileName 고리에 넣는다. 새 장치는 **사용자 Saves/rootservers.dat 우선,그 외 실제 workingDirectory 기준 ..\d\rootservers.dat**를 읽는다. 찾은 뒤 현재 표를 비우고 binary rb로 읽는다. 누락이면 빈 표/체크섬0이며 선택된 파일 열기 실패는 예외다.

00449680은 첫256BYTE만 보관하지만 실제 줄 끝까지 소비한다.257BYTE0 지역의 CR/LF를 flag1일 때 모두0으로 만든 뒤 첫NUL까지 비어 있지 않은 행만 추가한다. **개행 없는 마지막 줄은 마지막 글자가 한 번 더 복사된다.**00449700→00448ff0의 fread1이 EOF 때 이전byte를 덮지 않고00448e90은 실패한 읽기 뒤에야 feof가 되는 원문을 보존했다. ferror의 원문 무한 반복은 호스트 명시 오류로 끝낸다.

저장은 현재 순서의 첫NUL까지 binary w+b로 쓰고 LF가 없는 행에만 LF 하나를 덧붙인다.5차 요구에 따라 실제 생성 파일은 **Saved Games/Netstorm/Saves/rootservers.dat**다. 설치/원본 경로로 쓰지 않으며 기존 파일이 없어도 사용자 홈에서 생성한다. 이 정책은 원문 existing-probe의 NULL 경로와 구별한 호환 변경이다. 파일 내용은 원문 바이트이며 읽기/행 파싱은 DNS/소켓을 호출하지 않는다.

## 세미콜론 행과 현재 값

00471fa0의 고정 `([^;]*);([^;]*);?(.*)?`를 앞 두 문자열/정수로 파싱한다. 선택 출력은 먼저0으로 만들고 지정 버퍼-1만 캡처하며 정수는 최대79BYTE의 atoi다. 실제80BYTE 지역/79BYTE 출력 요청 경로의 두 문자열 최댓값78BYTE도 보존한다. 유효한 현재 행에서만 Decode하므로 범위 밖 호출은 caller 출력을 그대로 둔다.

Upsert는 첫 문자열을 소문자로 바꿔 현재 전체 행에서 부분 검색한다. 신규 행은 추가 후 체크섬을 계산하지만 **기존 행 교체는 체크섬을 갱신하지 않는다.** preserveValue 선택과 이 stale 체크섬을 원문처럼 유지한다. SetValue의 false는47220d의push0/문자열getter/47221b의Upsert와add esp8 기계어로 확인했다. 별도 문자열 owner를 사용해 Add 확장 중 기존 문자열 주소는 보존하지만 원문 Remove의 해제 누락/32비트 힙 배치는 재현하지 않는다.

## 확인과 다음 작업

첫 Release 빌드 종료0/C4244 관측,채움 값을 char NUL로 지정한 최종 Release 종료0·경고/오류0이다. 보호2800파일 불변·새fixture0이다. **새 단위/MP/게임/통신·소켓/DNS·새 표 읽기/쓰기·원본R 실행 검사는0**이며 전체 검사/34미션/ZIP도 반복하지 않았다. 모든 구형 CRT locale/atoi 넘침·실제 IO/힙·SEH 실패 동일성을 검증한 것은 아니다.

다음은 보조 객체·나머지 Root/CS/41h 수신·전체 초기화/정리,새R.exe 최종 owner/entry가 같은 checksum/표/기존 장치·Run/Reload를 실제로 연결하는 작업이다. 이어 다른 LAN hostname/0F존47·3BYTE/0F·1d 수신·메타/OS launch를 연결한다. 전체 목표 미완료다. 원문/PE·기계어·정적 감사/빌드 로그는 Git 제외 `extracted/cpp-resume-1076/`, 추적 보고서는 `cpppj/root-server-list-20261107.json`이다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이며 같은 호스트 기존 산출물만 읽었다. 재디컴파일/기존 산출물 변경0이다.
