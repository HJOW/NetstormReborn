# 1081 — CCh 관리 명령과 현재 예약

83h(00476490→00456b30)의 전체관리명령을현재CCh/소켓·전투/라우터와같은메인예약상태에연결했다. 제품1081/11.01,EXE 12,366,336바이트,SHA-256 `5423fc54ac6d91f2905ac43df6f34a07c6a6d479942e15380387f4e1a1b5c370`다. 직전CP는 `455bf786 / 20261107-145 CP`다. **새R.exe 최종owner/실제Bind·Install·Run은아직없으며관리/예약실행검사는하지않았다.**

## 등록과 전처리

원문00442950은83h의receiver만등록한다.00482b60의version/정책초기값0을유지하며observer/버전1등록을추가하지않았다. 고정176BYTE·nullable수신을기존제어관문에연결했고CHAL진단1349/signed존·현재토큰을보존한다.

00456df0은지역키9BYTE를복사하지만사용하지않고30회0..9위상만계산한다. 입력을암호화/복호화하는내용을추측으로추가하지않았다. 그뒤현재clientAdmin과입력+16의C문자열을대소문자구분으로비교한다. 빈현재값도같은비교를하며별도관리자/권한검사를추가하지않는다.

## 실제 명령

| 번호 | 원문 인자와 현재 처리 |
| --- | --- |
| 0 | p8 subscriber 첫일치의실제레코드id/현재연결에서하위socket만닫음 |
| 3 | signed p8을double초로변환해같은reset예약호출 |
| 4 | p12전투/p8지정참가자재배치 |
| 5 | p8전투전체재배치 |
| 6 | p174/175목적·자리→p166/170좌표를1eh로작성/p8실제이동 |
| 7/8 | p46문구+LF의일반/확장Root41h안내 |
| 9 | 발신참가자+75=1 |
| 10 | p8현재StartBattle |
| 그외 | 원문처럼부작용없이반환 |

명령0은노드kill/flush/Terminate나즉시명단제거를하지않는다. 새CloseSocket→SocketDevice.CloseNative는현재IPv4 open bit40/유효핸들,선택bit400의Sleep500,실제closesocket/INVALID,WSAGetLastError,bit40해제순서다. 기존RAII Close는바꾸지않았다. 현재장치가구현하지않은Zlocal bit8은명시오류다.

## 전체 안내와 로그인 차단

474e80은기존474d30과byte5=81만다른확장41h다. 같은BroadcastBuffer의extended선택으로결합했다. 83h7/8의지역124BYTE는원문strcpy/strcatLF후 **정확히122BYTE**를전달한다. strlen으로다시줄이거나NUL뒤후미를0으로정하지않고호스트할당입력으로보존한다. 원래스택값자체동일성은주장하지않는다.

같은현재51표/패킷으로CCh에직접전달하며앞수신/observer가바꾼내용도다음CCh에보인다. SetLoginBlocked는00474fd0의10h8BYTE/요청DWORD를같은현재표에보내고이미연결된10h수신이실제CCh카운터를쓴다.

## 같은 메인 예약 상태

00479090은scheduledAt가0이아니면새요청을무시한다. 분로그는ftol64(seconds/60)의lowDWORD,즉시관문은별도의ftol64(seconds)lowDWORD==0이다. 지연단위가분이라는로그문구만보고인자를분으로바꾸지않았다.

즉시는NOW방송→reset/stop증가만수행하고예약/차단/마지막안내필드를쓰지않는다. 지연은현재game+seconds→goingDown원값→로그인차단1→현재game의lastWarning→분안내방송/log29순서다. **기존MainLoop.PollSchedule이이같은MainLoopState를소비한다.**

Routing.Bind는CCh의Root릴레이/일반·확장안내를같은라우터에묶는다. BindSchedule는CCh생성전에같은장기ClockState/MainLoopState/라우터/log를빌린다. MainLoop객체의this를먼저요구하지않아CCh와메인객체의생성순서순환을피하면서실제예약함수를호출한다. 최종R owner에서이Bind들을실제호출해야한다.

## 확인과 잔여

Release/x64 NetstormCpp빌드종료0·경고/오류0이다. 새단위/MP/게임/통신·소켓열기/새close·관리/예약/원본R실행검사0,전체검사/34미션/ZIP반복0이다. 보호2800파일불변·새fixture0이다. 컴파일/원문·PE·기계어정적접합을실행검증으로계산하지않는다.

실제필드/참가자/전투·124BYTE문자열지역밖은명시오류다. 구형스택/힙·SEH/x87전체일치와실제장치닫힘/WSA오류/대기·예약/후속노드종료는미검증이다. 기존LAN정책/폐기검증비활성은유지한다.

원문18수신중11구현/폐기80하나제외·**6수신(0b/0c/11/12/66/87)**이남는다. 다음은87h명령·진단/LAN수신과보조객체/전체477e10/478af0·새R.exe최종owner/실제Bind·Install·Run이다. 다른LAN hostname/0F존47·3BYTE/0F·1d·메타/OSlaunch·나머지LAN/Outpost/checking5·한국어후속/Edit시나리오UI도남아전체목표미완료다.

원시근거/주요몸체전체기계어·정적감사·빌드로그는Git제외 `extracted/cpp-resume-1081/`,추적보고서는 `cpppj/root-admin-command-20261107.json`이다. 마지막전체정밀디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며같은호스트기존산출물만읽었다. 재디컴파일/생성소스변경0이다.
