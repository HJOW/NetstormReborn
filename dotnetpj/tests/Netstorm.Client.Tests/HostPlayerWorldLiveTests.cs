// Player 월드(화신) 갈고리의 live 연결을 검사하는 네임스페이스다.
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// Player 월드(화신) 갈고리의 live 연결을 검사하는 클래스다. 2026-11-12 0.475에서 R4 Player 월드 갈고리 live 연결용으로 추가했다.
public sealed class HostPlayerWorldLiveTests
{
    /// <summary>실제 자료로 화신 장면을 만든다. 타입 표·정의·SHP·프레임은 실파일 값이다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) LoadPlayerScene()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        var shpDb = new ShapeDatabase(shpRaw);
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, shpDb);
        session.BuildTerrainPopDevices(table.Types, shapes);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        return (session, table.Types);
    }

    /// <summary>장치 연결 전에 화신 갈고리 조립을 거부하는지 확인한다.</summary>
    [Fact]
    public void PlayerWorldHooksRequireDevices()
    {
        var bare = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => bare.BuildPlayerWorldHooks());
    }

    /// <summary>빈 풀의 찾기가 원문처럼 0을 돌려주는지 확인한다.</summary>
    [Fact]
    public void FindOnEmptyPoolReturnsZero()
    {
        var (session, _) = LoadPlayerScene();
        PlayerWorldHooks hooks = session.BuildPlayerWorldHooks();
        Assert.NotNull(hooks.Find);
        Assert.Equal(0, hooks.Find(11f, 1f, 152u));
    }

    /// <summary>공장 생성이 화신 슬롯을 만든다는 것을 확인한다. 원문 가상표·타입·void 설정을 그대로 둔다.</summary>
    [Fact]
    public void CreateConstructsAvatarSlot()
    {
        var (session, _) = LoadPlayerScene();
        PlayerWorldHooks hooks = session.BuildPlayerWorldHooks();
        Assert.NotNull(hooks.Create);
        int sid = hooks.Create(152u, 2u);
        Assert.True(sid > 0);
        ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
        Assert.Equal(152, raw[RawSidPool.TypeOffset]);
        uint vtable = (uint)(raw[0] | ((uint)raw[1] << 8) | ((uint)raw[2] << 16) | ((uint)raw[3] << 24));
        Assert.Equal(0x0050ee08u, vtable);
    }

    /// <summary>공간 등록 전에는 찾기가 0을 유지하는지 확인한다. 해시 일관성의 증거다.</summary>
    [Fact]
    public void FindMissesBeforeSpaceRegistration()
    {
        var (session, _) = LoadPlayerScene();
        PlayerWorldHooks hooks = session.BuildPlayerWorldHooks();
        Assert.NotNull(hooks.Create);
        Assert.NotNull(hooks.Find);
        int sid = hooks.Create(152u, 2u);
        Assert.True(sid > 0);
        Assert.Equal(0, hooks.Find(11f, 1f, 152u));
    }

    /// <summary>소유자 지정이 슬롯 소유자 바이트를 쓰고 타입을 보존하는지 확인한다.</summary>
    [Fact]
    public void SetOwnerWritesOwnerByte()
    {
        var (session, _) = LoadPlayerScene();
        PlayerWorldHooks hooks = session.BuildPlayerWorldHooks();
        Assert.NotNull(hooks.Create);
        Assert.NotNull(hooks.SetOwner);
        int sid = hooks.Create(152u, 2u);
        hooks.SetOwner(sid, 1u);
        ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
        Assert.Equal(152, raw[RawSidPool.TypeOffset]);
        Assert.Equal(1, raw[34]);
    }

    /// <summary>실제 Pop이 공간·Player 기술부모·공유 장부와 커널 예약을 함께 변경하는지 확인한다.</summary>
    [Fact]
    public void PopRegistersAvatarAndInitialRegular()
    {
        var (session, _) = LoadPlayerScene();
        session.MissionPlayer.PlayerKnown = 1u;
        session.MissionPlayer.BattleOwner = 1u;
        session.ProcessTime.Value = 17.25;
        PlayerWorldHooks hooks = session.BuildPlayerWorldHooks();
        Assert.Same(hooks, session.BuildPlayerWorldHooks());
        int sid = session.PlayerWorld.Create(1);
        Assert.Equal(sid, hooks.Find!(11f, 1f, 152u));
        Assert.Equal((uint)sid, session.PlayerWorld.Technology(1));
        Assert.Equal((uint)sid, session.PlayerWorld.CurrentTechnology());
        Assert.Same(session.PlayerRecords, session.PlayerWorld.Records());
        Assert.Same(session.MissionPlayer, session.PlayerWorld.State());
        Assert.Equal(0, session.Pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.VoidBit);
        RegularProcess regular = Assert.IsType<RegularProcess>(session.PlayerProcesses.FindEvent(sid, 1u));
        Assert.Equal(sid, regular.Parent());
        Assert.Equal(5f, regular.Payload());
        Assert.Equal(17.25, regular.Time());
        Assert.Equal(0u, regular.Count());
        Assert.Equal(1, session.Kernel.Size());
        Assert.Equal(0u, session.PostPopDepth);
        Assert.Throws<InvalidOperationException>(() => session.PlayerWorld.Create(1));
        string output = Path.GetTempFileName();
        try
        {
            var dump = new HostSessionDump(session.Kernel, session.Game, session.Crt, () => 90ul, () => 0ul);
            dump.BindSession(session);
            dump.Write(output);
            string[] lines = File.ReadAllLines(output);
            Assert.Contains($"process\t1\t{regular.Form()}\t{sid}", lines);
            Assert.Contains($"regular\t1\t1\t0\t{BitConverter.SingleToUInt32Bits(5f)}\t{BitConverter.DoubleToUInt64Bits(17.25)}", lines);
        }
        finally { File.Delete(output); }
        session.ReleasePlayerProcesses();
        Assert.Equal(0, session.Kernel.Size());
    }

    /// <summary>로컬이 아닌 화신은 기술부모를 등록하되 로컬 전용 정기 프로세스를 만들지 않는지 확인한다.</summary>
    [Fact]
    public void OtherOwnerRegistersTechnologyWithoutLocalRegular()
    {
        var (session, _) = LoadPlayerScene();
        session.MissionPlayer.PlayerKnown = 1u;
        session.MissionPlayer.BattleOwner = 1u;
        session.BuildPlayerWorldHooks();
        int sid = session.PlayerWorld.Create(2);
        Assert.Equal((uint)sid, session.PlayerWorld.Technology(2));
        Assert.Equal(0u, session.PlayerWorld.CurrentTechnology());
        Assert.Equal(2, session.Pool.Slot(sid)[34]);
        Assert.Equal(0, session.Kernel.Size());
        Assert.Equal(0u, session.PostPopDepth);
    }

    /// <summary>장부의 세 통계표와 scalar를 덤프·소유자·PostPop이 복사 없이 공유하는지 확인한다.</summary>
    [Fact]
    public void BookkeepingAndDumpUseSameStorage()
    {
        var (session, _) = LoadPlayerScene();
        Assert.Same(session.TotalMade.LocalCounts, session.TerrainBookkeeping.LocalCounts);
        Assert.Same(session.TotalMade.LocalSecondaryCounts, session.TerrainBookkeeping.LocalSecondaryCounts);
        Assert.Same(session.TotalMade.GlobalCounts, session.TerrainBookkeeping.GlobalCounts);
        session.TerrainBookkeeping.TotalCost = -23;
        session.TerrainBookkeeping.Depth = 7;
        session.TerrainBookkeeping.ProductionDirty = 11;
        Assert.Equal(-23, session.DumpTotals().TotalCost);
        Assert.Equal(7u, session.PostPopDepth);
        Assert.Equal(11u, session.ProductionDirty);
        session.TotalCost = 17;
        Assert.Equal(17, session.TerrainBookkeeping.TotalCost);
    }
}
