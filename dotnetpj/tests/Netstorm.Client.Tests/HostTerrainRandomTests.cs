using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>호스트 세션이 지형 생성 전용 난수 쌍을 분리 소유하는지 검사한다.
/// 근거: 0.446 확정(세션 전용 TerrainRandom+TerrainCrt, SP·호스트 스트림 분리). (2026-11-12 0.447에서 추가했다)</summary>
public sealed class HostTerrainRandomTests
{
    /// <summary>세션이 지형 난수 쌍을 초기값으로 따로 소유하는지 확인한다.</summary>
    [Fact]
    public void OwnsTerrainRandomPairWithInitialValues()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.NotNull(session.TerrainRandom);
        Assert.NotNull(session.TerrainCrt);
        Assert.Equal(0u, session.TerrainRandom.State);
        Assert.Equal(0u, session.TerrainCrt.Value);
    }

    /// <summary>세션마다 난수 쌍이 공유되지 않는지 확인한다.</summary>
    [Fact]
    public void TerrainRandomPairIsNotSharedAcrossSessions()
    {
        var first = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        var second = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.NotSame(first.TerrainRandom, second.TerrainRandom);
        Assert.NotSame(first.TerrainCrt, second.TerrainCrt);
    }
}
