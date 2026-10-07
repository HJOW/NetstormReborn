# 원본 \Ns\O\ 폴더의 공용 모듈 (Squid·Template·Player·Kernel·Bridge·Rifttype·Config·Xlat 등)
# 클라이언트와 서버가 함께 쓰던 코드로 보인다. 이 폴더의 코드는 client/ 와 platform/ 을 포함하지 않는다.
# 다시 만든 파일을 아래 목록에 더한다. 파일 이름은 SOURCE_MAP.md 의 "cpppj 경로"를 따른다.
target_sources(netstorm PRIVATE
    BaseFile.cpp BaseFile.h
    Config.cpp Config.h
    ConfigInterface.cpp ConfigInterface.h
    Xlat.cpp Xlat.h
    OriginalText.cpp OriginalText.h
    RiftType.cpp RiftType.h
    TypeParser.cpp TypeLoadOrder.cpp RiftTypeTable.cpp
    Template.cpp Template.h
    CanonDecoder.cpp CanonDecoder.h CanonDecoderTables.inc
    Bridge.cpp Bridge.h
    RawBridgeLifecycle.cpp RawBridgeLifecycle.h
    RawSquidFinder.cpp RawSquidFinder.h
    RawSquidDestroy.cpp RawSquidDestroy.h
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
    SquidFactory.cpp SquidFactory.h TypeConstructors.inc DerivedConstructors.inc
    SquidUnpop.cpp SquidUnpop.h UnpopVtables.inc
    SquidPop.cpp SquidPop.h PopVtables.inc
    SquidDisplay.cpp SquidDisplay.h
    SquidPostPop.cpp SquidPostPop.h
    SpStore.cpp SpStore.h
    SquidReward.cpp SquidReward.h
    Graph.cpp Graph.h
    RawGraph.cpp RawGraph.h
    BaseProcess.cpp BaseProcess.h
    Kernel.cpp Kernel.h
    GameClock.cpp GameClock.h
)
