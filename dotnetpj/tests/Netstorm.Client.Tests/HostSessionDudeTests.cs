using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// Dude 정기 세션 분배를 검사하는 클래스다. 2026-11-23 0.891에서 R4-4 Dude 정기용으로 추가했다.
// 근거: cpppj client/RawCampaignSession.cpp 1809행의 typed 경로 Dude 분기다.
public sealed class HostSessionDudeTests
{
    /// <summary>실제 자료로 Dude 장면을 만든다. 지형·요새·운반체·지역·Dude를 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildDudeScene()
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

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 0.891에서 추가했다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>Dude 부모 슬롯을 할당하고 가상표·타입을 채운다. 0.891에서 추가했다.</summary>
    /// <param name="session">Dude 장치가 연결된 세션이다.</param>
    /// <returns>채운 Dude 부모 번호다.</returns>
    private static int PrepareDudeSid(HostSession session)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x00508cc8u);
        raw[RawSidPool.TypeOffset] = 70;
        return sid;
    }

    /// <summary>Dude 비정기 사건을 세션 처리기로 보내면 그대로 돌려준다.</summary>
    [Fact]
    public void DudeRegularRoutesNonRitualEventThroughSession()
    {
        (HostSession session, _) = BuildDudeScene();
        int sid = PrepareDudeSid(session);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.7f, handler(sid, 9, 0, 0.7f));
    }

    /// <summary>Dude 폴링 사건은 세션에서 0을 돌려준다.</summary>
    [Fact]
    public void DudeRegularPollEventReturnsZero()
    {
        (HostSession session, _) = BuildDudeScene();
        int sid = PrepareDudeSid(session);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0, handler(sid, 0x1a2b, 0, 0.4f));
    }

    /// <summary>Dude 가상표가 아니면 세션 Dude 분배가 이전 처리기로 넘긴다.</summary>
    [Fact]
    public void DudeRegularFallsBackForOtherSids()
    {
        (HostSession session, _) = BuildDudeScene();
        int sid = PrepareDudeSid(session);
        Put(session.Pool.AllocatedBytes(sid), 0, 0x0050da88u);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.7f, handler(sid, 9, 0, 0.7f));
    }
}
