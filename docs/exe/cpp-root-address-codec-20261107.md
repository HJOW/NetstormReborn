# 1072 — Root 주소 서식·해석과 실제 로컬 주소

`root::AddressCodec`이004864b0/00486870과 같은20BYTE 문법/서식 고리를 복원하고,004870d0의 OS 경계를 실제 Windows 로컬 어댑터 장치에 연결한다. 제품 기준1072 / 11.01, EXE 12,366,336바이트, SHA-256 `094944fa5d0e0b9fd2a533b09af017f35eb2614c0495d6a5379ab3f08bc379f5`다. 직전 CP는 `052b2278 / 20261107-121 CP`다. **새R.exe 최종 owner/실제 호출은 아직 없다.** 컴파일·정적 대조를 실제 로그인/통신·OS 조회 실행 검증으로 계산하지 않는다.

## 현재 주소 문법과 서식

서식은 실제5칸30BYTE 고리/현재 index를 사용한다. 프로토콜 우선순위는tcp10→udp20→spx400→ipx200→zlocal8이다. IPv4 주소0은'*', 그 외 실제 network BYTE의 점 표기이며 IPX는 raw little-endian network DWORD의 uppercase X와6개02X node다. port는family6의+12, 그 외+2에서 network endian을 읽는다. flags가 켜졌고 현재DWORD가0이 아니면 ',' 뒤 c/s/n/b/r 순서로 해당 비트만 붙인다. protocol bit만 있어도 끝 ','는 남는다. callback은 고리 결과를 즉시 소유 복사한다.

파서는80BYTE0 지역에 입력의 앞79BYTE만 복사한다. 첫 ','로 프로토콜을 분리하고 host+1부터 두 번째 ','를 찾는 원문 경계,소문자 플래그와s=82,명시port가0일 때만 문자열port 사용/WORD절삭,flags DWORD 보존을 따른다. '*' 선두는family0이면zlocal20/flag8로 바꾼다. '.'이 정확히 하나면 IPX로 판정하며, **strtol의 시작 포인터는 network 문자열 앞이 아닌 '.' 자체**다.486b45 strchr→486b4d 저장→486b54/57 같은 포인터push→486b58 strtol의 기계어로 확인한 network0 특이동작을 보존했다. node는6hex쌍과 잘못된 nibble -1의 BYTE 결과까지 유지한다.

그 외 숫자 주소는 원문과 같은 inet_addr를 사용한다. 더 엄격한 InetPton으로 교체해 축약/8진수/16진수 숫자 문법을 좁히지 않는다. 첫 빌드의 C4996은 이 파일의 _WINSOCK_DEPRECATED_NO_WARNINGS로 처리했으며 전체 경고를 끄지 않았다. NULL 입력/80BYTE 밖 읽기·서식30BYTE 넘침은 명시 호스트 물리 오류다. 구형 CRT·로케일/미정 바이트 자체의 전체 동일성은 증명하지 않는다.

## 외부 조회를 복구하지 않는 OS 장치

원문004870d0은 gethostname/gethostbyname으로 첫 IPv4를 받는다. 새 장치는 GetAdaptersAddresses AF_INET으로 실제 Up 어댑터의 현재 OS 반환 순서상 첫 loopback/RFC1918/link-local unicast를 선택한다. 성공은00486800처럼20BYTE를 초기화하고family2/IP·port0/flags0을 쓰며, 실패는 원문처럼 family WORD만0으로 쓴다. 임의 loopback 실패 기본값을 만들지 않는다. 다중 어댑터에서 구형 hostname resolver의 첫 주소와 같다는 보장은 하지 않는다.

숫자가 아닌 입력 중 localhost는127.0.0.1로 해석하고 현재 컴퓨터의 DNS short/FQDN/NetBIOS 이름은 GetComputerNameExW의 실제 설정과 위 어댑터 주소를 사용한다. 사용자 hosts 파일의 localhost 재정의는 조회하지 않는다. **다른 LAN/공개 hostname은 DNS 조회하지 않고 원문 해석 실패의 IPv4주소0 경로를 사용한다.** 현재 LAN 접속에는 숫자 IPv4를 사용한다. 다른 LAN hostname의 외부 DNS 없는 실제 해석은 잔여 구현이며 목표에서 제외하지 않는다. 기존 SocketDevice의 실제 접속/송신 로컬 주소 정책은 유지한다. IPX/SPX/가상 주소 문자열 지원을 해당 OS 소켓·외부 서비스 구현 완료로 계산하지 않는다. [외부 서비스 분류](cpp-external-services-20261101.md)에 경계를 기록했다.

Bind는 실제 LoginRequest.localAddress와 CCh addressText/parseAddress를 같은 codec owner에 연결한다. 최종 R owner가 이 hook을 LoginRequest/ChallengeRuntime 생성 전에 적용하고 충분히 오래 유지해야 한다. 아직 최종 초기화/호출이 없다.

## 확인과 다음 작업

Release/x64 첫 빌드 종료0·C4996경고1, 숫자 문법을 유지한 최종 빌드 종료0·경고/오류0이다. 보호2800파일 불변·새fixture0이다. **새 단위/MP/게임/통신·소켓/DNS·OS 어댑터 조회 실행/원본R 검사는0**이며 전체 검사/34미션/ZIP도 반복하지 않았다. 다음은 다른 LAN hostname의 외부 DNS 없는 해석·시계/기록/보조 객체·나머지 Root/CS 수신/입력,새R.exe 전체 초기화/최종 owner와 실제 장치·파일 watcher 주기/호출,0F/1d 수신·메타/OS launch다.

원문/PE·기계어·정적 감사/빌드 로그는 Git 제외 `extracted/cpp-resume-1072/`, 추적 보고서는 `cpppj/root-address-codec-20261107.json`이다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이며 같은 호스트의 기존 산출물만 읽었다. 재디컴파일/기존 산출물 변경0이다.
