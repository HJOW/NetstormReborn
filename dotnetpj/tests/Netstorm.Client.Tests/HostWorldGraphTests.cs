using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 호스트 세션의 월드 그래프 재구성 장치가 실제 장면에서 표를 채우는지 검사한다.
public sealed class HostWorldGraphTests
{
    /// <summary>실제 자료로 그래프 장면을 만든다. 타입 표·정의·SHP·프레임은 실파일 값이다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) LoadRealGraphScene(uint seed = 1u)
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(seed), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        var shpDb = new ShapeDatabase(shpRaw);
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, shpDb);
        session.BuildTerrainPopDevices(table.Types, shapes);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        BuildLiveTerrainFirst(session, fort.Fort, table.Types);
        session.LoadFortObjects(fort.Fort, 1u);
        session.BuildWorldGraphDevices(table.Types);
        return (session, table.Types);
    }

    /// <summary>실제 요새의 객체 적재 장치를 만든다. live 시작 흐름과 같은 순서(지형 PlaceAll·Generate 뒤 Objects)용이다.</summary>
    private static void BuildLiveTerrainFirst(HostSession session, FortTemplate fort, IReadOnlyList<OriginalTypeRecord> types)
    {
        // live와 같은 120바이트 Territory 읽기와 같다. 스트림 위치·진단은 FortWorldLoad 쪽 검사에서 다룬다.
        Assert.True(fort.Territory.Length >= 120);
        byte[] records = new byte[120];
        Array.Copy(fort.Territory, 0, records, 0, 120);
        var islands = session.BuildIslands(new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        var islandState = new RawIslandBuilderState();
        var builder = new RawIslandBuilder(
            session.World, session.World, islands, session.WorldFrames,
            () => session.GameAux.PuzzlePieceType, islandState, _ => { });
        builder.PlaceAll(records, 0, 20);
        // live와 같은 전체 지도 생성이다. 전투 옵션이 고른 레이아웃·간격을 그대로 쓴다.
        var diagnostics = new NormalTerrainDiagnostics
        {
            Context = _ => { },
            Stamp = _ => { },
            Region = _ => { },
            Farm = _ => { },
            Postpass = _ => { },
        };
        var binding = session.BuildTerrainBinding(types, diagnostics, new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        binding.GenerateOriginal(
            islandState.Owner, islandState.Bounds, session.TerrainGlobals.NoRegion,
            session.Game, session.TerrainCrt, session.BuildTerrainStampHooks(types), (_, _, _) => { });
    }

    /// <summary>장치 인자 오류를 연결 전에 거부하는지 확인한다.</summary>
    [Fact]
    public void GraphDevicesRejectBadArgs()
    {
        var (session, types) = LoadRealGraphScene();
        Assert.Throws<ArgumentNullException>(() => session.BuildWorldGraphDevices(null!));
        Assert.Throws<ArgumentException>(() => session.BuildWorldGraphDevices(types.Take(187).ToArray()));
        // 지형 Pop 장치보다 먼저 연결하면 공유 공간이 없어 거부한다.
        var bare = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => bare.BuildWorldGraphDevices(types));
    }

    /// <summary>장치 미연결 재구성을 거부하는지 확인한다.</summary>
    [Fact]
    public void RebuildWorldGraphRequiresDevices()
    {
        var (session, _) = LoadRealGraphScene();
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var bare = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        bare.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        // 그래프 장치를 연결하지 않은 세션은 재구성을 거부한다.
        Assert.Throws<InvalidOperationException>(() => bare.RebuildWorldGraph(false));
        Assert.Throws<InvalidOperationException>(() => bare.RebuildWorldGraph(true));
    }

    /// <summary>실제 장면의 재구성이 표를 채우고 결정적인지 확인한다.</summary>
    [Fact]
    public void RebuildFillsTableDeterministically()
    {
        var (first, _) = LoadRealGraphScene();
        var (second, _) = LoadRealGraphScene();
        first.RebuildWorldGraph(false);
        second.RebuildWorldGraph(false);
        var firstRows = first.DumpGraph();
        var secondRows = second.DumpGraph();
        Assert.Equal(255, firstRows.Count);
        Assert.Equal(255, secondRows.Count);
        // 같은 장면·같은 시드는 같은 표를 만든다.
        for (int index = 0; index < firstRows.Count; ++index)
        {
            Assert.Equal(secondRows[index], firstRows[index]);
        }
        // 재구성 전 0이던 표가 채워진다.
        Assert.Contains(firstRows, row => row.Surfaces != 0 || row.InUse != 0);
        // 표면 슬롯의 그래프 바이트(오프셋 30)가 쓰인다.
        bool written = false;
        // 사용 중인 슬롯을 순서대로 훑는 반복문이다.
        for (int sid = RawSidPool.FirstOrdinary; sid < first.Pool.Capacity; ++sid)
        {
            if ((first.Pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0) continue;
            if (first.Pool.Slot(sid)[30] != 0) { written = true; break; }
        }
        Assert.True(written);
    }
}
