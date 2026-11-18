using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 지역 가상 Owner 세션 결선을 검사하는 클래스다. 2026-11-19 0.621에서 R4-2 지역 소유자 확정용으로 추가했다.
// 근거: cpppj RawCampaignOwnership.cpp 54~60행의 소유권 조립과 78~80행의 SetOwner 분배다.
public sealed class HostSessionRegionVortexTests
{
    /// <summary>실제 자료로 지역 투표·가상 Owner 장면을 만든다. 지형·요새·바인딩을 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildVortexScene()
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
        // 섬이 비어 있어 거주 구역 전파를 부르지 않으므로 두 갈고리는 호출되지 않는다.
        session.BuildTerrainBinding(table.Types, session.TerrainDiagnostics, new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        session.BuildRegionOwnershipDevices(table.Types, () => 1u);
        return (session, table.Types);
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 2026-11-19 0.621에서 추가했다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>Player 기록의 DWORD를 읽는다. 2026-11-19 0.621에서 추가했다.</summary>
    /// <param name="bytes">Player 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    private static uint Get(byte[] bytes, int offset)
    {
        // 네 바이트를 낮은 바이트부터 합친다.
        return (uint)(bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24));
    }

    /// <summary>같은 장치에서는 가상 Owner 몸체·자산 후처리·revision·갱신·진단이 연결된다.</summary>
    [Fact]
    public void BuildsVortexDevices()
    {
        (HostSession session, _) = BuildVortexScene();
        Assert.True(ReferenceEquals(session.Pool, session.RegionVortex.Pool));
        Assert.NotNull(session.RegionVortexPostPop);
        Assert.Equal(0u, session.RegionVortexRevision);
        Assert.Equal(0u, session.RegionVortexRefreshes);
        Assert.Equal(0u, session.RegionVortexSoundRequests);
        Assert.Empty(session.RegionVortexDiagnostics);
        // 자산 후처리의 신전 판정은 같은 가상 표를 읽는다.
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Put(session.Pool.AllocatedBytes(sid), 0, 0x00519860u);
        Assert.True(session.RegionVortexPostPop.Handles(sid));
        int plain = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(plain).Fill(0);
        Assert.False(session.RegionVortexPostPop.Handles(plain));
    }

    /// <summary>요새 적재의 신전 소유자 지정은 가상 Owner로 분배되고 없는 섬은 보고 뒤 계속한다.</summary>
    [Fact]
    public void RoutesFortTempleOwnerThroughVortex()
    {
        (HostSession session, _) = BuildVortexScene();
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x00519860u);
        raw[10] = 70;
        raw[11] = 0;
        raw[34] = 1;
        raw[40] = 0;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(40.5f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(50.25f));
        session.SetFortObjectOwner(sid, 2u);
        // 이전 신전 해제는 없고 현재 신전 두 DWORD에 같은 번호가 남는다.
        Assert.Equal((uint)sid, Get(session.PlayerRecords, 2 * 180 + 0x7c));
        Assert.Equal((uint)sid, Get(session.PlayerRecords, 2 * 180 + 0x80));
        // 패치 생산 갱신 뒤 revision이 늘고 갱신을 한 번 센다.
        Assert.Equal(1u, session.RegionVortexRevision);
        Assert.Equal(1u, session.RegionVortexRefreshes);
        // 섬이 없어 진단을 남기고 계속 진행한다.
        Assert.Single(session.RegionVortexDiagnostics);
        Assert.StartsWith("vortex-missing", session.RegionVortexDiagnostics[0]);
    }

    /// <summary>가상 표가 아니면 기존 공통 부모로 소유자를 정하고 가상 장치를 건드리지 않는다.</summary>
    [Fact]
    public void FallsBackToCommonOwnerWithoutVortexTable()
    {
        (HostSession session, _) = BuildVortexScene();
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        raw[10] = 70;
        session.SetFortObjectOwner(sid, 3u);
        Assert.Equal(3, session.Pool.Slot(sid)[34]);
        Assert.Equal(0u, session.RegionVortexRevision);
        Assert.Equal(0u, session.RegionVortexRefreshes);
        Assert.Empty(session.RegionVortexDiagnostics);
    }

    /// <summary>Pop 시점 분배는 없는 섬을 만나면 보고하고 계속한다. 전파 경로는 실제 Generate 덤프로 대조한다.</summary>
    [Fact]
    public void DispatchReportsMissingIslandAndContinues()
    {
        (HostSession session, _) = BuildVortexScene();
        // 합성 장면은 지형을 생성하지 않아 모든 좌표가 빈 지역(127)이다. 섬 기록이 없어도 존재 진단 경로가 된다.
        Assert.Equal(127, session.RegionIslandAt(40.5f, 50.25f));
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x00519860u);
        raw[10] = 70;
        raw[11] = 0;
        raw[34] = 2;
        raw[40] = 0;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(40.5f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(50.25f));
        session.DispatchRegionTempleOwner(sid);
        // 섬이 없어 진단을 남기고 계속 진행하며 첫 지역은 그대로 둔다.
        Assert.Single(session.RegionVortexDiagnostics);
        Assert.Equal(0u, Get(session.PlayerRecords, 2 * 180 + 0x78));
        Assert.Equal((uint)sid, Get(session.PlayerRecords, 2 * 180 + 0x7c));
        Assert.Equal(1u, session.RegionVortexRevision);
        Assert.Equal(1u, session.RegionVortexRefreshes);
    }

    /// <summary>장치가 없거나 가상 표가 아니면 분배를 거부한다.</summary>
    [Fact]
    public void RejectsDispatchWithoutDevicesOrTable()
    {
        var bare = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => bare.DispatchRegionTempleOwner(50));
        (HostSession session, _) = BuildVortexScene();
        int sid = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(sid).Fill(0);
        Assert.Throws<InvalidOperationException>(() => session.DispatchRegionTempleOwner(sid));
    }

    /// <summary>장치가 없으면 명시 예외로 멈춘다.</summary>
    [Fact]
    public void RejectsUnbuiltVortex()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => session.RegionVortex);
        Assert.Throws<InvalidOperationException>(() => session.SetFortObjectOwner(50, 1u));
    }
}
