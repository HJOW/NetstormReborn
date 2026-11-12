using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 호스트 세션의 지형 Pop 장치가 공장 생성·공간 등록을 실제 결선하는지 검사한다.
public sealed class HostTerrainPopTests
{
    /// <summary>지형 Pop 호출 플래그다. 원본 도장과 같은 0x02000061이다.</summary>
    private const uint PopFlags = 0x02000061u;

    /// <summary>실제 자료로 Pop 장면을 만든다. 타입 표·정의·SHP는 실파일 값이다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) LoadRealPopScene()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
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

    /// <summary>장치를 연결하기 전의 도장 효과가 중립(기록만)인지 확인한다.</summary>
    [Fact]
    public void StampHooksWithoutDevicesIsNeutral()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        Assert.Null(session.TerrainPopHash);
        Assert.Null(session.TerrainPopSpots);
        TerrainStampHooks hooks = session.BuildTerrainStampHooks(table.Types);
        int sid = hooks.Create!(97u, 1u);
        ReadOnlySpan<byte> created = session.Pool.Slot(sid);
        // 단순 할당 경로라 타입 바이트만 두고 void 상태로 둔다.
        Assert.Equal(97, created[RawSidPool.TypeOffset]);
        Assert.NotEqual(0, created[RawSidPool.StateOffset] & RawSidPool.VoidBit);
        hooks.Pop!(sid, 50f, 60f, PopFlags);
        // 중립 Pop은 슬롯을 바꾸지 않아 void가 그대로 남는다.
        ReadOnlySpan<byte> after = session.Pool.Slot(sid);
        Assert.NotEqual(0, after[RawSidPool.StateOffset] & RawSidPool.VoidBit);
    }

    /// <summary>장치 개수 불일치를 연결 전에 거부하는지 확인한다.</summary>
    [Fact]
    public void PopDevicesRejectMismatchedCounts()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        var shpDb = new ShapeDatabase(shpRaw);
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, shpDb);
        Assert.Throws<ArgumentNullException>(() => session.BuildTerrainPopDevices(null!, shapes));
        Assert.Throws<ArgumentNullException>(() => session.BuildTerrainPopDevices(table.Types, null!));
        Assert.Throws<ArgumentException>(() => session.BuildTerrainPopDevices(table.Types, []));
    }

    /// <summary>공장 생성이 가상 표까지 갖춘 슬롯을 만드는지 확인한다.</summary>
    [Fact]
    public void FactoryCreatesVtableSlot()
    {
        var (session, types) = LoadRealPopScene();
        Assert.NotNull(session.TerrainPopHash);
        Assert.NotNull(session.TerrainPopSpots);
        TerrainStampHooks hooks = session.BuildTerrainStampHooks(types);
        int sid = hooks.Create!(97u, 1u);
        ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
        // 타입 바이트와 void 상태를 둔다.
        Assert.Equal(97, raw[RawSidPool.TypeOffset]);
        Assert.NotEqual(0, raw[RawSidPool.StateOffset] & RawSidPool.VoidBit);
        uint vtable = unchecked((uint)(raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)));
        // 공장 생성 표와 Pop 지원 표를 모두 통과한다.
        Assert.True(RawSquidFactory.SupportsConstructedVtable(session.Pool.Edition, vtable));
        Assert.True(OriginalPopDisplay.Supports(session.Pool.Edition, vtable, PopFlags));
    }

    /// <summary>섬 Pop이 좌표·단계·체인·spot·void를 등록하는지 확인한다.</summary>
    [Fact]
    public void IslandPopWritesCoordsLevelChainSpotHash()
    {
        var (session, types) = LoadRealPopScene();
        Assert.NotNull(session.TerrainPopHash);
        Assert.NotNull(session.TerrainPopSpots);
        TerrainStampHooks hooks = session.BuildTerrainStampHooks(types);
        const float x = 50f;
        const float y = 60f;
        int sid = hooks.Create!(97u, 1u);
        hooks.Pop!(sid, x, y, PopFlags);
        ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
        // 좌표·화면 좌표를 쓴다.
        Assert.Equal(x, BitConverter.ToSingle(raw.Slice(14, 4)));
        Assert.Equal(y, BitConverter.ToSingle(raw.Slice(18, 4)));
        Assert.Equal((int)(x * 16.0 + 0.5), raw[22] | (raw[23] << 8));
        Assert.Equal((int)(y * 11.0 + 0.5), raw[24] | (raw[25] << 8));
        // 섬은 해시 0단계이며 섬 번호 127을 둔다.
        Assert.Equal(0, raw[33]);
        Assert.Equal(127, raw[8] | (raw[9] << 8));
        // void를 풀고 체인을 잇는다.
        Assert.Equal(0, raw[RawSidPool.StateOffset] & RawSidPool.VoidBit);
        Assert.Equal((ushort)sid, session.TerrainPopHash.Bucket(0, x, y));
        Assert.Equal(0, raw[4] | (raw[5] << 8));
        // 발자국 셀에 점유를 둔다.
        Assert.NotEqual(0, session.TerrainPopSpots[(int)y * RawSquidHash.WorldCells + (int)x]);
    }

    /// <summary>프린지 Pop이 실측 SHP 크기로 3단계를 등록하는지 확인한다.</summary>
    [Fact]
    public void FringePopUsesRealShpDimsLevel3()
    {
        var (session, types) = LoadRealPopScene();
        Assert.NotNull(session.TerrainPopHash);
        Assert.NotNull(session.TerrainPopSpots);
        TerrainStampHooks hooks = session.BuildTerrainStampHooks(types);
        const float x = 100f;
        const float y = 100f;
        int sid = hooks.Create!(102u, 1u);
        hooks.Pop!(sid, x, y, PopFlags);
        ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
        // 실측 SHP 크기가 4를 넘어 3단계를 둔다.
        Assert.Equal(3, raw[33]);
        Assert.Equal(0, raw[RawSidPool.StateOffset] & RawSidPool.VoidBit);
        Assert.Equal((ushort)sid, session.TerrainPopHash.Bucket(3, x, y));
        Assert.Equal(x, BitConverter.ToSingle(raw.Slice(14, 4)));
        Assert.Equal(y, BitConverter.ToSingle(raw.Slice(18, 4)));
    }
}
