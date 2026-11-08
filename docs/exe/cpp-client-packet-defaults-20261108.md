# 1167 — M6-b1 Client 기본 등록과 실제 C3 수신

2026-11-08 / HJOW-Athlon. 이전 CPP1166/ede61f05, 제품11.01이다. 정밀 원문은 Client2026-11-03·R2026-11-07/같은 호스트의 기존 산출물이며 새 디컴파일/생성 소스 변경은0이다.

Client 원문40b4b0의 C3 수신7종(0c/42/4d/55/56/57/5a)을 실제 비NULL callable로 등록했다. 원문4437f0은 실제 PE의 C3 한 명령이다. NULL 수신과 다르므로 기존 Validate/시각/기록/수신 분배를 통과하되 게임 State·ACK·성공값을 만들지 않는 빈 몸체를 사용한다. 각 observer/version1·policy0/보호PE 길이표도 같은 실제 표에 설치한다. 0c/55/56/57은 DWORD 미정값 진단 후 현재 lowWORD를 다시 읽어 길이를 쓰며 나머지는 원문40b380의 비NULL 빈 관찰기다.

비폐기 관찰 전용11종(10/11/12/16/17/1c/1e/1f/68/85/87)도 정확 metadata/observer를 설치하며 receiver는 건드리지 않는다. R의 실제 수신을 Client에 합성하지 않는다. 16은 현재 signed BYTE*4+14,1c는 현재 unsigned BYTE+46,85는 현재 WORD+7이다. 진단이 복귀하면 같은 packet을 원문 폭으로 재조회하며 수신 방향0은 필드 접근 전에 반환한다. 폐기 CD검증7f/80과 다른 실제 기능 본문은 이 빈 수신 등록으로 채우지 않는다.

InstallClientPacketDefaults는1166의 네 mutable 관찰기를 재사용하고 위18행을 함께 설치한다. 일반 coordinator의 BindSession 전, Meta TCP/UDP 공유표 생성 후, Editor 생성 후 Open 전의 실제 세 경계에 연결했다. 짧은 Session/World/Meta this를 캡처하지 않고 진단에 필요한 장기 Client storage만 보유한다. 기존 Session 임대/snapshot/반납 뒤에도 기본표가 유지된다. generic PacketRegistrationTable 생성자와 R 등록은 바꾸지 않았다.

Meta의16/1c clog 전용 중복 본문을 제거해 같은 AssertDisplay 장치를 사용한다. 1e/1f의 NULL 덮기도 제거하고 기존 version/길이를 유지한다. 늦게 생성되는 내부 LocalMissionTransport의87 bootstrap은 비NULL 빈 관찰기를 사용해 공용 기본표를 NULL로 되돌리지 않게 했다. 새 전송/시간/기록 장치나 소켓을 만들지 않는다.

원문18행 등록·버전·실제 PE 길이와 관찰기7개/빈observer/C3의9개 실제 함수범위를 대조했다. 원시는 extracted/cpp-m6-registration-prep/의 PLAN·전체원문발췌·range별 PE·등록장부·준비/적용해시와 extracted/cpp-resume-1167/의 빌드·보호 장부다. 단일/집합/송신의 기존 시각/기록 순서나 M6-l 지연 큐는 이번 단위에서 구현한 것으로 세지 않는다.

NetstormCpp/NetstormRoot Release/x64 빌드종료0·경고/오류0, 보호2800파일 해시 불변이다. 새 단위/MP패킷/게임/통신/ZIP 실행0이며 사용자 지시로 MP 실행 검사는 생략했다. 이 단위는 검사 소스를 바꾸지 않아1166의 기존 검사 코드 컴파일을 다시 반복하지 않았다. 제품 해시와 원문 metadata는 [보고서](../../cpppj/client-packet-defaults-20261108.json)에 기록한다.

M6-b1 기본 등록은 완료했다. 기능 본문 미등록12종 중 별도 c~j에 속하지 않던0bh 버전조회는 M6-b2로 명시해 다음에 연결한다. 나머지 M6-c~n/M7/M8·한국어·시나리오UI는 미완료다.
