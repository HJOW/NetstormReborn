# 1000 — 실제 LAN 화면과 일반 State7 World 교체

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 원문004d62b0의 Connect/lastMultiplayerFort→pending/004b3550(next6),004b6dd0 case7과004b4be0,10.72의0042d630을 참고했다. 공식 매칭 화면은 종료된 서버 대신999의 명시 로컬 대기실로 교체했다.

10.78 메인 메뉴 Multiplayer를 더 이상 NotImplemented로 보내지 않는다. 같은 일반 입력 큐의 Connect0→마지막 Fort/pending/next6→996/997의 실제 Fort/옵션 창을 사용한다. Connect1은 원문 옵션 버튼이며2/3/4/5/6은 로컬 host/join/주소/Start/Cancel의11.01 확장 인자다. CD10.72는 기존 보류 경계를 유지한다.

로컬 역할 선택·명단/주소·실제 실패/대기·Start/Cancel을 같은 실제 HTML/버튼/일반 입력으로 연결했다. 기본 게임 포트는6799, 별도 대기실은6800이며 현재 `gameServerPort`/`lanLobbyPort` 값을 읽는다. 미설정 서버 주소는127.0.0.1이고 명시 IPv4 입력은 기존 문자 입력/클립보드를 사용한다. 원격 이름을 HTML 본문으로 escape하며 같은 명단일 때 창을 반복 생성하지 않는다. 자기 창만 serial로 반납하고 주소 입력 취소도 원래 선택으로 돌아간다.

대기실 폴링 callback 밖에서 실제 ACK 완료 요청을 받고 다음 호스트 Tick에 World 교체를 예약한다. 실제 next7/주소/포트와 소유 가변 Zone 핸들1을 사용하며 포인터를DWORD로 자르지 않는다. client는 Zone 없이 접속하고 host만 실제 명단을 설치한다. 아직 실행 전인 자기 시작 요청은 Cancel에서 취소하고 실제 current7 실행 중인 요청은 cleanup에도 유지한다. 실제 Native Player 재할당 전에 대기실을 닫는다.

Client 수명의 기존 공통 State7/즉시8 부모 안에서 정리 전 지역값→cleanup/CD/명령→전투 모드/음악→일반 GameWorld/같은 서비스·작업 파일/원격 Fort→같은 Registry의 LAN host/client 전송→998 후반을 연결했다. 현재 Fort/사용자 이름은 미리 파일을 읽거나 Player를 합성하지 않고 실제 이전 값으로 인계한다. 특히 Session에 지역 소유돼 있던 이름64BYTE가 반납 뒤 빈 값으로 바뀌지 않게 GeneralBattleStartRequest가 보존한다. Guide 부재의 demo getter는 원문 자연0을 반환한다. 실제 phase14/PlayerKnown/참여 bit1 완료 뒤에만 같은 커서/입력/표시를 공개한다.

**일반 전투 전체는 아직 미완료다.** 다음 직접 잔여는 일반 로그인 타이머004b67c0의 초기화/팀·초기 간헐천/진행 안내·저장이다. 현재 해당 위치의 명시 미연결을 빈 성공으로 바꾸지 않았다. 전체 파생 writer/decoder·메타·저장/재접속·Outpost/checking8도 계속 진행한다. 이 UI/부모 연결만으로 LAN 플레이/안정성/전체 동일성 완료를 보고하지 않는다.

첫 Release 빌드는 ConfigInterface ReadString 호출 형식2곳의 원인과 파생 진단을 합쳐 오류8건/종료1이었다. 출력 인자 API로 교정한 빌드와 두 정적 검토 보완 빌드 모두 종료0·경고/오류0이다. 중간 보조 Python 편집의 인용 오류는 소스 쓰기 전에 실패했고 apply_patch로 적용한 뒤 다시 컴파일했다. 제품11.01은11,762,688바이트, SHA-256은 `ac82a2f3991de7cf2df767cca0853fdad69c498091caf8e92221c04978f9b5c0`다. 보고서는 `cpppj/lan-world-integration-20261107.json`이다.

신규 MP/단위/게임/통신 검사·fixture 생성은 모두0이며 보호2800파일은 모두 불변이다. 사용자 지시대로 실제 LAN/MP UI도 실행하지 않았다.
