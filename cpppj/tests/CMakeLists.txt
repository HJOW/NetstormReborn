# 현재 작업본·설치 checkpoint와 독립 부모의 최초 관측을 공용 회귀에 포함한다.
set_source_files_properties(MoviePlaybackTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_MOVIE_PLAYBACK_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/movie-playback-x86.tsv\"")
# 영상 창 없이 실제 CD 프레임/오디오를 독립 해시와 대조할 진단 실행 파일이다.
add_executable(netstorm_smacker_dump SmackerDecoderDump.cpp)
target_link_libraries(netstorm_smacker_dump PRIVATE netstorm bcrypt)
netstorm_apply_compiler_options(netstorm_smacker_dump)
set_source_files_properties(SmackerDecoderTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_SMACKER_DISC_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}/../../originalCD/MOVIE\";NETSTORM_SMACKER_TEST_ROOT=\"${CMAKE_CURRENT_BINARY_DIR}/smacker-data\"")
# NTFS readonly를 확인할 사용자 임시 경로와 원본/CD 보호 루트를 해당 검사에만 공급한다.
file(TO_CMAKE_PATH "$ENV{TEMP}/netstorm-editor-file-devices" _netstorm_editor_file_device_root)
get_filename_component(_netstorm_repository_root "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set_source_files_properties(EditorFortFileDevicesTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_EDITOR_FILE_DEVICE_TEST_ROOT=\"${_netstorm_editor_file_device_root}\";NETSTORM_REPOSITORY_ROOT=\"${_netstorm_repository_root}\"")
set_source_files_properties(FortCoreWriteTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_CORE_WRITE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-core-write-x86.tsv\";NETSTORM_FORT_CORE_WRITE_EXTRA_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-core-write-extra-x86.tsv\";NETSTORM_FORT_CORE_WRITE_CALLBACKS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-core-write-callbacks-x86.tsv\"")
set_source_files_properties(StartupStateCheckTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_STARTUP_STATE_CHECK_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/startup-state-check-x86.tsv\"")
set_source_files_properties(ChatManagerPrefixTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CHAT_MANAGER_PREFIX_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-manager-prefix-x86.tsv\"")
set_source_files_properties(PlayerCoreDataTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_CORE_DATA_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-core-data-x86.tsv\"")
set_source_files_properties(KencloudTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_KENCLOUD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/kencloud-v2-x86.tsv\"")
set_source_files_properties(ColorSortStartupTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_COLORSORT_STARTUP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/colorsort-startup-x86.tsv\"")
# Editor 로드/회복의 최초 독립 PE 관측을 해당 검사에만 공급한다.
set_source_files_properties(ScreenPaletteApplyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SCREEN_PALETTE_CLEAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/screen-palette-clear-x86.tsv\"")
set_source_files_properties(FortPaletteTransitionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_PALETTE_TRANSITION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-palette-transition-x86.tsv\"")
set_source_files_properties(FortSubscriberLoadTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_SUBSCRIBER_LOAD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-subscriber-load-x86.tsv\"")
set_source_files_properties(SubscriberIdGenerateTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SUBSCRIBER_ID_GENERATE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/subscriber-id-generate-x86.tsv\"")
set_source_files_properties(ExistingEditorInstallOuterTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_EXISTING_EDITOR_INSTALL_OUTER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/existing-editor-install-outer-x86.tsv\"")
set_source_files_properties(FortContentsLoadTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_CONTENTS_LOAD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-contents-load-x86.tsv\"")
set_source_files_properties(FortTypeNamesLoadTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_TYPE_NAMES_LOAD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-type-names-load-x86.tsv\"")
set_source_files_properties(FortTypeNamesAccessFixtureTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_TYPE_NAMES_LOAD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-type-names-load-x86.tsv\"")
set_source_files_properties(FortDeckLoadTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_DECK_LOAD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-deck-load-x86.tsv\"")
set_source_files_properties(DeckRestoreTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DECK_RESTORE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/deck-restore-x86.tsv\"")
set_source_files_properties(FortTerritoryRecoverTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_TERRITORY_RECOVER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-territory-recover-x86.tsv\"")
# 전체 저장과 Editor 기반의 독립 PE 관측을 각 검사에만 공급한다.
set_source_files_properties(FortSnapshotParentTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_SNAPSHOT_PARENT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-snapshot-parent-x86.tsv\"")
set_source_files_properties(FortMoneySaveTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_MONEY_SAVE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-money-save-x86.tsv\"")
set_source_files_properties(FortMissionSaveTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_MISSION_SAVE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-mission-save-x86.tsv\"")
set_source_files_properties(FortBadgesSaveTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_BADGES_SAVE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-badges-save-x86.tsv\"")
set_source_files_properties(FortBadgesGlobalsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_BADGES_GLOBALS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-badges-globals-x86.tsv\"")
set_source_files_properties(EditorPlayersTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_EDITOR_PLAYERS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/editor-players-x86.tsv\"")
set_source_files_properties(FortCurrentNameTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_CURRENT_NAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-current-name-x86.tsv\"")
set_source_files_properties(FortNotifyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_NOTIFY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-notify-x86.tsv\"")
# 게임메뉴부모와Tell인자의독립PE관측을해당검사에만공급한다.
set_source_files_properties(FortObjectsSaveTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_OBJECTS_PARENT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-objects-parent-x86.tsv\"")
set_source_files_properties(FortRegionSaveTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_REGION_SAVE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-region-save-x86.tsv\"")
set_source_files_properties(ThemeTechnologyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_THEME_TECHNOLOGY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/theme-technology-x86.tsv\"")
set_source_files_properties(DialogCommandLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DIALOG_COMMAND_LIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dialog-command-lifetime-x86.tsv\"")
set_source_files_properties(GameMenuBarTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GAME_MENU_BAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/game-menu-bar-x86.tsv\"")
set_source_files_properties(GameMenusTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GAME_MENUS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/game-menus-x86.tsv\";NETSTORM_PLAYER_LIST_MENU_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-list-menu-x86.tsv\"")
set_source_files_properties(TellParametersTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TELL_PARAMETERS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tell-parameters-x86.tsv\"")
set_source_files_properties(GameMenuActionsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GAME_MENU_ACTIONS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/game-menu-actions-x86.tsv\"")
set_source_files_properties(MessageGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MESSAGE_GUMP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/message-gump-x86.tsv\"")
set_source_files_properties(DialogExitCommandTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DIALOG_EXIT_COMMAND_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dialog-exit-command-x86.tsv\"")
# 원본 UserInput 앞단의 입력·정적 기록·합성 release 관측을 해당 검사에만 공급한다.
set_source_files_properties(GumpPointerBoundaryTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMP_POINTER_BOUNDARY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gump-pointer-boundary-x86.tsv\"")
# HUD 기하와 입력의 독립 원본 관측을 두 검사에만 공급한다.
set_source_files_properties(HudDockTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_HUD_DOCK_GEOMETRY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/hud-dock-geometry-x86.tsv\"")
# 전체 Uber 부모의 최초 독립 관측을 해당 검사에만 공급한다.
set_source_files_properties(UberRootGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_UBER_ROOT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/uber-root-x86.tsv\"")
set_source_files_properties(HudDividerInputTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_HUD_DIVIDER_INPUT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/hud-divider-input-x86.tsv\"")
# 미션 시작 안내의 독립 원문 조립/serial 재사용 관측을 해당 검사에 공급한다.
set_source_files_properties(MissionLoadingMessageTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_LOADING_MESSAGE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-loading-message-x86.tsv\"")
# 단위 테스트. 외부 테스트 프레임워크 없이 TestSupport.h 의 작은 도구만 쓴다
# (첫 구성 때 인터넷에서 내려받을 것이 없다).
# 테스트 파일을 만들면 아래 목록에 더한다. 파일 안의 TEST_CASE 는 자동으로 등록된다.
# 미니맵기하의독립원본관측을해당검사에만공급한다.
set_source_files_properties(MinimapGeometryTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MINIMAP_GEOMETRY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/minimap-geometry-x86.tsv\"")
# 실제 미니맵 복사·전체 외곽선의 독립 x86 픽셀과 guard 읽기 주소를 이 검사에만 공급한다.
set_source_files_properties(MinimapBlitTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MINIMAP_BLIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/minimap-blit-x86.tsv\"")
# 미니맵셀의독립원본전체bitmap관측을이검사에만공급한다.
set_source_files_properties(MinimapRasterTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MINIMAP_CELL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/minimap-cell-x86.tsv\"")
# 생산 단축키의 독립 원문 중 실제 키 구간에 도달한 관측을 대조한다.
set_source_files_properties(ProductionShortcutsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRODUCTION_SHORTCUTS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/production-shortcuts-x86.tsv\"")
# 객체/청크 writer의 독립 PE 관측을 해당 검사에서만 읽는다.
set_source_files_properties(FortObjectSaveTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_OBJECT_SAVE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-object-save-x86.tsv\";NETSTORM_FORT_CHUNK_SAVE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-chunk-save-x86.tsv\"")
# Territory 부모/두 getter/seed의 독립 원본 관측을 해당 검사에만 공급한다.
set_source_files_properties(FortTerritorySaveTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_TERRITORY_SAVE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-territory-save-x86.tsv\";NETSTORM_FORT_TERRITORY_QUERY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-territory-query-x86.tsv\";NETSTORM_FORT_TERRITORY_SEED_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-territory-seed-x86.tsv\"")
# 내용물 저장의 독립 원본 관측은 해당 검사에만 공급한다.
set_source_files_properties(FortContentSaveTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_CONTENT_SAVE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-content-save-x86.tsv\"")
# 원본004167a0 단계 함수의 독립 x86 관찰을 다리 진행 검사에 공급한다.
set_source_files_properties(AiBridgePathTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_AI_BRIDGE_STEP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ai-bridge-step-x86.tsv\"")
# 실제 색 띠·CRT 정렬·팔레트 병합의 독립 원본 입력을 새 검사에서만 읽는다.
set_source_files_properties(ParticleGradientTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PARTICLE_GRADIENT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/particle-gradient-x86.tsv\";NETSTORM_FIERYTAIL_LIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fierytail-lifetime-x86.tsv\"")
# 실제 Fiery Tail Frame/Draw의 독립 원본 비트는 이 검사에서만 읽는다.
set_source_files_properties(FieryTailFrameTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FIERYTAIL_FRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fierytail-frame-x86.tsv\"")
# 실제 생성·등록·120입자 초기화·공유 색/반납 순서의 원본 관찰이다.
# 공통 RenderProcess의 실제 Form 삭제 dirty 영역을 새 통합 검사에 공급한다.
set_source_files_properties(FieryTailVisualSystemTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_RENDER_REMOVAL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/render-removal-x86.tsv\"")
set_source_files_properties(FieryTailLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FIERYTAIL_LIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fierytail-lifetime-x86.tsv\"")
# 실제 Draw→화면 wrapper→VFX의 전체 픽셀 원문을 새 표시 통합 검사에 공급한다.
set_source_files_properties(ParticleRenderingTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FIERYTAIL_PIXELS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fierytail-pixels-x86.tsv\"")
# 썬더의 실제 포구/종점·Missile 생성/프레임 전체 원문을 새 검사에서만 읽는다.
set_source_files_properties(ThunderMissileTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_THUNDER_MISSILE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/thunder-missile-x86.tsv\";NETSTORM_THUNDER_HIT_EVENT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/thunder-hit-event-x86.tsv\"")
# 실제 장애물 후보 목록과 기존 Gun 통지의 독립 원본 입력이다.
set_source_files_properties(GunBlockerTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUN_BLOCKER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gun-blocker-x86.tsv\"")
# 공유 빔의 실제64BYTE/중간 상태/120입자 원문을 새 경로 검사에서만 읽는다.
set_source_files_properties(LightningPathTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_LIGHTNING_PATH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/lightning-path-x86.tsv\"")
# 공유 빔 Frame/Draw의 독립 전체 상태/출력 요청을 새 검사에서만 읽는다.
set_source_files_properties(LightningFrameTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_LIGHTNING_FRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/lightning-frame-x86.tsv\"")
# 공유 빔 생성/삭제의 두 정밀도 원문을 새 수명 검사에서만 읽는다.
set_source_files_properties(LightningLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_LIGHTNING_LIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/lightning-lifetime-x86.tsv\"")
# 반더 발사와 두 빔 wrapper의 독립 원문 전체 상태를 새 검사에서만 읽는다.
set_source_files_properties(VanderFireTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_VANDER_FIRE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vander-fire-x86.tsv\"")
# 픽셀 파편과 플라이어 전용 Pre의 독립 원문을 각각 새 검사에 공급한다.
set_source_files_properties(ShrapnelFrameTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SHRAPNEL_FRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/shrapnel-frame-x86.tsv\"")
set_source_files_properties(FlyerPreDestroyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FLYER_PREDESTROY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/flyer-predestroy-x86.tsv\"")
# 실제 연소 출력/수명과 공통 프레임gate의 독립 원문을 새 검사에만 공급한다.
set_source_files_properties(BurningLifetimeDrawTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BURNING_LIFETIME_DRAW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/burning-lifetime-draw-x86.tsv\";NETSTORM_RENDER_FRAME_GATE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/render-frame-gate-x86.tsv\";NETSTORM_BURNING_ARCHER_CAPTURE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/burning-archer-capture-x86.tsv\"")
add_executable(netstorm_tests
    EditorFortFileDevicesTests.cpp CampaignExistingEditorCheckpointTests.cpp
    StartupStateCheckTests.cpp ChatManagerPrefixTests.cpp PlayerCoreDataTests.cpp
    KencloudTests.cpp ClientGameRandomOwnerTests.cpp FortCoreWriteTests.cpp
    ClientChatCoreGlobalsTests.cpp
    ColorSortStartupTests.cpp
    CampaignTemplateLoadFileTests.cpp
    ScreenPaletteApplyTests.cpp
    ExistingEditorAfterCheckpointTests.cpp
    ClientRandomGlobalsTests.cpp
    CampaignExistingEditorSubscriberTests.cpp
    FortPaletteTransitionTests.cpp
    FortTypeNamesAccessFixtureTests.cpp
    FortTypeNamesStorageAccessTests.cpp
    FortTypeConversionStorageTests.cpp
    FortContentsLoadTests.cpp
    ExistingEditorInstallOuterTests.cpp
    FortSubscriberLoadTests.cpp
    SubscriberIdGenerateTests.cpp
    CampaignExistingEditorTests.cpp
    FortTypeNamesLoadTests.cpp
    FortDeckLoadTests.cpp
    DeckRestoreTests.cpp
    FortTerritoryRecoverTests.cpp
    EditorWorldFinishTests.cpp
    CampaignFortSnapshotTests.cpp
    ClientFortFileOptionsTests.cpp
    ClientScriptMenuOptionsTests.cpp
    FortSnapshotParentTests.cpp
    FortMoneySaveTests.cpp
    FortMissionSaveTests.cpp
    FortBadgesSaveTests.cpp
    FortBadgesGlobalsTests.cpp
    EditorPlayersTests.cpp
    FortCurrentNameTests.cpp
    FortNotifyTests.cpp
    TemplateSerializationGlobalsTests.cpp
    FortObjectsSaveTests.cpp
    FortRegionSaveTests.cpp
    CampaignTemplateDynamicTests.cpp
    CampaignFortObjectsSnapshotTests.cpp
    GameMenuBarTests.cpp GameMenusTests.cpp CampaignGameMenuTests.cpp TellParametersTests.cpp
    GameMenuActionsTests.cpp MessageGumpTests.cpp
    ViewKeyInputTests.cpp
    GifScreenshotTests.cpp
    F8KeyInputTests.cpp
    BannerOptionTests.cpp
    SendMoneyInputTests.cpp
    HideBuildingsTests.cpp
    CameraViewCommandsTests.cpp CampaignViewCommandsTests.cpp
    DialogExitCommandTests.cpp
    ThemeTechnologyTests.cpp
    DialogCommandLifetimeTests.cpp
    DialogRuntimeActionTests.cpp
    CampaignThemeTechnologyTests.cpp
    GumpPointerBoundaryTests.cpp
    MissionLoadingMessageTests.cpp
    HudDockTests.cpp
    UberRootGumpTests.cpp
    HudDividerInputTests.cpp
    MinimapGeometryTests.cpp
    MinimapBlitTests.cpp
    MinimapRasterTests.cpp
    ProductionShortcutsTests.cpp
    FortTerritorySaveTests.cpp
    FortContentSaveTests.cpp
    FortObjectSaveTests.cpp
    AiBridgePathTests.cpp
    AiBridgeCandidatesTests.cpp
    AiBridgeSearchTests.cpp
    AiBridgePlanningTests.cpp
    AiBridgeStrategiesTests.cpp
    AiRuntimeTests.cpp
    KnowledgeMenuTests.cpp
    MissionTechnologyTests.cpp
    GuideTransitionTests.cpp
    MissionLifecycleTests.cpp
    TutorialInitTests.cpp
    MissionFortRequestTests.cpp
    TemplateLifetimeTests.cpp
    TemplateLoadTests.cpp
    AiWalletTests.cpp
    AiObjectNoticeTests.cpp
    AiWorldRegistrationTests.cpp
    AiPreparationTests.cpp
    AiStrategySetupTests.cpp
    AiEconomyTests.cpp
    AiDeploymentTests.cpp
    AiCollectorsTests.cpp
    AiFactoryProductionTests.cpp
    AiDrySpotsTests.cpp
    AiMovementTests.cpp
    AiResourceChoiceTests.cpp
    AiCarrierActionsTests.cpp
    AiBuildSearchTests.cpp
    AiBuildChoiceTests.cpp
    AiPriestActionsTests.cpp
    AiDropTargetsTests.cpp
    AiDropEvaluationTests.cpp
    GunGroundTargetTests.cpp
    AiDropScanTests.cpp
    AiDropSelectionTests.cpp
    AiDryGeometryTests.cpp
    AiFakeDrySpotsTests.cpp
    AiDropActionsTests.cpp
    SquidNameTests.cpp
    FactoryUpgradeCommandTests.cpp
    FactoryUpgradeTests.cpp
    FactoryInputTests.cpp
    MissionControlTests.cpp
    MissionGuideInputTests.cpp
    ObjectCommandTests.cpp
    BurningLifetimeDrawTests.cpp
    ShrapnelFrameTests.cpp
    FlyerPreDestroyTests.cpp
    VanderFireTests.cpp
    LightningLifetimeTests.cpp
    LightningVisualSystemTests.cpp
    LightningFrameTests.cpp
    LightningPathTests.cpp
    ThunderMissileTests.cpp
    GunBlockerTests.cpp
    ParticleRenderingTests.cpp
    FieryTailVisualSystemTests.cpp
    FieryTailLifetimeTests.cpp
    FieryTailFrameTests.cpp
    ParticleGradientTests.cpp
    FenceRegularTests.cpp
    FenceMarkerClearTests.cpp
    FenceCreateTests.cpp
    BuriedRegularTests.cpp
    RainAviaryProcessTests.cpp
    FlyerPostPopTests.cpp
    FlyerFrameTests.cpp
    FlyerTargetTests.cpp
    PlayerColorTests.cpp
    GraphResetTests.cpp
    GunAngleTests.cpp
    FencePostPopTests.cpp
    FenceNeighborsTests.cpp
    FenceLineTests.cpp
    SunAviaryProcessTests.cpp
    BuriedPostPopTests.cpp
    DialogBodyLayoutTests.cpp
    HtmlTextParserTests.cpp
    HtmlConditionEffectTests.cpp
    DialogConditionFlowTests.cpp
    RegionOwnerTests.cpp
    LocalGeyserTests.cpp
    MissionStartTests.cpp
    ScriptCommandTests.cpp
    PlayerTechnologyTests.cpp
    PlayerGeyserBoundsTests.cpp
    ConstructionPacketTests.cpp
    AiBrainTests.cpp
    AiBrainInitialUpdateTests.cpp
    TypeRequiredEnergyTests.cpp
    CampaignLifetimeTests.cpp
    MissionFrameGateTests.cpp
    TutorialOneTests.cpp
    TutorialTwoTests.cpp
    WarningTests.cpp
    PriestMenuTests.cpp
    PlayerParticipationTests.cpp
    RainDrawTests.cpp
    FenceTargetTests.cpp
    CampaignNoticeTests.cpp
    FormPacketTests.cpp
    CampaignWorldRuntimeTests.cpp
    CampaignMotionTests.cpp
    CampaignFrameTests.cpp
    CampaignTargetTests.cpp
    CampaignFireTests.cpp
    CampaignHitTests.cpp
    CampaignSeedTests.cpp
    CampaignSessionTests.cpp
    CampaignConstructionTests.cpp
    ChatChannelsTests.cpp
    LocalMissionTransportTests.cpp
    EditorLocalTransportTests.cpp # Editor의 실제 음수 sender/큐/폴링 중단/상대 종료와 현재 수신 BYTE를 검사한다.
    MissionGuideTests.cpp
    PriestParticipationTests.cpp
    TutorialLaterTests.cpp
    MissionLoginTests.cpp
    MissionLoginTimerTests.cpp
    MissionClientPacketsTests.cpp
    MissionServerPacketsTests.cpp
    MissionBattleOptionsTests.cpp
    ProductionDeckTests.cpp
    FortImagePacketsTests.cpp
    MissionSettingsPacketTests.cpp
    LocalMissionStateTests.cpp
    CampaignMissionStartTests.cpp
    VortexPreDestroyTests.cpp
    BattleBroadcastTests.cpp
    WindowTitleTests.cpp
    CameraHomeTests.cpp
    ProjectileGeometryTests.cpp
    DiscRegularTests.cpp
    CampaignNativeFrameTests.cpp
    VortexDisplayTests.cpp
    CampaignInstallSupport.h
    VortexOwnerTests.cpp
    # 준비된 원본 관찰과 실제 캠페인 부모·Player 예약·Burning 프레임을 대조한다.
    CampaignWorldLoadTests.cpp
    PlayerRegularTests.cpp
    BurningFrameTests.cpp
    ProcessRetakeTests.cpp
    FortObjectLoadTests.cpp
    FortFinishOwnerTests.cpp
    GunProcessTests.cpp
    RawPathRuntimeTests.cpp
    RawPathSearchTests.cpp
    FactoryPostPopTests.cpp
    MetadisplayDirtyTests.cpp
    RawPathDataTests.cpp
    GunTargetTests.cpp
    RawPathStartTests.cpp
    GunAnimationTests.cpp
    AssetRegularTests.cpp
    RegionPaintTests.cpp
    MissileTests.cpp
    GunHitTests.cpp
    VortexAscendTests.cpp
    GeyserMinedTests.cpp
    PriestDamageTests.cpp
    PathCleanupTests.cpp
    TerrainStampTests.cpp
    PriestRitualActionsTests.cpp
    ArcherMissileTests.cpp
    ArcherAnimationTests.cpp
    CrossbowAnimationTests.cpp
    CannonFireTests.cpp
    CrossbowMissileTests.cpp
    CampaignDamageTests.cpp
    AiDamageTests.cpp
    IslandRuntimeTests.cpp
    TerrainWorldBindingTests.cpp
    TempleRitualTests.cpp
    TerrainContextTests.cpp
    RunePostPopTests.cpp
    IslandQueriesTests.cpp
    DamageableHitTests.cpp
    FarmClusterTests.cpp
    TerrainGrowthTests.cpp
    GunImpactTests.cpp
    SquidDefenseTests.cpp
    NormalTerrainWorldBindingTests.cpp
    VortexIslandBindingTests.cpp
    ArcherTypeDataTests.cpp
    DaisRegularTests.cpp
    AltarSacrificeTests.cpp
    IslandBuilderTests.cpp
    FortMoneyLoadTests.cpp
    DudeNoticeTests.cpp
    DaisLifecycleTests.cpp
    DaisProgressTests.cpp
    DamageablePostDestroyTests.cpp
    BurningVisualTests.cpp
    BurningParticleTests.cpp
    DamageablePostDestroyHostTests.cpp
    FortMetaLoadTests.cpp
    DudeTaskTests.cpp
    DaisContentNoticeTests.cpp
    DaisRitualConsumerTests.cpp
    DudeSpawnTests.cpp
    DudeDropTests.cpp
    DudeSpawnSupport.h
    FortWorldLoadTests.cpp
    FortWorldLoadBindingTests.cpp
    GraphFormsTests.cpp
    DaisBridgeTests.cpp
    BurningSourcesTests.cpp
    ShapeToBufferTests.cpp
    DudeSpaceTests.cpp DudeSpaceSupport.h
    GeyserAiRegistrationTests.cpp
    PriestHaltTests.cpp
    CarrierArrivalTests.cpp CarrierArrivalSupport.h
    PriestPathArrivalHostTests.cpp
    PlayerWorldTests.cpp
    RawButtonControlsTests.cpp
    GumpResetTests.cpp
    MissionOutcomeTests.cpp
    ContentLifetimeTests.cpp
    FormLifecycleTests.cpp
    TileGroupCollectionTests.cpp
    ObjectCollectionTests.cpp
    PreviewCollectionTests.cpp
    PreviewDrawTests.cpp
    SpriteDrawTests.cpp
    GumpCollectionTests.cpp
    GumpDrawTests.cpp
    GumpCreateTests.cpp
    GumpTreeTests.cpp
    GumpLifetimeTests.cpp
    GumpEventsTests.cpp
    GumpInputTests.cpp
    GumpFrameTests.cpp
    MenuGumpTests.cpp
    ButtonGumpTests.cpp
    GumpTextTests.cpp
    GumpGlyphTests.cpp
    DrawSubmissionTests.cpp
    DrawSortTests.cpp
    RenderCollectionTests.cpp
    ConstructionVisualSystemTests.cpp
    ConstructionLifetimeTests.cpp
    ConstructionFrameTests.cpp
    ConstructionVisualTests.cpp
    ContainedLookupTests.cpp
    ProcessFormTests.cpp
    ProcessTransmitTests.cpp
    FormTakeTests.cpp
    ConstructionTimeTests.cpp
    ConstructionDataTests.cpp
    ConstructionProcessTests.cpp
    ConstructionBuilderTests.cpp
    PriestLookupTests.cpp
    ConstructionEffectsTests.cpp
    ReturnPieceTests.cpp
    CursorDropTests.cpp
    TestMain.cpp
    TestSupport.h
    ClientMainTests.cpp
    RecoveryTests.cpp
    OracleTests.cpp
    AssetTests.cpp
    TypeHotspotTests.cpp
    CanonTypeDecoderTests.cpp
    CanonPixelShapeTests.cpp
    CanonPlacementGeometryTests.cpp
    CanonPlacementTests.cpp
    CanonPlacementPreviewTests.cpp
    CanonPlacementCollisionTests.cpp
    CanonPlacementTerrainTests.cpp
    CanonPlacementPermissionTests.cpp
    PlayerPlacementAnchorTests.cpp
    CanonPlacementSurroundingTests.cpp
    CanonPlacementRelationsTests.cpp
    CanonMayPlaceTests.cpp
    OutpostLifecycleTests.cpp
    RegionOwnershipTests.cpp
    ConstructionPlaceTests.cpp
    ConstructionConfirmTests.cpp
    ConstructionClearTests.cpp
    ConstructionNoticeTests.cpp
    ConstructionRequestTests.cpp
    ConstructionLocalTests.cpp
    PriestShieldTests.cpp
    PriestPostPopTailTests.cpp
    PriestFallTests.cpp
    CarrierCheckTests.cpp
    FrameAdvanceTests.cpp
    ForcefieldLifecycleTests.cpp
    SoundProcessTests.cpp
    SoundPlayTests.cpp
    SoundDeviceTests.cpp
    SoundControlTests.cpp
    SoundMusicTests.cpp
    SoundMusicStreamTests.cpp
    SoundMusicOpenTests.cpp
    SoundMusicSelectionTests.cpp
    SoundMusicRuntimeTests.cpp
    SoundSceneMusicTests.cpp
    ClientAudioTests.cpp
    PriestPopTests.cpp
    ConfigOracleTests.cpp
    ConfigInterfaceTests.cpp
    FortTests.cpp
    ScreenTests.cpp
    PaletteFlashTests.cpp
    PaletteColorsTests.cpp
    GumpVisualTests.cpp
    GumpBackgroundTests.cpp
    GumpBackgroundDrawTests.cpp
    TerritoryTests.cpp
    RendererTests.cpp
    MenuTests.cpp
    WorldTests.cpp
    RawSurfaceWorldTests.cpp
    RawSurfaceStorageTests.cpp
    RawPriestWorldBindingsTests.cpp
    SpRotateTests.cpp
    SpStoreNoticeTests.cpp
    AssetPostPopTests.cpp
    BuildingSpaceTests.cpp
    ConstructionArrivalTests.cpp
    BridgeTests.cpp
    BridgeDecayTests.cpp
    BridgeLifecycleTests.cpp
    BridgeEventTests.cpp
    RawSceneSupport.h
    ConstructionSupport.h
    BridgeConnectTests.cpp
    IslandLifecycleTests.cpp
    IslandPostPopTests.cpp
    SurfaceLifecycleTests.cpp
    SurfaceLifecycleInspect.h
    SetFrameTests.cpp
    PathAnimationTests.cpp
    FrameDisplayTests.cpp
    OwnerTests.cpp
    PriestOwnerTests.cpp
    PriestPostPopTests.cpp
    CarrierPostPopTests.cpp
    CarrierPreDestroyTests.cpp
    DamageablePreDestroyTests.cpp
    DamageableReleaseTests.cpp
    PriestSpawnTests.cpp
    PriestPlacementTests.cpp
    PriestPlacementPreviewTests.cpp
    PriestPlacementCollisionTests.cpp
    PriestPlacementGeometryTests.cpp
    PriestPlacementShapeTests.cpp
    PriestPlacementTerrainTests.cpp
    PriestStateTests.cpp
    PriestRegenTests.cpp
    PriestPreDestroyTests.cpp
    PriestForcefieldTests.cpp
    RawFinderTests.cpp
    NeighborTests.cpp
    RawDestroyTests.cpp
    DestroyDispatchTests.cpp
    DestroyLifecycleTests.cpp
    DestroyGraphTests.cpp
    RewardTests.cpp
    ProcessTests.cpp
    SurfaceTests.cpp
    HashTests.cpp
    SpatialTests.cpp
    SidTests.cpp
    CreationTests.cpp
    UnpopTests.cpp
    PopTests.cpp
    DisplayTests.cpp
    PostPopTests.cpp
    GraphTests.cpp
    RawGraphTests.cpp
)
netstorm_apply_compiler_options(netstorm_tests)
target_link_libraries(netstorm_tests PRIVATE netstorm)
# 원본 파일·Python·Ghidra 없이도 기계어 기대값 회귀 검사를 실행한다.
target_compile_definitions(netstorm_tests PRIVATE
    NETSTORM_ORACLE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/original-x86.tsv"
    NETSTORM_GRAPHICS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/graphics-x86.tsv"
    NETSTORM_CONFIG_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/config-x86.tsv"
    NETSTORM_OPTIONS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/options-x86.tsv"
    NETSTORM_RENDERER_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/renderer-x86.tsv"
    NETSTORM_BRIDGE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/bridge-x86.tsv"
    NETSTORM_BRIDGEDECAY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/bridgedecay-x86.tsv"
    NETSTORM_BRIDGEEFFECTS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/bridgeeffects-x86.tsv"
    NETSTORM_BRIDGEEVENT_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/bridgeevent-x86.tsv"
    NETSTORM_BRIDGECONNECT_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/bridgeconnect-x86.tsv"
    NETSTORM_SETFRAME_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/setframe-x86.tsv"
    NETSTORM_PATHANIMATION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/pathanimation-x86.tsv"
    NETSTORM_ISLANDLIFECYCLE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/islandlifecycle-x86.tsv"
    NETSTORM_ISLANDPOSTPOP_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/islandpostpop-x86.tsv"
    NETSTORM_OWNER_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/owner-x86.tsv"
    NETSTORM_PRIESTOWNER_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestowner-x86.tsv"
    NETSTORM_PRIESTPOSTPOP_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestpostpop-x86.tsv"
    NETSTORM_CARRIERPOSTPOP_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrierpostpop-x86.tsv"
    NETSTORM_PRIESTSTATE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/prieststate-x86.tsv"
    NETSTORM_RAWFINDER_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/rawfinder-x86.tsv"
    NETSTORM_NEIGHBOR_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/neighbor-x86.tsv"
    NETSTORM_DESTROY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/destroy-x86.tsv"
    NETSTORM_DESTROYLIFECYCLE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/destroylifecycle-x86.tsv"
    NETSTORM_DESTROYGRAPH_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/destroygraph-x86.tsv"
    NETSTORM_REWARD_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/reward-x86.tsv"
    NETSTORM_PROCESS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/process-x86.tsv"
    NETSTORM_SURFACE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/surface-x86.tsv"
    NETSTORM_HASH_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/hash-x86.tsv"
    NETSTORM_SPATIAL_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/spatial-x86.tsv"
    NETSTORM_SID_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sid-x86.tsv"
    NETSTORM_CREATION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/creation-x86.tsv"
    NETSTORM_DERIVED_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/derived-x86.tsv"
    NETSTORM_UNPOP_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/unpop-x86.tsv"
    NETSTORM_UNPOP_VTABLE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/unpop-vtables.tsv"
    NETSTORM_POP_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/pop-x86.tsv"
    NETSTORM_POP_VTABLE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/pop-vtables.tsv"
    NETSTORM_DISPLAY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/display-x86.tsv"
    NETSTORM_POSTPOP_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/postpop-x86.tsv"
    NETSTORM_BRIDGEPOSTPOP_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/bridgepostpop-x86.tsv"
    NETSTORM_GRAPH_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/graph-x86.tsv"
    NETSTORM_RAWGRAPH_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/rawgraph-x86.tsv"
    NETSTORM_REGIONGRAPH_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/regiongraph-x86.tsv"
    NETSTORM_GRAPHREMOVE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/graphremove-x86.tsv"
    NETSTORM_GRAPHLOOKUP_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/graphlookup-x86.tsv"
    NETSTORM_GRAPHREBUILD_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/graphrebuild-x86.tsv"
    NETSTORM_GRAPHRECOVERY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/graphrecovery-x86.tsv"
    NETSTORM_CONSTRUCTOR_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructors-x86.tsv")

# ctest 에 실행 파일 하나를 테스트 하나로 등록한다. 실패한 검사가 있으면 종료 코드가 1 이다.
# 새 회복 fixture는 해당 소스에만 지정해 기존 검사 전체의 재컴파일을 줄인다.
set_source_files_properties(PriestRegenTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTREGEN_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestregen-x86.tsv\"")
# 사제 삭제 준비의 독립 fixture는 해당 검사 소스에만 지정한다.
set_source_files_properties(PriestPreDestroyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTDESTROY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestdestroy-x86.tsv\"")
# 실제 보호막 조회와 삭제 준비의 독립 fixture는 해당 검사에만 지정한다.
set_source_files_properties(PriestForcefieldTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTFORCEFIELD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestforcefield-x86.tsv\"")
add_test(NAME netstorm_tests COMMAND netstorm_tests)
# 원본 돌 버튼의 전체 생성/그리기 관찰은 새 검사 소스에서만 읽는다.
set_source_files_properties(GumpVisualTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMPVISUAL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumpvisual-x86.tsv\"")
# 원본 전체 ColorSort/명암 생성/질감 배경의 독립 관찰을 이 검사에만 지정한다.
set_source_files_properties(GumpBackgroundTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMPBACKGROUND_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumpbackground-x86.tsv\"")
# Carrier/실제 contained 조회의 독립 fixture는 해당 검사 소스에만 지정한다.
set_source_files_properties(CarrierPreDestroyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIERPREDESTROY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrierpredestroy-x86.tsv\"")
# Damageable 소리 접두/실제 종속 순회의 독립 fixture를 해당 검사에만 지정한다.
set_source_files_properties(DamageablePreDestroyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DAMAGEABLEPREDESTROY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/damageablepredestroy-x86.tsv\"")

# 실제 종속 해방과 WORD 복사의 독립 fixture는 해당 검사에만 지정한다.
set_source_files_properties(DamageableReleaseTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DAMAGEABLERELEASE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/damageablerelease-x86.tsv\"")

# 사제 나선 탐색·생성/활성화의 독립 fixture는 해당 검사에만 지정한다.
set_source_files_properties(PriestSpawnTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTSPAWN_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestspawn-x86.tsv\"")

# 사제 배치 접두의 실제 기계어 관찰은 해당 검사 소스에만 지정한다.
set_source_files_properties(PriestPlacementTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTPLACEMENT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestplacement-x86.tsv\"")

# 사제 배치 미리보기의 실제 지도/관계 관찰은 해당 검사 소스에만 지정한다.
set_source_files_properties(PriestPlacementPreviewTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTPREVIEW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestpreview-x86.tsv\"")

# 사제 충돌 후보의 독립 원본 관찰 fixture 경로다.
set_source_files_properties(PriestPlacementCollisionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTCOLLISION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestcollision-x86.tsv\"")

# 비패턴 사제 모양의 독립 원본 관찰 fixture 경로다.
set_source_files_properties(PriestPlacementGeometryTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTGEOMETRY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestgeometry-x86.tsv\"")

# 비패턴 사제 픽셀 모양의 독립 원본 관찰 fixture 경로다.
set_source_files_properties(PriestPlacementShapeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTSHAPE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestshape-x86.tsv\"")

# 일반 사제 지형/지역 조회의 독립 원본 관찰 fixture 경로다.
set_source_files_properties(PriestPlacementTerrainTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTTERRAIN_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestterrain-x86.tsv\"")
# 타입 기준점의 실제 속성/ftol 관찰 fixture를 해당 검사에만 지정한다.
set_source_files_properties(TypeHotspotTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TYPEHOTSPOT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/typehotspot-x86.tsv\"")
# 타입별 패턴 생성/전체 순회의 독립 원본 fixture는 해당 검사 소스에만 지정한다.
set_source_files_properties(CanonTypeDecoderTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CANONTYPE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/canontype-x86.tsv\"")

# 일반/패턴 픽셀 모양의 독립 기계어 관찰 경로다.
set_source_files_properties(CanonPixelShapeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CANONSHAPE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/canonshape-x86.tsv\"")

# 일반/패턴 전체 배치 모양의 독립 원본 관찰 경로다.
set_source_files_properties(CanonPlacementGeometryTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CANONGEOMETRY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/canongeometry-x86.tsv\"")

# 일반 배치 접두의 별도 패턴 인자/원본 전역 관찰 경로다.
set_source_files_properties(CanonPlacementTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CANONPLACEMENT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/canonplacement-x86.tsv\"")
# 일반 타입/별도 패턴 인자의 미리보기 관찰은 기존 사제 fixture와 분리한다.
target_compile_definitions(netstorm_tests PRIVATE
    "NETSTORM_CANONPREVIEW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/canonpreview-x86.tsv\"")
# 일반 후보의 실제 중간 구간 관찰은 새 검사 소스에서만 읽는다.
set_source_files_properties(CanonPlacementCollisionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CANONCOLLISION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/canoncollision-x86.tsv\"")
# 일반 후보 지형/초기 권한/최종 지면 누적의 독립 원본 구간을 재생한다.
set_source_files_properties(CanonPlacementTerrainTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CANONTERRAIN_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/canonterrain-x86.tsv\"")
# 후보 권한/방향 소유 관계의 독립 원본 관찰을 해당 검사 소스에서만 읽는다.
set_source_files_properties(CanonPlacementPermissionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CANONPERMISSION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/canonpermission-x86.tsv\"")
# Player 기준점/거리/그래프/종속의 실제 함수 관찰은 해당 검사 소스에서만 읽는다.
set_source_files_properties(PlayerPlacementAnchorTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYERANCHOR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/playeranchor-x86.tsv\"")
# 모양 표면 finder/주변 권한 구간의 독립 원본 관찰을 해당 검사 소스에서만 읽는다.
set_source_files_properties(CanonPlacementSurroundingTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CANONSURROUNDING_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/canonsurrounding-x86.tsv\"")
# 최종 표면 관계/특수 지역/실제 반환 구간의 독립 관찰을 해당 검사 소스에서만 읽는다.
set_source_files_properties(CanonPlacementRelationsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CANONRELATIONS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/canonrelations-x86.tsv\"")
# 전체 MayPlace 정상 반환의 독립 관찰을 새 파이프라인 검사에서만 읽는다.
set_source_files_properties(CanonMayPlaceTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CANONMAYPLACE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/canonmayplace-x86.tsv\"")
# outpost의 두 별도 목록 수명 접두 관찰을 새 검사 소스에서만 읽는다.
set_source_files_properties(OutpostLifecycleTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_OUTPOSTLIFECYCLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/outpostlifecycle-x86.tsv\"")
# 지역 소유 투표 전체 wrapper의 독립 기계어 관찰 경로다.
set_source_files_properties(RegionOwnershipTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_REGIONOWNERSHIP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/regionownership-x86.tsv\"")
# 건설 배치 실행·비용 부족 처리·환불/취소 통지의 독립 원본 관찰이다.
set_source_files_properties(ConstructionPlaceTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONPLACE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructionplace-x86.tsv\"")
# 서버 건설 확정 전체 몸체의 독립 원본 관찰이다.
set_source_files_properties(ConstructionConfirmTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONCONFIRM_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructionconfirm-x86.tsv\"")
# 로컬 예측 조각 정리 전체 몸체의 독립 원본 관찰이다.
set_source_files_properties(ConstructionClearTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONCLEAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructionclear-x86.tsv\"")
# 사제 보호막 전체 생성 wrapper의 독립 원본 관찰이다.
set_source_files_properties(PriestShieldTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTSHIELD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestshield-x86.tsv\"")
# 사제 postPop 전체 wrapper의 독립 원본 관찰 경로다.
set_source_files_properties(PriestPostPopTailTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTPOSTPOPTAIL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestpostpoptail-x86.tsv\"")

# 원본 낙하 요청/0x25b 관찰을 일반 C++ 검사에서 재생할 경로다.
set_source_files_properties(PriestFallTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTFALL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestfall-x86.tsv\"")

# Carrier 지면 조회/사제 가상 낙하의 독립 원본 관찰을 재생할 경로다.
set_source_files_properties(CarrierCheckTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIERCHECK_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carriercheck-x86.tsv\"")

# 실제 프레임 진행/지정/글자 구간 생성의 독립 원본 관찰을 재생할 경로다.
set_source_files_properties(FrameAdvanceTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FRAMEADVANCE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/frameadvance-x86.tsv\"")

# 실제 공통 Pop/Activate→사제 postPop의 독립 원본 관찰을 재생한다.
set_source_files_properties(PriestPopTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTPOP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestpop-x86.tsv\"")

# 보호막 전용 postPop/Regular/일반 타입 finder의 독립 원본 관찰이다.
set_source_files_properties(ForcefieldLifecycleTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORCEFIELDPOSTPOP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/forcefieldpostpop-x86.tsv\"")

# 소리 프로세스/이름 표의 독립 원본 관찰을 재생한다.
set_source_files_properties(SoundProcessTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SOUNDPROCESS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/soundprocess-x86.tsv\"")

# 소리 재생 계층(전역/위치 재생·정지·화면 위치 계산)의 독립 원본 관찰을 재생한다.
set_source_files_properties(SoundPlayTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SOUNDPLAY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/soundplay-x86.tsv\"")

# 실제 Winmm 읽기를 세 PE의 독립 WAVE 관찰과 대조한다. 장치를 열지 않는다.
set_source_files_properties(SoundDeviceTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SOUNDDEVICE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sounddevice-x86.tsv\"")

# 소리 제어 여섯 몸체의 독립 원본 관찰이다. CTest는 장치/PE/Python 실행 없이 저장한 결과를 대조한다.
set_source_files_properties(SoundControlTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SOUNDCONTROL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/soundcontrol-x86.tsv\"")

# 실제 음악 채널/공유 음소거 명령 관찰이다. 테스트는 파일/장치/스레드를 열지 않는다.
set_source_files_properties(SoundMusicTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MUSICCONTROL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/musiccontrol-x86.tsv\"")

# 실제 음악 버퍼 생성/채우기의 독립 관찰이다. 기본 검사는 장치/스레드를 실행하지 않는다.
set_source_files_properties(SoundMusicStreamTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MUSICSTREAM_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/musicstream-x86.tsv\"")

# 실제 음악 파일 열기/길이/공개 시작의 두 x87 관찰을 이 검사에만 지정한다.
set_source_files_properties(SoundMusicOpenTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MUSICOPEN_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/musicopen-x86.tsv\"")
# 상위 음악 옵션/선택의 독립 기계어 관찰이다. 기본 C++ 검사는 게임/Python을 실행하지 않는다.
set_source_files_properties(SoundMusicSelectionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MUSICSELECTION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/musicselection-x86.tsv\"")
# 두 음악 채널/worker 초기화·종료의 독립 기계어 관찰이다. 실제 Windows worker 검사는 원본/장치를 열지 않는다.
set_source_files_properties(SoundMusicRuntimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MUSICRUNTIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/musicruntime-x86.tsv\"")

# 장면별 음악 감독(곡 요청·다음 곡·프레임·시작·날씨)의 독립 원본 관찰을 재생한다.
set_source_files_properties(SoundSceneMusicTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SCENEMUSIC_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/scenemusic-x86.tsv\"")

# 팔레트 색 검색의 세 실제 PE 관찰이다. 기본 CTest는 원본/창/장치를 실행하지 않고 저장한 관찰을 비교한다.
set_source_files_properties(ScreenTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PALETTECOLOR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/palettecolor-x86.tsv\"")

# 번개 생성/초기화/진행/중단의 실제 x86 관찰 파일을 새 검사에만 전달한다.
set_source_files_properties(PaletteFlashTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PALETTEFLASH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/paletteflash-x86.tsv\"")

# 전체 파일 팔레트 로더/색 표의 독립 x86 관찰이다. CTest는 원본/장치를 실행하지 않는다.
set_source_files_properties(PaletteColorsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PALETTECOLORS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/palettecolors-x86.tsv\"")

# 건설 배치 통지 처리기의 독립 원본 관찰이다. CTest는 원본 게임/Python을 실행하지 않는다.
set_source_files_properties(ConstructionNoticeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONNOTICE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructionnotice-x86.tsv\"")

# 서버 건설 요청 전체의 독립 원본 관찰이다. CTest는 실제 게임/Python을 실행하지 않는다.
set_source_files_properties(ConstructionRequestTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONREQUEST_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructionrequest-x86.tsv\"")

# 로컬 건설 배치 전체의 저장 PE 관찰이다. CTest는 원본 게임/Python을 실행하지 않는다.
set_source_files_properties(ConstructionLocalTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONLOCAL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructionlocal-x86.tsv\"")

# 커서 내려놓기의 세 실제 PE 관찰이다. 기본 검사는 게임/창/Python을 실행하지 않는다.
set_source_files_properties(CursorDropTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CURSORDROP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cursordrop-x86.tsv\"")

# 생산 칸 반환/contained 생성의 세 실제 PE 관찰이다. 기본 검사는 게임/Python을 실행하지 않는다.
set_source_files_properties(ReturnPieceTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_RETURNPIECE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/returnpiece-x86.tsv\"")

# 건설 밀어내기/섬 정리의 독립 PE 관찰이다. 기본 검사는 실제 게임/Python을 실행하지 않는다.
set_source_files_properties(ConstructionEffectsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONEFFECTS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructioneffects-x86.tsv\"")

# 소유 사제 조회 전체의 세 PE 관찰이다. CTest는 원본 게임/Python을 실행하지 않는다.
set_source_files_properties(PriestLookupTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTLOOKUP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestlookup-x86.tsv\"")

# 건설 생성/이동 전체와 공급원 소비의 독립 관찰은 해당 검사에만 지정한다.
set_source_files_properties(ConstructionBuilderTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONBUILDER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructionbuilder-x86.tsv\";NETSTORM_CONSTRUCTIONTIMER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructiontimer-x86.tsv\"")

# 건설 프로세스 시작/진행/완료의 독립 PE 관찰이다. CTest는 원본 게임/Python을 실행하지 않는다.
set_source_files_properties(ConstructionProcessTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONPROCESS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructionprocess-x86.tsv\"")

# 끝 시각 재설정 전체의 세 PE 관찰은 실제 host 연결 검사에서만 읽는다.
set_source_files_properties(ConstructionTimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONTIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructiontime-x86.tsv\"")

# 건설 상태의 독립 원본 패킷/필드 관찰만 읽으며 원본 게임을 실행하지 않는다.
set_source_files_properties(ConstructionDataTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONDATA_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructiondata-x86.tsv\"")

# 지정 free form/즉시 단일 분배의 저장 PE 관찰만 읽으며 실제 게임을 실행하지 않는다.
set_source_files_properties(ProcessFormTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PROCESSFORM_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/processform-x86.tsv\"")
set_source_files_properties(ProcessTransmitTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PROCESSTRANSMIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/processtransmit-x86.tsv\"")
set_source_files_properties(FortObjectLoadTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORTOBJECTLOAD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fortobjectload-x86.tsv\"")
set_source_files_properties(FortFinishOwnerTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORTFINISHOWNER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fortfinishowner-x86.tsv\"")
set_source_files_properties(GunProcessTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUNPROCESS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gunprocess-x86.tsv\"")
set_source_files_properties(RawPathRuntimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PATHRUNTIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/pathruntime-x86.tsv\"")
set_source_files_properties(RawPathSearchTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PATHSEARCH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/pathsearch-x86.tsv\";NETSTORM_PATHINIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/pathinit-x86.tsv\"")

# 내장 Form/공통 Unpop/풀 재확보의 두 실제 PE 600개 저장 관찰을 직접 대조한다.
set_source_files_properties(FormTakeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORMTAKE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/formtake-x86.tsv\"")

# 종속 조회/삭제의 저장 PE 관찰만 읽으며 실제 게임/Python은 실행하지 않는다.
set_source_files_properties(ContainedLookupTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONTAINED_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/contained-x86.tsv\"")

# 건설 표시 생성/초기화의 저장 PE 관찰만 읽으며 게임/Python은 실행하지 않는다.
set_source_files_properties(ConstructionVisualTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONVISUAL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructionvisual-x86.tsv\"")

# 건설 프레임 전체 몸체의 저장 PE 관찰만 읽고 실제 게임/장치를 실행하지 않는다.
set_source_files_properties(ConstructionFrameTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONFRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructionframe-x86.tsv\"")

# 표시 등록/소멸자의 독립 PE 관찰만 읽으며 원본 게임/장치는 실행하지 않는다.
set_source_files_properties(ConstructionLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONLIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructionlifetime-x86.tsv\"")

# 가시 표시 수집/환산/정렬 비교의 독립 PE 관찰을 새 검사에서만 읽는다.
set_source_files_properties(RenderCollectionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_RENDERCOLLECTION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/rendercollection-x86.tsv\"")

# 원본 CRT의 판본별 동률/교환 관찰을 새 검사에서만 읽는다.
set_source_files_properties(DrawSortTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DRAWSORT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/drawsort-x86.tsv\"")

# 공통 표시 병합/분배의 상위 Renderer 독립 관찰이다. CTest는 원본 게임/Python/장치를 실행하지 않는다.
set_source_files_properties(DrawSubmissionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DRAWSUBMISSION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/drawsubmission-x86.tsv\"")
# UI 트리/좌표의 독립 원본 기계어 관찰이다.
target_compile_definitions(netstorm_tests PRIVATE
    "NETSTORM_GUMPCOLLECTION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumpcollection-x86.tsv\"")

# 실제 건설 프로세스 제출 순서도 같은 독립 동률 관찰과 비교한다.
set_source_files_properties(ConstructionVisualSystemTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DRAWSORT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/drawsort-x86.tsv\"")

# 행별 지면/다리 묶음의 실제 PE 관찰은 이 검사 소스에서만 읽는다.
set_source_files_properties(TileGroupCollectionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TILEGROUP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tilegroup-x86.tsv\"")

# 일반 자산/그림자의 실제 PE 관찰은 이 검사 소스에서만 읽는다.
set_source_files_properties(ObjectCollectionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_OBJECTCOLLECTION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/objectcollection-x86.tsv\"")

# 커서 미리보기의 부분 출력/두 레이어와 준비된 SP 읽기 실제 관찰이다.
set_source_files_properties(PreviewCollectionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PREVIEWCOLLECTION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/previewcollection-x86.tsv\"")

# 미리보기 Draw의 전체 분기/격자/색 표/문구 배치 관찰이다.
set_source_files_properties(PreviewDrawTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PREVIEWDRAW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/previewdraw-x86.tsv\"")

# 일반 sprite Draw의 세 판본별 전체 기계어 관찰이다.
set_source_files_properties(SpriteDrawTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SPRITEDRAW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/spritedraw-x86.tsv\"")

# Gump Draw의 전체 클립 보호/가상 호출 관찰은 이 검사 소스에서만 읽는다.
set_source_files_properties(GumpDrawTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMPDRAW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumpdraw-x86.tsv\"")

# 기본/루트 생성의 전체 바이트와 1000개 등록 칸 관찰은 이 검사 소스에서만 읽는다.
set_source_files_properties(GumpCreateTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMPCREATE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumpcreate-x86.tsv\"")

# Gump 트리 전체 바이트/콜백 당시 상태는 이 검사 소스에서만 읽는다.
set_source_files_properties(GumpTreeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMPTREE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumptree-x86.tsv\"")

# 지연 삭제/기본 소멸자의 전체 표/노드/콜백 상태를 이 검사 소스에서만 읽는다.
set_source_files_properties(GumpLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMPLIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumplifetime-x86.tsv\"")

# 커서 선택/필터/가상 이벤트 당시 전체 노드·전역의 실제 관찰이다.
set_source_files_properties(GumpEventsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMPEVENTS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumpevents-x86.tsv\"")

# 입력 캡처/dialog/overlay의 실제 전체 표·노드·전역·콜백 기록이다.
set_source_files_properties(GumpInputTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMPINPUT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumpinput-x86.tsv\"")

# 실제 세 PE의 통지/idle/재그리기 전체 몸체와 콜백 당시 전체 상태를 저장한 관찰이다.
set_source_files_properties(GumpFrameTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMPFRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumpframe-x86.tsv\"")

# 전체 버튼 노드/표/콜백과 실제 Gump 하위 몸체를 관찰한 세 PE의 전수 입력이다.
set_source_files_properties(ButtonGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BUTTONGUMP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/buttongump-x86.tsv\";NETSTORM_BUTTONGUMP_PATCH_CASES=252;NETSTORM_BUTTONGUMP_CD_CASES=245")

# 원본 폭 표/논리 높이와 직접/기본 Gump 위임의 독립 PE 출력이다. 게임/원본 자산은 실행하지 않는다.
set_source_files_properties(GumpTextTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMPTEXT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumptext-x86.tsv\"")

# 실제 세 PE의 글리프/색/로그 호출 관찰이다. 기본 CTest는 원본 게임/Python/창을 실행하지 않는다.
set_source_files_properties(GumpGlyphTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMPGLYPH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumpglyph-x86.tsv\"")

# 전체 배경 Draw/CRT/소프트웨어 fill의 세 실제 PE 관찰이다. 기본 검사는 게임/원본/Python을 실행하지 않는다.
set_source_files_properties(GumpBackgroundDrawTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUMPBACKGROUNDDRAW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumpbackgrounddraw-x86.tsv\"")

# 실제 두 PE의 메뉴 크기/확장/스택 콜백 당시 상태를 이 검사 소스에서만 읽는다.
set_source_files_properties(MenuGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MENUGUMP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/menugump-x86.tsv\"")

# 감시 중첩 삭제/Content 정렬·피해의 두 PE 독립 출력이며 기본 CTest는 게임/원본/창을 실행하지 않는다.
set_source_files_properties(FormLifecycleTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORMLIFECYCLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/formlifecycle-x86.tsv\";NETSTORM_FORMLIFECYCLE_CASES=456")

# 실제 두 PE의 너겟 preDestroy/구조 연결·전체 상태 관찰이다. 기본 검사는 원본 게임/창을 실행하지 않는다.
set_source_files_properties(ContentLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONTENTLIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/contentlifetime-x86.tsv\"")

# 지정 ProcessForm 재수신의 실제 두 PE 관찰 입력이다.
set_source_files_properties(ProcessRetakeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PROCESSRETAKE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/processretake-x86.tsv\"")

# UI 정리/루트 교체와 실제 사제 상태 기반 승패의 두 PE 관찰 입력이다.
set_source_files_properties(GumpResetTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_UIRESET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/uireset-x86.tsv\"")
set_source_files_properties(MissionOutcomeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSIONOUTCOME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/missionoutcome-x86.tsv\"")
set_source_files_properties(RawButtonControlsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_RAWBUTTON_VISUAL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gumpvisual-x86.tsv\"")

# SP 저장 비트의 실제 회전과 lazy 초기화가 남긴 10.78 PE 독립 출력이다.
set_source_files_properties(SpRotateTests.cpp SpStoreNoticeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SPROTATE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sprotate-x86.tsv\"")

# 건물 가상 postPop과 일반 Pop/Unpop 옆면의 독립 원본 관찰이다.
set_source_files_properties(AssetPostPopTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_ASSETPOSTPOP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/assetpostpop-x86.tsv\";NETSTORM_ASSETPLAYER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/asset-player-x86.tsv\"")
# Factory/아비어리의 실제 일반 finder·프레임 구간·목록 등록 관찰이다.
set_source_files_properties(FactoryPostPopTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FACTORYPOSTPOP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factorypostpop-x86.tsv\"")
# 미니맵 큐의 고유 등록·카메라 비교·전체/부분 갱신 실제 PE 관찰이다.
set_source_files_properties(MetadisplayDirtyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_METADISPLAYDIRTY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/metadisplaydirty-x86.tsv\"")
# 실제 서버 Path 공유 버퍼와 두 판본 Prepare 속도 전달 관찰이다.
set_source_files_properties(RawPathDataTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PATHDATA_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/pathdata-x86.tsv\";NETSTORM_PATHPREPARE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/pathprepare-x86.tsv\"")
# 실제 Gun 표적 finder·각도·MayShoot 두 판본 관찰이다.
set_source_files_properties(GunTargetTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUNTARGET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/guntarget-x86.tsv\"")
set_source_files_properties(BuildingSpaceTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BUILDINGSPACE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/buildingspace-x86.tsv\"")

# 사제 월드의 실제 타입별 이동 Process selector와 전역 입력의 독립 원본 관찰이다.
set_source_files_properties(RawPriestWorldBindingsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTWORLDEVENTS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestworldevents-x86.tsv\"")

# 건설 운반 객체 도착의 실제 finder/시작/예측 대기/자기 삭제 두 PE 관찰이다.
set_source_files_properties(ConstructionArrivalTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTIONARRIVAL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constructionarrival-x86.tsv\"")

# 실제 사제 경로 시작/프로세스 종류 선택과 Gun 애니메이션 및 자산 Handle의 독립 PE 관찰이다.
set_source_files_properties(RawPathStartTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PATHSTART_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/pathstart-x86.tsv\";NETSTORM_PATHPROCESSTYPE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/pathprocesstype-x86.tsv\"")
set_source_files_properties(GunAnimationTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUNANIMATION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gunanimation-x86.tsv\"")
set_source_files_properties(AssetRegularTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_ASSETREGULAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/assetregular-x86.tsv\"")
set_source_files_properties(RegionPaintTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_REGIONPAINT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/regionpaint-x86.tsv\"")

# 썬 캐논 투사체 생성/비행·직접 피해 시각과 신전 Ascend 정리의 실제 두 PE 입력이다.
set_source_files_properties(MissileTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUNMISSILE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gunmissile-x86.tsv\"")
set_source_files_properties(GunHitTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUNHIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gunhit-x86.tsv\"")
set_source_files_properties(VortexAscendTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_VORTEXASCEND_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vortexascend-x86.tsv\"")

# 실제 채집 패킷·사제 피격과 기존 HP 전환/피해 효과 판정의 양판본 원본 입력이다.
set_source_files_properties(GeyserMinedTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GEYSERMINED_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/geysermined-x86.tsv\"")
set_source_files_properties(PriestDamageTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTDAMAGE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestdamage-x86.tsv\"")

# 실제 경로 해제의 두 판본 차이와 초기 지형의 독립 CRT/공유 후보 선택을 대조한다.
set_source_files_properties(PathCleanupTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PATHCLEANUP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/pathcleanup-x86.tsv\"")
set_source_files_properties(TerrainStampTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TERRAINSTAMP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/terrainstamp-x86.tsv\"")

# 신전이 요청하는 실제 사제 K~P 첫 프레임과604 SharedRegular 예약 wrapper의 양판본 입력이다.
set_source_files_properties(PriestRitualActionsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTRITUALACTIONS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestritualactions-x86.tsv\"")

# 썬 디스크 발사기71의 별도1888개 투사체 입력이며 이전 썬 캐논296개와 합치지 않는다.
set_source_files_properties(ArcherMissileTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_ARCHERMISSILE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/archermissile-x86.tsv\"")

# 실제 썬 디스크 발사기 애니메이션과 mutable100개 섬 레코드의 양판본 독립 입력이다.
set_source_files_properties(ArcherAnimationTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUNARCHER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gunarcher-x86.tsv\"")
set_source_files_properties(IslandRuntimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_ISLANDRUNTIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/islandruntime-x86.tsv\"")
set_source_files_properties(VortexOwnerTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_VORTEXOWNER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vortex-owner-x86.tsv\"")

# 실제 신전 의식 상위와 초기 지형 constructor/경계문자의 정상·assert 결과다.
set_source_files_properties(TempleRitualTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TEMPLERITUAL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/templeritual-x86.tsv\"")
set_source_files_properties(TerrainContextTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TERRAINCONTEXT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/terraincontext-x86.tsv\"")

# 실제Rune 파생Pop/건설시작과7/10/25초의정상출력은상위Temple 요청대조와별도입력이다.
set_source_files_properties(RunePostPopTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_RUNEPOSTPOP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/runepostpop-x86.tsv\"")

# 실제 현재 섬 검색·일반 피해·군집 단일배치·초기 성장의 고유 양판본 입력이다.
set_source_files_properties(IslandQueriesTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_ISLANDQUERIES_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/islandqueries-x86.tsv\"")
set_source_files_properties(DamageableHitTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DAMAGEABLEHIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/damageablehit-x86.tsv\"")
set_source_files_properties(FarmClusterTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FARMCLUSTER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/farmcluster-x86.tsv\"")
set_source_files_properties(TerrainGrowthTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TERRAINGROWTH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/terraingrowth-x86.tsv\"")

# 실제 충돌/기본방어 입력과 기존Ascend Count40개의 live3204바이트 재사용 검사다.
set_source_files_properties(GunImpactTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUNIMPACT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gunimpact-x86.tsv\"")
set_source_files_properties(SquidDefenseTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SQUIDDEFENSE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/squiddefense-x86.tsv\"")
set_source_files_properties(VortexIslandBindingTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_VORTEXASCEND_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vortexascend-x86.tsv\"")

# 실제썬디스크TYPE/좁은의식·Altar후속/초기섬배치·SP전수입력의단위11 검사다.
set_source_files_properties(ArcherTypeDataTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_ARCHERTYPE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/archertype-x86.tsv\"")
set_source_files_properties(DaisRegularTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DAISREGULAR_CORE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/daisregular-core-x86.tsv\"")
set_source_files_properties(AltarSacrificeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_ALTARSACRIFICE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/altarsacrifice-x86.tsv\"")
set_source_files_properties(IslandBuilderTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_ISLANDBUILDER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/islandbuilder-x86.tsv\"")
set_source_files_properties(FortMoneyLoadTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORTMONEY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fortmoney-x86.tsv\"")
# 실제피해반응/현재Regular 재설정·대피의414고유입력이다.
set_source_files_properties(DudeNoticeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DUDENOTICE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dudenotice-x86.tsv\"")

# 실제 수명·상위 의식·피해 후처리·Burning 생성/입자의 별도 고유 원본 입력이다.
set_source_files_properties(DaisLifecycleTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DAISLIFECYCLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/daislifecycle-current-x86.tsv\"")
set_source_files_properties(DaisProgressTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DAISPROGRESS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/daisprogress-x86.tsv\"")
set_source_files_properties(DamageablePostDestroyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DAMAGEABLEPOSTDESTROY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/damageablepostdestroy-x86.tsv\"")
set_source_files_properties(BurningVisualTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BURNINGPROCESS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/burningprocess-constructor-x86.tsv\"")
set_source_files_properties(BurningParticleTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BURNINGPARTICLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/burningparticle-x86.tsv\"")

# 실제 작업 설정/저장 후처리와 포획부착의 독립 원본 및 같은 Host 소비자를 대조한다.
set_source_files_properties(FortMetaLoadTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORTMETA_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fortmeta-x86.tsv\"")
set_source_files_properties(DudeTaskTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DUDETASK_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dudetask-x86.tsv\"")
set_source_files_properties(DaisContentNoticeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DAISCONTENTNOTICE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/daiscontentnotice-x86.tsv\"")

# 실제 Dude 생성/전체 Fort 부모 및 내부 Form 그래프의 독립 원본 결과다.
set_source_files_properties(DudeSpawnTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DUDESPAWN_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dudespawn-x86.tsv\"")
set_source_files_properties(FortWorldLoadTests.cpp FortWorldLoadBindingTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORTWORLDLOAD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fortworldload-x86.tsv\"")
set_source_files_properties(GraphFormsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GRAPHFORMS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/graphforms-x86.tsv\"")

# 의식 단계에서 실제 Bridge/Path 준비와 같은 Host 소비자를 대조한다.
set_source_files_properties(DaisBridgeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DAISBRIDGE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/daisbridge-x86.tsv\"")

# 실제 Dude 드롭 부모와 같은 생성/Host/Task 연결의 독립308행이다.
set_source_files_properties(DudeDropTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DUDEDROP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dudedrop-x86.tsv\"")

# 실제 Player와 도착 처리·픽셀 캡처의 단위14 원본 자료를 각 검사에만 공급한다.
set_source_files_properties(BurningSourcesTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BURNINGSOURCES_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/burningsources-x86.tsv\"")
set_source_files_properties(ShapeToBufferTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SHAPETOBUFFER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/shape-to-buffer-x86.tsv\"")
set_source_files_properties(DudeSpaceTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DUDESPACE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dude-space-x86.tsv\"")
set_source_files_properties(GeyserAiRegistrationTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GEYSER_AI_REGISTRATION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/geyser-ai-registration-x86.tsv\"")
set_source_files_properties(PriestHaltTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTHALT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priesthalt-x86.tsv\"")
set_source_files_properties(CarrierArrivalTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIERARRIVAL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrierarrival-x86.tsv\"")
set_source_files_properties(PlayerWorldTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_WORLD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-world-x86.tsv\"")
# 각 새 검사에서 독립 원본 fixture를 읽도록 해당 번역단위에만 경로를 지정한다.
set_source_files_properties(CampaignWorldLoadTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CAMPAIGN_WORLDLOAD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/campaign-worldload-x86.tsv\"")
set_source_files_properties(PlayerRegularTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_REGULAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-regular-x86.tsv\"")
set_source_files_properties(BurningFrameTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BURNINGFRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/burningframe-x86.tsv\"")

# 새 현재 방향/Factory 독립 원본 입력은 해당 번역단위에서만 읽는다.
set_source_files_properties(CampaignNativeFrameTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CAMPAIGN_FRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/campaign-frame-x86.tsv\"")

# 현재 ProcessForm 가상34의 독립 PE 입력을 해당 소비자에만 제공한다.
set_source_files_properties(CampaignTargetTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUN_DEPEND_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gun-depend-x86.tsv\"")

# 새 신전 활성 표시의 독립 native 변경표를 해당 검사에서만 읽는다.
set_source_files_properties(VortexDisplayTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_VORTEX_DISPLAY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vortex-display-x86.tsv\"")

# 포구/충돌 상자와 썬 캐논 방향 일치/불일치 사정선의 독립 원본 입력이다.
set_source_files_properties(ProjectileGeometryTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PROJECTILE_GEOM_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/projectile-geom-x86.tsv\";NETSTORM_CANNON_RAYEND_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cannon-rayend-x86.tsv\"")

# 썬 디스크 예약 Handle의 독립 요청/반환 입력이다.
set_source_files_properties(DiscRegularTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DISC_REGULAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/disc-regular-x86.tsv\"")

# 실제 총→사제 호출 사슬의 별도 독립 원본 입력이다.
set_source_files_properties(CampaignHitTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CAMPAIGN_HIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/campaign-hit-x86.tsv\"")

# 실제 생산 초기화에 필요한 원본 CRT/게임 시드의 별도 입력이다.
set_source_files_properties(CampaignSeedTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CAMPAIGN_SEED_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/campaign-seed-x86.tsv\"")

# 실제 Player9 송신 분배의 독립 원본 입력이다.
set_source_files_properties(BattleBroadcastTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BATTLE_BCAST_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/battle-bcast-x86.tsv\";NETSTORM_FLYER_COMMAND_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/flyer-command-x86.tsv\"")

# 현재 호스트에서 관찰한 두 판본의 창 제목 선택/설정 인자 자료다.
set_source_files_properties(WindowTitleTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_WINDOW_TITLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/window-title-x86.tsv\"")

# 초기 시야/실제 x87 중심 변환의 두 판본 독립 관찰이다.
set_source_files_properties(CameraHomeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CAMERA_HOME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/camera-home-x86.tsv\"")
  set_source_files_properties(FormPacketTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORM_PACKET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/form-packet-x86.tsv\"")

# 실제 PATCH Player 피해 래퍼의 AI 호출 경계 관찰이다.
set_source_files_properties(CampaignNoticeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_DAMAGE_NOTICE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-damage-notice-x86.tsv\"")

# 실제 PATCH fencemark 전체 표적/교차/포구의 원본 관찰이다.
set_source_files_properties(FenceTargetTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FENCE_TARGET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fence-target-x86.tsv\"")

# 원본 레인 신전 가상 Draw의 두 판본 호출 순서다.
set_source_files_properties(RainDrawTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_RAIN_DRAW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/rain-draw-x86.tsv\"")

# 원본 메시지 표/기한/idle의 명시 경계 관찰이다.
set_source_files_properties(WarningTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_WARNINGS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/warnings-x86.tsv\"")

# 실제 PATCH 사제 메뉴/선택 관문의 독립 기계어 관찰을 전수 재생한다.
set_source_files_properties(PriestMenuTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIESTMENU_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priestmenu-x86.tsv\"")

# 실제 Player 로그인/동맹/brain필드수명의고유기계어관찰을전수대조한다.
set_source_files_properties(PlayerParticipationTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_PARTICIPATION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-participation-x86.tsv\"")

# 실제 미션 공통 게이트와 Early1/Totalmade의 독립 원본 입력 경로다.
set_source_files_properties(MissionFrameGateTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSIONFRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/missionframe-x86.tsv\"")
set_source_files_properties(TutorialOneTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TUTORIAL_ONE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tutorial-one-x86.tsv\"")

# 공통 게이트와 같은 상태에서 실행하는 실제 Tutorial2의 독립 원본 입력이다.
set_source_files_properties(TutorialTwoTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TUTORIALTWO_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tutorial-two-x86.tsv\"")

# 실제 두 PE의 지면 소유자/float snap/CRT 독립 입력이다.
set_source_files_properties(RegionOwnerTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_REGION_OWNER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/region-owner-x86.tsv\"")

# 실제 로컬 간헐천 생성/난수/최근 목록의 독립 원본 입력이다.
set_source_files_properties(LocalGeyserTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_LOCALGEYSER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/local-geyser-x86.tsv\"")

# 실제 미션 시작/AI 설정의 독립 원본 결과다.
set_source_files_properties(MissionStartTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_START_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-start-x86.tsv\"")

# 두 실제 PE의 기본 HTML 본문 변환을 독립 입력으로 재생한다.
set_source_files_properties(HtmlTextParserTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_HTMLTEXT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/htmltext-x86.tsv\"")

# 실제 토큰/인라인 명령/Config/문자열80칸 고리를 대조한다.
set_source_files_properties(ScriptCommandTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SCRIPT_COMMAND_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/script-command-x86.tsv\"")

# 실제 Player 기술 제거/생성/목록을 대조한다.
set_source_files_properties(PlayerTechnologyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_TECHNOLOGY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-technology-x86.tsv\"")

# 실제 Player 간헐천 배치 사각형과 패턴/회전을 대조한다.
set_source_files_properties(PlayerGeyserBoundsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_GEYSER_BOUNDS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-geyser-bounds-x86.tsv\"")

# 실제 건설 패킷의 전체512바이트/가변길이를 대조한다.
set_source_files_properties(ConstructionPacketTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTION_PACKET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/construction-packet-x86.tsv\"")

# 실제 AI 생성자/소멸자의 전체 소유 저장소를 대조한다.
set_source_files_properties(AiBrainTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_AI_BRAIN_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ai-brain-x86.tsv\"")

# 실제 Player 내용물 통지와 같은 세션 공통 삭제를 대조한다.
set_source_files_properties(CampaignLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_CONTENT_NOTICE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-content-notice-x86.tsv\"")

# 실제 AI 초기갱신 원문을 대조한다.
set_source_files_properties(AiBrainInitialUpdateTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_AI_INITIAL_UPDATE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ai-initial-update-x86.tsv\"")

# 실제 TYPE mana 후처리와 같은 보호 자산 188개 공급을 대조한다.
set_source_files_properties(TypeRequiredEnergyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TYPE_ENERGY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/type-required-energy-x86.tsv\"")

# 실제 채팅 기록과 단독 미션 전송의 독립 기계어 관찰 경로다.
set_source_files_properties(ChatChannelsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CHAT_CHANNELS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-channels-x86.tsv\"")
set_source_files_properties(LocalMissionTransportTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_LOCAL_MISSION_TRANSPORT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/local-mission-transport-x86.tsv\"")

# 실제 Guide/Advance와 포획된 사제를 포함한 참여 상태의 독립 원본 기대값이다.
set_source_files_properties(MissionGuideTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_GUIDE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-guide-x86.tsv\"")
set_source_files_properties(PriestParticipationTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIEST_PARTICIPATION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priest-participation-x86.tsv\"")
set_source_files_properties(TutorialLaterTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TUTORIAL_LATER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tutorial-later-x86.tsv\"")
set_source_files_properties(MissionLoginTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_LOGIN_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-login-x86.tsv\"")
set_source_files_properties(MissionLoginTimerTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_LOGIN_TIMER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-login-timer-x86.tsv\"")
set_source_files_properties(MissionClientPacketsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_CLIENT_PACKETS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-client-packets-x86.tsv\"")
set_source_files_properties(MissionServerPacketsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_SERVER_PACKETS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-server-packets-x86.tsv\"")

# 실제 옵션과 현재 생산 Deck/Factory 재계산의 독립 원본 기대값이다.
set_source_files_properties(MissionBattleOptionsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_BATTLE_OPTIONS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-battle-options-x86.tsv\";NETSTORM_MISSION_OPTION_RESET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-option-reset-x86.tsv\"")
set_source_files_properties(ProductionDeckTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRODUCTION_DECK_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/production-deck-x86.tsv\";NETSTORM_PRODUCTION_REFRESH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/production-refresh-x86.tsv\"")
set_source_files_properties(FortImagePacketsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_IMAGE_PACKETS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-image-packets-x86.tsv\"")
set_source_files_properties(MissionSettingsPacketTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_SETTINGS_PACKET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-settings-packet-x86.tsv\"")
set_source_files_properties(LocalMissionStateTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_LOCAL_MISSION_STATE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/local-mission-state-x86.tsv\"")
set_source_files_properties(VortexPreDestroyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_VORTEX_PRE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vortex-pre-x86.tsv\"")

# buried의 현재 QA/개별 할당/예약 순서를 원본 기계어 결과와 대조한다.
set_source_files_properties(BuriedPostPopTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BURIED_POSTPOP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/buried-postpop-x86.tsv\"")

# 썬 아비어리의 네 원문 범위를 독립 기계어 기대값으로 대조한다.
set_source_files_properties(SunAviaryProcessTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SUN_AVIARY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sun-aviary-x86.tsv\"")

# 펜스 접두/이웃/검색점은 서로 다른 원문 관찰 자료로 대조한다.
set_source_files_properties(FencePostPopTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FENCE_POSTPOP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fence-postpop-x86.tsv\"")
set_source_files_properties(FenceNeighborsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FENCE_NEIGHBORS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fence-neighbors-x86.tsv\"")
set_source_files_properties(FenceLineTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FENCE_LINE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fence-line-x86.tsv\"")

# Gunanim 프레임→방향은 실제 원본 함수 반환값과 대조한다.
set_source_files_properties(GunAngleTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUN_ANGLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gun-angle-x86.tsv\"")

# 원본 Graph 초기화는 전체255행의 바이트 보존과 대조한다.
set_source_files_properties(GraphResetTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GRAPH_RESET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/graph-reset-x86.tsv\"")

# 실제 색 표/섬 프레임 요청은 원본216개 독립 입력과 대조한다.
set_source_files_properties(PlayerColorTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_COLOR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-color-x86.tsv\"")

# 별도 원본 Rain 생성/후처리/Frame/writer의1602개 전체 출력이다.
set_source_files_properties(RainAviaryProcessTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_RAIN_AVIARY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/rain-aviary-x86.tsv\"")

# 실제 오벨리스크 Handle의 전체 바이트/반환/난수/표시 순서를 대조한다.
set_source_files_properties(BuriedRegularTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BURIED_REGULAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/buried-regular-x86.tsv\"")

# 썬 바리케이드의 실제 제어/재조회/선택 효과 순서를 독립 원본과 대조한다.
set_source_files_properties(FenceRegularTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FENCE_REGULAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fence-regular-x86.tsv\"")

# 실제 일반 finder와 끝점 태그의 원본 삭제 선택을 비교한다.
set_source_files_properties(FenceMarkerClearTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FENCE_MARKER_CLEAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fence-marker-clear-x86.tsv\"")

# 실제 표식/포스필드/재생성의 독립 원본 전체 결과를 비교한다.
set_source_files_properties(FenceCreateTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FENCE_CREATE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fence-create-x86.tsv\"")

set_source_files_properties(FlyerPostPopTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FLYER_POSTPOP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/flyer-postpop-x86.tsv\"")

set_source_files_properties(FlyerFrameTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FLYER_FRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/flyer-frame-x86.tsv\"")

set_source_files_properties(FlyerTargetTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FLYER_TARGET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/flyer-target-x86.tsv\"")

set_source_files_properties(CrossbowAnimationTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CROSSBOW_ANIMATION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/crossbow-animation-x86.tsv\"")

set_source_files_properties(CannonFireTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CANNON_FIRE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cannon-fire-x86.tsv\"")

# 실제 크로스보우 포구·생성·방향·비행의 독립 원본 결과다.
set_source_files_properties(CrossbowMissileTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CROSSBOW_MISSILE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/crossbow-missile-x86.tsv\"")

# 썬더 캐논의 실제 가상 피해·방어/HP/권한·섬 경고를 독립 원본과 대조한다.
set_source_files_properties(CampaignDamageTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CAMPAIGN_DAMAGE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/campaign-damage-x86.tsv\"")

# 실제 AI 피해 목록의 전체 본체/네 backing/효과 순서를 독립 원본과 대조한다.
set_source_files_properties(AiDamageTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_AI_DAMAGE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ai-damage-x86.tsv\"")

# 실제9개 소유자 색표와 보호 SHP의 mapped 캡처 전체 원문이다.
target_sources(netstorm_tests PRIVATE OwnerCaptureTests.cpp)
# 실제 파편 생성/준비/반납의 전체 원문이다.
target_sources(netstorm_tests PRIVATE ShrapnelLifetimeTests.cpp)
# 비행 사망 뒤 홈 풀의 실제 상태/난수/지연 원문이다.
target_sources(netstorm_tests PRIVATE RainHomeNoticeTests.cpp)
set_source_files_properties(RainHomeNoticeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_RAIN_HOME_NOTICE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/rain-home-notice-x86.tsv\"")
set_source_files_properties(ShrapnelLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SHRAPNEL_LIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/shrapnel-lifetime-x86.tsv\"")
set_source_files_properties(OwnerCaptureTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_OWNER_CAPTURE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/owner-capture-x86.tsv\"")
# 실제 제단90프레임/전체 색표·추가 그림 비트의 독립 원본 픽셀이다.
set_property(SOURCE OwnerCaptureTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_ALTAR_CAPTURE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/altar-capture-x86.tsv\"")

# 실제 커서 취득/취소/임시 삭제의 전체 원문과 보호 자산 연결을 비교한다.
target_sources(netstorm_tests PRIVATE CursorLifecycleTests.cpp)
target_sources(netstorm_tests PRIVATE CursorPlacementTests.cpp)
target_sources(netstorm_tests PRIVATE CursorPreviewTests.cpp)
target_sources(netstorm_tests PRIVATE CursorPositionTests.cpp CursorPreviewSupport.h)
set_source_files_properties(CursorPositionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CURSOR_POSITION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cursor-position-x86.tsv\"")
set_source_files_properties(CursorPreviewTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CURSOR_PREVIEWS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cursor-previews-x86.tsv\"")
set_source_files_properties(CursorPlacementTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CURSOR_PLACEMENT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cursor-placement-x86.tsv\"")
set_source_files_properties(CursorLifecycleTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CURSOR_LIFECYCLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cursor-lifecycle-x86.tsv\"")
# 실제 Client 공통 트리의 원본 메뉴 입력/제목/하위 메뉴/픽셀과 삭제를 검사한다.
target_sources(netstorm_tests PRIVATE RawMenuControlsTests.cpp)
# 실제 두 PE의 객체·Damageable·회수 메뉴 전체 효과를 비교한다.
target_sources(netstorm_tests PRIVATE ObjectMenuTests.cpp)
set_source_files_properties(ObjectMenuTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_OBJECT_MENU_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/object-menu-x86.tsv\"")
# 두 판본 Carrier의 실제 운반물/주문 메뉴 전체 원문이다.
target_sources(netstorm_tests PRIVATE CarrierMenuTests.cpp)
set_source_files_properties(CarrierMenuTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_MENU_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-menu-x86.tsv\"")
# 실제 Player 식별자/네 getter/객체 메뉴DWORD의 독립 원문이다.
target_sources(netstorm_tests PRIVATE PlayerIdentityTests.cpp)
set_source_files_properties(PlayerIdentityTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_IDENTITY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-identity-x86.tsv\"")
# 실제 PATCH Player 메뉴 전체의 반환/정적 버퍼/효과 원문이다.
target_sources(netstorm_tests PRIVATE PlayerMenuTests.cpp)
set_source_files_properties(PlayerMenuTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_MENU_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-menu-x86.tsv\"")

# 실제 운반물 위치/전체 발자국 탐색과 기본53비트 절삭의 독립 원문이다.
target_sources(netstorm_tests PRIVATE CarrierDropPointTests.cpp)
set_source_files_properties(CarrierDropPointTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_DROP_POINT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-drop-point-x86.tsv\"")

# 실제 Carrier 부모 통지 전체의 독립 관측이다.
target_sources(netstorm_tests PRIVATE CarrierContentNoticeTests.cpp)
set_source_files_properties(CarrierContentNoticeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_CONTENT_NOTICE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-content-notice-x86.tsv\"")

# 실제 커서 마나 생성·프레임·이벤트·소멸의 독립 원문이다.
target_sources(netstorm_tests PRIVATE CursorManaTests.cpp)
set_source_files_properties(CursorManaTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CURSOR_MANA_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cursor-mana-x86.tsv\"")

# 실제 마나88/8c의 부분 변경 영역 표를 같은 검사에 공급한다.
set_property(SOURCE CursorManaTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_CURSOR_MANA_DISPLAY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cursor-mana-display-x86.tsv\"")

# 실제 사정거리 생성·직선/원형 프레임·통지·소멸의 독립 원문이다.
target_sources(netstorm_tests PRIVATE CursorRangeTests.cpp)
set_source_files_properties(CursorRangeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CURSOR_RANGE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cursor-range-x86.tsv\"")

# 실제 펜스 정리/이웃 갱신/전처리의 전체 원문이다.
target_sources(netstorm_tests PRIVATE FenceLifetimeTests.cpp)
set_source_files_properties(FenceLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FENCE_LIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fence-lifetime-x86.tsv\"")

# 실제 원문 반환/문자열/공유 공급 목록의 전체 결과를 대조한다.
target_sources(netstorm_tests PRIVATE EnergySupplyTests.cpp)
set_source_files_properties(EnergySupplyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_ENERGY_SUPPLY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/energy-supply-x86.tsv\"")

set_property(SOURCE EnergySupplyTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_ENERGY_PROVIDER_VTABLES_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/energy-provider-vtables.tsv\"")

# 실제 공급원 정렬/연속 소비의 전체 원문을 대조한다.
target_sources(netstorm_tests PRIVATE EnergySortTests.cpp)
set_source_files_properties(EnergySortTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_ENERGY_SORT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/energy-sort-x86.tsv\"")

# 실제 비행39와 마나 볼트 도착의 독립 원본 관측 자료다.
set_property(SOURCE RawPathRuntimeTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_FLYPATH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/flypath-x86.tsv\"")
set_property(SOURCE ConstructionArrivalTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_MANA_BOLT_ARRIVAL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mana-bolt-arrival-x86.tsv\"")

# 실제 마나 볼트105의 별도 부분 표시 관측이다.
set_property(SOURCE CursorManaTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_MANA_BOLT_DISPLAY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mana-bolt-display-x86.tsv\"")

# 실제 마나 볼트 Regular/투영/역투영의 독립 관측이다.
target_sources(netstorm_tests PRIVATE ManaBoltRegularTests.cpp)
set_source_files_properties(ManaBoltRegularTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MANA_BOLT_REGULAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mana-bolt-regular-x86.tsv\"")

# 실제 사각형 가장자리/Graph/재귀의 독립 원본 관측이다.
target_sources(netstorm_tests PRIVATE BoardApproachTests.cpp)
set_source_files_properties(BoardApproachTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BOARD_APPROACH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/board-approach-x86.tsv\"")

# 실제 Player 초기화가 확보한9개 공장 목록 크기다.
set_property(SOURCE CampaignSessionTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_FACTORY_INIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-factory-init-x86.tsv\"")

# 실제25바이트 요청과 서버 수신의 두 정밀도 원본 자료다.
target_sources(netstorm_tests PRIVATE ConstructionCommandTests.cpp)
set_source_files_properties(ConstructionCommandTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONSTRUCTION_COMMAND_WRITE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/construction-command-write-x86.tsv\";NETSTORM_CONSTRUCTION_COMMAND_RECEIVE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/construction-command-receive-x86.tsv\"")

# 실제 커서 파괴/불법/글자 그림자 색표의 전체 원본 출력이다.
target_sources(netstorm_tests PRIVATE PreviewColorMapsTests.cpp)
set_source_files_properties(PreviewColorMapsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PREVIEW_COLORS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/preview-colors-x86.tsv\"")

# 실제 CRT 기본 정밀도의 소유자 HSL 전용5종과64비트 별도 관측이다.
set_property(SOURCE PreviewColorMapsTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_OWNER_HSL_COLORS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/owner-hsl-colors-x86.tsv\"")

# 실제 잠금/해제/회전의 두 정밀도 원본289행이다.
target_sources(netstorm_tests PRIVATE CursorActionsTests.cpp)
set_source_files_properties(CursorActionsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CURSOR_ACTIONS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cursor-actions-x86.tsv\"")

# 커서의 실제 최상위 선택/재귀 수신/현재 부모 조회 경계를 검사한다.
target_sources(netstorm_tests PRIVATE RawButtonCursorBoundaryTests.cpp)
target_sources(netstorm_tests PRIVATE CampaignCursorClientEffectsTests.cpp)

# 같은 세션의 사제/Carrier/회수/객체/Player 메뉴 체인과 실제 정수 명령을 검사한다.
target_sources(netstorm_tests PRIVATE CampaignObjectMenuTests.cpp)

# 원본 강조/마우스 초기화의178입력을 두 정밀도의 격리 실행과 대조한다.
target_sources(netstorm_tests PRIVATE UserHighlightTests.cpp)
set_source_files_properties(UserHighlightTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_USER_HIGHLIGHT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/user-highlight-x86.tsv\"")

# 공통 객체 메뉴Open/위치의실제PE788입력과미연결경계를검사한다.
target_sources(netstorm_tests PRIVATE ObjectMenuOpenTests.cpp)

# 객체메뉴의4인자제목/현재경고와실제원본문자열helper를재생한다.
target_sources(netstorm_tests PRIVATE ObjectMenuInfoTests.cpp)

# 여덟 파생 우클릭 작성기의 실제 원문1024입력과 재진입 효과 순서를 대조한다.
target_sources(netstorm_tests PRIVATE ObjectMenuVariantsTests.cpp)
set_source_files_properties(ObjectMenuVariantsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_OBJECT_MENU_VARIANTS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/object-menu-variants-x86.tsv\"")

# 세 인자 선택과 짧은 메뉴 입구의 독립 PE 관찰을 대조한다.
target_sources(netstorm_tests PRIVATE UserSelectionTests.cpp)
set_source_files_properties(UserSelectionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_USER_SELECTION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/user-selection-x86.tsv\"")

# 객체 선택 효과의83VT/전체BYTE와 생성 입구 원문을 재생한다.
target_sources(netstorm_tests PRIVATE SquidSelectionTests.cpp)
set_source_files_properties(SquidSelectionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SQUID_SELECTION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/squid-selection-x86.tsv\"")

# 실제 내용물 표시 타이머의 원문957행과 같은 Host 삭제 수명이다.
target_sources(netstorm_tests PRIVATE ContentPreviewProcessTests.cpp)
set_source_files_properties(ContentPreviewProcessTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CONTENT_PREVIEW_PROCESS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/content-preview-process-x86.tsv\"")

# Carrier의 실제837입력과83VT의 보호PE 슬롯이다.
target_sources(netstorm_tests PRIVATE CarrierRegularTests.cpp)
set_source_files_properties(CarrierRegularTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_REGULAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-regular-x86.tsv\";NETSTORM_CARRIER_REGULAR_VTABLES=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-regular-vtables.tsv\"")

# 너겟의실제288개독립입력이다.
target_sources(netstorm_tests PRIVATE NuggetPostPopTests.cpp)
set_source_files_properties(NuggetPostPopTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_NUGGET_POSTPOP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/nugget-postpop-x86.tsv\"")

# 너겟Regular의실제9019개독립입력이다.
target_sources(netstorm_tests PRIVATE NuggetRegularTests.cpp)
set_source_files_properties(NuggetRegularTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_NUGGET_REGULAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/nugget-regular-x86.tsv\"")

# 캐리어 낙하/해방의 독립 PE 원문을 재생한다.
target_sources(netstorm_tests PRIVATE CarrierFallingTests.cpp)
set_source_files_properties(CarrierFallingTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_FALLING_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-falling-x86.tsv\";NETSTORM_CARRIER_FALLING_VTABLES=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-falling-vtables.tsv\"")

# 실제 자연 낙하 시작/Carrier task의 독립 원문이다.
target_sources(netstorm_tests PRIVATE CarrierFallStartTests.cpp)
set_source_files_properties(CarrierFallStartTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_FALL_START_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-fall-start-x86.tsv\";NETSTORM_WORM_PATH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/worm-path-x86.tsv\";NETSTORM_WORM_PATH_VTABLES=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/worm-path-vtables.tsv\"")

# 너겟 전달의 독립 PE 원문1110개를 재생한다.
target_sources(netstorm_tests PRIVATE CarrierTransferTests.cpp)
set_source_files_properties(CarrierTransferTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_TRANSFER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-transfer-x86.tsv\"")

# 운반 대상과 일반 접근점의 독립 원문2245개다.
target_sources(netstorm_tests PRIVATE CarrierTargetTests.cpp)
set_source_files_properties(CarrierTargetTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_TARGET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-target-x86.tsv\";NETSTORM_CARRIER_TARGET_HARDWARE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-target-hardware.tsv\"")

# 골렘 후보의 실제 주소 비교/정렬 호출 전체를 재생한다.
target_sources(netstorm_tests PRIVATE CarrierCandidateSortTests.cpp)
set_source_files_properties(CarrierCandidateSortTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_CANDIDATE_SORT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-candidate-sort-x86.tsv\"")

# 골렘의 후보 수집·거리·원본 정렬·실제 Prepare 독립 원문이다.
target_sources(netstorm_tests PRIVATE CarrierChoiceTests.cpp)
set_source_files_properties(CarrierChoiceTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_CHOICE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-choice-x86.tsv\"")

# 전체 원본 Halt 안의 운반 대상 재확인4801행을 대조한다.
target_sources(netstorm_tests PRIVATE PathRetargetTests.cpp)
set_source_files_properties(PathRetargetTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PATH_RETARGET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/path-retarget-x86.tsv\"")

# 실제 헤더/송신/로컬 수신의 보호PE864행이다.
target_sources(netstorm_tests PRIVATE CarrierMiningTests.cpp)
set_source_files_properties(CarrierMiningTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_MINING_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-mining-x86.tsv\"")

# 간헐천의 고유 전처리/후처리와 실제 중복 제거2016행이다.
target_sources(netstorm_tests PRIVATE GeyserLifetimeTests.cpp)
set_source_files_properties(GeyserLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GEYSER_LIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/geyser-lifetime-x86.tsv\"")

# 실제 회피1807입력의 깊이/좌표/시각/효과와 전체raw다.
target_sources(netstorm_tests PRIVATE CarrierEscapeTests.cpp)
set_source_files_properties(CarrierEscapeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_ESCAPE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-escape-x86.tsv\"")

# 실제 크랩 포획/수신 원문과 같은 전체 효과를 대조한다.
target_sources(netstorm_tests PRIVATE CarrierStealTests.cpp)
set_source_files_properties(CarrierStealTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_STEAL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-steal-x86.tsv\"")

# 일반포획의실제요청/서버/종속finder 원문을대조한다.
target_sources(netstorm_tests PRIVATE CarrierPickupTests.cpp)
target_sources(netstorm_tests PRIVATE CarrierDropTests.cpp)
target_sources(netstorm_tests PRIVATE BombActivationTests.cpp)
target_sources(netstorm_tests PRIVATE BombRegularTests.cpp)
target_sources(netstorm_tests PRIVATE BombEffectTests.cpp)
target_sources(netstorm_tests PRIVATE BombTargetsTests.cpp)
target_sources(netstorm_tests PRIVATE BombBlastTests.cpp)
target_sources(netstorm_tests PRIVATE BombStrikeTests.cpp)
set_source_files_properties(CarrierPickupTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_PICKUP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-pickup-x86.tsv\"")

# 독립원문의 오벨리스크 보상·변환·Deck 추첨을 검증한다.
target_sources(netstorm_tests PRIVATE GemRewardTests.cpp)
set_source_files_properties(GemRewardTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GEM_REWARD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gem-reward-x86.tsv\"")

# 직접좌표빔/화면wrapper/지연소리의독립원문을대조한다.
target_sources(netstorm_tests PRIVATE LightningScreenTests.cpp)
set_source_files_properties(LightningScreenTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_LIGHTNING_SCREEN_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/lightning-screen-x86.tsv\";NETSTORM_BOMB_PREDESTROY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/bomb-predestroy-x86.tsv\"")

# 반복 보상 보충 부모와 실제 Canon의 독립 원문을 검증한다.
target_sources(netstorm_tests PRIVATE NuggetReplenishTests.cpp)
set_source_files_properties(NuggetReplenishTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_NUGGET_REPLENISH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/nugget-replenish-x86.tsv\"")

# 벌룬42와 그림자 후처리의 새 독립 원문을 대조한다.
target_sources(netstorm_tests PRIVATE BalloonPathTests.cpp)
target_sources(netstorm_tests PRIVATE BulfPathTests.cpp)
set_source_files_properties(BalloonPathTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BALLOON_PATH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/balloon-path-x86.tsv\";NETSTORM_BALLOON_BODY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/balloon-body-x86.tsv\";NETSTORM_BALLOON_BODY_HEIGHT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/balloon-body-height-x86.tsv\"")

# 독립 원본40h 이동 요청과 수신 결과를 대조한다.
target_sources(netstorm_tests PRIVATE CarrierMoveTests.cpp)
set_source_files_properties(CarrierMoveTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CARRIER_MOVE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-move-x86.tsv\"")

# Early Mission 로그인에서실제짧은State 확장의독립원문을검증한다.
target_sources(netstorm_tests PRIVATE PlayerStateExpandTests.cpp)
set_source_files_properties(PlayerStateExpandTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_STATE_EXPAND_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-state-expand-x86.tsv\"")

target_sources(netstorm_tests PRIVATE CarrierInputTests.cpp)
target_compile_definitions(netstorm_tests PRIVATE NETSTORM_CARRIER_INPUT_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/carrier-input-x86.tsv")

# 동맹 수신 원문은 해당 검사 소스만 다시 컴파일하도록 개별 정의로 연결한다.
set_property(SOURCE PlayerParticipationTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_ALLIANCE_PACKET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-alliance-packet-x86.tsv\"")

# 독립 비용 표시 원문은 이 소스에만 경로를 공급한다.
target_sources(netstorm_tests PRIVATE FloatingSpGumpTests.cpp)
set_source_files_properties(FloatingSpGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FLOATING_SP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/floating-sp-x86.tsv\"")

# Factory의 실제 가상 그림/전체 RLE 원문은 해당 캡처 검사에만 공급한다.
set_property(SOURCE ShapeToBufferTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_FACTORY_CAPTURE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factory-capture-x86.tsv\"")

# 일반 자산의 독립 원본 세 색표를 이 검사 소스에만 연결한다.
target_sources(netstorm_tests PRIVATE SpriteColorMapsTests.cpp)
set_source_files_properties(SpriteColorMapsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SPRITE_COLORS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sprite-colors-x86.tsv\"")

# Factory 전체 메뉴와 마나 문자열의 독립 원문을 이 검사 소스에만 연결한다.
target_sources(netstorm_tests PRIVATE FactoryMenuTests.cpp)
set_source_files_properties(FactoryMenuTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FACTORY_MENU_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factory-menu-x86.tsv\"")

# 원본 연구216행을 새 몸체와 대조하며 기대값을 생성하지 않는다.
target_sources(netstorm_tests PRIVATE FactoryResearchTests.cpp)
set_source_files_properties(FactoryResearchTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FACTORY_RESEARCH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/factory-research-x86.tsv\"")

# 독립 플레어139행의 전체 슬롯·난수·효과·반환을 대조한다.
target_sources(netstorm_tests PRIVATE FlareTests.cpp)
set_source_files_properties(FlareTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FLARE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/flare-x86.tsv\"")

# 독립 원본98행의 전체 생산 내용물 동기화 호출을 대조한다.
target_sources(netstorm_tests PRIVATE ProductionSyncTests.cpp)
set_source_files_properties(ProductionSyncTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRODUCTION_SYNC_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/production-sync-x86.tsv\"")

# 독립 원본170행의 격자/항목 전체 바이트와 실제 공통 트리 수명을 대조한다.
target_sources(netstorm_tests PRIVATE InventoryGumpTests.cpp)
set_source_files_properties(InventoryGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_INVENTORY_GRID_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/inventory-grid-x86.tsv\"")

# 이미지 Gump의 독립 원본221행과 실제 자산 픽셀/적중을 대조한다.
target_sources(netstorm_tests PRIVATE ImageGumpTests.cpp)
set_source_files_properties(ImageGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_INVENTORY_IMAGE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/inventory-image-x86.tsv\"")

# 독립 원본132행과 실제 패턴·그림 자식·클릭 SID를 대조한다.
target_sources(netstorm_tests PRIVATE InventoryItemGumpTests.cpp)
set_source_files_properties(InventoryItemGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_INVENTORY_ITEM_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/inventory-item-x86.tsv\"")

# 실제 생산 아이콘 두 색표와 생성 직후 격자 부모0을 검사한다.
target_sources(netstorm_tests PRIVATE ProductionColorMapsTests.cpp)
set_source_files_properties(ProductionColorMapsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRODUCTION_COLORS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/production-colors-x86.tsv\"")

# 독립 원본86입력의 전체 생산 상태와 효과 순서를 검사한다.
target_sources(netstorm_tests PRIVATE ProductionMaintenanceTests.cpp)
set_source_files_properties(ProductionMaintenanceTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRODUCTION_IDLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/production-idle-x86.tsv\"")

# 실제0/0 커서 미리보기의 원본 발자국 경계 관측도 같은 검사에 연결한다.
set_property(SOURCE ProductionMaintenanceTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_NEIGHBOR_BOUNDARY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/neighbor-boundary-x86.tsv\"")

# 다리 wrapper와 보호60프레임/9색 캡처의 독립 원본 전체를 대조한다.
target_sources(netstorm_tests PRIVATE BridgeCollapseTests.cpp)
set_source_files_properties(BridgeCollapseTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BRIDGE_COLLAPSE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/bridge-collapse-x86.tsv\"")

# 실제 extra2의 가상 기본/추가 그림과 전체600개 캡처 원문도 같은 검사에 공급한다.
set_property(SOURCE BridgeCollapseTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_BRIDGE_EXTRA_CAPTURE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/bridge-extra-capture-x86.tsv\"")

# 동결한 실제 생산 실패 안내126입력을 대조한다.
target_sources(netstorm_tests PRIVATE ProductionHintGumpTests.cpp)
set_source_files_properties(ProductionHintGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRODUCTION_HINT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/production-hint-x86.tsv\"")

# 두 독립 원본의 생산 정보3888입력을 대조한다.
target_sources(netstorm_tests PRIVATE InventoryAboutMenuTests.cpp)
set_source_files_properties(InventoryAboutMenuTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_INVENTORY_ABOUT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/inventory-about-x86.tsv\";NETSTORM_INVENTORY_ABOUT_CLASS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/inventory-about-class-x86.tsv\"")

# 새 C++ 전 동결한 실제 Quick Help 요청985행을 대조한다.
target_sources(netstorm_tests PRIVATE QuickHelpRequestTests.cpp)
target_sources(netstorm_tests PRIVATE QuickHelpControlTests.cpp)
target_sources(netstorm_tests PRIVATE QuickHelpHtmlTests.cpp)
target_sources(netstorm_tests PRIVATE QuickHelpWindowTests.cpp)
set_source_files_properties(QuickHelpWindowTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_QUICK_HELP_WINDOW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/quick-help-window-x86.tsv\";NETSTORM_QUICK_HELP_WINDOW_PATHS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/quick-help-window-paths-x86.tsv\"")
set_source_files_properties(QuickHelpHtmlTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_QUICK_HELP_HTML_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/quick-help-html-x86.tsv\";NETSTORM_QUICK_HELP_HTML_NESTED_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/quick-help-html-nested-x86.tsv\"")
set_source_files_properties(QuickHelpControlTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_QUICK_HELP_CONTROL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/quick-help-control-x86.tsv\"")
set_source_files_properties(QuickHelpRequestTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_QUICK_HELP_REQUEST_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/quick-help-request-x86.tsv\"")

# 새 C++ 전에 동결한 실제 신전/윈드 타워432입력을 현재 같은 월드 자료로 대조한다.
target_sources(netstorm_tests PRIVATE SpecialDamageTests.cpp)
set_source_files_properties(SpecialDamageTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SPECIAL_DAMAGE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/special-damage-x86.tsv\"")

# 훨리기그의 실제136바이트 생성/홈/소리/그림자391입력을 대조한다.
target_sources(netstorm_tests PRIVATE SunFlyerPostPopTests.cpp)
set_source_files_properties(SunFlyerPostPopTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SUN_FLYER_POSTPOP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sun-flyer-postpop-x86.tsv\"")

# 제품 전 동결한 전용 비행/폭탄/통지/장식/종료/빈 표적410행이다.
target_sources(netstorm_tests PRIVATE SunFlyerFrameTests.cpp)
set_source_files_properties(SunFlyerFrameTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SUN_FLYER_FRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sun-flyer-frame-x86.tsv\"")

# 템플 기본 프레임/전용Draw의 제품 전 실제264입력을 대조한다.
target_sources(netstorm_tests PRIVATE TempleDrawTests.cpp)
target_compile_definitions(netstorm_tests PRIVATE
    "NETSTORM_TEMPLE_DRAW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/temple-draw-x86.tsv\"")

# 실제 Session 파편 이미지 전체를 새 원본 캡처와 대조한다. 다른 검사 정의는 바꾸지 않는다.
set_property(SOURCE CampaignSessionTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_TEMPLE_CAPTURE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/temple-capture-x86.tsv\"")

# 실제83VT의 공통 Handle/HP 분배 데이터를 새 통합 검사에만 공급한다.
set_property(SOURCE CampaignSessionTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_DAMAGEABLE_REGULAR_TABLES_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/damageable-regular-tables-x86.tsv\"")

# 세 제네레이터의 실제 전체 캡처180입력은 해당 세션 검사에만 공급한다.
set_property(SOURCE CampaignSessionTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_GENERATOR_CAPTURE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/generator-capture-x86.tsv\"")

# 전진 공략의 수동 UI/생산 명령 입력이다. 원본 함수 기대값 fixture와 구별한다.
set_property(SOURCE CampaignSessionTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_FORWARD_BATTLE_PLAN=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/campaign-forward-battle-plan.tsv\"")

# 타워의 실제 전체 캡처120입력을 같은 세션의 생성 결과와 대조한다.
set_property(SOURCE CampaignSessionTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_BLOCKER_CAPTURE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/blocker-capture-x86.tsv\"")

# 사제 희생 wrapper의 독립128입력은 이 검사에서만 읽는다.
target_sources(netstorm_tests PRIVATE PriestSacrificeRequestTests.cpp)
set_source_files_properties(PriestSacrificeRequestTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIEST_SACRIFICE_REQUEST_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priest-sacrifice-request-x86.tsv\"")

# 실제 보호 SHP의 의식 목표점 원문을 같은 Session 조회 검사에서만 읽는다.
set_property(SOURCE CampaignSessionTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_RITUAL_SHAPE_OFFSETS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ritual-shape-offsets-x86.tsv\"")

# 사제604의 독립 원본1050입력을 이 검사에서만 읽는다.
target_sources(netstorm_tests PRIVATE PriestRitualFrameTests.cpp)
set_source_files_properties(PriestRitualFrameTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PRIEST_RITUAL_FRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/priest-ritual-frame-x86.tsv\"")

# 독립 원본의 회수 요청/54h 송수신3458입력을 같은 실제 몸체와 대조한다.
target_sources(netstorm_tests PRIVATE SalvageCommandTests.cpp)
set_source_files_properties(SalvageCommandTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SALVAGE_COMMAND_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/salvage-command-x86.tsv\"")

# 다이스 별도 TYPE의 독립 원본768입력과 미연결 문맥을 검사한다.
target_sources(netstorm_tests PRIVATE DaisDrawTests.cpp)
set_source_files_properties(DaisDrawTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DAIS_DRAW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dais-draw-x86.tsv\"")

# Dude 정지/도착/지면/삭제 후처리의 독립 원본410입력이다.
target_sources(netstorm_tests PRIVATE DudeBehaviorTests.cpp)
set_source_files_properties(DudeBehaviorTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DUDE_BEHAVIOR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dude-behavior-x86.tsv\"")

# Dude24 생성자의 독립 원문과 실제 공유 가상 슬롯을 읽는다.
target_sources(netstorm_tests PRIVATE DudePathTests.cpp)
set_source_files_properties(DudePathTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DUDE_PATH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dude-path-x86.tsv\";NETSTORM_DUDE_PATH_VTABLES=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dude-path-vtables.tsv\"")

# 발자국0의 독립 일반 탐색480입력과 실제 경로 실패640입력을 공급한다.
target_sources(netstorm_tests PRIVATE DudeFootprintTests.cpp)
set_source_files_properties(DudeFootprintTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DUDE_FOOTPRINT_FIND=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dude-footprint-find-x86.tsv\";NETSTORM_DUDE_FOOTPRINT_FAIL=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dude-footprint-fail-x86.tsv\"")

# Dude 정기/하위 처리1597개와 정적 초기화3개의 독립 원문이다.
target_sources(netstorm_tests PRIVATE DudeRegularTests.cpp)
set_source_files_properties(DudeRegularTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DUDE_REGULAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dude-regular-x86.tsv\"")
# First/현재 안내 함수와 제한된 F8 내부 분기의 최초 원문만 새 검사에 공급한다.
set_source_files_properties(MissionGuideInputTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_FIRST_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-first-x86.tsv\";NETSTORM_MISSION_COMMON_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-common-x86.tsv\"")
# 실제 Abort 사건과 세 제어 이름/토큰/큐의 최초 원문을 새 검사에서만 읽는다.
set_source_files_properties(MissionControlTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_ABORT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-abort-x86.tsv\"")
# 실제 Guide/얇은 단계/진입 Pause의 최초 전체 상태를 새 수명 검사에서 읽는다.
set_source_files_properties(MissionLifecycleTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_LIFECYCLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-lifecycle-x86.tsv\"")
# 실제 Tutorial Init 전체 제어/정규화/옵션과 큰 효과 순서의 최초 원문이다.
set_source_files_properties(TutorialInitTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TUTORIAL_INIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tutorial-init-x86.tsv\"")
# 실제004b51e0의 전체144BYTE/Zone512·복제86BYTE 원문을 새 요청 검사에만 공급한다.
set_source_files_properties(MissionFortRequestTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_FORT_REQUEST_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-fort-request-x86.tsv\"")
# 실제 Template9 정리/단일 반납/초기화의 독립240개 전체 헤더·경계 원문이다.
set_source_files_properties(TemplateLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TEMPLATE_LIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/template-lifetime-v2-x86.tsv\"")
# 실제 Guide 교체와 새 제어 이름의 독립 원문을 이 검사에만 공급한다.
set_source_files_properties(GuideTransitionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GUIDE_TRANSITIONS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/guide-transitions-x86.tsv\"")
# 실제 미션 기술 허용 목록/표 쓰기의 최초 원문을 이 검사에만 공급한다.
set_source_files_properties(MissionTechnologyTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_MISSION_TECHNOLOGY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mission-technology-x86.tsv\"")
# 원본 전체 지식 창25입력의 최초 결과를 이 검사에만 공급한다.
set_source_files_properties(KnowledgeMenuTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_KNOWLEDGE_MENU_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/knowledge-menu-x86.tsv\"")
# 실제 DataManager/섹션 소멸의 독립392입력과 저장소 반납을 컴파일한다.
target_sources(netstorm_tests PRIVATE DataManagerLifetimeTests.cpp)
set_source_files_properties(DataManagerLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DATAMANAGER_LIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/datamanager-lifetime-x86.tsv\"")

# 현재 DataManager 검색/선택의 최초864입력과 실제 Player State 선택을 컴파일한다.
target_sources(netstorm_tests PRIVATE DataManagerSelectionTests.cpp)
set_source_files_properties(DataManagerSelectionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DATAMANAGER_SELECTION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/datamanager-selection-x86.tsv\"")

# 빈 State의 원본 부모2560입력과 실제 섹션 재생성/버퍼 공유를 컴파일한다.
target_sources(netstorm_tests PRIVATE PlayerStateInitializationTests.cpp)
set_source_files_properties(PlayerStateInitializationTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_STATE_INITIALIZATION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-state-initialization-x86.tsv\"")

# 중첩 하위까지 실행한 MemStream/DataManager의 최초404행을 같은 공통 몸체에 대조한다.
target_sources(netstorm_tests PRIVATE DataManagerStreamsTests.cpp)
set_source_files_properties(DataManagerStreamsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DATAMANAGER_STREAMS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/datamanager-streams-x86.tsv\"")

# 원본 최소 확장/생성/이름 교체와 모든 하위를 관측한 최초286행을 대조한다.
target_sources(netstorm_tests PRIVATE DataManagerResizeTests.cpp)
set_source_files_properties(DataManagerResizeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DATAMANAGER_RESIZE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/datamanager-resize-x86.tsv\"")

# 생성/복제·대입 부모와 전체 실제16384BYTE의 최초220행을 컴파일한다.
target_sources(netstorm_tests PRIVATE DataManagerCreateTests.cpp)
set_source_files_properties(DataManagerCreateTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DATAMANAGER_CREATE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/datamanager-create-x86.tsv\"")

# 파일 이미지의 최초144행과 목표11파일의 실제 새 목록 연결을 컴파일한다.
target_sources(netstorm_tests PRIVATE DataManagerReadTests.cpp)
set_source_files_properties(DataManagerReadTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DATAMANAGER_READ_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/datamanager-read-x86.tsv\"")
# 원본 단독/편집 맵 목록의 전체캐시와행표시를독립관측과대조한다.
target_sources(netstorm_tests PRIVATE SoloMapListTests.cpp)
# 실제 디스크/TAFF Fort와 같은 Template/State 수명의 목록 연결을 검사한다.
target_sources(netstorm_tests PRIVATE SoloMapCatalogTests.cpp)
# Tell 맵 목록 부모의 실제 기본블록 원문과 현재 메타 호출 시점을 대조한다.
target_sources(netstorm_tests PRIVATE FortSelectionTests.cpp)
# TypeNames/Deck snapshot의 독립 기계어와 실제 명시 stream 수명을 확인한다.
target_sources(netstorm_tests PRIVATE FortSnapshotBasicsTests.cpp)
set_source_files_properties(FortSnapshotBasicsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_SNAPSHOT_BASICS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-snapshot-basics-x86.tsv\"")
set_source_files_properties(FortSelectionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_SELECTION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-selection-x86.tsv\"")
target_sources(netstorm_tests PRIVATE TellMapMenusTests.cpp)
set_source_files_properties(TellMapMenusTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TELL_MAP_MENUS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tell-map-menus-x86.tsv\"")
# 실제 저장 부모의 현재 관문과 두 가상 파일명 조회를 독립 관측에 대조한다.
target_sources(netstorm_tests PRIVATE DataManagerFileSaveTests.cpp)
# 실제 작업본의 Notify/직렬화/파일 쓰기/재읽기와 원본 보호를 확인한다.
target_sources(netstorm_tests PRIVATE EditorFilesTests.cpp)
# 데빌메이커 전용80BYTE 생성/송수신을 독립 원본 관찰로 대조한다.
target_sources(netstorm_tests PRIVATE WindAviaryStateTests.cpp)
# 원본 생산 상태의 독립1639행을 대조한다.
target_sources(netstorm_tests PRIVATE WindAviaryProcessTests.cpp)
set_source_files_properties(WindAviaryProcessTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_WIND_AVIARY_FRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/wind-aviary-frame-x86.tsv\"")
# 클라우드 플로터 전용 후처리/Regular의 독립88행을 대조한다.
target_sources(netstorm_tests PRIVATE CloudFloaterTests.cpp)
# 더스트 데빌의334입력원본관측과동일공간해시를대조한다.
target_sources(netstorm_tests PRIVATE WindFlyerTests.cpp)
set_source_files_properties(CloudFloaterTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CLOUD_FLOATER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cloud-floater-x86.tsv\"")
set_source_files_properties(DataManagerFileSaveTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DATAMANAGER_FILE_SAVE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/datamanager-file-save-x86.tsv\"")

# Walker38의 실제 전체 프레임과 자기 삭제 경계를 검증한다.
target_sources(netstorm_tests PRIVATE WalkerPathTests.cpp)

# 아크 스파이어 다섯 몸체와 정수 근처 경계의 독립 원문을 대조한다.
target_sources(netstorm_tests PRIVATE ArcSpireTests.cpp)
# 원본 분해의84바이트 상태와 같은 장치의 전체 픽셀을 대조한다.
target_sources(netstorm_tests PRIVATE DissolveFrameTests.cpp)
set_source_files_properties(DissolveFrameTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DISSOLVE_FRAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dissolve-frame-x86.tsv\"")
# 원본 생성/등록/캡처/반납과 콜백 뒤 현재 부모를 대조한다.
target_sources(netstorm_tests PRIVATE DissolveLifetimeTests.cpp)
target_sources(netstorm_tests PRIVATE DissolveVisualSystemTests.cpp)
# 애시드 포획 수신/헤더의 최초1017행을 비트 그대로 대조한다.
target_sources(netstorm_tests PRIVATE AcidZapTests.cpp)
target_sources(netstorm_tests PRIVATE AcidFenceTests.cpp)
target_sources(netstorm_tests PRIVATE IceTowerTests.cpp)
target_sources(netstorm_tests PRIVATE BulwarkHitTests.cpp)
target_sources(netstorm_tests PRIVATE WindAviaryDestroyTests.cpp)
target_sources(netstorm_tests PRIVATE BombEffectTimerTests.cpp)
target_sources(netstorm_tests PRIVATE PriestPrayerTests.cpp)
target_sources(netstorm_tests PRIVATE BombTimerPacketTests.cpp)
target_sources(netstorm_tests PRIVATE BombStatusTests.cpp)
target_sources(netstorm_tests PRIVATE WindSpellTests.cpp)
set_source_files_properties(AcidZapTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_ACID_ZAP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/acid-zap-x86.tsv\"")
# 훨리기그의기존원문410행과별도전체DWORD경계30행을같이사용한다.
set_property(SOURCE SunFlyerFrameTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_SUN_FLYER_CLEAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sun-flyer-clear-x86.tsv\"")
set_source_files_properties(DissolveLifetimeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DISSOLVE_LIFETIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dissolve-lifetime-x86.tsv\"")

target_sources(netstorm_tests PRIVATE PriestPanicTests.cpp)

# 원문 최초 관측과 실제 Client 소유자를 구별해 신규34검사를 같은 실행 파일에 등록한다.
target_sources(netstorm_tests PRIVATE ColorSortFixedInputTests.cpp StateWorldCleanupTests.cpp
    SavedStateReadTests.cpp DotCommandRegisterTests.cpp ClientDotCommandGlobalsTests.cpp
    FortSpWriteTests.cpp PlayerCoreRefreshTests.cpp ColorMapCacheTests.cpp
    ClientChatCoreOwnersTests.cpp ClientFortCoreSeedTests.cpp ClientDotCommandOwnerTests.cpp)
# 각 원문 검사에는 최초 fixture의 실제 절대 경로만 전달한다.
set_source_files_properties(ColorSortFixedInputTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_COLORSORT_FIXED_INPUT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/colorsort-fixed-input-x86.tsv\"")
set_source_files_properties(StateWorldCleanupTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_STATE_WORLD_CLEANUP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/state-world-cleanup-x86.tsv\"")
set_source_files_properties(SavedStateReadTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SAVED_STATE_READ_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/saved-state-read-x86.tsv\"")
set_source_files_properties(DotCommandRegisterTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DOT_COMMAND_REGISTER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dot-command-register-x86.tsv\";NETSTORM_DOT_COMMAND_REGISTER_V2_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dot-command-register-v2-x86.tsv\"")
set_source_files_properties(ClientDotCommandGlobalsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DOT_COMMAND_STATIC_IMAGE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dot-command-static-image.hex\"")
set_source_files_properties(FortSpWriteTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_FORT_SP_WRITE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-sp-write-x86.tsv\"")
set_source_files_properties(PlayerCoreRefreshTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_CORE_REFRESH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-core-refresh-x86.tsv\";NETSTORM_PLAYER_CORE_REFRESH_GUARD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-core-refresh-guard-x86.tsv\"")
set_source_files_properties(ColorMapCacheTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_COLOR_MAP_CACHE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/colorsort-cache-parent-x86.tsv\"")

# 세션 전환의 개별 원문과 실제 장기 소유자·커서 서비스에 신규22검사를 등록한다.
target_sources(netstorm_tests PRIVATE GameClockResetTests.cpp ClockSyncResetTests.cpp
    PlayerCoreRecordsTests.cpp ClientCoreRecordsStorageTests.cpp PlayerLinksCleanupTests.cpp
    MetaZoneCleanupTests.cpp ProductionDeckReleaseTests.cpp SessionCursorColorInputTests.cpp)
# 최초 Native 자료를 새로 작성하지 않고 공용 검사에 같은 경로를 전달한다.
set_source_files_properties(GameClockResetTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_GAME_CLOCK_RESET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/game-clock-reset-x86.tsv\"")
set_source_files_properties(ClockSyncResetTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CLOCK_SYNC_RESET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/clock-sync-reset-x86.tsv\"")
set_source_files_properties(PlayerCoreRecordsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_CORE_RECORDS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-core-records-x86.tsv\"")
set_source_files_properties(PlayerLinksCleanupTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_LINKS_CLEANUP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-links-cleanup-x86.tsv\"")
set_source_files_properties(MetaZoneCleanupTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_META_ZONE_CLEANUP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/meta-zone-cleanup-x86.tsv\"")
# 기술 비트의 원문 숫자 변환과 보호 TYPE 전체188필드를 별도2검사로 대조한다.
target_sources(netstorm_tests PRIVATE TypeTechBitTests.cpp)
set_source_files_properties(TypeTechBitTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TYPE_TECH_BIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/type-tech-bit-x86.tsv\";NETSTORM_TYPE_TECH_BIT_ASSETS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/type-tech-bit-assets-x86.tsv\";NETSTORM_TYPE_TECH_BIT_ORIGINALS=\"${CMAKE_CURRENT_SOURCE_DIR}/../../originals\"")

# 같은 Core/Subscriber/색상 소유자의 실제 접합과 보존된 원문 관측을 함께 검사한다.
target_sources(netstorm_tests PRIVATE SessionPlayerCoreRefreshTests.cpp ClientLocalStateGlobalsTests.cpp SubscriberGlobalsTests.cpp
    ClientSubscriberGlobalsTests.cpp ColorSortPrimaryTests.cpp ClientColorSortPrimaryTests.cpp)
# 원본 State 부모가 같은 실제 Client 전역을 읽고 쓰는 주소·폭·재조회 경계를 검사한다.
target_sources(netstorm_tests PRIVATE ClientStateWorldGlobalsTests.cpp)
set_source_files_properties(SubscriberGlobalsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SUBSCRIBER_LIFETIME_EDGES_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/subscriber-lifetime-edges-v2-x86.tsv\"")
set_source_files_properties(ColorSortPrimaryTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_COLORSORT_PRIMARY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/colorsort-primary-x86.tsv\"")
# State 정리의 점유 Chat 행 전체 반납을 최초216행의 독립 PE 관측과 대조한다.
target_sources(netstorm_tests PRIVATE ChatStateClearTests.cpp)
set_source_files_properties(ChatStateClearTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CHAT_STATE_CLEAR_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-state-clear-x86.tsv\"")
# 원문202행의 전체 물리/순서와 실제 Core 선택 해제 후 재할당 수명을 대조한다.
target_sources(netstorm_tests PRIVATE PlayerInitializationTests.cpp)
# 실제 Client Player/factory 소유자와 raw 헤더/재대여/초기화 실패를 검사한다.
target_sources(netstorm_tests PRIVATE ClientPlayerRecordsTests.cpp)
# 이전 중단 자료의 Native310/170행과 실제 전체 자식·공통 수명/그리기를 등록한다.
target_sources(netstorm_tests PRIVATE HudNsInfoTests.cpp HudNsInfoCreateTests.cpp NsInfoClientTests.cpp)
# 준비 자료의 실제 Chat/Console/편집 버퍼/부착 Native 관측을 공통 검사에 등록한다.
target_sources(netstorm_tests PRIVATE ChatWindowTests.cpp HudChatGeometryTests.cpp TextEntryGumpTests.cpp TextEntryBufferTests.cpp)
# 실제 물리/공통 등록표/글꼴·픽셀/교체 수명의 결합을 별도로 검사한다.
target_sources(netstorm_tests PRIVATE TextEntryClientTests.cpp)
# 실제 Client 전역과 전체 Chat 공장/공유 로그/메뉴/픽셀/동시 수명을 검사한다.
target_sources(netstorm_tests PRIVATE ChatWindowClientTests.cpp)
target_sources(netstorm_tests PRIVATE ChatInputTests.cpp) #Enter/전환/프레임말 원문378행의독립관측을검사한다.
target_sources(netstorm_tests PRIVATE ChatNameCompletionTests.cpp) #원문자동완성189행의물리상태와순환을검사한다.
set_source_files_properties(ChatNameCompletionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CHAT_NAME_COMPLETION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-completion-x86.tsv\"")
set_source_files_properties(ChatInputTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CHAT_INPUT_KEY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-input-key-x86.tsv\";NETSTORM_CHAT_INPUT_TOGGLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-input-toggle-x86.tsv\"")
target_sources(netstorm_tests PRIVATE ChatRecipientsTests.cpp) # 원문 이름 검사 부모의 실제350관측을 독립 대조한다.
set_source_files_properties(ChatRecipientsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CHAT_RECIPIENTS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-recipients-x86.tsv\";NETSTORM_CHAT_RECIPIENT_IDENTITY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-recipient-identity-x86.tsv\"")
set_source_files_properties(ChatWindowClientTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CHAT_CALLBACKS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-callbacks-x86.tsv\";NETSTORM_CHAT_INPUT_RECIPIENTS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-input-recipients-x86.tsv\"")
set_source_files_properties(ChatWindowTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_CHAT_WINDOW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-window-x86.tsv\"")
set_source_files_properties(HudChatGeometryTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_HUD_CHAT_GEOMETRY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/hud-chat-geometry-x86.tsv\"")
set_source_files_properties(TextEntryGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TEXT_ENTRY_GUMP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/text-entry-gump-x86.tsv\"")
set_source_files_properties(TextEntryBufferTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TEXT_ENTRY_BUFFER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/text-entry-buffer-x86.tsv\"")
set_source_files_properties(HudNsInfoTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_HUD_NSINFO_ATTACH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/hud-nsinfo-attach-x86.tsv\"")
set_source_files_properties(HudNsInfoCreateTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_HUD_NSINFO_CREATE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/hud-nsinfo-create-x86.tsv\"")
# Session 없는 실제 생산 생성/공통 수명과 현재 전역 대여를 검사한다.
target_sources(netstorm_tests PRIVATE ProductionInitialContextTests.cpp)
set_source_files_properties(PlayerInitializationTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_PLAYER_INITIALIZATION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/player-initialization-x86.tsv\"")

# 원문 GIF 창308행과 실제 보호 GIF 전체/공통 입력·픽셀·수명을 같은 검사기에 등록한다.
target_sources(netstorm_tests PRIVATE BackgroundGifGumpTests.cpp BackgroundGifClientTests.cpp)
set_source_files_properties(BackgroundGifGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BACKGROUND_GIF_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/background-gif-gump-x86.tsv\"")
set_source_files_properties(BackgroundGifClientTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BACKGROUND_GIF_ASSETS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/background-gif-assets-x86.tsv\";NETSTORM_BACKGROUND_GIF_VARIANTS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/background-gif-variants-x86.tsv\"")

# HTML 부모의 독립 원문250행과 실제 Client/제어/본문/공통 수명을 등록한다.
target_sources(netstorm_tests PRIVATE HtmlWindowGumpTests.cpp HtmlWindowClientTests.cpp)
set_source_files_properties(HtmlWindowGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_HTML_WINDOW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/html-window-gump-x86.tsv\";NETSTORM_HTML_WINDOW_IDLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/html-window-idle-v2-x86.tsv\";NETSTORM_HTML_WINDOW_ROWS=338")

# 원문 전체 Fit/_ftol160행의 실제 객체/메뉴/장치 순서를 대조한다.
target_sources(netstorm_tests PRIVATE HtmlWindowFitTests.cpp)
target_sources(netstorm_tests PRIVATE HtmlWindowFitClientTests.cpp) # 실제 글꼴/메뉴/픽셀·교체 결합을 확인한다.
set_source_files_properties(HtmlWindowFitTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_HTML_WINDOW_FIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/html-window-fit-x86.tsv\"")

# 실제 원문 전체 메타128행의 물리 버퍼/현재 필드/장치 순서를 대조한다.
target_sources(netstorm_tests PRIVATE HtmlWindowMetadataTests.cpp)
target_sources(netstorm_tests PRIVATE HtmlWindowMetadataClientTests.cpp) # 실제 공통 UI/고리/타이머/보호 맵 목록 연결을 확인한다.

# 전체 HTML 공장106행의 현재 부모/전역·두 번 파서·빈 본문·모달/반환 순서를 등록한다.
target_sources(netstorm_tests PRIVATE HtmlWindowFactoryTests.cpp)
target_sources(netstorm_tests PRIVATE HtmlWindowFactoryClientTests.cpp) # 실제 Config/두 번 파서·링크/모달/긴 본문·픽셀 접합이다.
set_source_files_properties(HtmlWindowFactoryTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_HTML_FACTORY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/html-window-factory-x86.tsv\"")
set_source_files_properties(HtmlWindowMetadataTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_HTML_METADATA_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/html-window-metadata-x86.tsv\"")
target_sources(netstorm_tests PRIVATE TellSectionTests.cpp) # 현재 Tell 이름과 등록 객체·미션·새 tell 섹션 조회를 검증한다.
set_source_files_properties(TellSectionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TELL_SECTION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tell-section-x86.tsv\"")
target_sources(netstorm_tests PRIVATE TellFileMenusTests.cpp) # 실제 @/# 원문과 같은 Config 수명·고리/행을 확인한다.
target_sources(netstorm_tests PRIVATE FortCurrentNameDevicesTests.cpp) # 실제 Client 이름/목록 dirty·manager·Notify/저장·보호 파일 장치를 함께 확인한다.
target_sources(netstorm_tests PRIVATE ClientChatRuntimeTests.cpp) # 실제 Chat 시작/명령표/현재 설정/Core와 보호·현재 icon 개수를 함께 대조한다.
target_sources(netstorm_tests PRIVATE StaticStringsTests.cpp) # 원본204관측과 실제 Client 고리/번역/서식 수명을 확인한다.
target_sources(netstorm_tests PRIVATE ClientPlayerRuntimeTests.cpp) # 실제 공통 Player/장기 Core/재할당 선행과 초기화 기록 대여 수명을 확인한다.
target_sources(netstorm_tests PRIVATE ClientSpGlobalsTests.cpp) # 실제 SP48관측과 공통 시작/Session 반납 뒤의 같은 SP/난수/추적 수명이다.
target_sources(netstorm_tests PRIVATE ClientKencloudGlobalsTests.cpp) # 실제 구름 물리/VFS/옵션·부분 효과와 전체 부모 접합을 확인한다.
target_sources(netstorm_tests PRIVATE KencloudRenderTests.cpp) # 제품 전 고정 Native944행과 자연 생략/인접 alias/부분 효과다.
set_source_files_properties(KencloudRenderTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_KENCLOUD_RENDER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/kencloud-render-x86.tsv\"")
add_executable(netstorm_cloud_probe KencloudClientProbe.cpp) # 실제 GUI 장치 실행은 명시 CLI로만 하며 일반 CTest에 자동 실행하지 않는다.
netstorm_apply_compiler_options(netstorm_cloud_probe)
target_link_libraries(netstorm_cloud_probe PRIVATE netstorm)
add_executable(netstorm_cloud_render_probe KencloudRenderClientProbe.cpp) # 실제 Screen 픽셀 장치는 명시 실행하며 일반 CTest에서 창을 열지 않는다.
netstorm_apply_compiler_options(netstorm_cloud_render_probe)
target_link_libraries(netstorm_cloud_render_probe PRIVATE netstorm)
set_source_files_properties(ClientSpGlobalsTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_SP_INITIALIZATION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sp-initialization-x86.tsv\"")
set_source_files_properties(StaticStringsTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_STATIC_STRINGS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/static-string-runtime-x86.tsv\"")
set_source_files_properties(TellFileMenusTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_TELL_FILE_MENUS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tell-file-menus-x86.tsv\";NETSTORM_TELL_INLINE_LIST_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tell-inline-list-x86.tsv\";NETSTORM_TELL_INLINE_ERRORS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tell-inline-errors-x86.tsv\"")
# 실제 활동 수신/Client·Session 접합과 Client9 존재 수명을 서로 다른 검사에서 확인한다.
target_sources(netstorm_tests PRIVATE ConnectionActivityTests.cpp ConnectionActivitySessionTests.cpp ConnectionActivityTransportTests.cpp)

# 새 기대값은 보호 PE/x86 에뮬레이터의 독립 관측16행이다.
set_source_files_properties(CameraViewTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_CAMERA_VIEW_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/camera-view-x86.tsv\"")
target_sources(netstorm_tests PRIVATE CameraViewTests.cpp)

# Native 단일 Clear 잔류와 실제 Client/Session 교체의 Host·시각 수명을 검사한다.
target_sources(netstorm_tests PRIVATE ProcessHostStorageTests.cpp ProcessHostSessionTests.cpp)
#443의장기커서상태/DWORD폭/실제Cancel꼬리검사다.
target_sources(netstorm_tests PRIVATE ClientCursorGlobalsTests.cpp)
# 원문 PE에서 먼저 고정한 마커·Zone과 실제 Client 정리 저장소를 대조한다.
target_sources(netstorm_tests PRIVATE StateStorageCleanupTests.cpp)
# 원본 실제 Pool 부모·하위 Reset·삭제80DWORD와 장기 실패점을 대조한다.
target_sources(netstorm_tests PRIVATE SquidPoolResetTests.cpp)
#전체Deck명령부모·실제입력관문의독립CPU관측과실제Session연결을대조한다.
target_sources(netstorm_tests PRIVATE FullDeckCommandTests.cpp)

# 최초 Native66관측과 같은 장기 SID/count 소비자를 검증한다.
target_sources(netstorm_tests PRIVATE StateSidListsTests.cpp)

# SP 표시 부모의 최초 Native120관측과 실제 UI 대여 경계다.
target_sources(netstorm_tests PRIVATE StormPowerRefreshTests.cpp)
target_sources(netstorm_tests PRIVATE ConnectionCleanupTests.cpp) # 원문580관측과 실제 Client/Editor 반환을 검사한다.
target_sources(netstorm_tests PRIVATE ConnectionRegistrationTests.cpp) #원문288관측과 실제 장기 표/양끝/재배치를 검사한다.
target_sources(netstorm_tests PRIVATE MissionBackgroundLifetimeTests.cpp) #실제 같은 HTML 배경의 시작 첫BYTE와 연속 미션 보존을 검사한다.
target_sources(netstorm_tests PRIVATE StateBridgeResetTests.cpp) #Native64관측과 실제 World 앞/뒤의 현재 색 표·스캔 초기화를 검사한다.
target_sources(netstorm_tests PRIVATE RawBridgeDecayScanTests.cpp) #현재 주기/실제 슬롯/각 칸 catch를 독립 원문73행과 대조한다.
target_sources(netstorm_tests PRIVATE BridgeCarrierRetreatTests.cpp) #실제 finder/Graph와 중첩 대피의 원문258입력을 검증한다.
set_source_files_properties(BridgeCarrierRetreatTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_BRIDGE_CARRIER_RETREAT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/bridge-carrier-retreat-x86.tsv\"")
set_source_files_properties(RawBridgeDecayScanTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_BRIDGE_DECAY_SCAN_RUNTIME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/bridge-decay-scan-runtime-x86.tsv\"")

# 원문 두 PE의 섬 배치 검사와 현재 임시 지도 불변을 독립 fixture로 재생한다.
target_sources(netstorm_tests PRIVATE IslandPlacementTests.cpp)
set_source_files_properties(IslandPlacementTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_ISLAND_PLACEMENT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/island-placement-x86.tsv\"")

# 독립 원문30행과 전체 풀·공유 난수·현재 실제 Session 소비자를 대조한다.
target_sources(netstorm_tests PRIVATE RawBridgeEditorTests.cpp)
set_source_files_properties(RawBridgeEditorTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_EDITOR_BRIDGES_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/editor-bridges-x86.tsv\";NETSTORM_EDITOR_BRIDGE_ONE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/editor-bridge-one-x86.tsv\"")

# 원본 State3 분기와 실제 설정·편집 저장·모달 입력·장기 덱 경계를 같은 제품에 검증한다.
target_sources(netstorm_tests PRIVATE NativeMainMenuStateTests.cpp NativeMainMenuStateNativeTests.cpp ClientMainMenuGlobalsTests.cpp
    EditorSaveGoMainTests.cpp ClientProductionSyncTests.cpp SaveFortNameDialogTests.cpp TellTipTests.cpp)
target_sources(netstorm_tests PRIVATE SavedStateFilesTests.cpp) # 원문 오류 표와 한글 임시 파일/State3 정리의 실제 연결을 확인한다.
set_source_files_properties(SavedStateFilesTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_SAVED_STATE_FILE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/saved-state-file-x86.tsv\"")
set_source_files_properties(EditorSaveGoMainTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_EDITOR_SAVE_GO_MAIN_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/editor-save-go-main-x86.tsv\"")
set_source_files_properties(SaveFortNameDialogTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_SAVE_FORT_NAME_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/save-fort-name-x86.tsv\"")
set_source_files_properties(TellTipTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_TELL_TIP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tell-tip-x86.tsv\"")

# 작업본의 실제 파일 재조회/보호 경계와 독립 clipboard880관측을 같은 제품에서 검증한다.
target_sources(netstorm_tests PRIVATE EditorMapCatalogFilesTests.cpp TextClipboardTests.cpp)
target_sources(netstorm_tests PRIVATE EditorAtomicWriteTests.cpp)

# 보호 원본의 실제 토글 마커와 공통 메뉴 픽셀을 대조한다.
target_sources(netstorm_tests PRIVATE MenuCheckMarkerTests.cpp)

# 원문 NewFort1400행과 실제 manager/작업 파일의 보호·수명을 함께 검증한다.
target_sources(netstorm_tests PRIVATE NewFortTests.cpp NewFortManagerDevicesTests.cpp)
set_source_files_properties(NewFortTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_NEW_FORT_MANAGER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/new-fort-manager-x86.tsv\";NETSTORM_NEW_FORT_DATA_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/new-fort-data-x86.tsv\";NETSTORM_NEW_FORT_MISSING_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/new-fort-missing-x86.tsv\";NETSTORM_NEW_FORT_MISSING_BITS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/new-fort-missing-bits-x86.tsv\"")

# 독립 원본 Subscriber 90관측과 공유 이름표/빈 manager 76관측 및 실제 저장소 수명을 대조한다.
target_sources(netstorm_tests PRIVATE FortSubscriberWriteTests.cpp DataManagerInitializeTests.cpp)
set_source_files_properties(FortSubscriberWriteTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_FORT_SUBSCRIBER_WRITE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-subscriber-write-x86.tsv\"")
set_source_files_properties(DataManagerInitializeTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_DATA_MANAGER_INITIALIZE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/data-manager-initialize-x86.tsv\";NETSTORM_TEMPLATE_SECTION_NAMES_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/template-section-names-x86.tsv\"")
set_source_files_properties(TextClipboardTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_TEXT_CLIPBOARD_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/text-clipboard-x86.tsv\"")

# 독립 원본 population118행/drag40행과 실제 Editor 섬 배치·충돌 거부를 검증한다.
target_sources(netstorm_tests PRIVATE IslandPopulationTests.cpp EditorIslandDragTests.cpp)
set_source_files_properties(IslandPopulationTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_ISLAND_POPULATION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/island-population-x86.tsv\"")
set_source_files_properties(EditorIslandDragTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_EDITOR_ISLAND_DRAG_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/editor-island-drag-x86.tsv\"")

# 실제 보호 texture/RLE를 실행한 Native Bitmap28행과 같은 Client 장기 분류를 비교한다.
target_sources(netstorm_tests PRIVATE IslandBitmapTests.cpp)
set_source_files_properties(IslandBitmapTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_ISLAND_BITMAP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/island-bitmap-x86.tsv\";NETSTORM_ISLAND_BITMAP_PARENT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/island-bitmap-parent-x86.tsv\"")

# 섬 추가 중 수신자0 삭제 보상도 원본 float/ftol 비트로 처리하는지 독립 관측과 대조한다.
target_sources(netstorm_tests PRIVATE RewardSpBitsTests.cpp)
set_source_files_properties(RewardSpBitsTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_SP_ADD_BITS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sp-add-bits-x86.tsv\"")

# 원본 Image crop/실제 VFX 인코더/복사/이중 소멸의 전체 바이트를 재생한다.
target_sources(netstorm_tests PRIVATE EditorImageTests.cpp)
set_source_files_properties(EditorImageTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_EDITOR_IMAGE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/editor-image-x86.tsv\"")

# 원본 MetaImage326행과 실제 공통 트리/픽셀/비소유 수명을 검증한다.
target_sources(netstorm_tests PRIVATE MetaImageGumpTests.cpp)
set_source_files_properties(MetaImageGumpTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_META_IMAGE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/meta-image-x86.tsv\"")

# 실제 목록/MetaImage 자식의 전체 tail/기하/효과 순서를 재생한다.
target_sources(netstorm_tests PRIVATE MetaImageListTests.cpp)
set_source_files_properties(MetaImageListTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_META_IMAGE_LIST_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/meta-image-list-x86.tsv\"")

# 실제 섬 추가 창의 배치/드래그 격자·색상·효과 순서를 원문 관측과 비교한다.
target_sources(netstorm_tests PRIVATE EditorIslandLayoutTests.cpp)
set_source_files_properties(EditorIslandLayoutTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_EDITOR_ISLAND_LAYOUT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/editor-island-layout-x86.tsv\"")

# 원본 내장 설명의 조회/부모 설치와 실제 Config 수명을 재생한다.
target_sources(netstorm_tests PRIVATE EmbeddedTellTests.cpp)
set_source_files_properties(EmbeddedTellTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_EMBEDDED_TELL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/embedded-tell-x86.tsv\"")

# 실제 일반 입력/Guide174행과 편집 테스트 메뉴·부분 실패를 재생한다.
target_sources(netstorm_tests PRIVATE EditorTestBattleTests.cpp)
set_source_files_properties(EditorTestBattleTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_EDITOR_TEST_BATTLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/editor-test-battle-x86.tsv\"")

# 실제 작업 namespace/미션 source 수명과 언어 script/선택 Fort의 같은 읽기를 검증한다.
target_sources(netstorm_tests PRIVATE MissionFileSourceTests.cpp)

# 기존 Session helper를 빌리는 실제 로그인 작업본 재로드/소유 수명/실패 검사를 IDE에도 표시한다.
target_sources(netstorm_tests PRIVATE CampaignMissionReloadSourceTests.inc)

# 실제 Init/State의 이전 Session 반납과 새 Guide 요청 보존을 검사한다.
target_sources(netstorm_tests PRIVATE CampaignGuideWorldLifetimeTests.inc)

# 실제 미션 파일의 바이트/경로와 논리 이름을 같은 로그인에 전달하는지 검사한다.
target_sources(netstorm_tests PRIVATE CampaignMissionFortPathTests.inc)

# 보호PE에서 독립 관찰한 아이스80BYTE 발사와 공통 부모를 대조한다.
target_sources(netstorm_tests PRIVATE IceCannonFireTests.cpp)
target_compile_definitions(netstorm_tests PRIVATE NETSTORM_ICE_CANNON_FIRE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ice-cannon-fire-x86.tsv")

# 초기 아이스 투사체의 실제128BYTE/세 슬롯/포구·사정선과 파생 충돌 호출을 대조한다.
target_sources(netstorm_tests PRIVATE IceMissileInitialTests.cpp)
set_source_files_properties(IceMissileInitialTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_ICE_MISSILE_INITIAL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ice-missile-initial-x86.tsv\"")

# 원문 반사점/난수/선택/후속 인자를 파일 전용 정의로 대조한다.
target_sources(netstorm_tests PRIVATE IceRicochetTests.cpp)
set_source_files_properties(IceRicochetTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_ICE_RICOCHET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ice-ricochet-x86.tsv\"")

# 원본 반사탄의 전체 필드/수명·피해·효과 순서를 독립 행에 대조한다.
target_sources(netstorm_tests PRIVATE IceReflectedTests.cpp)
set_source_files_properties(IceReflectedTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_ICE_REFLECTED_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ice-reflected-x86.tsv\"")

# 원문 냉각 연기의 전체 생성·입자/프레임·색/선·반납을 파일 전용 정의로 대조한다.
target_sources(netstorm_tests PRIVATE SmokeEffectTests.cpp)
set_source_files_properties(SmokeEffectTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_SMOKE_EFFECT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/smoke-effect-x86.tsv\"")
target_sources(netstorm_tests PRIVATE SmokeVisualSystemTests.cpp) # 실제21 Host/공통 목록·자기 삭제·생성 실패 정리를 검증한다.

# 실제 두 판본의 섬 설명 자료와 원문 섹션/HTML을 파일 전용 정의로 대조한다.
target_sources(netstorm_tests PRIVATE EmbeddedIslandTextTests.cpp)
set_source_files_properties(EmbeddedIslandTextTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_EMBEDDED_ISLAND_TEXT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/embedded-island-text-x86.tsv\"")

target_sources(netstorm_tests PRIVATE ScreenPresentationTests.cpp) # Windows10/11 호환 화면비/실제 DIB 행과 입력 변환이다.

target_sources(netstorm_tests PRIVATE EdgeScrollTests.cpp) # 실제 원문 입력/FTOL/목표의 독립 관찰을 대조한다.
set_source_files_properties(EdgeScrollTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_EDGE_SCROLL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/edge-scroll-x86.tsv\"")

# 보호 원문 Present 말미/실제 FTOL의 독립 결과를 파일 전용 정의로 재생한다.
target_sources(netstorm_tests PRIVATE CameraPresentTests.cpp)
set_source_files_properties(CameraPresentTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_CAMERA_PRESENT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/camera-present-x86.tsv\"")

# 실제 카메라 적용의 두64BYTE·표·효과를 파일 전용 정의로 대조한다.
target_sources(netstorm_tests PRIVATE CameraCommitTests.cpp)
set_source_files_properties(CameraCommitTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_CAMERA_COMMIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/camera-commit-x86.tsv\"")

target_sources(netstorm_tests PRIVATE ScreenScrollTests.cpp) # 실제 DIB 겹침/행/padding과 Renderer 후처리 저장소를 검사한다.
set_source_files_properties(ScreenScrollTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_SCREEN_SCROLL_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/screen-scroll-x86.tsv\"") # 실제357회 원문 픽셀을 파일 전용 정의로 읽는다.

# 기존 Burning gold를 유지하고 투명 모서리의 두 판본 읽기 순서만 별도 관측에 대조한다.
target_sources(netstorm_tests PRIVATE BurningSourcesCornerTests.cpp)
set_source_files_properties(BurningSourcesCornerTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_BURNING_CORNER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/burning-sources-corner-x86.tsv\"")

# 기존 Path/FlyPath 정의를 유지하고 가상 ProcessFinder의 독립 관측만 추가한다.
set_property(SOURCE RawPathRuntimeTests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "NETSTORM_PROCESS_REMOVAL_FILTER_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/process-removal-filter-x86.tsv\"")

# 실제 타입 변경/비석·주문 입력의 독립 원문 전체바이트를 재생한다.
target_sources(netstorm_tests PRIVATE ObjectOwnerCommandsTests.cpp)
set_source_files_properties(ObjectOwnerCommandsTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_OBJECT_OWNER_COMMAND_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/object-owner-command-x86.tsv\"")
target_sources(netstorm_tests PRIVATE ObjectConversionTests.cpp)
set_source_files_properties(ObjectConversionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SQUID_RETYPE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/squid-retype-x86.tsv\";NETSTORM_OBJECT_CONVERSION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/object-conversion-x86.tsv\"")

# 독립 Meltdown 기계어의 전체 요청/피해/패킷을 제품에 재생한다.
target_sources(netstorm_tests PRIVATE MeltdownTests.cpp)
set_source_files_properties(MeltdownTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_MELTDOWN_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/meltdown-x86.tsv\"")

# 실제 일반 입력의 Detonate 미시전 경로와 부모0반환을 대조한다.
target_sources(netstorm_tests PRIVATE DetonateInputTests.cpp)
set_source_files_properties(DetonateInputTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_DETONATE_INPUT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/detonate-input-x86.tsv\"")
# 합성 후보의 원문1092조건과 가상 hit 호출 순서를 보존한다.
target_sources(netstorm_tests PRIVATE UserPickTests.cpp)
set_source_files_properties(UserPickTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_USER_PICK_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/user-pick-x86.tsv\"")

# 독립498000원문의고정캡처/실제슬롯호출순서를재생한다.
target_sources(netstorm_tests PRIVATE ObjectHitTests.cpp)
set_source_files_properties(ObjectHitTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_OBJECT_HIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/object-hit-x86.tsv\"")

# 비용 생략/0의 실제 두 판본 후처리를 기존 원문과 분리해 검증한다.
target_sources(netstorm_tests PRIVATE TypeDefaultCostTests.cpp)
set_source_files_properties(TypeDefaultCostTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_TYPE_DEFAULT_COST_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/type-default-cost-x86.tsv\"")

# 일반 F2/F3 표시 키의 독립310입력과 실제 큐 관측을 해당 검사에만 공급한다.
set_source_files_properties(ViewKeyInputTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_VIEW_KEY_INPUT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/view-key-input-x86.tsv\"")

# F8의 Guide/Gump/console 뒤558개 독립 입력과 현재 모드 읽기를 비교한다.
set_source_files_properties(F8KeyInputTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_F8_KEY_INPUT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/f8-key-input-x86.tsv\"")

# 배너 설정/최초 표 등록2066입력과 Shift 없는 F7의208입력은 각 독립 원문 관측이다.
set_source_files_properties(BannerOptionTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_BANNER_OPTION_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/banner-option-x86.tsv\";NETSTORM_BANNER_KEY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/banner-key-x86.tsv\"")

# 원본 송금 분기의 금액/현재 대상/AI 및 연결 플래그를 독립 관측한 입력이다.
set_source_files_properties(SendMoneyInputTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SEND_MONEY_INPUT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/send-money-input-x86.tsv\"")
# Shift F7 원문167관측 중 제품에 연결한135입력의 독립 fixture다.
set_source_files_properties(GifScreenshotTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_SCREENSHOT_KEY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/screenshot-key-x86.tsv\"")

# 독립 원문 프로세스 진단 바이트와 현재 슬롯/부모 재조회를 검증한다.
target_sources(netstorm_tests PRIVATE ProcessSummaryTests.cpp)
set_source_files_properties(ProcessSummaryTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_PROCESS_SUMMARY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/process-summary-x86.tsv\"")

# 객체 trace의 전체 순회/서식 기록은 원본 x86이 생성한 고정 기대값과 비교한다.
target_sources(netstorm_tests PRIVATE ObjectTraceTests.cpp)
set_source_files_properties(ObjectTraceTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_OBJECT_TRACE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/object-trace-x86.tsv\"")

# 객체 공통/내장Form의 서식 바이트와 파생 필드 재조회 시점을 원문으로 대조한다.
target_sources(netstorm_tests PRIVATE ObjectTraceFieldsTests.cpp)
set_source_files_properties(ObjectTraceFieldsTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_OBJECT_TRACE_FIELDS_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/object-trace-fields-x86.tsv\"")

# Main/Task의 원문 현재 필드/숫자 인자와87개 실제 가상 분배표를 대조한다.
target_sources(netstorm_tests PRIVATE ObjectTraceMainTests.cpp)

# 로그의필터/마크업/개행/현재공유상태와중첩호출을원문으로대조한다.
target_sources(netstorm_tests PRIVATE TraceLogTests.cpp)

# 실제객체덤프/파일후처리와typed EH범위 및격리디스크장치를대조한다.
target_sources(netstorm_tests PRIVATE ObjectDumpTests.cpp)
target_sources(netstorm_tests PRIVATE ThunderProcessTests.cpp) # 실제 번개 Form/부모/Kernel의 결합 수명을 검증한다.
target_sources(netstorm_tests PRIVATE ClientDiagnosticsTests.cpp) # 실제 원문 개발자 F7과 현재 물리/모드 참조를 검증한다.
target_sources(netstorm_tests PRIVATE ClientOptionsFileTests.cpp) # 기존 바이트를 유지하며 유니코드 설치 경로만 호환 저장한다.
set_source_files_properties(ClientDiagnosticsTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_SCREENSHOT_KEY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/screenshot-key-x86.tsv\"")
set_source_files_properties(ObjectDumpTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_OBJECT_DUMP_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/object-dump-x86.tsv\";NETSTORM_OBJECT_DUMP_WORK=\"${CMAKE_SOURCE_DIR}/../extracted/cpp-object-dump-558-20261029\"")
set_source_files_properties(TraceLogTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_TRACE_LOG_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/trace-log-x86.tsv\"")
target_sources(netstorm_tests PRIVATE NativeIntroStateTests.cpp) # 새 설치의 원문39~41/전체 상태 복사/메뉴 복귀를 검증한다.
target_sources(netstorm_tests PRIVATE TraceChatTests.cpp) # 진단→Chat의 원문 전체 문맥/현재자식/부분효과를 대조한다.

target_sources(netstorm_tests PRIVATE DotCommandDispatchTests.cpp) # 실제 분배/Regexp512행의 전체 표·입력·콜백 뒤 현재 상태를 대조한다.
set_source_files_properties(DotCommandDispatchTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_DOT_COMMAND_DISPATCH_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dot-command-dispatch-x86.tsv\"")

target_sources(netstorm_tests PRIVATE ChatSubmitTests.cpp) # 실제 제출/수신자 추가의 원문 전체 물리와 실제 소유자의 확장·반납을 대조한다.
set_source_files_properties(ChatSubmitTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_CHAT_SUBMIT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-submit-x86.tsv\"")
target_sources(netstorm_tests PRIVATE ChatFormatTests.cpp) # 서식 명령 원문372행의 전체 manager/공통 캡처와 부분 중단을 대조한다.
set_source_files_properties(ChatFormatTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_CHAT_FORMAT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-format-x86.tsv\"")
target_sources(netstorm_tests PRIVATE ChatMessageTests.cpp) # 일반 본문 조합 원문524행의 전체 manager/입력·지역·부분 중단을 대조한다.
target_sources(netstorm_tests PRIVATE ChatPacketTests.cpp) # 단독 송신/길이 observer 원문256행과 실제 수신자 확장/반납을 대조한다.
target_sources(netstorm_tests PRIVATE ChatReceiveTests.cpp) # 일반 수신 원문238행과 wrapper/장치 누락의 부분효과를 대조한다.
target_sources(netstorm_tests PRIVATE ChatRelayTests.cpp) # 단독 중계 원문132행의 전체 패킷/현재 마스크를 대조한다.
target_sources(netstorm_tests PRIVATE BattleLeaveTests.cpp) # 원문16행과 실제 로컬52h 송수신/다음State3 예약을 대조한다.
target_sources(netstorm_tests PRIVATE SmackerDecoderTests.cpp) # 실제 CD 디코더의 규격/프레임/실패/한글 경로를 확인한다.
target_sources(netstorm_tests PRIVATE MoviePlaybackTests.cpp) # 실제 원문 영상 입력/팔레트/idle의 최초 관측을 대조한다.
set_source_files_properties(BattleLeaveTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_BATTLE_LEAVE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/battle-leave-x86.tsv\";NETSTORM_BATTLE_ABORT_STATE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/battle-abort-state-x86.tsv\"")
set_source_files_properties(ChatRelayTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_CHAT_RELAY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-relay-x86.tsv\"")
set_source_files_properties(ChatReceiveTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_CHAT_RECEIVE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-receive-x86.tsv\"")
set_source_files_properties(ChatPacketTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_CHAT_PACKET_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-packet-x86.tsv\"")
set_source_files_properties(ChatMessageTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_CHAT_MESSAGE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/chat-message-x86.tsv\"")
set_source_files_properties(TraceChatTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_TRACE_CHAT_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/trace-chat-x86.tsv\"")
set_source_files_properties(NativeIntroStateTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_NATIVE_INTRO_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/native-intro-state-x86.tsv\"")
set_source_files_properties(ObjectTraceMainTests.cpp PROPERTIES COMPILE_DEFINITIONS "NETSTORM_OBJECT_TRACE_MAIN_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/object-trace-main-x86.tsv\";NETSTORM_OBJECT_TRACE_VTABLE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/object-trace-vtables-x86.tsv\";NETSTORM_OBJECT_TRACE_COORDINATE_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/object-trace-coordinate-x86.tsv\"")
