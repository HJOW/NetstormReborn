# RawCampaignSession 공개 경계·과거 메모 감사와 partial 닫기 — WB04-E1328

2026-11-24, HJOW-Athlon. `PORT_MAP.tsv` 에 마지막 `partial` 1행으로 남아 있던 `cpppj/src/client/RawCampaignSession.cpp`(7,130줄, base `baf141eb`)의 C++ 공개 경계(`RawCampaignSession.h` 공개 메서드 294개, `RawCampaignSessionServices`, `CampaignLoginServices`)와 대응표 과거 메모 191건의 잔여 문장을 전수 대조해 5건의 실제 앱 서비스 연결 누락을 찾아 수정하고, 남은 항목의 해소 단위·웹 제외 사유를 확정해 행을 `port` 로 승격했다. 이로써 기준선 `225553df` 이식 대상 1,007단위 전체가 `port` 이상 1,007/1,007(100%, `port` 275·`session` 732, `partial` 0, `todo` 0)이 됐다.

## 1. 공개 경계 감사에서 발견·수정한 앱 연결 누락 5건

C++ `CampaignClientServices.cpp`·`UberGump.cpp` 가 채우는 `RawCampaignSessionServices` 와 웹 `www/netstorm.js` 의 `_campaignSharedServices()`·`_campaignGameplayServices()`·`_campaignLoginServices()` 를 1:1 대조한 결과, 앱에 장치가 이미 존재하거나 웹 `raw-campaign-session-*.js` 가 호출하는 서비스 중 5건이 미연결 상태였다.

| 서비스 칸 | C++ 근거 | 웹 누락 증상 | 수정 (`webpj/www/netstorm.js`) |
| --- | --- | --- | --- |
| `kencloudGlobals` | `CampaignClientServices.cpp:210` | `_campaignSharedServices()` 누락으로 `raw-campaign-session-owners.js:353` 이 지역 `{ value: 0 }` 를 생성. 섬 소유권 변경(`islands`)·지역 도장(`paint`)·소유권 투표(`votes`)·스커미시 분할·즉시 카메라(`centerImmediately`·`cameraCommit`)가 `worldRedraw` 를 올려도 렌더러의 실제 `0x0059a8b0`(`_clientKencloud.worldRedraw()`)에 전달되지 않음 | `_campaignSharedServices()` 에 `kencloudGlobals: this._clientKencloud` 전달 및 필수 수명 검사에 포함 |
| `htmlWindowGlobals` | `CampaignClientServices.cpp:206` | `_campaignSharedServices()` 누락으로 `raw-campaign-session-login-front.js:124` 가 `_clientHtml.text.background[0]` 대신 `#localTitle` 지역 변수 사용 | `_campaignSharedServices()` 에 `htmlWindowGlobals: this._clientHtml` 전달 및 필수 수명 검사에 포함 |
| `factoryHelp` | `UberGump.cpp:1075` | `raw-campaign-session-factories.js:35` → `raw-factory-research.js:111` 의 연구 명령 `0x03000080`(`kind=0`)이 `researchHelp` 도움말을 열려다 `Factory 실제 연구 도움말 미연결` 예외 발생 (`help.english`·`help.korean:500` 에 `<a name="researchHelp">` 실존) | `_campaignGameplayServices()` 에 `(section) => { this._closeCampaignPopups(); this._openHelp(section); }` 연결 |
| `removeConnectionStatus` | `CampaignClientServices.cpp:222` | `raw-campaign-session-state.js:176`(`connectionActivity.eraseStatus`)가 비미션 또는 kind 11·16 경로에서 `실제 연결 상태 WarningMessages 삭제 미연결` 예외 발생 | `_campaignGameplayServices()` 에 `(kind, player) => { this._campaignProduction?.ui?.warnings()?.remove(kind \| 0, player \| 0, 0); }` 연결 |
| `templateStateBytes` | `CampaignClientServices.cpp:225` | `raw-campaign-session-players.js:42` 가 빈 Template State 초기화 시 `빈 State의 현재 미정 바이트 입력 미연결` 예외 발생 가능 | `_campaignGameplayServices()` 에 `currentAllocationBytes` 연결 |

참고로 `services.archer` 의 `firingSound`·`impactSound`·`ricochetPointBytes` 는 `raw-campaign-session-combat.js:52-78` 에서 `services.sound.playOnceAt` 와 `zeroPointBytes`(E1226)로 이미 자체 결선되어 있고, `services.deletion.selected` 는 `campaign-game-world.js:53` 에서, `services.ai.skirmishPlayers` 는 `campaign-client-services.js:45` 에서 이미 연결되어 있음을 확인했다.

## 2. C++ 공개 메서드 294개 및 `CampaignLoginServices` 전수 대조

- **JS `RawCampaignSession` 직접 대응(164개+):** 생성·적재·로그인·송수신·프레임·카메라·입자·튜토리얼 1~6·생산·메뉴·AI·건설·운반·사제·스커미시·반납 등 단독 미션·스커미시 활성 공개 경계 전부. (`PrepareGameMenus`·`GameMenuTrackingOffset`·`BindGameMenus` 는 E1327 및 `netstorm.js:_campaignGameMenuDevices` 로 연결, `MakeObjectMenus`·`MakeObjectMenuOpen` 은 `makePriestMenu` 등 파생별 팩토리로 대응.)
- **웹 제외(미매칭 130개 분류):**
  - **Edit 전용 (`exclude-edit`):** `LoadExistingEditor*`·`InstallExistingEditor*`·`WithExistingEditor*`·`CommitExistingEditorInstall`·`WriteExistingEditorCoreData`·`BuildEditorIsland*`·`SeedEditorIslandChoices`·`DropEditorIsland`·`CanPlaceEditorIsland`·`EditorIslandManagerAvailable`·`SaveCurrentMap`·`BindFortFileSave`·`WriteFortObjectChunk`·`CycleBridgeQuality`·`SetBridgeQuality`·`SetAllBridgeQuality`·`OwnedIslandCount`·`IslandCount`·`MutableFortState`·`editorPlayerUi`·`editorIslandGreen`.
  - **MP·LAN·Meta·Host 전용 (`exclude-mp` / `exclude-host` / 단독 미도달):** `BindConnectionTransport`·`SharedConnectionTransport`·`FinalizeBattleLeave`·`RequestForcedAlliance`·`PollReconnectSave`·`TakeServerForState`·`PrepareFortDescend`·`RequestFortDescend`·`BuildMetaParticipantImage`·`MetaMenuType`·`RitualChoiceInputs`·`AppendTechnologyMenu`·`AppendThemeTechnologyMenu`(E1180 비미션 전용)·`challengeGlobals`·`nsInfoGlobals`·`remoteFortPath`·`remoteFortFiles`·`reconnectFile`·`fortTechnologyUi`·`localLogin.abortBattle`·`localLogin.initializeChat`(`raw-player-core-data.js:99` `missionPost !== 0` 즉시 반환)·`localLogin.requestPhase`(승리/패배 곡 단독 미도달 E1238)·`localLogin.creatingIslands`(`local-mission-transport.js:189` `makeGuideMissionZone` 의 Zone 플래그 `0x82000004` 에 `0x100` 비트가 없어 단독·스커미시 모두 미도달 확정).
  - **C++ Native 훅 어댑터·내부 래퍼:** `ReceiveFixed{Construction,Login,Gameplay,Carrier}Packet`, `HandleDamageableResourceRegular`, `StartResourceConstruction`, `DestroyResourceObject`, `NotifyGeyserAi`, `CheckCarrierPostPop`, `NotifyAiObject`, `RecheckCurrentSelection`, `EmitDamageableCollapse`, `EmitDamageableExplosion`, `FinishDaisSacrificeForDeletion`, `HandleCarrierResourceRegular`, `SpawnReleasedPriest`, `DropDamageableDude`, `DropDamageableIsland`, `BorrowAiWalletMemory`, `GroundImpact`, `constructionGlobals`, `adoptInitializedPlayers` — 웹은 C++ Native Surface 역호출이 아니라 `raw-campaign-session-*.js` 내부에서 직접 호출한다(E1193~E1221·E1273~E1274·E1321).
  - **개발자·시험 전용:** `31h` `developer*`·`buildReplay`(E1242), `AiBrainBytes`(E1241), `DispatchAiDebugCommand`, `ReportCarrierHover`, `DecoderDiagnostics`, `AddRandomObjects`·`CreateAt`·`DestroyAt`·`GetFullDeck`·`GameMenuReload*`·`ReloadTheamAt`(E1190 출하 영어 대본 82개 전수 감사에서 단독 미션 미사용 확인).

## 3. `PORT_MAP.tsv` 과거 메모 191건 잔여 문장 대조

- **E574~E697(생성자 구간·모듈·Load 잔여):** E697 typed Load 및 E698~E702(Fort/간헐천/너겟)로 해소.
- **E700~E893(내부 로그인·Carrier·건설·연구·카메라·튜토리얼·AI·State·덤프 잔여):** E750~E893 에서 공개·등록 완료, 비미션 너겟(E701)은 스커미시 미션 모드 판정(E1270)으로 미도달 확정.
- **앱 수명·HUD·Renderer·음향·메뉴·캡처·L2·L3 잔여(E584·E750~E893·E1316·E1326):** E946~E1283(앱 조립·4-D 닫기·L2), E1298(Kencloud), E1304~E1309(L3), E1312~E1316(건물·파편·골렘·받침·섬 투하 캡처/Session), E1318~E1319(주문·포획), E1322~E1327(16:10·한국어 M2·예산 우클릭/재부착·송금 MessageBoard·ESC 메뉴 SP 추적)과 이번 E1328 앱 서비스 5건 연결로 모두 해소.
- **남는 미연결 경계:** 위 2절의 Edit·MP/LAN/Meta·개발자 전용 분기는 웹 제외 사유로 명시 오류를 유지한다.

## 4. 검증 및 실패·교정

- **새 Chromium e2e(`e2e/session-services-wiring.spec.js`):** Tutorial3 실제 미션 실행에서 ① `kencloudGlobals`·`htmlWindowGlobals` 동일 소유자 공유와 `centerCameraOn`/`commitRequestedCamera` 의 `worldRedraw`(`0x0059a8b0`) 증가 및 후속 렌더 프레임의 0 소비, ② `removeConnectionStatus(16, 0)` 의 `warning_count 1 → 0` 제거, ③ `templateStateBytes` 28바이트 호출, ④ 실제 아군 워크샾(VT `0x509148`)의 `commandFactory(sid, 0x03000080, sid)`(`kind=0`) → `researchHelp` 도움말 창 열기 및 닫기 후 10프레임 정상 진행(콘솔/페이지 오류 0)을 확인했다.
  - **초기 실패와 교정:** 첫 실행에서 Playwright를 루트에서 실행해 `baseURL` 미적용 실패 → `webpj` 디렉토리 실행으로 교정; 두 번째 실행에서 `SidPool.isAllocated` 미존재(`slot(sid)[11] & 1` 검사로 교정); 세 번째 실행에서 `centerCameraOn` 은 `app._commitCampaignCamera` 가 아니라 `centerImmediately` 의 `worldRedraw` 상자를 직접 올리므로 `redrawBox.value` 증가와 `commitRequestedCamera` 상자 동일성을 분리 검증해 **1/1 통과(26.1초)**.
- **기존 L2/L3·상태 회귀 Chromium e2e(6/6 통과, 3.3분):** `e2e/fixed-time.spec.js`(4:3·16:9·16:10 오라클 0픽셀·덤프 일치·비고정 거부), `e2e/menu-sp.spec.js`(ESC SP 103→106→106→109·화면 0픽셀), `e2e/mission-status.spec.js`(6ch 송금 수신·MessageBoard 억제).
- **집중 Node 시험:** `campaign-request-frame`, `raw-campaign-session-{load,state,priests,factories}`, `bundle-fresh`, `port-map` **179/179 통과**.
- **번들·대응표·manifest 검증:** `npm --prefix webpj run build` 통과(`3,429,669바이트 / 3.27MiB`, 한도 9.9MB), `node webpj/tools/port-map.mjs --check`, `tests/port-map.test.mjs`, `tests/fixture-manifest.test.mjs`, `tests/bundle-fresh.test.mjs` 통과.
- **회귀 주기:** 이번 단위는 집중 검사로 마감한다. Node v26 전체 회귀는 E1325(6,847/6,847) 뒤 3/4, Node v22 전체 회귀는 E1324(6,845/6,845) 뒤 4/8이다.
