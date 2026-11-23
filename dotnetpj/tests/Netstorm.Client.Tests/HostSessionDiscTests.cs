using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// Disc 정기 세션 분배를 검사하는 클래스다. 2026-11-23 0.893에서 R4-4 Disc 정기용으로 추가했다.
// 근거: cpppj client/RawSurfaceFrameRegular.cpp의 DiscRegular 조립이다.
public sealed class HostSessionDiscTests
{
    /// <summary>실제 자료로 Disc 장면을 만든다. 지형·요새·운반체·지역·Disc를 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildDiscScene()
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

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 0.893에서 추가했다.</summary>
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

    /// <summary>방향/클러스터 표가 유효한 첫 타입 번호를 찾는다. 0.893에서 추가했다.</summary>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <returns>NextFrame이 진행하는 타입 번호다.</returns>
    private static int FindDiscType(IReadOnlyList<OriginalTypeRecord> types)
    {
        // 실제 자료의 방향/클러스터 값을 그대로 쓰며 합성 표는 넣지 않는다.
        for (int number = 0; number < types.Count; number++)
        {
            if (types[number].Directions > 0 && types[number].FrameCount >= types[number].Directions)
            {
                return number;
            }
        }

        throw new InvalidOperationException("Disc 진행 가능 타입 없음");
    }

    /// <summary>Disc 부모 슬롯을 할당하고 가상표·타입·프레임을 채운다. 0.893에서 추가했다.</summary>
    /// <param name="session">Disc 장치가 연결된 세션이다.</param>
    /// <param name="type">채울 타입 번호다.</param>
    /// <returns>채운 Disc 부모 번호다.</returns>
    private static int PrepareDiscSid(HostSession session, int type)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x0050b570u);
        raw[RawSidPool.TypeOffset] = unchecked((byte)type);
        return sid;
    }

    /// <summary>Disc 사건을 세션 처리기로 보내면 현재 프레임이 다음 프레임으로 지정되고 payload가 돌아온다.</summary>
    [Fact]
    public void DiscRegularRoutesThroughSessionWithNextFrame()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildDiscScene();
        int type = FindDiscType(types);
        int sid = PrepareDiscSid(session, type);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.7f, handler(sid, 9, 0, 0.7f));
        int next = RawDiscRegular.NextFrame(0, types[type].FrameCount, types[type].Directions);
        Assert.Equal(next, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid).Slice(36, 4)));
    }

    /// <summary>Disc 가상표가 아니면 세션 Disc 분배가 이전 처리기로 넘긴다.</summary>
    [Fact]
    public void DiscRegularFallsBackForOtherSids()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildDiscScene();
        int sid = PrepareDiscSid(session, FindDiscType(types));
        Put(session.Pool.AllocatedBytes(sid), 0, 0x0050da88u);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.7f, handler(sid, 9, 0, 0.7f));
    }
}
