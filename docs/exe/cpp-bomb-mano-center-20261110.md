# 작업1231 — 맨 오 워 소환의 발자국 중심 연결 교정

**제품1231 구현과 허용 검증을 마감했다.** 제품1230의 후속 검토에서 중심 callback이 대상 검색용 `RawBombTargets::Center`에 잘못 연결된 것을 발견했다. 세 자격의 원본 ASM은 모두 `004adea0`을 호출한다. 기존 `core.neighbors.Center`로 교정하여 현재 좌표에서 현재 발자국 폭/높이의 절반을 뺀 float 중심을 사용한다. 검색용 정수 스냅·0.5 감산·1~255 클램프가 이 소환에 들어가지 않는다.

세 원본 자격/실제 중심/클램프의 전체 C·PE5함수를 정적으로 보존했다. 소환 본문/입력/기대값/fixture는 바꾸지 않았다. 전용 소환·공중3종의 PE/C++/게임 실행은 사용자 제외로0이다. 새 fixture도0이며 일반 비MP 회귀 통과를 실제 소환 동작 확인으로 세지 않는다.

최종 Release/x64 제품2·집중1대상 빌드는135.140초/종료0·경고/오류0, 고정 소스1개 불변이다. 비MP104/104와 기존 직접 피해1420입력 재대조가 통과했다. 최종 EXE의 Tutorial4 생산·건설 요청→TYPE75/owner1 새 서버 객체 생성→600프레임 대기/정상 종료0·PNG 확인을 마쳤다. 원본/CD/기존fixture·AGENTS2,803파일과 실행 전후 EXE는 불변이다. 시작한 빌드·집중·GUI 작업은 모두 종료했다.

HJOW-Athlon의 Client 양판본2026-11-03/R전용2026-11-07 산출물을 사용했고 새 디컴파일/원문 변경은0이다. 공통 Bomb15/16VT·Regular58/87VT, 잔여29VT/20함수는 유지한다. 다음은 트리즌1VT의 실제 소유권 전파다. 큰 AI/배치/Fort Finish·전수 감사·지연큐/JoinTestBattle 및 M7/M8·K/S/B는 미완료다. 지정 전투/포획/희생/승리·MP·사용자 직접 확인은 실행하지 않았다.

[직전 구현](cpp-bomb-mano-owner-20261110.md)·[확정 보고서](../../cpppj/bomb-mano-center-20261110.json)·[현행 인계](../../LEFT_JOBS.md). 원시 `extracted/cpp-resume-1231/`.

- Netstorm.exe: 14084608 bytes, SHA256 `5ada77e1f056f8118bbd7117293657bdafdd1d8b2a59c41500e3a8bd209458f2`.

- R.exe: 677376 bytes, SHA256 `f241e823a553ab3d8999ed0fa069b4db098dab4cc65f4db8282bab588451857b`.
