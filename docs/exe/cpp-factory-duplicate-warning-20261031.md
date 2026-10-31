# 워크샾 중복 경고의 실제 로그 연결 — 단위647

최종 Release 게임/집중 대상 빌드 **종료0·경고/오류0**, 선택한 Session 회귀 **1개/실패0**을 확인했다. 집중 대상의 등록20개 전체 실행 결과가 아니다. [보고서](../../cpppj/factory-duplicate-warning-20261031.json)에 수정 전 세 실패와 최종 성공·현재 소스/바이너리/원시 해시를 기록했다. 마지막 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 현재 호스트와 같다. 산출물과 보호 원본은 바꾸지 않았다.

원본4538f0은 같은 위치/타입/매립 상태의 다른 Factory를 발견하면004c2630 채널4에 `WARNING!  Extra workshop destroyed at (%d,%d)\n`을 기록한 뒤 가상+10 삭제를 실행한다. 경고 문자열은10.72 PE에도 그대로 있으며 해당 본체/로그 대응은47a4b0/48ca70이다. 기존 FactoryPostPop 몸체/독립 기계어 자료에는 이미 경고→삭제 순서가 복원돼 있다.

실제 Session은 상위가 제공한 `assets.factory.warnDuplicate`를 무조건 SessionMissing으로 덮어썼다. 이 덮어쓰기를 필수 누락 검사로 바꾸고, 실제 Client 서비스는 원본 채널/문구를 현재 `WriteDiagnosticLog`로 전달한다. 원래 로그의 필터/들여쓰기/Chat 전달/FILE 수명을 사용하며, 경고 뒤의 기존 삭제 몸체는 바꾸지 않는다.

새 창 없는 Session 검사 입력을 준비했다. 최초 입력은1-1에 windFactory가 있다고 가정해 실패했고, 다음 입력은 사제 위치의 spot 겹침으로 일반 Pop에서 거부됐다. 두 원시는 보존했다. 일반 Pop의 거부를 바꾸지 않고, 실제 첫 워크샾을 별도 위치에 등록한 뒤 Native +20 후처리가 받을 두 번째 객체 좌표를 명시 공급하는 장치 접합 검사로 구분했다. 세 번째 수정 전 실행은 정확히 SessionMissing1271에 도달해 실패했다. 이것은 합법 GUI 건설/배치의 성공이나 원본 게임 실행 증거가 아니다.

같은 세 번째 입력의 수정 후 실행에서는 경고 callback이 살아 있는 기존 SID/좌표를 먼저 읽고, 그 뒤 기존 워크샾만 실제 공통 삭제로 반납했다. 새 객체의 명시 입력 상태와 타입은 유지됐고 삭제 전후 깊이는0으로 복구됐다. 실제 검사 호출은 **4회/고유1개/최종 수락1개**다. 선택 명령은 `netstorm_menu_session_tests.exe --filter WorkshopDuplicate`이며 다른 등록 사례를 실행하지 않았다. 초기 두 입력 오류를 제품 실패로 분류하거나 기존 테스트의 통과 조건을 바꾸지 않았다. 단일 플레이 자산/내 워크샾만 입력으로 사용했다.

새 게임 EXE SHA-256은 `867022e3fd9fc014b78d56a7c39f9a46c2bcbd9970294696e70066ad373d54f8`이다. 주 검사 EXE/새ZIP/전체34는 다시 만들거나 실행하지 않았다. 관련6입력(AGENTS·두 원본PE·TAFF·기존 FactoryPostPop/TraceLog fixture)의 현재 해시는645의 보호 기준과 같으며, 전체2632 재감사/기존 Native fixture 재실행의 결과로 세지 않는다. 실제 Client 로그 adapter는 원문/소스/빌드로 대조했으며 새 GUI 호출은 하지 않았다.

원시 접두는 `extracted/cpp-factory-duplicate-647-`다. 중간 빌드/입력/실패를 덮어쓰지 않았다. 스테이지 진입/대기·신규MP·실제 포획/희생·적 공격·승리·공중3종 전용 검사는 하지 않았다. 남은 메뉴/상태/수명은 더 많은 기능 묶음 뒤 실제 통합한다.

함께 점검한 메뉴의c2는 `DispatchEditorTestBattle`,c5는 Guide/Leave 처리에 이미 연결돼 있다. 미연결c6(Refresh Zones)/db(Declare a Draw)는 RawGameMenus의 메타맵/일반 Battle 전용 표시 분기다. 이를 비멀티 메뉴의 새 미연결로 취급하지 않는다. 기존 Add Island와 Editor 후처리는 모두 Graph(false)를 호출하므로493/494의 배치 직후/재로드 차이를 그 호출 인자만 바꿔 숨기지 않았다. 실제 원본 전체 월드와의 대조 잔여는 유지한다.
