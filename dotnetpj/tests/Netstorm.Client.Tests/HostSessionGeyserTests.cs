using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 간헐천 Pop·정기 세션 결선을 검사하는 클래스다. 2026-11-19 0.634에서 R4-4 간헐천 Pop용으로 추가했다.
// 근거: cpppj client/RawSurfaceStorage.cpp 236행의 GeyserPostPop 분배와 297행의 정기 분배다.
public sealed class HostSessionGeyserTests
{
    /// <summary>실제 자료로 간헐천 장면을 만든다. 지형·요새·바인딩·지역·자산을 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildGeyserScene()
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
        session.BuildPlayerWorldHooks();
        session.BuildTerrainBinding(table.Types, session.TerrainDiagnostics, new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        session.BuildRegionOwnershipDevices(table.Types, () => 1u);
        return (session, table.Types);
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 2026-11-19 0.634에서 추가했다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 너비(BYTE 수)다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓰는 반복문이다.
        for (int index = 0; index < width; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>간헐천 부모 슬롯을 할당하고 가상표·타입·좌표를 채운다. 2026-11-19 0.634에서 추가했다.</summary>
    /// <param name="session">간헐천 장치가 연결된 세션이다.</param>
    /// <returns>채운 간헐천 부모 번호다.</returns>
    private static int PrepareGeyserSid(HostSession session)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x0050da88u);
        raw[RawSidPool.TypeOffset] = 122;
        raw[RawSidPool.StateOffset] = 0;
        raw[40] = 0;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(50.75f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(52.5f));
        Put(raw, 36, 7u);
        return sid;
    }

    /// <summary>간헐천 Pop은 목록·등록·비용 WORD·사건 0 예약을 연결한다.</summary>
    [Fact]
    public void GeyserPostPopAttachesEventZero()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildGeyserScene();
        session.TerrainBookkeeping.Suppressed = true;
        types[122].Cost = 100f;
        int sid = PrepareGeyserSid(session);
        Put(session.Pool.AllocatedBytes(sid), 12, 80u, 2);
        session.RegionVortexPostPop.GeyserPostPop(sid, 1u);
        Assert.Equal(100u, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(sid)[12..]));
        Assert.Single(session.GeyserRegistrations);
        RegularProcess? found = session.Kernel.Get(1) as RegularProcess;
        Assert.NotNull(found);
        Assert.Equal(sid, found.Parent());
        Assert.Equal(0u, found.Event());
    }

    /// <summary>간헐천 가상표의 사건 0을 세션 처리기로 보내면 비율 구간 프레임이 지정된다.</summary>
    [Fact]
    public void GeyserRegularRoutesEventZeroThroughSession()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildGeyserScene();
        session.TerrainBookkeeping.Suppressed = true;
        types[122].Cost = 100f;
        int sid = PrepareGeyserSid(session);
        Put(session.Pool.AllocatedBytes(sid), 12, 80u, 2);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        float result = handler(sid, 0, 0, 0.1f);
        Assert.Equal(BitConverter.UInt32BitsToSingle(0x3cf5c28fu), result);
        Assert.Equal(8, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid)[36..]));
    }

    /// <summary>간헐천 가상표가 아니면 세션 간헐천 분배가 이전 처리기로 넘긴다.</summary>
    [Fact]
    public void GeyserRegularFallsBackForOtherSids()
    {
        (HostSession session, _) = BuildGeyserScene();
        int sid = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(sid).Fill(0);
        byte[] before = session.Pool.Slot(sid).ToArray();
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(3.25f, handler(sid, 0, 0, 3.25f));
        Assert.Equal(before, session.Pool.Slot(sid).ToArray());
    }

    /// <summary>장치가 없으면 간헐천 Pop·분배 경로를 명시 예외로 멈춘다.</summary>
    [Fact]
    public void RejectsUnbuiltGeyser()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => session.RegionVortexPostPop);
        Assert.Empty(session.GeyserRegistrations);
    }
}
