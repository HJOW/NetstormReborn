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

# 실제 커서 마나의 월드 객체와 프로세스 수명을 연결한다.
target_sources(netstorm PRIVATE client/RawCursorMana.cpp client/RawCursorMana.h)

# 실제 원형/직선 사정거리의 월드 객체와 프로세스 수명을 연결한다.
target_sources(netstorm PRIVATE client/RawCursorRange.cpp client/RawCursorRange.h)

# 실제 펜스 연결 해제/이웃 갱신/가상 전처리 수명이다.
target_sources(netstorm PRIVATE o/RawFenceLifetime.cpp o/RawFenceLifetime.h)

# 실제 에너지 공급 수집/요구 문자 대응/범위 판정과 공유 목록이다.
target_sources(netstorm PRIVATE o/RawEnergySupply.cpp o/RawEnergySupply.h o/EnergyProviderVtables.inc)

# 같은 월드의 실제 커서와 로컬 예측 건설을 조립한다.
target_sources(netstorm PRIVATE client/RawCampaignCursor.cpp client/RawCampaignCursor.h)

# 실제 커서 수집/dirty 표시와 세 원본 색 변환표다.
target_sources(netstorm PRIVATE client/PreviewColorMaps.cpp client/PreviewColorMaps.h client/RawCampaignCursorDisplay.cpp client/RawCampaignCursorDisplay.h)

# 현재 커서 잠금/회전의 실제 세 몸체다.
target_sources(netstorm PRIVATE client/RawCursorActions.cpp client/RawCursorActions.h)

# 현재 Client 장치와 단일 Gump 트리를 실제 커서에 공급한다.
target_sources(netstorm PRIVATE client/CampaignCursorClientEffects.cpp client/CampaignCursorClientEffects.h)

# 원본 선택과 별개인 강조 모드/마우스 타입의 현재 상태를 처리한다.
target_sources(netstorm PRIVATE client/RawUserHighlight.cpp client/RawUserHighlight.h)

# 세 인자 선택과 다중 선택 정리 및 사제/Carrier/Archer 메뉴 입구다.
target_sources(netstorm PRIVATE client/RawUserSelection.cpp client/RawUserSelection.h)

# 원본 선택 가상+7c와 사정거리/491 생성 입구다.
target_sources(netstorm PRIVATE o/RawSquidSelection.cpp o/RawSquidSelection.h o/SelectionVtables.inc)

# 내용물 미리보기의 생성/만료/빈 통지/부모 삭제 수명이다.
target_sources(netstorm PRIVATE o/RawContentPreviewProcess.cpp o/RawContentPreviewProcess.h)

# 실제 Carrier42a750 전체 사건과 내용물 교대 표시다.
target_sources(netstorm PRIVATE o/RawCarrierRegular.cpp o/RawCarrierRegular.h)

# 너겟의실제전체484f90예약/난수/기본처리다.
target_sources(netstorm PRIVATE o/RawNuggetPostPop.cpp o/RawNuggetPostPop.h)
