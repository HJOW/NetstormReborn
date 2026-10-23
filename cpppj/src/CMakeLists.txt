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
# 생산 문자 단축키의 원본 큐 예약을 클라이언트 입력과 같은 명령으로 연결한다.
target_sources(netstorm PRIVATE client/ProductionShortcuts.cpp client/ProductionShortcuts.h)
# 같은 실제 월드의 원본 미니맵 셀 래스터다.
target_sources(netstorm PRIVATE o/RawMinimapRaster.cpp o/RawMinimapRaster.h client/MinimapGeometry.cpp client/MinimapGeometry.h client/RawMinimapControls.cpp client/RawMinimapControls.h)
# 원본 단일 객체/청크 저장을 같은 현재 TYPE·가상 분배와 Finder에 연결한다.
target_sources(netstorm PRIVATE o/RawFortObjectSave.cpp o/RawFortObjectSave.h)
# 현재 섬 소유권/청크 기록과 원본 Territory 저장 부모를 함께 사용한다.
target_sources(netstorm PRIVATE o/RawFortTerritorySave.cpp o/RawFortTerritorySave.h)
# 현재 내용물의 robust/QA/flags/중첩 목록을 원본 stream 형식으로 저장한다.
target_sources(netstorm PRIVATE o/RawFortContentSave.cpp o/RawFortContentSave.h)
# 현재 월드 TypeNames/Deck의 원본 snapshot 본체와 명시 stream 쓰기를 연결한다.
target_sources(netstorm PRIVATE o/RawFortSnapshotBasics.cpp o/RawFortSnapshotBasics.h)
# 공통 객체 명령의 현재 모드/가상 소유자·피해·삭제 분기다.
target_sources(netstorm PRIVATE o/RawObjectCommand.cpp o/RawObjectCommand.h)
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

# 너겟의왕복프레임/지면조회/가속낙하와실제게임난수다.
target_sources(netstorm PRIVATE o/RawNuggetRegular.cpp o/RawNuggetRegular.h)

# 캐리어의 실제 다리 판정/낙하/포획 사제 해방이다.
target_sources(netstorm PRIVATE o/RawCarrierFalling.cpp o/RawCarrierFalling.h)

# 캐리어의 자연 낙하 시작과 원본 공통 task 예약이다.
target_sources(netstorm PRIVATE o/RawCarrierTask.cpp o/RawCarrierTask.h o/RawCarrierFallStart.cpp o/RawCarrierFallStart.h)

# 미등록187 정지 분기와 실제 너겟 전달 수신 명령이다.
target_sources(netstorm PRIVATE o/RawCarrierTransfer.cpp o/RawCarrierTransfer.h)

# 운반체의 실제 대상 접근점과 경로 시작/귀환 명령이다.
target_sources(netstorm PRIVATE o/RawBoardTarget.cpp o/RawBoardTarget.h o/RawCarrierTarget.cpp o/RawCarrierTarget.h)

# 골렘 후보 비교기의 주소 감김과 구형 CRT 동률 순서다.
target_sources(netstorm PRIVATE o/RawCarrierCandidateSort.cpp o/RawCarrierCandidateSort.h)

# 골렘의 실제 귀환·간헐천 목록과 임시 BFS/시각 평가다.
target_sources(netstorm PRIVATE o/RawCarrierChoice.cpp o/RawCarrierChoice.h)

# 운반 경로의 실제 대상 재확인과 새 이동의 Runtime 수명을 보존한다.
target_sources(netstorm PRIVATE o/RawPathRetarget.cpp o/RawPathRetarget.h)

# 실제 채취 통지의 작성/송신/로컬 수량 차감을 같은 월드에 연결한다.
target_sources(netstorm PRIVATE o/RawCarrierMining.cpp o/RawCarrierMining.h)

# 고갈한 간헐천의 고유 프레임/AI·목록 제거와 공통 삭제 순서다.
target_sources(netstorm PRIVATE o/RawGeyserLifetime.cpp o/RawGeyserLifetime.h)

# 원본 네 방향 회피와 임시BFS/도착시각 최소값 선택이다.
target_sources(netstorm PRIVATE o/RawCarrierEscape.cpp o/RawCarrierEscape.h)

# 실제 크리스탈 크랩 포획과60h 수신 몸체다.
target_sources(netstorm PRIVATE o/RawCarrierSteal.cpp o/RawCarrierSteal.h)

# 일반운반체의실제4f 요청/서버객체내용물변환이다.
target_sources(netstorm PRIVATE o/RawCarrierPickup.cpp o/RawCarrierPickup.h)
target_sources(netstorm PRIVATE o/RawCarrierDrop.cpp o/RawCarrierDrop.h)
target_sources(netstorm PRIVATE o/RawBombActivation.cpp o/RawBombActivation.h)
target_sources(netstorm PRIVATE o/RawBombRegular.cpp o/RawBombRegular.h)
target_sources(netstorm PRIVATE o/RawBombEffect.cpp o/RawBombEffect.h)
target_sources(netstorm PRIVATE o/RawBombTargets.cpp o/RawBombTargets.h)
target_sources(netstorm PRIVATE o/RawBombBlast.cpp o/RawBombBlast.h)
target_sources(netstorm PRIVATE o/RawBombStrike.cpp o/RawBombStrike.h)

# 오벨리스크의 실제 임시Deck 추첨과 보상 변환이다.
target_sources(netstorm PRIVATE o/RawDeckDraw.cpp o/RawDeckDraw.h o/RawGemReward.cpp o/RawGemReward.h)

# 반복 보상 획득의 현재 모드 관문과 지역 보충 범위를 복원한다.
target_sources(netstorm PRIVATE o/RawNuggetReplenish.cpp o/RawNuggetReplenish.h)

# 보상 Bomb의 고유 전처리와 실제 공통 삭제 연결이다.
target_sources(netstorm PRIVATE o/RawBombPreDestroy.cpp o/RawBombPreDestroy.h)

# 벌룬의 실제 고도 프레임·그림자 후처리와 삭제를 연결한다.
target_sources(netstorm PRIVATE o/RawBalloonFrame.cpp o/RawBalloonFrame.h o/RawBalloonBody.cpp o/RawBalloonBody.h)

# 원본40h 이동 요청/서버 수신을 같은 실제 Session에서 사용한다.
target_sources(netstorm PRIVATE o/RawCarrierMove.cpp o/RawCarrierMove.h)

# 같은Session의원본로그인/로컬전송수명과실제Client장치경계다.
target_sources(netstorm PRIVATE client/CampaignLoginServices.h client/RawCampaignSessionLogin.inc)

target_sources(netstorm PRIVATE client/RawCarrierInput.h client/RawCarrierInput.cpp)

# 실제 Client 입력 장치와 동일 월드의 선택/강조/명령 연결부다.
target_sources(netstorm PRIVATE client/GameWorldInput.inc client/RawCampaignCarrierInput.inc client/CampaignUserInputServices.h client/CampaignUserInputServices.cpp)

# 실제 Factory 수집/원본 일반 Draw와 세 소스 색표다.
target_sources(netstorm PRIVATE client/SpriteColorMaps.cpp client/SpriteColorMaps.h client/RawCampaignFactoryDisplay.cpp client/RawCampaignFactoryDisplay.h)

# 원본 Factory의 실제 생산/지식/마나/CRT정렬 메뉴다.
target_sources(netstorm PRIVATE client/RawFactoryMenu.cpp client/RawFactoryMenu.h)

# 실제 Client 생산 패널과 원본 비활성/선행 색표다.
target_sources(netstorm PRIVATE client/ProductionColorMaps.cpp client/ProductionColorMaps.h client/RawProductionControls.cpp client/RawProductionControls.h)

# 원본 생산 유지의 다리 공급·품질/재고와 비용 전환이다.
target_sources(netstorm PRIVATE client/RawProductionMaintenance.cpp client/RawProductionMaintenance.h)

# 사제 제단 자격/희생 wrapper의 실제 요청 몸체다.
target_sources(netstorm PRIVATE o/RawPriestSacrificeRequest.cpp o/RawPriestSacrificeRequest.h)

# 사제604 의식 애니메이션의 실제 반복 몸체다.
target_sources(netstorm PRIVATE o/RawPriestRitualFrame.cpp o/RawPriestRitualFrame.h)

# 실제 회수 관문과54h 일반 삭제 요청/서버 수신을 같은 세계에 연결한다.
target_sources(netstorm PRIVATE o/RawSalvageCommand.cpp o/RawSalvageCommand.h)

# 같은 Session에 원본 처리기를 조립하는 MSVC 번역 단위의 COFF 섹션 한도를 확장한다.
if(MSVC)
    set_property(SOURCE client/RawCampaignSession.cpp APPEND PROPERTY COMPILE_OPTIONS /bigobj)
endif()
# 데빌메이커 전용 본체와 상태 패킷의 실제 생성/부분 갱신 규칙이다.
target_sources(netstorm PRIVATE o/RawWindAviaryState.cpp o/RawWindAviaryState.h)
# 데빌메이커 생산 단계와 실제 Kernel 어댑터다.
target_sources(netstorm PRIVATE o/RawWindAviaryProcess.cpp o/RawWindAviaryProcess.h)
