using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 세션 지형 공용 후처리와 Pop에 월드 그래프 엔진을 연결했는지 검사한다.
// cpppj/src/client/RawCampaignWorldRuntime.cpp 65·83행의 core.graph 공유 엔진 결선과
// o/SquidPostPopPatch.cpp 의 per-Pop 그래프 관문에 대응한다.
// R4-2 적재 중 postPop의 다섯 번째 단계(그래프 per-Pop 번호)다. 0.605에서 추가됐다.
namespace Netstorm.Client.Tests;

/// <summary>지형 Pop 호출에서 그래프 per-Pop 번호 부여가 실행되는지 검사한다. 0.605에서 추가됐다.</summary>
public sealed class HostSessionTerrainGraphTests
{
    /// <summary>실제 자료로 Pop 장면을 만든다. 157번 받침의 프레임 코드만 호출자가 정한다.</summary>
    /// <param name="codes">157번 받침의 프레임 코드다. Prefix 분기 선택을 고정한다.</param>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) LoadRealPopScene(IReadOnlyList<RiftFrameCode> codes)
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        var shpDb = new ShapeDatabase(shpRaw);
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, shpDb);
        session.BuildTerrainPopDevices(table.Types, shapes);
        // 실제 프레임 표를 복사해 157번 받침 코드만 고정한다. 이웃 탐색·그래프 엔진이 읽는 다른 타입 표는 실파일 값 그대로 둔다.
        RiftTypeFrames[] frames = session.WorldFrames.ToArray();
        frames[157] = new RiftTypeFrames(codes);
        session.BuildIslandPostPopDevices(table.Types, frames);
        return (session, table.Types);
    }

    /// <summary>검사 크기의 풀 슬롯을 세션 풀에 확보한다.</summary>
    private static void AllocateScene(RawSidPool pool)
    {
        // 5번부터 200번까지 클라이언트 영역에 확보한다.
        for (int id = 5; id <= 200; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }
    }

    /// <summary>그래프 장치 뒤에 결선해야 하며 순서를 어기면 거부한다.</summary>
    [Fact]
    public void WireRequiresIslandAndGraphDevices()
    {
        var bare = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => bare.WireTerrainPostPopGraph());
        var codes = new RiftFrameCode[] { new((byte)'G', (byte)'P', 1, 0) };
        var (session, _) = LoadRealPopScene(codes);
        AllocateScene(session.Pool);
        // 그래프 장치 전에는 거부한다.
        Assert.Throws<InvalidOperationException>(() => session.WireTerrainPostPopGraph());
    }

    /// <summary>결선 뒤 표면 Pop이 그래프 번호 바이트를 기록한다.</summary>
    [Fact]
    public void WiredPopMarksSurfaceGraphNumber()
    {
        var codes = new RiftFrameCode[] { new((byte)'G', (byte)'P', 1, 0) };
        var (session, types) = LoadRealPopScene(codes);
        AllocateScene(session.Pool);
        session.BuildWorldGraphDevices(types);
        session.WireTerrainPostPopGraph();
        TerrainStampHooks hooks = session.BuildTerrainStampHooks(types);
        int surface = hooks.Create!(157u, 1u);
        Assert.Equal(0, session.Pool.Slot(surface)[30]);
        hooks.Pop!(surface, 10f, 10f, 0x02000061u);
        // NoGraph 억제 경로의 ResetGraph가 무효 번호를 기록한다.
        Assert.Equal(254, session.Pool.Slot(surface)[30]);
        // 같은 호출이 분배 Pop으로 기록된다.
        Assert.Contains(session.TerrainPopCalls, entry => entry.Contains("island:ready") && entry.Contains("Registered"));
    }

    /// <summary>결선 없이 표면 Pop을 실행하면 그래프 번호 바이트가 그대로 남는다.</summary>
    [Fact]
    public void UnwiredPopLeavesGraphNumber()
    {
        var codes = new RiftFrameCode[] { new((byte)'G', (byte)'P', 1, 0) };
        var (session, types) = LoadRealPopScene(codes);
        AllocateScene(session.Pool);
        TerrainStampHooks hooks = session.BuildTerrainStampHooks(types);
        int surface = hooks.Create!(157u, 1u);
        hooks.Pop!(surface, 10f, 10f, 0x02000061u);
        // 그래프 관문이 닫혀 있으면 ResetGraph가 실행되지 않는다.
        Assert.Equal(0, session.Pool.Slot(surface)[30]);
    }

    /// <summary>실제 지형 생성의 호출열·풀 바이트가 결선 뒤에도 결정적이다.</summary>
    [Fact]
    public void LiveTerrainStaysDeterministicWithGraph()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        HostSession first = BuildLiveTerrainScene(startup, fort);
        HostSession second = BuildLiveTerrainScene(startup, fort);
        // 실제 지형 생성이 Pop을 호출한다.
        Assert.NotEmpty(first.TerrainPopCalls);
        Assert.Equal(first.TerrainPopCalls.Count, second.TerrainPopCalls.Count);
        // 호출열 전체가 순서대로 같다.
        for (int index = 0; index < first.TerrainPopCalls.Count; ++index)
        {
            Assert.Equal(first.TerrainPopCalls[index], second.TerrainPopCalls[index]);
        }

        // 기록과 분배·그래프는 풀 바이트에 결정적으로만 영향을 준다.
        Assert.Equal(first.Pool.Bytes.ToArray(), second.Pool.Bytes.ToArray());
    }

    /// <summary>실제 지형 생성 장면을 만든다. 그래프 결선 뒤 지형을 생성한다.</summary>
    /// <param name="startup">실제 자료 시작 화면이다.</param>
    /// <param name="fort">적재할 미션 요새다.</param>
    /// <returns>지형 생성이 끝난 세션이다.</returns>
    private static HostSession BuildLiveTerrainScene(HostStartupScene startup, MissionFortLoadResult fort)
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(shpRaw));
        session.BuildTerrainPopDevices(table.Types, shapes);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        session.BuildIslandPostPopDevices(table.Types, session.WorldFrames);
        session.BuildWorldGraphDevices(table.Types);
        session.WireTerrainPostPopGraph();
        HostFortObjectLiveTests.BuildLiveTerrainFirst(session, fort.Fort, table.Types);
        return session;
    }
}
