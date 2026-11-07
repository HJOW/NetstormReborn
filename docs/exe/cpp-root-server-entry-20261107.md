# 1091 — 새 R.exe 최종 owner와 실행 진입점

복원한 현재 모든 서버 모듈을 같은 Server owner에 묶고, 새R.exe 진입점의 Server.Run→MainLoop.Run 호출과 Lifecycle/실제 장치 접합을 연결했다. 제품1091/11.01, Netstorm.exe 12,366,336바이트/SHA-256 `f4ee1ec457395bfa48749f8cde509a1a203e2a7050f341db14a0b0c4e4aed7b5`, **새R.exe 669,696바이트/SHA-256 `90d4809a85b59dfde57ece535d8d3a12b11f7f74b8f520cc5a902f19df31366f`**다. 직전 CP는 `ec7dee19 / 20261107-162 CP`다. 원본R.exe를 실행/복사하거나 덮어쓰지 않았다. **서버 실행/통신 검증은0이며 클라이언트의 새R OSlaunch/배포는 아직 미연결이다.**

## 실제 같은 owner와 장기 상태

Server.State는 현재 ConnectionState/NetworkState/LifecycleState/ClockState/Random·ChallengeMessages/SharedFileTables/MirrorPlayerDefaults/MainLoopState를 한 번 소유한다. reset/cycle마다 사본을 만들거나 난수를 재시드하지 않는다. 파일명 고리도 한 RawFileName owner를 VFS/Config/설정·서버목록이 공유한다. 실제 생성자들은 참조/상태를 준비하며 설정읽기/시각·창/소켓 실행은 MainLoop.Run이 선택하는 원문 위치로 미룬다. 작업 폴더의 절대 경로 계산은 호스트 경계다.

Config는 기존 실제 전체읽기/FILE·키 RAII와 같은 VFS를 사용한다. 가상CWD의 FileSpecDirectoryHooks/실제 CRT getenv를 IDENTITY·USER와 Config 환경 치환에 연결한다. 사용자 Saves/options.cfg가 있으면 먼저 읽고 나머지 원문 Config파일 순서는 유지한다. 사용자 Options의 실제 읽기 실패는 성공으로 바꾸지 않는다. 서버 Config의 기존registryValue callback은 아직 미연결계약이며 R에서 필요한 경로 대조/접합은 후속이다. 이를 완료된 레지스트리 지원으로 확대하지 않는다.

SharedFiles.Bind/PacketRecord.Bind→Configuration→그 **같은 Configuration.Values() 참조**를 빌린 ProcessTrace를 구성한다. 생성 시 실행되지 않는 forward 로그/진단은 같은 Trace를 호출하며 구성 전 선택되면 명시 오류다. 실제 실행 전에 Trace가 이미 구성돼 있고 이후 LogDevice/AssertionDevice를 Clock/전송·Root/보조·메인 장치에 연결한다. 별도 기본Options/빈 로그나 가짜 진단 callback을 만들지 않았다.

같은 Clock.Bind와 PacketRecord 방향기록을 실제 전송에 연결하고, CCh 종료/타임아웃 주소1/0/0/실제 CRT _exit를 연결한다. 같은 CCh 저장소·주소·라우팅/현재기본바이트·페이지·목록 checksum참조를 만들고 CommandPages.Bind/Routing.Bind/BindSchedule을 연결한다. LoginRequest→현재 ChallengeRuntime.Login과 ChallengeRuntime→현재 LoginRequest.ClaimSocket은 같은 owner의 안정된 객체를 forward 대여한다. 같은 AddressCodec.Bind도 두 로그인/CCh 소비자에 적용한다.

RootControl/RootDirectory는 같은 현재Options/Clock/실제CCh 바이트를 빌리고 기본 RemoteCommandServer는 비활성 생성 그대로다. **원문에 없는 원격포트Open/handler등록은 하지 않는다.** 같은17개 참조의 LifecycleModules를 넘겨 Lifecycle.Bind 뒤 MainLoop를 만들었다. 메인의 시작은 실제 설정/시계/전체패킷·일반번호발행/공지/listener/CCh/파일을 실행하는1089 부모이며, 중복Tick/현재통신/파일감시·예약 처리도 같은 상태를 사용한다.

## 새 진입점과 정상/예외 반납

root/main.cpp는 실제 CRT argv0도 포함한 개수·대소문자를 보관한다. 정확히argc2/argv1소문자lan일 때만 같은 MainLoop.Run의LAN선택을 전달한다. 나머지 argv를 새 설정 명령으로 해석하지 않는다. 실제 현재CWD와 UserDataRoot.Resolve를 Server에 넘기며 배포판을 위한 임의 chdir/원본R 실행은 추가하지 않는다. MainLoop의11.01banner/LAN사전통지→실제Lifecycle.Start/프레임·통신→정리/재시작 호출이 진입점에 연결돼 있다. **이 호출 경로를 실행해 확인한 것은 아니다.**

정상 Run은 원문 Lifecycle.Shutdown을 수행한다. 최종 State 반납은 메인/수명hook과 등록표를 소비모듈보다 먼저 반납하고 나머지 모듈을 역순으로 정리한다. 실행/구성 예외 경로에서 새로운 Terminate/정상Shutdown callback을 합성하지 않는다. 같은 시계/옵션·파일/실제노드의 RAII owner는 소비자보다 오래 남는다. 호출자는 Server를 Run/모든콜백보다 오래 보관해야 하며 콜백 중 서버 반납은 지원하지 않는다. 선택된 원문 종료0/1은 실제::_exit이고 성공값/예외로 바꾸지 않는다.

## 실제 CMake/PE와 링크 증거

NetstormRoot는 새owner/진입점/서버전용Version리소스·UTF-8 manifest로 **새R.exe**를 만든다. 게임실행파일은 Netstorm.exe이며 import library는 NetstormRoot/netstorm/NetstormCpp를 구분해 덮어쓰지 않는다. NetstormCpp가 NetstormRoot에 의존하므로 기존 게임빌드 명령으로 새서버도 빌드된다. .map은 빌드경로에만 생성한다.

실제 새PE는AMD64/Windows CUI·제품/파일11.01·OriginalFilename=R.exe·UTF-8/asInvoker manifest다. DLL 목록도 실제PE에서 읽었다. 새Root는NetstormSmacker DLL을 import하지 않는다. 최종 링크맵에서 Server/MainLoop/Lifecycle/전체패킷·17수신에 대응하는 실제 수신부모/87h관리·진단 등19개 핵심심볼을 대조했다. Lifecycle.Shutdown의 별도심볼은 Release최적화로 없어졌으므로 **Bind의 두번째lambda _Do_call에 인라인된 실제256순회/Close→CCh.Shutdown→Remote.Shutdown 호출을PE기계어로 확인**했다. 심볼 유무만으로 정리누락/실행성공을 판정하지 않았다.

현재 활성 NetstormCpp.slnx는84프로젝트(Application79=검사이름73/게임1/서버1/probe·dump4,라이브러리2,Utility3)다. 디스크에 남은94vcxproj 중 활성솔루션에 없는10개는 제외했다. 이는 전체검사 통과 결과가 아니다.

## 검증과 잔여

Release/x64 NetstormCpp/NetstormRoot 빌드 종료0·경고/오류0이다. 새단위/MP/게임/통신/소켓열기/새R·원본R 실행0, 전체검사/34미션/ZIP 반복0이다. 마지막 실행검사는1052 기존7개며 이번에 반복하지 않았다. 보호2800파일 불변·새fixture0이다. 원문·소스와 실제PE/링크맵·정리콜백 기계어만 정적으로 대조했다. 실제 네트워크/수명·OS실패/부분I/O·예외/메시지pump·CRT환경인코딩은 실행 미검증이며 구형32비트힙/SEH·ABI 전체동일성을 주장하지 않는다. 기존 IPv4 TCP/UDP/LAN경계·IPX/SPX명시미지원과 폐기7f/80·PIRATE quota비활성을 유지한다.

다음은 클라이언트 NativeLocalServerLaunch에 **이 새R.exe**의 실제 OSlaunch를 연결하고 배포판에 포함하는 일이다. 원본R을 대신 실행하면 안 된다. 원문500 통지는 초기화 전 사전통지이며 준비성공ACK가 아니다. 세.server콘텐츠/배포·LAN hostname/0F존47·3BYTE/0F·1d/메타·Config 레지스트리 경계·LAN/Outpost/checking5·한국어후속/Edit시나리오UI도 남아 전체목표는 미완료다.

원시/기계어·PE/링크/정적감사·활성솔루션/빌드로그는 Git제외 `extracted/cpp-resume-1091/`,추적보고서는 `cpppj/root-server-entry-20261107.json`이다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**,R전용 **2026-11-07/HJOW-Athlon**이며 같은 호스트 기존 산출물만 읽었다. 재디컴파일/생성소스 변경0이다.
