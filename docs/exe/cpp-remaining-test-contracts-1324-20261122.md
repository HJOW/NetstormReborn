# cpppj1324 — 잔여 집중 검사 계약 정리

2026-11-22, HJOW-Athlon. 마지막 디컴파일 호스트와 일치하며 원문·기존 fixture·제품 구현은 이 단위에서 수정하지 않았다. `guide_fort_request`·`menu_session`·`main_popup`·`file`의 옛 실패를 실제 검사 이름과 현재 연결 계약으로 다시 분류했다.

## 최초 관측

기존 실행 파일에서 비멀티 메뉴·편집·파일·지상 건설·경로와 읽기 전용 조회를 선별했다. 1차348건은 전부 통과했고 추가403건은393건 통과·10건 실패였고, 단순 미션 초기화1건에서 같은 loader 연결 실패를 추가 확인했다. 합계 **752건 중741건 통과·11건 실패**이며 전체 대상이나 전체 솔루션 결과가 아니다. `CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop`는 기존 일반 Battle/MP 이월 경계의 사용자 실행 제외를 유지했다. MP·교전·포획/희생·공중3종 생성/생산/행동 검사는 선택하지 않았다.

| 실행 파일 | 선정 수 | 최초 실패 | 최초 실행 파일 시각(한국시간) | SHA-256 접두 |
| --- | ---: | ---: | --- | --- |
| guide_fort_request | 89 | 2 | 2026-11-17 00:53:30 | `b5f243014b13c94b` |
| menu_session | 194 | 4 | 2026-11-11 03:01:27 | `d0584a72003da9ac` |
| main_popup | 79 | 0 | 2026-11-11 03:00:55 | `220824c24e8e174e` |
| file | 390 | 5 | 2026-11-11 03:02:50 | `7ea645310d1bbd2f` |

기존 exe의 해시·크기·UTC 시각 전체는 `extracted/cpp-remaining-tests-1324/executables-before.json`에 있다. 따라서 이 최초 결과를 제품1323 이후 새 빌드 통과로 표시하지 않는다. 선정 이름·소스는 같은 폴더의 `all-selected-with-source.json`, 최초 실패와 stdout/stderr 원시는 `initial-summary.json`·`additional-summary.json`·`initial-entry-only.json` 및 검사별 로그에 보존했다. 첫348건과 추가403건 모두 실행 전후 보호 **2,808파일**의 SHA-256이 같았다.

## 교정 근거

| 실패 | 원인과 변경 |
| --- | --- |
| `CampaignNativeFortContinuation_ActualCoreReadSeedAndLocalIntegrity` | 불일치 시 진단 DWORD를0으로 만들던 옛 기대가 남았다. `FortCoreDataLoad.cpp`의 불일치 callback과 `RawCampaignSession::ReadExistingFortCoreData`는 현재 표본을 복사한다. 장기 표본 초기값6은 `ae0e58b3ac`에서 원본 PE에 맞췄다. 검사에는 초기값과 다른 `0x13579bdf`를 명시하고 손상 입력에서 그 값이 복사되는지 확인한다. 정상 입력의 진단6·Core bytes·seed·깊이 기대는 보존했다. |
| `CampaignNativeFortContinuation_RejectsWrongModeOriginAndUnpreparedBorrow` | `49cac5a0c4`의 일반 복귀 State6 지원으로 실제 오류 문자열이 State1·2·6 계약으로 바뀌었지만 검사는 이전 State1·2 문자열을 사용했다. 정확한 현행 계약 문자열로 교정하며 네 잘못된 입력의 거부 자체는 유지한다. |
| `RewardTypeNumber_ActualSessionsBorrowSameCurrentOwnerWithoutLoad`·`LoadedCostTable_ActualSessionsKeepOriginalAssetCostsAcrossMutableWorldReuse`·`RegularCurrent_ActualSessionNumbersAndHostSurviveWorldReuse` | 표면·게임 전역은 재대여하면서 Template 저장소를 매 Session마다 새로 만들었다. [1316과 같은 검사 장치 수명 문제](cpp-mission-parent-tests-1316-20261117.md)다. 실제 Client처럼 반복문 밖의 한 Template owner를 함께 전달한다. 비용·번호·Host 주소·효과 기대값을 변경하지 않았다. |
| `DaisCheckingParents_DepthDiagnosticsReturnCurrentRuntimeValue` | 독립 TYPE 표에 실제 ProcessForm 생성자 주소가 없어 현재 공통 Factory에서 실패했다. 기존 `FormTypeInputs.h`의 원본 네 Form 생성자 입력을 사용한다. 이동/도착/의식/희생 효과는 여전히 명시 실패이며 깊이 getter와 진단 정상 복귀만 검사한다. |
| `ExistingEditorAfterCheckpoint_LiveRequestAndCommandReads` | State6 BattleOver가 정상 반환해도 항상 예외라는 옛 기대가 남았다. `14369776e9`의 현재 몸체는 실제 callback을 호출한 뒤 반환한다. callback1회와 기한·next0·입력 고리 보존을 검증한다. BattleOver 본체나 실제 승패를 실행한 결과로 세지 않는다. |
| `RawSurfaceWorld_RuntimeSharesStorageAndCurrentDispatch` | 현재 공통 Pop/삭제는 runtime의 실제 Squid 출력 장치를 요구하지만 검사에 없었다. 완성된 로컬 바이트를 보관하는 관찰 장치를 연결한다. `RawSurfaceWorld.cpp`는 PATCH에서만 이 공통 Pop runtime을 설치하므로 PATCH 출력 있음·CD 출력 없음으로 구별한다. 추가 관찰 단언이 처음 CD에서도 출력을 요구해1건 실패한 원시도 보존했고, 정확한 판본 분기로 교정했다. 소켓·LAN/Join·MP 연결은 없다. |
| `CampaignMotion_ActualWarBeginsPriestPathArrival`·`CampaignMotion_ActualMasterOfWhirligigsLocalPriestPath`·`CampaignMotion_ActualMasterOfWhirligigsInitialization` | 공통 `CampaignInstallSupport.h`가 실제 지형 생성 상위의 `BindGenerationInputs`를 공급하지 않아 로더에서 중단됐다. 현재 제품과 기존 `FortWorldLoadBindingTests.cpp`처럼 실제 표면 GameAux의 같은 전역을 연결한다. 일반 미션의 실제 mission/battle을 읽으며 군도 전용 하위는 `InstallForbidden`으로 남긴다. 이 helper의 현재 Squid writer 출력도 별도 바이트 관찰기로 보관한다. 이동 기대값·프레임 상한·도착·슬롯 반납 단언은 보존한다. |

## 새 빌드 검증

네 집중 대상의 Release 재빌드 종료0을 확인했고 최초11실패를 **11/11 통과**로 교정했다. guide2건·menu4건과 file5건이며, 실제 사제 이동은1-1의189프레임·1-2의148프레임으로 도착/Host Form 반납을 완료했다. 이 이동 관측은 포획·희생·교전·승리를 실행한 결과가 아니다.

최초6건은 `after-guide-menu/summary.json`, file의 마지막4건은 `after-file-final/summary.json`, PATCH/CD 공통 runtime1건은 `after-cd-dispatch.log`·`after-cd-dispatch-exit.json`에 있다. CD 추가 단언의 교정 전 반례는 `after-file/`에 보존했다. 빌드 로그는 `extracted/cpp-resume-20261121/build-final-targets.log`와 `extracted/cpp-remaining-tests-1324/file-rebuild.log`다.

| 최신 실행 파일 | SHA-256 |
| --- | --- |
| `guide_fort_request` | `5900c6243ea0025f4108ac355a9a2fd313b9b3e14f7fc6a64c480dcad0b29bb8` |
| `menu_session` | `89b11ffd159c795eb21246b35c18b4c7883edccd3db790a1c236a24ebe3b00f9` |
| `main_popup` | `ef03a3381fcd481e185fe4230e080ef9c23eebc68f7d8f2b69d03d12bb46817a` |
| `file` | `73264cd3ff00affe9c5742f65e51978634aebcfffbacd39ddc9f33f031e7dc2b` |

검사 계약 교정1324는 완료다. **같은 최신 빌드의 선정752건 일괄은 M8 관문 검증으로 진행 중**이며 이 단위의 완료와 구별한다. 1321의 기존325개 이름과 교집합0을 확인했으며 다시 실행하지 않는다. 기존752관측이나 이11건 통과를 전체 솔루션 통과로 확대하지 않는다. [기계 판독 기록](../../cpppj/remaining-test-contracts-1324-20261122.json)에 선정 목록·최초 실패·현행 exe 정체성과 범위를 담았다.

## 보호와 범위

원본 게임·별도 게임 창 실행은0이다. AGENTS.md·originals/originalCD·기존 fixture·README 개요와 DN/WB 파일/인계는 보존했다. 마지막 전체 디컴파일 Client 양판본2026-11-03·R2026-11-07, 호스트HJOW-Athlon을 유지하며 재디컴파일/원문 수정은 없다. CD10.72의 새 플레이 경로와 기존 선택 보류는 재개하지 않았다.
