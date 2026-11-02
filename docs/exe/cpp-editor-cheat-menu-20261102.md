# cpppj 단위725 — 실제 객체 메뉴의 CHEAT 편집 전용 노출

2026-11-02 / HJOW-Athlon / Windows10. 마지막 전체 정밀 디컴파일은 HJOW-Athlon·2026-10-23 유지다. 제품725 / 표시11.01이다.

## 확인한 문제

사용자2026-10-28 결정은 CHEAT 문구가 있는 항목을 게임 진행 중에는 표시하지 않고 Edit 안에서만 원문 추가 조건에 따라 표시하는 것이다. 원본의 독립 공통 객체 작성기004b1270은 editor0/special1/missionPost1에서 CHEAT Hurt Me!/Destroy/Make Yours와 Set Owner를 작성할 수 있고, 파생00462390도 special만으로 CHEAT Dig Up을 요청한다. 기존제품의 실제 Session 연결이 이 입력을 그대로 UI에 노출했다. 기본special0의 정상 플레이에서 이를 관측했다고 주장하지 않는다.

실제 보호자산/RawCampaignSession/controlled 메뉴로 editor·special 네 조합×공통/오벨리스크 두 메뉴의8개 실제 항목 트리를 만들었다. 수정 전에는 전투 모드의 CHEAT 항목 부재 단언7개가 실패했다(`tests-before.log`). 원문작성기 자체의 외부효과 fixture를 바꿔 기대를 약화하지 않았다.

## 수정

제품`RawCampaignSession::MakeObjectMenus`의 공통 객체 special getter는 현재editor와special을 모두 요구한다. 공통 메뉴의special은 CHEAT 분기에만 사용하므로 실제 전투의 CHEAT 노출을 막고 Edit의 원문Destroy/SetOwner 분기를 유지한다.

파생 항목 장치는 실제CHEAT Dig Up 명령(005427ac의ID)이고 현재editor0이면include만0으로 전달한다. Edit에서는 원문의special/include/enabled 값을 그대로 전달한다. 다른 파생 메뉴·명령의special 의미를 바꾸지 않는다. 원문 `RawObjectMenu`/`RawObjectMenuVariants` 소스와독립관측은그대로이며 사용자지정노출규칙은제품Session에만적용한다. 한국어주석에적용이유를명시했다.

## 검증

최종 집중 **7개·실패0**이다. 신규1개는8개실제메뉴에서 전투의CHEAT5종 부재, Edit의Destroy/SetOwner 유지, DigUp의Edit+파생+special 조건과TurnToSpell/About 보존을 확인한다. 오벨리스크는 실제112생성자/VT50a748을쓰되Pop·보상·전환명령은실행하지않는다.

기존공통/회수/파생작성기5개검사(양판본1434행+10.78파생1024행, 총2458개원본관측행)도통과했다. 이는기존원본관측재대조이며새원본실행이아니다. 기존 실제Detonate메뉴·클릭·큐·부모호출뒤월드무변경1개도통과했다. 특정적공격/시나리오승리를재현한검사가아니다.

수정전/수정후제품/집중참조빌드 모두Release 경고/오류0이다. EXE SHA-256은 `82daddd899323ddb01dc43d8304225a35ae0374465e6a77d011da9958bde6032`다. AGENTS·원본/CD·기존fixture2641개 전체변경0이고새원본fixture/실제게임실행은0이다. 최초실패로그는보존한다.

전체C++/34/ZIP·물리입력/PCM/Windows11·새MP/통신·사제포획/희생/특정적공격/승리/공중3종전용검사는0이다. 전체동일성/자연완주는사용자확인대상이며나머지객체상태·UI/Edit도구/파일/시나리오/호환성은계속진행한다. [요약보고서](../../cpppj/editor-cheat-menu-20261102.json), 원시 `extracted/cpp-editor-cheat-menu-725/`를따른다.
