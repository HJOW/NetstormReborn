# 1056 — 실제 메타 연결20~27와 TCP10/11 요청

1055(`c1c8bd0a / 20261107-72 CP`)의 실제 State19 다음에 메타 연결20~27/서버 행·endpoint/16h·1ch 요청을 접합했다. 현재 Client의 실제 Player/상태/연결표·Core·시계와 기존 TCP를 사용한다. **State34~38 로컬 검색/서버 시작, 로그인 응답17h/1dh·State28~33/메타 표시는 아직 미연결**이다. 멀티플레이 실행/통신/새 단위 검사는0이며 전체 메타 진입이나 LAN 플레이 완료가 아니다.

## 서버 행과 endpoint

00458f90의 무효 인덱스는 출력도 보존한다. 유효 행의00458e70만 선택 출력 버퍼/subscriber를0으로 만들고 `([^;]*);([^;]*);?(.*)?`의 첫 일치/탐욕 그룹을 읽는다. 첫 두 그룹은 목적 크기-1까지 복사/NUL, subscriber는 원문 임시80BYTE/최대79에 옮겨 기존 CRT32비트 parser를 쓴다. NSInfo 이름 조회도 같은 실제 parser를 재사용한다.

004b3ed0은 먼저 원래 C문자열을 지역80BYTE에 복사한다. `([^:]*):([0-9]+)` 첫 일치를 선택하며 앞서 실패한 콜론을 host에 합치지 않는다. 숫자 그룹은 연속 십진 부분만 읽는다. 매치가 없을 때만 현재 rootServerBasePort를 조회하며, 명시 포트가 비영이어도 그 앞의 해석/조회는 생략하지 않는다. 마지막에 명시 포트를 우선한다. 지역 물리 밖 쓰기는 명시 거부한다. 빈 capture는 원문처럼 복사를 건너뛰고 NUL만 쓴다. 이 두 고정 패턴을 직접 계산했으며 범용 정규식 엔진을 새로 흉내 내지 않는다.

현재 host/port는 기존 LocalTcpTransport.ConnectPeerText의 실제 비영 준비 재사용→IPv4 LAN 검증→새 socket 순서로 전달한다. 기존 tcp, 접두와 문자 주소 해석도 재사용한다. 공개 주소/도메인·잘못된 포트는 로그/false로 반납하며 DNS/외부 접속을 하지 않는다. 보호 기본 목록의 `netstorm.game-host.org:6800`, `play.netstormworld.com:6800`을 로컬 주소나 성공값으로 바꾸지 않았다. 기존 내부/Game LAN 대기실을 메타 root 서버 응답으로 대체하지 않았다.

## 실제 상태·TCP/현재 Client 수명

원문 점프 표004b874c를 확인했다.20은 실패 안내→다음3,21은 현재 serverNum 행의 endpoint→연결10→다음22,22는 준비 조회→현재 open 확인→실패 시 disconnect10/안내/즉시20·진행0이면다음22·양수면16h/즉시23이다.23은 응답 대기 안내만 하며24는즉시25다.25는현재 serverNum의 원래 전체 행→연결11→다음26,26은실패 시disconnect11/다음38·진행0이면다음26·양수면1ch/대기 안내/즉시27이다. **27의 점프 대상은004b86bd 공통 확정**이며 새 대기 성공 상태를 합성하지 않는다.25의 무효 인덱스는 원문 진단 뒤 NULL 읽기 지점에서 물리 오류로 중단하며 임의 빈 행을 만들지 않는다.

실제 Uber의 같은 장기 분배기/큰 부모20~27와 현재 transient HTML/즉시 요청을 연결했다. TCP owner 생성만으로는 소켓을 열지 않는다. World 없는 slot4에서 flush→수신을 진행하며 응답 예약을 그 자리에서 State로 중첩 실행하지 않는다. 기본 type2 종료는 기존 실제 ReceiveConnectionEnded를 쓰므로 sender10/phase23의 종료는 다음20을 예약한다. 게임9/Player1..8 전용 종료 부모는 메타 owner에서 선택하면 명시 거부한다.

Native cleanup은 reason과 무관하게10/11을 끊은 뒤 Player를 재할당한다. 그 전체 부모가 끝난 뒤 메타 TCP owner를 반납하므로 이전 Player span을 다시 읽지 않는다. callback 중 Tick/전체 정리 진입도 막는다. 메타와 게임 World가 공존하는 새 coordinator를 가정하지 않고 실제19의 World 정리 뒤만 이 owner를 설치한다. **17h/1dh와 나머지 메타 수신/목록·채팅/선택적 packet-record writer는 아직 연결하지 않았다.** 연결 준비/요청 송신을 로그인 응답/CS 목록 설치로 계산하지 않는다. 코드상21 실패는20→3이며26 실패의38은 아직 남는다. 실제 GUI/통신으로 재현하지 않았다.

## 요청/observer

004b40c0의16h는 물리78BYTE 중 header/version1/초기길이0→종류BYTE0→현재 phase+44 서버 subscriber→친구 수1→현재 자기 subscriber→현재 Core 기술 합계 DWORD를 쓴다.0040a190 observer는 송신에서만 signed 친구 수×4+14를 선언WORD에 기록해 실제18BYTE를 전달한다.0xff 진단 정상 복귀 뒤 계산도 보존한다.

004b4480의1ch는 현재 Chat manager 재생성 뒤 물리300BYTE/초기길이0을 쓰고 Native 프로토콜10/78/78, 자기 subscriber(0만ffffffff), 현재+34 하위BYTE/+48 DWORD/+4c WORD, 현재 Core10BYTE/다시 조회한 기술 합계·현재005435b4 언어, CD/현재 passedDiags 정보 비트, 같은 현재 handle/NUL 포함 길이를 순서대로 쓴다.0040a270 observer는 unsigned 이름 길이+46을 확정한다. 미작성 **+29h DWORD와 다른 scratch**는 기존 현재 할당 입력으로 보존했으며 Native x86 스택의 모든 미정 비트 일치를 주장하지 않는다. 실제 패킷은 같은 mutable observer/Player·연결 gate와 partial send 장치를 사용한다.

제품 표시/파일 버전은 **11.01**이며 위10/78은 원본10.78 wire 호환 필드다. 현재 언어와 공개 LanguageNumber는 서로 다른 원문 DWORD여서 새 getter로 구분했다. passedDiags는 PE BSS0/같은 현재 옵션 슬롯을 연결했으며 원본 초기 DiagResults 레지스트리 reader/진단 실행은 이번에 추가하지 않았다. 이 정보 비트 때문에 원격 클라이언트 유효성 검사를 실행하거나 성공1을 합성하지 않는다. 폐기7f/80과 Auto Updater 검증은 계속 비활성화한다.

## State28 정정과 CD 비교

이전1053의 다른 CS reset 부모를 State30으로 적은 것은 잘못이었다. 보호 PE 표에서 **State28→004b833b→0042ba10**을 확인했다.1053 보고서/문서와 현재 인계를 정정했다. State28의 직접Reset은 State19 Prepare의 생성/listsInitialized 진단 경로를 재사용하는 계약이 아니다. 이는 기능을 바꾸지 않는 설명 정정이다.

CD 비교 함수는004cfc20/004cfd30/00430b70/00430e50/004311d0이다. 두 문자열 패턴/포트 선택 흐름은 같지만 CD root 요청은 친구 수0이며 PATCH의 자기 subscriber 한 항목을 쓰지 않는다. CD 비교본의 challenge 요청에는 관측된 minor25h/최소20h 상수가 있으므로 PATCH의78을 CD에 적용하지 않는다. CD 전체 복원/판본 해석은 별도 후순위다.

첫 빌드는 namespace 닫힘 누락 C1075로 실패했고, 수정 후 종료0에서 char 버퍼 fill의 C4244 두 건을 확인했다. 명시 char 초기값으로 교정한 빈 capture/TCP 문자열 접두 보완 뒤 확인한 최종 NetstormCpp Release는 **종료0·경고/오류0**이다. 실행파일 12,292,096바이트/SHA-256 `692c04d1680c4eecf404ea83fc70fb2a5a7051b1b35cee39c3c56ce1d4456fc1`. 보호 2800파일 불변·새fixture0·새 단위/게임/통신 검사0이다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이며 이번 산출물 변경은 없다. 전체 솔루션/플레이/배포 ZIP 검증으로 확대하지 않는다.
