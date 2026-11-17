using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 세션 지형 Pop 호출 연결의 기록과 실측 발화 준비를 검사한다.
// cpppj/src/client/RawCampaignWorldRuntime.cpp 46행의 world.Pop 결선에 대응하며,
// R4-2 적재 중 postPop의 두 번째 단계(호출 연결 측정)다. 0.602에서 추가됐다.
namespace Netstorm.Client.Tests;

/// <summary>지형 Pop 호출 기록이 실제 등록과 같은 순서로 남는지 검사한다. 0.602에서 추가됐다.</summary>
public sealed class HostSessionTerrainPopCallTests
{
    /// <summary>실제 자료로 Pop 장면을 만든다. 타입 표·정의·SHP는 실파일 값이다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) LoadRealPopScene()
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
        return (session, table.Types);
    }

    /// <summary>장치가 없으면 중립 호출을 기록하고 슬롯을 바꾸지 않는다.</summary>
    [Fact]
    public void NeutralPopRecordsNeutralCall()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        TerrainStampHooks hooks = session.BuildTerrainStampHooks(table.Types);
        int sid = hooks.Create!(97u, 1u);
        hooks.Pop!(sid, 50f, 60f, 0x02000061u);
        // 중립 호출 한 줄이 순서대로 남는다.
        Assert.Single(session.TerrainPopCalls);
        Assert.Contains("neutral", session.TerrainPopCalls[0]);
        Assert.Contains("island:missing", session.TerrainPopCalls[0]);
        // 중립 Pop은 슬롯을 바꾸지 않아 void가 그대로 남는다.
        ReadOnlySpan<byte> after = session.Pool.Slot(sid);
        Assert.NotEqual(0, after[RawSidPool.StateOffset] & RawSidPool.VoidBit);
    }

    /// <summary>실제 섬 Pop이 등록 결과와 섬 후처리 준비 상태를 함께 기록한다.</summary>
    [Fact]
    public void RealIslandPopRecordsRegisteredCall()
    {
        var (session, types) = LoadRealPopScene();
        TerrainStampHooks hooks = session.BuildTerrainStampHooks(types);
        int sid = hooks.Create!(97u, 1u);
        hooks.Pop!(sid, 50f, 60f, 0x02000061u);
        // 실제 등록 한 줄이 순서대로 남는다.
        Assert.Single(session.TerrainPopCalls);
        Assert.Contains("Registered", session.TerrainPopCalls[0]);
        Assert.Contains("island:missing", session.TerrainPopCalls[0]);
        // 실제 Pop은 기존 계약대로 좌표·void·체인을 등록한다.
        ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
        Assert.Equal(50f, BitConverter.ToSingle(raw.Slice(14, 4)));
        Assert.Equal(0, raw[RawSidPool.StateOffset] & RawSidPool.VoidBit);
        Assert.Equal((ushort)sid, session.TerrainPopHash!.Bucket(0, 50f, 60f));
    }

    /// <summary>섬 후처리 장치를 연결하면 호출 기록에 준비 상태가 찍힌다.</summary>
    [Fact]
    public void IslandDevicesMarkCallsReady()
    {
        var (session, types) = LoadRealPopScene();
        session.BuildIslandPostPopDevices(types, session.WorldFrames);
        TerrainStampHooks hooks = session.BuildTerrainStampHooks(types);
        int sid = hooks.Create!(97u, 1u);
        hooks.Pop!(sid, 50f, 60f, 0x02000061u);
        // 섬 후처리 연결 뒤의 호출은 준비 상태로 기록된다.
        Assert.Single(session.TerrainPopCalls);
        Assert.Contains("Registered", session.TerrainPopCalls[0]);
        Assert.Contains("island:ready", session.TerrainPopCalls[0]);
        // 기록은 풀 바이트에 영향을 주지 않는다.
        Assert.Equal(0, session.Pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.VoidBit);
    }

    /// <summary>실제 지형 생성의 Pop 호출열이 결정적이고 풀 바이트와 같다.</summary>
    [Fact]
    public void LiveTerrainFirstPopCallsAreDeterministic()
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

        // 기록은 풀 바이트에 영향을 주지 않는다.
        Assert.Equal(first.Pool.Bytes.ToArray(), second.Pool.Bytes.ToArray());
    }

    /// <summary>실제 지형 생성 장면을 만든다. live 시작 흐름과 같은 순서다.</summary>
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
        HostFortObjectLiveTests.BuildLiveTerrainFirst(session, fort.Fort, table.Types);
        return session;
    }
}
