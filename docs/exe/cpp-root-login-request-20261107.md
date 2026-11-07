# 1068 — R1Ch 앞단과 실제 소켓 이전

`root::LoginRequest`가00476180의 Root1Ch 수신 앞단과 실제 대기 소켓 이전을 같은 ConnectionState/ChallengeStore/TransceiverTransport에 연결한다. 제품 기준은1068 / 11.01, 게임 EXE 12,365,824바이트, SHA-256 `5722fc6e0c69dc1cfdfcaa760f74c7cfeb821bc3a3de2dbe40a4d1b05bd67a47`다. 직전 CP는 `958526d9 / 20261107-103 CP`다. **CCh0044e940 로그인 본체/참가자0044e0d0 종료·응답과 새R.exe/실제 초기화 호출은 아직 없다.** 실제 CCh 로그인 callback을 요구하며 빈 장치로 접속 성공을 만들지 않는다.

## 실제 Root 관문

명시 Install에서1Ch observer004444f0/버전1·정책0/가변길이·실제 수신 앞단을 등록한다. observer는 송신일 때만 nameLen255 진단1223과46+unsignedBYTE 길이 WORD를 쓴다. 수신은 exitRequested→실제_exit0, Chal bit 진단1206, 현재 IPv4 주소 관문, signed BYTE 존+19/현재 개수/현재 토큰 순서다. IPv4는 원문처럼 **127.0.0.1 한 주소** 예외와 local/peer 주소 DWORD의 lowWORD 비교를 유지한다. 임의 전체 RFC1918 통과로 바꾸지 않는다.

positive sender는 유효 CCh를 원문 고정 인자1/현재 요청으로 호출하고1252 로그를 남긴다. 무효 존은1245 로그다. sender<1은 무효 존1257, non-root9/1263 로그, 현재 CCh 재조회 뒤 pending 소켓 이전을 수행한다. 현재 소켓 추출/옛핸들 무효화→같은 ClosePending/개수 변경→현재 CCh flags1 관문→요청+29h 쓰기→고정1/실제 CCh 호출 순서다. RootDirectory의 별도 ReadTransceiver negative 관문을 사용하지 않는다.

R004870d0의 원문 로컬 주소 조회는 gethostname/gethostbyname이다. 시스템 이름 해석기 사용 가능성을 외부 서비스 문서에 추가했다. 새 앞단은 실제 localAddress 장치를 요구하며 직접 DNS/폐기 공식 도메인 조회를 추가하지 않는다. 장치는 향후 실제 R 프로세스의 로컬 주소에 연결해야 한다. 이번에 실제 조회를 실행하지 않았다.

## 64비트 소켓 수명

원문은 inline socket의 핸들 DWORD를 읽고 옛 필드에-1을 써서 ClosePending이 OS 소켓을 닫지 않게 한 뒤 CCh에 넘긴다. 새 SocketDevice::Detach는 실제 uintptr_t 핸들과 WSA shared owner/현재 장치를 새 소유권으로 옮기고 옛 핸들만 무효화한다. OS socket/close/WSAStartup을 새로 호출하지 않는다. 먼저 할당을 마친 뒤 이전하므로 실패 시 옛 닫힘 권한을 잃지 않는다.

호스트 핸들을 DWORD로 자르거나 원격 패킷 숫자를 OS 핸들로 import하지 않는다. Root 요청 내부+29h에는 비재사용 private 토큰을 쓰고 실제 CCh가 ClaimSocket으로 한 번 소비한다. callback 대여 중 map이 핸들/WSA를 보유한다. 미소비/예외 경로는 소유권으로 닫으며, 원문의 비소유 CCh 분기에서 잃을 수 있는 핸들 누수 자체를 재현하지 않는다. 내부 요청은 mutable 소유 바이트로 복사하므로 원본 입력 버퍼 주소/OS 핸들 숫자의 바이트 동일성은 주장하지 않는다.

AdoptTransferredStream은 실제0044e9d3..e9e9 호출의 **flags444/capacity4096/이전핸들/sender0**를 구성한다. 기존 실제 노드의 두 시각/첫 peer/ff 입력·stream 출력 생성 뒤 별도 getpeername을 실행하고 IPv4일 때만 flags54를 쓴다. accepted가 상속했던 listener 비트를 새 CCh stream에 그대로 남기지 않는다. 실제 참가자 슬롯 공개/복합 sender/버퍼 모드는 후속0044e940 몸체가 수행해야 한다. 구성 API만으로 참가자 참여를 완료 처리하지 않는다.

## 확인 범위와 후속

Release/x64 NetstormCpp 빌드 종료0·경고/오류0, 보호2800파일 불변·새fixture0이다. 새 단위/MP/게임/통신/소켓 열기/DNS/원본R 실행 검사는 모두0이며 전체 검사/34미션/ZIP도 반복하지 않았다. 원문/PE·기계어 정적 대조와 컴파일 결과다. 실제 소켓 이전·로그인/종료의 OS/통신 동작 검증은 아니다. 물리 필드 부족/무효 토큰은 호스트 범위 예외로 막고 원문의 불법 주소 읽기/SEH·힙 배치 자체를 재현하지 않는다.

다음은 CCh0044e940의 실제 참가자 준비/데이터·목록·로그인 응답과0044e0d0 종료, 나머지 Root/CS 수신·시계/기록/주소·moderation/보조 객체와 새R.exe 전체 초기화/호출이다. 이어0F/1d 클라이언트 수신·메타/OS launch를 연결한다. 원시 함수/기계어·정적 대조/빌드 로그는 Git 제외 `extracted/cpp-resume-1068/`, 보고서는 `cpppj/root-login-request-20261107.json`이다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이며 재디컴파일/기존 산출물 수정은0이다.
