# Game·View·Options의 현재 상태와 공통 팝업 수명 (단위920)

실제 Session의 BindGameMenus와 RawGameMenus→BindGameMenusControls→공통 Gump에서32메뉴·32클릭을 확인했다. 새2/기존21 고유23사례가 최종 통과했다. 제품 변경은 없어916/11.01을 유지한다.

같은 HJOW-Athlon의 마지막 전체 정밀 디컴파일은2026-11-03이다. 원문 Game `004cbb80`·View `004ca4d0`·Options `004ca850`을 읽었고 기존 원문918입력 fixture를 현재 라이브러리의 집중 검사로 재대조했다. 새 Native 실행/fixture는 없다.

- Game8메뉴: 실제 공유 editor와 명시 demo/편집복귀 getter의 여덟 조합에서 Test Battle/편집복귀·목표/재시작·Quit 행을 확인했다. Quit8클릭은 정확한 큐 명령까지만 확인했다.
- View8메뉴: 같은 미션의 Chat 행 부재, editor에 따른 Knowledge, 현재 로컬 사제 인자와 useBanners의 현재 체크/반전 문자열을 확인했다. 실제 사제8클릭은 현재 owner를 큐에 넣었다. 사제 조작·게임 상태 전환은 실행하지 않았다.
- Options16메뉴: 명시 화면 flags/음향 가용성·실제 Config의 여덟 조합을 열고 재개방했다. 창 모드의 Direct Draw 항목과 전체화면의 Adjust/Window 행을 구별했다. 비활성 Sound4클릭은 큐를 비워 두고 활성4클릭과 Edge Scroll8클릭은 정확한 현재 반전 문자열을 보관했다. 설정 명령의 실제 실행/화면 전환은 이번 범위가 아니다.
- 모든 메뉴 정리 뒤 실제 루트가 사라지고 풀/Kernel 수량이 보존됐다. 객체 Form 메뉴의 종료와 공통 팝업의 종료를 구별했다.

첫 실행의 Game/View는 통과했으나 Options의 비활성4상태에서8단언이 실패했다. 비활성 클릭은 원문대로 팝업을 닫지 않는데 검사에서 객체 메뉴용 Clear를 사용한 탓이다. 실제 공통 RequestClosePopups→CleanupFrame으로 검사 정리기를 바꾼 두 번째 실행은 모두 통과했다. 제품/기존 기대값은 불변이며 최초 로그를 보존했다.

최종7명령/고유23사례·실패0이다. 새2·기존 원문1·Building4/Production4/State4/Upgrade1/Carrier7이다. Release3명령(메뉴 집중2·원문 집중1)은 종료0·경고/오류0이다. 보호2787개/fixture967개 불변·새fixture0·checking8·제품916 EXE 해시 불변이다.

장치 전역과 demo/편집복귀 getter는 명시 입력이고 모드/Player/설정은 같은 Client/Session 소유자를 쓴다. UberGump의 전체 가로막대·HUD 도킹/Chat·전체 픽셀/물리 입력 완료를 주장하지 않는다. 실제 게임/GUI·포획/희생/공격/승리·공중3종 전용·신규MP/패킷/통신·물리입력/PCM/CD·전체검사/34미션/ZIP은 실행하지 않았다.

기계 판독 결과는 [JSON](../../cpppj/game-menu-state-ui-20261105.json), 원문/최초·최종 로그·보호 해시는 Git 제외 `extracted/cpp-resume-920/`이다. 다음921도 UI1의 HUD/Chat/Console·남은 명령 효과를 이어간다.
