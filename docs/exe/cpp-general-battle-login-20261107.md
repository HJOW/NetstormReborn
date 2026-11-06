# 998 — 일반 LAN의 State7/Login 후반

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 10.78의004b6dd0 case7/004b4be0과10.72의0042d630을 대조했다. 정적 원문 발췌는 `extracted/cpp-resume-998/state-parent.c`와 `zone.c`다.

GeneralBattle 설치와 GeneralBattleStartRequest를 단독 미션·편집 설치와 구별했다. 실제 State7 정리 뒤 같은 Client/Player/Core/SP/Kernel/표면을 대여하며 미션 파일 전체 Load를 호출하지 않는다. 준비된 기존 Fort 전용이던 BridgeDecay의 private editor 별칭은 일반 비편집 값에 맞춘다. 같은 공유 모드 값이나 Core/Player를 다시 초기화하지 않는다.

Login 후반은 정리 전 캡처한 Zone 표식으로 가변 호스트 명단 설치와 client의 Zone 부재를 구별한다. 단독 미션의86BYTE/네 정적 이름 fallback은 사용하지 않는다. 공통 Zone 설치는 기존 실제004b4be0 몸체를 공유한다. simulconnect listener→선택 Zone 설치/해제·서버 시간 등록→Renderer/known0/UI2/기한/즉시8→일반 서버 밀도·채널40/80 정리·안내/팔레트 순서를 유지한다. 재접속의00460980은 파일 Reload로 대신하지 않고 별도의 현재 이름 선택 장치를 요구한다.

일반 빈 월드의 실제 건설/이동/커서 등 게임 모듈과 같은 명령 수신기를 준비한다. 내부 loaded는 모듈 존재를 표시할 뿐이고 GameWorld connecting·실제 phase14/PlayerKnown/참여 bit1의 완료 관문은 계속 유지한다. 서버 권한/Player 성공/Ready를 합성하지 않는다.

**실제 Uber Connect/World 교체 부모·LAN 대기실/참가자 명단 공장은 아직 연결 전이다.** 원문 Login은 이미 Zone에 등록된 subscriber만 받으므로 임의 자기 명단 하나로 원격 참가를 성공시키지 않는다. 다음 단위에서 LAN 대기실과 이 앞단을 이어간다. 원문004e2430은 이미 존재하는 listener를 재사용하므로 복합 LAN의 simulconnect→Serve 중복 listener도 후속 보완한다. 재접속 현재 이름 선택·전체 파생 writer/decoder·메타·저장/재접속·Outpost/checking8은 잔여다.

첫/보완 후 Release 빌드 모두 종료0·경고/오류0이다. 제품11.01은11,691,520바이트, SHA-256은 `889c9363ba7e84ba2721a3d09c488f317a233884f1f71911435b1c2e9174c195`다. 보고서는 `cpppj/general-battle-login-20261107.json`이다. 신규 MP/단위/게임/통신 검사 및 fixture 생성0이며 보호2800파일은 모두 불변이다. 정적 구현/컴파일 결과를 일반 전투나 LAN 동작 검증으로 세지 않는다.
