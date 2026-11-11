# 외부 접속 기능 목록과 구 클라이언트 검증 비활성화 — 단위684

**2026-11-08 / 1163 갱신:** 명시 접속요청의 hostname을 OS gethostbyname으로 해석한다. OS 설정 DNS/hosts/LLMNR 등에 이름 질의가 생길 수 있으며 첫IPv4가loopback/RFC1918/link-local일 때만 실제TCP접속한다. 뒤사설주소 재검색/공인IP접속·폐기7f/80/온라인유효성검증 복구는 없다. 과거의 DNS 미사용/숫자IPv4만 지원 표기는 당시 상태이며 현재는 이 범위로 대체한다. 레지스트리 기본값 조회는 읽기 전용 OS환경 계약이고 폐기security.dat/검증은 연결하지 않는다. [1163 근거](cpp-lan-environment-contracts-20261108.md).

**2026-11-07 / 1092 갱신:** 클라이언트37에클론모듈폴더의새11.01 R.exe만선택하는파일식별과실제Unicode CreateProcessW lan/wait0을연결했다. 자식CWD는게임자료폴더/환경상속·전용콘솔·핸들반납이며원본R/공식서버실행으로대신하지않는다. 새R은기존LAN IPv4경계/외부DNS·7f/80비활성을유지한다. 배포ZIP에새R/두실행파일런타임합집합을동봉하고실제바이트/해시를확인했으나게임/서버·통신실행은0이다([1092 기록](cpp-local-server-process-package-20261107.md)).

**2026-11-07 / 1091 갱신:** 원본R을실행/복사하지않고 같은실제서버모듈/환경·VFS/진단/주소·전송/수신/메인을묶은 새R.exe를 빌드했다. 진입점은현재CWD/정확한lan인자→같은Lifecycle/MainLoop호출이며기존LAN IPv4경계와외부DNS/7f·80비활성을유지한다. 기본원격포트Open/handler등록을추가하지않았다. 실제서버/통신실행0·클라이언트OSlaunch/배포는후속이다([1091 기록](cpp-root-server-entry-20261107.md)).

**2026-11-07 / 1090 갱신:** R485e42의 ZAssertion 표시에는 bugs@titanic.com으로 정보를 보내 달라는 문구가 있다. 표시문구이며 함수 자체에 접속/메일발송은 없다. 새 ProcessTrace는 사용자 Diagnostics/assert.html 위치 안내로 바꾸고 외부메일/HTTP/DNS를 추가하지 않았다. 새진단표시는11.01이며 폐기7f/80·온라인검증은 비활성 유지한다. 실제로그/진단/대화상자/서버실행은0이고 최종owner 연결은 후속이다([1090 기록](cpp-root-process-trace-20261107.md)).

**2026-11-07 / 1087 갱신:** R의004d23f8은 원격명령 TCP/4세션 장치다. 기본생성7ee90은 listener/handler NULL이며 현재R시작/전역참조에는 Open/handler등록이 없다. rootTelnetPort 읽기만으로 포트를 열지 않는다. 명시Open API는 실제LAN IPv4 listener/기존Accept허용범위를 사용하고 외부DNS/폐기공식서버 접속을 추가하지 않았다. 최종owner/실제Bind·Run/통신실행은 아직없다([1087 기록](cpp-root-remote-command-20261107.md)).

**2026-11-07 / 1083 갱신:** R11h 진단의 역방향 TCP 연결을 같은 실제 IPv4/client 장치에 연결했다. 실제 connect 앞에서 loopback/RFC1918/link-local만 허용하며 요청자의 주소/요청port를 사용한다. 공인 IP/폐기 공식 DNS 접속은 연결하지 않았다. IPX/SPX/Zlocal/UDP는 이 새 송신 TCP 장치의 범위 밖이다. 최종R owner/실제 호출과 MP/통신 실행은 아직 없다([1083 기록](cpp-root-diagnostic-20261107.md)).

**2026-11-07 / 1082 갱신:** R66h서버재공지를현재표/실제SendTo에연결했다. UDP는255.255.255.255와현재rootServerBroadcastPort로동일서버포트안내를세번전송하는몸체이며DNS/폐기공식도메인을조회하지않는다. 원문IPX주소구성은보존하나현재IPv4장치는실제IPX통신을지원하지않는다. 최종R owner/실제번호발행·시작공지와MP/통신실행은아직없다([1082 기록](cpp-root-control-20261107.md)).

**2026-11-07 / 1080 갱신:** 1Fh 준비/실제전투시작을연결하면서기존legacyLimitPirates기록전용정책을유지했다. 기존flag200집계는보존하되과반PIRATE·limitPirates의25h5가입거부/재배치분기는연결하지않는다. 원문전분기동일성과구분한폐기클라이언트검증비활성화정책이며7f/80·온라인검증도계속비활성화한다. master의정확한IPv4 loopback주소만기존어댑터장치로현재20BYTE에대체하며DNS/새외부접속을추가하지않았다. 실제MP/통신/주소조회실행0이다([1080 기록](cpp-root-battle-launch-20261107.md)).

**2026-11-07 / 1079 갱신:** CCh 이동0044fad0의 기본 BADVER 가입 거부문구004be1c8에도 폐기 HQ HTTP 링크가 있다. 기본 문구만 현재11.01 로컬 빌드 안내로 바꾸고 현재badVersionNoBattle옵션/기존 플래그와 확장비트는 보존했다. countPirates는 현재명단의기존flag200 집계/진단이며 이동에서는 반환값을사용하지않는다. URL열기/HTTP/DNS·온라인검증·quota판단은연결하지않았다. 실제새수신/MP/통신실행0이며[1079 기록](cpp-root-challenge-move-20261107.md)을따른다.

**2026-11-07 / 1076 갱신:** rootservers.dat의 현재 문자열 표/체크섬·행 읽기/갱신과 사용자 Saves 저장을 구현했다. 입력은 사용자 Saves 우선/원문 workingDirectory 기준..\d이며 파일에 도메인/주소 문자열이 있어도 이 로더·행 처리는 DNS/소켓을 호출하지 않는다. 같은 메인 재로드 hook/Root 체크섬 참조 계약을 제공하며 최종 R owner·실제 파일 실행은 아직 없다. 실제 접속은 기존 LAN 정책/폐기 검증 비활성화와 구분한다([1076 기록](cpp-root-server-list-20261107.md)).

**2026-11-07 / 1072 갱신:** Root00486870의 inet_addr 실패 뒤 gethostbyname과004870d0의 로컬 hostname resolver 경계는 새 AddressCodec에서 외부 DNS로 연결하지 않는다. 숫자 IPv4/localhost/현재 컴퓨터 이름만 처리하며 실제 로컬 IPv4는 GetAdaptersAddresses의 Up/LAN unicast에서 얻는다. 다른 LAN/공개 hostname은 주소0 해석 실패로 남으므로 LAN 접속은 숫자 IPv4를 사용한다. 최종 R owner/실제 호출은 아직 없고 DNS/OS 조회/MP 실행0이다. 다중 어댑터의 구형 resolver 순서·hosts 재정의 동일성을 주장하지 않는다([1072 기록](cpp-root-address-codec-20261107.md)).

**2026-11-07 / 1069 갱신:** CCh0044e940의 낮은 버전 기본 메시지004be0f0에는 폐기 HQ HTTP 링크가 있다. 새 CCh 기본 안내는 현재11.01 로컬 빌드 안내로 작성했으며 URL 열기/HTTP/DNS·외부 검증을 추가하지 않는다. LegacyPIRATE/BADDIAG 표시는 현재 wire 진단 문자열만 보존하고 온라인 검증 소비자로 연결하지 않는다. 실제 MP/통신/파일 쓰기 실행0이며 [1069 기록](cpp-root-challenge-runtime-20261107.md)을 따른다.

**2026-11-07 / 1068 갱신:** 제공 R의1Ch 관문00476180은004870d0에서 `gethostname`→`gethostbyname`으로 로컬 hostname의 첫 IPv4를 얻고,127.0.0.1 예외/주소 lowWORD를 비교한다. 시스템 이름 해석기가 네트워크를 사용할 수 있는 원문 경계다. 새 `root::LoginRequest`는 실제 localAddress 장치를 요구하며 직접 DNS/공식 도메인 조회를 추가하지 않는다. 장치·새R.exe 호출은 아직 미연결이고 실제 DNS/소켓/MP 실행0이다. 폐기7f/80·외부 클라이언트 검증은 계속 비활성화한다([1068 기록](cpp-root-login-request-20261107.md)). 마지막 전체 정밀 디컴파일은2026-11-03/HJOW-Athlon, R 전용은2026-11-07/HJOW-Athlon이다.

**2026-11-07 / 1061 갱신:** AccountSetup(005428e0/0200007a)은 원문 명령 표의 외부 계정 등록 경로다.004d7ac3은 `register`를004d5070에 넘기고 Tell `Setup`을 연다. 다른 비영 인자 분기는 도구 활성 확인/400h 송신이다. 기존 setup.exe 실행/외부 등록은 제품에 연결하지 않으며 새 R 실행 장치로 대신 실행하지 않는다. 등록 메시지100h의 현재 Fort ID 동기화는 같은 Client/Template의 로컬 파일 작업으로 따로 연결했다. 외부 접속·원본 실행은0이며 [1061 기록](cpp-registered-fort-subscriber-20261107.md)을 따른다.

**2026-11-07 / 1058 갱신:** 실제 UDP12 검색은 `255.255.255.255`와 현재 `rootServerBroadcastPort`로만 브로드캐스트한다. 원문 State38은 OS가 알려 준 자기 hostname만 `gethostbyname`으로 해석한다. 시스템 이름 해석기를 사용할 수 있으나 폐기된 공식 서버 도메인을 전달하지 않는다. 이후 TCP 연결은 기존 로컬 IPv4 정책을 따른다.7f/80·원격 클라이언트 검증은 계속 비활성화하며 실제 통신은 실행하지 않았다. [1058 범위/잔여](cpp-meta-udp-discovery-20261107.md)를 따른다.

**2026-11-07 / 1056 현행 갱신:** 원문 Root/Challenge 연결10/11과16h/1ch 요청·20~27을 기존 LAN TCP에 연결했다. IPv4 loopback/RFC1918/link-local만 허용하며 기본 두 폐기 도메인은 DNS/접속 전에 거부한다.7f/80·온라인 클라이언트 검증은 계속 비활성화한다. 실제 MP/통신 검사는 하지 않았다. 아래684의 당시 import 부재/미구현 표현은 그 당시 산출물 기준이며, 현재 범위/잔여는 [1056 기록](cpp-meta-connection-20261107.md)과 LEFT_JOBS.md를 따른다. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-11-03이다.

2026-11-01, **HJOW-Athlon**. 현재와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 같다. 원본/CD/디컴파일 파일을 변경하지 않았다. [기능·주소 목록](../../cpppj/external-endpoints-20261101.json), [제품·검증 보고서](../../cpppj/external-services-20261101.json)를 따른다.

## 적용한 새 요구

사용자가 갱신한 [AGENTS.md](../../AGENTS.md)는 로컬 멀티플레이 서버 접속을 제외한 외부 도메인/IP 접속 기능의 별도 문서화와 **클라이언트 유효성 검사 비활성화**를 요구한다. 서비스 종료는 사용자 지시의 사유이며 이번에 실제 서버 상태를 조회하지 않았다. 세 GUI 실행의 보호 기준 AGENTS SHA는 `ce9229c657458d72baa767335f5a46250db611f9e4fe968f081bcabedc359e8b`다.

기존673의 시작 writer 감사에서 찾은 원문 주소와 의미를 재사용했다. 당시 ‘원문 기본 실행의 Not Validated 안내는 정상’이라는 판정은 **원문 참조에만** 남긴다. 현재 제품에는 새 지침을 적용한다. [기존 네트워크 문서](network-ports.md)의 2026-09-30 비활성 결정과도 일치한다.

## 기능별 경계와 현재 처리

| 기능 | 원본 출처/주소·설정 | cpppj 현재 처리 |
| --- | --- | --- |
| Auto Updater의 클라이언트 검증 | WinMain438dc0의 비기본 명령줄/security.dat,438f32의 UserUpdated0054da8c 쓰기. State3/4b72b6의 UserNotUpdated 제목 Not Validated | 온라인 검증·security.dat writer는 미구현이며 앞으로 연결하지 않는다. 제품 시작 분기의 검증 getter와 안내도 비활성화했다. 검증 성공1을 합성하지 않는다. |
| 인터넷 master/root 서버·디렉터리 | masterServer, officialBasePort/rootServerBasePort, rootServerBucketsServer, !rootservers.dat, 구 nsLaunchC.exe 실행 흐름 | 외부 디렉터리/DNS/공식 서버 접속 미구현.3차에서는 사용자 선택 LAN 로컬 서버만 사전 구현하며 옛 인터넷 서비스를 복구하지 않는다. 709의 HTML Zsend는87h 요청 작성과 현재11번 송신기 조회만 정적으로 연결했다. 해당 연결이 없으면 생략하며 새 서버 접속/DNS/검증 성공을 만들지 않는다([709](cpp-html-zsend-20261102.md)). |
| 인터넷 ping·네트워크 진단 | Ping0..4/PingName0..6, diagServer/diag*Port/diagMode* | 현재 제품의 외부 ping/진단 접속 미구현. 원본 설정값/원문 포트 근거를 목록에 보존한다. 실제 요청/응답·프로토콜 동작은 시험하지 않았다. |
| 사용자 선택 웹·업데이트·메일·telnet 링크 | 원본 config/help/tell/update.english와 사용자 시나리오의 URL/치환값, PatchUrl, autoOpen/프로토콜 명령 | Help 링크 선택은 OpenClientExternalHelp→HelpLinkPolicy→ShellExecuteW 또는 프로토콜 실행에 연결돼 있다. 설정/시나리오가 새 주소를 제공할 수 있다. 사용자 선택 링크 기능은 유지하며 자동 업데이트 다운로드/검증·오류 자동 전송은 구현하지 않는다. |
| Technical Help·설정 명령 실행 | techHelpExe/techHelpFile의 현재 해석 결과, 프로토콜 설정/레지스트리 fallback | 현재 외부 실행 장치가 있다. 기본 로컬 도움말 파일과 사용자 설정의 외부 명령을 구분해야 하며, 고정 원격 서버 주소가 있는 별도 TCP 클라이언트로 보지 않는다. |
| 로컬 게임/root 서버·LAN 탐색 | 원본 게임6799/root6800/UDP탐색8998,127.0.0.1·0.0.0.0, 기본off인 내장HTTP80 | 사용자 지시의 로컬 접속 예외다. 현재 싱글/Edit 전송은 프로세스 내부다. 실제 로컬 TCP/IP는3차이며 구 인터넷 검증 성공을 요구하지 않는다. 0.0.0.0은 리슨 주소이고 원격 목적지가 아니다. |

정적 목록은 위 경계를5분류로 묶었다(웹/외부 도움말 실행을 한 분류로 합침). 현재 제품 PE의 직접 WSOCK32/WS2_32/WinINet/WinHTTP import는 없다. ShellExecuteW/CreateProcessW 경계는 존재하며, 이 직접 import 결과를 모든 간접 동작의 부재 증명으로 확대하지 않는다.

## 확인한 기본 목적지

| 출처 | 기본값/목적지 |
| --- | --- |
| masterServer / Master URL | master.titanic.com, telnet:master.titanic.com |
| !rootservers.dat | netstorm.game-host.org:6800 / play.netstormworld.com:6800 |
| diagServer | diag.netstorm.activision.com |
| Ping0..4 | 208.201.96.13 / 208.201.96.133 / 192.36.148.17 / 128.63.2.53 / 128.8.10.90 |
| PingName0..6 | netstorm.game-host.org / subserver.game-host.org / www.netstormhq.com / www.netscape.com / www.yahoo.com / www.intel.com / www.activision.com |
| 기본 사이트/패치·도움말 | www.activision.com / www.netstormhq.com / te.netstormhq.com / www.titanic.com, 구 netstorm.game-host.org의70Update 다운로드 경로 등 |
| 원문 오류 보고 메시지 | www.titanic.com/netstorm/bugreport.cgi. 정적 오류 문구이며 현재 제품의 자동 전송 기능을 뜻하지 않는다. |
| 동적 대상 | tell.english의 param.WebSite/UPDATEURL/EMAIL1 등 치환값, 사용자 시나리오 href·mailto, 프로토콜/레지스트리 명령 |

정확한 URL 문자열·원문 줄·아카이브 항목·설정 키·SHA는 JSON 목록에 있다. 두 판본의 실제 원본 아카이브246/258항목, 느슨한 HTML/시나리오, setup.cfg, 메인 PE와 R.exe를 읽었다. **10.78 URL후보59개/10.72 18개와 설정43/39행**을 기록했다. 이 수에는 중복·치환 템플릿·본문 문구가 포함되며 고유 서버 수/실제 연결 수가 아니다. 별도 도구 `tools/cpp_external_endpoints_audit.py`는 파일만 읽고 DNS/HTTP/프로그램을 실행하지 않는다. 새 설정/시나리오가 제공할 미래의 임의 주소를 고정 목록에 모두 열거할 수는 없으므로 소비 함수와 동적 경계도 함께 문서화했다.

10.72는 주소/설정/문자열 인벤토리다. 이번 정책의 실제 GUI/시작 경로는10.78 복원판이며, CD 검증 부모가10.78과 같다는 새 실행 증거는 없다.

## 제품 변경과 검증

NativeMainMenuState의 기본 정책 `referenceValidationNotice=false`와 실제 Client 조립의 빈 userUpdated getter/명시false로 구 검증 조회·Not Validated 안내를 제외했다. UserUpdated의 BSS0과 진단값은 그대로다. 원문과 다른 이유를 한국어 코드 주석에 명시했다. Welcome·저장 재접속 질문·첫 메뉴·Tips 조건은 유지한다. 원문 참조 검사만 명시true로 기존 분기를 선택하며 제품에 이를 켜는 설정/명령줄은 없다. 패킷 타입/길이/형식 같은 로컬 자료 검사는 폐기된 온라인 클라이언트 검증과 구별해 유지한다.

최종 Release 제품/집중 빌드는 종료0·경고/오류0이고 **전용12개(새2·기존10), 실패0**이다. 새 정책은 getter 부재·검증값0 보존과 Welcome/startup/first/Tips의16조합을 확인했다. 기존 원문 **41입력/82관측 fixture**는 참조 모드로 그대로 재생했다. 새 PE 실행이나 fixture/기대 관측 수정은 없다.

실제 GUI는 이전683제품의 안내 존재/Dialog1/검증값0과 수정684제품의 안내 없음/MainMenu0/현재 모달0/검증값0을 확인했다. 최종 수락2회다. 전체 관측에는 **3게임 프로세스, 모두 종료0**가 포함된다. 첫 수정판 관측은 도구가 Dialog1을 계속 기대해 상위 검사가 실패했고, MainMenu0 기대값으로 고친 재관측이 통과했다. 최초 빌드도 검사 어댑터의 한 줄 주석이 닫는 중괄호를 가려 실패했으며 수정 후 최종 빌드/12검사가 통과했다. 실패 로그를 보존했다.

세 실행 전후 originals1394·originalCD425·fixture784·AGENTS1 **총2604파일 경로/내용 동일**, 설정 복원이다. 새 원격 접속·DNS/HTTP probe·MP/미션·공격·포획/희생·승리·공중3종 행동·물리 입력·PCM 검사는0이다. 사용자 AGENTS 변경 전의 옛 SHA와 새 보호 기준을 혼동하지 않는다. 제품 SHA는 `29248df35d60eb66e375659024b0946452b5cbc9f41c8692904df2dad8613021`이다. 원시는 `extracted/cpp-external-services-684/`이고, 마지막 전체34/ZIP은645의644제품으로 유지한다.

문서 작성 중 추가된11.01 버전 표기/세 번째 Patch Credits 요구도 다시 읽었다(현재 AGENTS SHA `138808039a2d171d8e697906776e620f9ea8dc2038c35b4dc01ce18d3e462b77`). 다음 단위에서 이 추가 요구를 먼저 구현한 뒤 보류한 시나리오 명령/메뉴 수명 연결을 계속한다.3차 MP 사전 구현 때도 이 목록과 검증 비활성 정책을 적용한다. 기존 전체 목표와4~7차는 진행 중이다.


## 2026-11-06 추가 — CD 바이트 확인7f/80 (965)

785의 미분류 파일 소비자0042cd10은0040b4b0/0040c26d에서7f로 등록되고, 패킷에서 CD 파일명/offset을 풀어4BYTE를 읽어 논리수신자11에 응답80을 보내는 경로로 확인했다. 클라이언트 검증 용도 분류는 동작과 사용자 정책에 근거한 판단이며 고정 도메인/업데이트 검증 전체와의 동일성을 주장하지 않는다. 제품의 LocalMissionTransport/Editor 공유 단일·집합 수신과 송신에서7f/80을 제외했다. 실제 CD 조회·응답 본문·검증성공은 연결하지 않는다. 새 MP/프로토콜/통신 검사 없이 코드/컴파일과 일반 싱글 진입만 확인했다. [965 기록](cpp-retired-cd-validation-20261106.md)·[정적 출처/제품 위치](../../cpppj/retired-cd-validation-static-20261106.json)를 따른다. 위684의 당시 제품/5분류/목적지 목록은 당시 기록으로 보존한다.

## 2026-11-06 추가 — LAN TCP 부품(982)

`LocalTcpTransport`의 명시 Listen/Connect/accept만 WinSock2를 사용하도록 컴파일했다. 목적지/수락 상대는127/8·RFC1918·169.254/16의 IPv4 숫자 주소이며 bind0.0.0.0은 리슨 주소다. DNS/공식·master/root·인터넷 진단·검증7f/80은 연결하지 않는다. 생성 자체는 무소켓이며 아직 실제 Login/UI caller가 없어 현재982 EXE에는 WS2_32 import도 없다. 부품 컴파일과 활성 연결/기능 검증을 구별한다. 새 MP/소켓/통신/게임 검사는 실행하지 않았다. [982 범위와 다음 접합](cpp-lan-tcp-transport-20261106.md)을 따른다.


## 2026-11-06 단위984 — 공유 전송 실제 단독 경로 접합

공유 등록표/per-ID 복합 전송이 실제 단독 미션의 Client/World에 연결돼 EXE에 `WS2_32.dll`이 링크된다. `InternalMission` 역할은 TCP backend를 만들지 않는다. 실제 tutorial1 진입/대기/종료만 확인했고 LAN/소켓/통신 검사는 하지 않았다. TCP 역할의 주소 제한·DNS/공식 서버/폐기 유효성 비활성 정책은982와 같다. Multiplayer UI와 일반 전투 접합은 미완료다. [984 범위와 검증](cpp-shared-transport-lifetime-20261106.md)을 따른다.

## 2026-11-11 단위1301 — 14차 QuickJS-ng 엔진 탑재

`Netstorm.exe`에 QuickJS-ng v0.18.0 엔진을 정적으로 넣었다([기록](cpp-quickjs-engine-20261111.md)). quickjs-libc(`std`/`os` 모듈)는 넣지 않았고, 스크립트 컨텍스트에는 `fetch`·`XMLHttpRequest`·`WebSocket`·`require`가 없으며 모듈 적재기가 없어 `import()`가 거부된다(검사 14건으로 확인). 엔진 소스는 저장소에 고정해 두어 빌드 중 내려받기도 없다. 가져오기 표는 DLL 28개 집합이 그대로이고 새 함수 39개는 CRT 수학/힙·시각/시간대·동기화뿐이다. WinINet/WinHTTP/URLMon·새 WinSock·`LoadLibrary` 가져오기는 0이며 새 외부 도메인/IP 접속 기능은 없다. 현재 게임 안 사용은 시작 시 자체 검사(고정 식 6개)뿐이다. 연동을 넓힐 때(결정 D7) 새 노출 API가 외부 접속을 만들지 않는지 이 문서에서 다시 감사한다.
