using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 펜스 Pop·정기 세션 결선을 검사하는 클래스다. 2026-11-19 0.636에서 R4-4 펜스 Pop용으로 추가했다.
// 근거: cpppj client/RawSurfaceFence.cpp의 펜스 분배와 o/RawFenceRegular.cpp의 정기 본체다.
public sealed class HostSessionFenceTests
{
    /// <summary>실제 자료로 펜스 장면을 만든다. 지형·요새·바인딩·지역·펜스를 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildFenceScene()
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

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 2026-11-19 0.636에서 추가했다.</summary>
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

    /// <summary>펜스 부모 슬롯을 할당하고 가상표·타입·좌표를 채운다. 2026-11-19 0.636에서 추가했다.</summary>
    /// <param name="session">펜스 장치가 연결된 세션이다.</param>
    /// <returns>채운 펜스 부모 번호다.</returns>
    private static int PrepareFenceSid(HostSession session)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x00509470u);
        raw[RawSidPool.TypeOffset] = 136;
        raw[RawSidPool.StateOffset] = 0;
        raw[40] = 0;
        raw[34] = 1;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(50.75f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(52.5f));
        Put(raw, 36, 0u);
        return sid;
    }

    /// <summary>펜스 Pop은 이웃 조회·소유자 캐시·연결 수·사건 0·2 예약을 연결한다.</summary>
    [Fact]
    public void FencePostPopAttachesEventZeroAndTwo()
    {
        (HostSession session, _) = BuildFenceScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareFenceSid(session);
        session.FencePostPop.PostPop(sid, 1u);
        Assert.NotNull(session.PlayerProcesses.FindEvent(sid, 0));
        Assert.NotNull(session.PlayerProcesses.FindEvent(sid, 2));
        Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(session.Pool.Slot(sid).Slice(42, 4)));
    }

    /// <summary>펜스 사건 0을 세션 처리기로 보내면 현재 글자 구간에서 프레임이 진행된다.</summary>
    [Fact]
    public void FenceRegularRoutesEventZeroThroughSession()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildFenceScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareFenceSid(session);
        int size = session.WorldFrames[136].Run((byte)'A').Count;
        Assert.True(size > 0, "펜스 정기 사건 0의 실제 프레임 구간이 비었다");
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        float result = handler(sid, 0, 0, 0.1f);
        Assert.Equal(0.1f, result);
        Assert.Equal(1 % size, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid).Slice(36, 4)));
    }

    /// <summary>펜스 사건 2를 세션 처리기로 보내면 재생성 뒤 0을 돌려준다. 삭제 unlink는 L2 덤프로 확인한다.</summary>
    [Fact]
    public void FenceRegularEventTwoRunsOnce()
    {
        (HostSession session, _) = BuildFenceScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareFenceSid(session);
        session.FencePostPop.PostPop(sid, 1u);
        Assert.NotNull(session.PlayerProcesses.FindEvent(sid, 2));
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.0f, handler(sid, 2, 1, 0.1f));
    }

    /// <summary>펜스 가상표가 아니면 세션 펜스 분배가 이전 처리기로 넘긴다.</summary>
    [Fact]
    public void FenceRegularFallsBackForOtherSids()
    {
        (HostSession session, _) = BuildFenceScene();
        int sid = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(sid).Fill(0);
        byte[] before = session.Pool.Slot(sid).ToArray();
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(3.25f, handler(sid, 0, 0, 3.25f));
        Assert.Equal(before, session.Pool.Slot(sid).ToArray());
    }

    /// <summary>펜스 이웃 탐색에 필요한 사거리가 파싱되는지 확인한다. 2026-11-19 0.636에서 추가했다.</summary>
    [Fact]
    public void FenceRangeIsParsedFromTypeFile()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildFenceScene();
        Assert.True(types[136].Range > 25f, $"sunFence range={types[136].Range}");
        _ = session;
    }

    /// <summary>장치가 없으면 펜스 Pop·분배 경로를 명시 예외로 멈춘다.</summary>
    [Fact]
    public void RejectsUnbuiltFence()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => session.FencePostPop);
    }
}
