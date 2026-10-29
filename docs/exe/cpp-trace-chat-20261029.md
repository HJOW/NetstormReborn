# 진단 로그의 실제 Chat 전달·문맥·스크롤 — 단위565

2026-10-29 / HJOW-Athlon / Windows10 19045. AGENTS.md·공통/DN 인계를 다시 읽고564의 `fc046cf3` 뒤 시작했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 이번에 산출물을 바꾸지 않았다. 실제 창 실행 확인 예외 호스트다.

기존 `Client::DiagnosticLogHooks()`의 비영 traceToChat 명시 오류를 실제 공통 Chat UI 전달로 교체했다. 원문434f40의 현재 관문/색→42f190의1024BYTE 서식→현재 Chat 포인터→42eca0의 로그 추가를 연결했다. 기본 관문은 원문처럼0이며 새로운 옵션이나 자동 활성화를 추가하지 않았다.

## 원문과 보존한 동작

- 434f40은 관문0이면 색·문자열·Chat 포인터를 읽지 않는다. 비영이면 현재005acd24로 `{0,1,color,0,0,0,1,1}`의32BYTE 지역 문맥을 만든다. 추가 가변인자는 전달하지 않는다.
- 42f190은 현재 Chat 포인터를 읽기 전에 서식을 처리한다. 이번 실제 진단 경로는 일반 문자·`%%`와 첫NUL 의미를 지원한다. `%s`처럼 제공되지 않은 추가 스택 인자를 요구하거나1024BYTE를 넘는 원문의 미정의 읽기/쓰기는 **명시 거부**한다. 일반 가변인자 Console 전체를 복원했다는 뜻이 아니다.
- 42eca0의 실제 명령을 대조했다. **자식+80의8DWORD를 저장하지만 스타일 지정과 정상 복원은 부모+80에 쓴다.** 자식 스타일을 임의로 바꿔 전달 색이 적용되도록 “수정”하지 않았다. NULL 스타일에서도 마지막에 부모 문맥이 진입 당시 자식 문맥으로 바뀌는 원문을 보존했다.
- atBottom은 진입 당시 자식을 사용한다. 그 뒤 parse와 bottom 요청은 매번 부모+a4의 **현재 자식**을 다시 읽는다. 하위가 바꾼 자식/문맥과 alias, 정상 복귀의 저장 문맥 복원 및 중간 중단의 부분효과를 보존했다.
- 실제 RawMenuControls/ChatWindow의 같은 ScrollingTextGump·StyleTextBuffer·markup·requestBottom을 사용한다. 별도 표시 로그나 임시 스타일 저장소를 만들지 않았다. 부모는 Client의 공유 로그/현재 포인터 수명과 같은 Uber UI에서 호출한다.

구현은 [TraceChat](../../cpppj/src/client/TraceChat.cpp), [ChatWindow](../../cpppj/src/client/ChatWindow.cpp), 실제 `RawMenuChat.inc`·`RawMenuControls`·`UberGump`·`ClientDiagnostics`에 있다. 화면 초기화 전의 색/Chat 전역0도 같은 Client에서 읽으며 현재 포인터가 있는데 실제 UI 소유자가 없으면 명시 거부한다.

## 독립 관측과 집중 검사

[독립 x86 도구](../../tools/decomp_trace_chat.py)는434f40/42f190/42eca0의 원문 분기·문맥 복사·현재 포인터 재읽기를 **37입력**으로 실행했다. 관문0/비영/상위비트·NULL Chat·NULL/명시/부모/자식 문맥 alias·0/1/상위비트 bottom·현재 자식/문맥 변경·3개 중단을 포함한다. 정상34/명시 중단3이다. UI의 atBottom/parse/requestBottom은 명시 장치이며 formatter는 정의된 인자 없는 서식을 처리하는 호스트 CRT 장치다. 최초 관측과 `--verify`의 차이는0이고, 원본 게임/OS/GUI/네트워크 실행을 뜻하지 않는다.

C++은 같은37입력의 부모/두 자식32BYTE·최종 자식·전체 호출/문자열을 대조했다. 별도 실제 공통 UI 검사는0관문의 무효과, 비영 전달의 공유 로그 증가·문맥 복원·맨 아래가 아닌 스크롤 유지/맨 아래 자동 요청·현재 포인터0을 확인했다. 실제 Client의 초기 NULL Chat/서식 순서도 검사했다.

같은 최종 Release 검사 실행파일에서 TraceChat3·ChatWindow9·ClientDiagnostics4·TraceLog4·ObjectDump4를 실행했다. 중복1을 제외한 **고유23개/실패0**이며 실행24개, 등록3431이다. 최초 빌드의 검사 스크롤 float→int 경고2를 정수 입력으로 교정했고 최초 로그를 보존했다. 최종 게임/검사 빌드는 종료0·경고/오류0이다. 기존 두 파일의 줄바꿈 확대도 원래 LF로 되돌렸으며 제품 의미를 바꾸지 않았다. 전체 C++ 검사는 실행하지 않았다.

## 실제 게임과 한계

최종 같은 게임으로 별도 데이터 사본에서 TheWarBegins 진입→실제 Play Mission→Ctrl Shift F7→짧은 진행을1회 실행해 종료0했다. 실제게임 시각은3.640→7.312초(3.672초 증가), Native phase14/미션 시작1을 유지했다. `squids.txt`71060BYTE와 `procSumm.txt`1972BYTE를 새로 생성했다. 원시3관측의 현재 trace level/depth/suppress/indent는 모두0이며 덤프 뒤 원래 상태로 돌아왔다. 실제 화면도 확인했다.

이 GUI 실행은 원본 기본 **traceToChat0**이다. 비영 전달은 창 없는 실제 공통 UI 조립 검사로 확인했으며, GUI에서 비영 관문을 임의 조작한 것으로 기록하지 않는다. 원본 게임·자연 완주·특정 적 공격·포획/희생·새 네트워크 검사는 하지 않았다. 현재565제품의 전체34경로·Windows11 실기도 미실행이다. 마지막34/34는560제품/561단위, 새 설치/배포 런처3회는563제품의 별도 근거다.

최종 게임 SHA256은 `601190571370eeca0781b2d17a27445d702b9faae281ff061d7a76928379a724`다. 기존 원본/CD·AGENTS·fixture **2541파일(기존fixture721)**은 동일하고 새 fixture1개를 추가했다. GUI/빌드 프로세스는 모두 종료했다. 원시는 `extracted/cpp-trace-chat-565-20261029/`에 보존하며 [독립 관측](../../cpppj/trace-chat-native-20261029.json)·[집계](../../cpppj/trace-chat-runtime-20261029.json)를 따른다.

다음은 메타맵 번개00479650 등 남은 UI/Edit·시나리오·수명·호환성이다. 자연 완주/사용자 개입·스펠 도움말 그림 추가 분석·연녹색 직사각형·영문 부재 개선 보류, 공중3종 정적 제한과 신규 MP/통신 검사 금지는 유지한다.
