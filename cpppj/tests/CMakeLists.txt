# 단위 테스트. 외부 테스트 프레임워크 없이 TestSupport.h 의 작은 도구만 쓴다
# (첫 구성 때 인터넷에서 내려받을 것이 없다).
# 테스트 파일을 만들면 아래 목록에 더한다. 파일 안의 TEST_CASE 는 자동으로 등록된다.
add_executable(netstorm_tests
    CampaignWorldRuntimeTests.cpp
    CampaignMotionTests.cpp
    CampaignFrameTests.cpp
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
    RawPriestWorldBindingsTests.cpp
    SpRotateTests.cpp
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
set_source_files_properties(SpRotateTests.cpp PROPERTIES COMPILE_DEFINITIONS
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

# 새 신전 활성 표시의 독립 native 변경표를 해당 검사에서만 읽는다.
set_source_files_properties(VortexDisplayTests.cpp PROPERTIES COMPILE_DEFINITIONS
    "NETSTORM_VORTEX_DISPLAY_FIXTURE=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vortex-display-x86.tsv\"")
