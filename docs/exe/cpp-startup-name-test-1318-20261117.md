# cpppj 1318 — 시작 경로 고리 검사의 실제 읽기 수 반영

2026-11-17, HJOW-Athlon. 제품1315·원문·fixture를 유지하고 `file_directory`의 오래된 **1검사·3단언 실패**를 해결했다.

`StartupFileNames_ActualClientUnicodeStartupSaveAndReload`는 옛 값형 읽기의 25칸을 기대했다. 현행 Client는 `MakeConfigFileReadHooks` → `LoadConfigFile` → `ReadWholeBaseFile(check=1)`의 실제 파일 경로를 사용하므로 존재조회와 열기 각각 장치/기본 경로 두 칸을 소비한다. 이름 일곱 개 준비와 파일 여섯 개의 이름 한 칸·존재조회 두 칸·열기 두 칸은 **7+6×5=37칸**이다. 저장 네 칸·값형 재읽기 두 칸도 기존대로 이어진다.

검사의 시작 고리 번호를 기준으로 37·41·43칸 뒤의 정확한 번호를 검사한다. 파일 owner 동일성·우선순위·설정 내용·Unicode 경로 왕복 단언은 그대로며 원문 fixture24개도 바꾸지 않았다. 이 수정은 제품의 고리 소비를 줄이거나 기대 검사를 제거한 것이 아니다.

- 관련 Release 집중 빌드 종료0.
- 수정 전 전체18건 중 **1건·3단언 실패**.
- 수정 후 `netstorm_file_directory_tests`: **18/18 통과**.
- 원시: `extracted/cpp-baselines-1318/file-directory-before.log`·`file-directory-after.log`·`file-build.log`.

같은 작업에서 `world_generation`의 허용된 비MP 검사 다섯 개를 개별 프로세스로 재확인했다. 제네레이터 표시(6단언), `CampaignSharedKernel_ActualFormsAndSingleClientFrame`(1단언), `CampaignSharedKernel_PartialConstructorPreservesExistingProcess`(1단언)가 실패했다. `CampaignSharedKernel_FinalDrainPrecedesPlayerAndServiceRelease`는 오래된 종료 기대 뒤 접근 위반(0xC0000005)으로 끝났으며, `CampaignSharedKernel_GameWorldSurvivesExplicitGlobalClear`는 통과했다. 이 별도 원인은 다음 단위에서 분석한다. 전체 world_generation·사제 포획/희생·MP·공중3종 생성/행동 검사는 실행하지 않았다.

`netstorm_guide_fort_request_tests`도 컴파일만 갱신했다. 새 전체 실행 통과로 기록하지 않는다. 마지막 전체 디컴파일 호스트는 HJOW-Athlon이며 원문 변경은 없다.
