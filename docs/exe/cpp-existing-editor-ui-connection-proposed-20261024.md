# 기존 Editor checkpoint 이후 실제 Client UI와 State 연결 제안

2026-10-24 현재 **미적용 제안 사본이며 C++ 빌드·링크·검사·게임·GUI 실행은 모두 미수행**이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. [앞선 정적 분석](cpp-existing-editor-after-checkpoint-20261024.md), 실제 10.78 PE, 현 `RawLocalMissionState`·`RawCampaignSession`·`GameWorld`·`UberGump`를 읽었다. 동결된 Random owner `4a137a87…`와 Subscriber `99ee09dc…`는 수정하지 않았다. 원본/CD·공유 소스·CMake·LEFT·git 기록·공용 build/lib는 변경하지 않았다.

`extracted/cpp-existing-editor-ui-20261024-proposed/`의 `before/`는 **단위383 커밋 a9af29da → Random owner4a137a87… → Subscriber99ee09dc…**를 사본에만 순서대로 적용한 기준이다. 준비 중 확정된383의 FullDeck 소비자 교정도 before/copy 양쪽에 보존했다. 도구는 해당 확정 커밋을 별도 신규 사본으로 다시 읽고 두 선행 patch를 적용해 before 전체 문맥과 같은지 확인한다. `copy/`와 형제 `cpp-existing-editor-ui-20261024-proposed.patch`는 그 뒤에 적용할 후속 검토용이다. 이 patch는 실제 State 저장소와 재사용 가능한 작은 접합을 마련하지만, 아래의 최종 UI scope/전체 외곽/loaded 연결을 이미 끝냈다는 뜻은 아니다. `ExistingEditorAfterCheckpointTests.cpp`의 네 검사는 적용후 검사 초안이다.

## 제안 사본에 있는 구체 변경

| 구간 | 실제 소유/호출 | 사본의 범위 |
|---|---|---|
|State 저장소|`ClientLocalStateGlobals::Blocks/State`|old/phase/immediate/current/next의 별개144BYTE 다섯 개를 Client 한 개가 소유한다. `Client::LocalStateGlobals()`와 `services.localStateGlobals`가 같은 소유자를 전달한다.|
|활동 기한|같은 소유자의 `ActivityEnd()`|00544d58 double 한 개를 Editor와 Login이 직접 빌린다. Session 교체에서 초기화하지 않는다. 기존 Login은 소유자가 없을 때만 원래 local 블록/기한을 쓴다.|
|현재 명령|`HudModeCommand/AddIslandCommand`|PE 초기값0200003d/02000022의 실제 DWORD를 Client 수명에 보관하고, 각 원문 호출 위치에서 읽는다. 정적 주소→상수 변환표를 매번 현재값인 것처럼 읽지 않는다.|
|HUD|`RawButtonControls::NotifyHudMode(command)`|기존 동일 `GumpInput::Route(command,1,0,true)`를 호출한다. 캡처/dialog/root 우선순위·자식 우선·숨은 자식 포함·성공 후 현재 캡처 해제는 기존 몸체를 재사용한다. 포인터 폴링 기록을 새로 쓰거나 트리 Broadcast로 치환하지 않는다.|
|음악/팔레트|`ExistingEditorAfterCheckpointEffects`|음악은 기존 `ClientAudio::RequestMusic("ser22.mus")`에 연결할 필수 경계다. 팔레트는 실제 `PathSpec("fortPal",0,0,0)→004b6100` 부모 전체를 요구하며 미연결 장치에 기본 성공을 넣지 않는다.|
|빈섬|`RawCampaignSession::IslandCount()`|실제 `RawIslandRuntime::Count()`가 현재 `islandBytes` 첫 DWORD를 읽는다. 구식 배치용 `IslandList::Islands().size()`·미니맵·Gump 개수를 쓰지 않는다.|
|Add Island|같은 `Client::Input()`|현재 명령을 `InputQueue::PushCommand(id,0,0)`으로 기존40칸 고리에만 예약한다. 정규화/39개 유효칸/포화 때 원래 tail 복구는 기존 큐 몸체를 재사용한다. 이 함수는 창이나 섬을 만들지 않는다.|
|State 4|`RawLocalMissionState::Step`|**PE004b874c의 네번째 엔트리는004b86bd**다. 기존12/14와 같은 공통 확정만 하도록 case4를 추가했다. next4는 다음 Poll에서 소비한다.|
|GameWorld|기존 생성자의 명시 `deferConnection=true`|ExistingEditor에 한해서 로컬 로그인 없이도 실제 Session/표시 소유자를 먼저 만들 수 있다. Editor의 자동 Campaign.Load를 거부하며 Campaign의 기존 장치 검사·로그인·Load 순서는 유지한다.|

State 저장소 초기값은 PE 이미지의 해당 다섯144BYTE와 기한8BYTE가 모두0인 점을 정적으로 확인했다. command DWORD는005427ec=0200003d,00542780=02000022다. 이것은 **정적 PE 이미지/BSS 값**이며 새 CRT/원본 실행 관측으로 세지 않는다. 실제 초기 State 설정은 기존 시작/메뉴 부모가 이후 쓰는 그대로 이어야 한다.

## tail helper와 전체 State의 역할

`ApplyExistingEditorAfterCheckpoint(owner,input,effects)`는004b743b~004b74ec의 현재 분기만 담당한다. 선행 외곽이 만든 ESI1/EBP0 문맥을 전제하고 다음 순서를 실행한다.

1. 현재 HUD 명령 → 실제 Route(includeHidden1).
2. 매호출 `Client::WallSeconds()` 반환+10.0 → 같은 `ActivityEnd`.
3. `RequestMusic("ser22.mus")` → **그 뒤 현재 current[0] 재조회**. 6이면 다른 BattleOver 분기로 인계하고 한정 Editor 성공을 거부한다.
4. 실제 fortPal 조회/전환 부모 → **그 뒤 현재 current[0] 재조회** → next[0]=4 무조건 쓰기. 나머지140BYTE는 보존한다.
5. 현재 요청3이면 자연 생략, 그 외 실제 Island count가0일 때만 현재 Add Island 명령을 일반 입력 고리에 예약한다.

이 helper는 old/phase/current의 공통 확정, Poll, `loaded`, UI 표시 phase를 쓰지 않는다. 실제 `RawLocalMissionState::Run`의 `parentState`에서 앞선 prefix와 `InstallExistingEditorFort(actualHooks)` 및 이 tail을 순서대로 실행한 뒤 `Commit`을 반환해야 한다. Run이 기존 **현재 immediate 여부 캡처→old=phase144→phase=current144→current 첫DWORD0→필요한 immediate144 재실행**을 소유하게 한다. 이 복사를 Editor callback에서 중복 구현하지 않는다.

정상 요청1/2·immediate0의 첫 ret는 phase1/2/current 첫DWORD0/next4다. 이후 **다음 UI 루프 State 자리에서 Poll 한 번**이 State4를 공통 확정한다. `state_.Post({"..."})`의 선착순 문자열 큐는 현재 UI command 예약용으로 남길 수 있으나 실제 native next 덮어쓰기·144BYTE 부가 인자·phase를 대신할 수 없다. `FortSelectionGlobals.nextState`와 `currentState`는 각각 같은 owner의 next[0]/phase[0]을 빌려야 한다.

`RawLocalMissionState`의 기존 Login용 `parentState`가 비어 있다는 사실도 유지한다. 기존 Editor에서 실제 큰 부모를 연결한 분배기만1/2를 실행한다. 공유 owner를 도입했다고 모든 미복원 큰 상태가 자동 복원되는 것은 아니다. 인접 bool/DWORD 전역은 현재 각 Session의 실제 owner를 계속 빌리며, 이번 두 저장소만으로 전체 전역 수명 복원이 끝났다고 하지 않는다.

## 아직 사본 patch에 넣지 않은 최종 UI/loaded 연결

현재 `RawCampaignSession.cpp`의 production sync는 !loaded를 거부한다. `RawProductionControls.cpp`의 패널 생성자는 HUD mode(first1)를 직접 호출하며, 같은 처리기가 `SynchronizeProductionContents()`를 실행한다. `UberGump::Compose`는 앞부분에서 `session.Cursor()`도 즉시 조회한다. 따라서 `Compose()`를 그대로 호출하려고 loaded를 먼저 켜는 방식은 사용할 수 없다.

root의 실제 전체 외곽 조립 시 다음 세 단계를 좁게 분리하는 것을 제안한다. 아래 이름은 **새 연결 API 제안**이며 현재 제품에 이미 존재하는 메서드가 아니다.

1. **`PrepareExistingEditorWorldUi`**: 실제 World/Deck 및 현재 필요한 설치 하위가 정상 종료했을 때, Campaign.Load 마지막 부분의 같은 `ConnectDevastation/ConnectCarrierDrop/ConnectPriestPrayer/MakeBuilder/ConnectGameplayPlacement/ConnectCursor`를 재사용해 한 번 구성한다. Campaign.Load를 다시 부르지 않는다. 객체/Player/Deck를 재로드하지 않는다.
2. **`WithExistingEditorInstallUi(body)`**: 현재 ExistingEditor/World 준비/중복 진입을 검사하고 설치용 scope만 열어 실제 패널 준비와 원문 tail을 실행한다. `SynchronizeProductionContents`의 동일 본문 및 **이미 생성된 실제 Cursor 조회**에 한해서 이 scope를 허용한다. 모든 입력/AI/공개 Snapshot의 !loaded 계약은 유지한다. 예외 시 scope만 반드시 닫고 앞선 상태/객체/장치 효과는 되돌리지 않는다. scope 종료로 loaded를 켜지 않는다.
3. **`CommitExistingEditorInstall`**: 실제 큰 외곽의 prefix→파일/Subscriber→World/Deck→사제/카메라/생산→Core/private Snapshot→State/Notify→startupStateCheck→현재 EditorPlayers/맵 후처리→위 tail→RawLocalMissionState 정상ret가 끝난 호출자만 실행한다. 같은 owner 주소/ExistingEditor 모드/미완료 상태와 각 실제 단계 완료를 확인하고, 정상 선택1/2·current0·immediate0·next4 및 tail 정상복귀를 확인한 뒤 마지막 한 곳에서 `impl_->loaded=true`를 쓴다. 그 다음 실제 사용자 장치와 외부 Kernel 표시를 공개한다. 공개 Snapshot은 이 확정 전 계속 거부한다.

scope는 `editorUiInstalling`과 같은 내부 표식 한 개로 제한할 수 있다. 이것은 원문 전역을 복제하는 플래그가 아니라 C++ 조립 중 public UI가 완성 상태를 요구하는 문제를 한정하는 보호 장치다. World/Deck가 준비됐다는 이유만으로 독립적인 임의 callback이 마지막 commit을 얻지 않게, **전체 외곽의 정상 반환/동일 State 실행 결과**를 소비하는 private 호출 관계로 묶어야 한다. 단순 bool setter나 검사 전용 강제 API를 만들지 않는다. 실제 원문 하위가 immediate를 요청해 다른 상태까지 실행했다면 그 분배 결과에 따른 완료/전환을 먼저 처리하고 Editor 완료를 강제하지 않는다.

## UberGump와 실제 HUD 재사용

`UberGump::LoadMissionWorld`는 Script/MissionRuntime/Guide/로컬 로그인 필수 조건이 있으므로 Editor의 생성 경로로 호출하지 않는다. Editor는 `services.installMode=ExistingEditor`, 같은 Template/Badges/변환표/난수/State owner, 실제 작업본 `missionFortImage`, 실제 작업본 파일경로를 전달한 명시 지연 `GameWorld`를 만든다. pending/current Fort 이름, `EditorFiles`와 Template 저장소는 현재 Client/UI가 Session보다 오래 소유해야 한다.

`Compose`의1208~1220에 있는 생산/미니맵 생성·Resize·SetPalette 몸체만 **`EnsureWorldControls` 같은 공통 private 본문으로 추출**해 기존 Campaign과 새 설치 scope가 같이 쓰게 한다. 전체 Compose를 선행 실행하면 Cursor/표시/입력 공개까지 섞이므로 이 작은 UI 생성 본문만 먼저 재사용한다. HUD 노드가 원래 트리에 아직 없다면 원문 Route의 미처리0을 그대로 보존한다. 노드를 숨겼다는 이유로 includeHidden1을 생략하지 않는다. 현재 패널의 생성자 자체 HUD 통지와004b743b의 별도 HUD 통지는 서로 다른 호출이므로 임의로 한 번으로 합치지 않는다. 새 패널 생성 위치의 최종 원문 수명 일치도 통합 검증에서 따로 확인한다.

표시용 `ClientPhase`에는 Editor를 별도로 두는 편이 현재의 MissionRuntime 필수 가정을 드러낸다. 다음 지점만 실제 활성 world의 공통 판정으로 좁혀 재사용하고 미션 Script/Guide 전환은 기존 조건을 유지한다.

- `Compose`: 활성 world의 스프라이트/커서 제출, 모달 배경 합성, 현재 HUD 표시.
- 입력/폴링: 현재 UserInput/커서/생산 단축키/선택/미니맵 경로. 사용자 입력은 마지막 loaded 확정 뒤에만 공개한다.
- `OpenGameMenu`: 실제 world·loaded·활성 화면을 검사하고 `missionRuntime_`가 없는 Editor의 **현재 Mission 존재를0**으로 공급한다. 실제 존재하는 미션의 editingMission/demo는 기존 Runtime에서 계속 읽는다. 가짜 Runtime을 새로 만들지 않는다. Editor/Battle/Mission 모드는 이미 `session.BindGameMenus`의 실제 globals getter를 사용한다.
- ESC: 현재 `mission_ && phase==Mission` 조건만으로는 Editor에서 메뉴가 열리지 않는다. Editor 활성 world를 같은 메뉴 진입에 포함한다. Restart/EndGuide 같은 미션 전용 명령을 null Runtime에 전달하지 않는다.
- 제목과 맵 목록: 현재 Fort 이름/실제 작업본 경로/owner를 읽고 기존 미션 이름을 만들어 넣지 않는다. `MapMetadata`의 pending 이름과 설치 완료 current 이름을 구별한다.

`GameWorld::SetPalette`는 현재 Session.Cursor 조회를 경유하므로 실제 palette callback이 새 scope 안에서 동작해야 한다. `BindCampaignUserInputServices`와 Cursor mana/선택 API의 loaded 계약을 일괄 풀지 않고, 마지막 commit 뒤 같은 실제 raw 커서에 사용자 장치를 붙인다. 이미 있는 `CampaignObjectMenuClient`, 생산/미니맵/PaletteChanged는 그대로 재사용한다. world 포인터가 Subscriber 조회에 먼저 필요하면 포인터를 등록하되 **미완료 world를 외부 Kernel 프레임에 먼저 등록하지 않는다**. 실패 정리에서는 UI가 Session을 빌리는 순서에 맞춰 기존 `ClearWorld`처럼 UI부터 반납한다.

별도 읽기 검토에서 다섯 블록의 순서/참조 struct와 tail의 재조회·예약 순서에 명백한 불일치는 발견되지 않았다. 또한 `make_unique<GameWorld>` 생성자가 반환하기 전에는 `menu.world_`가 아직 null일 수 있으므로, 생성·설치 중 Template callback은 **그 시점의 실제 Session/Impl 참조에서 직접 공급**해야 한다. 같은 주소를 나중에 얻을 수 있다는 이유로 null인 메뉴 getter를 선행 호출하지 않는다. 이 검토도 실제 빌드/실행과 구별한다.

Editor 사용자 명령 전송은 별도 경계다. 기존 Campaign 커서 send가 호출하는 `SendLocalPacket`은 실제 Login/transport가 필수다. 로컬 로그인 없이 설치한 Editor에 가짜 접속을 붙이거나 패킷을 즉시 서버 처리하는 방식으로 대체하지 않는다. Editor 원문의 직접 명령/전송 경로를 별도 확인한 실제 adapter가 필요하며, 그 전에는 설치 화면의 존재를 사용자 편집 전체 완료로 표시할 수 없다.

## 팔레트와 Add Island의 남은 필수 장치

음악 adapter는 `client.WorldAudio().RequestMusic(name)` 또는 같은 실제 활성 ClientAudio provider를 사용한다. `StartScene(false)`는 추가 음악 정리 부모 효과가 있으므로 이번 요청을 대신하지 않는다. 벽시각은 `client.WallSeconds()`이며 `Time().game` 또는 정지 가능한 누적 게임 시각이 아니다.

palette callback은 병렬로 준비된 `RawFortPaletteTransition`의 실제 이름 버퍼/ColorSort 캐시·계산/PathSpec/arg2=1 화면 장치/현재 renderer dirty를 같은 Client owner에 연결한 전체 부모를 호출해야 한다. `Client::LoadScenePalette`는 기존 소비자 전파 본문으로 재사용하되, 그것만 호출해서004b6100 전체가 복원됐다고 하지 않는다. 이 장치가 없으면 helper의 해당 필수 경계에서 이전 HUD/기한/음악 효과를 남기고 중단한다.

Add Island 예약 뒤 실제 소비자는00480090→가상+20(0)→00466600의 편집창이다. 현재 `GameMenuCommands`에ID 매핑은 있지만 일반 분배기의 미복원 메뉴 명령 throw와 실제 창/배치가 남아 있다. 섬0 입력에서 명령 예약은 이 제안의 관측 범위이고 창 열림/사용자 섬 배치는 다음 구현·GUI 검증 범위다. 기존 Fort가 섬을 갖고 있으면 이 하위는 자연 미선택이다.

## 검증·인계

네 검사 초안은 실제 Client Run 없는 소유자/getter·Config 변경/외부 공유 수명, 실제 `RawLocalMissionState`의 전체720BYTE 확정과 다음Poll4, 실제 `InputQueue` 예약, callback 뒤 현재 요청/명령 재조회, immediate14와 next4 구별, palette 누락의 부분 효과를 검사한다. 기대 순서는 PE 정적 명령에서 정했고 제품 reader로 기대를 만들지 않았다. 새 Native fixture·원본 실행 또는 전체 Editor 완료 검사가 아니다.

`verification.json`은 원문 PE의 정적 주소/초기값·함수 바이트 SHA, frozen 선행 patch SHA 보존, 사본 patch의 문맥 검사와 UTF-8 검사, 검사4개 **미실행**, 공유 파일의 준비 당시 SHA와 현재 변경을 구분한다. root는 준비 도중 단위383의 실제 Editor 모드 소비자 `fortDestroyBlocked=true`를 고쳤다. 후속 diff는 그 ctor 구간을 건드리지 않으며, 과거 shared SHA를 강제하거나 root의 교정을 덮어쓰지 않는다.

최종 통합에서는 기존 Campaign 진입/자연 Early1/ESC·해상도·설정 회귀를 유지하고, 실제 Editor 첫 ret의 phase1/current0/next4와 다음 Poll의 phase4, 설치 중 공개 Snapshot 거부→마지막 완료 뒤 허용, 실제 HUD/기한/music/palette/현재섬Count/입력 예약 순서를 관측해야 한다. 이후 실제 작업본 저장/별도 Session 재로드·Add Island 창/사용자 배치·ESC/Return은 별도 확인한다.
