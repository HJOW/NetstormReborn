# WB04-E861 Client trace와 console 출력

기준선 `225553df`의 `ClientTraceGlobals`, `Client::WriteDiagnosticLog`와 `Client::TraceObject` 경계를 이식했다. 같은 장기 로그 상태·재귀 깊이·덤프 억제·Chat 관문을 사용하고 기존 `RawTraceLog`, 객체 가상 진단·종속 재귀와 `TraceChat`을 연결한다. 웹의 FILE 출력은 console 출력으로 적응하고 선택된 stderr 미러와 Chat 콜백 순서는 보존한다. SID 0은 Client 장치를 읽지 않는다. 기준선 이후 해당 C++ 파일 변경은 없다.

집중 검사에서 레벨 필터·들여쓰기·console/Chat 순서와 미러를 확인하고 실제 TheWarBegins Session의 사제에서 같은 Pool·TYPE의 Base/Main trace와 재귀 깊이 복원을 확인했다. 집중 검사 2건과 Node v26.13.0 전체 회귀 6,380건이 통과했다(실패·취소·건너뜀 0).

실제 브라우저 Client 수명과 Chat UI, 공통 오류/예외 처리, 건설 기록·Kernel 요약/덤프와 원본 파일 장치의 웹 정책은 남아 있다. 원본 게임 창은 실행하지 않았다.
