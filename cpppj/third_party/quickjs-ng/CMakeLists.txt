# 14차: QuickJS-ng JavaScript 엔진을 게임에 정적으로 넣는 라이브러리 대상이다(프로젝트가 추가한 빌드 자료이며 상류 파일은 바꾸지 않는다).
# 상류 v0.18.0 CMakeLists.txt 의 qjs 라이브러리 구성(소스 4개·정의·MSVC 경고 억제)을 따른다. quickjs-libc(파일·OS·워커)는 넣지 않는다.
# 포함 경로와 정의는 PRIVATE 로만 둔다. 링크하는 netstorm 라이브러리의 다른 소스 명령줄이 바뀌어 전체 재컴파일되는 일을 막기 위해서다.
# 엔진 헤더는 src/platform/ScriptEngine.cpp 한 파일에만 원천 단위 포함 경로로 알린다(src/platform/CMakeLists.txt).
# (2026-11-11 14차 QuickJS 엔진 탑재 신규)
cmake_minimum_required(VERSION 3.21)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(NetstormQuickJs LANGUAGES C) # 배포한 이 폴더만으로도 같은 정적 라이브러리를 다시 빌드할 수 있게 한다.
endif()

# 상류 qjs 라이브러리의 소스 목록과 같다(dtoa·정규식·유니코드·엔진 본체).
add_library(NetstormQuickJs STATIC
    quickjs.c
    dtoa.c
    libregexp.c
    libunicode.c)
# 상류와 같은 C11 이며 컴파일러 고유 확장 이름(gnu11)은 쓰지 않는다.
set_target_properties(NetstormQuickJs PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
# 상류가 Windows 에서 쓰는 정의다. Windows 7 하한(_WIN32_WINNT=0x0601)과 windows.h 축소 포함이다.
target_compile_definitions(NetstormQuickJs PRIVATE _GNU_SOURCE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)
if(MSVC)
    # /utf-8: 저작권 주석의 UTF-8 문자를 cp949 등 시스템 코드 페이지로 잘못 읽지 않게 한다.
    # /experimental:c11atomics: 상류와 같이 <stdatomic.h> 를 켜서 Atomics/SharedArrayBuffer 구현을 포함한다.
    # /wd4018 ~ /wd5045 는 상류 CMakeLists.txt 의 MSVC 경고 억제 목록 그대로다.
    # /wd4098 은 프로젝트가 더한 것이다. 상류 quickjs.c 의 void 함수가 void 식을 return 하는 4곳(JS_FreeCString 계열)에서 나는 무해한 경고다.
    target_compile_options(NetstormQuickJs PRIVATE
        /utf-8
        /experimental:c11atomics
        /wd4018 /wd4061 /wd4100 /wd4200 /wd4242 /wd4244 /wd4245 /wd4267
        /wd4388 /wd4389 /wd4456 /wd4457 /wd4710 /wd4711 /wd4820 /wd4996 /wd5045
        /wd4098)
endif()
