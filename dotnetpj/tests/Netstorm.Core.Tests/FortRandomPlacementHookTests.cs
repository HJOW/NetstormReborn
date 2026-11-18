// Fort 임의 배치 훅 메이커의 풀 일치와 실제 결선을 검증하는 검사다.
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// Fort 배치 어댑터 결선을 검증하는 검사 클래스다. 2026-11-18 0.612에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다.
public sealed class FortRandomPlacementHookTests
{
    /// <summary>파이프라인·공장·보상 조립에 쓰는 빈 타입 표를 만든다.</summary>
    /// <returns>10.78 규격 188개 타입 기록이다.</returns>
    private static List<OriginalTypeRecord> Types()
    {
        var types = new List<OriginalTypeRecord>(OriginalConstructors.TypeCount);
        // 모든 타입 번호마다 빈 기록을 넣는 반복문이다.
        for (int i = 0; i < OriginalConstructors.TypeCount; ++i)
        {
            types.Add(new OriginalTypeRecord());
        }
        return types;
    }

    /// <summary>같은 풀에서 파이프라인·공장·보상을 조립한다.</summary>
    /// <param name="pool">공유할 풀이다.</param>
    /// <param name="types">공유할 타입 표다.</param>
    /// <returns>파이프라인·공장·보상 묶음이다.</returns>
    private static (RawCanonPlacementPipeline Pipeline, RawSquidFactory Factory, RawSquidReward Hp) Scene(RawSidPool pool, List<OriginalTypeRecord> types)
    {
        var hash = new RawSquidHash();
        var books = new RawPostPopState();
        var additional = new RawPostPopList();
        var contained = new ContainedFinderState();
        var state = new CanonPlacementPipelineState();
        var frames = new PriestPlainCanonType[types.Count];
        // 모든 타입 번호마다 빈 프레임 표를 넣는 반복문이다.
        for (int i = 0; i < frames.Length; ++i)
        {
            frames[i] = new PriestPlainCanonType();
        }
        var shapes = new SquidDisplayShape[types.Count];
        // 모든 타입 번호마다 빈 셰이프를 넣는 반복문이다.
        for (int i = 0; i < shapes.Length; ++i)
        {
            shapes[i] = new SquidDisplayShape();
        }
        var hotspots = new PriestTypeHotspot[types.Count];
        // 모든 타입 번호마다 빈 핫스폿을 넣는 반복문이다.
        for (int i = 0; i < hotspots.Length; ++i)
        {
            hotspots[i] = new PriestTypeHotspot(0, 0);
        }
        var data = new CanonPlacementPipelineData
        {
            Types = types,
            Frames = frames,
            Shapes = shapes,
            Hotspots = hotspots,
            Spots = new byte[65536],
            Islands = new ushort[65536],
            Regions = new CanonPlacementIslandRegion[128],
        };
        var pipeline = new RawCanonPlacementPipeline(pool, hash, data, books, additional, contained, state);
        var factory = new RawSquidFactory(pool, types);
        var store = new RawSpStore(new GameRandom(1), () => 1u);
        var hp = new RawSquidReward(pool, types, books, new SquidRewardState(), store);
        return (pipeline, factory, hp);
    }

    /// <summary>같은 풀에서는 다섯 갈고리가 실제 구현으로 바뀌고 같은 묶음을 돌려준다.</summary>
    [Fact]
    public void WiresRealHooks()
    {
        var pool = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        var types = Types();
        (RawCanonPlacementPipeline pipeline, RawSquidFactory factory, RawSquidReward hp) = Scene(pool, types);
        var hooks = new FortRandomPlacementHooks();
        FortRandomPlacementHooks result = RawFortRandomPlacement.MakeFortRandomPlacementHooks(pool, pipeline, factory, hp, hooks);
        Assert.Same(hooks, result);
        Assert.NotNull(hooks.MayPlace);
        Assert.NotNull(hooks.Create);
        Assert.NotNull(hooks.Construct);
        Assert.NotNull(hooks.HasHitPoints);
        Assert.NotNull(hooks.TypeHitPoints);
    }

    /// <summary>풀 하나라도 다르면 거부한다.</summary>
    [Fact]
    public void RejectsForeignPool()
    {
        var pool = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        var types = Types();
        (RawCanonPlacementPipeline pipeline, RawSquidFactory factory, RawSquidReward hp) = Scene(pool, types);
        var foreign = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        Assert.Throws<ArgumentException>(() => RawFortRandomPlacement.MakeFortRandomPlacementHooks(foreign, pipeline, factory, hp, new FortRandomPlacementHooks()));
    }
}
