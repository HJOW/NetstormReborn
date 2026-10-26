# Guide 미션 수명과 실제 Native mission 모드 해제

- 작업 PC: HJOW-Athlon, 2026-10-26.
- 마지막 전체 정밀 디컴파일: HJOW-Athlon / 2026-10-23. 디컴파일 소스는 변경하지 않았다.
- 범위: Guide 미션 생성/해제와 실제 `00594fa0` 및 `global.inMission`의 수명을 연결한다.

## 원본 근거와 문제

패치판 `00482bf0`은 미션 객체를 만들며 `00594fa0=1`, `00594fa4=1`, `global.inMission=1`을 쓴다. 미션 해제 `00483210`/`00483260`은 보유 스크립트 Config를 해제한 뒤 `00594fa0=0`, `global.inMission=0`을 쓴다.

기존 복원은 일반 Campaign Session을 만들 때 `RawSurfaceStorage::links.mission=true`를 공급했다. Guide 스크립트를 해제하는 `mission_.reset()`은 그와 같은 Native bool을 해제하지 않았다. 전체 `004b2df0` 정리 부모도 원문처럼 이 전역을0으로 쓰지 않고 마지막에 현재 mission을 `00594fa4`에 복사한다. 따라서 캠페인 이탈 뒤 기존 맵 Edit 진입에서 prepared Session이 잔류 `mission=true`를 관측해 준비 계약을 거부할 수 있었다.

## 구현

- `CreateGuideMissionScript`는 실제 `MissionScript`를 만든 뒤 같은 `links.mission`을1로 쓰고 현재 `ConfigInterface::Set("global.inMission","1")`을 호출한다. `00594fa4`는 기존 미션 설치 부모가 계속 소유한다.
- `ReleaseGuideMissionScript`는 현재 스크립트가 있으면 이를 먼저 해제하고, 같은 Native bool을0으로 쓴 뒤 현재 Config에 `global.inMission=0`을 공개한다. 현재 스크립트가 없으면 추가 효과가 없다. Config 알림 예외는 이미 발생한 해제/쓰기 효과를 되돌리지 않고 전파한다.
- 실제 `CreateMission`, Guide 교체, 메인 메뉴 복귀의 스크립트 생성/해제 경계가 이 helper를 사용한다.
- `RetireWorldForState`는 이전 Session/입력/미션 Runtime의 고정 참조만 반납하고 스크립트와 Native mission을 유지한다. 원본 State1은 전체 cleanup 이후 이름 비교→대상 맵의 미션 속성 검사→선택된 `closeGuide` 순서에서만 미션을 해제하므로, `UberEditor.inc`의 실제 checkpoint `closeGuide`를 이 helper에 연결했다.
- 준비 Editor Session 생성자는 파일 지연 중인 mission을 덮어쓰거나 거부하지 않는다. 기존 일반 Editor World의 mission0 관문은 실제 `LoadExistingEditorWorld` 시작으로 옮겼다. 이름이 같거나 대상도 미션이면 원문 외곽이 미션 분기로 빠지며 일반 Editor 성공으로 정규화하지 않는다.
- 현재 World에서 메인 메뉴로 돌아오는 패치판 경로는 기존 `CleanupNativeStateWorld(0)` 전체 부모를 먼저 재사용한다. Editor0/모드0은 그 원문 cleanup이 쓰고, 기존 호스트 `ClearWorld`/Guide 해제와 확인된 State3의 fort1 쓰기가 뒤따른다. Editor0을 호스트에서 직접 합성하지 않는다. 최초 World 없는 메뉴는 기존 시작 경로를 유지한다.
- Native Runtime 반납의 나머지 자식 순서와 증빙은 그대로다. State3 전체의 로컬 listener/메시지·최초 안내/이름 비움/상태 블록 확정은 이번 범위에서 완료로 주장하지 않는다.

## 신규 검증

기존 검사 본문을 바꾸지 않고 `CampaignExistingEditorCheckpointTests.cpp`에 네 사례를 추가했다.

- `CampaignExistingEditorCheckpoint_GuideReleaseClearsSameNativeMissionBeforeEditor`: 실제 Client/TEST01 스크립트/Config의 생성·해제를 실행하고 Config 알림 시점에 스크립트 등록 해제→Native0→Config0 순서를 확인한다. 같은 실제 물리의 prepared Editor Session checkpoint/UI/공통 State 확정/commit까지 이어져 모드 강제 덮어쓰기 없이 설치되는지 검사한다.
- `CampaignExistingEditorCheckpoint_GuideReleaseKeepsPartialEffectsOnConfigNotificationFailure`: 실제 Config 알림을 명시 중단하고 이미 실행한 스크립트 해제/Native0/Config0을 보존하는지, 빈 현재의 재해제가 알림을 다시 호출하지 않는지 확인한 뒤 같은 prepared Session 생성으로 이어 간다.
- `CampaignExistingEditorCheckpoint_GuideLifetimeSurvivesPreparedCtorUntilSelectedLateClose`: 실제 Client의 기존 Guide를 준비 Session 생성 뒤까지 유지하고 원문 이름 비교/미션 분류가 선택한 늦은 closeGuide 뒤에만 World/checkpoint/UI/확정/commit이 진행되는지 확인한다.
- `CampaignExistingEditorCheckpoint_SameNameKeepsGuideAndSelectsMissionOutside`: 이름이 같으면 closeGuide를 강제하지 않고 실제 미션 범위 밖 분기로 빠지며 일반 Editor 설치 완료를 공개하지 않는지 확인한다.

Source2 준비본을 실제 적용하고 최종 Release 빌드 경고/오류0, 새4사례를 포함한 집중103고유/112관측·전체 C++3,165개·실패0을 확인했다. 같은 게임 `c8739bd5…`에서 실제 Early1→메인 메뉴→Early1의 연결 반납/새 토큰/재진입과 자연 Early2 완주→Tutorial3를 확인했다. Script 없는 Editor의 종료 질문 No 뒤 동일 World/표시 복귀도 확인했다. 직접 캠페인→Edit의 늦은 Guide 해제 분기는 신규 실제 Client/Session 검사 근거이며 자연 GUI 완료로 합산하지 않는다. [통합 문서](cpp-editor-runtime-integration-20261026.md), `cpppj/editor-runtime-integration-20261026.json`을 본다.

## source2 적용과 수명 보완

source1의 `RetireWorldForState`는 Script/Native mission을 보존하지만 `MissionRuntime` 전체를 먼저 반납했다. 그 결과 기존 `ClearWorld`의 guided 허용표·missionNumber·tutorialNumber·stage·stageDirty 정리가 실제 Guide 해제 위치까지 남지 않았다. source2는 `extracted/cpp-editor-runtime-466-source2-prepared/`에 별도 준비한 뒤 입력/결과 SHA를 확인해 실제 적용했다. Script 없는 Editor의 HTML 취소/Blank 복귀도 같은 준비본에서 보완했으며 최종 build2 동안 소스는 변경되지 않았다.

- `DetachMissionWorldBindings`는 Tutorial→Start 순서로 Session 참조 실행기를 닫고 호스트 입력을 반납한다. `guided`, `gate`, `state`, 나머지 미션 데이터와 Script는 유지한다. `ClearWorld`와 Native Runtime 반납이 이 경계를 공유한다.
- `ReleaseCurrentGuideMission`은 실제 늦은 closeGuide에서 기존 자기 정지·허용표255DWORD·번호/단계 정리를 실행하고 미션 Runtime의 남은 데이터를 반납한다. 마지막 Script 해제→Native mission0→Config0은 기존 `ReleaseGuideMissionScript`를 그대로 사용한다. MainMenu, Guide 교체, Editor closeGuide, 최종 소멸자가 같은 해제 함수를 쓴다.
- 원문 `004c3210` Tutorial 소멸은 허용표→denySalvage/Ascend→번호/단계→여덟 AI 제거→base Mission 해제 순서다. 이 준비본은 기존에 있던 허용표/번호/단계/자기 정지 효과의 수명 누락만 보완한다. denySalvage/Ascend는 현재 Session 지역이라 Session 이후의 실제 장기 전역으로 보존되지 않는다. AI 제거의 늦은 Guide 소멸 부모와 전체 cleanup이 이미 반납한 자식의 구별도 별도 잔여다. 이 두 부분의 새 코드나 합성0 쓰기를 넣지 않았다.
- 기존 검사 본문·기대값을 바꾸지 않았다. 적용 뒤 집중·전체 C++와 Early→MainMenu→Early 재진입·자연 Early2 완주→Tutorial3·Editor 종료 질문 No 복귀를 통과했다. 직접 캠페인→Edit의 늦은 closeGuide는 실제 Client/Session 검사이며 자연 GUI로 합산하지 않는다. Editor→MainMenu→Quit의 저장질문/SaveGoMain와 Native State3 전체는 아직 미연결이다.

최초 World 없는 메인 메뉴의 여섯 getter 전체가 원문 State3와 동일한 것은 아니다. 현재는 `editor0/fort1/challenge0/missionPost1/playerKnown0/owner1`이고 원문 전체 State3 뒤는 `editor0/fort1/challenge0/missionPost0/playerKnown1/owner1`이다. `RequestQuitApp`은 앞의 세 모드만 읽으므로 최초 Quit의 분기 검증과 여섯 전역의 전체 동일성은 구별한다. 나머지 초기 상태의 정확한 연결은 최초 State3 전체 부모 복원의 잔여다.

## 남은 범위

기존 `d/!state.dat`의 실제 open/8Player 복구/질문과 재접속·삭제 명령은 별도 미완료다. Native State3 전체와 다른 State1/7 분기, Editor 도구·새 맵·저장·Test Battle·시나리오와 전체 미션의 자연 완주 및 원본 동일성도 이 미션 모드 해제 작업의 완료 범위가 아니다.
