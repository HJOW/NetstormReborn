# 1087 — R 원격 명령 보조 장치

004d23f8의 실제 원격 명령 TCP서버/4세션과 메인 통신 hook을 작성했다. 제품1087/11.01, EXE 12,366,336바이트, SHA-256 `61684a804bb9d8efeef9810a2336043c10ce2c07bc2393038a3bb9e27dbda8ba`다. 직전 CP는 `43488d47 / 20261107-154 CP`다. **새R.exe 최종owner/실제Bind·초기화·Run 호출은 아직 없다.** 원격 포트/통신/세션을 실행해 검증하지 않았다.

## 역할과 원문 기본 상태

**479bb0은 생성자가 아니라 활성 조회**다. listener!=NULL과 하위flags40을 읽는다. 실제 기본생성은7ee90/전역CRT ba540,소멸은7ef90/bbac0이다. RemoteCommandServer는 실제listener/handler NULL과4개 고정세션을 만들며 IO를 수행하지 않는다.

현재 R 원문의 전역리터럴참조14곳과477e10 시작에서는 이 객체에 대한 Open/handler등록이 없다. rootTelnetPort를 읽는 것만으로 포트가 열리는 것이 아니다. 최종owner는 기본객체와 실제Bind를 연결하고 **Open을 자동추가하지 않아야 한다.** 실제 상태 객체의 활성판정을 제공하며 상수false callback으로 대체하지 않았다.

## 실제 소켓과 세션 수명

Open은7f020의 낮은WORD 포트/실제 TCP Listen/open판정이다. 실패한 listener 객체도 Shutdown까지 보관한다. 기존 LAN IPv4/backlog4/WinSock2.2 장치와 실제 Accept/Recv/Send를 재사용한다. 중복Open 진단333이복귀하면옛소켓을즉시닫지않고host종료까지보관한다. 정상최초Open에서는이보관표가비어있다.

Poll은 한번accept→첫빈4칸→Start/현재서버handler Started통지→거부시End→현재활성4칸Poll이다. 모두바쁘면실제새소켓을반납한다. 세션Active는 소켓포인터 존재이며 서버Active의flags40과 다르다. callback 교체는 shared lease로현재실행함수의수명을보호한다. 원문0/1/행포인터는 typed Event/같은160BYTE mutable span으로옮겨호스트주소를DWORD로자르지않는다.

Start는lineLength/outputLength/masking만0으로만들고 FF FB03→FF FB01을각3BYTE queue한다. **협상/입력차단·마지막명령·maskCharacter·userData/기존버퍼는재초기화하지않는다.** End는실제socket반납/NULL뒤현재 Endedcallback을통지하고반환은무시한다. 이미NULL이어도End통지는하며세션소멸자는포인터가남아있을때만End한다.

## 입력과 출력

세션은20BYTE씩한번읽는다. -1이면종료다. signed lineLength<159동안만각BYTE를처리한다. CR은CR/LF에코→현재행NUL→같은행callback/거부종료→길이0이다. BS는길이감소/BS·공백·BS,LF/NUL은무시한다. 보통문자는현재마스킹상태/문자로에코한다. IAC0/1/2·literalFF 무에코·F0/FA 입력차단도원문대로다. **159도달때남은수신을버리고다음수신에서도자동잘라내기/행초기화를하지않는다.**

출력은signed DWORD(outLength+len)<4801관문뒤실제4800BYTE에붙인다. Print는첫NUL전strlen,Printf는1024BYTE지역범위안에서서식화해같은queue를쓴다. Poll은입력뒤한번송신해부분/0의나머지를현재앞으로이동·보관한다. FlushComplete(7e9b0)는현재길이보다짧은첫송신이면종료하며두정책을합치지않았다.

## 집합·정리와 후속

Sets는현재listener/소켓포인터가있는4세션의실제핸들을 nullable read/write/error에직접추가한다. 중복/64상한·현재순서와최고핸들을유지하고uintptr폭을보존한다. Bind는기존메인의actual Active/Sets(read,NULL,error)/Poll을같은owner에연결해조건부중복Sets/일반수신뒤Poll을유지한다.

Shutdown은현재listener가있고flags40일때만현재활성4세션을순서대로End한뒤listener반납/NULL이다. 원문소멸순서는Shutdown→서버handler해제→세션역순소멸이며닫힌listener때남은세션도각소멸에서End한다. 최종owner는등록hook/모든callback 실행보다오래살아야한다.

Release/x64 NetstormCpp빌드종료0·경고/오류0이다. 새단위/MP/게임/통신·소켓열기/원격명령·세션/원본R실행검사0,전체검사/34미션/ZIP반복0이다. 보호2800파일불변·새fixture0이다. 원문/보호PE·모든범위기계어와현재OS장치접합만정적으로대조했다.

구형32비트5000BYTE ABI/SEH·힙/스택·미작성물리자체,callback교체/예외·소멸중예외/동시재진입·실제통신실패/수명은실행미검증이다. vsprintf1024넘침/깨진DWORD오프셋은명시오류,겹침이동은memmove다. 중복Open의원문미반납listener는최종host종료에서반납하므로누수전체일치는주장하지않는다. 기존현대WSA2.2/IPv4·LAN정책을사용하며원문WSA1.1/IPX ABI자체를복제하지않았다.

다음은전체전역패킷/477e10초기화·478af0정리·새R.exe최종owner/entry·실제Bind/Install/Run이다. 기본RemoteCommandServer/Bind/Shutdown을같은수명으로연결하되원문에없는Open/handler등록은하지않는다. 세.server콘텐츠/배포·LAN hostname/0F존47·3BYTE/0F·1d/메타/OSlaunch·LAN/Outpost/checking5·한국어후속/Edit시나리오UI도남아전체목표는미완료다.

원시/기계어·정적감사/빌드로그는Git제외 `extracted/cpp-resume-1087/`,추적보고서는 `cpppj/root-remote-command-20261107.json`이다. 마지막전체정밀디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며같은호스트기존산출물만읽었다. 재디컴파일/생성소스변경0이다.
