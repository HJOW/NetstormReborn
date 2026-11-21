# 실제 CPP 소스의 순수 CS 배치 좌표를 관찰하는 개발 도구다.
cmake_minimum_required(VERSION 3.25)
project(NetstormChallengeGeometryProbe LANGUAGES CXX)
# 도구 소스에서 저장소 루트를 고정하여 빌드 디렉토리와 무관하게 읽는다.
get_filename_component(repo_root "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
add_executable(challenge-geometry-probe main.cpp "${repo_root}/cpppj/src/o/RawChallengeState.cpp")
target_include_directories(challenge-geometry-probe PRIVATE "${repo_root}/cpppj/src")
target_compile_features(challenge-geometry-probe PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(challenge-geometry-probe PRIVATE /utf-8 /fp:strict)
endif()
