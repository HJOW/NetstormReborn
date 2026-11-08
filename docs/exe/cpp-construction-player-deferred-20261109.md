# cpppj 1191 — Player 미할당 단계의 건설 구성

제품은 **1191 / 11.01**이다. Native 건설은 현재 Player 조회기를 바로 보관하고 실제 원문 선택 위치에서만 현재9기록을 요구한다. 기존처럼 먼저 initial span을 검사하거나 BindPlayerRecords를 실행한 다음 getter를 덧붙이지 않는다. RawPlayerGeyserBounds/RawLocalGeyser에는 첫 인자가 current getter인 공개 생성자를 추가했다. 같은 private 생성자가 실제 판본/TYPE/프레임/장치 계약을 검사하며 typed span 생성자의 생성 전9기록 검사도 유지한다. Native 로컬 간헐천은 실제 선택 this를 전달할 부모가 없으면 구성부터 거부한다.

구성은 getter를 실행하거나 World/Acquire/Player/난수/가짜9기록을 만들지 않는다. Bounds는 진입에서 고른 this를 진단 뒤에도 유지한다. Generate/제외 순회는 각 Player 차례의 current base를 고르고 그 차례 this를 실제 BoundsRecord까지 전달한다. 비용 안내의 SP 효과 뒤 같은 레코드 이름 조회도1190 계약 그대로다. Populate의 권한/fort guard·최근목록 쓰기와 Generate의 설정/TimeSeed 효과를 Player 조회 뒤로 이동하지 않았다. 현재 기록이 없거나 물리 폭이 틀리면 실제 선택 위치에서 명시 오류가 발생한다.

Release/x64 NetstormCpp·NetstormRoot·집중 대상은 종료0, 경고/오류0, 고정9소스 불변이다. 비MP32검사 실패0이며 신규5개는 지연 생성자의 독립 PE439/221행 재생과 미할당/잘못된폭/선행 효과/실제 ClientPlayerRecords 최초·재할당 경계다. 원본 기계어 기대값을 제품 계산으로 만들지 않았다. 실제 owner 검사는 typed World에 current getter를 명시 연결한 경계 검사이며 전체 Native/NoWorld owner 구현 완료로 세지 않는다.

최초 빌드는 성공했으나 신규 하네스의 동맹 인자를 번호로 해석해32중1검사가 실패했다. 실제 RawPlayerParticipation.SetAlliance(owner,mask)로 수정하는 과정의 hidden BYTE 폭 컴파일 오류도 바로잡았다. 제품 몸체는 바꾸지 않았으며 실패 로그와 최종 재빌드/32검사 성공을 JSON의 verificationHistory에 구분해 보존했다.

최종 클론 GUI에서 Tutorial4 생산/건설 요청→실제 TYPE75/owner1 생성·600frame 대기, 같은 Client의800×600 Edit TEST01 진입→메인 복귀2회와 정상 종료를 확인했다. 원본 실행/새MP 단위·패킷·통신/Test Battle/포획·희생·특정 공격·승리/사용자 직접 확인 지정 검사는 실행하지 않았다. 보호2800파일과 설치 자료는 불변이다. stderr의 기존 부재 파일 메시지는 JSON에 보존했다.

전체 RawCampaignConstructionBindings owner/state, 실제 create/pop/destroy·표시·경제 장치는 아직 Session/World 종단이다. 장기 건설 전체/큰 NoWorld/AI/Carrier·다른 Form·일반 삭제 및 Source/listener/표/Poll·63잔여F·지연 큐·JoinTestBattle와 M6-l/m/n은 미완료다. 태그25/활성88/무태그63·큐 비활성을 유지한다. M7~M8 뒤 한국어·시나리오UI·9차를 계속한다.

마지막 정밀 디컴파일은 Client2026-11-03/R2026-11-07/HJOW-Athlon으로 현재와 같으며 디컴파일 소스 변경은 없다. 실제 C/PE6함수의 전체 분리 범위/바이트 해시/C와 실행 로그는 extracted/cpp-resume-1191/에 있다. [검증 보고서](../../cpppj/construction-player-deferred-20261109.json), [1190 참조/선택 this](cpp-construction-player-view-20261109.md).

Netstorm.exe 13615616바이트/SHA-256 `4174edcee5affd47f8287d19a1c1e61ed553f0ea27e139663ba663ea881c2c59`. R.exe 677376바이트/SHA-256 `c934c1c1842ea1e938544d744dda6cd860a9b3b0bfeaf738d90f876b365e1391`. 최신 ZIP1092는 이번 바이너리 ZIP이 아니며 M8에서 재생성한다.
