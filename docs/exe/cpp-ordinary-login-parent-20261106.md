# cpppj 일반 로그인 부모 복원 — 2026-11-06, 단위985

원문10.78 `004a7800`의 일반 전투 분기를 기존 `RawMissionLogin` 부모에 복원하고 실제 Session의 저장/전투 상태와 SP 지정에 연결했다. **제품985 / 11.01**이며 전체 일반 전투/LAN 기능은 아직 미완료다. 호스트와 마지막 정밀 디컴파일 호스트는 **HJOW-Athlon**, 정밀 자료 날짜는 **2026-11-03**으로 일치한다. 새 디컴파일은 하지 않았다.

`extracted/refined/originals/Netstorm.c`의004a7800 전체와0041c7d0을 현재 클래스/부모 순서와 대조했다. 같은 subscriber의 첫 owner, owner0 거부, CheckLogin/Begin/옵션/Player·metadata/Fort/SP·시계/Ready의 기존 순서를 유지한다.

- simulconnect의 음수 pending은 기존 owner 연결을 먼저 끊는다. 양수 번호가1..8 밖이면 실제 진단 후 계속하며, 그 효과 뒤 이전 human bit2를 읽는다.
- CoreData0048fb30은 previous0에서만 복원한다. 실제 Client는 미션Post1에서도 같은 하위를 호출하며, 그 하위의 첫 자연 반환을 보존한다. 독립 미션Post1 어댑터의 미사용 장치 누락은 동일한 자연 반환 계약으로 남긴다.
- `!saved && inBattle && !missionPost`에서 watcher를 먼저 쓰고 owner+64를 검사한다. 처음이면1..8 전체에 현재 watcher 옵션과 시작 SP 옵션을 매번 다시 읽어, watcher는0, 나머지는 원문1000/2500/3000+4000을 지정한다. 기존00447810 `SetPlayerSpAmount` 전체를 재사용해 같은 분산 SP·현재 로컬/HUD·Editor 판정을 수행한다. 실제 저장/전투 bool 주소를 빌리며 모드를 강제로 켜지 않는다.
- 새 Fort의 미션 World는 기존 Load를 사용한다. 일반 전투는 missionPost1 또는 watcher0인 때만 실제 전투섬3단계0046d9b0→0046da60→0046e2c0을 요구하고, 일반 watcher의 섬 생략은 원문 조건 그대로다.

일반 부모를 감추던 guard를 위 본문으로 바꿨으며, 미복원 큰 효과를 빈 성공 callback으로 채우지 않았다. **원격 `loadFortImage`, 004a7350 전체 월드 동기화, 전투섬 전체 생성, metadata 중앙섬**은 아직 실제 Session에서 미공급인 필수 하위다. 선택 시 해당 위치에서 명시 실패한다. 방 roster/Zone·일반 State6/7/8/9/13·Commander·LAN UI·재접속/메타맵/Outpost/checking8도 남는다. 기존 debugBurst·지연/중첩집합·SPX/IPX 경계는 이번 범위에서 바꾸지 않았다. 시작 SP 옵션의0..2 밖 색인은 원본 미정 스택 읽기 대신 호스트 오류로 처리한다.

변경4파일의 Release 첫 명령은 **컴파일·링크 종료0·경고/오류0**이었다. 사용자 지시에 따라 새 MP/단위/소켓/통신/게임 검사는 작성·실행하지 않았다. 마지막 실제 단독 GUI는984의 tutorial1 진입/대기/창 종료이며985 검사로 합산하지 않는다. 원본·CD·AGENTS 등 보호2800개와 cpppj fixture980개는 실제 SHA-256 불변이고 새 fixture는 없다. 공식 서버/DNS·폐기 유효성 검사 연결도 추가하지 않았다.

EXE 해시/크기와 컴파일 결과는 [단위 보고서](../../cpppj/ordinary-login-parent-20261106.json)에 있다. 원문 대조표/초안/빌드 로그는 Git 제외 경로 `extracted/cpp-resume-985/`에 보존했다. 다음986은 원격 Fort 이미지의 실제 이름/Template manager/읽기 수명 연결이다. 전체 목표는 진행 중이다.
