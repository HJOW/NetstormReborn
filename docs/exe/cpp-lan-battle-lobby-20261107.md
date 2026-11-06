# 999 — 로컬 LAN 대기실 명단과 시작 합의

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 원문004b4be0의 Zone 설치/71BYTE 명단,004b4ed0의 일반 예제 Zone 정책,004e2430과10.72의004bfa40의 listener 재사용을 참고했다.

폐기된 공식 매칭 서버를 대신하는 LanBattleLobby 부품을 작성하고 제품 라이브러리의 CMake에 등록했다. 생성은 소켓을 열지 않으며 Host/Join의 명시 호출이 필요하다. **실제 Uber/화면 호출자는 아직 없다.** 따라서 새 부품은 컴파일됐지만 실제 UI/LAN 플레이가 연결된 제품으로 보고하지 않는다. Native 게임의 Client 등록표/Player/서버 권한/Ready는 쓰지 않고 별도 대기실 등록표와 포트에서 기존 실제 TCP 프레임/수명을 재사용한다.

가입은 현재 Fort의 실제 subscriber/이름을 받고0/중복 ID·제어문자/표시 부호화31BYTE 초과 이름·8명 초과를 거부한다. 실제 TCP 접속 뒤 Join을 보내고 host가 실제 빈0..7 슬롯을 배정한다. 1a Zone의 실제 가변15+71×인원 길이와 고유 번호/ID/자기 포함을 확인한다.8명은583BYTE이므로512BYTE로 자르지 않는다. 일반 기본 정책 비트는 원문004b4ed0의 `0xb8000112`를 명시 로컬 기본값으로 사용하며 네 디버그 이름/ID를 복사하지 않는다.

11.01 로컬 대기실 확장은 별도 포트에서만 version1/type87 프레임의4BYTE 헤더 뒤 `NRL1` 서명4BYTE·명령1BYTE·게임포트WORD·payload를 사용한다. 명령은 Join1/Roster2/Start3/ACK4/Reject5다. Join payload는 subscriberDWORD+NUL 포함32BYTE 이름, Roster/Start는 원문 Zone 이미지, ACK는 Zone ID DWORD다. 이는 원문 공식 매칭 프로토콜을 그대로 복원한 것이 아니며 Native 게임의 Zsend 등록표는 변경하지 않는다. 대기실/게임 포트는1~65535의 서로 다른 값이고 주소는 기존 TCP의 사설/loopback/link-local IPv4 literal 제한을 따른다. DNS/외부 공식 서버/유효성 검사는 연결하지 않는다.

Start는 명단을 고정하고 실제 원격 참가자의 ACK와 송신 drain 뒤에만 외부에 요청을 인계한다. client는 실제 연결된 서버 주소를 캡처해 제어 연결 종료 뒤에도 보존한다. 대기 중 이탈은 명단에서 제거하고 시작 전 ACK 없는 이탈/접속·시작 timeout은 실패로 남긴다. 미가입 pending30초를 정리하고 잘못된 외부 제어 입력은 해당 연결에서 거부한다. Reject의 partial tail은 송신 완료 뒤 닫는다. 게임 World/State 교체는 TCP callback에서 하지 않는다.

기존 복합 Serve는 원문004e2430처럼 이미 있는 TCP/내부 listener를 재사용한다. 뒤 생성 실패가 앞서 열린 TCP를 임의로 되돌리지 않는다. TCP 수신 폴링은 음수 pending의 unbuffered partial 송신 tail도 전진시킨다. 원문 정상22칸 Flush에 pending을 끼워 넣지는 않았다.

첫/보완 후 Release 빌드 각각 종료0·경고/오류0이다. 제품11.01은11,691,008바이트, SHA-256은 `e0375403c0b9296945e05451db1d3d01967e5831d42ee0191c8a970777e67449`다. 보고서는 `cpppj/lan-battle-lobby-20261107.json`이다. 신규 MP/단위/게임/통신 검사·fixture 생성0, 보호2800파일 모두 불변이다. 다음은 대기실 화면/Host·Join·Start→실제 Uber Connect/State7 World 교체·현재 이름 인계다. 재접속/메타·전체 파생 writer/decoder·Outpost/checking8 등은 잔여다.
