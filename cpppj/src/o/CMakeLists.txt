# 원본 \Ns\O\ 폴더의 공용 모듈 (Squid·Template·Player·Kernel·Bridge·Rifttype·Config·Xlat 등)
# 클라이언트와 서버가 함께 쓰던 코드로 보인다. 이 폴더의 코드는 client/ 와 platform/ 을 포함하지 않는다.
# 다시 만든 파일을 아래 목록에 더한다. 파일 이름은 SOURCE_MAP.md 의 "cpppj 경로"를 따른다.
target_sources(netstorm PRIVATE
    RawDataManagerDump.cpp RawDataManagerDump.h #가상파일명/이름표와실제current의원문진단덤프다.
    RawBaseFileReadPolicy.cpp RawBaseFileReadPolicy.h #모드0 읽기의 실제 장치/접두/기본·보조 조회 순서다.
    RawForeignRitualReward.cpp RawForeignRitualReward.h #비로컬 SP5000/기술 Core 장부의 실제 보상이다.
    # 실제 저장상태 질문·Chat 랭크 접두·CoreData 선택과 Kencloud 계산을 원문 부모로 사용한다.
    RawStartupStateCheck.cpp RawStartupStateCheck.h
    RawChatManagerPrefix.cpp RawChatManagerPrefix.h
    RawPlayerCoreData.cpp RawPlayerCoreData.h
    RawKencloud.cpp RawKencloud.h
    # Editor에서도 manager·Metamap 진단을 수행하는 실제 CoreData 저장 부모다.
    RawFortCoreWrite.cpp RawFortCoreWrite.h
    # 팔레트 초기 논리 RGB를 검색 객체로 고정하는 실제 생성 부모의 순서다.
    RawColorSortStartup.cpp RawColorSortStartup.h
    # 기존 Fort 팔레트 전환의 이름·Kencloud 수명과 장치 호출 순서다.
    RawFortPaletteTransition.cpp RawFortPaletteTransition.h
    # 기존 Editor의 실제 타입 변환·덱 로드·지형 회복과 마지막 맵 정리다.
    RawFortTypeNamesLoad.cpp RawFortTypeNamesLoad.h
    RawFortContentsLoad.cpp RawFortContentsLoad.h
    RawExistingEditorInstallOuter.cpp RawExistingEditorInstallOuter.h
    RawFortSubscriberLoad.cpp RawFortSubscriberLoad.h
    RawSubscriberIdGenerate.cpp RawSubscriberIdGenerate.h
    RawFortDeckLoad.cpp RawFortDeckLoad.h
    RawDeckRestore.cpp RawDeckRestore.h
    RawFortTerritoryRecover.cpp RawFortTerritoryRecover.h
    RawEditorWorldFinish.cpp RawEditorWorldFinish.h
    # 전체 Fort 저장 부모와 실제 Money·Mission·Badges 기록의 원문 순서다.
    RawFortSnapshotParent.cpp RawFortSnapshotParent.h
    RawFortMoneySave.cpp RawFortMoneySave.h
    RawFortMissionSave.cpp RawFortMissionSave.h
    RawFortBadgesSave.cpp RawFortBadgesSave.h
    RawFortBadgesGlobals.cpp RawFortBadgesGlobals.h
    # Editor 플레이어 초기화와 현재 Fort 이름의 원문 부수 효과다.
    RawEditorPlayers.cpp RawEditorPlayers.h
    RawFortCurrentName.cpp RawFortCurrentName.h
    # 같은 실제 Temp/rename 부모와 Territory/Chaff 전체 청크 순회다.
    RawFortObjectsSave.cpp RawFortObjectsSave.h
    RawFortRegionSave.cpp RawFortRegionSave.h
    # 원소별 현재 기술의 조회/중복 방지/실제 contained 생성 순서다.
    RawThemeTechnology.cpp RawThemeTechnology.h
    # AI 다리의 여섯 탐색 단계·조각 소비·연결점 진행이다.
    RawAiBridgePath.cpp RawAiBridgePath.h
    RawAiBridgeCandidates.cpp RawAiBridgeCandidates.h
    RawAiBridgeSearch.cpp RawAiBridgeSearch.h
    RawAiBridgePlanning.cpp RawAiBridgePlanning.h
    RawAiBridgeStrategies.cpp RawAiBridgeStrategies.h
    RawAiRuntime.cpp RawAiRuntime.h
    # DataManager 목록의 unlink와 실제 MemStream 버퍼/섹션 소멸 순서다.
    RawDataManagerLifetime.cpp RawDataManagerLifetime.h
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
    RawFlyerTarget.cpp RawFlyerTarget.h
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
    RawBulfFrame.cpp RawBulfFrame.h
    RawWalkerFrame.cpp RawWalkerFrame.h
    RawPathSearch.cpp RawPathSearch.h
    RawFactoryPostPop.cpp RawFactoryPostPop.h
    RawMetadisplayDirty.cpp RawMetadisplayDirty.h
    RawGunTarget.cpp RawGunTarget.h
    # 스톤 타워 배치가 기존 Gun의 표적을 바꾸는 실제 주변 수집/통지다.
    RawGunBlocker.cpp RawGunBlocker.h
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
    RawCrossbowAnimation.cpp RawCrossbowAnimation.h
    RawCannonFire.cpp RawCannonFire.h
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
    RawIslandPlacement.cpp RawIslandPlacement.h # 편집기의 실제 임시 지도 충돌 판정이다.
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
    RawPlayerIdentity.cpp RawPlayerIdentity.h
    # 실제 캠페인 부모와 Player 예약·저장 State 및 Burning 프레임 복원이다.
    RawCampaignWorldLoad.cpp RawCampaignWorldLoad.h
    RawPlayerRegular.cpp RawPlayerRegular.h
    RawPlayerTemplateState.cpp RawPlayerTemplateState.h
    RawTemplateLifetime.cpp RawTemplateLifetime.h
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
# 실제 AI 피격의 네 목록과 조건부 공격자·홈 반응을 같은 본체 메모리에 적용한다.
target_sources(netstorm PRIVATE RawAiDamage.cpp RawAiDamage.h)

# 실제 Fiery Tail/건설/불타는 표시가 공유하는 RGB 색 띠와 팔레트 병합이다.
target_sources(netstorm PRIVATE ParticleGradient.cpp ParticleGradient.h)

# 실제 Fiery Tail의 시간 조각·난수·입자와 선 출력 요청 몸체다.
target_sources(netstorm PRIVATE RawFieryTailFrame.cpp RawFieryTailFrame.h)

# 실제 Fiery Tail 생성·공통 표시 등록·120입자 확보와 반납 순서다.
target_sources(netstorm PRIVATE RawFieryTailLifetime.cpp RawFieryTailLifetime.h)

# 모든 입자 효과의 원본 단일 표시 목록과 실제 Host 부착/공통 수집·분배다.
target_sources(netstorm PRIVATE RenderProcessSystem.cpp RenderProcessSystem.h)

# 원본 Fiery Tail 몸체를 실제 같은 Kernel/공통 표시 목록에 부착한다.
target_sources(netstorm PRIVATE FieryTailVisualSystem.cpp FieryTailVisualSystem.h)

# 반더 타워/맨 오 워 공유 빔의 snapshot 경로와 재귀 입자 생성이다.
target_sources(netstorm PRIVATE RawLightningPath.cpp RawLightningPath.h)
# 같은 공유 빔의 실제 시간 조각/전체 용량 수명과 굵기 선을 복원한다.
target_sources(netstorm PRIVATE RawLightningFrame.cpp RawLightningFrame.h)
# 공유 빔의 생성/색/경로 반납과 실제 process19 Host/Kernel 소유권이다.
target_sources(netstorm PRIVATE RawLightningLifetime.cpp RawLightningLifetime.h LightningVisualSystem.cpp LightningVisualSystem.h)
# 실제 전투 플라이어 Pre와 별도 CRT 스트림/픽셀 파편 수치·표시 본체다.
target_sources(netstorm PRIVATE RawFlyerPreDestroy.cpp RawFlyerPreDestroy.h CrtRandom.cpp CrtRandom.h RawShrapnelFrame.cpp RawShrapnelFrame.h)
# 실제 bitmap/입자 반납과 dummy/process16의 공통 Host/Kernel 수명이다.
target_sources(netstorm PRIVATE RawShrapnelLifetime.cpp RawShrapnelLifetime.h ShrapnelVisualSystem.cpp ShrapnelVisualSystem.h)

# 사제와 Carrier의 실제 내용물 부모 가상 통지를 공유한다.
target_sources(netstorm PRIVATE RawCarrierContentNotice.h RawCarrierContentNotice.cpp)

# 실제 마나 볼트 도착 분기를 연결한다.
target_sources(netstorm PRIVATE RawManaBoltArrival.cpp RawManaBoltArrival.h)

# 실제 비행/마나 도착이 공유하는 ProcessFinder 전체 제거다.
target_sources(netstorm PRIVATE RawProcessRemoval.cpp RawProcessRemoval.h)

# 실제 마나 볼트의 주기 프레임/현재 대상 옆 위치 조정이다.
target_sources(netstorm PRIVATE RawManaBoltRegular.cpp RawManaBoltRegular.h)

# 실제 Boardcoord 가장자리/Graph 접근점이다.
target_sources(netstorm PRIVATE RawBoardApproach.cpp RawBoardApproach.h)

# 실제25바이트 로컬 건설 요청과 서버 수신 처리기다.
target_sources(netstorm PRIVATE RawConstructionCommand.cpp RawConstructionCommand.h)

# Factory 기본 프레임과 별도 추가 그림을 실제 현재 슬롯에서 조회한다.
target_sources(netstorm PRIVATE RawFactoryDraw.cpp RawFactoryDraw.h)

# 실제 연구58h/예측 교체/Factory 내용물 통지의 독립 몸체다.
target_sources(netstorm PRIVATE RawFactoryResearch.cpp RawFactoryResearch.h)

# 연구 효과의 원본 생성/Regular/난수/수명을 연결한다.
target_sources(netstorm PRIVATE RawFlare.cpp RawFlare.h)

# 현재 생산Deck/기술 부모의 원본teleport 내용물 동기화다.
target_sources(netstorm PRIVATE RawProductionSync.cpp RawProductionSync.h)

# 신전 건설 중 피해와 윈드 타워의 현재 방향 방어를 같은 월드에 연결한다.
target_sources(netstorm PRIVATE RawSpecialDamage.cpp RawSpecialDamage.h)

# 실제 훨리기그 전용 초기화/폭탄/통지/장식 프레임이다.
target_sources(netstorm PRIVATE RawSunFlyerFrame.cpp RawSunFlyerFrame.h)

# 세 템플의 현재 기본 프레임과 윈드/썬더 전용Draw를 같은 원본 자료로 처리한다.
target_sources(netstorm PRIVATE RawTempleDraw.cpp RawTempleDraw.h)

# 실제 Burning14의 출력/해제와 공통 프레임gate를 같은 원본 자료에 연결한다.
target_sources(netstorm PRIVATE RawBurningLifetimeDraw.cpp RawBurningLifetimeDraw.h BurningVisualSystem.cpp BurningVisualSystem.h)

# 실제131의 방향별 기본/추가 Draw를 Factory와 구별하여 캡처에 연결한다.
target_sources(netstorm PRIVATE RawThunderCannonCapture.cpp RawThunderCannonCapture.h)

# 실제72의 생산 기본/추가 가상 그림을 파편·건설·화염 캡처에 제공한다.
target_sources(netstorm PRIVATE RawSunAviaryCapture.cpp RawSunAviaryCapture.h)

# 간헐천 고정 본체 뒤 현재 애니메이션을 캡처와 실제 표시에 공유한다.
target_sources(netstorm PRIVATE RawGeyserCapture.cpp RawGeyserCapture.h)

# 다이스의 별도 TYPE169 추가 그림을 실제 표시와 캡처에 공유한다.
target_sources(netstorm PRIVATE RawDaisDraw.cpp RawDaisDraw.h)

# Dude 부모의 정지/도착·표면 전환과 population 후처리다.
target_sources(netstorm PRIVATE RawDudeBehavior.cpp RawDudeBehavior.h)

# Dude의 실제 정기/네 하위 처리와30칸 초기 목록을 같은 게임 상태에 연결한다.
target_sources(netstorm PRIVATE RawDudeRegular.cpp RawDudeRegular.h)

# 현재 DataManager 섹션 검색/선택을 같은 Template 저장소에 연결한다.
target_sources(netstorm PRIVATE RawDataManagerSelection.cpp RawDataManagerSelection.h)

# 원본 MemStream 전체 처리와 같은 DataManager 추가/제거를 실제 템플릿 소비자에 공유한다.
target_sources(netstorm PRIVATE RawMemStream.cpp RawMemStream.h RawDataManagerList.cpp RawDataManagerList.h)

# 같은 원문/목록에서 최소 확장·임시 생성·이름 교체의 전체 부모를 연결한다.
target_sources(netstorm PRIVATE RawDataManagerResize.cpp RawDataManagerResize.h)

# 실제 파일 이미지의 섹션 병합/mark/이름 표 진행 부모를 같은 저장소에 연결한다.
target_sources(netstorm PRIVATE RawDataManagerImage.cpp RawDataManagerImage.h)
# Template manager의 같은 파일명/생성자와 바깥 Load의 소유권 분기를 복원한다.
target_sources(netstorm PRIVATE RawTemplateLoad.cpp RawTemplateLoad.h)
# 보상/차감의 실제 AI 지갑 가산을 같은 Player/본체에 연결한다.
target_sources(netstorm PRIVATE RawAiWallet.cpp RawAiWallet.h)
# 생성/삭제의 실제 AI 타입 수·목록·타이머를 같은 본체에 연결한다.
target_sources(netstorm PRIVATE RawAiObjectNotice.cpp RawAiObjectNotice.h)
# 같은 본체의 초기 자산 등록·간헐천 분류와 매 판단 전 준비를 복원한다.
target_sources(netstorm PRIVATE RawAiWorldRegistration.cpp RawAiWorldRegistration.h RawAiPreparation.cpp RawAiPreparation.h)
# 원본 공통 AI의 첫 전략 준비와 목표 선택을 같은 실제 월드에 연결한다.
target_sources(netstorm PRIVATE RawAiStrategySetup.cpp RawAiStrategySetup.h)
# 원본 AI 행동 말미의 지갑/분산SP 정리를 같은 실제 저장소에서 실행한다.
target_sources(netstorm PRIVATE RawAiEconomy.cpp RawAiEconomy.h)
# 원본 공통 AI의 생산 조각 소비/대기/비용과 실제 건설 배치를 연결한다.
target_sources(netstorm PRIVATE RawAiDeployment.cpp RawAiDeployment.h)
# 원본 수집자 선택/자기 영역 무작위 후보/실제 생산 배치를 같은 월드에 연결한다.
target_sources(netstorm PRIVATE RawAiCollectors.cpp RawAiCollectors.h)
# 원본 AI의 원소별 생산 선택과 시설의 미처리/업그레이드 대기 목록을 처리한다.
target_sources(netstorm PRIVATE RawAiFactoryProduction.cpp RawAiFactoryProduction.h)
# 기존58h 연구와 같은 메시지의 업그레이드 요청/수신 접두를 실제 Host/SP에 연결한다.
target_sources(netstorm PRIVATE RawFactoryUpgradeCommand.cpp RawFactoryUpgradeCommand.h)
# 원본Factory의예측그림/완료/예약·종속그림정리를같은Host에연결한다.
target_sources(netstorm PRIVATE RawFactoryUpgrade.cpp RawFactoryUpgrade.h)
# 실제Factory입력의자금/진행중안내와편집상태순환을기존연구/공통부모에합성한다.
target_sources(netstorm PRIVATE RawFactoryInput.cpp RawFactoryInput.h)
# 원본AI의에너지부족좌표중복/순환검사/압축을같은실제본체에연결한다.
target_sources(netstorm PRIVATE RawAiDrySpots.cpp RawAiDrySpots.h)
# AI로그와객체메뉴가원본가상이름/같은레벨문자표를공유한다.
target_sources(netstorm PRIVATE RawSquidName.cpp RawSquidName.h SquidNameVtables.inc)
# 사제/수집자판단이공유할실제AI경로조회·정지·이동·복귀를연결한다.
target_sources(netstorm PRIVATE RawAiMovement.cpp RawAiMovement.h)
# 같은실제간헐천/포획운반목록의자원선택과신전귀환하위를연결한다.
target_sources(netstorm PRIVATE RawAiResourceChoice.cpp RawAiResourceChoice.h)
# 전체수집/포획행동과실제내용물genus조회·다이스접근점을연결한다.
target_sources(netstorm PRIVATE RawAiCarrierActions.cpp RawAiCarrierActions.h)
# 사제AI의현재spot/배치허용/벽시각을공유하는실제행별탐색을연결한다.
target_sources(netstorm PRIVATE RawAiBuildSearch.cpp RawAiBuildSearch.h)
# 사제AI의다음건설TYPE/수집자비용여유/실제적지갑조건을연결한다.
target_sources(netstorm PRIVATE RawAiBuildChoice.cpp RawAiBuildChoice.h)
# 실제사제의의식/건설/탐색/회수/수집·귀환전체행동을연결한다.
target_sources(netstorm PRIVATE RawAiPriestActions.cpp RawAiPriestActions.h)
# AI 건물 배치의 실제 표적 점수와 Gun/장애물 후보 평가를 연결한다.
target_sources(netstorm PRIVATE RawAiDropTargets.cpp RawAiDropTargets.h)
# AI 별도지상검색/Depend필터와실제에너지·배치후보평가를연결한다.
target_sources(netstorm PRIVATE RawGunGroundTarget.cpp RawGunGroundTarget.h RawAiDropEvaluation.cpp RawAiDropEvaluation.h)
# 실제다리주변후보/원본예산스캔과같은본체30후보목록을연결한다.
target_sources(netstorm PRIVATE RawAiDropScan.cpp RawAiDropScan.h)
# 실제기술목록/직전후보·회전/블로커우선과쿨다운의다음배치TYPE를연결한다.
target_sources(netstorm PRIVATE RawAiDropSelection.cpp RawAiDropSelection.h)
# 가짜 부족좌표의 원본 수직 양방향 탐색/현재 지도/실제 디버그 표식을 연결한다.
target_sources(netstorm PRIVATE RawAiDryGeometry.cpp RawAiDryGeometry.h)
# 현재 공급시설/부족거리/같은 양방향 탐색으로 전체 가짜 부족좌표를 연결한다.
target_sources(netstorm PRIVATE RawAiFakeDrySpots.cpp RawAiFakeDrySpots.h)
# 현재 선택/예산스캔·생산/사제 관문과 실제 배치·성공표적/후보 압축을 연결한다.
target_sources(netstorm PRIVATE RawAiDropActions.cpp RawAiDropActions.h)
# 단독/편집 맵의 같은50개 캐시와 원본 목록 행 표시를 연결한다.
target_sources(netstorm PRIVATE RawSoloMapList.cpp RawSoloMapList.h)
# 원본 실제 파일 저장 부모의 현재 관문과 파일 장치 순서를 복원한다.
target_sources(netstorm PRIVATE RawDataManagerFileSave.h RawDataManagerFileSave.cpp)

# 새 맵의 같은 manager 초기화, 공유 SNART 이름표와 Subscriber 저장 부모를 등록한다.
target_sources(netstorm PRIVATE RawFortSubscriberWrite.cpp RawFortSubscriberWrite.h
    RawSnart.cpp RawSnart.h RawDataManagerInitialize.cpp RawDataManagerInitialize.h
    RawTemplateSectionNames.cpp RawTemplateSectionNames.h)

# 신규 Fort manager/초기 자료/파일 부재 부모의 원문 제어 순서를 복원한다.
target_sources(netstorm PRIVATE RawNewFort.cpp RawNewFort.h)

# 실제 섬 추가의 지형·초기 객체 교체·Territory 후처리 부모를 등록한다.
target_sources(netstorm PRIVATE RawIslandAdd.cpp RawIslandAdd.h RawIslandPopulation.cpp RawIslandPopulation.h)
# 원본 섬 미리보기의 실제8비트 래스터와 Client 분류 캐시를 등록한다.
target_sources(netstorm PRIVATE RawTerrainBitmap.cpp RawTerrainBitmap.h)
# 더스트 데빌의 전용104BYTE·애니메이션·범위 피해를 실제 월드에서 사용한다.
target_sources(netstorm PRIVATE RawWindFlyer.h RawWindFlyer.cpp)

# 아크 스파이어의 전용 동맹 연결·범위·연쇄 피해를 복원한다.
target_sources(netstorm PRIVATE RawArcSpire.h RawArcSpire.cpp)
# 애시드 분해의 실제 픽셀 입자와 최초 시계/표면 표시를 복원한다.
target_sources(netstorm PRIVATE RawDissolveFrame.h RawDissolveFrame.cpp)
# 같은 표시 목록/실제 Kernel과 dummy 상호 감시로 분해 수명을 연결한다.
target_sources(netstorm PRIVATE RawDissolveLifetime.h RawDissolveLifetime.cpp DissolveVisualSystem.h DissolveVisualSystem.cpp)
# 애시드 포획 수신/헤더와 검증한 일반 유닛의 실제 캡처 분배다.
target_sources(netstorm PRIVATE RawAcidZap.h RawAcidZap.cpp RawPlainUnitCapture.h RawPlainUnitCapture.cpp)
target_sources(netstorm PRIVATE RawAcidFence.h RawAcidFence.cpp)
target_sources(netstorm PRIVATE RawIceTower.h RawIceTower.cpp)
target_sources(netstorm PRIVATE RawBulwarkHit.h RawBulwarkHit.cpp)
target_sources(netstorm PRIVATE RawPlainWorkshopCapture.h RawPlainWorkshopCapture.cpp)
target_sources(netstorm PRIVATE RawPlainAssetDraw.h RawPlainAssetDraw.cpp)
target_sources(netstorm PRIVATE RawBombEffectTimer.h RawBombEffectTimer.cpp)
target_sources(netstorm PRIVATE RawPriestPrayer.h RawPriestPrayer.cpp)
target_sources(netstorm PRIVATE RawBombTimerPacket.h RawBombTimerPacket.cpp)
target_sources(netstorm PRIVATE RawBombStatus.h RawBombStatus.cpp)
target_sources(netstorm PRIVATE RawWindCircle.h RawWindCircle.cpp)
target_sources(netstorm PRIVATE RawWindSpell.h RawWindSpell.cpp)

target_sources(netstorm PRIVATE RawPriestPanic.h RawPriestPanic.cpp)
# 같은 Player/Core/stream 물리와 필수 장치를 빌리는 실제 저장·정리·색상 캐시 부모다.
target_sources(netstorm PRIVATE RawStateWorldCleanup.h RawStateWorldCleanup.cpp
    RawSavedStateRead.h RawSavedStateRead.cpp RawFortSpWrite.h RawFortSpWrite.cpp
    RawPlayerCoreRefresh.h RawPlayerCoreRefresh.cpp RawColorMapCache.h RawColorMapCache.cpp)
# 원문 시계 전역·Core 기록·Player 링크·메타맵 표를 각각 같은 물리에 적용한다.
target_sources(netstorm PRIVATE RawClockSyncReset.h RawClockSyncReset.cpp
    RawPlayerCoreRecords.h RawPlayerCoreRecords.cpp RawPlayerLinksCleanup.h RawPlayerLinksCleanup.cpp
    RawMetaZoneCleanup.h RawMetaZoneCleanup.cpp)
# 최초 80행 관측으로 복원한 ColorSort 기록을 실제 Client 소유자에 연결한다.
target_sources(netstorm PRIVATE RawColorSortPrimary.h RawColorSortPrimary.cpp)
# 이전 공장/Player 해제와 같은 현재base 재조회로236BYTE 전체 초기화를 실행한다.
target_sources(netstorm PRIVATE RawPlayerInitialization.h RawPlayerInitialization.cpp)

# 원본10.78의 전체 다리 품질 변경과 같은 풀/게임 난수/표시 경계다.
target_sources(netstorm PRIVATE RawBridgeEditor.h RawBridgeEditor.cpp)

# 원본 아이스 캐논의80BYTE 발사와4초/6초 전환을 같은 공통 Fire에 연결한다.
target_sources(netstorm PRIVATE RawIceCannonFire.h RawIceCannonFire.cpp)

# 객체별 가상 진단과 모든 Form의 재귀 출력 순서를 같은 물리 풀에서 처리한다.
target_sources(netstorm PRIVATE RawObjectTrace.h RawObjectTrace.cpp)

# Base/내장Form의 TYPE·현재필드·원래서식을 실제 진단에 제공한다.
target_sources(netstorm PRIVATE RawObjectTraceFields.h RawObjectTraceFields.cpp)

# 83개 실제 자산의 Main/Task와 네 내장Form 진단을 원래 가상 표로 분배한다.
target_sources(netstorm PRIVATE RawObjectTraceMain.h RawObjectTraceMain.cpp)

# 실제 Editor의 객체 읽기/생성 교대를 현재 MemStream과 같은 월드에 연결한다.
target_sources(netstorm PRIVATE RawFortObjectStream.h RawFortObjectStream.cpp)
