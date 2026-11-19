using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 장식 정기 세션 결선을 검사하는 클래스다. 2026-11-19 0.642에서 R4-4 장식 정기용으로 추가했다.
// 근거: cpppj client/RawSurfaceDecoration.cpp의 장식 분배와 o/RawDecorationRegular.cpp의 정기 본체다.
public sealed class HostSessionDecorationTests
{
    /// <summary>실제 자료로 장식 장면을 만든다. 지형·요새·바인딩·지역·장식을 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildDecorationScene()
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

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 0.642에서 추가했다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>장식 부모 슬롯을 할당하고 가상표·타입·HP를 채운다. 0.642에서 추가했다.</summary>
    /// <param name="session">장식 장치가 연결된 세션이다.</param>
    /// <param name="types">같은 실제 TYPE 표다. 최대 HP를 덮어쓴다.</param>
    /// <returns>채운 장식 부모 번호다.</returns>
    private static int PrepareDecorationSid(HostSession session, IReadOnlyList<OriginalTypeRecord> types)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x0050a820u);
        raw[RawSidPool.TypeOffset] = 20;
        // 최대 900·개수 0이면 임계값 900이다. 현재 1000이면 100을 차감한다.
        // 받침 분기를 피하도록 dais 비트를 지운다. 장면 표는 검사마다 새로 읽으므로 공유 자료에 닿지 않는다.
        types[20].Flags2 &= ~OriginalTypeRecord.Dais;
        types[20].MaxHitPoints = 900;
        Put(raw, 26, 1000u);
        return sid;
    }

    /// <summary>장식 사건 0을 세션 처리기로 보내면 현재 HP가 임계값까지 차감된다.</summary>
    [Fact]
    public void DecorationRegularRoutesEventZeroThroughSession()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildDecorationScene();
        int sid = PrepareDecorationSid(session, types);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        float result = handler(sid, 0, 0, 0.4f);
        Assert.Equal(0.4f, result);
        Assert.Equal(900, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid).Slice(26, 4)));
    }

    /// <summary>두 번째 장식 가상표도 세션에서 같은 본체로 처리한다.</summary>
    [Fact]
    public void DecorationRegularRoutesSecondVtable()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildDecorationScene();
        int sid = PrepareDecorationSid(session, types);
        Put(session.Pool.AllocatedBytes(sid), 0, 0x0050a8e8u);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        float result = handler(sid, 0, 0, 0.4f);
        Assert.Equal(0.4f, result);
        Assert.Equal(900, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid).Slice(26, 4)));
    }

    /// <summary>장식 가상표가 아니면 세션 장식 분배가 이전 처리기로 넘긴다.</summary>
    [Fact]
    public void DecorationRegularFallsBackForOtherSids()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildDecorationScene();
        int sid = PrepareDecorationSid(session, types);
        Put(session.Pool.AllocatedBytes(sid), 0, 0x0050da88u);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.7f, handler(sid, 9, 0, 0.7f));
        Assert.Equal(1000, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid).Slice(26, 4)));
    }
}
