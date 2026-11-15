# cpppj Poll 본체를 게임이나 창 없이 호출하는 Windows 전용 관측 도구를 빌드한다.
cmake_minimum_required(VERSION 3.20)
project(clock_poll_observer LANGUAGES CXX)
get_filename_component(REPO_ROOT "${CMAKE_SOURCE_DIR}/../../.." ABSOLUTE)
add_executable(clock-poll-observer main.cpp "${REPO_ROOT}/cpppj/src/o/RawClockSynchronization.cpp")
target_include_directories(clock-poll-observer PRIVATE "${REPO_ROOT}/cpppj/src")
target_compile_features(clock-poll-observer PRIVATE cxx_std_20)
target_compile_options(clock-poll-observer PRIVATE /Gy /utf-8)
target_link_options(clock-poll-observer PRIVATE /OPT:REF)
