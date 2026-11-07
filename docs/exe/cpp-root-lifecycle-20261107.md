# 1089 — R 시작·정리와 메인 수명 접합

00477e10/00478af0의 시작/정리 부모와 같은 실제 메인 hook을 작성했다. 제품1089/11.01, EXE 12,366,336바이트, SHA-256 `692bc973f28bee347f454431f5f6727e2c40ff026b62f233c9e762f4e17e5e49`다. 직전 CP는 `27e5a762 / 20261107-159 CP`다. **새R.exe 최종owner/entry에서 실제 Bind/Start/Run을 호출하는 접합은 아직 없다.** 서버·소켓/통신을 실행해 검증하지 않았다.

## 같은 현재 모듈과 시작 순서

LifecycleModules는 같은 ConnectionState/NetworkState/Configuration/Clock/ChallengeStore/ServerList/SharedFiles/SharedFileStorage/RootControl/RootDirectory/LoginRequest/Routing/ChallengeRuntime/RemoteCommandServer/AddressCodec/TransceiverTransport를 참조로 빌린다. 별도 가짜 상태나 수신기를 만들지 않으며 Lifecycle 생성은 IO/OS/네트워크를 실행하지 않는다.

Start는 원문12개 전역0→현재 pending0..31 close/칸0→프로세스 핸들50개/별도004d2200/프로세스 소유51개/일반256개0→options로그→flushAfterTrace1→서버목록 Reload/개수 캡처·현재 각행80BYTE Decode/로그→Configuration.Startup 순서다. pendingCount/pendingInitialized는 이 위치에서 재설정하지 않는다. localLaunch/useIPAsLanProtocol와 나머지 옵션도 임의 재초기화하지 않는다. 이후 모드17h/기본포트 진단→일반256개 두 번째0→Clock.Initialize→같은1088 전역 패킷 초기화/실제2번 종료 수신→baseport 로그다.

Configuration의 dataPrefix가 확정된 뒤 같은 SharedFileStorage.DataDirectory를 UTF-8 경로로 갱신한다. 이는 호스트 파일 경계이며 뉴스 읽기는 아래 원문 위치에 남긴다. 서버목록 개수는 최초 한 번, 각 행은 매번 현재 표를 읽는다. 로그가 행을 없앴다면 기존 Decode가 보존한 이전80BYTE 문자열과 새 value0을 사용한다. 첫 행부터 미작성 출력인 경우는 구형 printf의 불법 물리 읽기 대신 명시 오류다.

## 실제 서버 노드 생성과 번호 공개

기존 OpenDatagram/Listen의 실제 생성 부분을 CreateServer로 공유했다. 시작 부모는 원문 주소 parser의 udp/ipx/tcp/spx wildcard 문자열과 낮은WORD 포트를 사용한다. 새 공개 CreateServer는 **현재 시작 부모의 s 전용 wildcard IPv4 TCP/UDP 장치 범위**이며 일반 socket 생성자의 모든 옵션을 복원한 API가 아니다. 다른 레거시 IPX/SPX는 선택/주소 해석 뒤 명시 logic_error로 거부한다. 실패를 성공이나 OS 오류 결과로 합성하지 않는다. 기존 현대 IPv4/WSA2.2·LAN 경계를 유지한다.

CreateServer는 번호표를 직접 바꾸지 않고 실제 socket을 열어 결과가 실패여도 두 시각/버퍼를 가진 노드를 구성해 반환한다. UDP버퍼512·listener자체8/수락4096이며 listener는 원문 위치에서 최초 pending 초기화를 수행한다. 기존 OpenDatagram/Listen은 같은 생성기 뒤 기존 번호 공개/실패반납을 유지한다.

LAN broadcast는 먼저 현재 일반번호를 udpBroadcastId/ipxBroadcastId에 발행하고, 생성 뒤 현재 번호에 노드를 공개한다. open은 원문478a70의 하위socket flags40/상위connection kill40이다. 실패면 현재노드 close→현재칸0→번호0, 성공이면 현재포트 로그→현재노드 noMainTimeout1이다. 두 분기 뒤 같은 RootControl.Announce를 호출한다. TCP listener는 Root일 때 첫 일반번호/원문 프로토콜 선택→실제 생성/표 공개→open판정/실패 close·칸0이다. 번호와 노드 토큰을 혼동하지 않는다.

다음은 현재 Root모드면 같은 ChallengeStore.InitializeRoot의 강제 단일프로세스/현재1..count-1 생성, 현재 Challenge모드면 CheckNews→CheckBlacklist다. 끝의 모드 snapshot→실제 GetCurrentProcessId→원문 대문자16진수 로그도 유지한다. IPX/SPX를 선택하는 구형 구성은 현재 실제 장치 범위 밖이므로 이 전체 경로의 실행 완료를 주장하지 않는다. 현대 로컬 TCP/IP 대상과 원문 레거시 전체 구현을 구분한다.

## 정상 정리와 메인 Bind

Shutdown은 현재 일반0..255의 각 close/복귀 뒤 같은칸0→현재 CCh1..50 소멸/각칸0→같은 RemoteCommandServer.Shutdown 순서다. **pending은 다음 Start에서 닫고 이 함수에서 닫지 않는다.** 시계·난수/예약·요청/유휴시각/이전참가자 수나 옵션을 추가로 지우지 않는다. 프로세스 핸들은 원문의 현재 단일프로세스에서 새로 생성/발행하지 않으며 uintptr_t로 폭을 보존한다.004d2200은 제공 R에서0을 쓰는 참조만 있고 읽는 참조는 없어 별도 값으로 보관한다.

Bind는 같은 Start/Shutdown·ServerList.Reload와 기본 RemoteCommandServer.Active/Sets/Poll을 실제 MainLoopHooks에 연결한다. 원문의477e10은 최초/재시작에 같은 함수라 first 인자는 여기서 동작을 바꾸지 않는다. **원문에 없는 원격 포트Open/handler등록은 추가하지 않는다.** hook/수신 실행보다 실제 전체 owner가 오래 살아야 하며 전송 callback 중 시작/소유권 반납은 명시 거부한다.

## 검증과 후속

Release/x64 NetstormCpp 빌드 종료0·경고/오류0이다. 새단위/MP/게임/통신/소켓열기/원본R 실행0, 전체검사/34미션/ZIP 반복0이다. 마지막 실행 검사는1052 기존7개며 이번에 반복하지 않았다. 보호2800파일 불변·새fixture0이다. 원문/보호PE·전체 함수범위 기계어와 현재 소스의 전역값/표·호출 순서만 정적으로 대조했다. 구형32비트 힙/SEH·포인터 ABI·원문 누수 전체일치/비정상 예외 정리·콜백 교체/동시 재진입·실제 통신 실패/전체 수명은 실행 미검증이다.

다음은 새R.exe 최종owner/entry에서 같은 장기 모듈·프로세스 환경/VFS/로그·진단을 만들고 실제 Lifecycle.Bind/MainLoop.Run을 연결하는 일이다. 클라이언트 OSlaunch/배포·세.server 콘텐츠/배포·LAN hostname/0F존47·3BYTE/0F·1d/메타·LAN/Outpost/checking5·한국어후속/Edit시나리오UI도 남아 전체목표는 미완료다.

원시/기계어·정적 감사/빌드로그는 Git제외 `extracted/cpp-resume-1089/`, 추적 보고서는 `cpppj/root-lifecycle-20261107.json`이다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 같은 호스트 기존 산출물만 읽었다. 재디컴파일/생성소스 변경0이다.
