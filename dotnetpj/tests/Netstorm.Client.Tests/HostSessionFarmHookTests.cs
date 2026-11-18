// 세션 농지 월드훅 결선을 검사하는 클래스다. 2026-11-18 0.613에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다.
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 농지 단일 배치의 월드 효과 여섯 갈고리 결선을 검사한다.
public sealed class HostSessionFarmHookTests
{
    /// <summary>실제 자료로 농지 월드훅 장면을 만든다. 운반체 결선 검사와 같은 공유 저장소를 쓴다.</summary>
    /// <returns>세션·타입 표·바인딩·파이프라인·보상 묶음이다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, RawNormalTerrainWorldBinding Binding, RawCanonPlacementPipeline Pipeline, RawSquidReward Hp) BuildHookScene()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        var shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(shpRaw));
        session.BuildTerrainPopDevices(table.Types, shapes);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        session.BuildIslandPostPopDevices(table.Types, session.WorldFrames);
        session.BuildWorldGraphDevices(table.Types);
        session.WireTerrainPostPopGraph();
        session.TerrainDiagnostics.Context = _ => { };
        session.TerrainDiagnostics.Stamp = _ => { };
        var binding = session.BuildTerrainBinding(table.Types, session.TerrainDiagnostics, new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        var hotspots = new PriestTypeHotspot[table.Types.Count];
        // 모든 타입 번호마다 TYPE 정의 기준점을 넣는 반복문이다.
        for (int i = 0; i < hotspots.Length; ++i)
        {
            hotspots[i] = new PriestTypeHotspot(table.Types[i].HotspotX, table.Types[i].HotspotY);
        }
        var regions = new CanonPlacementIslandRegion[128];
        var state = new CanonPlacementPipelineState();
        Array.Copy(new uint[] { 107, 82, 94, 157, 131, 129, 140, 142 }, state.Geometry.PatternTypes, 8);
        state.Relations.IslandCount = 128;
        var data2 = new CanonPlacementPipelineData
        {
            Types = table.Types,
            Frames = session.WorldFrames.Select(wf => new PriestPlainCanonType { Frames = wf }).ToArray(),
            Shapes = shapes,
            Hotspots = hotspots,
            Spots = session.TerrainPopSpots!,
            Islands = session.TerrainPopHash!.BorrowEntries(0),
            Regions = regions,
        };
        var pipeline = new RawCanonPlacementPipeline(session.Pool, session.TerrainPopHash, data2, session.TerrainBookkeeping, new RawPostPopList(), new ContainedFinderState(), state);
        return (session, table.Types, binding, pipeline, session.Reward);
    }

    /// <summary>같은 풀에서는 월드 효과 여섯 갈고리와 실결선 다섯 갈고리가 모두 연결된다.</summary>
    [Fact]
    public void WiresWorldHooks()
    {
        (HostSession session, _, RawNormalTerrainWorldBinding binding, RawCanonPlacementPipeline pipeline, RawSquidReward hp) = BuildHookScene();
        FortRandomPlacementHooks hooks = session.MakeFarmWorldHooks(binding.Islands, pipeline, hp);
        Assert.NotNull(hooks.MayPlace);
        Assert.NotNull(hooks.ChunkRegion);
        Assert.NotNull(hooks.RegionOwner);
        Assert.NotNull(hooks.Create);
        Assert.NotNull(hooks.Construct);
        Assert.NotNull(hooks.HasHitPoints);
        Assert.NotNull(hooks.TypeHitPoints);
        Assert.NotNull(hooks.SetOwner);
        Assert.NotNull(hooks.Unpop);
        Assert.NotNull(hooks.Repop);
        Assert.NotNull(hooks.Pop);
        Assert.Equal(unchecked((sbyte)session.World.At(0, 0).Island), hooks.ChunkRegion!(0, 0));
        Assert.Equal(binding.Islands.Owner(1), hooks.RegionOwner!(1));
        Assert.True(ReferenceEquals(session.Pool, binding.Islands.Pool()));
    }

    /// <summary>장치가 없으면 명시 예외로 멈춘다.</summary>
    [Fact]
    public void RequiresDevices()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        var types = new List<OriginalTypeRecord>();
        // 공장 규격 188개를 빈 기록으로 채우는 반복문이다.
        for (int i = 0; i < OriginalConstructors.TypeCount; ++i)
        {
            types.Add(new OriginalTypeRecord());
        }
        var islands = session.BuildIslands(new IslandRuntimeHooks { SetResidenceOwner = (_, _) => { }, Assertion = _ => { } });
        var hash = new RawSquidHash();
        var books = new RawPostPopState();
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
        var pipeline = new RawCanonPlacementPipeline(session.Pool, hash, data, books, new RawPostPopList(), new ContainedFinderState(), new CanonPlacementPipelineState());
        var hp = new RawSquidReward(session.Pool, types, books, new SquidRewardState(), new RawSpStore(new GameRandom(1), () => 1u));
        Assert.Throws<InvalidOperationException>(() => session.MakeFarmWorldHooks(islands, pipeline, hp));
    }

    /// <summary>풀 하나라도 다르면 거부한다.</summary>
    [Fact]
    public void RejectsForeignPool()
    {
        (HostSession session, _, _, _, RawSquidReward hp) = BuildHookScene();
        var foreign = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        var types = new List<OriginalTypeRecord>();
        // 외부 풀 파이프라인용으로 188개를 빈 기록으로 채우는 반복문이다.
        for (int i = 0; i < OriginalConstructors.TypeCount; ++i)
        {
            types.Add(new OriginalTypeRecord());
        }
        var hash = new RawSquidHash();
        var books = new RawPostPopState();
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
        var data = new CanonPlacementPipelineData
        {
            Types = types,
            Frames = frames,
            Shapes = shapes,
            Hotspots = new PriestTypeHotspot[types.Count],
            Spots = new byte[65536],
            Islands = new ushort[65536],
            Regions = new CanonPlacementIslandRegion[128],
        };
        var pipeline = new RawCanonPlacementPipeline(foreign, hash, data, books, new RawPostPopList(), new ContainedFinderState(), new CanonPlacementPipelineState());
        var islands = session.BuildIslands(new IslandRuntimeHooks { SetResidenceOwner = (_, _) => { }, Assertion = _ => { } });
        Assert.Throws<ArgumentException>(() => session.MakeFarmWorldHooks(islands, pipeline, hp));
    }
}
