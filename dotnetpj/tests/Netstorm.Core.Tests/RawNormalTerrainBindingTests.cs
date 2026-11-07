// 일반지형 바인딩 계약의 분기·범위·연결 검사를 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 지형 바인딩 계약을 검증하는 테스트 클래스다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.
public sealed class RawNormalTerrainBindingTests
{
    /// <summary>같은 연결 전역으로 유효 장면을 만든다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    private static (TerrainContextGlobals Globals, FortRandomPlacementState Placement, TerrainStampState Stamp, List<OriginalTypeRecord> Types, RawSidPool Pool) Scene()
    {
        var globals = new TerrainContextGlobals();
        var placement = new FortRandomPlacementState();
        var stamp = new TerrainStampState();
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        types[(int)globals.IslandType].ConstructorAddress = OriginalConstructors.Address(pool.Edition, (int)globals.IslandType);
        types[(int)placement.EdgeFarmType].ConstructorAddress = OriginalConstructors.Address(pool.Edition, (int)placement.EdgeFarmType);
        return (globals, placement, stamp, types, pool);
    }

    /// <summary>일반 분기·내부 범위만 통과한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    [Fact]
    public void BranchAndBounds()
    {
        var valid = new NormalTerrainRequest(1u, new ChunkRect(3, 3, 5, 5), 127, NormalTerrainBranch.Normal);
        RawNormalTerrainChecks.RequireBranch(valid);
        RawNormalTerrainChecks.RequireBounds(valid);
        Assert.Throws<InvalidOperationException>(() => RawNormalTerrainChecks.RequireBranch(valid with { Branch = NormalTerrainBranch.Archipelago }));
        Assert.Throws<InvalidOperationException>(() => RawNormalTerrainChecks.RequireBranch(valid with { Branch = NormalTerrainBranch.UnrestoredParent }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.RequireBounds(valid with { Bounds = new ChunkRect(0, 3, 5, 5) }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.RequireBounds(valid with { Bounds = new ChunkRect(3, 0, 5, 5) }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.RequireBounds(valid with { Bounds = new ChunkRect(3, 3, 16, 5) }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.RequireBounds(valid with { Bounds = new ChunkRect(3, 3, 5, 16) }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.RequireBounds(valid with { Bounds = new ChunkRect(5, 3, 5, 5) }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.RequireBounds(valid with { Bounds = new ChunkRect(3, 5, 5, 5) }));
    }

    /// <summary>군도 상태에서는 일반 전용 요청을 거부한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    [Fact]
    public void NormalRejectsArchipelago()
    {
        (TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, List<OriginalTypeRecord> types, RawSidPool pool) = Scene();
        var farm = new FarmClusterState();
        RawNormalTerrainChecks.RequireNormal(farm, globals, placement, stamp, types, pool);
        farm.Archipelago = true;
        Assert.Throws<InvalidOperationException>(() => RawNormalTerrainChecks.RequireNormal(farm, globals, placement, stamp, types, pool));
        Assert.Throws<ArgumentNullException>(() => RawNormalTerrainChecks.RequireNormal(null!, globals, placement, stamp, types, pool));
    }

    /// <summary>타입·지역 전역 불일치와 생성자 단절을 거부한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    [Fact]
    public void StorageGuardsConnections()
    {
        (TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, List<OriginalTypeRecord> types, RawSidPool pool) = Scene();
        var farm = new FarmClusterState();
        RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool);
        placement.IslandType = 98u;
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool));
        placement.IslandType = 97u;
        stamp.IslandType = 99u;
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool));
        stamp.IslandType = 97u;
        stamp.EdgeType = 99u;
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool));
        stamp.EdgeType = 98u;
        stamp.NoRegion = 0;
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool));
        stamp.NoRegion = 127;
        types[(int)globals.IslandType].ConstructorAddress += 1u;
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool));
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireNormal(farm, globals, placement, stamp, types, pool));
        globals.IslandType = 999u;
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool));
        Assert.Throws<ArgumentNullException>(() => RawNormalTerrainChecks.RequireStorage(null!, placement, stamp, types, pool));
    }
}
