# 보호 원본의 기하 함수만 격리 관측하는 별도 Win32 도구다.
cmake_minimum_required(VERSION 3.21)
project(CarrierGeometryHardware LANGUAGES CXX)
if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Build this helper with MSVC and -A Win32")
endif()
add_executable(carrier_geometry_hardware ../native_carrier_geometry.cpp)
target_compile_features(carrier_geometry_hardware PRIVATE cxx_std_20)
target_compile_options(carrier_geometry_hardware PRIVATE /utf-8 /W4)
# 전용 큰 섹션 안에 원본 절대 주소가 들어가도록 helper 자체를 낮은 주소에 둔다.
target_link_options(carrier_geometry_hardware PRIVATE /BASE:0x10000 /DYNAMICBASE:NO)
