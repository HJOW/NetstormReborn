using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 거주 구역 Pop·정기 세션 결선을 검사하는 클래스다. 2026-11-19 0.635에서 R4-4 거주 구역 Pop용으로 추가했다.
// 근거: cpppj client/RawSurfaceStorage.cpp의 자산 분배와 o/RawResidenceRegular.cpp의 정기 본체다.
public sealed class HostSessionResidenceTests
{
    /// <summary>실제 자료로 거주 구역 장면을 만든다. 지형·요새·바인딩·지역·자산을 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildResidenceScene()
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

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 2026-11-19 0.635에서 추가했다.</summary>
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

    /// <summary>거주 구역 부모 슬롯을 할당하고 가상표·타입·좌표를 채운다. 2026-11-19 0.635에서 추가했다.</summary>
    /// <param name="session">거주 구역 장치가 연결된 세션이다.</param>
    /// <returns>채운 거주 구역 부모 번호다.</returns>
    private static int PrepareResidenceSid(HostSession session)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x0050f540u);
        raw[RawSidPool.TypeOffset] = 89;
        raw[RawSidPool.StateOffset] = 0;
        raw[40] = 0;
        Put(raw, 12, 0x07u, 2);
        Put(raw, 36, 10u);
        Put(raw, 42, 20u);
        Put(raw, 14, BitConverter.SingleToUInt32Bits(50.75f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(52.5f));
        return sid;
    }

    /// <summary>거주 구역 Pop은 목록·예약 3건·BYTE 통지·잔여 프레임을 연결한다.</summary>
    [Fact]
    public void ResidencePostPopAttachesThreeRegulars()
    {
        (HostSession session, _) = BuildResidenceScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareResidenceSid(session);
        session.RegionVortexPostPop.PostPop(sid, 1u);
        Assert.NotNull(session.PlayerProcesses.FindEvent(sid, 0));
        Assert.NotNull(session.PlayerProcesses.FindEvent(sid, 1));
        Assert.NotNull(session.PlayerProcesses.FindEvent(sid, 3));
        Assert.Equal(0, session.Pool.Slot(sid)[12] & 15);
        Assert.Equal(10, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid)[42..]));
    }

    /// <summary>거주 구역 사건 0을 세션 처리기로 보내면 주민 nibble이 뒤집히고 15~35초 지연이 된다.</summary>
    [Fact]
    public void ResidenceRegularRoutesEventZeroThroughSession()
    {
        (HostSession session, _) = BuildResidenceScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareResidenceSid(session);
        uint before = session.Game.State();
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        float result = handler(sid, 0, 0, 0.1f);
        Assert.InRange(result, 15f, 35f);
        uint start = before == 0 ? 0x0bad0bad : before;
        uint expected = unchecked(start * 0x10003u + 3u);
        Assert.Equal(expected, session.Game.State());
        Assert.Equal(0, session.Pool.Slot(sid)[12] & 15);
    }

    /// <summary>거주 구역 사건 1을 세션 처리기로 보내면 1초 뒤로 재예약된다.</summary>
    [Fact]
    public void ResidenceRegularRoutesEventOneThroughSession()
    {
        (HostSession session, _) = BuildResidenceScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareResidenceSid(session);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(1.0f, handler(sid, 1, 0, 0.2f));
    }

    /// <summary>거주 구역 사건 3을 세션 처리기로 보내면 0.1초 뒤로 재예약된다.</summary>
    [Fact]
    public void ResidenceRegularRoutesEventThreeThroughSession()
    {
        (HostSession session, _) = BuildResidenceScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareResidenceSid(session);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.1f, handler(sid, 3, 0, 0.5f));
    }

    /// <summary>거주 구역 가상표가 아니면 세션 거주 구역 분배가 이전 처리기로 넘긴다.</summary>
    [Fact]
    public void ResidenceRegularFallsBackForOtherSids()
    {
        (HostSession session, _) = BuildResidenceScene();
        int sid = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(sid).Fill(0);
        byte[] before = session.Pool.Slot(sid).ToArray();
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(3.25f, handler(sid, 0, 0, 3.25f));
        Assert.Equal(before, session.Pool.Slot(sid).ToArray());
    }

    /// <summary>장치가 없으면 거주 구역 Pop·분배 경로를 명시 예외로 멈춘다.</summary>
    [Fact]
    public void RejectsUnbuiltResidence()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => session.RegionVortexPostPop);
    }
}
