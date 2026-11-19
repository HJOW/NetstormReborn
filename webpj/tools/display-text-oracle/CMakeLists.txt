# cpppj 표시 부호화의 관측 TSV를 만드는 독립 콘솔 검사 도구다.
cmake_minimum_required(VERSION 3.20)
project(WebDisplayTextOracle LANGUAGES CXX)
add_executable(web-display-text-oracle ../display-text-oracle.cpp ../../../cpppj/src/o/OriginalText.cpp)
target_include_directories(web-display-text-oracle PRIVATE ../../../cpppj/src)
target_compile_features(web-display-text-oracle PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(web-display-text-oracle PRIVATE /utf-8)
endif()
