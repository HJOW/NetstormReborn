using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>호스트 세션이 월드 적재 공유 저장소 묶음을 같은 풀·규격으로 소유하는지 검사한다.
/// 근거: RawCampaignWorldLoad의 Load 인자(ChunkMap·섬 저장소·프레임 188·Template 180·전역). (2026-11-12 0.436에서 추가했다)</summary>
public sealed class HostWorldStoresTests
{
    /// <summary>세션이 월드 적재 저장소를 규격대로 소유하는지 확인한다.</summary>
    [Fact]
    public void OwnsWorldLoadStoresWithExpectedShapes()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.NotNull(session.World);
        Assert.NotNull(session.IslandModes);
        Assert.NotNull(session.IslandResidences);
        Assert.NotNull(session.IslandHash);
        Assert.NotNull(session.WorldGlobals);
        Assert.Equal(RawIslandStorage.Bytes, session.IslandRecords.Length);
        Assert.Equal(HostSession.WorldFrameCount, session.WorldFrames.Length);
        Assert.All(session.WorldFrames, frame => Assert.NotNull(frame));
        Assert.Equal(HostSession.WorldTemplateBytes, session.WorldTemplates.Length);
        Assert.Equal(0u, session.IslandChanged);
        Assert.Equal(0u, session.IslandRevision);
    }

    /// <summary>저장소 규격 상수가 원본 적재 본체와 같은 값인지 확인한다.</summary>
    [Fact]
    public void WorldStoreConstantsMatchLoaderContract()
    {
        Assert.Equal(188, HostSession.WorldFrameCount);
        Assert.Equal(180, HostSession.WorldTemplateBytes);
        Assert.Equal(3204, RawIslandStorage.Bytes);
    }
}
