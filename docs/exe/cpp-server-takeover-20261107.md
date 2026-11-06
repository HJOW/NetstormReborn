# 1010 — 실제 State15 서버 인수와63h 송신

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 10.78의004b6dd0 case15/16·004e29c0/004e2430/004e2870·0040adc0·004af1a0/004abe00·004c1f20을 대조했다. 보호 PE4b7ad1 이후 실제 몸체와005140b8의 `Server takeover` 로그,005140a8의 zlocal 이름 형식을 확인했다. 이번 구현은10.78이다.

같은 Login의 StateHooks에15번 몸체를 연결하고16은14와 같은 공통 확정 경로에 등록했다. TakeServerForState의 실제 외부 진입은 현재15를 쓰고 기존 RawLocalMissionState.Run을 호출한다. 별도 World/Player를 만들지 않으며 현재144BYTE 공통부가15와 즉시16을 순서대로 확정한다. 실행 중 callback/State 재진입은 호스트 수명 경계에서 거부하고 상위 재접속 caller를 외부 입력/Poll에서 호출하도록 한다.

몸체는 채널1d 실제 로그→savedGame1→같은Registry.CloseAll의 양수22칸→현재server/boss1→MessageBoard 종류1/player0/표시0/parameter0→현재 포트Serve→현재 포트 재조회/실제 내부client9 등록→현재MySubscriber를 붙인63h/1/8 송신 순서다. 등록/observer/송신 큐는1008/1009의 같은 실제 장치를 사용한다. Native4e2870은 buffered 전환이 아니라 실제transceivers[id] 등록이며 기존 내부Connect.Register(9)가 같은 역할을 수행한다. zlocal은 기존 typed backend API를 쓰며 폐기SPX/공식매칭을 연결하지 않는다.

같은 현재owner1..8에서 subscriber가 있고flags2/미flags1이면100을 켜고 모든 flags는702만 보존한다. 그 뒤 실제 현재server 진단→기존SidPool.RebuildServer→기존로그인타이머 시작을 수행한다. 원문 freeCount의 증가를 reset으로 정규화하지 않는다. 그 시점GameTicks를 보존하고 기존ResetClockSynchronization의 정확한 세DWORD를 지운 뒤 같은미니맵revision++→즉시16 요청→같은GameTicks 복구를 수행한다. 카메라/World/PlayerKnown/성공Ready를 임의로 덮어쓰지 않는다.

**StateReconnect/ForceServerDeath와004b8cb0의 실제 서버 선택/caller·복구State7/전역handle 수명 인계는 아직 남는다.** 이 API/상태 몸체 연결을 실제 서버 이전 또는 재접속 동작 검증 완료로 쓰지 않는다. 일반 프레임486bc0/스펠·전체 decoder·메타·Outpost/checking6·한국어 후속·Edit 시나리오 UI도 이어간다.

Release 제품 빌드 종료0·경고/오류0이다. 제품11.01은11,859,968바이트, SHA-256 `347a5ac1c46eb023dc8f2869b9f8f8e579a97b1b7a1dad5fada886d8508d6c09`다. 보고서는 `cpppj/server-takeover-20261107.json`이다. 신규 MP/단위/게임/통신 검사·fixture 생성0이며 보호2800파일은 모두 불변이다.
