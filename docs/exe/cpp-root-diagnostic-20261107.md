# 1083 — R 진단 수신과 역방향 TCP

`RootControl`에11h/12h 실제 수신과00474920 연결 부모를 추가하고, 기존 전송/소켓에 실제 LAN 송신 TCP 경로를 연결했다. 제품1083/11.01, EXE 12,366,336바이트, SHA-256 `abcb20ac7ad3d202bfe1ebfc00be124cf4a0fe5639a88d7b36560b14b1955873`다. 직전 CP는 `c43cb95c / 20261107-148 CP`다. **새R.exe 최종owner/실제 Install·Connect·Run 호출과 LAN 동작 검증은 아직 없다.**

## 원문 수신

등록은11/12의 원문444060/4440f0 빈 observer·버전1/정책0·길이12와 nullable 실제 두 수신이다. 두 몸체를 하나의 연결 조회로 합치지 않았다.

00475260은 exit0→diag flags11h 관문 뒤 초기 code==2를 저장한다.443ff0은11h/버전1/길이12 헤더만 쓰며, 그뒤 요청 code DWORD를 복사한다. body+8은 쓰지 않으므로 현재 CRT allocation 입력을 남긴다. 원본 스택 바이트 자체의 동일성을 뜻하지 않는다. 기본 주소는 기존 Parse의0.0.0.0/port0이다.

code2는 pending 접근/응답/닫힘 없이 로그만 쓴다. 그외는4754e0의 **실제 하위 pending 토큰**과 주소를 빌린다. 요청port 비영과 diagRecv가 함께 켜져 있으면 역방향 TCP 연결을 연 뒤 같은 pending 노드로11h 응답·flush한다. 정상/예외 복귀에서475520처럼 **같은 노드의 현재sender로 ClosePending**한다. 마지막 로그는 현재 요청 필드를 다시 읽는다.

기계어에서59560은 인자를 받는 port setter가 아니라 주소+4 getter다. 그 전에 push한 요청port DWORD가 sprintf의 signed `%d` 인자로 남는다. 따라서 실제 주소 BYTE4..7/요청port로 `tcp,주소:포트,cb`를 만들며, 기존 parser에서 포트가 낮은 WORD로 절삭된다. cb는 client4/nonblocking1, TCP10과 합쳐15h다. 연결 버퍼는512다. native Connect는 실패 뒤에도 할당번호를 반환하므로 callback failed 로그도 반환0일 때만 쓴다.

00475580은 diagRecv→상위 ReadTransceiver→실제 connection 동일성 진단830→비영이면11h code1/같은노드 flush→현재주소 로그다. 이 몸체에는 exitRequested 관문이 없으며 임의로 추가하지 않았다. 유효 음수에서 상위조회0인 특이 관문도 유지했다. 응답 body+8은 여기서도 미작성이다.

11h 최종 로그는 초기 code2에 따라 udp, 그외 tcp 라벨을 쓴다. Format(1,0,0)이 이미 포트를 포함하는데 별도 `:port`를 또 붙이는 원문 서식도 유지했다.

## 실제 연결과 장치

00474920은 mode&13h 진단213→같은 Clock의 실제 CaptureGame 시작→현재1..255 첫빈번호→주소Parse/실제 socket 생성→현재 시각·주소/버퍼 생성→표 공개 순서다. open/노드kill 관문 실패이면 원문 로그→Close→표0 뒤에도 선택번호를 반환한다. 공간이 없을 때만9로그/0을 반환한다. 마지막에 별도 CaptureGame 종료와39레벨 시간 로그를 쓴다. 현재 wall 정수나 가짜 경과시간을 사용하지 않는다.

소켓은004875c0의 IPv4/client TCP 범위를 구현했다. 실제 WSA2.2/socket→선택 TCP_NODELAY→선택 FIONBIO1→connect를 수행한다. nonblocking의 **실제 WSAEWOULDBLOCK**만 열린 연결 시작으로 유지한다. 생성 실패는 마지막 오류·무효 핸들을 남기고 수신/송신용 bit2000을 추가하지 않는다. 기존 Status/Receive/Send/QueryPeerAddress와 같은 노드/번호/깊이·닫힘 수명을 사용한다.

실제 connect 전에 loopback/RFC1918/link-local만 허용한다. 외부 IP/폐기 공식 DNS를 조회하거나 연결하지 않는다. IPX/SPX/Zlocal/UDP를 TCP 성공으로 가장하지 않으며 선택한 장치 범위 밖은 명시 오류다. 실제 callback 대상도 같은 요청자의 LAN 주소다.

## 확인과 잔여

Release/x64 NetstormCpp 빌드 종료0·경고/오류0이다. 새단위/MP/게임/통신·소켓열기/새진단/원본R 실행검사0, 전체검사/34미션/ZIP 반복0이다. 보호2800파일 불변·새fixture0이다. 주요 원문/PE·전체 기계어와 현재 장치 접합만 정적으로 대조했다.

callback 뒤 재접근/비동기 실제 실패·수명 전체, WSA1.1/x86 SEH·구형 힙/스택/미작성 지역 물리 자체 일치는 검증하지 않았다. 실제 동작/안정성 검증 완료로 계산하지 않는다.

원문18수신 중16구현·폐기80 비활성 유지·**87h 하나**가 남는다. 다음은87 명령 전체 몸체, 보조객체·전체 패킷 등록/477e10초기화·478af0정리와 새R.exe 최종owner/실제 UDP번호발행·Announce/Bind/Install/Run이다. RootControl에는 같은 장기 Clock/실제 로그도 넘겨야 한다. 다른 LAN hostname/0F존47·3BYTE/0F·1d·메타/OSlaunch·나머지LAN/Outpost/checking5·한국어후속/Edit시나리오UI도 남아 전체 목표는 미완료다.

원시/기계어·정적 감사/빌드 로그는Git제외 `extracted/cpp-resume-1083/`, 추적 보고서는 `cpppj/root-diagnostic-20261107.json`이다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 같은 호스트의 기존 산출물만 읽었다. 재디컴파일/생성소스 변경0이다.
