# ExistingEditor 전체 설치의 Session 수명 연결 후속

2026-10-24 읽기 설계다. 현재/마지막 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 공유 소스·CMake·LEFT/git·공용 build/library·게임/GUI를 바꾸지 않았다. 기준은383 a9af29da와384의 동결 owner4a137a87→Subscriber99ee09dc 순차 적용 사본이며, root가 실제 공용 적용한 위치도 읽기 확인했다. 이 문서는 전체 설치 성공 구현이나 실행 판정이 아니다.

## 생성과 실제 파일 설치를 나누는 최소 경계

`RawCampaignSession::Impl`의 현재 ctor 끝 `InstallFile(1,…)/Install(1,…)`와 뒤 `MakeRuntime()`는 약1270행이다. **기본 Campaign 및 LocalLogin은 이 eager 설치를 유지**한다. ExistingEditor 전체 외곽 진입만 명시적 지연 모드를 추가해 보조 fort/원문 요청/현재 경로의 실제 수명을 Impl 안에 보관하고, 파일 부모가 요청할 때 [새 Storage LoadFile](cpp-template-storage-load-proposed-20261024.md)을 실행한다. 기존 단독 `LoadExistingEditorWorld()` 검사가 쓰는 설치 기반 계약을 묵시적으로 전부 바꾸지 않는다.

가능한 좁은 API는 `BeginExistingEditorInstall()` 한 개와 그 전용 `deferExistingEditorFile` 입력이다. ctor는 loaded=false 상태로 실제 모듈/난수/Storage/Runtime 주소만 구성하고 Begin이 필수 파일/Config/표시/State 서비스를 받아 원문 부모를 조립한다. 기존 default false 입력은 Campaign/LocalLogin 및 기존 단계별 검사를 유지한다. 지연모드는 ExistingEditor에서만 유효하며 Campaign으로 자동 전환하지 않는다.

`templateState`의 약821행 Get/Name 콜백은 생성 시 조회하지 않는다. `MakeRuntime` 약2548행의 templateAt/territoryAt도 lazy이며 `RawCampaignWorldRuntime::Impl`은 헤더/span/필수 모듈의 동일성만 검사한다. 따라서 **실제 Runtime을 먼저 소유하되 manager는 나중에 공개**할 수 있다. constructor의 다른 하위에서 Template getter를 새로 호출하는 변경은 이 계약의 회귀 대상이다.

파일 읽기 입력은 `TemplateFileLoadHooks::read`가 원문 호출 위치에서 `BaseFileSystem::TryRead`/실제 작업본 장치를 선택하도록 연결한다. 기존에 앞서 읽은 `missionFortImage`만 조기에 설치하는 경로로 가장하지 않는다. TryRead의 optional 없음/빈 파일은 정상 false로 인계할 수 있으나 실제 해석 오류/장치 throw를 전부 false로 삼키지 않는다. readonly는 별도0041af80의 출처2/4/stat 장치다. 현재 `EditorFiles`에는 prepare/권한/write만 있으므로 실제 exists/read/readonly까지 있다고 주장하지 않는다.

`FortTemplate` 보조metadata는 기존 파서를 그대로 사용하되 성공한 실제 raw 파일과 일치해야 한다. 새 Storage LoadFile은 파서/TypeNames 변환을 복제하지 않는다. 생성 전에 이미 파싱한 선택 요청을 재사용할 때에는 해당 요청의 시점과 원문 File 읽기 시점 사이의 파일 교체를 검토해야 하며, 확인되지 않은 사전 파싱 성공을 원문 File 부모 성공으로 바꾸지 않는다.

## 원문 부모에 공급할 현재 저장소

|원문 순서/부모|현재 실제 연결과 최소 추가 경계|
|---|---|
|외곽 이름 비교→Guide→setName|현재/대기 이름은 Client/UberGump 장기 owner.00460980은 같은 Storage 이름복사/Notify/스냅샷·정리 의미가 있어 InstallFile 재설치로 대체 금지.별도 원문 부모가 필수다.|
|File exists→Load004bf550|현재경로를 각각 재조회.신규 Storage.LoadFile은 기존같은/다른이름을 유지하고 생성한 actual manager를 read 전에 공개한다.|
|valid0041aad0|**현재 고정 헤더 첫DWORD 비영 여부만 반환**한다.이 함수를 별도 서명 재파싱/무조건 true로 채우지 않는다.|
|manager004bd340/진단410|각 호출의 owner로 Headers.subspan(owner*20)를 읽는다.이때의 manager 비영 진단과 실제 IsDemo0을 유지한다.|
|State첫BYTE\|1→Notify|같은 `templateState->Get(owner)`의 span을 수정한 후 현재owner를 다시 읽어 Changed.첫 Notify 전에 실제 `BindFortFileSave(Client.ReadOnlyForts(), EditorFiles)`가 필요하다.미연결 save를 no-op으로 두지 않는다.|
|Subscriber→Config→Title|실제 `LoadExistingEditorSubscriber()`/장기 globals.mySubscriber/handle.기존 parsedSubscriber/ReadSubscriberName으로 대체 금지.실제 Config `fort.mySubHandle` 변경callback과 제목이 뒤따른다.|
|World→Deck|현재 LoadExistingEditorWorld/Deck.같은 playerState.templateOwner와 Storage/Badges/TypeConversion/stream을 읽고, editorWorldStarted는 시작 시·editorWorldReady는 실제 World 정상복귀 뒤만 쓴다.|
|Priests→Camera→Production|004923e0/004340d0/0047d890의 현재 실제 부모.모드상 자연생략은 각 원문 몸체가 판단하며 성공 기본hook 금지.|
|CoreWrite004beb30|manager0/Metamap 진단 후 `missionPost==0 && editor==0`에서만 실제 쓰기다.Editor 자연생략도 해당 부모의 진단까지 유지한다.|
|전체Snapshot|현재 private `SnapshotFortBody(owner)`의 동일 RawFortSnapshotParent/Money/Mission/Badges/Objects/Deck/Notify.기술/Mission Create 중복 금지.공개 SnapshotFort의 loaded 요구는 유지한다.|
|State첫BYTE&fe→Notify→startupStateCheck|각 owner를 다시 읽는다.004b5ba0은 Config startupStateCheck0에서 자연 반환하며 Editor bool로 선행 생략하지 않는다.현재 timestamp/벽시각/조건부 Tell이 실제 선택되면 필수다.|
|현재Editor→Config→EditorPlayers|현재 editor를 다시 읽고 `lastEditFort` 실제 Config 후 InstallExistingEditorPlayers와 실제0046e980 tail.여기는004b743b checkpoint이고 loaded 완료점이 아니다.|

파일 부모의 중간 Title과 Subscriber/config 뒤 Title은 둘 다 보존한다. callback이 header/owner/currentName/inMission/editor를 바꿀 수 있으므로 부모가 재조회하는 위치에서 Impl의 현재 필드를 읽는다. Get이 돌려준 State span을 외곽 다른 단계까지 보관하지 않는다. 정상 false의 InvalidFile/없는 파일의 NewFort/현재Mission/빈 이름은 현재 RawExistingEditorInstallOutside로 명시 인계하며 ExistingEditor 성공으로 돌리지 않는다.

## 후반 UI와 최종 loaded

[004b743b 이후 기존 분석](cpp-existing-editor-after-checkpoint-20261024.md)과 fury의 `ExistingEditorAfterCheckpoint` 제안은 현재 HUD 명령→실제 wall+10→ser22→현재요청6 관문→전체 fortPalette→현재요청 재조회/next4→조건부 AddIsland 예약까지만 수행한다. State commit은 여전히 같은 Client의 다섯144BYTE owner와 실제 분배기에 연결해야 한다. next4를 이번 호출에서 임의 드레인하지 않는다.

HUD includeHidden1은 같은 GumpInput.Route이고 public `SynchronizeProductionContents`의 loaded 관문을 통과하려고 loaded를 미리 켜지 않는다. 같은 본문의 설치 전용 진입은 실제 World/Deck 완료를 요구해야 한다. palette의 실제 Kencloud는 **현재 Impl.random(GameRandom)**을 빌려야 하며 새 RNG/CRT로 바꾸지 않는다. Client/메뉴의 world_는 make_unique 반환 전 null일 수 있으므로 설치중 hook은 현재 Session/Impl 참조에서 직접 공급한다.

최종 loaded=true는 전체 외곽→후반 UI/palette/입력→현재State 기록/동일Run 즉시요청 분배의 실제 정상 ret 뒤에만 쓴다. 중간 오류는 이미 수행한 manager/State/Config/파일 쓰기/객체/난수/부분 깊이를 보존하고 loaded=false를 유지한다. 재호출로 부분 설치를 중복 실행하지 않도록 시작 플래그와 최종 완료를 구별한다. 전체 State의 work 요청이 남으면 Editor helper만의 완료로 축약할 수 없다.

Client의 Storage/Badges/TypeConversion/Random/LocalState 및 EditorFiles/readOnly 참조는 Session과 모든 bound callback보다 오래 살아야 한다. Session은 Runtime/하위모듈 해제 전에 같은 Kernel 프로세스를 비우는 기존 종료 순서를 유지한다. 실패 때 원문이 요구하지 않은 Clear/재설치/예외 삼키기를 넣지 않는다. 실제 최초 Editor 메뉴/전체 UI·사용자 편집/저장·별도 재로드는 후속 검증이다.
