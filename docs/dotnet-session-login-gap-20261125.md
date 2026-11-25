# R4-7c3b 잔여 13검사 이식 범위 분석 (세션 로그인 본체)

2026-11-25, HJOW-Athlon. R4-7c3c 대상 6파일 13검사가 요구하는 제품 표면을 실측했다.
결론: 전부 레거시 `RawCampaignSession`의 로그인 본체(`LoadLocalConnection`·`Load`·`InstallExistingEditorCheckpoint` 계열)를
요구하며, DN은 같은 본체를 `HostSession` 쪽에 이식해 두었다(PORT_MAP: `RawCampaignSessionLogin.inc` → `HostSession.cs`).
레거시 쪽에 다시 이식하면 같은 본체를 중복 소유하게 된다. 아래는 파일별 필요 목록이다.

## 파일별 필요 제품 표면

- `CampaignMissionFortPathTests.inc` (2건 중 1건 완료·1건 잔여). 잔여 1건(`ActualLoginKeepsLogicalSelectionAndSamePhysicalFile`)은
  `RawCampaignSessionServices`(파일 선택·읽기·장치·장부 묶음), 세션 생성자 바인딩, `LoadLocalConnection`(14단계·`PlayerKnown` 1·
  읽기 2회·Template 설치·지형 일치), 리더 소유권(작업본 해제 뒤 세션 reader 생존)이 필요하다.
- `CampaignLoginObjectStreamTests.inc` (4건). 위 전부 + `missionFortLoad.readOnly`·`install.refreshWindowTitle` 장치,
  `FortObjectSettings`, `HasCursorRuntimeForState`, 세션 월드 조회·Chaff 위치 대조 helper가 필요하다.
- `CampaignMissionReloadSourceTests.inc` (3건). 위 전부 + `CheckReloadTerrain483`(Template 섹션 대조 helper, 세션 없이도
  옮길 수 있는 순수 함수) + 리더·작업본 수명 공유가 필요하다.
- `CampaignObjectStreamSessionTests.inc` (2건). `session.Load()`(ConnectBuilding/Motion/Devastation/CarrierDrop/
  PriestPrayer + runtime Load + Builder + Placement/Cursor + loaded)와 월드 Chaff 대조 helper가 필요하다.
- `CampaignNewFortTests.inc` (3건) + `CampaignNewFortConfigTests.inc` (1건). 전체 Client 소유 세션,
  `InstallExistingEditorCheckpoint`·`CommitExistingEditorInstall`·`SnapshotFort`·`SetSp`·`Sp`·`CurrentSubscriber`·
  `CurrentHandle`·`EditorInstallTail`·`RawLocalMissionState` 결선이 필요하다.
- `TemplateDiagnosticClientTests.cpp` (1건). 완료(1.002). `SoloMapCatalogTests` Tell 메타 1종은 대화 본체(R3k-11)로 이관했다.

## DN 현행 보유분 (2026-11-25 실측)

- 있음: `MissionScript.FortFileSelector/FortFileReader/FortPath/FileSource/FortName`,
  `GuideMissionScriptLifetime.Create/Release`, `CampaignTemplateStorage`(설치·이름·섹션·쓰기·접근),
  `FortTypeConversionStorage`, `EditorFiles`, `EditorCatalogWorkspace`, `FortTemplate` 해석·비교,
  세션 월드(`World()`·`Objects()`·`Find()`·`IslandCount()`), `RawLocalMissionState`.
- 없음(레거시 세션): `RawCampaignSessionServices`, `UseWorldTestLogin` 상당, `LoadLocalConnection`,
  `Load` 본체, `FortObjectSettings`, `HasCursorRuntimeForState`, `LocalConnectionReady`,
  `InstallExistingEditorCheckpoint` 계열, `CurrentSubscriber/CurrentHandle/TechnologyParent` 세션 getter.

## 선택지

- A. 레거시 세션에 로그인 본체를 통째로 이식한다. 13검사를 그대로 옮길 수 있으나 HostSession 이식분과 중복 소유가
  생기고 분량상 수 단위가 필요하다(월드 적재·패킷·수명 포함 시 R6 규모).
- B. HostSession 실제 경로 기준으로 동등 검사를 새로 쓴다. L2 34경로 덤프가 이미 진입·설치·로그인·월드 적재를
  대조하므로 세션 통합치는 L2에 맡기고, 남는 것은 컴포넌트 단위 대조로 메꾼다. 완료 기준(R4-7c3c 체크박스)의
  문구를 바꿔야 하므로 사용자 결정이 필요하다.
- C. R6(세션 수명·입력·AI·음향) 또는 R11(수송 backend)까지 미룬다. 로그인 본체는 세션 수명·수송과 함께 잇는 것이
  순서상 맞다. 이 경우 R4-7c3c 체크박스를 해당 단계로 옮긴다.

## 결정: C안 확정 (2026-11-25 사용자 결정)

미룬 세션 로그인 본체를 R6 SL-9·R11 수송으로 연기한다. R4-7은 마감한다. R4-7c3c 체크박스는 R6 SL-9로 옮겼다.

## 다음 턴 착수점 (C안 이후)

- A라면: `RawCampaignSessionServices` 구조체 + 세션 생성자 바인딩 + 파일 선택·읽기 경계부터 잇고,
  `ReloadSourceTests.SelectedMissingEmptyAndThrowKeepGlobalManagerAbsent`(읽기 실패·무상태 유지)부터 통과시킨다.
- B라면: L2 대조 항목과 컴포넌트 검사를 파일별로 매핑한 뒤 체크박스를 다시 쓴다.
- C라면: 체크박스를 R6 SL 잔여 또는 R11 수송 항목으로 옮기고 R4-8 M1 판정으로 넘어간다.
