# 원본 \Ns\O\ 폴더의 공용 모듈 (Squid·Template·Player·Kernel·Bridge·Rifttype·Config·Xlat 등)
# 클라이언트와 서버가 함께 쓰던 코드로 보인다. 이 폴더의 코드는 client/ 와 platform/ 을 포함하지 않는다.
# 다시 만든 파일을 아래 목록에 더한다. 파일 이름은 SOURCE_MAP.md 의 "cpppj 경로"를 따른다.
target_sources(netstorm PRIVATE
    # 썬 바리케이드의 현재 프레임/소유자/연결 수 변경과 실제 사건 위임이다.
    RawFenceRegular.cpp RawFenceRegular.h
    RawFenceMarkerClear.cpp RawFenceMarkerClear.h
    # 원본 내부 표식/포스 필드 생성과 동맹 연결 재생성의 같은 월드 계산이다.
    RawFenceCreate.cpp RawFenceCreate.h RawFenceRebuild.cpp RawFenceRebuild.h
    # 오벨리스크의 signed 사건 횟수/현재 프레임/공유 난수 지연이다.
    RawBuriedRegular.cpp RawBuriedRegular.h
    # 맨 오 워 풀의 실제 후처리/48BYTE 프로세스/0x73 writer를 같은 Kernel에서 실행한다.
    RawRainAviaryProcess.cpp RawRainAviaryProcess.h
    RawFlyerProcess.cpp RawFlyerProcess.h
    RawFlyerFrame.cpp RawFlyerFrame.h
    # 총 방향과 같은 실제 소유자 색 표/섬 프레임 변경의 원문 계산이다.
    RawGunAngle.h RawPlayerColor.h
    # 원본 Guide 반복과 현재 실제 사제 참여 상태를 같은 저장소에서 처리한다.
    RawMissionGuide.cpp RawMissionGuide.h
    RawPriestParticipation.cpp RawPriestParticipation.h
    # 같은 미션 게이트와 제작 장부로 Early Missions3~6의 실제 단계 조건을 판정한다.
    RawTutorialLater.cpp RawTutorialLater.h
    # 같은 Player9의 실제 로컬 로그인과 벽시계 완료/중단 순서를 공유한다.
    RawMissionLogin.cpp RawMissionLogin.h
    RawMissionLoginTimer.cpp RawMissionLoginTimer.h
    # 실제 로컬 서버가 보낸 초기8종 패킷도 같은 Player/SP/State로 소비한다.
    RawMissionClientPackets.cpp RawMissionClientPackets.h
    # 같은 현재 Player 이름/IP와 원본 중앙섬 metadata를 실제 wire로 작성한다.
    RawMissionServerPackets.cpp RawMissionServerPackets.h
    # 현재 옵션의 범위/TYPE 비용과 생산 Deck의 실제 저장소를 복원한다.
    RawMissionBattleOptions.cpp RawMissionBattleOptions.h
    RawProductionDeck.cpp RawProductionDeck.h
    # 현재 Fort의 35섹션 이미지와 로그인 0x2a 송신 길이를 복원한다.
    RawFortImage.cpp RawFortImage.h
    RawMissionLoginPackets.cpp RawMissionLoginPackets.h
    # 로그인 뒤 같은 7DWORD 설정을 0x32로 송수신한다.
    RawMissionSettingsPacket.cpp RawMissionSettingsPacket.h
    # 실제 로컬 미션의 다섯 상태 블록을 공통 확정 순서로 전환한다.
    RawLocalMissionState.cpp RawLocalMissionState.h
    # 같은 현재 신전 목록/섬/Player를 원본 PreDestroy 순서로 정리한다.
    RawVortexPreDestroy.cpp RawVortexPreDestroy.h
    RawMissionFrameGate.cpp RawMissionFrameGate.h
    RawTutorialOne.cpp RawTutorialOne.h RawTotalMade.cpp RawTotalMade.h
    RawTutorialTwo.cpp RawTutorialTwo.h
    RawLocalGeyser.cpp RawLocalGeyser.h
    RawMissionStart.cpp RawMissionStart.h RawMissionAiSettings.cpp RawMissionAiSettings.h RawAiCreate.cpp RawAiCreate.h
    RawTextToken.cpp RawTextToken.h
    RawPlayerTechnology.cpp RawPlayerTechnology.h
    RawPlayerGeyserBounds.cpp RawPlayerGeyserBounds.h
    RawConstructionPacket.cpp RawConstructionPacket.h
    RawAiBrain.cpp RawAiBrain.h
    RawAiBrainInitialUpdate.cpp RawAiBrainInitialUpdate.h
    RawTypeRequiredEnergy.cpp RawTypeRequiredEnergy.h
    RawPlayerContentNotice.cpp RawPlayerContentNotice.h
    RawSpStoreNoticeBindings.cpp RawSpStoreNoticeBindings.h
    RawRainVortexDraw.cpp RawRainVortexDraw.h
    RawFenceTarget.cpp RawFenceTarget.h
    RawContentParentNotice.cpp RawContentParentNotice.h ContentNoticeVtables.inc
    RawPlayerDamageNotice.cpp RawPlayerDamageNotice.h
    RawPlayerParticipation.cpp RawPlayerParticipation.h
    RawFormPacket.cpp RawFormPacket.h
    RawProjectileGeometry.cpp RawProjectileGeometry.h
    RawSquidLevel.cpp RawSquidLevel.h
    RawGunMode.cpp RawGunMode.h
    RawVortexOwner.cpp RawVortexOwner.h
    RawMissionOutcome.cpp RawMissionOutcome.h RawRegionOwner.cpp RawRegionOwner.h
    RawFortObjectLoad.cpp RawFortObjectLoad.h
    RawFortFinishOwner.cpp RawFortFinishOwner.h
    RawGunProcess.cpp RawGunProcess.h
    RawPathRuntime.cpp RawPathRuntime.h
    RawPathProcess.cpp RawPathProcess.h
    RawPathSearch.cpp RawPathSearch.h
    RawFactoryPostPop.cpp RawFactoryPostPop.h
    RawMetadisplayDirty.cpp RawMetadisplayDirty.h
    RawGunTarget.cpp RawGunTarget.h
    RawPathData.cpp RawPathData.h
    RawPathStart.cpp RawPathStart.h
    RawGunAnimation.cpp RawGunAnimation.h
    RawAssetRegular.cpp RawAssetRegular.h
    RawRegionPaint.cpp RawRegionPaint.h
    RawMissile.cpp RawMissile.h
    RawGunHit.cpp RawGunHit.h
    RawVortexAscend.cpp RawVortexAscend.h
    RawFrameChoose.cpp RawFrameChoose.h
    RawGeyserMined.cpp RawGeyserMined.h
    RawPriestDamage.cpp RawPriestDamage.h
    RawDamageableBurning.cpp RawDamageableBurning.h
    RawPathCleanup.cpp RawPathCleanup.h
    RawTerrainStamp.cpp RawTerrainStamp.h
    RawSquidRegion.cpp RawSquidRegion.h
    RawPriestRitualActions.cpp RawPriestRitualActions.h
    RawArcherAnimation.cpp RawArcherAnimation.h
    RawIslandRuntime.cpp RawIslandRuntime.h
    RawSquidHitPoints.cpp RawSquidHitPoints.h
    RawRunePostPop.cpp RawRunePostPop.h
    RawTempleRitual.cpp RawTempleRitual.h
    RawTerrainContext.cpp RawTerrainContext.h
    RawIslandQueries.cpp RawIslandQueries.h
    RawDamageableHit.cpp RawDamageableHit.h
    RawFortRandomPlacement.cpp RawFortRandomPlacement.h
    RawFarmCluster.cpp RawFarmCluster.h
    RawTerrainGrowth.cpp RawTerrainGrowth.h
    RawGunImpact.cpp RawGunImpact.h
    RawSquidDefense.cpp RawSquidDefense.h
    RawArcherTypeData.cpp RawArcherTypeData.h
    RawDaisRegular.cpp RawDaisRegular.h
    RawAltarSacrifice.cpp RawAltarSacrifice.h
    RawDaisLifecycle.cpp RawDaisLifecycle.h
    RawIslandBuilder.cpp RawIslandBuilder.h
    RawFortMoneyLoad.cpp RawFortMoneyLoad.h
    RawDudeNotice.cpp RawDudeNotice.h
    RawDamageablePostDestroy.cpp RawDamageablePostDestroy.h
    RawBurningVisual.cpp RawBurningVisual.h
    RawFortMetaLoad.cpp RawFortMetaLoad.h
    RawFortWorldLoad.cpp RawFortWorldLoad.h
    RawDudeSpawn.cpp RawDudeSpawn.h
    RawDudeDrop.cpp RawDudeDrop.h
    RawBurningSources.cpp RawBurningSources.h
    RawGeyserAiRegistration.cpp RawGeyserAiRegistration.h
    RawPlayerWorld.cpp RawPlayerWorld.h
    # 실제 캠페인 부모와 Player 예약·저장 State 및 Burning 프레임 복원이다.
    RawCampaignWorldLoad.cpp RawCampaignWorldLoad.h
    RawPlayerRegular.cpp RawPlayerRegular.h
    RawPlayerTemplateState.cpp RawPlayerTemplateState.h
    RawBurningFrame.cpp RawBurningFrame.h
    RawPriestHalt.cpp RawPriestHalt.h
    RawCarrierArrival.cpp RawCarrierArrival.h
    RawPriestArrival.cpp RawPriestArrival.h
    RawDudeTask.cpp RawDudeTask.h
    RawDaisContentNotice.cpp RawDaisContentNotice.h
    BaseFile.cpp BaseFile.h
    Config.cpp Config.h
    ConfigInterface.cpp ConfigInterface.h
    Xlat.cpp Xlat.h
    OriginalText.cpp OriginalText.h
    RiftType.cpp RiftType.h
    TypeParser.cpp TypeLoadOrder.cpp RiftTypeTable.cpp
    Template.cpp Template.h
    CanonDecoder.cpp CanonDecoder.h CanonDecoderTables.inc
    CanonTypeDecoder.cpp CanonTypeDecoder.h CanonTypePatterns.inc
    Bridge.cpp Bridge.h
    RawBridgeLifecycle.cpp RawBridgeLifecycle.h
    RawBridgeEvents.cpp RawBridgeEvents.h
    RawBridgeConnect.cpp RawBridgeConnect.h
    RawIslandLifecycle.cpp RawIslandLifecycle.h
    RawIslandPostPop.cpp RawIslandPostPop.h
    RawSurfaceLifecycle.cpp RawSurfaceLifecycle.h
    RawSquidFinder.cpp RawSquidFinder.h
    RawSquidNeighbors.cpp RawSquidNeighbors.h
    RawSquidDestroy.cpp RawSquidDestroy.h
    RawSquidDestroyDispatch.cpp RawSquidDestroyDispatch.h
    SquidDestroyLifecycle.cpp SquidDestroyLifecycle.h
    ChunkMap.cpp ChunkMap.h
    Islandbuilder.cpp Islandbuilder.h
    TerrainBuilder.cpp TerrainBuilder.h
    Player.cpp Player.h
    Squid.cpp Squid.h
    SquidFinder.cpp SquidFinder.h
    SquidHash.cpp SquidHash.h
    SquidSpatial.cpp SquidSpatial.h
    SidPool.cpp SidPool.h
    SquidForms.cpp SquidForms.h
    SquidFactory.cpp SquidFactory.h TypeConstructors.inc DerivedConstructors.inc
    GumpFormHost.cpp GumpFormHost.h
    DependFormHost.cpp DependFormHost.h
    ContentFormHost.cpp ContentFormHost.h
    ContentFormLifetime.cpp ContentFormLifetime.h
    SquidFormDispatch.cpp SquidFormDispatch.h
    SquidUnpop.cpp SquidUnpop.h UnpopVtables.inc
    SquidPop.cpp SquidPop.h PopVtables.inc
    RawBuildingSpace.cpp RawBuildingSpace.h
    SquidDisplay.cpp SquidDisplay.h
    SquidFrame.cpp SquidFrame.h
    RawPathAnimation.cpp RawPathAnimation.h
    SquidFrameBinding.cpp SquidFrameBinding.h
    SquidPostPop.cpp SquidPostPop.h
    SpStore.cpp SpStore.h
    SquidReward.cpp SquidReward.h
    SquidOwner.cpp SquidOwner.h
    RawPriestOwner.cpp RawPriestOwner.h
    RawPriestPostPop.cpp RawPriestPostPop.h
    RawAssetPostPop.cpp RawAssetPostPop.h
    RawPriestState.cpp RawPriestState.h
    RawPriestRegen.cpp RawPriestRegen.h
    RawPriestPreDestroy.cpp RawPriestPreDestroy.h
    RawPriestForcefield.cpp RawPriestForcefield.h
    RawPriestShield.cpp RawPriestShield.h
    RawPriestPostPopTail.cpp RawPriestPostPopTail.h
    RawPriestSpawn.cpp RawPriestSpawn.h
    RawCanonPlacement.cpp RawCanonPlacement.h
    RawCanonPlacementPreview.cpp RawCanonPlacementPreview.h
    RawCanonPlacementCollision.cpp RawCanonPlacementCollision.h
    RawCanonPlacementTerrain.cpp RawCanonPlacementTerrain.h
    RawCanonPlacementPermission.cpp RawCanonPlacementPermission.h
    RawPlayerPlacementAnchor.cpp RawPlayerPlacementAnchor.h
    RawCanonPlacementSurrounding.cpp RawCanonPlacementSurrounding.h
    RawCanonPlacementRelations.cpp RawCanonPlacementRelations.h
    RawCanonPlacementPipeline.cpp RawCanonPlacementPipeline.h
    RawOutpostLifecycle.cpp RawOutpostLifecycle.h
    RawRegionOwnership.cpp RawRegionOwnership.h
    RawConstructionPlace.cpp RawConstructionPlace.h
    RawConstructionConfirm.cpp RawConstructionConfirm.h
    RawConstructionBuilder.cpp RawConstructionBuilder.h
    RawConstructionProcess.cpp RawConstructionProcess.h
    RawConstructionArrival.cpp RawConstructionArrival.h
    RawConstructionData.cpp RawConstructionData.h
    RawPacketDispatch.cpp RawPacketDispatch.h RawPacketLengths.h
    RawConstructionSelection.cpp RawConstructionSelection.h
    RawPriestLookup.cpp RawPriestLookup.h
    RawConstructionClear.cpp RawConstructionClear.h
    RawConstructionNotice.cpp RawConstructionNotice.h
    RawConstructionRequest.cpp RawConstructionRequest.h
    RawConstructionEffects.cpp RawConstructionEffects.h
    RawConstructionLocal.cpp RawConstructionLocal.h
    RawContainedCreate.cpp RawContainedCreate.h
    RawPriestPlacement.cpp RawPriestPlacement.h
    RawPriestPlacementPreview.cpp RawPriestPlacementPreview.h
    RawPriestPlacementCollision.cpp RawPriestPlacementCollision.h
    RawPriestPlacementGeometry.cpp RawPriestPlacementGeometry.h
    RawCanonPlacementGeometry.cpp RawCanonPlacementGeometry.h
    RawPriestPlacementShape.cpp RawPriestPlacementShape.h
    RawCanonPixelShape.cpp RawCanonPixelShape.h
    RawPriestPlacementTerrain.cpp RawPriestPlacementTerrain.h
    RawCarrierPostPop.cpp RawCarrierPostPop.h
    RawCarrierPreDestroy.cpp RawCarrierPreDestroy.h
    RawContainedFinder.cpp RawContainedFinder.h
    RawContainedLookup.cpp RawContainedLookup.h
    RawConstructionVisual.cpp RawConstructionVisual.h
    RawConstructionFrame.cpp RawConstructionFrame.h
    RawConstructionLifetime.cpp RawConstructionLifetime.h
    RawRenderCollection.cpp RawRenderCollection.h
    ConstructionVisualSystem.cpp ConstructionVisualSystem.h
    RawDamageablePreDestroy.cpp RawDamageablePreDestroy.h
    RawDamageableRelease.cpp RawDamageableRelease.h
    SquidProcess.cpp SquidProcess.h
    RawPriestFall.cpp RawPriestFall.h
    RawFrameAdvance.cpp RawFrameAdvance.h
    RawForcefieldLifecycle.cpp RawForcefieldLifecycle.h
    RawCarrierCheck.cpp RawCarrierCheck.h
    Graph.cpp Graph.h
    RawGraph.cpp RawGraph.h
    BaseProcess.cpp BaseProcess.h
    Kernel.cpp Kernel.h
    GameClock.cpp GameClock.h
)

# 썬 디스크의 방향별 modulo Regular는 글자별 진행기와 별도로 연결한다.
target_sources(netstorm PRIVATE RawDiscRegular.cpp RawDiscRegular.h)

# 실제 CRT/Unix 시각 재시드를 게임 난수와 같은 상태에 연결한다.
target_sources(netstorm PRIVATE RawGameRandomSeed.cpp RawGameRandomSeed.h)

# 현재 Player9/연결 객체 표의 원본 송신 분배를 실제 세션에 제공한다.
target_sources(netstorm PRIVATE RawBattleBroadcast.cpp RawBattleBroadcast.h)

# 저장된 buried의 QA와 최초 Regular 두 예약을 실제 공통 PostPop에 연결한다.
target_sources(netstorm PRIVATE RawBuriedPostPop.cpp RawBuriedPostPop.h)

# 썬 아비어리 최초 부착/프레임/상태 패킷의 실제 Kernel 객체다.
target_sources(netstorm PRIVATE RawSunAviaryProcess.cpp RawSunAviaryProcess.h)

# 썬 펜스 최초 후처리의 이웃/연결선/Regular를 같은 월드에 연결한다.
target_sources(netstorm PRIVATE RawFencePostPop.cpp RawFencePostPop.h RawFenceNeighbors.cpp RawFenceNeighbors.h RawFenceLine.cpp RawFenceLine.h)
