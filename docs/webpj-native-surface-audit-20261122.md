# webpj: Native 장기 결선(RawSurface*·ClientConstruction*)과 웹 typed 조립의 훅 대조 (WB04-E1206~E1215, 2026-11-22)

cpppj 의 `client/RawSurface*.cpp`·`ClientConstruction*.cpp` 는 "Native 장기 결선"으로 각 원문 모듈에 훅을 조립한다. 웹은 같은 훅을 `webpj/www/core/raw-campaign-session-*.js`(C++ `RawCampaignSession.cpp` 의 typed 경로)에서 조립한다. 행을 닫을 때마다 훅 이름별로 웹의 소유 지점을 찾아 대조했고, 차이는 아래 유형으로 나눴다.

| 유형 | 뜻 | 결과 |
| --- | --- | --- |
| A | 원문 Native 는 `AssertDisplay` 로 진단을 표시하고 계속하는데 웹 훅은 오류로 중단 | 아래 "고친 것" |
| B | 원문에는 있는데 웹에 훅/분기가 없음 | 거주 구역·Player·스프라이트·주문 효과의 가상 삭제(고침), Wind 홈 통지(도달 불가) |
| C | 상수·기본값·호출 순서 차이 | 모두 기본값이 같거나 쓰이지 않음(무해) |
| D | `native=true`·`EnableNativeBody()`·`nativeArithmetic` 가 끄는 typed 선검사 | 정상 입력에서는 같고 비정상(비유한·범위 밖·이미 free) 입력에서만 갈림(무해) |

## 고친 것 (실제 동작이 달랐던 곳)

| 단위 | 곳 | 내용 |
| --- | --- | --- |
| E1207 | `raw-campaign-session-gems.js` | Gem(0xf6·0x104·0x10a)·Deck(`isOpen()`·목록 추가 여섯) 진단을 표시 후 계속 |
| E1207 | `raw-campaign-session-damageable.js` | 신전 삭제(Vortex.cpp 0x16f·0x2ab·0x2ac) 진단을 표시 후 계속 |
| E1211 | players·carrier-regular·flyers·spells·damageable·`raw-dude-task.js` | Player 정기(0x41d)·내용물 타이머 부모(Baseprocess 0xa4)·더스트 데빌(0xa4·0x19e)·애시드 송신(0x2fa..0x2fd)·Dude(0xde·0x127·0x9c)·낙하 task 교체 진단을 표시 후 계속 |
| E1215 | `raw-campaign-session-deletion.js` | 실제 앱 탐침에서 `'다른 실제 자산 가상 삭제 미연결'` 17종 → 거주 구역(0x50f540: 공통 Pre·목록 제거+공통 Post)·Player(0x50ee08: 이전 포인터/상태 정리 전처리)·주문 폭발 효과 열 개·0x50c9f0·0x50cc08·0x50db78(공통 Pre/Post)을 원문 분배표대로 연결. 남은 것은 멀티플레이 전용 Outpost 하나 |
| E1215 | `raw-campaign-session-runtime.js` | 의존 통지(`dependencyNotice`)를 감시자 VT 별 원문 분류로 교체: Factory 업그레이드 그림(건물과 같은 TYPE) 같은 자산 감시자는 flags 0 으로 삭제, 비행장 홈은 홈 통지, 나머지 자산·Depend/Gump/Content Form 은 빈 반환, Process Form 만 PID 통지 |
| E1215 | `raw-campaign-session-state.js` | 보호막 Regular 상태의 `checking` 연결(점검 모드의 Dais.cpp 0x479 진단) |
| E1231 | `www/netstorm.js` | Session 이 큐에 올리는 통지 `0x020000b4`·`0x030000b2`·`0x02000039` 를 `TUTORIAL_NOTICE_COMMANDS` 에 더해 큐 머리에서 바로 소비(이전에는 허용 목록 밖이라 3초 동안 모든 입력을 막고 "미구현 입력 사건을 버립니다"로 버려졌다 — 실측). Tell 단추 `MissionAbort` 는 미션 중이면 명령 큐(0x0200008c, 인자 그대로)로 보내 원본 `dispatchMissionAbort` 가 인자 0 은 ABORT 확인 창, 그 밖은 LeaveBattle 예약을 하게 함 |
| E1230 | `www/core/raw-campaign-session.js`·`www/core/campaign-game-world.js`·`www/netstorm.js` | 다리 붕괴 검사(원본 `ClientMain` 의 `RunClientBridgeDecay`)를 앱의 Client 단일 Kernel 프레임(`prepareKernelFrame`)에 연결 — 검사는 `Session.runFrame` 에서만 불렸고 앱은 쓰지 않아 다리가 금 가거나 무너지지 않았다(스캔 기한이 10초 주기로 진행함을 실제 앱에서 확인). 미션을 떠날 때 건물 숨김(0059a8ac)을 0 으로 되돌림(원본 State 정리) |
| E1229 | `www/ui/help-window.js`·`www/netstorm.js` | 도움말 링크 이동·Back 이 정보 패널·그림을 지우던 것을 원본 `QuickHelpRequest::Open(0, 주제)` 의 TYPE 이름 조회로 대신함: 끝의 `type`·`help` 를 떼고 `%s`·`sun%s`·`wind%s` 이름을 TYPE 표에서 대소문자 없이 찾아 번호 1 이상이면 정보 패널·그림을 붙인다(`HelpWindow.resolveType` 훅과 앱의 `_resolveHelpType`; F1 생산 정보와 같은 `_helpTypeDisplay` 를 쓴다). 월드가 없는 메인 메뉴 Help 는 `ReadAssetQuickHelpType` 미이식이라 정보 없음 |
| E1228 | `www/netstorm.js` | 스크립트 명령 `Warning`(Tell 본문 `<$Warning,"…">`)을 HUD 경고판(`warnings().add`)으로 연결 — 이전에는 콘솔에만 남았다. 원본 UberGump 의 `kScriptWarning` 과 같고, 경고판은 단독 미션(`missionPost`)에서 원본과 같이 억제한다 |
| E1227 | `www/netstorm.js` | 안내 창 단추 `ShowTechnology`(Review Knowledge)·`GetTechnology`·`ShowTecSideMenu`·`ShowTecMenu2` 를 원본 이름표(`knowledgeScriptCode`)로 명령 큐에 올리고(미션 월드 없으면 소비만), 입력 루프에 GetTechnology(0x0200009f→`Session.learnNewTechnology`)를 추가. 기술 획득(`snapshotBattle`)이 요구하는 `services.fortMission`(Guide 저장 토큰·이름·순번·플래그)·`services.fortBadges`(Client 단일 28x220 장부)를 앱에 결선 — 이전에는 전투 모드(미션)에서 기술을 얻는 순간 "현재 Mission 읽기 장치 미연결" 오류로 프레임이 멈췄다 |
| E1226 | `raw-campaign-session-combat.js` | 아이스 반사의 변위 0 지역점 8바이트 입력(`initialPointBytes`)을 앱 서비스가 없으면 0점(valid 판정 실패 → 반사 없음)으로 공급 — 앱이 `ricochetPointBytes` 를 공급하지 않아 변위 0 반사에서 "현재 지역점 8BYTE 입력 미연결" 오류로 프레임 루프가 멈추던 것을 막음(원본은 미초기화 스택값이라 비결정, cpppj 는 프로세스 메모리 조회) |
| E1225 | `www/netstorm.js`·`raw-campaign-session.js` | 객체 메뉴 명령 네 가지(사제 기도 0x030000de·운반체 Cast 0x0300011a·Drop 0x03000076·Factory 순환 0x03000063)를 입력 루프 허용 목록과 분배에 추가하고 Session 에 `commandPriest`·`commandCarrierDrop` 를 더함 — 이전에는 큐 머리에서 소비되지 않아 3초 뒤 버려졌다(원문 `CampaignObjectMenuClient::Command` 188~192행) |
| E1224 | `www/ui/main-menu.js` | 메인 메뉴 버튼 와이드 보정(높이 800 초과에서 baseline=(H-480)/2+112, 원문 cpppj `LayoutMainMenuControls` 와 같음) — 1600x900·1920x1080 에서 버튼이 로고 위로 올라오던 것을 고침 |
| E1223 | `raw-campaign-session.js` | `showSacrificeMenu` 추가(원문 ShowSacrificeMenu: 미션에서는 즉시 반환, 비미션에서만 `SacrificeMenu(대상,레벨,업그레이드)` Tell) — 이전에는 단독 미션에서 사제를 제단에 보내면 '희생의 실제 메뉴 부모 미연결' 오류로 중단됐다 |
| E1219 | `raw-region-ownership.js`·`raw-campaign-ownership.js`·`raw-campaign-asset-bindings.js` | 섬 번호 조회(PE 0046f530)가 좌표에 0041d7a0 편향 snap(0.99999 를 더해 0 쪽 절삭)을 먼저 적용함을 역어셈블로 확인하고 웹 두 getter 에 `snapRegionCoordinate` 적용(소수 좌표의 칸이 달랐다). Reward refreshSp 는 HUD 예산 글자가 매 프레임 SP 를 읽어 무해로 확인 |
| E1218 | `raw-campaign-session-arrivals.js` | 마나 볼트 주기의 frameZero 훅을 Rift SHP 접근(frameCheck 진단 후 계속)에 연결 |
| E1217 | `depend-form-host.js`·`raw-surface-world.js` | DependFormHost 의 Native 진단 모드(`bindNativeFind`, World 가 결선): remove 인자 이상 다섯(Squid.cpp 0xb48·0xb49·0xb4b·0xb4c·0xb4e)·has/remove 시작 진단(Squidfinder.cpp 0x158)·notify 뒤 watchedFid≠0(0xadf)을 표시 후 계속 |
| E1216 | `raw-campaign-session-contents.js` | unit-lost 소리/좌표 억제 두 전역(`unitLostSuppressed1/2`)을 현재 적재 깊이(≠0)와 fort 모드에 연결 — 이전에는 항상 false 라 적재 중 삭제되는 로컬 유닛도 손실 소리와 좌표를 남겼다 |

## 닫지 못한 차이 (PORT_MAP 해당 행에 기록)

- **DestroyPatch 진단 열두 지점**(Squid 삭제의 `AssertPatch` 0x24d·0x269·0x276·0x27d·0x280·0x27e·0x28f·0x2a2·0x2a9·0x15c·0x15d·0x123)과 `SquidDestroyLifecycle` 0x771: 원문은 표시 후 계속, 웹 `squid-destroy.js` 는 같은 지점에서 오류 중단. 식을 표로 옮겨 `assertDisplay` 로 바꾸는 단위가 필요하다(`RawSurfaceDeletion`·`RawSurfaceLifetime` 행).
- `rememberPlacement` 미연결(`RawSurfaceDeletion`). (적재 중 unit-lost 억제 두 전역은 E1216 에서 `raw-campaign-session-contents.js` 가 현재 적재 깊이·fort 모드에 연결했다.)
- **타깃(0x130)·활성화(0x206)·타이머 패킷(0xcc/0xe6) 진단**: cpppj typed 와 웹이 같은 오류 중단이며 원문 헤더는 복귀 가능한 진단이라 적는다(원문 식 문자열을 PE 에서 읽어야 한다).

## 방법 메모

- 실제 앱 탐침: 미션을 시작한 뒤 `window.netstorm._campaignWorld.session().world()` 로 TYPE 별 `create`→`destroy` 를 시도해 오류 문구를 모았다(임시 Playwright spec, 확인 뒤 삭제). 좌표 없이 만든 자산은 `Damageable 해방 좌표 범위 오류` 가 나오는 탐침 부작용이므로 제외한다.
- 읽기 전용 에이전트 네 개가 `RawSurface*` 스물세 파일의 훅 대응을 나눠 대조했고, 위 고친 것/닫지 못한 차이는 그 보고를 코드로 다시 확인한 뒤 반영했다(확인하지 못한 항목은 보고에서 '미확인'으로 표시).

## Client 상태·입력 계열 감사 (E1223)

읽기 전용 에이전트 두 개가 `ClientState*`·`Campaign*Services`·`UserInput`·`FortCurrentName*`·`RawPacketFrame`·`RawPeerWorldSync`·`RawClockSynchronization`·`ClientDiagnostics`·`SessionStateDump`·`ClientRendererFrameGlobals`·`*BuildReplay`·`UberReconnect` 를 대조했다(결과는 PORT_MAP 해당 행에 기록).

- **닫은 행**: CampaignUserInputServices(희생 메뉴 훅을 고친 뒤)·UserInput·FortCurrentNameDevices·RawFortCurrentName·RawPacketFrame·RawPeerWorldSync·ClientDiagnostics·SessionStateDump·ClientRendererFrameGlobals.
- **State 정리 다섯 행**(WorldCleanup·WorldGlobals·StorageCleanup·RuntimeRetirement·UserSelectionCleanup): 코드 이식은 동등하나 앱이 `cleanupClientStateWorld` 를 부르지 않는다. 웹은 미션 이탈을 `_leaveCampaignMission`→`_releaseCampaignWorld` 의 일반 수명으로 처리하고 초기화는 다음 대여(`storage.acquire`→`#freshGeneralInputs`, gameplay `#fresh`)에서 한다. 이 부모를 앱에 연결하려면 `interpretStateOptions`·`audio.writeSongEndBits`·`killGumps`/`replaceUber`/`leaveMetamap`/`refreshSp`/`retireRuntime` 등 앱에 없는 장치가 먼저 필요하다. 이탈 시 원문만 하는 효과(ownerMode 재설정, MapRevision+1, ChatChannels.resetOccupied, 이탈 시점 선택 해제 통지, 정지 깊이 반복 해제)의 영향 확인이 남았다.
- **UberReconnect**: 앱이 `pollClientConnectionHealth` 를 부르지 않아 하트비트가 실행되지 않는다(단독 미션 관측 가능성 미확인).
- **BuildReplay 두 행**: 개발자 전용 재생(읽기·키·Process)이 웹에 없다.

## UI 컨트롤 계열 감사 (E1224)

읽기 전용 에이전트가 `RawButtonControls`·`RawProductionControls`·`RawMinimapControls`·`RawTextEntryControls`·`TextEntryGump`·`InventoryItemGump`·`RawUberRootControls`·`RawGameMenus`·`RawMenuChat.inc`·`UberHud.inc` 를 대조했다.

- **닫은 행**: RawTextEntryControls·TextEntryGump·InventoryItemGump·RawGameMenus(앞 노트의 남음은 낡았음).
- **고친 것**: 메인 메뉴 와이드 버튼 기준선.
- **남은 차이(표시만)**: 우클릭 도움말(`SetHelpText`/`SetHelpWord`: 예산 "Storm Power Available"·생산 "Production Window"·미니맵 "Sky Overview"·채팅 "Chat Window")이 웹 전체에서 죽어 있다 — `alternateTextPointer` 를 설정하는 곳이 앱에 없고 `GumpHelp` 는 `{node}` 훅만 가진다(text/beginMenu/title/entry/attachMenu/sound 와 함께 연결해야 한다). 채팅 도우미 점멸, `ToggleMinimap`(0x02000037) 핸들러, 앱에서 도달하지 않는 모듈(storm-power-refresh·raw-menu-tree·hud-ns-info·background-gif-gump·text-clipboard), 배경 GIF(0xb0) 자식·지식 공장.
- **견고성 메모**: UI 진단 훅은 cpppj 도 throw 라 같지만, 웹은 프레임 예외 한 번에 렌더 루프가 멈춘다(`netstorm.js` `_startFrameLoop` 가 보고 후 rethrow, 재예약은 `_onFrame` 끝에서만). 설계 결정이 필요하다.
