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

    /// <summary>화신 Pop 호출이 기록만 하고 멈추지 않는지 확인한다. 공간 등록은 R6 경계다.</summary>
    [Fact]
    public void PopIsRecordOnlyBoundary()
    {
        var (session, _) = LoadPlayerScene();
        PlayerWorldHooks hooks = session.BuildPlayerWorldHooks();
        Assert.NotNull(hooks.Create);
        Assert.NotNull(hooks.Pop);
        int sid = hooks.Create(152u, 2u);
        byte state = session.Pool.Slot(sid)[RawSidPool.StateOffset];
        hooks.Pop(sid, 11f, 1f, 0u);
        Assert.Equal(state, session.Pool.Slot(sid)[RawSidPool.StateOffset]);
    }

    /// <summary>실제 적재 호출 순서(찾기→생성→소유자→직접 프레임 0→Pop)로 화신을 만드는지 확인한다.</summary>
    [Fact]
    public void FullAvatarFlowThroughRawPlayerWorld()
    {
        var (session, types) = LoadPlayerScene();
        PlayerWorldHooks hooks = session.BuildPlayerWorldHooks();
        var state = new PlayerWorldState { PlayerKnown = 1u, BattleOwner = 1u, PlayerType = 152u };
        var inner = new RawPostPop(session.Pool, types, new RawPostPopState { Suppressed = true });
        var player = new RawPlayerWorld(session.Pool, inner, session.PlayerRecords, state, hooks);
        int sid = player.Create(1);
        Assert.True(sid > 0);
        ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
        Assert.Equal(152, raw[RawSidPool.TypeOffset]);
        Assert.Equal(1, raw[34]);
        Assert.Equal(0u, raw[36] | ((uint)raw[37] << 8) | ((uint)raw[38] << 16) | ((uint)raw[39] << 24));
    }
}
