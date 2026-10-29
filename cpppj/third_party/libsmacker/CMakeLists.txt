# 상류 LGPL 소스는 변경 없이 별도 DLL로 만들어 사용자가 디코더를 교체할 수 있게 한다.
cmake_minimum_required(VERSION 3.21)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(NetstormSmacker LANGUAGES C) # 배포한 소스 폴더만으로도 교체 DLL을 다시 빌드한다.
endif()
add_library(NetstormSmacker SHARED smacker.c)
set_target_properties(NetstormSmacker PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS NO WINDOWS_EXPORT_ALL_SYMBOLS YES)
target_compile_definitions(NetstormSmacker PRIVATE _CRT_SECURE_NO_WARNINGS)
