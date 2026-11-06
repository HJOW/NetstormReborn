# 1012 — 원문 저장 주소 문자열과 typed LAN 연결

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 10.78의004e2620/004e0cd0/004e0a00·004b8cb0을 대조했다. CD 연결 대응은004bfc60(strong0.817), 문자열 parser는004a7030(strong0.87)이며 구현 판본은10.78이다.

기존 State8 서비스의20BYTE는 clone의 typed backend 연결 계약이다. 원문4b8cb0이 Player+30의 문자열을 복사하는 next+54..7b는 포트DWORD+7c 앞의40BYTE이며,4e2620은 그 문자열을4e0cd0으로 해석한 뒤4e0a00으로 별도 현재 포트를 적용한다. 원문의20BYTE Zaddress 지역 객체와 입력 문자열 영역을 동일시하지 않는다. 과거 서비스 주석의20BYTE 표현도 이 차이를 명시하도록 고쳤다.

실제 Login.Connect는 현재8/기존 typed 인자를 확인하고 current+84의 재접속 표식이 있을 때 같은 현재 블록의40BYTE에서 첫NUL을 찾아 connectPeerText에 전달한다. 잘린20BYTE 인자로 문자열을 해석하거나 임의0패딩을 하지 않는다. 표식이 없는 기존 private LAN/단독 시작은 기존 typed 연결을 유지한다. 원본과 별개의 사설 LAN 시작 프로토콜을 구 클라이언트와 호환된다고 표현하지 않는다.

TCP의 실제 문자열 어댑터는 저장 파일의 IP:port 또는tcp,IP:port,cn에서 IPv4 부분을 뽑아 현재 별도 포트로 기존Connect를 호출한다. 문자열의 임베디드 포트는 원문처럼 나중 별도 포트로 덮어쓴다. 현재 지원하는 사설/loopback/link-local IPv4·주소/포트 검사와 비동기 socket/같은ID 등록은 기존 실제 장치를 사용한다. DNS/공개주소/폐기SPX/UDP를 추가하지 않는다. 이는 기존LAN 지원 범위의 문자열 연결이며 원문 전체Zaddress parser 복원 완료가 아니다.

복합/독립TCP/내부 선택의 서비스 바인더도 같은 실제 backend에 연결했다. 내부 역할은 기존 zlocal client9만 만들고 TCP/DNS로 바꾸지 않으며 Editor에는 로그인 문자열 연결을 제공하지 않는다. 연결 성공/Ready/Player를 새로 쓰지 않는다. 본문 빌드 뒤 서비스 주석만 정정했으며 실행 코드는 바꾸지 않았다.

**StateReconnect/ForceServerDeath/004b8cb0의 실제 caller와 복구State7 선택/현재Fort 이름 인계는 아직 남는다.** 이 문자열 어댑터를 실제 재접속 동작 검증 완료로 쓰지 않는다. 일반 프레임486bc0/스펠·전체 decoder·메타·Outpost/checking6·한국어 후속·Edit 시나리오 UI도 이어간다.

Release 제품 빌드 종료0·경고/오류0이다. 제품11.01은11,863,040바이트, SHA-256 `85452c49ac008aa56cea05b637aa6eb9430e0407f65addc2b78968b005b77c1d`다. 보고서는 `cpppj/reconnect-address-text-20261107.json`이다. 신규 MP/단위/게임/통신 검사·fixture 생성0이며 보호2800파일은 모두 불변이다.
