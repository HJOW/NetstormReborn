# S1 시나리오 모델 병합과 Edit 메뉴 연결 (제품 1277, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음, 원본 소스 재디컴파일 없음).

## 1. 범위

986 준비본(`docs/exe/handoffs/cpp-pause-20261106/scenario/`)의 시나리오 모델·편집 후보를 현행 편집기에 병합하고 실제 Editor에서 열고 닫는 데까지 확인했다. 시나리오 편집 전용 화면의 세부 필드 편집은 S3, 기본 english 자동 생성과 english 부재 Test Battle은 S2, 사용자 홈/읽기 전용 설치 검증은 S4에서 이어간다.

## 2. 병합한 파일

| 구분 | 파일 | 역할 |
| --- | --- | --- |
| 신규 | `ScenarioDocument.*` | `.english` 대본의 줄 단위 문서 모델. 편집하지 않은 줄·주석·순서·줄바꿈을 보존하고 수정한 키만 다시 쓴다. |
| 신규 | `EditorScenarioModel.*` | 문서를 실제 Config 스냅샷으로 해석해 플레이어/AI/브리핑 필드 값을 검증한다. |
| 신규 | `EditorScenarioFiles.*` | 현재 선택 언어/English 대상 경로 결정, 원자적 저장, english 부재 시 `EnsureEnglish`. |
| 신규 | `EditorScenarioController.*` | 페이지(전체/로컬/일반/브리핑/AI 2~8)·필드 편집·저장·취소·상속 명령. |
| 신규 | `UberScenario.inc` | `UberGump` 안의 패널 열기/닫기/필드 입력/Test Battle 준비/명령 분배. |
| 수정 | `UberGump.*`, `UberEditor.inc`, `RawMenuControls.*`, `RawMenuSaveFort.inc`, `RawGameMenus.*`, `EditorMapCatalogFiles.*`, `RawCampaignMissionStartBindings.cpp`, `SaveFortNameDialog.*`, `src/CMakeLists.txt` | 메뉴 훅·모달 입력·새 맵 저장 시 sidecar 연결과 빌드 등록. |
| 신규 시험 | `ScenarioDocumentTests.cpp`, `EditorScenarioIntegrationTests.cpp`, `tests/CMakeLists.txt` | 집중 대상 `netstorm_editor_scenario_tests`. |

## 3. 병합 중 발견·교정한 결함

1. `EditorScenarioFiles.cpp`의 `EditorFiles.h` 포함 누락 → 컴파일 오류.
2. `ScenarioDocumentTests`가 CP1252 바이트를 `EncodeConfigFile`에 넘김 → 입력은 UTF-8이므로 `DecodeOriginalText` 경유로 교정.
3. 이름 없는 Config를 빈 문자열 이름으로 만들던 Snapshot → 이름이 비면 `std::make_unique<o::Config>(registry)`.
4. `UberGump.cpp`의 거부된 patch 한 곳을 수동 병합.
5. **실제 화면 확인에서 발견:** Scenario 행을 클릭하면 `현재 Client 원본 메뉴 항목 명령 미연결: 50389248`로 프로세스가 종료됐다. `UberGump.cpp`의 메뉴 명령 관문이 새 확장 명령(`0x0300e100~0x0300e105`)을 모르는 원문 미연결 명령으로 거부했다. `IsEditorScenarioCommand`를 추가해 관문에 허용했다.
6. **같은 확인에서 발견:** 패널이 열린 채 종료/월드 반납(`ClearWorld → CloseScenarioUi(true)`)하면 이미 닫힌 Gump 토큰을 다시 Kill해 `Gump 삭제 토큰/저장소 없음`으로 `std::terminate`가 호출됐다(오류 로그가 원인을 즉시 보여 줌). `CloseScenarioUi`가 `Menus()->Node(token)`로 노드 존재를 확인한 뒤에만 Kill하도록 교정했다.

## 4. 검증

- Release 빌드 0 경고, 집중 대상 `netstorm_editor_scenario_tests` 20건 중 실패 0(5번 결함의 회귀 시험 `EditorScenarioIntegration_CommandGateAcceptsOnlyExtensionRange` 포함).
- 기존 집중 대상 `menu_session`/`main_popup`/`file` 실패 목록은 병합 전 기준선과 동일(MENU_SAME, POPUP_SAME, FILE_SAME).
- 실제 실행(`--mcp --window --no-audio`, 격리 `NETSTORM_USER_DATA`, 복사본 게임 폴더): 메인 → Edit → TEST01 → Edit 메뉴에 `Save as...`, `Add Island`, `Scenario...`, `Set All Bridge` 표시 → `Scenario...` 클릭 → 패널(Player start, General, Briefing, File, AI common, Player 1, AI 2~8, Save, Discard) 표시 → ESC로 닫힘 → 종료 코드 0, 로그 파일 미생성, 게임 폴더 파일 불변. 스크린샷은 `extracted/cpp-resume-1277/sc2/`(git 제외).
- 패널 제목/행은 영어와 한국어를 함께 표기한다(`Player start / 시작 SP·기술` 등). 한국어 글꼴은 exe에 내장하지 않고 기존 외부 `fonts/` 경로를 사용한다.

## 5. 남은 일

- S2: 새 맵 기본 english 자동 생성과 english 부재 Test Battle 진입(후보의 `PrepareScenarioTest`/`EnsureEnglish` 연결은 있으나 실제 확인 전).
- S3: 필드별 편집(텍스트 입력 모달 → 적용 → Save)의 실제 화면 왕복과 기존 배치 도구와의 병행 확인.
- S4: 사용자 홈/읽기 전용 설치에서의 저장·불러오기.
