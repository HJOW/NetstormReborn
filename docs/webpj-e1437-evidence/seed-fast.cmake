# 기존1377 라이브러리를 읽기 전용으로 재사용하고 씨앗 입력을 바꾼 UberGump 한 파일만 다시 컴파일한다.
include("${CMAKE_CURRENT_LIST_DIR}/seed-input.cmake")
# 원래 타깃의 옵션/포함 경로/전이 시스템 라이브러리를 유지한다. 기존 빌드 폴더에는 쓰지 않는다.
function(webpj_e1426_fast)
    set_property(TARGET netstorm PROPERTY SOURCES "${WEBPJ_E1426_SEED_SOURCE}")
    get_target_property(libraries netstorm LINK_LIBRARIES)
    get_target_property(interfaces netstorm INTERFACE_LINK_LIBRARIES)
    message(STATUS "원래 netstorm 링크: ${libraries}; 인터페이스: ${interfaces}")
    # 같은 기존 엔진 archive를 선택해 게임 코드와 무관한 엔진 재빌드를 생략한다.
    string(REPLACE "NetstormQuickJs" "H:/Workspace/git/NetstormReborn/cpppj/build/lib/Release/NetstormQuickJs.lib" libraries "${libraries}")
    string(REPLACE "NetstormQuickJs" "H:/Workspace/git/NetstormReborn/cpppj/build/lib/Release/NetstormQuickJs.lib" interfaces "${interfaces}")
    set_property(TARGET netstorm PROPERTY LINK_LIBRARIES "${libraries}")
    set_property(TARGET netstorm PROPERTY INTERFACE_LINK_LIBRARIES "${interfaces}")
    target_link_libraries(netstorm PUBLIC "${WEBPJ_E1426_BASE_LIBRARY}")
endfunction()
cmake_language(DEFER CALL webpj_e1426_fast)
