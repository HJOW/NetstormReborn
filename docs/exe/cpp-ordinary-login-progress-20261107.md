# 1002 — 일반 로그인 대기 화면과 강제 시작

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다.10.78의004b67c0/00490c50/00490cf0/004c8d90/004cf570/004d62b0과 보호 PE를 읽기 전용으로 대조했다. CD 대응은 타이머004336e0·Pending00407aa0(medium0.438)·인자 serial 닫기004c3820(strong1.0)이다. Connecting 집계의 CD 대응은 없다. 구현 판본은10.78이다.

같은 현재9×180BYTE Player에서 owner1..8의 실제 subscriber/이름/flags를 읽어 Connecting·Pending·Disconnected·Transferring·Ready 문구를 번역한다. Pending은 subscriber/flags2가 있고 flags1/100이 모두 없는 기록, Connecting은 subscriber가 있고 flags2가 없는 기록이다. 원문은 주소 객체를 지역에 작성하지만 서식/화면에는 쓰지 않는다. 이 버려지는 주소 변환과 hostname DNS는 생성하지 않았다. 이는 로컬 LAN 외 도메인 접속을 추가하지 않는 호환 차이이며 주소 표시를 임의 추가한 결과가 아니다.

PE4b6a8b/93은 end005c85f0에서 프레임 벽시각0055b4d8을 뺀 값을 x87에 보존한 채4e49c0(_ftol)에 전달한다. 같은 Client.Time.wall로 계산하고 signed64 정수부의 하위DWORD를 signed로 해석한 뒤 음수만0으로 바꾼다.0이면 원문 지연/Abort 안내를 쓰며, 현재 서버에서는 Disconnected→Subscribers→Pending→Connecting→Ready 순서로 집계하고 차이가Ready이며 timeout-remaining>10일 때 강제 시작 안내로 덮어쓴다. 최초 비서버 제목과 서버 분기의 번역 후 재조회도 구별한다.

실제 제목/명단/정수/추가안내 네 인자 서식을 번역하고 같은 cf570/HTML·메타·ButtonInstant/일반 입력 큐에 전달한다. 원문의1024BYTE 논리 버퍼를 넘는 이름/목록/본문과 NUL 누락, 실제 네 인자 외 CRT 서식은 명시 거부한다. 같은 transient005cab34의 실제 반환 serial을 별도 Client 수명005c85b0에 저장한다. 완료/Abort는4c8d90처럼 그 serial의 실제 Gump를 Kill하고 해당DWORD만0으로 만든다. transient를 함께0으로 바꾸거나 다른 창을 닫지 않는다. 기존 State13의 공통 안내 닫기는 유지한다.

PE 이름표00543330은 ServerForceBattleStart→00542ac8→020000f5다. 일반 입력은 원문처럼 뗌을 거부하고 Begin005c7a74만1로 만든다. 버튼 수신 중 패킷/월드교체를 실행하지 않으며 다음 기존 타이머가 실제 Begin6e/완료 경로를 처리한다. **일반 재접속 저장004b54e0은 아직 명시 미연결이며 다음 작업이다.** 전체 decoder·메타/재접속·Outpost/checking8·한국어 후속·시나리오 UI도 남는다.

Release 제품 빌드는 종료0·경고/오류0이다. 제품11.01은11,798,016바이트, SHA-256 `c7b2cd419085e25893ee735bb589b446c41efda53a7f0c8306f136f29aad1730`다. 보고서는 `cpppj/ordinary-login-progress-20261107.json`이다. 신규 MP/단위/게임/통신 검사·fixture 생성0이며 보호2800파일은 모두 불변이다. 화면/버튼/실제 LAN 플레이 동작을 실행 확인한 결과가 아니다.
