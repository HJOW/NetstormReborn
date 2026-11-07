# 1067 — R 실제 CCh 저장소와 통신 접합

`root::ChallengeState`와 `ChallengeStore`가 실제 R CCh 기록/최초 좌표 표·CRT 소유권/숫자 토큰·Root 단일 프로세스 생성과40개 전송/정리를 구현한다. 제품 기준은1067 / 11.01, 게임 EXE 12,365,824바이트, SHA-256 `4181cd296a8e5b266767a3b0d95a91323f5b632b8332286813a65ab59211eab2`다. 직전 CP는 `9e65b735 / 20261107-101 CP`다. **새 R.exe/실제 초기화·호출은 아직 없다.** 현재 부품을 서버 실행/로그인 성공으로 계산하지 않는다.

## 실제 물리와 최초 생성

R CCh는2b80BYTE이며 전투10×344BYTE는+8, 참가자40×188BYTE는+d78, 전송 토큰40DWORD는+2ad8, 카운터는+2b78이다. 클라이언트21328BYTE/292·460BYTE 배열을 재사용하지 않는다.0044cfb0처럼 헤더8→참가자1d60→전투d70→전송a0→카운터4 순서로 지우고 **마지막+2b7c DWORD는 덮지 않는다.**0044ce90/cee0의 자식 header/고정 값/10BYTE 초기화는 뒤 Reset이 모두 덮으며 외부 효과가 없어 최종 쓰기에 통합했다.

실제 zero 플래그 없는 CRT malloc 입력을 생성자에 넘긴다. 호스트 포인터를 DWORD로 자르지 않고 비재사용 숫자 토큰으로 현재 물리를 찾는다. 소유 바이트/RootDirectoryView는 같은 물리를 빌린다. 할당 끝의 미정 CRT 바이트가 원본32비트 힙과 같다는 의미는 아니다. 물리 크기/현재 토큰/잘못된 번호는 호스트 범위 예외로 막는다.

좌표 표는 같은 프로세스의 initialized가0일 때만 구성하고 객체 교체/Shutdown에서 초기화하지 않는다. R PE double 각도0.7853975/float 증가량3f32b8b8·13/2/26과 실제 기계어를 대조했다. sqrt20 결과를 float32에 저장한 뒤 floor→FTOL4, 반경/중심/고리 반경과 **매 누적 각도를 float32에 저장**하는 원문 위치를 보존한다.4×4 순회를 반복해 grid1..39, 고리1..9만 쓰고 두 배열0번은 보존한다. 두 반경/원점/FTOL 크기 뒤에 initialized1을 쓴다. 호스트 long double/표준 수학 라이브러리의 모든 x87 내부/예외 비트가 일치한다는 주장은 하지 않는다.

## Root 생성과 현재 통신

InitializeRoot는 Root bit가 있을 때만 실제 원문 순서를 수행한다. 현재 count/PP 로그→상한2015 진단→PP=count→원문의 count/count 단일 프로세스 배치→현재 processOwners0→상한2115 진단→1..count-1 CCh 생성/표 공개/index/flags/Challenge bit OR다. count0은 원문의0나누기에 대응하는 호스트 오류다. 유효 count에서 원문 numProcesses는 항상1이라 실행 불가능한 다중 프로세스 분기를 새로 만들지 않는다. 전체 R 초기화 앞부분의 processOwners 전체0은 상위 부모가 별도로 수행해야 한다.

BindNetwork는 같은 현재 CCh40칸 조회와004578f0 수신/004579d0 flush를 실제 TransceiverTransport에 연결한다. 각 callback 뒤 다음 현재 슬롯을 다시 읽으며0번도 포함한다. NetworkPump는 이미 수신 뒤 같은 현재 Challenge를 재조회하므로 기존 부모 순서가 유지된다. 실제 보조 객체/시계·상위 장치는 caller가 제공한다. RootDirectory도 같은 Bytes 대여 장치를 받아야 하며 CCh 사본을 만들지 않는다.

명시 Shutdown은00478af0의 CCh1..50을 순회하고0044cf30처럼 연결0..39의 현재 비NULL 노드를 Close→같은 슬롯0으로 쓴 뒤 CCh 물리를 반납하고 Root 슬롯0을 쓴다. 원문의 일반256 노드 선행 정리와 보조 객체 후행 정리는 상위 부모다. 전송 callback 중 CCh 물리 반납은 거부한다. C++ 최종 소멸자는 예외 종료의 자기 물리/Root 별칭만 반납하며 사용자 callback을 재실행하지 않는다. 정상 종료의 flush/close를 이 경계로 대신하지 않는다. Root/Network 대여와 실제 표는 소유자보다 짧게 살아야 한다.

## 확인 범위와 다음 작업

Release/x64 NetstormCpp 빌드 종료0·경고/오류0, 보호2800파일 불변·새fixture0이다. **새 단위/MP/게임/통신/소켓 열기/원본R 실행 검사는0**이고 전체 검사/34미션/ZIP도 반복하지 않았다. 원문/PE·기계어 정적 대조와 컴파일 결과다. 실제 CCh 생성/통신/정리·좌표 수치 실행 대조는 하지 않았다. 콜백 예외의 부분 닫힘/원본 SEH·힙 배치까지 검증한 것은 아니다.

다음은 Root/CS 나머지 수신·참가자 로그인/종료, 실제 시계·기록/주소 formatter·moderation/보조 객체와 새R.exe 전체 초기화/정리·호출이다. 이어 클라이언트0F 존47/3BYTE 차이·0F/1d 수신·메타/OS launch를 연결한다. 원시 함수/PE·생성자 기계어·정적 범위 기록/빌드 로그는 Git 제외 `extracted/cpp-resume-1067/`, 보고서는 `cpppj/root-challenge-store-20261107.json`이다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이며 재디컴파일/기존 산출물 수정은0이다.
