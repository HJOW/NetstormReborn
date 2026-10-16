# 원본 클라이언트 폴더(assert 문자열의 ".\")의 모듈
# (ClientMain·UserInput·Gump 계열·Screen·Renderer·Sound·State·Mission 등)
# 다시 만든 파일을 아래 목록에 더한다. 파일 이름은 SOURCE_MAP.md 의 "cpppj 경로"를 따른다.
target_sources(netstorm PRIVATE
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
    GameWorld.cpp GameWorld.h
    RawSurfaceWorld.cpp RawSurfaceWorld.h
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
target_link_libraries(netstorm PUBLIC user32 gdi32 winmm)

# 실제 입자 색 할당/인접 메모리와 같은 캠페인의 Renderer/VFX 출력을 연결한다.
target_sources(netstorm PRIVATE ParticlePaletteMemory.cpp ParticlePaletteMemory.h RawCampaignParticles.cpp RawCampaignParticles.h)
# 실제 소유자 색표/색 변환 RLE를 같은 캡처 저장소에 연결한다.
target_sources(netstorm PRIVATE OwnerColorMaps.h OwnerColorMaps.cpp VFXMappedDraw.h VFXMappedDraw.cpp)
