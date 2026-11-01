# 메인 Help·Options의 실제 팝업 연결 — 단위697

2026-11-02, HJOW-Athlon. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽었다. 마지막 전체 정밀 디컴파일은 같은 PC의 2026-10-23 자료이며 재생성하지 않았다. 제품697/11.01이다.

메인 메뉴의 Help·Options가 단순한 호스트 목록을 열던 경로를 원본 `004cebf0`의 실제 공통 메뉴·모달·명령 큐에 연결했다. Help는 General Help, Technical Help, Version을 작성하고, Options는 ESC와 같은 `RawGameMenus::Options`를 사용한다. 기존 CD 호스트 메뉴에는 10.78 부모를 적용하지 않았다.

## 원문과 구현

- Help `004ceca5..004ced97`: 실제 세 항목을 작성한 뒤 현재 버튼의 `baseWidth/2+x`, `y`에 부착하고 깊이를 -90으로 맞춘다. 모달을 등록한 뒤 메뉴 flags에 `04004000`을 더한다.
- Options `004ceda8..004cee34`: 기존 `004ca850`으로 작성한 메뉴 flags에 `04000000`을 먼저 더하고, 현재 버튼의 `baseWidth/2+x`, `y-40`에 부착한다. 깊이 -90과 모달을 적용한다. `005069bc`의 float 원문은 **40**이다.
- `RawButtonControls::ControlGump`는 현재 페이지 번호에 대응하는 실제 버튼만 조회한다. `RawMenuControls::OpenMainMenuPopup`은 작성 뒤 버튼의 현재 기하를 읽고 공통 Attach/AddDepth/SetModal을 사용한다. Help의 controlled 메뉴는 현대 어댑터의 소유권 인계 시 꺼내며, 독립 팝업은 기존 공통 Kill/Cleanup으로 반납한다.
- `UberGump::CurrentGameMenuHooks`는 메인·ESC에 같은 Client 화면·설정·음향·모드를 제공한다. ESC의 Session 결합 위치는 유지했다. 메인 팝업도 일반 입력 전에 공통 Gump Route가 소비한다. 별도 중앙 목록이나 그 목록의 임시 명령 번호를 만들지 않는다.
- 기존 Windows 10/11 DIB 호환 전체화면과 추가 화면비는 유지하며 원본과 다른 이유를 한국어 주석에 남겼다. 이번에는 새 화면비나 전체화면 구현을 추가하지 않았다.

실제 State3는 원문처럼 fort 모드를 세우므로 메인 Options에는 Pass Server Diagnostic 행이 나오고 Counterclockwise Piece Rotation은 나오지 않는다. 진단의 실제 외부 접속을 새로 연결하거나 실행하지 않았다. 일반 전투/편집의 다른 모드는 기존 원문 조건을 유지한다.

## 확인

Release 제품과 `netstorm_main_popup_tests`를 빌드했고 최종 경고·오류는 0이다. 새 집중 검사 2개와 기존 메뉴/명령 검사 3개가 통과했다.

- 새 2개는 실제 자산·등록표·Gump 입력으로 Help의 세 항목/비활성, 모달·깊이·기하, 선택 명령, Options의 실제 하위 메뉴와 `SETSIZE(1024,768)` 문자열 고리, 바깥 클릭 정리, 원래 돌 버튼 수명 보존을 확인했다.
- 기존 `GameMenus_Native918`, `GameMenuActions_Native610CurrentBodyAndPartialEffects`, `GameMenuActions_UnhandledReleaseAndRequiredEffects`를 재생했다. 원문 fixture의 새 생성/원문 본체 실행은 없다.
- 첫 집중 실행의 두 실패는 메뉴 좌표 조회가 완전한 표시 라벨을 요구하는데 접두어를 넘긴 검사 입력 오류였다. 실제 명령·인자를 조회하거나 실제 보고 라벨을 검사하도록 고쳤다. 제품 기대값을 약화하지 않았고 최초 로그를 보존했다.

실제 메인 메뉴는 **총2회 실행, 모두 종료0**이다. 최종 `live-v2`가 통과했다. Help 세 행과 General Help/Version, Options와 해상도 하위의 1280×720 행, 설정 반전, 반복 열기/바깥 클릭 닫기를 확인했다. 팝업을 닫은 7개 관측점의 등록 객체 수가 모두 184이고 모달은 0이다. 실제 출력 BMP도 확인했다. 입력은 앱 내 검사 입력이며 물리 입력 근거는 아니다.

`live-v1`은 검사 스크립트가 기존 Technical Help 설정을 비어 있다고 가정해 단언에서 멈췄다. 같은 원시 보고에 나머지 동작과 정상 종료가 남아 있다. 다음 실행에서는 두 설정을 명시적으로 비웠다. 메인 fort 모드에 따른 진단/회전 행의 예상도 원문 조건에 맞춰 정정했다. 최초 원시는 지우지 않았다.

EXE SHA-256: `7395e66021c7c595d3a1c8f404d44dbee929b42cfab40ceca964eff4d02f4b54`. 보호 대상 2617개(AGENTS·원본1394·CD425·fixture797)는 변경0이고 두 실행의 설정은 복원했다. [보고서](../../cpppj/main-menu-popup-20261102.json), [실행 도구](../../tools/cpp_main_menu_popup_smoke.py), 원시 `extracted/cpp-main-menu-popup-697/`를 따른다.

## 잔여

메인 부모의 전체 버튼 생성/배치·Intro 조건·autoDemo 시간 수명, 전체 픽셀/음향 동일성, DD 조정·실제 장치/물리 입력, Edit/시나리오/연속 전환과 다른 타입의 수명은 별도다. 기존 전체34·ZIP은645의644제품이고 전체 C++는 실행하지 않았다. 새 MP/통신·스테이지 진입·포획/희생·특정 적 공격·승리 재현은0이다. 사용자 자연 완주를 기다리는 조건으로 되돌리지 않으며 잔여 1·2차 뒤 MP 무시험 사전 구현과 4~7차를 이어 간다.
