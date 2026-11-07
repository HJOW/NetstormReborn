# 1090 — R 프로세스 로그와 복귀형 ZAssertion

최종 R owner가 사용할004800e0/4800b0·485cf0/485e42의 실제 로그/진단 장치를 작성했다. 제품1090/11.01, EXE 12,366,336바이트, SHA-256 `3112bf2e1edcf511ace683a55a156bd86c40f732b988134733ae699757a3e84c`다. 직전 CP는 `9971c3cd / 20261107-161 CP`다. **새R.exe 최종owner/entry·이 장치의 실제hook 연결/서버 실행은 아직 없다.** 최종 owner에 빈 로그나 일률적 throw만 연결하면 원문 동작이 달라지므로 해당 프로세스 장치를 먼저 복원했다.

## 현재 로그와 출력

ProcessTrace는 같은 ConnectionState/Options 참조와 UserDataRoot를 보관한다. 생성은BSS0의1024BYTE 버퍼/NULL FILE/현재 들여쓰기0/진단guard0·NULL 추가정보만 구성하며 OS 시각/파일·창을 실행하지 않는다. LogDevice/AssertionDevice는 이 같은 장기 객체를 대여하므로 실제 모든 소비자보다 오래 살아야 한다.

로그 관문은signed0<=level<=현재traceLevel이다. 관문 실패는 버퍼/시각/FILE을 건드리지 않는다. 허용하면 첫NUL 전 문자열의 연속 선두 ~두BYTE 코드를 먼저 보관하고 현재signed 들여쓰기가 양수일 때 그 수만큼 공백을 넣는다. 뒤에 이미 완성된 메시지를 복사한다. 원문 가변인자를 다시 해석해%문자열을 실행하지 않는다. 현재버퍼의 모든 후미CR/LF를0으로 만들고 하나라도 있었으면 PE004be634의LF 한 개만 복원한다. 나머지 물리 tail은 지우지 않는다.

첫 허용 로그에서만 현재 FILE이 stdout이 된다. 원문 모드 접두는diag/root/chal 소문자이며 각 조건은11h/2/4다. 빈 접두도 `%s %s %s `의 공백으로 남는다. 실제_time32→localtime→asctime 앞19BYTE/': '가 이어지고, 현재traceToStdErr가 비영이면 **본문만 stderr에 복제**한 뒤 현재 stdout에 본문을 쓴다. 현재flushAfterTrace가 비영이면 fflush한다. Flush는 이미 현재FILE이 있을 때만 flush하며 새 출력대상을 설치하지 않는다. 게임Clock으로 이 시각을 대신하지 않는다.

## ZAssertion의 실제 복귀

485cf0은 현재 진단guard가0이면1→stderr진단→assert.html w+b/무버퍼·원문HTML/표현식·닫기→현재로그flush→반환주소 수집→현재추가정보/대화상자→guard0/추가정보 반납 순서다. 이미 진단 중이면 추가정보만 반납하고 복귀한다. 정상 대화상자 복귀 뒤 caller가 계속 실행하는 계약을 유지한다. 파일/변환 등 호스트 예외가 발생하면 이미 수행한 앞선 효과와 guard1을 임의로 정상화하지 않는다.

**rootServerThrowExceptionOnAssert를 이 경로에서 예외 정책으로 사용하지 않는다.** 보호 R.text에서 해당 전역004cd958의 직접 참조는477623/4779aa 두 곳이고 모두 옵션 읽기 인자로 주소를push한다.485cf0/485e42에는 그 값을 읽는 관문이 없다. 원문 함수 이름/옵션명만 보고 모든 진단을 throw하는 구현으로 대신하지 않았다.

보고서는 설치/CWD 대신 사용자Netstorm/Diagnostics/assert.html에 같은 Unicode w+b로 쓴다. 스택은 원문의 최대20개/네 개마다CRLF·번호/대문자16진수 형식을 유지하되 실제 Windows CaptureStackBackTrace의 현재 반환주소/포인터 전체 폭을 사용한다. 구형 EBP 체인을64비트에서 잘못 따라가거나 원본 주소를 합성하지 않는다. MessageBoxW의 경고48/정상복귀 순서를 유지한다. wire10.71과 별개로 사용자 요구에 따라 새 진단의 제품표시는11.01이다.

원문485e42에는 bugs@titanic.com으로 정보를 보내 달라는 표시문구가 있다. 이 자체는 접속/메일 발송이 아니다. 현재 문구는 사용자 진단파일 위치로 교체했으며 외부메일/HTTP/DNS/공식 검증을 추가하지 않았다. 외부서비스 목록에도 이 원문문구와 변경을 기록했다.

## 호스트 경계와 다음 owner 접합

구형 vsprintf/1024BYTE 넘침·미완성 ~코드/잘못된 UTF-8/NULL FILE은 명시 오류다. 원문 가변인자스택/입력포인터가 현재버퍼와 같은 특수 별칭 ABI,32비트EBP/SEH·힙/원문 스택 주소 전체 동일성을 주장하지 않는다. 실제시각/CRT·FILE실패/예외·재진입/guard잔류/메시지pump와 전체 수명은 실행 미검증이다. 자동 쓰기는 사용자 홈에만 둔다.

다음 최종 owner는 같은 Configuration.Values() 참조로 ProcessTrace를 만들고 LogDevice/AssertionDevice를 실제Config/시계·전송/파일/CCh/Root/메인 장치에 연결해야 한다. Configuration 생성은 IO/로그/진단을 호출하지 않으므로 필요하면 생성 순서에서 forward capture를 쓴 뒤, 실행 전 동일 ProcessTrace가 실제로 구성됐는지 보장한다. 별도Options 사본이나 fallback 빈 장치를 만들지 않는다. 그 뒤 현재1089 Lifecycle.Bind/MainLoop.Run을 새R.exe entry에서 실제 호출한다. 클라이언트OSlaunch/배포·세.server콘텐츠/배포·LAN hostname/0F존47·3BYTE/0F·1d/메타·LAN/Outpost/checking5·한국어후속/Edit시나리오UI도 남아 전체목표는 미완료다.

Release/x64 NetstormCpp 빌드 종료0·경고/오류0이다. 새단위/MP/게임/통신/소켓/로그·진단/보고서쓰기·대화상자/원본R 실행0,전체검사/34미션/ZIP 반복0이다. 마지막 실행검사는1052 기존7개며 이번에 반복하지 않았다. 보호2800파일 불변·새fixture0이다. 동일호스트 원문/보호PE·전체 함수범위 기계어와 현재 소스만 정적으로 대조했다.

원시/기계어·옵션참조/정적 감사/빌드로그는Git제외 `extracted/cpp-resume-1090/`,추적보고서는 `cpppj/root-process-trace-20261107.json`이다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**,R전용 **2026-11-07/HJOW-Athlon**이며 같은 호스트 기존 산출물만 읽었다. 재디컴파일/생성소스 변경0이다.
