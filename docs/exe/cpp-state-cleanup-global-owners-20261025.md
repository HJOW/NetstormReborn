# State 정리 직접 전역의 실제 소유자와 Session 반납 경계

2026-10-25 읽기 전용 조사다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 모두 **HJOW-Athlon**, 마지막 수행일은 **2026-10-23**이다. `RawStateWorldCleanup.cpp/.h`, 현재 Client/Session/UI 저장소와 같은 호스트의 `extracted/refined/originals/Netstorm.c`를 대조했다. 새 디컴파일·Native 실행·게임·GUI·빌드·검사는 하지 않았다. 본 문서만 작성했으며 428의 동결 CPP·검사·CMake·LEFT는 수정하지 않았다.

범위는 `004b2df0`의 **직접 전역 접근, 실제 소유자, 중복 초기화와 반납 순서**다. `StateWorldCleanupEffect`의 각 실제 하위 API/미복원 본문은 별도 조사 범위다. 아래의 ‘없음’은 현재 조사한 제품 연결에 해당 Native 주소의 단일 실제 저장소가 없다는 뜻이다. 원문 자체가 0이라는 뜻이 아니다. 작은 raw 모듈의 값형 검사 상태나 fixture의 주소 사전은 제품 전역 소유자로 세지 않았다.

**430 정정:** 아래 표의 소유 상태는 원래428 시점 조사다. 이후429의 State/Login 공유는 [완료 문서](cpp-state-client-globals-20261025.md)에 기록했다. 이번430의 PE 직접 대조로 `RawIslandRuntime`의 첫 `revision`과 Region 투표/도장의 `revision`은 **0059a8b0**, 섬의 두 번째 `ownerChanged`와 Region `dirty`는 **005949d4**임을 확정했다. 이전 표에서 섬의 첫 인자를005949d4로 분류한 것은 잘못이었다. 아래 해당 행을 바로잡았으며 [두 DWORD 작업 문서](cpp-map-revision-owner-20261025.md)에 기계어 위치/빌드 중단/교정 검증 상태를 기록한다. Native cleanup 부모의 접합 완료를 뜻하지 않는다.

## 바로 사용할 수 있는 실제 장기 소유자

| 대상 | 현재 실제 소유자/접근 | 연결 조건 |
|---|---|---|
| PlayerKnown·두 owner | `ClientGameplayGlobals::player` | 같은 `Client::GameplayGlobals()`를 읽고 쓴다. Session의 `playerState`는 이미 이 객체의 참조다. |
| 현재 Player9·factory | `ClientPlayerRecords::Records()/Base()` 및 내부 factory 할당 | 각 원문 store에서 현재 `Records()`를 다시 읽는다. `Factory()`는 private이며 바깥 adapter의 공개 API가 아니다. 기존 `Use::BindFactory`와 실제 Player 링크 정리/할당 소유자를 통한 좁은 경계로 연결한다. Session의 이전 span을 재할당 뒤 재사용하지 않는다. |
| 현재 Core·캐시·active·메타데이터 | `ClientCoreDataGlobals` | 실제 `Select(player,metamap,0,1)`을 Player 재할당 전에 수행한다. 캐시 복사/active0/현재 NULL의 원문 몸체를 재사용한다. |
| SP·추적 offset·게임/CRT 난수 | `ClientSpGlobals::Store()`, `ClientRandomGlobals` | 별도 SP 초기화/난수 seed를 만들지 않는다. 정리 중 SP 쓰기도 같은 실제 store와 난수를 소비해야 한다. |
| State 다섯144BYTE·활동 기한 | `ClientLocalStateGlobals::Blocks()/State()/ActivityEnd()` | 정리의 `005c8758`은 `phase[0]`이다. `State::phase`의 UI enum이나 문자열 큐를 쓰지 않는다. |
| Fort 현재/pending 이름·목록 | `ClientFortGlobals` | 현재 주소 정체성을 유지한다. cleanup의 Template manager는 이름 목록이 아니라 아래의 실제 Template 저장소다. |
| Template9·직렬화 상태 | `UberGump::templateStorage_`의 `shared_ptr<CampaignTemplateStorage>` | Session도 같은 shared_ptr를 빌린다. 현재 host UberGump가 살아 있는 동안 유지되지만 `ClientFortGlobals` 자체의 멤버는 아니다. Native raw 루트 교체를 host 전체 재생성으로 대체하여 저장소를 조기 폐기하면 안 된다. |
| TYPE·Pool·Hash·spot·일부 모드 | `Client::SurfaceStorage()` | `RawSurfaceStorage`의 실제 배열을 사용한다. `GameAssets::TypeTable()`의 불변 원본 사본을 쓰거나 새 Session의 복사본만 고치지 않는다. |
| NSInfo zone | `ClientNsInfoGlobals::zone` | `005467b8`의 실제 Client 수명 DWORD다. NSInfo 현재 토큰 `current`와 다르다. |
| 세계 redraw | `ClientKencloudGlobals::WorldRedraw()` | `UberGump::worldRedraw_`가 이미 같은 DWORD 참조다. Renderer의 dirty 사각형이나 `SceneMusicState::paletteDirty`와 자동으로 같은 주소가 되지는 않는다. |
| 게임 정지·시계 | `Client::clock_`의 `GameClock` | 중첩 깊이와 누적 정지 시간은 이미 존재한다. 공개 API 범위와 정확한 reset 의미는 아래에 따로 적었다. |

Player 직접0쓰기는 owner1..8의 `+7c,+80,+84,+88,+8c,+90,+94,+98` 여덟 DWORD다. Player0, `+9c` 기술 부모, 이름/Core/동맹 전체를 이 반복에서 지우지 않는다. `ClearPlayerLinks`는 뒤의 별도 자식으로 `+9c/+8/+68` 수명을 처리한다. `players()` callback을 앞에서 한 번 캡처하는 대신 각 실제 DWORD store에서 `ClientPlayerRecords::Records()`를 재조회한다.

## read32/write32 주소별 매핑

‘장기’는 현재 Client 또는 Client의 실제 UI 저장소 수명에 존재함, ‘지역’은 Session/Login이 없어지면 사라짐, ‘분산’은 같은 Native 주소를 여러 제품 필드가 따로 소유함을 뜻한다. 주소를 받아 실제 필드를 고르는 adapter는 가능하지만, 새로운 `map<주소,DWORD>`를 상태 원본으로 두고 이 표를 사후 복사하는 설계는 해당하지 않는다.

| Native 주소 | 정리 본문의 실제 사용 | 현재 필드/소유 상태와 필요한 연결 |
|---|---|---|
| `005c85c0` | 접속 정리 가지 read, 초반 0 store | **지역/분산.** Login의 `zonePending`, `MissionFortLoadRequest`가 만드는 `MissionFortRequestGlobals::sequence`의 외부 참조다. ClientLocalStateGlobals에는 아직 이 필드가 없다. 요청 객체를 새로 만든 뒤 0을 공급하는 것으로 대체하지 않는다. |
| `0054db14` | 초반 0 store | **지역/분산.** Session `fullDeck`, `objectState.fortDestroyBlocked`, `constructionState.confirm.skipBuilderCheck`의 값형 소비자와 HUD `HudRuntime::shown`이 같은 주소 의미를 나눠 갖는다. Deck/생산/Gump 표시까지 한 장기 DWORD를 빌리도록 정리해야 한다. `shown=1`만 바꿔도 같은 전역 전체를 쓴 것이 아니다. |
| `00594fb8` | 모드 진단·저장 read, 후반 0 | **부분 장기/분산.** `RawSurfaceStorage::ownerMode.fort`가 장기 실제 모드지만 `ClientGameplayGlobals::player.fortMode`, Session `assetState/regularState/objectState/finishState`의 fort 값과 `RawSurfaceWorld`의 지역 mode도 따로 존재한다. 모두 실제 소비 위치에 같은 원천을 연결해야 한다. |
| `00594fbc` | 모드 진단·Battle 저장 read, 후반 0 | **부분 장기/분산.** storage `ownerMode.battle`, Session `assetState.inBattle`와 Audio `SceneMusicState::battle`이 별개다. `Session::BattleActive()`는 assetState를 읽는다. storage만 0으로 썼다고 전체 소비자가 즉시 같은 값이 되지 않는다. |
| `00594fc8` | 모드 진단·메타맵 이탈 read, 후반 0 | **장기 일부 연결.** storage `ownerMode.challenge`; 기존 Login/State는 core를 통해 이 bool을 빌린다. 작은 raw 모듈 값형 fallback이 같은 저장소인지 개별 확인이 필요하다. |
| `00540cac` | manager 조회/저장 앞 read, Fort 저장 복귀 뒤 재조회 | **장기.** `ClientGameplayGlobals::player.templateOwner`. `CoreDataGlobals::Select`도 같은 실제 필드를 쓴다. 최초 owner를 전체 정리 동안 캐시하지 않는다. |
| `005467b8` | 0 store | **장기.** `ClientNsInfoGlobals::zone`. 해당 필드만 직접 쓴다. NSInfo 창 생성/재구성 효과를 setter에 섞지 않는다. |
| `00542544` | 1 store | **지역.** Login `scriptBreak`, `MissionLoginTimerState::scriptBreak`가 참조한다. 실제 입력/Abort와 공유할 Client 수명 DWORD가 필요하다. 미션의 `MissionFrameGateState::scriptPaused`는 다른 객체 필드다. |
| `005950ac` | 0 store | **지역.** Session `missionStart.ordinaryGeysers` (`RawMissionStart`가 moreGeysers의 부정을 쓰는 필드). 실제 미션 소비자도 장기 필드를 직접 빌려야 한다. |
| `005950b0` | 0 store | **없음.** 현재 직접 원문 대응 owner를 찾지 못했다. 원문 간헐천/프레임 쪽 read가 존재하므로 미구현이라는 이유로 read를 상수0으로 만들 수 없다. |
| `00595090` | 0 store | **없음.** `SavedStateReadGlobals::tail[0]`의 caller 공급 참조 계약은 있으나 Client owner에 연결되지 않았다. 저장 경과/잔여 전역을 새 Session 지역값으로 대신하면 저장 복원과 cleanup이 갈라진다. |
| `005c85e0` | 0 store | **없음.** 원문 `004b6350`이 1을 쓰고 후속 상태/판정이 읽는다. `requestPhase`, `worldRedraw` 또는 UI rebuild bool과 임의로 합치지 않는다. |
| `005c85cc` | 하위 DWORD만 0 store | **없음.** 원문의 인접 전역과 폭을 보존할 실제 필드가 필요하다. 코드에 `_DAT` 표기가 있다고 double 전체를 지우면 안 된다. |
| `005c85c8` | 0 store | **없음.** 원문 State/Renderer의 별도 분기 전역이다. 공통 State 요청 `current[0]`나 `next[0]`와 다른 주소다. |
| `005c85a4` | 저장/초기 SP read, 후반 0 | **장기.** `ClientGameplayGlobals::world.editor` (DWORD). prepared 대여는 이 값을 재설정하지 않는다. 지역 `BridgeDecayMode.editor`와 여러 raw 호환 필드는 추가 소비자다. |
| `00594fa4` | Battle 저장 read, 마지막에 mission 값 복사 | **장기.** `ClientGameplayGlobals::world.missionPost` (현재 bool 표현). 428의 준비 Editor 건설 두 reader도 이 실제 필드를 빌린다. Native `DWORD`의 임의 비영 비트를 보존하는 저장소와 bool의 폭 차이는 미해결이며 bool 주소에 DWORD memcpy하면 안 된다. |
| `005c8758` | Battle 저장/state29·33 분기 read | **장기.** `ClientLocalStateGlobals::State().phase[0]`. 정리 끝에서 이 값 자체를 임의 변경하지 않는다. |
| `00595344` | 매 Player SP 가지 read, 중간 0 store, 후반 재조회 | **장기.** `ClientGameplayGlobals::player.playerKnown`. `ClientAudio`의 playersReady 값은 별도 사본이다. 원문 중간 0 뒤 후반 조건이 자연 생략되는 경계를 그대로 둔다. |
| `00540c70` | SP owner read | **장기.** `ClientGameplayGlobals::player.battleOwner`. Player0/owner 범위를 편의상 1로 정규화하지 않는다. |
| `0054c8a0` | 메타맵 캡처 뒤 0, Reset54c8a0 뒤 1 | **없음/유사 필드와 구별.** 고정 this에 실제 `00432360`이 실행되는 원문 객체다. HUD `shown`이나 PlayerKnown과 같은 것으로 정하지 않는다. 첫 DWORD만 별도 bool로 만들어서는 이 객체의 다른 필드/생성 효과를 소유하지 못한다. |
| `00540bdc` | signed DWORD→float 변환 read | **없음.** 현재 TYPE 기본값 초기 입력에 해당하는 실제 옵션 DWORD owner가 없다. `MissionAiSettingsState::startingSp`는 `0052f3c8`이므로 대체할 수 없다. |
| `00541240` | TYPE 선택 read | **지역/분산.** Session `geyserType`, `CarrierPostPopState::geyserType`, 각 경로/옵션 selectedType 등이 대응한다. 보호 PE의 정상122를 매번 상수로 반환하는 것은 현재 옵션/TYPE 선택의 복원이 아니다. 실제 한 TYPE 번호 owner로 연결해야 한다. |
| `005467bc` | 0 store | **없음.** 원문 Player/menu 표시 쪽 별도 전역이다. 이웃 `ClientNsInfoGlobals::zone`과 자동으로 같은 필드가 아니다. |
| `0059508c` | DWORD만 0 store | **없음.** 값형 삭제/간헐천 상태 또는 Login clock 필드로 임의 지정하지 않는다. 원문 호출부의 현재 슬롯을 확정해 장기 수명을 구성해야 한다. |
| `005424bc` | 25 store | **지역/분산.** `rewardState.percent`와 Login `optionValue` 및 `RawMissionBattleOptions::value`가 같은 주소를 나눠 갖는다. 하나를 25로 바꾸고 다른 쪽을0으로 남기는 adapter는 같은 전역이 아니다. |
| `005453e4` | 0 store | **지역.** Session `missionStart.allowAnyCapture`; 미션 Start/허용 판정도 같은 장기 DWORD를 읽게 해야 한다. |
| `005ca8f0` | 0 store | **UI 수명.** `UberGump::tutorialSeen_`. `missionNumber_`는 `005ca8e0`, gate.tutorialNumber는 `005ca8e4`로 서로 다른 주소다. |
| `005ca938` | 0 store | **UI 수명.** `RawMenuControls::Impl::popupState.command`; `PopupState()`가 같은 실제 객체를 반환한다. `QuickHelpPopupState::Mark(0,nullopt)`는 인접80BYTE argument까지 지우므로 이 DWORD store의 대체가 아니다. |
| `005c8bb0` | 0 store | **없음.** 원문 추가 분기 전역이며 `Login::archipelago`의 `005c8bb4`와 다르다. |
| `0059a8ac` | 0 store | **UI 수명.** `UberGump::hideBuildings_`; 메뉴/실제 표시 getter가 공유한다. Native raw 루트 교체 때문에 host UberGump를 새로 만들어 조기0으로 바꾸지 않는다. |
| `0059a8b0` | 1 store | **장기.** `ClientKencloudGlobals::WorldRedraw()`, UberGump가 직접 참조한다. 섬 Owner의 첫 인자와 Region 투표/도장의 `revision`도 이 주소를 증가시킨다. 430 source2에서 이 기존 실제 owner에 연결하며 검증 상태는 별도 문서를 따른다. `Renderer::InvalidateAll()`은 별도 동작이고 저장소가 아니다. |
| `005c85a0` | 0 store | **지역.** Login `central`, `MissionServerPacketState::centralCreated`가 빌린다. Client 수명의 중앙섬 생성 DWORD가 필요하다. |
| `0055a420` | 0 store | **지역/미접합.** Login `source`, RawLocalMissionState의 `sourceType`, RawMissionClientPackets의 `sourceTicks`, SavedStateRead tail[1]이 해당 주소다. Client에 단일 실제 필드가 없으며 frame delta/프레임 번호와 합치지 않는다. |
| `005c848c`, `005c8490` | 각 `c61c4000` 비트 store | **없음.** 원문 `004b0b..` 공통 삭제의 unitLost 뒤 기록하는 두 float 좌표다. `GameWorld`의 카메라/스크롤이나 `ConstructionNoticeState::history` (`00558ea4`)와 다르다. 동일한 장기 최근 손실 좌표를 삭제 consumer와 공유해야 한다. |
| `00595088` | 0 store | **지역/분산.** `rewardState.geyserPool`, Login `battleSeed`, `BombEffectState::spentCounter`의 주소 설명이 겹친다. 현재 이름만 보고 별도 전역으로 확정하지 않는다. 같은 원문 주소를 실제 한 DWORD로 통합하고 각 signed/unsigned 읽기만 구별해야 한다. |
| `005c85d4` | 0 store | **지역.** Login `saved`, RawLocalMissionState/패킷/타이머가 참조한다. ClientLocalStateGlobals의 720BYTE 블록 안에 포함된 필드가 아니다. |
| `00594fa0` | 마지막 모드 초기화 앞 read | **부분 장기/분산.** storage `links.mission`과 Session `objectState.mission`, 미션 객체 존재/실제 inMission 표시가 나뉜다. `world_!=nullptr`, 현재 page나 UI phase로 합성하지 않는다. Editor cleanup은 이 현재값을 `00594fa4`에 복사해야 한다. |
| `005949d4` | 현재 read→하위32비트+1 store | **428 당시 지역/UI 분산.** Vortex/Login의 증가, 섬 Owner의 두 번째 인자·Region `dirty`의1쓰기, HUD fullRefresh, 미니맵 dirty의1쓰기/증가/0소비가 같은 주소다. 섬/Region의 첫 `revision`은0059a8b0이다. 430 source2에서 이 주소는 `ClientGameplayGlobals::MapRevision()`을 빌리며 unsigned 감김과store1을 구별한다. |
| `005c7a74` | 0 store | **지역.** Login `begin`, Login/ClientPacket/Timer가 같은 Login 필드는 공유하지만 Editor/Session 교체를 넘는 Client owner는 없다. |
| `005c85b4` | 0 store | **없음.** 원문 State의 별도 연결 표시다. Login `requestPhase`는 `005c85ac`로 다른 주소이며 이 값을 빌려서는 안 된다. |
| `00540bc0` | 마지막 0 store | **지역/분산.** `RawSurfaceWorld::Impl::processState.server`가 현재 여러 게임 raw consumer에 연결돼 있지만 World가 없어지면 사라진다. EditorLocalTransport의 외부 `const bool& server`도 별도다. Native Client 서버 DWORD owner 공백을 wire 생성 때 임의 Session bool로 바꾸지 않는다. |
| `00540bc4` | 마지막 0 store | **지역/분산.** Session `assetState.boss`, `gunState.boss`, 일부 raw 상태의 authority 등으로 갈린다. 서버 `00540bc0`와도 다른 원문 전역이다. |
| `00542534`, `00542538`, `0054253c`, `00542540` | 각 `ffffffff` store | **없음.** 실제 State의 서로 다른 네 DWORD owner가 없다. 저장 복원의 `00542528/2c`, command 큐의 상태번호, Player 배열 첫 네 칸과 합치지 않는다. 각 필드를 명시적으로 소유하고 실제 읽는 자식과 연결해야 한다. |

### 22DWORD 연속 표

`005c84e8..005c853c`의 **88BYTE,22DWORD**는 현재 실제 제품의 장기 owner가 없다. 원문 `004b6180`이 sender/연결 번호별로 `(&DAT_005c84e8)[param_1]`에 현재 frame count를 쓰고 State/연결 정리가 같은 항목을 다시 읽거나 지운다. 인접 `005c8548` 표와는 별개다.

실제 clear 대상은 다음22주소다.

```text
005c84e8 005c84ec 005c84f0 005c84f4 005c84f8 005c84fc
005c8500 005c8504 005c8508 005c850c 005c8510 005c8514
005c8518 005c851c 005c8520 005c8524 005c8528 005c852c
005c8530 005c8534 005c8538 005c853c
```

추가 시 Client 수명의 명시22DWORD 배열을 실제 연결/State 처리기들이 함께 빌려야 한다. cleanup만을 위한0배열이나 주소 사전을 만들고 다음 Session에서 다시0배열로 교체해서는 안 된다. 22번째 store 뒤의 인접 필드를 덮지 않는 것도 검사한다.

## write16/write64Bits와 TYPE float

| Native 주소/폭 | 현재 owner와 연결 판단 |
|---|---|
| `005c89b4` low16 | **장기:** `ClientRandomGlobals::FortCoreSeed()`의 전체 DWORD다. `(old & ffff0000)`만 남기는 쓰기로 연결한다. 전체 DWORD0/seed 재설정으로 대신하지 않는다. |
| `005950b8` double8 | **없음.** 원문 간헐천/세계 프레임의 별도 다음 실행 기한이다. Scene/Frame/ClientLocalState 활동 기한과 구별한다. |
| `005950c8` double8 | **없음.** 위 기한과 별개의 원문 게임 프레임 기한이다. 동일0 초기값이라는 이유로 합치지 않는다. |
| `005950c0` double8 | **없음.** 현재 실제 Client 장기 필드에 연결되지 않았다. |
| `00565e68` double8 | **장기:** `ClientAudio::scene_.songEnd`, `SceneMusicState::songEnd`다. 현재 `Scene()`은 const 조회만 제공한다. 같은 장면 음악 저장소에 원시 bits를 쓰는 좁은 API가 필요하며 새 `RequestMusic/StartScene` 호출로0쓰기 효과를 대신하지 않는다. |
| `005c8618` double8 | **지역:** Login `captureEnd`, Zone/ClientPacket/Timer의 현재 wall 기한이다. Client 수명으로 이전하고 같은 consumer 참조를 유지해야 한다. |
| `00595358` double8 | **없음.** 원문 게임 시각과 비교하는 별도 예약이다. `CoreDataGlobals::Active`나 Core 캐시와 주소가 가깝다는 이유로 합치지 않는다. |
| `005c8610` double8 | **없음.** 마지막 `WallSeconds()+10.0` 저장 기한이다. 이미 있는 `ClientLocalStateGlobals::ActivityEnd()`는 **00544d58**로 다른 기한이다. 하나를 빌려 같은 값으로 갱신하면 원문이 다른 위치에서 읽는 두 기한을 합친다. |
| `005c85f0` double8 | **지역/분산:** Session `loginEnd`를 Login timer/Dais/Flare가 일부 공유하고 `GunImpactState::suppressedTime`은 값형 필드를 갖는다. Client 수명의 같은 실제 double에 모두 연결해야 한다. 게임 process clock.now로 대체하지 않는다. |
| `005c85f8` double8 | **지역:** Login `connectEnd`, RawLocalMissionState의 연결 wall 기한이다. Editor State도 빌릴 Client 수명 double이 필요하다. |
| `TYPE[read(00541240)]+c4` float4 | **장기 실제 물리:** `Client::SurfaceStorage()->types.at(type).cost`. 원문 입력은 `read32(00540bdc)`를 **signed32 정수로 해석→float32 저장**이다. 그 DWORD를 float 비트로 재해석하거나 double 슬롯에 저장하지 않는다. Session이 없을 때도 이 같은 TYPE를 고쳐야 하며 `SessionAssets::types`/GameAssets의 불변 사본에만 쓰면 다음 raw World가 다른 값을 읽는다. |

`write64Bits`의0은 +0.0의 원시8BYTE다. 정상 값만 가정해 초 단위 정수로 변환하거나 주변 DWORD 두 개를 부수적으로 초기화하지 않는다. TYPE 입력 번호/기본 정수의 장기 owner가 없다는 사실은 cost 물리 자체가 이미 존재한다는 사실과 구별한다.

## 정지·시계·Renderer의 같은 소유자

`Client::clock_`는 이미 게임 정지 중첩 깊이와 원점을 갖는다. `PushGamePause/PopGamePause/Paused`는 같은 객체에 연결돼 있지만 정확한 깊이의 공개 조회와 `GameClock::Reset()`을 호출하는 Client용 API는 현재 없다. cleanup을 위해 별도 `pauseCount`를 만들 필요가 없다. 실제 기존 depth를 좁게 조회해야 한다.

`GameClock::Reset()`은 `00460e70`의 **누적 정지 시간 double 하나만 +0**으로 쓴다. `clock_=GameClock(timeGetTime())`, 원점 재설정, 현재 Frame 재캡처, 모든 `FrameTime`0쓰기는 다른 효과다. 원문 반복은 현재 pauseDepth가 비영인 동안 게임 Resume→Audio Resume 뒤 다시 읽는다. `Client::uiPaused_`, `MissionFrameGateState::scriptPaused`, `movieUpdatePauseDepth_`는 깊이의 복제본이 아니라 서로 다른 소유/영상 관문이다. 정리 뒤 해당 보유자가 다시 Pop하도록 남기지 않는 반납 설계가 필요하다.

Renderer 객체와 세계 redraw는 별개 수명이다. `Client::renderer_`의 교체/반납은 `ClientKencloudGlobals::BindRenderer`로 opaque32 존재 토큰과 연결돼 있다. 직접 cleanup store `0059a8b0=1`은 이미 있는 `WorldRedraw()`에 쓰고, 실제 Renderer 자식 효과는 별도 순서에서 호출한다. 단순 `InvalidateAll`로 전역 store 자체를 대신하거나, redraw가1이라는 이유로 Native Renderer 전체 복원이 끝났다고 판단하지 않는다.

Audio는 Client가 소유하지만 `SceneMusicState`의 battle/playersReady/localPlayer/paletteDirty가 아직 다른 게임 owner들의 참조형은 아니다. `00565e68`의 실제 songEnd는 그 Audio 내부 저장소에 직접 써야 한다. 별도 cleanup 음악 state를 만들어 나중에 복사하지 않는다.

## 현재 Session 소멸은 Native cleanup 전체의 대체가 아니다

현재 `UberGump::ClearWorld()`는 HTML/Help/팝업·미니맵·생산 Form·NSInfo/Chat·메뉴 참조와 Renderer callback을 먼저 반납하고, 미션이 가진 정지 깊이와 Runtime/Input을 정리한 뒤 `world_`를 이동해 소멸한다. `GameWorld::~GameWorld()`는 factoryDisplay를 먼저 반납하고 Campaign Session을 해제한다.

현재 Session 소멸은 다시 다음 효과를 수행한다.

1. 428 Editor의 실제 등록13종 receive를 Session 반납 전에 해제한다. observer/version/mismatch/length 표를 지우는 전송 초기화와 다르다.
2. `Impl::~Impl()`이 Player1..8의 `RemoveBrain`을 호출한다.
3. `ReleasePlayerLinks()`를 호출한다.
4. `world.DrainProcesses()`를 호출한다.
5. 같은 장기 Pool의 PostPop/삭제 callback에서 해제될 지역 모듈 포인터를 떼어낸다.
6. 실제 모듈과 Player/표면/전역 대여가 반납된다.

이는 C++ 최종 소멸의 안전 정리이며 `CleanupStateWorld`의 원문 순서를 모두 수행했다는 증거가 아니다. Native 부모가 이미 `resetAi`, `ClearPlayerLinks`, `ClearKernel`을 수행한 뒤 기존 소멸자를 그대로 실행하면 같은 효과가 추가로 호출된다. 현재 우연히 빈 목록/빈 Kernel에서 끝난다는 이유로 호출 순서가 같다고 판단하지 않는다.

또한 Native의 `InitializePlayers`는 **Core0 선택 뒤, Pool reset 뒤**에 온다. Session이 살아 있으면 `ClientPlayerRecords::Use`가 기존 Player span을 빌리고 있어 `InitializeClientPlayerRecords`가 정상적으로 거부한다. 반대로 함수 진입에서 `ClearWorld()`를 호출하여 Session을 먼저 없애면 초반 저장/SP/사제/AI의 실제 자식과 지역 전역을 이미 잃는다.

따라서 다음 구현은 두 가지 작업을 구별해야 한다.

- 실제 Native 효과는 기존 원문 위치에서 한 번 실행하고, 완료한 효과를 C++ destructor에서 다시 실행하지 않도록 **명시 반납 수명**을 마련한다. 단순한 `cleanupDone=true` 성공 setter가 아니라 실제 해당 하위 정상 복귀와 모듈 참조 반납을 묶는다.
- Session/World가 없어지는 동안에도 이 문서의 실제 전역/TYPE/State/Player owner는 Client 수명에 남는다. 특히 지역 `RawGraph`, 장부, 서버/boss, sourceType/기한, fullDeck/갱신 카운터를 Session 소멸 전에 새0사본으로 갈아 끼우지 않는다. 현재 `RawSurfaceStorage`는 Pool/Hash/spot/TYPE만 장기화했고 Graph와 `SquidPostPopState` 장부는 여전히 `RawSurfaceWorld::Impl` 지역이다.

## 준비 대여와 다음 연결의 최소 조건

`ClientGameplayGlobals::AcquirePrepared()`는 준비된 owner/모드/커서/에너지를 대여 시작과 종료에서 재초기화하지 않는다. 일반 `Acquire()`는 player/finder/world/energy와 cursor.lockDepth1을 재설정하며, 일반 Use 소멸도 known/editor/커서를 초기화한다. Native 정리/prefix 뒤 새 Editor Session에는 prepared 경로를 사용해야 한다.

`RawSurfaceStorage::Acquire()`는 두 번째 대여부터 Pool.Reset(0)/Hash.Reset/spot0을 실행한다. `AcquirePrepared()`는 현재 물리와 모드/TYPE를 보존한다. State가 이미 같은 순서로 정리한 뒤 일반 Acquire를 쓰면 추가 reset이 생긴다. 현재 prepared `RawSurfaceWorld`도 Graph와 장부는 지역 새 모듈이므로 이 부분까지 Native Graph/장부 수명이 복원된 것은 아니다.

주소 접근 adapter는 다음 조건을 만족할 때 작성할 수 있다.

1. 위 ‘장기’ 항목은 기존 실제 owner의 현재 getter/직접 store로만 분배한다. 접근할 수 없는 private 필드는 그 실제 owner에 필요한 폭의 좁은 API를 추가한다.
2. ‘지역/분산’ 항목은 한 장기 저장소를 정하고 현재 모든 실제 소비자에게 참조/현재 getter를 전달한 뒤 연결한다. Session 전후 값을 복사하는 사후 동기화를 원문 alias라고 부르지 않는다.
3. ‘없음’ 항목은 원문 실제 사용을 읽어 명시 필드/연속 배열을 구성하고 그 소비자와 함께 연결한다. 아직 그 작업이 안 된 주소는 도달 시 명시 거부한다. 원문 분기상 자연 미선택인 경우와 필요한 read/write에0을 공급한 경우를 구별한다.
4. Native DWORD 모드를 bool host 필드에 재해석 참조하지 않는다. 현재 bool 모델은 정상0/1 읽기/0쓰기까지 가능한 범위를 분명히 하고, 임의 비영 DWORD 보존이 필요하면 실제 소유자 표현과 소비자 참조를 함께 바꾼다.
5. 첫 메뉴→Edit, 기존 Campaign→Edit, Editor→메뉴/재진입에서 같은 owner 주소와 전체값을 각각 관측한다. Player 재할당 전 Core0/대여 해제, 상태5블록 보존, low16 쓰기 상위WORD 보존,22DWORD 경계, unsigned revision 감김, 실제 서로 다른 wall 기한을 별도로 검증한다.

이 문서는 위 연결을 제안하고 현재 공백을 식별한 결과다. 전체 cleanup/prefix 또는 실제 Edit 사용자 편집의 완료 보고서는 아니다. CD10.72의 대응 부모는 기존 분석의 `0042f5c0`으로 기록하되 이번 매핑과 직접 전역 주소는10.78 기준이다.

## 직접 확인한 현재 소스

- [RawStateWorldCleanup.cpp](../../cpppj/src/o/RawStateWorldCleanup.cpp): 실제 Native 접근 순서/폭과 분기.
- [ClientLocalStateGlobals.h](../../cpppj/src/client/ClientLocalStateGlobals.h), [ClientGameplayGlobals.h](../../cpppj/src/client/ClientGameplayGlobals.h), [ClientCoreDataGlobals.h](../../cpppj/src/client/ClientCoreDataGlobals.h), [ClientPlayerRecords.cpp](../../cpppj/src/client/ClientPlayerRecords.cpp): 현재 참조·Core/Player 재할당 관문.
- [ClientMain.h](../../cpppj/src/client/ClientMain.h), [GameClock.cpp](../../cpppj/src/o/GameClock.cpp), [ClientAudio.h](../../cpppj/src/client/ClientAudio.h): 실제 Client/시계/음향 소유자와 공개 접근 범위.
- [RawCampaignSession.cpp](../../cpppj/src/client/RawCampaignSession.cpp), [RawCampaignSessionLogin.inc](../../cpppj/src/client/RawCampaignSessionLogin.inc): 지역/분산 전역, 준비 대여, 생성/반납 효과.
- [RawSurfaceStorage.cpp](../../cpppj/src/client/RawSurfaceStorage.cpp), [RawSurfaceWorld.cpp](../../cpppj/src/client/RawSurfaceWorld.cpp): 장기 배열과 지역 Graph/장부의 수명 차이.
- [UberGump.cpp](../../cpppj/src/client/UberGump.cpp), [UberGump.h](../../cpppj/src/client/UberGump.h), [UberHud.inc](../../cpppj/src/client/UberHud.inc), [RawMenuControls.h](../../cpppj/src/client/RawMenuControls.h): 현재 Template/UI/Help/HUD 소유자와 `ClearWorld()` 반납 순서.

소스 행 번호는 진행 중인428 변경으로 이동할 수 있으므로 함수/멤버명과 Native 주소로 고정했다. 현재 `RawStateWorldCleanup.cpp`의 직접 주소가 문서에서 누락되지 않았는지와 문서 UTF-8/공백 오류를 정적으로 확인했다. 실행 결과를 추가하지 않았다.
