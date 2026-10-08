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
