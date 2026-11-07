// 호스트 정기 예약(일반·공유 생성·수신·다리 낙하)의 부착·시각·진단·조회를 검증한다.
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// 정기 예약 호스트를 검증하는 테스트 클래스다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.
public sealed class RawSquidRegularReservationTests
{
    /// <summary>188개 빈 타입 목록을 만든다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> Types188() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();

    /// <summary>같은 풀에 묶은 삭제 몸체를 만든다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="types">188개 타입이다.</param>
    private static RawSquidDestroy DestroyFor(RawSidPool pool, List<OriginalTypeRecord> types) =>
        new(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types);

    /// <summary>정기 예약용 직접 호스트 장면을 만든다. 부착 본체 단위의 AttachScene과 같은 부모·폼 구성을 쓴다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.</summary>
    private static (RawSidPool Pool, Kernel Kernel, SquidProcessState State, SquidProcessHost Host, int Parent) ReservationScene()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        types[80].ConstructorAddress = OriginalConstructors.Address(pool.Edition, 2);
        var kernel = new Kernel(8u);
        var state = new SquidProcessState { CurrentProcessFormType = new UIntBox { Value = 80u } };
        var host = new SquidProcessHost(pool, types, kernel, DestroyFor(pool, types), state);
        int parent = pool.Allocate(SidAllocation.Server);
        pool.RawSlot(parent).Fill(0);
        return (pool, kernel, state, host, parent);
    }

    /// <summary>공유 수신 28바이트 패킷을 만든다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.</summary>
    /// <param name="parent">쓸 부모 번호다.</param>
    /// <param name="time">쓸 실행 시각이다.</param>
    /// <param name="ev">쓸 사건 번호다.</param>
    /// <param name="payload">쓸 실수 인자다.</param>
    /// <param name="count">쓸 횟수다.</param>
    private static byte[] SharedPacket(int parent, double time, uint ev, float payload, uint count)
    {
        var packet = new byte[28];
        // 원문 WORD/DWORD/double 필드만 낮은 바이트부터 쓴다
        BitConverter.GetBytes((ushort)parent).CopyTo(packet, 6);
        BitConverter.GetBytes(time).CopyTo(packet, 8);
        BitConverter.GetBytes(ev).CopyTo(packet, 16);
        BitConverter.GetBytes(payload).CopyTo(packet, 20);
        BitConverter.GetBytes(count).CopyTo(packet, 24);
        return packet;
    }

    /// <summary>일반 예약은 붙인 뒤 현재 시각을 다음 실행 시각으로 둔다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.</summary>
    [Fact]
    public void AddRegularAttachesAndStampsNow()
    {
        (RawSidPool _, Kernel _, SquidProcessState state, SquidProcessHost host, int parent) = ReservationScene();
        state.Now = 10.0;
        RegularProcess? regular = host.AddRegular(parent, 0x5101u, 1.5f);
        Assert.NotNull(regular);
        Assert.Equal(0x5101u, regular.Event());
        Assert.Equal(1.5f, regular.Payload());
        Assert.Equal(10.0, regular.Time());
        Assert.False(regular.Shared());
        Assert.Equal(parent, regular.Parent());
        Assert.Same(regular, host.FindEvent(parent, 0x5101u));
    }

    /// <summary>공유 예약은 실제 writer 구별을 켜고 같은 조회에 걸린다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.</summary>
    [Fact]
    public void AddSharedRegularMarksShared()
    {
        (RawSidPool _, Kernel _, SquidProcessState state, SquidProcessHost host, int parent) = ReservationScene();
        state.Now = 10.0;
        RegularProcess? regular = host.AddSharedRegular(parent, 0x5102u, 2.5f);
        Assert.NotNull(regular);
        Assert.True(regular.Shared());
        Assert.Equal(10.0, regular.Time());
        Assert.Same(regular, host.FindEvent(parent, 0x5102u));
    }

    /// <summary>빈 예약 입력은 받기 전에 거부한다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.</summary>
    [Fact]
    public void AttachRegularRejectsNull()
    {
        (RawSidPool _, Kernel _, SquidProcessState _, SquidProcessHost host, int parent) = ReservationScene();
        Assert.Throws<ArgumentNullException>(() => host.AttachRegular(null!, parent, false, 0x50u));
    }

    /// <summary>공유 예약은 깊이가 0이 아니면 원문 진단을 먼저 부른다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.</summary>
    [Fact]
    public void AttachRegularSharedDepthReportsDiagnostics()
    {
        (RawSidPool _, Kernel _, SquidProcessState state, SquidProcessHost host, int parent) = ReservationScene();
        state.PostPopDepth = 1u;
        Assert.Throws<InvalidOperationException>(() => host.AddSharedRegular(parent, 0x5103u, 1f));
        var calls = new List<(string Expression, string File, uint Line)>();
        state.Assertion = (expression, file, line) => calls.Add((expression, file, line));
        RegularProcess? regular = host.AddSharedRegular(parent, 0x5103u, 1f);
        Assert.NotNull(regular);
        Assert.Single(calls);
        Assert.Equal("basePostPop == 0", calls[0].Expression);
        Assert.Equal(0x70u, calls[0].Line);
    }

    /// <summary>현재 대여가 깊이 대리자 없이 공유 예약을 받으면 그 위치에서 거부한다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.</summary>
    [Fact]
    public void AttachRegularRuntimeWithoutDepthRejects()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        types[80].ConstructorAddress = OriginalConstructors.Address(pool.Edition, 2);
        var kernel = new Kernel(8u);
        var state = new SquidProcessState { CurrentProcessFormType = new UIntBox { Value = 80u } };
        var runtime = new ProcessHostRuntimeBindings { Destroy = DestroyFor(pool, types) };
        var host = new SquidProcessHost(pool, types, kernel, state, () => runtime);
        int parent = pool.Allocate(SidAllocation.Server);
        pool.RawSlot(parent).Fill(0);
        Assert.Throws<InvalidOperationException>(() => host.AddSharedRegular(parent, 0x5104u, 1f));
    }

    /// <summary>수신 패킷의 시각·횟수·사건을 복원하고 현재 시각으로 덮지 않는다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.</summary>
    [Fact]
    public void ReceiveSharedRegularRestoresPacketFields()
    {
        (RawSidPool pool, Kernel _, SquidProcessState state, SquidProcessHost host, int parent) = ReservationScene();
        state.Now = 99.0;
        byte[] packet = SharedPacket(parent, 12.5, 0x5105u, 3.25f, 7u);
        RegularProcess? regular = host.ReceiveSharedRegular(packet);
        Assert.NotNull(regular);
        Assert.True(regular.Shared());
        Assert.Equal(0x5105u, regular.Event());
        Assert.Equal(3.25f, regular.Payload());
        Assert.Equal(12.5, regular.Time());
        Assert.Equal(7u, regular.Count());
        Assert.Equal(parent, regular.Parent());
        Assert.Same(regular, host.FindEvent(parent, 0x5105u));
    }

    /// <summary>판본 밖·짧은 수신 패킷은 풀을 바꾸기 전에 거부한다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.</summary>
    [Fact]
    public void ReceiveSharedRegularRejectsContract()
    {
        (RawSidPool pool, Kernel _, SquidProcessState _, SquidProcessHost host, int parent) = ReservationScene();
        byte[] before = pool.Bytes.ToArray();
        Assert.Throws<ArgumentException>(() => host.ReceiveSharedRegular(new byte[10]));
        Assert.Equal(before, pool.Bytes.ToArray());
        var cdPool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, OriginalEdition.Cd1072);
        var cdTypes = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var cdKernel = new Kernel(8u);
        var cdState = new SquidProcessState();
        var cdHost = new SquidProcessHost(cdPool, cdTypes, cdKernel, cdState, () => null);
        Assert.Throws<ArgumentException>(() => cdHost.ReceiveSharedRegular(new byte[28]));
    }

    /// <summary>다리 낙하 예약은 0x2692 사건으로 걸고 조회에 걸린다. 2026-11-07 F2d Host 정기 예약 이식에서 추가됐다.</summary>
    [Fact]
    public void ScheduleBridgeFallReservesAndReports()
    {
        (RawSidPool _, Kernel _, SquidProcessState _, SquidProcessHost host, int parent) = ReservationScene();
        Assert.False(SquidProcessHost.HasScheduledBridgeFall(host, parent));
        RegularProcess? regular = SquidProcessHost.ScheduleBridgeFall(host, parent, 1f, 2f);
        Assert.NotNull(regular);
        Assert.Equal(0x2692u, regular.Event());
        Assert.Equal(RawBridgeLifecycle.DelayedFallPayload(1f, 2f), regular.Payload());
        Assert.True(SquidProcessHost.HasScheduledBridgeFall(host, parent));
        Assert.Throws<ArgumentNullException>(() => SquidProcessHost.ScheduleBridgeFall(null!, parent, 1f, 2f));
        Assert.Throws<ArgumentNullException>(() => SquidProcessHost.HasScheduledBridgeFall(null!, parent));
    }
}
