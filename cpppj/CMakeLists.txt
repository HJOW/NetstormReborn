# NetStorm C++ 빌드 (cpppj/)
# 기존 게임을 디컴파일한 소스를 토대로 공용 데이터·시간·프로세스 계층을 복원하는 프로젝트다.
# 설명: README.md, ../docs/cpp-build.md
cmake_minimum_required(VERSION 3.21)

project(NetstormCpp
    VERSION 0.1.0
    DESCRIPTION "NetStorm: Islands at War - C++ build reconstructed from the decompiled original"
    LANGUAGES CXX)

# cpppj는 Windows 전용이다(AGENTS.md). 원본과 같은 Win32 API를 직접 쓴다.
if(NOT WIN32)
    message(FATAL_ERROR "cpppj builds on Windows only")
endif()

# C++ 표준: C++20 (MSVC 2022, GCC 11+, Clang 14+)
set(CMAKE_CXX_STANDARD 20)
# 표준을 지원하지 않는 컴파일러에서는 조용히 낮추지 않고 구성 단계에서 멈춘다
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 컴파일러 고유 확장(gnu++20 등)은 쓰지 않는다
set(CMAKE_CXX_EXTENSIONS OFF)
# 편집기·분석 도구용 compile_commands.json 을 빌드 폴더에 만든다 (Ninja·Makefile 생성기에서만 생긴다)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 단위 테스트 빌드 여부 (기본 켜짐)
option(NETSTORM_BUILD_TESTS "단위 테스트를 함께 빌드한다" ON)

# 실행 파일과 라이브러리를 빌드 폴더의 한곳에 모은다 (게임 데이터·글꼴을 실행 파일 기준으로 찾기 쉽게)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# 공통 컴파일 옵션 함수(netstorm_apply_compiler_options)
include(cmake/CompilerOptions.cmake)

add_subdirectory(src)

if(NETSTORM_BUILD_TESTS)
    # enable_testing() 은 최상위 CMakeLists 에서 불러야 빌드 폴더에서 ctest 가 테스트를 찾는다
    enable_testing()
    add_subdirectory(tests)
endif()
