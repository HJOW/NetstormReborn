# 8차 Scenario 실제 UI·파일·Test Battle 통합 준비본

2026-11-06 / HJOW-Athlon. AGENTS.md, LEFT_JOBS.md, LEFT_JOBS.dotnetpj.md, 기존 `cpp-editor-scenario-preparation/PLAN.md`와 미등록 모델을 다시 읽었다. 마지막 전체 정밀 디컴파일 HJOW-Athlon / 2026-11-03과 현재 호스트가 일치한다.

**현재 상태는 실제 연결 코드를 작성한 미반영 준비본이다.** 실제 제품/기존 미등록 모델5파일/자산/원본/fixture/CMake/LEFT/AGENTS/WB는 수정하지 않았다. 빌드·C++ 검사·게임/GUI/MCP/MP 실행은 모두0건이다. 사용자 최신 지시에 따라 기능 범위를 고정하고 이 인계를 마친 뒤 루트의 문서/로컬 커밋/목표 일시중지로 넘긴다.

## 보존할 파일과 다른 PC 재개

- `pending/`: 적용 후보의 완전한 UTF-8 사본이다. 기존 파일17개 변경, 신규6개(아래 표), 기존 모델 검사1개 무변경 사본이다.
- `base/`: 실제 제품/미등록 모델을 읽어 복사한 기준18파일이다. 이를 기존 제품 위에 일괄 덮어쓰지 않는다.
- `scenario-integration.patch`: 원래 경로 `a/cpppj/...` → `b/cpppj/...`의 변경23파일/789행 패치다.
- `manifest.json`: 기준/준비본/실제 제품 SHA256, 호스트, 실행0, 패치 SHA를 기록한다.
- `CMake-additions.txt`: 실제 등록해야 할 소스/검사와 필터다. 제품 CMake는 변경하지 않았다.

기존 미등록5파일은 ScenarioDocument.h/.cpp, EditorScenarioModel.h/.cpp, ScenarioDocumentTests.cpp다. 모두 base/pending에 보존했다. 다른 PC에서 실제 cpppj 위치에 이 다섯 파일이 없으면 **base의 다섯 파일만 먼저 해당 경로로 복원**한 후 패치를 적용한다. 이미 존재하면 manifest 기준 SHA를 비교하고 hunk로 병합한다. 신규 파일은 패치에 들어 있다. 루트가 이 디렉터리를 Git 추적 아티팩트로 보존하므로 런타임 등록 완료나 구현 완료 커밋으로 혼동하지 않는다.

최종 `git apply --check --whitespace=error --ignore-space-change`는 종료0이다. 최종 비교 시 변경대상17개 실제 제품/미등록 모델 모두 base와 byte 동일했다. 패치 SHA256: `d8cd5e633ef73e0190ea97ab2f7e2ce47da017479623ea2e61653db36d9deee8`. 정적 적용 가능성 확인이며 C++ 컴파일/링크/동작 검증이 아니다.

| 신규 파일 | 연결 책임 |
| --- | --- |
| `EditorScenarioFiles.h/.cpp` | 현재 Config 스냅샷으로 요청 언어/English missionSpec 계산, 기존 workspace 읽기/삭제 표식/보호 atomic 쓰기, 초기 English, 선택 파일 identity/동시 수정, Save as 복사, TestBattle의 같은 reader |
| `EditorScenarioController.h/.cpp` | 일반/로컬/공통AI/ownerAI/첫 브리핑 페이지, 고정 필드 ID, 보류 문서, 실제 실효값·원문 값 구별, 검증/저장/취소 |
| `UberScenario.inc` | 실제 Editor 메뉴 명령 → Fancy Gump 패널 → TextEntry 모달 → 보류 모델 → 일반 입력 큐 복귀, 현재 Editor/맵 수명 검사, ESC/취소, TestBattle snapshot 뒤 준비 실패 처리 |
| `EditorScenarioIntegrationTests.cpp` | 새 창 없는 일반 검사9개. 실제 Config/보존형 문서/AI 설정 계산/메뉴 작성기와 명시 파일 장치를 사용하며 실행은0건 |

## 실제 코드 연결

1. `GameMenusHooks.scenarioCommand`는 기본 없음이다. 원문 `RawGameMenus::Edit()`의 Save as/Add Island 뒤에 이 훅이 있을 때만 Scenario 행을 더한다. 실제 UberGump는 설치된 Editor에서만 훅을 준다. 원문 메뉴 fixture 기본 경로는 그대로다.
2. 새 명령 `0x0300e100..105`는 기존 PushCommand의 `0x0100ffff|0x02000000` 마스크를 통과하는 명시 클론 명령이다. 원문 명령 ID나 포인터를 재사용하지 않는다. UberGump::ScenarioEvent는 이 범위와 열려 있는 Scenario의 ESC만 처리한다.
3. 별도 Fancy 패널은 시작 SP/로컬 기술/허용 기술, 제목·원문 규칙, 공통AI, owner2~8 AI 이름/SP/Tech/Off/Color/AllyList/Enemy/NoTemple/Ability/rate/수치, 첫 `[A.]` 브리핑을 노출한다. 일반 TestBattle의 로컬 owner AI 행은 비활성이고 로컬 시작값은 Player 페이지에서 편집한다. 각 AI 행은 실제 `FindPriestParticipation(owner)` 결과를 읽어 사제 배치 필요를 안내한다. 없는 사제를 자동 생성하지 않는다.
4. 기존 SaveFortNameDialog에 **별도 ShowScenarioInput** 생성기를 더했다. 원문 Show/ShowMoney/ShowInput의 기하/상한은 바꾸지 않는다. 같은 장식/Stone/TextEntry/Slider/클립보드/IME/MCP 입력 경로를 쓰며 현재 문서 값을 전용 상태 주입으로 넣거나 확정하지 않는다. 초기 텍스트만 정상 SetText로 설치한다.
5. 다중 줄 Enter는 실제 TextEntry가 LF를 삽입한 뒤 부모의 자동 확정 통지만 소비한다. OK 버튼으로 확정하며 Cancel은 해당 필드 편집만 취소하고 이전 패널로 돌아간다. 일반 명령 큐에 페이지 복귀를 예약해 입력 callback 중 현재 모달을 즉시 해제/새로 만들지 않는다.
6. UTF-8 사용자가 입력한 값을 표시 BYTE에서 DecodeDisplayText로 되돌린 뒤 모델에 반영한다. 실패하면 경고를 표시하고 같은 입력창/이전 문서를 유지한다. 현재 맵/Editor identity가 달라진 지연 callback은 거부한다. 패널 Save만 명시 저장, Discard/ESC는 보류 변경을 버린다.
7. 보류 문서는 페이지 이동/필드 편집 중 유지되고 World 반납 전에 패널의 실제 토큰/문서를 반납한다. 일반 SaveGoMain Yes는 보류 sidecar 저장 실패 전에 원문 nextState가 바뀌지 않게 먼저 확인한다. No는 기존 저장 안 함 의미를 유지한다.

## 파일 경로·기본 English·원본 보호

실제 `originals/d/setup.cfg`를 기존 Python decoder로 읽기만 하여 다음을 확인했다.

```text
DataDir = "\D"
missionSpec = "{DataDir}\{local.1}.{currentLanguage}"
fortSpec = "{DataDir}\{local.1}.fort"
```

경로를 `D/<name>.english`로 하드코딩하거나 전역 currentLanguage를 잠시 바꾸지 않는다. 실제 ConfigRegistry의 Prefix/등록 순서/메모리 본문을 **독립 Registry**에 복제하고 local.1과 currentLanguage=english만 그 사본에 적용한다. 간접 missionSpec/사용자 경로 override와 named Config 참조도 현재 문법으로 계산한다. staged mission 실효값도 같은 방식으로 마지막 mission 사본을 등록해 계산한다. 외부 `{@미션...}` 파일을 새로 읽는 loadMission 콜백은 복제하지 않으며 해당 동적 식의 완전한 외부 파일 미리보기는 보장하지 않는다. 실제 저장 원문은 보존한다.

동일한 EditorCatalogWorkspace를 공유한다. 요청 언어 파일이 있으면 해당 파일이 UI 저장 대상이다. 없으면 같은 English 파일을 선택한다. TAFF/loose/user 우선순위와 전체 상대경로 매핑은 기존 workspace에 남긴다. 부재 optional과 존재하지만 읽지 못한 파일을 구별하며 후자는 오류다. 손상/빈 기존 파일도 새 기본값으로 덮지 않는다.

새 `EditorCatalogWorkspace::Deleted()`는 완료 장부 항목이 있는데 현재 작업 파일이 없는지만 조회한다. source/TAFF를 다시 복사하거나 장부를 바꾸지 않는다. English 기본 생성은 실제 부재에만 수행하며 이 삭제 표식이면 자동 생성하지 않는다. Scenario를 열면 기본 문서를 메모리에 제시할 수 있지만 **명시 Save**에서만 삭제된 파일을 재작성한다. 자동 TestBattle 경로는 재작성을 거부하고 같은 Editor에서 안내한다.

정상 Editor checkpoint 뒤 신규/기존 현재 맵의 English만 보장한다. 이 단계에서 오류가 있어도 이전 파일/기존 Editor 설치를 기본 대본으로 가장하지 않으며 안내한다. 기본 문서는 기존 모델의 Tutorial/5000SP/all/최소 Go 버튼이고 AI/사제/승리 조건을 만들지 않는다. 원본/설치 폴더에는 쓰지 않고 `workspace.Files().WriteAtomic(workspace.Resolve(path),bytes)`만 사용한다. 사용자 홈에서는 현재 5차 정책의 CustomMaps 전체 하위경로다.

Save는 최초 baseline과 현재 bytes가 다르면 중단해 동시 수정/삭제를 덮지 않는다. atomic 정상 반환 뒤에만 보류 기준을 재설정한다. Save as는 현재 언어와 English sidecar만 보존 복사하며 모든 대상 충돌을 첫 쓰기 전에 확인한다. 원래 source/TAFF/이전 sidecar를 삭제하거나 loadFort/수동 대본을 문자열 치환하지 않는다. 서로 다른 두 파일과 Fort 저장은 하나의 트랜잭션이 아니며 첫 파일 성공 뒤 실패는 그대로 남고 같은 bytes 재시도로 이어간다.

## TestBattle 연결

기존 DispatchEditorTestBattle 자체는 변경하지 않았다. 실제 `hooks.snapshot`에서 `SaveCurrentMap()` 정상 복귀 **뒤** 보류 문서 저장/English 보장을 수행한다. 새 준비 실패만 EditorScenarioPrepareError로 구별하고 Guide가 닫히기 전에 경고로 처리한다. 기존 Guide/원문 실패를 무조건 정상 Editor 성공으로 바꾸지 않는다.

같은 scenarioFiles owner를 `CreateMission`의 명시 FileReader가 소유한다. 해당 현재 맵의 요청 언어 script가 없을 때만 canonical English로 fallback하며 Fort/loadFort/추가파일은 그대로 기존 workspace reader다. Script/Fort 선택·로그인 재로드는 기존 명시 FileSource 수명을 이어간다. 이후 다른 이름으로 Fork한 수동 대본까지 일반 언어 fallback을 확대한 것은 아니다.

## 모델 교정과 입력 범위

- 원래 미등록 모델은 AI Name에 EncodeOriginalText를 써 한글을 거부했다. 모델 복사본과 실제 RawCampaignMissionStartBindings의 **Name 조회만** EncodeDisplayText로 바꿨다. Tech/수치/능력/원문 키는 기존 CP1252/영어 의미를 유지한다. 새 이름은 29표시BYTE 이하이며 한글 음절은4BYTE다. 실제 런타임도 확장 Name이 경계를 넘으면 명시 거부한다. 기존 CP1252 이름의 원문 복사 동작은 이 새 경계로 제한하지 않는다.
- 실효 AI 값은 기존 RawMissionAiSettings::Prepare를 독립600BYTE에 계산하며 실제 AI/프로세스/참가자/동맹표는 만들지 않는다. startingSp/stuffRefreshRate는 추정 Config 키가 아니라 현재 Session.MissionAiSettings의 실제 값을 읽는다.
- 공통AI/owner override와 AllyList/Ability의 가산 의미를 유지한다. myAllyList를 새 실제 소비 키로 만들지 않고 AI별 AllyList에 로컬 owner를 넣는 원문 경로를 사용한다. 공통 관계를 제거하려면 공통 페이지에서 직접 편집한다. 자동 대칭화하지 않는다.
- ScenarioDocument의 첫 섹션 읽기/SetSection과 모델 SetBriefing을 추가했다. 선택한 첫 A. 본문만 바꾸고 다른 이벤트/승리 대본/주석/미지 키/중복은 보존한다. 본문에 새 `[section]` 행을 넣는 입력은 거부한다.
- 필드 입력은 변경한 값만 검증하며 기존 raw 식/escape를 그대로 OK하면 no-op이다. 기존 매크로를 실효값으로 펼쳐 저장하지 않는다. 새로 바꾼 값은 SetLiteral의 명시 literal 의미다.
- 첫 브리핑 UI의 상한은 **999표시BYTE**다. 실제 TextEntry Draw의1024BYTE 한 줄을 넘지 않게 하는 유한 기능 범위다. 기존 더 긴 값/표시 인코딩 미지원 문자는 절단/손실 없이 편집 진입을 거부한다. 저장하지 않은 원문은 그대로다. 일반 field511BYTE/title239BYTE/AI Name29BYTE 상한이며 파일명은 기존 ASCII 영숫자/underscore/공백 정책이다. 파일명 Unicode 확대는 하지 않는다.

## 통합/검증 순서와 미검증 경계

1. 다른 PC에서 미등록 모델5개가 없으면 위의 base 사본을 먼저 복원한다. 기존 cpppj 파일은 새 HEAD의 root MP 변경과 hunk 단위로 병합한다.
2. 한국어 final-pending의 DisplayTextInput.h/.cpp, 토큰 원자 편집/IME/Unicode clipboard, 실제 클론 HTML의 유효4BYTE 보존을 먼저 적용한다. 기존 한국어 준비본은 이 작업에서 수정하지 않았다. English UI에서 한글 Name/제목도 보존하는 실제클론 flag 정책은 한국어 통합 담당 인계를 따른다.
3. CMake-additions.txt의 라이브러리4 cpp/관련 h/inc와 기존 모델 검사/새 통합 검사를 등록한다. 사용자 일시중지 지시에 따라 이번 에이전트가 실제 등록/빌드하지 않았다.
4. 새 `EditorScenarioIntegration_`9개와 기존 `ScenarioDocument_`7/`EditorScenarioModel_`3개 준비 검사를 빌드/실행한다. 기존 GameMenus/SaveFortNameDialog/EditorTestBattle/EditorSaveGoMain/EditorCatalog 회귀도 해당 fixture를 바꾸지 않고 확인한다. 현재 모두 미실행이다.
5. 실제 확인은 루트만 수행한다: 새 빈 맵/English 없는 사용자 사본 → Scenario의 시작SP/기술/제목/브리핑 저장 → 같은 사용자 파일 확인 → TestBattle 진입/짧은 대기 → Editor 복귀/재열기. 기존 override/TAFF/삭제 표식, 저장 실패/동시 변경, Save as의 대상 충돌, 한국어 AI 이름/브리핑과 영어 UI 상태도 별도 확인한다. 공격/포획/희생/승리/공중3종 전용/신규MP·통신 검사는 하지 않는다.

컴파일/링크/폰트/실제 Fancy 메뉴 기하·모달·Enter/ESC/취소/Slider·IME·MCP·사용자 홈 I/O·실제 TestBattle의 성공을 확인하지 않았다. 단위9개 소스 작성과 정적 patch check를 제품 동작/8차 완료로 합산하지 않는다. 사용자 중지 지시가 우선이므로 이 문서 뒤에는 새 기능/빌드/실행을 하지 않고 루트에 완성 후보를 반환한다.
