# 게임 본체 라이브러리와 실행 파일
#
# 폴더는 원본 exe 의 assert 문자열에 남은 원본 소스 트리를 따른다 (SOURCE_MAP.md):
#   o/        원본 \Ns\O\      공용 모듈 (게임 규칙·데이터·프로세스 커널)
#   client/   원본 .\          클라이언트 모듈 (화면·입력·소리·메인 루프). 원본처럼 Win32 를 직접 부른다
#   zacket/   원본 \Ns\Zacket\ 네트워크 패킷
#   platform/ 새로 쓰는 코드   원본에 없는 보조 기능 (콘솔 출력·BMP 저장·파일 쓰기)
#   app/      새로 쓰는 코드   실행 파일 진입점, 임시 검사용 화면

# 게임 본체. 실행 파일과 테스트가 함께 쓰도록 정적 라이브러리로 묶는다.
# 소스 목록은 각 폴더의 CMakeLists.txt 가 target_sources 로 더한다.
add_library(netstorm STATIC)
netstorm_apply_compiler_options(netstorm)
# 포함 경로의 기준은 src/ 다. 예: #include "client/ClientMain.h"
target_include_directories(netstorm PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(o)
add_subdirectory(client)
add_subdirectory(zacket)
add_subdirectory(platform)

# 실행 파일. C# 빌드의 NetstormClone 과 구분되도록 이름을 달리한다.
target_sources(netstorm PRIVATE app/InspectView.cpp app/InspectView.h)
target_sources(netstorm PRIVATE app/FrameBindingInspect.cpp app/FrameBindingInspect.h)
target_sources(netstorm PRIVATE app/CanonMayPlaceInspect.cpp)
add_executable(NetstormCpp app/main.cpp)
netstorm_apply_compiler_options(NetstormCpp)
target_link_libraries(NetstormCpp PRIVATE netstorm)
