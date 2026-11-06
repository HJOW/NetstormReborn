# 1008 — 같은 LAN 전송 owner의 서버 역할 전환

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 10.78의004b6dd0 case15와004e2430의 서버 이전 경계를 대조했다. 원문 State15는 같은 월드/등록표를 보유한 채 실제 server/boss를 쓰고 listener/내부 자기 연결을 연다. 이번 변경은 그 실제 Serve backend의 선행 연결이며 전체 State15 몸체는 다음 작업이다.

CompositeMissionTransport는 처음 선택한 LanClient에서도 상위 원문이 같은 실제 server 참조를 참으로 쓴 뒤 Serve를 요청하면 기존 TCP listener를 먼저 열거나 유지하고, 같은 현재 Player1620BYTE/PacketDispatchHooks/Registry/PacketRegistrationTable로 내부 전송 owner를 늦게 만든다. 그 생성이 성공한 뒤 역할을 LanHost로 바꾸고 기존 Serve가 내부 listener를 연다. 실제 client9/accepted/63h는 아직 이 함수가 만들지 않는다.

현재 서버 권한 없는 client 요청은 계속 명시 거부한다. callback 실행 중 역할 전환도 거부해 별도 상위 State 요청을 callback 복귀 뒤에 처리하게 한다. 같은 연결/패킷 등록 대여와 World/Client 수명을 유지하고 새 Player/Ready/서버 권한을 합성하지 않는다. 기존 TCP/내부 listener는 중복 생성하지 않으며 뒤 내부 할당/열기 실패로 앞선 TCP 열기 성공을 되돌리지 않는다. 직접 host/단독/Editor의 기존 선택은 유지한다. 원본의 폐기 공식 매칭/SPX를 연결하지 않는다.

**State15 부모/실제63h 송수신·StateReconnect/ForceServerDeath 입력과004b8cb0·복구State7 인계는 아직 남는다.** 이번 단위를 실제 서버 이전/재접속 동작 완료로 쓰지 않는다. 일반 프레임486bc0/스펠·전체 decoder·메타·Outpost/checking7·한국어 후속·Edit 시나리오 UI도 이어간다.

Release 제품 빌드 종료0·경고/오류0이다. 제품11.01은11,849,216바이트, SHA-256 `b8106ce08930c84b2428c48bab037c566df4b727856a8c27beb60674053279e5`다. 보고서는 `cpppj/lan-server-promotion-20261107.json`이다. 신규 MP/단위/게임/통신 검사·fixture 생성0이며 보호2800파일은 모두 불변이다.
