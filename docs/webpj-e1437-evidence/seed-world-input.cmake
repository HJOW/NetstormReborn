# 같은 지도/생성 전 난수 입력을 명시하는 독립 오라클이다.
include("${CMAKE_CURRENT_LIST_DIR}/seed-fast.cmake")
# 원문 모듈 하나의 초기 입력 복사본만 덧붙인다. 기존1377 archive는 읽기 전용이다.
function(webpj_e1426_world_input)
    target_include_directories(netstorm PRIVATE "${CMAKE_SOURCE_DIR}/src/client")
    target_sources(netstorm PRIVATE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/RawCampaignSession.world-input.cpp")
    # 원래큰RawCampaignSession TU에선택된COFF큰객체형식을복사경로에도유지한다.
    target_compile_options(netstorm PRIVATE /bigobj)
    # 기존계획생성함수만호출하는읽기전용CLI다. 별도게임창은띄우지않는다.
    add_executable(WebpjSkirmishInput "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/world-plan-main.cpp")
    target_include_directories(WebpjSkirmishInput PRIVATE "${CMAKE_SOURCE_DIR}/src")
    target_link_libraries(WebpjSkirmishInput PRIVATE netstorm)

endfunction()
cmake_language(DEFER CALL webpj_e1426_world_input)
