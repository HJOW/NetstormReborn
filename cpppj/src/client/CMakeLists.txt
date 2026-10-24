# 원본 클라이언트 폴더(assert 문자열의 ".\")의 모듈
# (ClientMain·UserInput·Gump 계열·Screen·Renderer·Sound·State·Mission 등)
# 다시 만든 파일을 아래 목록에 더한다. 파일 이름은 SOURCE_MAP.md 의 "cpppj 경로"를 따른다.
target_sources(netstorm PRIVATE
    # 기존 Editor checkpoint 이후의 실제 State/입력 고리와 장치 순서를 연결한다.
    ExistingEditorAfterCheckpoint.cpp ExistingEditorAfterCheckpoint.h
    # 실제게임메뉴부모·명령ID와Tell지역param파서를같은Client/Session에연결한다.
    RawGameMenuBar.cpp RawGameMenuBar.h RawGameMenus.cpp RawGameMenus.h
    GameMenuCommands.cpp GameMenuCommands.h TellParameters.cpp TellParameters.h
    GameMenuActions.cpp GameMenuActions.h MessageGump.cpp MessageGump.h
    DialogExitCommand.cpp DialogExitCommand.h
    # 원본 창 경계의 Alt/Ctrl 폴링과 재진입 좌우 release 보정을 같은 입력 큐에 연결한다.
    GumpPointerBoundary.cpp GumpPointerBoundary.h
    MissionLoadingMessage.cpp MissionLoadingMessage.h
    # 원본 HUD의 divider 생성·슬레이브 재배치·지도 영역과 공통 입력 드래그 본체다.
    HudDock.cpp HudDock.h HudDividerInput.cpp HudDividerInput.h
    # 현재 미션 F8/First의 같은 입력 큐와 원본 안내 재표시를 연결한다.
    MissionGuideInput.cpp MissionGuideInput.h
    # 실제 Guide 순번과 월드 진입 Pause/얇은 가상 단계를 현재 Client 수명에 연결한다.
    MissionLifecycle.cpp MissionLifecycle.h
    TutorialInit.cpp TutorialInit.h
    MissionFortRequest.cpp MissionFortRequest.h
    CampaignTemplateStorage.cpp CampaignTemplateStorage.h
    FortTypeConversionStorage.cpp FortTypeConversionStorage.h
    SoloMapCatalog.cpp SoloMapCatalog.h
    TellMapMenus.cpp TellMapMenus.h
    EditorFiles.cpp EditorFiles.h
    # 현재 Fort 설정 경로를 같은 보호된 작업본 파일 장치에 연결한다.
    EditorFortFileDevices.cpp EditorFortFileDevices.h
    # 실제 Chat 할당과 현재 CoreData/캐시의 주소 수명을 서로 분리해 소유한다.
    ClientChatManagerGlobals.cpp ClientChatManagerGlobals.h
    ClientCoreDataGlobals.cpp ClientCoreDataGlobals.h
    # 실제 Guide의 시작/분기/재시작/종료 교체와 원본 제어 사건을 연결한다.
    GuideMissionTransitions.cpp GuideMissionTransitions.h
    # 실제 초기화의 기술 허용 목록을 건설/생산의 동일 금지 표에 적용한다.
    MissionTechnology.cpp MissionTechnology.h
    # 전체 지식 창과 Factory가 같은 원본 행/CRT 정렬을 사용하고 실제 메뉴 카드를 작성한다.
    KnowledgeSort.cpp KnowledgeSort.h RawKnowledgeMenu.cpp RawKnowledgeMenu.h
    ThemeTechnologyCommand.cpp ThemeTechnologyCommand.h
    DialogCommandLifetime.cpp DialogCommandLifetime.h
    # 실제 Client 수명의 원본 100기록 채팅 채널을 공유한다.
    RawChatChannels.cpp RawChatChannels.h
    DialogBodyLayout.cpp DialogBodyLayout.h
    RawCampaignLifetimeBindings.cpp RawCampaignLifetimeBindings.h
    HtmlTextParser.cpp HtmlTextParser.h
    HtmlConditionEffect.cpp HtmlConditionEffect.h
    ScriptCommand.cpp ScriptCommand.h
    MessageBoard.cpp MessageBoard.h WarningMessages.cpp WarningMessages.h
    ClientAudio.cpp
    RawCampaignArcherBindings.cpp RawCampaignArcherBindings.h
    RawCampaignHitBindings.cpp RawCampaignHitBindings.h
    RawCampaignSession.cpp RawCampaignSession.h
    RawCampaignConstructionBindings.cpp RawCampaignConstructionBindings.h
    # 같은 Player/Factory 목록과 실제 Deck으로 생산 가능 항목을 재계산한다.
    RawCampaignProductionBindings.cpp RawCampaignProductionBindings.h
    # 실제 미션 Start의 같은 Script/Player/SP/기술/AI 설정을 연결한다.
    RawCampaignMissionStartBindings.cpp RawCampaignMissionStartBindings.h
    # Early1의 같은 실제 프레임/입력/장부와 안내 정지를 연결한다.
    RawCampaignTutorialOneBindings.cpp RawCampaignTutorialOneBindings.h
    CampaignClientServices.cpp CampaignClientServices.h
    ClientAudio.h
    ClientMain.cpp
    ClientMain.h
    VFXDraw.cpp VFXDraw.h
    VFXTransform.cpp VFXTransform.h
    Screen.cpp Screen.h
    PaletteColors.cpp PaletteColors.h
    PaletteFlash.cpp PaletteFlash.h
    GameAssets.cpp GameAssets.h
    AiTypeRequiredEnergy.cpp AiTypeRequiredEnergy.h
    PriestPlacementAssets.cpp PriestPlacementAssets.h
    Mission.cpp Mission.h
    WindowTitle.cpp WindowTitle.h
    CameraHome.cpp CameraHome.h
    InputEvent.cpp InputEvent.h
    Renderer.cpp Renderer.h
    DrawSort.cpp DrawSort.h
    DrawSubmission.cpp DrawSubmission.h
    SquidRenderer.cpp SquidRenderer.h
    ConstructionVisualRenderer.cpp ConstructionVisualRenderer.h
    BitmapFont.cpp BitmapFont.h
    Cursor.cpp Cursor.h
    GifImage.cpp GifImage.h
    DialogScript.cpp DialogScript.h
    Gump.cpp Gump.h
    RawButtonControls.cpp RawButtonControls.h
    GumpCollection.cpp GumpCollection.h
    GumpDraw.cpp GumpDraw.h
    GumpCreate.cpp GumpCreate.h
    GumpTree.cpp GumpTree.h
    GumpLifetime.cpp GumpLifetime.h
    GumpEvents.cpp GumpEvents.h
    GumpInput.cpp GumpInput.h
    GumpFrame.cpp GumpFrame.h
    ButtonGump.cpp ButtonGump.h
    StoneButtonGump.cpp StoneButtonGump.h
    GumpBackgroundCreate.cpp GumpBackgroundCreate.h
    GumpBackgroundDraw.cpp GumpBackgroundDraw.h
    GumpHelp.cpp GumpHelp.h
    GumpText.cpp GumpText.h
    GumpTextDraw.cpp GumpTextDraw.h
    MenuGump.cpp MenuGump.h
    MenuGumpLayout.cpp MenuGumpLayout.h
    GumpDecoration.cpp GumpDecoration.h
    MenuGumpWidgets.cpp MenuGumpWidgets.h
    RawPriestMenu.cpp RawPriestMenu.h
    RawObjectMenu.cpp RawObjectMenu.h
    RawCarrierMenu.cpp RawCarrierMenu.h
    RawPlayerMenu.cpp RawPlayerMenu.h
    GumpFormBinding.cpp GumpFormBinding.h
    MenuGumpInput.cpp MenuGumpInput.h
    MenuGumpDraw.cpp MenuGumpDraw.h
    MenuGumpControls.cpp MenuGumpControls.h
    StyleText.cpp StyleText.h
    StyleTextMarkup.cpp StyleTextMarkup.h
    StyleTextBuffer.cpp StyleTextBuffer.h
    TextGump.cpp TextGump.h
    ScrollGump.cpp ScrollGump.h
    VFXLine.cpp VFXLine.h
    ScrollGumpWidgets.cpp ScrollGumpWidgets.h
    ScrollingTextGumpWidgets.cpp ScrollingTextGumpWidgets.h
    ScrollingTextGumpMarkup.cpp ScrollingTextGumpMarkup.h
    Markup.cpp Markup.h
    TileGroupCollection.cpp TileGroupCollection.h
    ObjectCollection.cpp ObjectCollection.h
    PreviewCollection.cpp PreviewCollection.h
    PreviewDraw.cpp PreviewDraw.h
    SpriteDraw.cpp SpriteDraw.h
    GumpVisual.cpp GumpVisual.h
    GumpBackground.cpp GumpBackground.h
    PaletteShade.cpp PaletteShade.h
    State.cpp State.h
    UberGump.cpp UberGump.h
    UberRootGump.cpp UberRootGump.h
    GameWorld.cpp GameWorld.h
    RawSurfaceWorld.cpp RawSurfaceWorld.h
    RawSurfaceStorage.cpp
    ClientGameplayGlobals.cpp ClientGameplayGlobals.h ProductionControlContext.h
    ClientNsInfoGlobals.cpp ClientNsInfoGlobals.h RawNsInfoContext.h
    HudNsInfo.cpp HudNsInfo.h HudNsInfoCreate.cpp HudNsInfoCreate.h
    ChatWindow.cpp ChatWindow.h HudChatGeometry.cpp HudChatGeometry.h
    TextEntryGump.cpp TextEntryGump.h TextEntryBuffer.cpp TextEntryBuffer.h
    RawTerrainWorldBinding.cpp RawTerrainWorldBinding.h
    RawNormalTerrainWorldBinding.cpp RawNormalTerrainWorldBinding.h
    RawFortWorldLoadBinding.cpp RawFortWorldLoadBinding.h
    RawShapeToBuffer.cpp RawShapeToBuffer.h
    # 같은 raw 피해객체와 실제 SHP에서 Burning 프레임의 최초 이미지를 얻는다.
    BurningFrameCapture.cpp BurningFrameCapture.h
    RawPriestWorldBindings.cpp RawPriestWorldBindings.h
    # 실제 Player 신전 기록과 같은 Core의 자산/사제 경로를 공유하는 캠페인 연결부다.
    RawCampaignAssetBindings.cpp RawCampaignAssetBindings.h
    RawCampaignOwnership.cpp RawCampaignOwnership.h
    RawCampaignWorldRuntime.cpp RawCampaignWorldRuntime.h
    RawCampaignPathBindings.cpp RawCampaignPathBindings.h
    RawCampaignMotionBindings.cpp RawCampaignMotionBindings.h
    RawCampaignGunQueries.cpp RawCampaignGunQueries.h
    UserInput.cpp UserInput.h
    Sound.cpp Sound.h
    SoundDevice.cpp SoundDevice.h
    SoundMusic.cpp SoundMusic.h
    SoundMusicFile.cpp SoundMusicFile.h
    SoundMusicSelection.cpp SoundMusicSelection.h
    SoundMusicRuntime.cpp SoundMusicRuntime.h
    SoundSceneMusic.cpp SoundSceneMusic.h
    SoundProcess.cpp SoundProcess.h
    RawCursorDrop.cpp RawCursorDrop.h
    RawCursorReturnPiece.cpp RawCursorReturnPiece.h
)
# 원본처럼 Win32 GDI·멀티미디어 타이머를 직접 쓴다(Screen·ClientMain).
# 시작 로딩의 실제 Windows 합성 표시 완료를 기다리는 DwmFlush를 연결한다.
target_link_libraries(netstorm PUBLIC user32 gdi32 winmm dwmapi)

# 실제 입자 색 할당/인접 메모리와 같은 캠페인의 Renderer/VFX 출력을 연결한다.
target_sources(netstorm PRIVATE ParticlePaletteMemory.cpp ParticlePaletteMemory.h RawCampaignParticles.cpp RawCampaignParticles.h)
# 실제 소유자 색표/색 변환 RLE를 같은 캡처 저장소에 연결한다.
target_sources(netstorm PRIVATE OwnerColorMaps.h OwnerColorMaps.cpp VFXMappedDraw.h VFXMappedDraw.cpp)
# 사제 건설 선택과 내려놓기의 취득/취소·임시 객체 수명을 공유한다.
target_sources(netstorm PRIVATE RawCursorLifecycle.h RawCursorLifecycle.cpp)
target_sources(netstorm PRIVATE RawCursorPlacement.h RawCursorPlacement.cpp)
target_sources(netstorm PRIVATE RawCursorPreviews.h RawCursorPreviews.cpp)
target_sources(netstorm PRIVATE RawCursorPosition.h RawCursorPosition.cpp)
# 같은 실제 Client Gump 트리에 메뉴·장식·제목·Slider를 조립한다.
target_sources(netstorm PRIVATE RawMenuControls.h RawMenuControls.cpp)

# 실제 운반물 위치와 발자국 탐색을 같은 메뉴/배치 파이프라인에 연결한다.
target_sources(netstorm PRIVATE RawCarrierDropPoint.h RawCarrierDropPoint.cpp)
# 실제 폭발 효과를 같은 월드의 기존 피해/지면/다리 처리기에 연결한다.
target_sources(netstorm PRIVATE RawBombBlastBindings.h RawBombBlastBindings.cpp)

# 공통 객체 메뉴 Open과 원본의 객체 중심 위치 계산을 연결한다.
target_sources(netstorm PRIVATE RawObjectMenuOpen.h RawObjectMenuOpen.cpp)

# 원본객체메뉴의4인자제목/현재상태정보를복원한다.
target_sources(netstorm PRIVATE RawObjectMenuInfo.h RawObjectMenuInfo.cpp)

# 실제 객체 메뉴의 원본 VT 슬롯과 Client/Session/입력 연결을 사용한다.
target_sources(netstorm PRIVATE RawObjectMenuSlots.h RawObjectMenuSlots.cpp ObjectMenuSlots.inc CampaignObjectMenuClient.h CampaignObjectMenuClient.cpp)

# 원본 비용 표시를 같은 TextGump/게임 시계/등록표에 연결한다.
target_sources(netstorm PRIVATE FloatingSpGump.h FloatingSpGump.cpp)

# 실제 생산 격자와 내용물 항목의 원본 생성/배치/통지를 복원한다.
target_sources(netstorm PRIVATE InventoryGump.h InventoryGump.cpp)

# 생산 아이콘의 실제 SHP Gump와 VFX 출력/투명 적중을 연결한다.
target_sources(netstorm PRIVATE ImageGump.h ImageGump.cpp)

# 실제 생산 내용물 항목의 자식 배치·입력·장식·비용을 복원한다.
target_sources(netstorm PRIVATE InventoryItemGump.h InventoryItemGump.cpp)

# 같은 생산 실패 안내 파생을 공통 Gump 트리에 연결한다.
target_sources(netstorm PRIVATE ProductionHintGump.h ProductionHintGump.cpp)

# 실제 생산 항목 우클릭 정보 작성기를 연결한다.
target_sources(netstorm PRIVATE InventoryAboutMenu.h InventoryAboutMenu.cpp)

# 실제 Quick Help 요청의 주제/이력/창·가상 부착 계층을 복원한다.
target_sources(netstorm PRIVATE QuickHelpRequest.h QuickHelpRequest.cpp)
# 실제 전용 도움말 창의 재사용·이력/팝업 상태·전체 입력 분배를 복원한다.
target_sources(netstorm PRIVATE QuickHelpControl.h QuickHelpControl.cpp)
# 실제 전용 Help의 주제/TYPE 정보·링크 저장소와 마나 그림 문자를 기존 HTML에 연결한다.
target_sources(netstorm PRIVATE QuickHelpHtml.h QuickHelpHtml.cpp HtmlLinkText.h HtmlLinkText.cpp)
# 실제 전용 Help의 전체 생성/배치/본문·그림 갱신과 HTML 부모를 같은 기존 Gump에 연결한다.
target_sources(netstorm PRIVATE QuickHelpWindow.h QuickHelpWindow.cpp HtmlBodyGump.h HtmlBodyGump.cpp)
# 실제 고정 색상 입력과 초기 say를 보존하는 같은 Client 명령 표를 등록한다.
target_sources(netstorm PRIVATE ClientColorSortInput.h ClientColorSortInput.cpp
    RawDotCommandRegister.h RawDotCommandRegister.cpp ClientDotCommandGlobals.h ClientDotCommandGlobals.cpp)
# 편집 맵의 pending 이름과 다음 State 덮어쓰기 및 전투 Fort 보존 순서다.
target_sources(netstorm PRIVATE FortSelection.h FortSelection.cpp)
# Client와 모든 Session이 같은 Subscriber 주소 및 할당 입력 장치를 빌린다.
target_sources(netstorm PRIVATE ClientSubscriberGlobals.h CurrentAllocationBytes.h)
# 실제 Player/factory 할당과 세션 고정뷰의 수명 관문을 단일 Client에 둔다.
target_sources(netstorm PRIVATE ClientPlayerRecords.h ClientPlayerRecords.cpp)

# 실제 편집 메모리와 공통 UI 조립 계층을 함께 빌드한다.
target_sources(netstorm PRIVATE TextEntryStorage.cpp TextEntryStorage.h RawTextEntryContext.h RawTextEntryControls.cpp RawTextEntryControls.h)

# 실제 Client 수명의 Chat 전역과 공통 UI 전체 공장을 빌드한다.
target_sources(netstorm PRIVATE ClientChatWindowGlobals.h RawChatWindowContext.h RawMenuChat.inc)

# 원본 Uber의 BackgroundGif 자식을 실제 공통 UI에 연결한다.
target_sources(netstorm PRIVATE BackgroundGifGump.h BackgroundGifGump.cpp RawBackgroundGifControls.h RawBackgroundGifControls.cpp)

# 같은 Client 전역과 HTML 부모의 실제 생성/본문/제어 수명을 공통 UI에 조립한다.
target_sources(netstorm PRIVATE HtmlWindowGump.cpp HtmlWindowGump.h ClientHtmlWindowGlobals.h RawHtmlWindowContext.h RawMenuHtml.inc)

# 실제 HTML 전체 Fit의 폭/높이·메뉴·버튼·가상 부착을 원문 순서로 계산한다.
target_sources(netstorm PRIVATE HtmlWindowFit.cpp HtmlWindowFit.h)

# 실제 HTML 메타의9종 우선순위/원시 캡처·버퍼/현재 명령과 UI 효과를 연결한다.
target_sources(netstorm PRIVATE HtmlWindowMetadata.cpp HtmlWindowMetadata.h)
