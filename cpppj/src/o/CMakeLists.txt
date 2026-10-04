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
    ChunkMap.cpp ChunkMap.h
    Islandbuilder.cpp Islandbuilder.h
    BaseProcess.cpp BaseProcess.h
    Kernel.cpp Kernel.h
    GameClock.cpp GameClock.h
)
