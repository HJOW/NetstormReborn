# Edit·커스텀 맵·시나리오의 실제 저장 경로 잔여 감사

2026-10-23 단위376 진행 중의 읽기 전용 조사다. LEFT_JOBS.md는 최신 상단95행만 읽고 관련 문서와 현재 cpppj 소스를 확인했다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon / 2026-10-23이다. 게임·GUI·빌드·검사·git을 실행하지 않았으며 신규 분석 문서 외 제품/공용 파일을 변경하지 않았다. 아래 상태는 소스 연결의 정적 증거이며 실제 사용자 저장 성공 관측이 아니다.

## 현재 경로에서 막히는 정확한 위치

| 단계 | 구현된 부분 | 사용자 경로의 남은 차이와 근거 |
|---|---|---|
| 메인 Edit | 원본 돌 버튼 생성·입력·표시 | [UberGump.cpp](../../cpppj/src/client/UberGump.cpp):1093의 Edit 동작이 아직 `Tell,NotImplemented`다. |
| 파일 목록 | SoloMapCatalog의 실제 디스크 열거·분류·현재 FileSpec 및 FORTLIST/SOLOLIST 열 분할 | UberGump:348~357은 실제 ReadFort 행/문자열 토큰을 만든다. 하지만 Supported:143~147, DialogScriptCode:58~68, Execute:986~994에는 FortSelection 코드가 없다. 따라서 행 표시를 실제 선택·설치 완료로 볼 수 없다. |
| 선택 명령 | [FortSelection.cpp](../../cpppj/src/client/FortSelection.cpp):31~56의 c9/ca 모드/pending/현재 State 보존/next 요청 | `DispatchFortSelectionCommand`와 `QueueFortSelectionAction`의 제품 호출자는 없다. UberGump의 현재 State는 [State.cpp](../../cpppj/src/client/State.cpp):6에서 먼저 대기한 요청이 있으면 새 요청을 무시한다. 원본 c9/ca의 현재 DWORD 덮어쓰기와 동작이 달라 별도 원본 next 저장소/소비 경계가 필요하다. |
| Editor 설치 | [RawFortWorldLoad.cpp](../../cpppj/src/o/RawFortWorldLoad.cpp):32~43의 기존 Fort/alternate 부모, [RawFortWorldLoadBinding.cpp](../../cpppj/src/client/RawFortWorldLoadBinding.cpp):8~25의 실제 객체·Money·Meta·지형·Graph child 결합 | 실제 Session은 아직 캠페인 전용이다. [RawCampaignWorldRuntime.cpp](../../cpppj/src/client/RawCampaignWorldRuntime.cpp):61~62의 Money/Badges/Technology 및 alternate populate는 명시 throw다. Load:89~93은 campaign->Load만 호출한다. |
| 생성 전 모드 | 기존 Core/Player/TYPE/Factory·현재 값 참조 | [RawCampaignSession.cpp](../../cpppj/src/client/RawCampaignSession.cpp):703~704는 mission/chaff/전투 값을 초기화한다. [RawCampaignWorldLoad.h](../../cpppj/src/o/RawCampaignWorldLoad.h):12의 missionPost 기본값은true, editor는0이다. 일반 캠페인 Load 후 editor 하나만 켜는 방식으로 원본 State1/2를 대신할 수 없다. |
| 현재 월드 snapshot | TypeNames/Deck/Technology/Territory의 실제 Session writer | Session:3204~3223에 네 명시 snapshot API가 있다. Objects는3228~3245의 **단일 청크**뿐이다. 전체 Terr00~19/Chaff 부모, Money/Mission/Badges writer 및 전체 호출 순서·마지막 Notify는 없다. |
| 이미지·파일 쓰기 | [CampaignTemplateStorage.cpp](../../cpppj/src/client/CampaignTemplateStorage.cpp):261~280의 같은 manager 이미지→원본 저장 부모, [EditorFiles.cpp](../../cpppj/src/client/EditorFiles.cpp):23~58의 보호 경로/작업본/권한/쓰기 | EditorFiles의 제품 사용은 자기 구현뿐이다. Session:788~793은 TemplateState를 구성하지만 BindFileSave를 호출하지 않는다. [RawPlayerTemplateState.cpp](../../cpppj/src/o/RawPlayerTemplateState.cpp):79~89의 Notify는 파일명·isMine·미션·preventSave 관문 뒤 save가 없으면 명시 실패한다. |
| Save as/NewFort·실제 편집 명령 | [RawGameMenus.cpp](../../cpppj/src/client/RawGameMenus.cpp):81~84가 Save as/Add Island/다리 품질 행을 작성한다. Game:49~51에는 Test Battle/Return to Editing도 있다. | 명령 ID가 있다는 것과 실행된다는 것은 다르다. GameMenuCommands:13/22/27은79/c2/d6을 알고 있지만 [GameMenuActions.cpp](../../cpppj/src/client/GameMenuActions.cpp):12~14의 처리9명령에는 없다. UberGump:869~870은 미연결 메뉴 명령을 명시 오류로 보고한다. |

`OpenGameMenu` 자체도 UberGump:638에서 `missionRuntime_` 및 Mission phase를 요구한다. Editor 전용 세션이 만들어져도 메뉴·입력·표시를 실제 Editor 수명에서 열도록 이 상위 조건을 원본 상태 소비자에 맞춰야 한다. 기존 창 제목 모듈의 Editor/testBattle 분기는 [WindowTitle.cpp](../../cpppj/src/client/WindowTitle.cpp):11~18에 있지만 UberGump:457~459는 현재 Mission 정보만 공급한다.

## 저장 하위에서 재사용할 것과 새로 필요한 것

실제 파일 저장까지 이어지는 저수준 경로는 이미 있다. [EditorFilesTests.cpp](../../cpppj/tests/EditorFilesTests.cpp):25~38은 TEST01 작업본을 준비하고 BindFileSave→Notify→SaveFile→실제 파일 재읽기를 구성한다. :69~89는 FortSelection과 같은 저장 장치의 결합도 검사한다. 이들은 현재 사용자 UI의 성공 증거가 아니라 재사용 가능한 연결 예다. 원본/CD를 사용자 저장 경로로 쓰지 않고 실제 작업본 namespace를 FileSpec·현재 이름·manager 이름·VFS 읽기에도 일관되게 적용해야 한다.

현재 snapshot에서 가장 작은 독립 후속 단위는 **동적 Temp 노드와 전체 Objects 부모**다. 기존 단일 객체/청크 writer와 같은 Finder/Hash/Pool/명시 stream을 그대로 쓰면서 Terr00~19와 Chaff를 모두 새 현재 월드로 교체하는 범위를 완성할 수 있다. [CampaignTemplateStorage.cpp](../../cpppj/src/client/CampaignTemplateStorage.cpp):19~21의 LoadSection은 표준35개 이름밖의 노드를 거부하며, CreateSection:108~109도 그 함수를 사용한다. 따라서 Temp를 Terr00의 다른 이름으로 가장하거나 기존 raw 섹션을 그대로 저장하는 것으로 대신할 수 없다. stream 생성 수명·unlink/free·rename 및 현재 선택 변경을 같은 manager에 연결해야 한다. [객체 저장 기록](cpp-fort-object-save-20261023.md)의 마지막 두 절이 원본 부모004be430/Temp/버전 전역의 남은 경계를 정리한다.

그 다음 세 writer는 다음처럼 기존 실제 저장소에 연결해야 한다.

- **Money:** 현재 Session의 실제 SP와 Impl::random을 써야 한다. 별도 난수 사본 또는 과거 Fort Money 섹션을 재사용하면 snapshot 뒤 난수 소비 순서·현재 자원이 달라진다. [snapshot 분석](cpp-fort-snapshot-basics-20261023.md)의 마지막 절에004832c0의 Next(0x3f2)+333.0f 경계가 있다.
- **Mission:** 실제 UberGump::MissionRuntime::state/현재 MissionScript/flags와 파일명을 사용해야 한다. guided Tutorial 전용 getter만으로 존재를 판단하면 일반 Battle을 놓친다. 같은 분석 문서의 마지막 절을 따른다.
- **Badges:** [RawFortMetaLoad.h](../../cpppj/src/o/RawFortMetaLoad.h):38~39의 로더는 장기28×220BYTE 실제 장부를 요구한다. client 전체의 현재 참조는 loader API와 원시 Template cursor뿐이고 실제 Session 장부/Meta 인스턴스는 발견하지 못했다. 원시 Badges 섹션을 live 장부라고 간주하지 말고 소유자·초기화·수량 갱신과 writer를 함께 복원해야 한다.

최종 전체 snapshot 부모004bf9a0은 위 child와 이미 있는 네 writer, State/core의 원본 Editor 관문, 마지막 Notify를 원본 순서에 묶는 별도 경계다. 현재 부분 snapshot을 파일로 쓰는 동작을 전체 사용자 저장으로 보고해서는 안 된다.

## 다음 작업의 우선순위

1. **작게 검증 가능한 저장 핵심:** Temp 동적 노드/rename 수명과 전체 Objects 부모를 먼저 구현한다. 기존 실제 한 청크 writer를 재사용하여 서로 다른 두 지역 및 Chaff의 현재 변경이 저장·재로드에 모두 반영되는지 확인한다. 이어 Money/Mission/실제 Badges와 전체 snapshot 부모/Notify를 잇는다. 이미 끝난 이미지·OS 파일 writer를 다시 작성할 필요는 없다.
2. **generic Editor 설치:** 현재 Session의 공통 소유 모듈을 재사용하되 첫 객체 생성 전에 원본 Editor/missionPost/fullDeck/inFortMode를 설정하는 진입 부모를 둔다. 기존Fort Load0에는 실제 Money·Badge장부·현재 기술 변환/내용물/UI를 연결하고, 새Fort alternate1에는 실제 populate를 연결한다. Editor Player2~8의004904b0은 모든 eligible 타입 직접 Create 경로이며 기존 명시 owner Ensure와 치환하지 않는다. [메뉴·설치 의존성 기록](cpp-tell-map-menus-20261023.md):55~57을 따른다.
3. **Client에서 실제 끝까지 연결:** 장기 현재 이름/pending/current Fort/원본 next 상태를 분리하고, 보호 원본의 작업본을 준비한 뒤 실제 파일 namespace·공유 Template9·BindFileSave·snapshot 수명과 묶는다. 그 뒤 메인 Edit→c9→LoadEditable→ca→다음 State의 실제 설치를 연결한다. 현재 모듈이 이미 갖춘 선택 순서를 즉시 월드 생성 또는 State::Post의 선착순 큐로 바꾸지 않는다. 이 단위의 성공 기준은 사용자가 편집한 현재 월드가 실제 작업 파일에 저장되고 다시 열렸을 때 보존되는 것이다.
4. **NewFort/Save as/시나리오 왕복:** `$Input`을 실제 TextEntry 수명/명령으로 연결하고 이름 변경·새 파일·추가 섬·다리 품질·Test Battle/Return을 복원한다. [DialogScript.cpp](../../cpppj/src/client/DialogScript.cpp):168~187은 현재 Key/Input/Zsend를 명시 거부한다. 입력칸 없이 임의 기본 이름으로 저장하는 우회는 사용자 기능 완료가 아니다.

1과2의 독립 원문·모듈 복원은 병렬화할 수 있다. 3의 버튼 활성화만 앞당기면 현재 generic 설치 throw/미연결 저장에 도달하므로 실제 저장으로 이어지는 최소 완결 단위가 되지 않는다.

## 커스텀 시나리오의 구분

현재 [Mission.cpp](../../cpppj/src/client/Mission.cpp):13~19는 missionSpec을 실제 Reader로 읽고, FortName/FortPath는 loadFort/fortSpec 경로를 사용한다. UberGump:472~479의 공장은 실제 tutorial/battle만 지원하며 사용자 파일명도 이 공장을 통해 적재할 수 있다. 직전 [미션 로딩 실행 기록](cpp-mission-loading-runtime-20261023.md)은 TEST01 진입/Cancel을 관측했다. 이는 Edit에서 사용자가 시나리오를 작성·저장·테스트·복귀하는 기능의 근거가 아니다.

AGENTS.md에 따라 `.english` 등 언어 스크립트의 내용은 사용자가 텍스트 편집기로 작성하는 원본 방식이다. 별도 GUI 스크립트 IDE를 새 요구사항으로 늘릴 필요는 없다. 필요한 차이는 사용자 `.fort`와 동명의 언어 스크립트를 같은 실제 작업 namespace에서 읽고, 원본 Test Battle→Return to Editing에서 현재 Editor와 Mission 수명·snapshot을 보존하는 것이다. 현재 NewFort/Input/Test Battle/Return 명령이 미연결이므로 이 왕복은 아직 완료되지 않았다.
