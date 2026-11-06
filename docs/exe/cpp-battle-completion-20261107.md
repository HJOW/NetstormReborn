# 1006 — 재접속 선행 종료 관문과 Player 변경 소비자

2026-11-07,HJOW-Athlon.마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다.10.78의00448f50/00448fd0/004490d0/00449200을 대조했다.CD 대응은 순서대로없음/00483e70(strong0.627)/00483fe0(strong0.71)/00484120(strong0.963)이며 구현 판본은10.78이다.

00449200은 메뉴 창 존재가 아니라 전투 종료 여부의 실제 관문이다.기존 역사적 서비스 필드명playerUiBusy를 유지하되10.78 실제 Client의 HasMenus 연결을 현재 참가자/양방향 동맹/사제 판정으로 바꿨다.이는 재접속004b8cb0의 선행 조건과 같은 함수이며 기존 Player 패킷 소비자도 같은 현재 Session을 조회한다.CD의 기존 어댑터는 별도 유지하고 패치9×180BYTE 판정을 CD172BYTE에 적용하지 않는다.

RawBattleCompletion은 같은 Player1620BYTE/옵션87BYTE/동맹81DWORD·battle/post/boss/scriptBreak/loginEnd 참조를 빌린다.OnlyAllied는 subscriber를 세고 비관전자2명 이상이 모두 양방향 동맹인지 원문 순서로 방문한다.TeamFinished는 현재 전투/비post/scriptBreak0/실제 연결9 존재 뒤 그 동맹 판정을 먼저 하고, 아니면 각 subscriber의 사제 판정→watcher를 읽어 대표/적을 찾는다.전체 subscriber2명 이상에서 살아 있는 비관전자 팀이 없거나 하나이면 참이다.

FlagFinished는 같은 모드/연결 관문 뒤 현재LoginEnd0에서만 subscriber/flags1 기록을 세고 실제 PriestDead 뒤 같은 flags80을 다시 읽는다.원문의 all/flagged·boss/mode·현재 로그인수1/2 이상 선택을 유지한다.Finished는 TeamFinished가 참이면 뒤 FlagFinished(1)와 연결 재조회를 생략한다.별도 Ready/승리/포획/희생 사건이나 상태 쓰기를 만들지 않는다.첫 두 방향 동맹 조회의 원문 행 순서도 보완했다.

**StateReconnect/DelStateFile 입력·004b8cb0의 실제 서버 선택/상태 요청과 복구State7 인계는 아직 남는다.**이번 단위는 그 선행 관문을 분리·접합한 것이며 실제 재접속 완료가 아니다.일반 프레임486bc0/스펠·전체 decoder·메타·Outpost/checking7·한국어 후속·Edit 시나리오 UI도 이어간다.

첫/보완 Release 제품 빌드 각각 종료0·경고/오류0이다.제품11.01은11,848,192바이트,SHA-256 `bdf2269ee29f63ebd70fe3492963fdc86cecf90112f443fd6ca2a5ff614c0a93`다.보고서는 `cpppj/battle-completion-20261107.json`이다.신규 MP/단위/게임/통신 검사·fixture 생성0이며 보호2800파일은 모두 불변이다.사제/승리 조건을 실행 재현한 결과가 아니다.
