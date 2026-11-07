# 1064 — R 실제 전송 노드와 메인 통신 접합

`root::TransceiverTransport`가 실제 SocketDevice/고정 버퍼·노드/숫자 토큰/공통 프레임 분배를 같은 R256/32 표와 NetworkPump에 연결한다. 제품 기준은 **1064 / 11.01**, 게임 EXE 12,365,824바이트, SHA-256 `592d70f40d8d453392092a637f483953a718f077745bdcd2c425a47a089a545a`다. 직전 CP 커밋은 `6c4a981c / 20261107-93 CP`다. **새 R.exe 대상·프로세스 초기화/Root·CS 관리자와 실제 호출은 아직 없다.** 전송 class 작성/부모 접합을 서버 실행/접속 성공으로 계산하지 않는다.

## 실제 노드/번호/수명

생성은 소켓을 열지 않는다. 명시 OpenDatagram은 첫 일반1..255 번호와 실제 UDP/512BYTE, Listen은 같은 표의 실제 listener/버퍼8과 accepted 기본4096을 사용한다. 실패한 실제 열기도 버퍼 구성/표 공개/닫힘·0 쓰기를 통과한다. 최초 listener는 기존 InitializePending을 수행한다. listener의 sender0과 실제 일반 슬롯 번호를 구별하고, accepted의 별도 가상 수신 종류를 상속된 socket listener bit로 대체하지 않는다. PE485be0의 CMP/SETL처럼 signed pendingCount<31일 때만 수락해 data초기1을 포함한 최대30개를 허용한다. 음수 sender 공개와 개수 증감은 기존 Register/Unregister/ClosePending 부모를 사용한다.

두 별도 지역 FullTime 캡처→현재 peer 조회→현재 sender/논리 bufferSize/ff 초기 입력·stream 출력 순서다. 새 getpeername 장치는89620/896d0처럼 실제 조회 오류 코드를 기록하되 이 조회만으로 socket 오류 bit2000을 추가하지 않고 앞16BYTE만 쓴다. 등록 처리기가 받은 같은 PacketConnection에서 실제 생존 토큰을 찾는 TokenFor도 제공한다. 호스트 포인터를 DWORD로 자르거나 토큰을 재사용하지 않는다.

2번 수신은 명시 InstallTerminationReceiver에서 원문75170에 연결한다. Close는 killed40 쓰기 뒤 depth0에서만 실제 소멸/flush/버퍼·소켓 반납을 수행한다. callback 중에는 공통 분배의 복귀가 close/release를 완료하고, 물리는 최외곽 호출 뒤 수거한다. 소멸 flush도 일반 flush와 같은 부모다. 최종 C++ owner 종료의 잔여 핸들 반납은 사용자 callback을 재호출하지 않는 호스트 경계이며 정상 R cleanup 완료를 대신하지 않는다. 실제 부모는 CallbacksActive/owner 수명을 유지해야 한다. 오래된 Root 표 별칭을 임의 새 연결로 복구하지 않는다.

## 부분 수신과 송신

| 경계 | 반영한 순서 |
| --- | --- |
| 첫 stream 수신 | 현재 socket 준비→최초접속20 해제→실제1번 처리기가 있을 때만 현재 BYTE0/헤더4로 통지→현재 peer 재조회 |
| 데이터그램/stream | 실제 recvfrom 또는 부분4BYTE 헤더/본문→선언 길이·현재 논리 크기/실제 할당 검사→완성 뒤 부분 길이0→검증→첫 Type의 observer0→현재 헤더/등록으로 분배 |
| 반환 | 원문 NULL/길이0 sentinel/실제 현재 입력 포인터를 Stopped/Sentinel/Packet으로 구별. false 입력은1/2 통지·실제 패킷 분배만 생략하며 observer는 유지 |
| 직접/sendto | 매 송신 observer1→현재 선언 길이→실제 쓰기→짧으면2번 통지→원문이 계속하는 경로의 현재 기록 |
| 집합 쓰기 | 65535 초과면 먼저 flush/재판정→필요 시 used+1024+length 재할당/+38 갱신→실제 복사/현재 포인터 증가→기록 |
| flush | 현재 집합의 type4/길이 확정→다음 헤더4 예약 offset→현재 signed writeLimit→실제 부분 쓰기→확정 접두만 이동/두 포인터 감소 |
| 모드 전환 | 켤 때마다 buffered2/4·0을 다시 기록. 끌 때 먼저 flush→남은 현재 집합 진단→bit2 해제 |

835f0은0 반환이고83610/444c50/483e00은 no-op이다. Byte0의80을 observer 완료 marker로 만들지 않고 송신마다 관찰한다.444b40은 Type BYTE만 쓰므로1/2번 통지와 집합 헤더의 BYTE0도 현재 버퍼 값으로 보존한다. type4를 flush할 때 별도 observer4/송신 기록을 추가하지 않는다. 집합의 다음 예약 헤더는 실제로 쓰기 전까지 단순 offset이므로 할당 끝보다 최대3BYTE 커져도 바로 잘못된 메모리 접근으로 판정하지 않는다.

NetworkPump는 실제 현재 노드의 시각/+90/socket/status·select/수신/flush와 기존 R 번호 종료에 연결된다. Challenge/보조 객체의 실제 상위 callback은 caller가 제공해야 하며 빠졌을 때 가짜 빈 서버 상태를 만들지 않는다. 현재 전역 시각(now)과 새 지역 캡처(captureNow), 방향 있는 실제 기록, 주소 formatter/진단/종료도 프로세스 장치로 남긴다.

## 호스트 경계와 확인

원문 write realloc은+38의 공유 논리 크기를 늘리지만 input 할당은 바꾸지 않는다. 새 class는 이 논리 쓰기를 보존하면서 실제 입력 할당을 별도 검사해 그 뒤의 초과 recv를 막는다. 잘못된 길이/재할당 중 쓰기 버퍼 자체 별칭/반납된 노드 접근도 물리 경계로 거부한다. 재할당 tail의 미정 원본 힙 값/32비트 포인터 배치·임의 overlap 자체 일치는 주장하지 않는다. 일부 console/packet trace는 설명을 간소화했고, 지연 호출 큐는 기존 명시 미연결 경계다. 폐기7f/80은 실제 송수신 관찰/처리 전에 제외하며 외부 등록/검증을 복구하지 않는다.

첫/최종 게임 Release/x64 빌드는 모두 종료0·경고/오류0이다. **새 단위/MP/게임/통신/소켓 열기/원본R 검사는0**, 보호2800파일 불변·새fixture0이다. 전체 검사/34미션/ZIP은 반복하지 않았다. 원시 함수/SETL/PE 진단 줄 사본과 빌드 로그는 Git 제외 `extracted/cpp-resume-1064/`, 보고서는 `cpppj/root-transceiver-transport-20261107.json`이다.

다음은 새 R.exe의 실제 초기화/옵션·Root/CS 관리자/수신과 보조 객체·시계/기록·formatter/실행 호출이다. 이어서 OS launch·17h/1dh/수동 검색/메타28~33·LAN/Outpost/checking5→한국어 후속→Edit 시나리오UI다. 전체 목표 미완료이며 MP 검사는 계속 제외한다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이고 이번에는 다시 수행하지 않았다.
