// Fort 적재 바인딩의 경계·지형·Money·Graph 배선을 검증한다.
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// Fort 적재 바인딩을 검증하는 테스트 클래스다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.
public sealed class FortWorldLoadBindingTests
{
    /// <summary>188개 타입 목록을 만든다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> Types188() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();

    /// <summary>같은 풀에 묶은 적재 3종 장면을 만든다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="types">188개 타입이다.</param>
    private static (RawFortObjectLoad Objects, RawFortMetaLoad Meta, RawGraph Graph) LoadScene(RawSidPool pool, List<OriginalTypeRecord> types)
    {
        var hash = new RawSquidHash();
        var frames = Enumerable.Range(0, types.Count).Select(_ => new RiftTypeFrames([])).ToList();
        var objects = new RawFortObjectLoad(pool, hash, types, frames, new GameRandom(), new FortObjectLoadState(), new FortObjectLoadHooks
        {
            Create = (_, _) => 0,
            NotifySurface = _ => { },
            SetState = (_, _) => { },
            SetOwner = (_, _) => { },
            HasHitPoints = _ => false,
            MaxHitPoints = _ => 0,
            VirtualCount = _ => 0,
            Pop = (_, _, _, _) => { },
            Destroy = (_, _) => { },
            CreateContent = (_, _, _, _, _, _) => 0,
            ChunkRegion = (_, _) => 0,
            TerritoryAllowed = (_, _, _) => false,
            LoadingDepthChanged = _ => { },
            FinishOwner = _ => { },
        });
        uint depth = 0u;
        var meta = new RawFortMetaLoad(pool, new byte[28 * 220], () => depth, value => depth = value, new FortMetaLoadState(), new FortMetaLoadHooks
        {
            Assertion = _ => { },
            LoadingDepthChanged = _ => { },
            TechnologyParent = () => new Sid(0),
            PrepareTypeNames = () => { },
            Contents = (_, _, _, _) => { },
            ReleaseTypeConversion = () => { },
            RefreshTechnologyUi = () => { },
        });
        var graph = new RawGraph(pool, hash, new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells], types);
        return (objects, meta, graph);
    }

    /// <summary>같은 풀 CD Money 장면을 만든다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.</summary>
    private static (RawFortMoneyLoad Money, Func<uint> GetDepth, FortMoneyCursor Cursor) MoneyScene()
    {
        uint depth = 0u;
        var money = new RawFortMoneyLoad(OriginalEdition.Cd1072, new SquidRewardState(), new RawPostPopState(), new byte[9 * 0xacu],
            () => depth, value => depth = value, new FortMoneyState { TemplatePresent = true }, null, new FortMoneyHooks
            {
                Assertion = _ => { },
                LoadingDepthChanged = _ => { },
                RefreshSp = () => { },
            });
        var cursor = new FortMoneyCursor();
        cursor.Set(0u, 8u);
        return (money, () => depth, cursor);
    }

    /// <summary>서로 다른 풀·빠진 Money 선택을 거부한다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void GuardsRejectBadPoolsAndMissingSelection()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var other = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        Func<FortTemplate, uint, FortMoneyCursor> stream = (_, _) => new FortMoneyCursor();
        Assert.Throws<ArgumentException>(() => RawFortWorldLoadBinding.MakeFortWorldLoadBinding(
            pool, other, null!, null!, null!, null!, stream, null, null, null!, null!));
        Assert.Throws<ArgumentException>(() => RawFortWorldLoadBinding.MakeFortWorldLoadBinding(
            pool, pool, null!, null!, null!, null!, null, null, null, null!, null!));
    }

    /// <summary>빠진 장치·묶음을 거부한다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void GuardsRejectMissingDevices()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        (RawFortObjectLoad objects, RawFortMetaLoad meta, RawGraph graph) = LoadScene(pool, types);
        (RawFortMoneyLoad money, Func<uint> _, FortMoneyCursor _) = MoneyScene();
        Func<FortTemplate, uint, FortMoneyCursor> stream = (_, _) => new FortMoneyCursor();
        Action<FortTemplate, uint, bool> select = (_, _, _) => { };
        Func<FortTemplate, bool, FortMetaStream> metaStream = (_, _) => new FortMetaStream();
        Action<RawIslandBuilderState, int> generate = (_, _) => { };
        Assert.Throws<ArgumentNullException>(() => RawFortWorldLoadBinding.MakeFortWorldLoadBinding(
            pool, pool, graph, objects, money, meta, stream, select, metaStream, null!, new FortWorldLoadHooks()));
        Assert.Throws<ArgumentNullException>(() => RawFortWorldLoadBinding.MakeFortWorldLoadBinding(
            pool, pool, graph, objects, money, meta, stream, select, metaStream, generate, null!));
        Assert.Throws<ArgumentNullException>(() => RawFortWorldLoadBinding.MakeFortWorldLoadBinding(
            pool, pool, null!, objects, money, meta, stream, select, metaStream, generate, new FortWorldLoadHooks()));
    }

    /// <summary>자식·생성·Money·Graph를 묶음으로 연결한다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void ChildrenInstalled()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        (RawFortObjectLoad objects, RawFortMetaLoad meta, RawGraph graph) = LoadScene(pool, types);
        (RawFortMoneyLoad money, Func<uint> _, FortMoneyCursor _) = MoneyScene();
        var hooks = new FortWorldLoadHooks();
        FortWorldLoadHooks result = RawFortWorldLoadBinding.MakeFortWorldLoadBinding(
            pool, pool, graph, objects, money, meta,
            (_, _) => new FortMoneyCursor(), (_, _, _) => { }, (_, _) => new FortMetaStream(),
            (_, _) => { }, hooks);
        Assert.NotSame(hooks, result);
        Assert.NotNull(result.Objects);
        Assert.NotNull(result.Badges);
        Assert.NotNull(result.Technology);
        Assert.NotNull(result.Generate);
        Assert.NotNull(result.Money);
        Assert.NotNull(result.RebuildGraph);
    }

    /// <summary>Money 선택은 호출 때까지 미룬다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void MoneySelectsLazily()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        (RawFortObjectLoad objects, RawFortMetaLoad meta, RawGraph graph) = LoadScene(pool, types);
        (RawFortMoneyLoad money, Func<uint> getDepth, FortMoneyCursor cursor) = MoneyScene();
        var seen = new List<(FortTemplate Fort, uint Owner)>();
        var fort = new FortTemplate();
        FortWorldLoadHooks result = RawFortWorldLoadBinding.MakeFortWorldLoadBinding(
            pool, pool, graph, objects, money, meta,
            (selected, owner) => { seen.Add((selected, owner)); return cursor; },
            (_, _, _) => { }, (_, _) => new FortMetaStream(),
            (_, _) => { }, new FortWorldLoadHooks());
        Assert.Empty(seen);
        result.Money!(fort, 3u);
        Assert.Single(seen);
        Assert.Same(fort, seen[0].Fort);
        Assert.Equal(3u, seen[0].Owner);
        Assert.Equal(0u, getDepth());
    }

    /// <summary>지형 생성을 같은 인자로 넘긴다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void GeneratePassthrough()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        (RawFortObjectLoad objects, RawFortMetaLoad meta, RawGraph graph) = LoadScene(pool, types);
        (RawFortMoneyLoad money, Func<uint> _, FortMoneyCursor _) = MoneyScene();
        var seen = new List<(RawIslandBuilderState Builder, int Region)>();
        FortWorldLoadHooks result = RawFortWorldLoadBinding.MakeFortWorldLoadBinding(
            pool, pool, graph, objects, money, meta,
            (_, _) => new FortMoneyCursor(), (_, _, _) => { }, (_, _) => new FortMetaStream(),
            (builder, region) => seen.Add((builder, region)), new FortWorldLoadHooks());
        var builder = new RawIslandBuilderState();
        result.Generate!(builder, 5);
        Assert.Single(seen);
        Assert.Same(builder, seen[0].Builder);
        Assert.Equal(5, seen[0].Region);
    }

    /// <summary>Graph 재구성을 같은 값으로 넘긴다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void RebuildGraphDelegates()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        (RawFortObjectLoad objects, RawFortMetaLoad meta, RawGraph graph) = LoadScene(pool, types);
        (RawFortMoneyLoad money, Func<uint> _, FortMoneyCursor _) = MoneyScene();
        FortWorldLoadHooks result = RawFortWorldLoadBinding.MakeFortWorldLoadBinding(
            pool, pool, graph, objects, money, meta,
            (_, _) => new FortMoneyCursor(), (_, _, _) => { }, (_, _) => new FortMetaStream(),
            (_, _) => { }, new FortWorldLoadHooks());
        result.RebuildGraph!(true);
        result.RebuildGraph!(false);
    }
}
