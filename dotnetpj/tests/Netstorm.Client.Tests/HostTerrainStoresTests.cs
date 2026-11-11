using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>호스트 세션이 지형 생성 공유 상태 묶음을 원본 기본값으로 소유하는지 검사한다.
/// 근거: RawNormalTerrainWorldBinding의 바인딩 저장소와 TerrainGenerationInputs. (2026-11-12 0.444에서 추가했다)</summary>
public sealed class HostTerrainStoresTests
{
    /// <summary>세션이 지형 상태 저장소를 기본값으로 소유하는지 확인한다.</summary>
    [Fact]
    public void OwnsTerrainStoresWithOriginalDefaults()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.NotNull(session.TerrainGlobals);
        Assert.NotNull(session.PlacementState);
        Assert.NotNull(session.StampState);
        Assert.NotNull(session.TerrainDiagnostics);
        Assert.NotNull(session.PuzzlePieceBox);
        Assert.NotNull(session.GenerationInputs);
        Assert.Equal(97u, session.TerrainGlobals.IslandType);
        Assert.Equal(98u, session.TerrainGlobals.EdgeType);
        Assert.Equal(127, session.TerrainGlobals.NoRegion);
        Assert.Equal(97u, session.PlacementState.IslandType);
        Assert.Equal(121u, session.PlacementState.EdgeFarmType);
        Assert.Equal(97u, session.StampState.IslandType);
        Assert.Equal(98u, session.StampState.EdgeType);
        Assert.Equal(102u, session.StampState.FringeType);
        Assert.Equal(127, session.StampState.NoRegion);
        // 진단 묶음은 연결 전까지 비어 있다.
        Assert.Null(session.TerrainDiagnostics.Context);
        Assert.Null(session.TerrainDiagnostics.DecoderDiagnostics);
    }
}
