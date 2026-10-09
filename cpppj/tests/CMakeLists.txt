# 단위 테스트. 외부 테스트 프레임워크 없이 TestSupport.h 의 작은 도구만 쓴다
# (첫 구성 때 인터넷에서 내려받을 것이 없다).
# 테스트 파일을 만들면 아래 목록에 더한다. 파일 안의 TEST_CASE 는 자동으로 등록된다.
add_executable(netstorm_tests
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
    PriestShieldTests.cpp
    PriestPostPopTailTests.cpp
    PriestFallTests.cpp
    CarrierCheckTests.cpp
    FrameAdvanceTests.cpp
    PriestPopTests.cpp
    ConfigOracleTests.cpp
    ConfigInterfaceTests.cpp
    FortTests.cpp
    ScreenTests.cpp
    TerritoryTests.cpp
    RendererTests.cpp
    MenuTests.cpp
    WorldTests.cpp
    RawSurfaceWorldTests.cpp
    BridgeTests.cpp
    BridgeDecayTests.cpp
    BridgeLifecycleTests.cpp
    BridgeEventTests.cpp
    RawSceneSupport.h
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
