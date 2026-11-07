# 1047 — 기존 연결 재사용과 현재 socket 준비 조회

1046에서 남긴004e2620의 기존 연결 guard/새 연결 공개와004e1800/004e2b70 준비 상태를 실제 backend에 연결했다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이다.004e2620/004e1800/004e1770/004e2b70/004e2430의 정밀 C와 보호 PE를 읽기 전용으로 대조했으며 산출물을 변경하지 않았다.

기존 코드는 현재 ID칸이 비어 있어야만 TCP Connect를 허용했고 준비 상태는 error/connected 캐시로 반환했다. 원문은 **현재 칸이 비영이고 그 실제 socket 준비값이 비영이면 주소·포트·초기 패킷 해석 전에1을 반환**한다. 준비 조회의-1도 이 guard에서는 비영이다. 이를 새 연결 완료의 합성 성공으로 해석하지 않는다. Connect의bool은 원문 해당 함수의1/0이고 실제 연결 상태는 별도 조회/Poll이 다룬다. 현재 문자열/20BYTE/저장 주소 세 입구는 guard를 먼저 한 번만 실행한다.

Registry는 각 token의 실제 TCP/내부/Editor 준비 장치를 함께 보관한다. 다른 backend가 소유한 현재 노드도 이 장치로 조회한다. 이미 선택한 node를 현재 ID로 다시 선택하거나 PacketConnection flags/Player bit를 socket 준비로 대신하지 않는다. TCP는 현재 OS socket의 writable/exception을 select로 조회한다. 원문timeout8BYTE는 초기0이며 정밀 C 직접 참조 네 곳은 select 입력이다. SO_ERROR 추가 조회와 이전 connected/error 캐시를 제거했다. 오류/exception이면 원문4e1770처럼 socket을 닫고 무효 핸들로 바꾼 뒤 현재 WSAGetLastError를 저장하며-1을 반환한다. 조회 자체는 type1/2나 PacketConnection 반납을 실행하지 않는다. 다음 실제 Poll이 닫힌 socket의 종료 통지를 소비한다.

준비0/빈 칸은 새 socket을 만든다. socket/ioctl/connect의 통상 실패는 false이며 현재 노드/초기 패킷 칸은 보존하고 새 임시 핸들만 반납한다. 새 노드가 성공한 뒤에만 현재 transceiver칸을 **직접 store**하고1046의 기존 초기 버퍼free/0→observer/복제를 수행한다. 원문은 이 위치에서004e2870 일반 Register를 호출하지 않으므로 기존 칸 비영 진단을 추가하지 않는다. 이전 노드를 미리 닫거나 새 생성 실패에 앞서 지우지 않는다. 덮어쓴 기존 노드의 잔류 물리는 C++최종 transport owner가 반납하는 호스트 수명 경계이며 원문 잃어버린32비트 포인터/OS 자원 누수 자체 동일성 주장이 아니다. 등록되지 않은 token·장치/호스트 밖 접근은 명시 오류다. 로컬IPv4/DNS·공개주소 제한은 기존 LAN 정책이다.

004e2430의 TCP listener guard도 현재 핸들이 있으면 새 주소/포트 해석 없이 자연 반환하도록 맞췄다. 전체 listener/SPX·accept의 나머지, endpoint/server/boss와 전체 전송·프로세스 writer/decoder·메타/Outpost/checking5를 이 guard 변경만으로 완료 처리하지 않는다.

NetstormCpp Release 빌드는 **종료0·경고/오류0**, EXE 12,168,192바이트/SHA-256 `0f005d7ebecc7aa41f43c5bf5b785099bc8e6833aea4531c5881177838564382`다. 보호 2800파일 불변·새 fixture0이다. 신규 MP/단위/게임/통신·실제 재접속/준비 오류 검사는 지시대로0이다. 이번 기록은 정적 대조·코드 연결/컴파일이며 LAN 동작/안정성 검증 완료가 아니다. 다음은 나머지 LAN→한국어 후속→Edit 시나리오 UI다.
