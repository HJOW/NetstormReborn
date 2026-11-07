# 1063 — R 실제 WinSock 장치와 공통 프레임 분배

R 전용 `SocketDevice`의 실제 Windows IPv4 소켓 기능을 작성하고, 기존 Client의 단일/집합 프레임 몸체를 Player/22칸 연결표와 독립적인 `o::DispatchPacketFrame`으로 옮겼다. 실제 Client도 같은 함수를 호출한다. 제품 기준은 **1063 / 11.01**, 게임 EXE 12,365,824바이트, SHA-256 `882b10c1a79ef492d8fb9e75e462d22c146214e2f504c16c54c6d160d502c0d5`다. 직전 CP 커밋은 `150b6019 / 20261107-92 CP`다. **새 R.exe/전송 노드 owner/NetworkPump 실제 장치 연결·Root/CS 관리자는 아직 없다.** 소켓 장치 작성/컴파일을 서버 실행 성공으로 계산하지 않는다.

## 공유한 것과 R 전용 경계

R PE004cd160의 최대 처리기 번호는137이다. R004cd168과10.78 PE0052ec40의 패킷 길이 WORD138개를 직접 비교해 차이0을 확인했다. 서버가17h에 쓰는10.71/클라이언트 판본10.78/제품 표시11.01은 별개다. 기본 패킷 표와 분배를 공유해도 버전 표시나 CS 메모리 배치 전체가 같다는 뜻이 아니다.

기존 LocalMissionTransport.DispatchFrame의 단일/집합 몸체를 그대로 공통 계층으로 이동하고 같은 실제 표/현재 callback/endpoint 또는 외부 노드의 close·release 장치를 전달한다. 단일은 버전 관문 뒤 시각/기록/수신, 집합은 시각→버전 관문→기록/observer→수신 순서를 유지한다. 집합 뒤 깊이0/닫힘의 반납도 같은 원문 위치다. 같은 등록표의 callback pin과 callback 뒤 현재 등록 재조회가 유지된다. Player9/게임 Registry를 가짜로 만들어 R 분배를 호출하지 않는다. 폐기7f/80은 단일/집합 모두 수신 처리 전에 제외한다. 중첩 집합/지연 큐 경계는 기존 명시 오류로 남으며 전체 R 패킷 진단 출력·선택적 record/지연 큐 완료로 계산하지 않는다.

## 실제 SocketDevice

생성은 WSA/네트워크를 시작하지 않는다. 명시 OpenDatagram/Listen에서 공유 WSA2.2 owner를 만들고 실제 소켓을 연다. accept 자식은 같은 owner를 유지해 마지막 소켓 소유자 뒤에 WSACleanup한다. 핸들을 DWORD로 절삭하지 않으며 유효한 OS 소켓0도 보존한다. 이는 원본32비트 핸들 배치나 WSA1.1 ABI 자체 복원을 뜻하지 않는다.

| 경계 | 구현 |
| --- | --- |
| 열기 | IPv4 wildcard UDP bind/broadcast, 실제 TCP listen backlog4, bind 실패 뒤 SO_REUSEADDR/재시도, 실제 오류/핸들 반납 |
| 수락 | 88760의 read/error select0→accept. 새 소켓은873a0처럼 listener의 현재 플래그/오류를 상속. peer20BYTE를 보관하고 외부 IPv4는 공개하지 않음 |
| 읽기 | 88a90/89170의 read/error select0→한번 recv/recvfrom. 준비없음0/실패-1/부분 길이 보존, TCP EOF도 원문처럼 오류 |
| 쓰기 | 88e90/893e0의 FIONBIO1→send/sendto→FIONBIO0. would-block만0, 실제 부분 길이·오류 보존. sendto는20BYTE 목적지와 datagram 관문을 사용 |
| 전체 select | NetworkPump의64칸 세 집합을 중복 제거 없이 WinSock fd_set으로 옮기고 실제 반환 집합/값을 보존. Windows가 무시하는 nfds는0으로 전달 |
| 주소/정책 | 실제 network endian 주소20BYTE/getsockname. DNS·구 도메인 조회 없음. 기존 Client와 같은 loopback/RFC1918/link-local 및 명시 broadcast로 제한 |

사용자 지침의 LAN 범위 제한 때문에 외부 TCP peer는 닫고 외부 UDP 데이터그램은 분배하지 않는다. IPX/SPX·zlocal·임의 외부/호스트명 목적지·옵션 문자열 전체 해석은 이 IPv4 장치에 넣지 않는다. 원본의 조건부 사전 reuse/임의 플래그 전체 경로까지 모두 복원한 것은 아니다. R의 수신 capacity512/4096과 부분 프레임·집합 송신/번호 토큰·깊이 반납은 **다음 transceiver owner**에서 이 장치와 같은 표에 연결해야 한다.

## 확인과 인계

첫 게임 Release/x64 빌드는 종료0이다. 검토에서 datagram 수신/SendTo의 non-datagram 반환0·20BYTE 인자/실패한 WSA 시작 재시도를 보완한 최종 빌드도 종료0·경고/오류0이다. **새 단위/MP/게임/통신/소켓 열기/원본R 검사는 모두0**이다. 보호2800파일 불변·새fixture0이며 전체 검사/34미션/ZIP을 반복하지 않았다. 원시 함수/PE138개 비교와 빌드 로그는 Git 제외 `extracted/cpp-resume-1063/`, 보고서는 `cpppj/root-socket-device-20261107.json`이다.

다음은 실제 R transceiver의512/4096 버퍼·부분 읽기/집합 송신·숫자 토큰/깊이 수명과 SocketDevice/NetworkPump 접합이다. 이어서 새R.exe/Root·CS·OS launch·17h/1dh→수동 검색/28~33·메타/LAN/Outpost/checking5→한국어 후속→Edit 시나리오UI다. 전체 목표 미완료이며 MP 검사는 계속 제외한다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이며 이번에는 다시 수행하지 않았다.
