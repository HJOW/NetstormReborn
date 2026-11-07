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
target_sources(netstorm PRIVATE root/ChallengeState.cpp root/ChallengeState.h root/ChallengeStore.cpp root/ChallengeStore.h) #1067:R 실제 CCh 물리/최초 좌표·단일 프로세스 생성/통신·정리를 연결한다.
target_sources(netstorm PRIVATE root/Configuration.cpp root/Configuration.h) #1066:R 시작/재해석 옵션과 실제 Config/현재 통신 전역을 접합한다.
target_sources(netstorm PRIVATE root/RootDirectory.cpp root/RootDirectory.h) #1065:R16h 요청의 실제0Fh 추천 존/친구 응답을 전송 계층에 연결한다.
target_sources(netstorm PRIVATE o/RawLocalServerDiscovery.cpp o/RawLocalServerDiscovery.h) #1057검색몸체와1058실제UDP/상태접합이다.새R서버실행은후속이다.
target_sources(netstorm PRIVATE client/LocalUdpTransport.cpp client/LocalUdpTransport.h) #1058:실제UDP12를같은연결/패킷표에연결한다.원본R서버복원은별도후속이다.
target_sources(netstorm PRIVATE root/TransceiverTransport.cpp root/TransceiverTransport.h root/TransceiverInternals.h root/TransceiverReceive.cpp root/TransceiverSend.cpp) #1064:R 고정 전송 노드/버퍼/번호·닫힘과 실제 소켓/메인 부모를 연결한다.
target_sources(netstorm PRIVATE root/SocketDevice.cpp root/SocketDevice.h o/RawPacketFrame.cpp o/RawPacketFrame.h) #1063:R 실제 WinSock 장치와 Player 없는 공통 프레임 분배다.
target_sources(netstorm PRIVATE root/NetworkPump.cpp root/NetworkPump.h) #1062:R의256/32/51x40 통신 부모와64칸 select 집합을 분리해 복원한다.
target_sources(netstorm PRIVATE root/ConnectionState.cpp root/ConnectionState.h client/NativeLocalServerLaunch.cpp client/NativeLocalServerLaunch.h) #1059:R전용번호/모드와실제37검색부모이며새서버OS장치는후속이다.
target_sources(netstorm PRIVATE client/FortSubscriberClient.cpp client/FortSubscriberClient.h) #1061:World 없는 현재 Fort도 같은 raw 변경/저장/Notify를 사용한다.
target_sources(netstorm PRIVATE client/RegisteredMessages.cpp client/RegisteredMessages.h) #1060:설정 도구와 LAN 시작 알림이 같은 등록 ID/현재 상태를 사용한다.
# 기록 재생의 원문 판독/180BYTE Run과 실제 Client/Kernel·입력 부모를 함께 컴파일한다.
target_sources(netstorm PRIVATE client/RawBuildReplay.cpp client/RawBuildReplay.h client/ClientBuildReplay.cpp client/ClientBuildReplay.h)
# 메모리 실패의 원문 부모와 실제 Run 등록 수명·Client 종료 연결을 함께 빌드한다(979).
target_sources(netstorm PRIVATE o/RawAllocationFailure.cpp o/RawAllocationFailure.h client/ClientAllocationFailure.cpp platform/ScopedNewHandler.h)
# 원문 TCP 프레이밍/현재 연결표를 사용하는 LAN 전송 부품을 컴파일한다. 실제 Login/UI 접합은 후속 단위다(982).
target_sources(netstorm PRIVATE client/LocalTcpTransport.cpp client/LocalTcpTransport.h)
target_sources(netstorm PRIVATE client/LanBattleLobby.cpp client/LanBattleLobby.h) # 폐기 공식 매칭을 대체하는 로컬 명단/시작 합의이며 Native 게임 전송과 수명을 구별한다.
target_sources(netstorm PRIVATE o/RawPeerWorldSync.cpp o/RawPeerWorldSync.h) # 원문 참가자별 지형/종속 송신과 walker 재등록 순서를 연결하며 MP 검사는 실행하지 않는다.
target_sources(netstorm PRIVATE o/RawBattleIsland.cpp o/RawBattleIsland.h) # 일반 전투의 현재 Fort 영역/지형/권한별 객체 적재 부모다. 신규 MP 검사는 하지 않는다.
target_sources(netstorm PRIVATE o/RawCentralIsland.cpp o/RawCentralIsland.h o/RawIslandPartition.cpp o/RawIslandPartition.h) # 일반 중앙섬/군도 지역 분리를 실제 같은 월드 부모에 연결한다. MP 검사는 실행하지 않는다.
# 원문 공유 등록표/per-ID 복합 순서와 실제 Client/World 수명 대여를 같은 라이브러리에 연결한다(984).
target_sources(netstorm PRIVATE
    o/PacketRegistrationTable.cpp o/PacketRegistrationTable.h o/RawTransportPolling.h
    client/CompositeMissionTransport.cpp client/CompositeMissionTransport.h
    client/CampaignTransportServices.cpp client/CampaignTransportServices.h
    client/ClientCampaignTransport.cpp client/ClientCampaignTransport.h)
target_link_libraries(netstorm PUBLIC ws2_32 version) # qdd의 시스템 DLL 버전 리소스를 Windows W API로 읽으며 드라이버/네트워크 접속은 하지 않는다.
target_sources(netstorm PRIVATE client/ScreenModeTransition.cpp client/ScreenModeTransition.h) #원문 화면 변경 부모의 실패 복구/설정/공유 갱신 순서다(746).
target_sources(netstorm PRIVATE client/SavedCameraKey.cpp client/SavedCameraKey.h) #숫자/Shift 기호와 현재 Renderer 중심의 저장 카메라다(719).
target_sources(netstorm PRIVATE client/LostPositionKey.cpp client/LostPositionKey.h) #U/u는 공통 삭제가 기록한 현재 손실 위치로 즉시 이동한다(751).
target_sources(netstorm PRIVATE o/RawConstructionHistory.h client/ConstructionHistoryKey.cpp client/ConstructionHistoryKey.h) #Tab과 실제 건설 통지가 공유하는 Client 이력/순환이다(718).
target_sources(netstorm PRIVATE client/RotationKeyInput.cpp client/RotationKeyInput.h) #C/c의 원문 회전 방향 반전과 현재 Config 쓰기다(717).
target_sources(netstorm PRIVATE client/LanguageRequest.cpp client/LanguageRequest.h) #원문 언어 요청/현재 Client 큐와 Gump 알림 경로다.
target_sources(netstorm PRIVATE o/RawXlat.cpp o/RawXlat.h) #10.78 최초 다중 번역 장부와 원문 파일 바이트 수명이다.
target_sources(netstorm PRIVATE client/LanguageOptions.cpp client/LanguageOptions.h) #언어 옵션의 원문 이름/감지와 두 용어표 읽기 순서다.
target_sources(netstorm PRIVATE client/KoreanResources.cpp client/KoreanResources.h) #6차 한국어: 클론이 추가한 xlat.korean과 D2Coding 글꼴 파일 탐색이다.
target_sources(netstorm PRIVATE client/FsrConstants.cpp client/FsrConstants.h) #7차 FSR 1.0: AMD 공식 헤더(third_party/fsr1)의 CPU 경로로 EASU/RCAS 상수를 계산한다.
set_source_files_properties(client/FsrConstants.cpp PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/fsr1") # 서드파티 헤더 위치는 이 파일에만 알린다.
# 7차 FSR 1.0: Direct3D 11 후처리 출력기. 런타임 HLSL 컴파일에 쓸 AMD 헤더 두 개를 빌드 때 C++ 바이트 배열로 바꿔 실행 파일에 넣는다.
target_sources(netstorm PRIVATE client/GpuPresenter.cpp client/GpuPresenter.h)
target_link_libraries(netstorm PUBLIC d3d11 dxgi d3dcompiler)
set(NETSTORM_FSR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/fsr1")
set(NETSTORM_FSR_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated/FsrShaderHeaders.cpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${NETSTORM_FSR_DIR}/ffx_a.h" "${NETSTORM_FSR_DIR}/ffx_fsr1.h") # 헤더가 바뀌면 다시 구성해 생성 파일을 갱신한다.
if(NOT EXISTS "${NETSTORM_FSR_GENERATED}" OR "${NETSTORM_FSR_DIR}/ffx_a.h" IS_NEWER_THAN "${NETSTORM_FSR_GENERATED}" OR "${NETSTORM_FSR_DIR}/ffx_fsr1.h" IS_NEWER_THAN "${NETSTORM_FSR_GENERATED}")
    set(NETSTORM_FSR_CODE "// 자동 생성 파일이다(src/CMakeLists.txt). third_party/fsr1의 AMD 헤더 바이트를 그대로 담는다. 직접 고치지 않는다.\nnamespace netstorm::client {\n")
    foreach(NETSTORM_FSR_ENTRY "ffx_a.h|kEmbeddedFfxA" "ffx_fsr1.h|kEmbeddedFfxFsr1")
        string(REPLACE "|" ";" NETSTORM_FSR_PARTS "${NETSTORM_FSR_ENTRY}")
        list(GET NETSTORM_FSR_PARTS 0 NETSTORM_FSR_FILE)
        list(GET NETSTORM_FSR_PARTS 1 NETSTORM_FSR_SYMBOL)
        file(READ "${NETSTORM_FSR_DIR}/${NETSTORM_FSR_FILE}" NETSTORM_FSR_HEX HEX)
        string(LENGTH "${NETSTORM_FSR_HEX}" NETSTORM_FSR_HEX_LENGTH)
        math(EXPR NETSTORM_FSR_SIZE "${NETSTORM_FSR_HEX_LENGTH} / 2")
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," NETSTORM_FSR_BYTES "${NETSTORM_FSR_HEX}")
        string(REGEX REPLACE "((0x[0-9a-f][0-9a-f],)(0x[0-9a-f][0-9a-f],)(0x[0-9a-f][0-9a-f],)(0x[0-9a-f][0-9a-f],)(0x[0-9a-f][0-9a-f],)(0x[0-9a-f][0-9a-f],)(0x[0-9a-f][0-9a-f],)(0x[0-9a-f][0-9a-f],))" "\\1\n" NETSTORM_FSR_BYTES "${NETSTORM_FSR_BYTES}")
        string(APPEND NETSTORM_FSR_CODE "extern const unsigned char ${NETSTORM_FSR_SYMBOL}[] = {\n${NETSTORM_FSR_BYTES}0};\nextern const unsigned long long ${NETSTORM_FSR_SYMBOL}Size = ${NETSTORM_FSR_SIZE};\n")
    endforeach()
    string(APPEND NETSTORM_FSR_CODE "}\n")
    file(WRITE "${NETSTORM_FSR_GENERATED}" "${NETSTORM_FSR_CODE}")
endif()
target_sources(netstorm PRIVATE "${NETSTORM_FSR_GENERATED}")
target_sources(netstorm PRIVATE client/HtmlZsend.cpp client/HtmlZsend.h) #HTML Zsend는실제연결표의11번송신기에원문87h요청을전달한다.신규MP검사는하지않는다.
target_sources(netstorm PRIVATE client/HtmlInputDialog.cpp client/HtmlInputDialog.h) #HTML문자/숫자입력의후속Tell확인을공통실제창에연결한다.
target_sources(netstorm PRIVATE client/RitualChoiceInput.cpp client/RitualChoiceInput.h o/RawRitualChoice.cpp o/RawRitualChoice.h) #의식선택6b입력/수신은디컴파일기반사전구현이며신규MP검사는하지않는다.
target_sources(netstorm PRIVATE client/DestroyAtInput.cpp client/DestroyAtInput.h o/RawDestroyAt.cpp o/RawDestroyAt.h) # 대본의 실제 점 검색/필터/가상 삭제 부모다.
target_sources(netstorm PRIVATE client/UrlInput.cpp client/UrlInput.h) # HTML의URL버튼도기존Help링크분배기에연결한다.
target_sources(netstorm PRIVATE client/RawCaptureSequence.cpp client/RawCaptureSequence.h) # 연속화면저장의원문상태/전체표면복사/반납부모다.
target_sources(netstorm PRIVATE client/RawIndexedBitmap.cpp client/RawIndexedBitmap.h client/IndexedBitmapFile.cpp client/IndexedBitmapFile.h) # 원문8비트BMP와실제비절삭파일장치다.
target_sources(netstorm PRIVATE client/CaptureInput.cpp client/CaptureInput.h client/ClientCaptureSequence.cpp client/ClientCaptureSequence.h) #실제Capture입력과Client화면/할당/파일/안내접합이다.
target_sources(netstorm PRIVATE client/RecheckCdInput.cpp client/RecheckCdInput.h) # RecheckCD 이름의 현재 serial 조회/공통 Kill/0쓰기 입력이다(696).
target_sources(netstorm PRIVATE client/AddRandomInput.cpp client/AddRandomInput.h o/RawAddRandom.cpp o/RawAddRandom.h) # 대본의 실제 무작위 배치 관문/고정 범위 부모다.
target_sources(netstorm PRIVATE client/ScenarioEffectInput.cpp client/ScenarioEffectInput.h) # 실제 일반 연기/화면 번개/명시 무동작 대본 관문이다.
target_sources(netstorm PRIVATE client/ScenarioObjectInput.cpp client/ScenarioObjectInput.h) # 메뉴를 열기 전에도 같은 원문 객체 명령을 처리한다.
target_sources(netstorm PRIVATE client/MoveDialog.cpp client/MoveDialog.h) #004c8df0의 현재 모달 위치/원문 문자열 규칙이다.
target_sources(netstorm PRIVATE client/CdPresence.cpp client/CdPresence.h client/ClientCdGlobals.h) # 현재 CD 루트 열거/캐시 판단을 실제 State 수명과 연결한다.
target_sources(netstorm PRIVATE o/RawBaseFileIO.cpp o/RawBaseFileIO.h) # FILE 위치/길이/종료와 원문4096BYTE 암호 쓰기를 공유한다.
target_sources(netstorm PRIVATE o/RawBaseFileOpenPolicy.cpp o/RawBaseFileOpenPolicy.h) # 전체41b4c0 부모의 장치/TAFF/보조/키/typed 실패와 실제 VFS 메타데이터를 연결한다(757).
target_sources(netstorm PRIVATE o/RawWorldGeyser.h o/RawWorldGeyser.cpp) # 세계 간헐천의 실제 생성 부모00486440이다.
target_sources(netstorm PRIVATE o/RawWorldGeyserBounds.h o/RawWorldGeyserBounds.cpp) # 전체 간헐천의 청크 범위·분할·Player 예산이다.
target_sources(netstorm PRIVATE o/RawPlayerNotification.h o/RawPlayerNotification.cpp) #프레임말 대기 결과 알림과 Core 기술 합계다.
target_sources(netstorm PRIVATE client/TechnicalHelp.h client/TechnicalHelp.cpp client/TechnicalHelpClient.cpp) # 가용성/별도 실행과 실제 Filespec 경로 해석이다.
target_sources(netstorm PRIVATE o/FileSpecQuery.cpp o/FileSpecQuery.h) # 설정Load/Help의원문FileSpec경로와검색핸들수명을공유한다(731).

target_sources(netstorm PRIVATE client/HelpLinkPolicy.cpp client/HelpLinkPolicy.h client/ExternalHelpClient.cpp client/ExternalHelpClient.h) # 원문 Help 정책과 UTF-8 Windows 실행 경계다.
target_link_libraries(netstorm PUBLIC shell32 ole32 mmdevapi) # 원본 ShellExecuteA에 대응하는 Windows10/11 W API를 제공한다.

target_sources(netstorm PRIVATE o/RawRitualRefresh.cpp o/RawRitualRefresh.h) # 의식 뒤 Core/기록/현재 snapshot의 원문 순서다.

target_sources(netstorm PRIVATE o/RawNewTechnology.cpp o/RawNewTechnology.h) # 기술 획득의 중복/저장/설정/안내 순서다.
target_sources(netstorm PRIVATE client/CdDirectory.cpp client/CdDirectory.h) # 원문의 CD-ROM/깊이2 표시 파일 탐색과 실제 W API다.
target_sources(netstorm PRIVATE client/MovieTimer.cpp client/MovieTimer.h) # 원본 SDK의 fallback 영상 대기 시계다.
target_sources(netstorm PRIVATE client/MoviePlayback.cpp client/MoviePlayback.h) # 원문 영상 옵션/입력/idle 판단이다.
target_sources(netstorm PRIVATE client/MovieLifecycle.cpp client/MovieLifecycle.h) # 원문 영상 시작/종료/중앙 표시 수명이다.
target_sources(netstorm PRIVATE client/RawMovieControls.cpp client/RawMovieControls.h client/MoviePcmOutput.cpp client/MoviePcmOutput.h) # 실제 MovieGump/디코더/PCM을 공통 UI 수명에 연결한다.
target_sources(netstorm PRIVATE client/SmackerDecoder.cpp client/SmackerDecoder.h) # 인트로용 실제 팔레트/PCM 디코더 장치다.
add_dependencies(netstorm NetstormSmacker) # 실행 파일 옆의 별도 LGPL DLL을 게임 빌드와 함께 만든다.
target_sources(netstorm PRIVATE client/RawChatPacket.cpp client/RawChatPacket.h) # 일반 채팅 패킷 작성과 송신 직전 길이 확정이다.
target_sources(netstorm PRIVATE client/RawChatReceive.cpp client/RawChatReceive.h) # 일반 수신의 채널/관계 필터와 실제 텍스트 문맥이다.
target_sources(netstorm PRIVATE client/RawChatRelay.cpp client/RawChatRelay.h) # 같은 패킷의 중계·수신 마스크와 실제 observer 등록이다.
target_sources(netstorm PRIVATE client/RawBattleLeave.cpp client/RawBattleLeave.h) # 원본 전투 종료52h와 다음 State3 예약이다.
target_sources(netstorm PRIVATE client/RawEdgeScroll.cpp client/RawEdgeScroll.h) # 원문 가장자리 시간/입력과 현재 카메라 목표 쓰기다.
target_sources(netstorm PRIVATE client/ScreenPresentation.cpp) # 원본과 다른 Windows10/11 전체화면 비율/입력 계산이다.
target_sources(netstorm PRIVATE o/RawSmokeEffect.cpp o/RawSmokeEffect.h) # 실제 냉각 연기 생성/프레임/색/소멸이다.
target_sources(netstorm PRIVATE o/RawEvilRayLifetime.cpp o/RawEvilRayLifetime.h) # 실제 EvilRay 생성/3색/중단/소멸이며 Frame/표시 연결은 후속 범위다(880).
target_sources(netstorm PRIVATE o/RawEvilRayFrame.cpp o/RawEvilRayFrame.h) # 원문 Frame/Draw의 독립 상태·음향/dirty/타원 요청이며 실제 Host/래스터 연결은 후속 범위다(881).
target_sources(netstorm PRIVATE client/VFXEllipse.cpp client/VFXEllipse.h) # 실제 VFX 타원 윤곽/퇴화 선 픽셀과32비트 분기를 복원한다(882).
target_sources(netstorm PRIVATE o/EvilRayVisualSystem.cpp o/EvilRayVisualSystem.h) # 같은 실제 Host/Kernel/표시 목록에서10의 생성/통지/프레임/타원 수명을 연결한다(883).
target_sources(netstorm PRIVATE o/SmokeVisualSystem.cpp o/SmokeVisualSystem.h) # 같은 Host/Kernel·공통 표시 목록의 실제21 수명이다.
target_sources(netstorm PRIVATE o/RawIceRicochet.cpp o/RawIceRicochet.h) # 원본 반사점/표적 선택과 후속 Missile 생성 요청이다.
target_sources(netstorm PRIVATE o/RawGameAuxState.h) # 같은 실제 게임17보조 전역 owner다.
target_sources(netstorm PRIVATE o/RawAuthorityState.h) # 같은 Client의 실제 서버/권한 두 논리값만 장기 소유한다.
# 생산 문자 단축키의 원본 큐 예약을 클라이언트 입력과 같은 명령으로 연결한다.
target_sources(netstorm PRIVATE client/ProductionShortcuts.cpp client/ProductionShortcuts.h)
# 같은 실제 월드의 원본 미니맵 셀 래스터다.
target_sources(netstorm PRIVATE o/RawMinimapRaster.cpp o/RawMinimapRaster.h client/MinimapGeometry.cpp client/MinimapGeometry.h client/RawMinimapControls.cpp client/RawMinimapControls.h)
target_sources(netstorm PRIVATE client/RawUberRootControls.cpp client/RawUberRootControls.h)
target_sources(netstorm PRIVATE client/ClientFortGlobals.cpp client/ClientFortGlobals.h)
target_sources(netstorm PRIVATE client/ClientChatRuntime.cpp client/ClientChatRuntime.h) # 같은 실제 시작/명령표/설정/Core/아이콘 자산을 Chat 본체에 연결한다.
target_sources(netstorm PRIVATE client/RawIssueCommands.cpp client/RawIssueCommands.h) #원문 초기 점 명령 등록/실제 효과를 같은 Chat 표에 연결한다.
target_sources(netstorm PRIVATE o/RawStaticStrings.cpp o/RawStaticStrings.h client/ClientStaticStrings.cpp client/ClientStaticStrings.h) # 원문 임시 고리와 실제 Client 물리/정수 CRT 장치다.
target_sources(netstorm PRIVATE client/ClientPlayerRuntime.cpp client/ClientPlayerRuntime.h) # 원문 공통 Player 초기화와 실제 방향별 동맹 장치다.
target_sources(netstorm PRIVATE client/ClientSpGlobals.cpp client/ClientSpGlobals.h) # 실제 공통 SP/추적 DWORD와 같은 게임·CRT 난수의 장기 수명이다.
target_sources(netstorm PRIVATE client/ClientKencloudGlobals.cpp client/ClientKencloudGlobals.h) # 실제 구름 메모리/파일/옵션과 전체 팔레트 장치를 조립한다.
target_sources(netstorm PRIVATE o/RawKencloudRender.cpp o/RawKencloudRender.h) # 원문 구름 표시/갱신/dirty의 같은 packed 전역과 장치 경계다.

target_sources(netstorm PRIVATE client/EditorLocalTransport.cpp client/EditorLocalTransport.h) # Editor의 로그인 없는 같은 단일 로컬 접속/비동기 큐를 기존 분배기에 연결한다.
target_sources(netstorm PRIVATE o/RawConnectionActivity.cpp o/RawConnectionActivity.h client/ClientConnectionActivityGlobals.h) # 실제64h 수신 몸체와 두22DWORD/Zone 간극의 Client 장기 소유자를 연결한다.
target_sources(netstorm PRIVATE o/RawConnectionCleanup.cpp o/RawConnectionCleanup.h client/ClientConnectionGlobals.h) # 원문 연결/pending 표와 실제 Editor 지연 종료 부모를 연결한다.
target_sources(netstorm PRIVATE o/RawConnectionRegistration.cpp o/RawConnectionRegistration.h o/RawConnectionRegistry.cpp o/RawConnectionRegistry.h client/ClientConnectionGlobals.cpp) #같은 실제 생성/재배치/종료를 장기 연결표에 등록한다.
target_sources(netstorm PRIVATE client/FortCurrentNameDevices.cpp client/FortCurrentNameDevices.h)
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

# 게임 실행 파일은 Netstorm.exe로 만들고 기존 빌드 명령의 NetstormCpp 대상 이름은 유지한다.
target_sources(netstorm PRIVATE app/InspectView.cpp app/InspectView.h)
target_sources(netstorm PRIVATE app/McpServer.cpp app/McpServer.h) # 4차 로컬 MCP 서버(stdio JSON-RPC)다. 프로토콜 처리는 게임 없이 검사한다.
target_sources(netstorm PRIVATE app/FrameBindingInspect.cpp app/FrameBindingInspect.h)
target_sources(netstorm PRIVATE app/CanonMayPlaceInspect.cpp)
add_executable(NetstormCpp app/main.cpp app/ProductVersion.rc) # 새 제품 버전11.01을 Windows 파일 속성에도 제공한다(사용자 요청).
# Windows 배포 이름을 맞추고 import library는 별도 이름으로 둔다. netstorm.lib와 겹치면 MSBuild가 입력을 출력으로 간주해 재링크를 생략한다.
set_target_properties(NetstormCpp PROPERTIES OUTPUT_NAME Netstorm ARCHIVE_OUTPUT_NAME NetstormCpp)
target_sources(NetstormCpp PRIVATE app/NetstormCpp.manifest) # 원본ANSI와 다른 Windows10/11의 프로세스 UTF-8 경로 호환 선언이다.
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

# 같은 선택9DWORD/현재 다중 배열과 첫 메뉴의 실제 raw Clear adapter다.
target_sources(netstorm PRIVATE client/ClientUserSelectionGlobals.h client/ClientUserSelectionCleanup.h client/ClientUserSelectionCleanup.cpp)

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
target_sources(netstorm PRIVATE o/RawBombPostPop.cpp o/RawBombPostPop.h) # 세 주문 효과의 실제 가상20 예약 접두다.

# 일반운반체의실제4f 요청/서버객체내용물변환이다.
target_sources(netstorm PRIVATE o/RawCarrierPickup.cpp o/RawCarrierPickup.h)
target_sources(netstorm PRIVATE o/RawCarrierDrop.cpp o/RawCarrierDrop.h)
target_sources(netstorm PRIVATE o/RawBombActivation.cpp o/RawBombActivation.h)
target_sources(netstorm PRIVATE o/RawBombRegular.cpp o/RawBombRegular.h)
target_sources(netstorm PRIVATE o/RawBombEffect.cpp o/RawBombEffect.h)
target_sources(netstorm PRIVATE o/RawBombTargets.cpp o/RawBombTargets.h)
target_sources(netstorm PRIVATE o/RawMeltdown.cpp o/RawMeltdown.h) # 제네레이터의 실제 요청/검색/폭발 경로다.
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

# 실제64BYTE 카메라와 Client 장기 snapshot/논리Renderer/목표를 구별한다.
target_sources(netstorm PRIVATE client/RawCameraView.cpp client/RawCameraView.h client/ClientCameraGlobals.cpp client/ClientCameraGlobals.h)

# 같은 실제 Kernel/현재 프레임을 pin하고 현재 World 처리기만 대여하는 장기 Host다.
target_sources(netstorm PRIVATE client/RawProcessHostStorage.cpp client/RawProcessHostStorage.h)
#443은커서data-only owner와Session없는실제Cancel/Prepare연결만추가한다.
target_sources(netstorm PRIVATE client/ClientCursorGlobals.h client/ClientCursorStateCleanup.h client/ClientCursorStateCleanup.cpp)
# Native State 직접 전역의 실제 Client/TYPE/Player/UI 주소 연결이다.
target_sources(netstorm PRIVATE client/ClientStateWorldGlobals.h client/ClientStateWorldGlobals.cpp)
# Native ClockSync·MetaZone·Zone6BYTE 정리의 실제 장기 owner 연결이다.
target_sources(netstorm PRIVATE o/RawClockSynchronization.cpp o/RawClockSynchronization.h) # 서버/Commander의 실제5번 시간 응답·70표본·게임보정을 같은 Client에 연결한다.
target_sources(netstorm PRIVATE client/NativeBattleReturnState.cpp client/NativeBattleReturnState.h) # 일반 State6의 배경/음악·정리 전 결과 지역값과 Fort 뒤 결과 안내 부모다.
target_sources(netstorm PRIVATE client/MultiplayerOptionsWindow.h client/RawMenuMultiplayer.inc) # 같은 공통 트리의 원문 일반 옵션 창과 현재 문자열/선택 장치다.
target_sources(netstorm PRIVATE client/UberLan.inc) # 실제 일반 Fort/로컬 대기실/State7 World 교체를 같은 Uber 수명에 연결한다.
target_sources(netstorm PRIVATE client/ClientStateStorageCleanup.h client/ClientStateStorageCleanup.cpp o/RawStateZoneReset.h o/RawStateZoneReset.cpp)
# 하위 SID Reset을 재사용하는 실제 월드 Pool 부모와 기록·실패점 꼬리다.
target_sources(netstorm PRIVATE o/RawSquidPoolReset.h o/RawSquidPoolReset.cpp)

# State의 장기 SID 목록과 실제20포인터 개수 정리다.
target_sources(netstorm PRIVATE o/RawStateSidLists.cpp o/RawStateSidLists.h)

# 실제 SP 표시의 serial 조회와 가상 재부착 부모다.
target_sources(netstorm PRIVATE client/RawStormPowerRefresh.cpp client/RawStormPowerRefresh.h)

# State의 현재 소유자 색과 장기 다리 스캔 초기화다.
target_sources(netstorm PRIVATE o/RawStateBridgeReset.cpp o/RawStateBridgeReset.h)
target_sources(netstorm PRIVATE o/RawBridgeDecayScan.cpp o/RawBridgeDecayScan.h) #현재 슬롯 방문/칸별 예외 경계를 원문 프레임 순서로 실행한다.
target_sources(netstorm PRIVATE o/RawBridgeDecay.cpp o/RawBridgeDecay.h) #실제 수명/금 간 프레임/방문 순서의 직접 효과를 실행한다.
target_sources(netstorm PRIVATE o/RawBridgeCarriers.cpp o/RawBridgeCarriers.h) #실제 칸 위 이동 조회와 같은Graph의 안전한 표면 대피를 실행한다.
target_sources(netstorm PRIVATE client/ClientBridgeDecay.cpp client/ClientBridgeDecay.h) #메뉴와 실제 World의 같은 스캔을469f60 앞에서 호출한다.

# 실제 Present 말미의 진단·목표 제한을 장치 및 현재 view 적용과 구별한다.
target_sources(netstorm PRIVATE client/RawCameraPresent.cpp client/RawCameraPresent.h)

# 원문 현재 view/snapshot·dirty/배율 적용 본체를 외부 장치와 구별한다.
target_sources(netstorm PRIVATE client/RawCameraCommit.cpp client/RawCameraCommit.h)

# 실제 Guide 뒤 입력과 Present 말미에 같은 카메라/현재 Client 장치를 연결한다.
target_sources(netstorm PRIVATE client/UberGumpCamera.cpp)

# 같은 SID의 비석·주문 가상 입력과 원문 타입 변경을 연결한다.
target_sources(netstorm PRIVATE o/RawObjectConversion.cpp o/RawObjectConversion.h)

# 원본 Detonate의 실제 가상 입력과 반환 무시를 일반 부모에 연결한다.
target_sources(netstorm PRIVATE client/DetonateInput.cpp client/DetonateInput.h)
# 원본4d5840 후보 순위는 실제 GameWorld의 해시/우클릭/커서가 공유한다.
target_sources(netstorm PRIVATE client/RawUserPick.cpp client/RawUserPick.h)

# 원본 프로세스 진단의 현재 슬롯/이름/부모 순회와 바이트 서식이다.
target_sources(netstorm PRIVATE o/RawProcessSummary.cpp o/RawProcessSummary.h)
# 원본 Alt/가운데 카메라 입력의 중심 거리·Shift 축·지연 목표를 연결한다.
target_sources(netstorm PRIVATE client/RawCameraPan.cpp client/RawCameraPan.h)

# Renderer 앞단의 화면 옵션/override/구름 행 스캔·dirty 준비를 원문 순서로 복원한다.
target_sources(netstorm PRIVATE client/RawRendererPrelude.cpp client/RawRendererPrelude.h)

# Renderer의 별도 구름4칸/현재 전역과 dirty별 배경 선택을 원문 순서로 복원한다.
target_sources(netstorm PRIVATE client/RawRendererClouds.cpp client/RawRendererClouds.h client/ClientRendererFrameGlobals.h)

# 실제 Client Frame에 같은 원문 앞단/구름/지형/현재 UI 장치를 연결한다.
target_sources(netstorm PRIVATE client/ClientRendererFrame.cpp client/ClientRendererFrame.h)

# 기본 Gump의 원문 갱신 요청/지도 좌표별 clamp를 같은 실제 위치와 Renderer 경계에 제공한다.
target_sources(netstorm PRIVATE client/GumpRedraw.cpp client/GumpRedraw.h)

# 마나103의 실제 조건부 그림자와 현재 프레임/SHP 추가 Draw를 공유한다.
target_sources(netstorm PRIVATE o/RawManaDraw.cpp o/RawManaDraw.h)

# 메타 배율/고정 추상색의 원문 추가 Draw 몸체를 공통 표면에 제공한다.
target_sources(netstorm PRIVATE o/RawAuxiliaryDraw.cpp o/RawAuxiliaryDraw.h)

# 원본 파일과 내부 판본을 보존하며 제품11.01 표시와 추가 Credits를 같은 UI에 연결한다.
target_sources(netstorm PRIVATE client/ProductPresentation.cpp client/ProductPresentation.h client/ProductVersion.h)

#현재 Fort Subscriber 변경의 원문 관문·현재값 재조회 몸체다(713).
target_sources(netstorm PRIVATE o/RawFortSubscriberChange.cpp o/RawFortSubscriberChange.h)

# 편집 모드1의 원문 객체 취득을 같은 Form/커서 수명에 연결한다(749).
target_sources(netstorm PRIVATE client/CursorObjectPickup.cpp client/CursorObjectPickup.h)

# 선택 뒤 Ctrl+special의 지점 진단을 실제 월드 탐색/로그와 연결한다(750).
target_sources(netstorm PRIVATE client/SpecialPointerTrace.cpp client/SpecialPointerTrace.h)

# snapshot/직접OS API와 분리한 명시 전체읽기41baa0 부모다(763).
target_sources(netstorm PRIVATE o/RawBaseFileWholeRead.cpp o/RawBaseFileWholeRead.h)

# 최초Help만같은Client FILE/키의필수전체읽기에연결한다(767).
target_sources(netstorm PRIVATE client/QuickHelpFileRead.cpp client/QuickHelpFileRead.h)

# DataManager만 현재 FILE/키/최소4 할당과 논리 길이를 별도로 소유한다(769).
target_sources(netstorm PRIVATE client/DataManagerFileRead.cpp client/DataManagerFileRead.h)

# 실제Config만40380의존재/해독/Append복귀후free를사용한다(771).
target_sources(netstorm PRIVATE o/RawConfigFileLoad.cpp o/RawConfigFileLoad.h client/ConfigFileRead.cpp client/ConfigFileRead.h)

# 일반간헐천/AI직접명령의실제본문writer를기존제품에연결한다(772).
target_sources(netstorm PRIVATE o/RawAiCommandState.cpp o/RawAiCommandState.h client/AiUtilityCommandInput.cpp client/AiUtilityCommandInput.h)

# 일반MakeAi와owner0객체수명을기존실제생성/World부모에연결한다(773).
target_sources(netstorm PRIVATE o/RawMakeAi.cpp o/RawMakeAi.h client/MakeAiCommandInput.cpp client/MakeAiCommandInput.h)
# 실제10.78 TYPE 시작의 필수 live FILE/키 수명만 명시 연결한다(777).
target_sources(netstorm PRIVATE client/TypeFileRead.cpp client/TypeFileRead.h)
# 현재 checking 진단의 SID 부모만 기존 제품 모듈에 추가한다.
target_sources(netstorm PRIVATE o/RawSidRobust.cpp o/RawSidRobust.h o/RawCheckingFlag.h o/RawCheckingDiagnostics.h)
# 상태 주문/효과 만료가 같은 현재 펜스 타입 진단 부모를 사용한다(802).
target_sources(netstorm PRIVATE o/RawFenceTypeParent.cpp o/RawFenceTypeParent.h)

# TYPE HP의 현재 진단 복귀/선택된 this와 현재 표 기준 계산을 공유한다(898).
target_sources(netstorm PRIVATE o/RawRiftHitPoints.cpp o/RawRiftHitPoints.h)

# 같은 실제 공장에 원문 Create의 진단/할당/현재 생성자/VT 순서를 연결한다(899).
target_sources(netstorm PRIVATE o/RawSquidCreation.cpp o/RawSquidCreation.h)

# 선택 막대의 현재HP/최대HP/색 읽기 부모를 실제 표시기에 연결한다(901).
target_sources(netstorm PRIVATE client/SpriteHitPointBar.cpp client/SpriteHitPointBar.h)

# 연소점 목표 행의 원문 DWORD 감김/나눗셈을 같은 실제 선택 부모에 연결한다(902).
target_sources(netstorm PRIVATE o/RawBurningSourceCount.cpp o/RawBurningSourceCount.h)

# 회복 HP의 순서 부모와 월드 설치 전후의 현재 TYPE HP reader를 공유한다(903).
target_sources(netstorm PRIVATE o/RawPriestRegenHitPoints.cpp o/RawPriestRegenHitPoints.h client/CurrentTypeHitPoints.cpp client/CurrentTypeHitPoints.h)

# 공통 Take의 현재 가상 해제와 반환 객체 순서를 같은 공장에 연결한다(905).
target_sources(netstorm PRIVATE o/RawSquidTake.cpp o/RawSquidTake.h)

# 감시 Form 쌍도 현재 두 Create/WORD·VT/가상 부착의 원문 부모를 사용한다(910).
target_sources(netstorm PRIVATE o/RawDependCreate.cpp o/RawDependCreate.h)

# 현재 TYPE/VT와 부분 쓰기의 공통 부착 부모다(911).
target_sources(netstorm PRIVATE o/RawFormAttach.cpp o/RawFormAttach.h)

# 현재 권한/진단 후 상태와 공유 깊이의 공통 Form Pop 부모다(912).
target_sources(netstorm PRIVATE o/RawFormPop.cpp o/RawFormPop.h)

# 정렬 Content의 원문 링크/늦은 Pop·통지는 공통 Form +74와 구별한다.
target_sources(netstorm PRIVATE o/RawContentAttach.cpp o/RawContentAttach.h)

# Process 파생 접두의 진단/현재 서버와 공통 부착 호출 순서다(959).
target_sources(netstorm PRIVATE o/RawProcessAttach.cpp o/RawProcessAttach.h)

# BaseProcess 상위 부착의 현재 필드/등록/부분 실패 부모다(960).
target_sources(netstorm PRIVATE o/RawBaseProcessAttach.cpp o/RawBaseProcessAttach.h)

# 공통 Owner의 현재 값/진단과 늦은 목록 선택을 복원한다(961).
target_sources(netstorm PRIVATE o/RawCommonOwner.cpp o/RawCommonOwner.h)
target_sources(netstorm PRIVATE o/RawOrdinaryBattleStart.cpp o/RawOrdinaryBattleStart.h) # 일반 Login 완료의 현재 Core/Player/스펠/팀 입력을 조립한다.
target_sources(netstorm PRIVATE o/RawSavedStateWrite.cpp o/RawSavedStateWrite.h client/SavedStateFileWrite.cpp client/SavedStateFileWrite.h) # 일반 재접속 저장의 개별 원문 쓰기를 실제 사용자 FILE에 연결한다.
target_sources(netstorm PRIVATE client/SavedStateFileRead.cpp client/SavedStateFileRead.h) # startup 저장상태 reader가 같은 실제 Client Player/Core와 사용자 FILE을 읽는다.
target_sources(netstorm PRIVATE o/RawBattleCompletion.cpp o/RawBattleCompletion.h) #449200/48fd0/490d0의 현재 전투 종료 관문을 실제 Player와 연결한다.
target_sources(netstorm PRIVATE o/RawBattleReconnect.cpp o/RawBattleReconnect.h) #4b8cb0의 실제 서버 선택과 현재15/다음7·3 요청을 복원한다.
target_sources(netstorm PRIVATE o/RawConnectionHealth.cpp o/RawConnectionHealth.h) #4b9010의 프레임 활동 송신/지연/자동 서버 장애 관문이다.
target_sources(netstorm PRIVATE o/RawConnectionEnded.cpp o/RawConnectionEnded.h) #4b6660의 실제2번 종료 통지/Player flags·선택 활동표 정리다.
target_sources(netstorm PRIVATE o/RawBattleNuggetPlacement.cpp o/RawBattleNuggetPlacement.h) #40e050의 비미션 보충 배치를 같은 실제 월드에 연결한다.
target_sources(netstorm PRIVATE o/RawBattleResourceOptions.cpp o/RawBattleResourceOptions.h) # 현재 SP 지급/간헐천 주기 표를 실제 수신·프레임이 공유한다.
target_sources(netstorm PRIVATE o/RawBattleResourceFrame.cpp o/RawBattleResourceFrame.h) #486bc0의 스펠/SP 주기/간헐천 전체 프레임을 같은 현재 상태에 연결한다.
target_sources(netstorm PRIVATE o/RawRemoteTellPacket.cpp o/RawRemoteTellPacket.h) #70h의 실제 문자열 writer/관찰기/현재 Tell 수신이다.
target_sources(netstorm PRIVATE o/RawDeveloperCommandPacket.cpp o/RawDeveloperCommandPacket.h) #31h의 실제 길이 관찰/현재 문자열 수신이다.
target_sources(netstorm PRIVATE o/RawConstructionFailurePackets.cpp o/RawConstructionFailurePackets.h) # 비용실패의6ch/2fh와같은실제예측정리수신이다.
target_sources(netstorm PRIVATE o/RawSquidReceive.cpp o/RawSquidReceive.h) # 실제35BYTE 객체 수신/검증·PlayerNoMake를 같은 Session에 연결한다.
target_sources(netstorm PRIVATE o/RawSquidCompressedReceive.cpp o/RawSquidCompressedReceive.h) #7ah의16필드 decoder/실제 길이 관찰기와 같은 수신 캐시다.
target_sources(netstorm PRIVATE o/RawSquidVerification.cpp o/RawSquidVerification.h) #4ah/4bh의 검증 시작과 자유 슬롯/이전 SID 간격 부모다.
target_sources(netstorm PRIVATE o/RawSquidMutationReceive.cpp o/RawSquidMutationReceive.h) # 내용물2dh/삭제2eh의 같은 실제 부착·환불·삭제 수신이다.
target_sources(netstorm PRIVATE o/RawAviaryProcessData.cpp o/RawAviaryProcessData.h) # Sun/Rain 공통 decoder와 세 아비어리의 같은 PID/명시 Form 수신이다.
