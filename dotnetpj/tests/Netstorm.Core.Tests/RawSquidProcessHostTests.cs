// 프로세스 호스트 껍데기의 생성·조회·번호를 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 프로세스 호스트 껍데기를 검증하는 테스트 클래스다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.
public sealed class RawSquidProcessHostTests
{
    /// <summary>188개 타입 목록을 만든다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> Types188() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();

    /// <summary>같은 풀에 묶은 삭제 몸체를 만든다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="types">188개 타입이다.</param>
    private static RawSquidDestroy DestroyFor(RawSidPool pool, List<OriginalTypeRecord> types) =>
        new(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types);

    /// <summary>직접 생성자는 판본 개수와 같은 풀 삭제를 요구한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    [Fact]
    public void DirectCtorValidates()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState();
        var destroy = DestroyFor(pool, types);
        var host = new SquidProcessHost(pool, types, kernel, destroy, state);
        Assert.Same(pool, host.Pool());
        Assert.Equal(188, host.TypeCount());
        Assert.Same(types[7], host.Type(7));
        Assert.Throws<ArgumentOutOfRangeException>(() => host.Type(188));
        Assert.Throws<ArgumentOutOfRangeException>(() => host.Type(-1));
        Assert.Throws<ArgumentException>(() => new SquidProcessHost(pool, [new OriginalTypeRecord()], kernel, destroy, state));
        var otherPool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var otherDestroy = DestroyFor(otherPool, Types188());
        Assert.Throws<ArgumentException>(() => new SquidProcessHost(pool, types, kernel, otherDestroy, state));
        Assert.Throws<ArgumentNullException>(() => new SquidProcessHost(null!, types, kernel, destroy, state));
    }

    /// <summary>장기 생성자는 판본 개수와 현재 대여 getter를 요구한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    [Fact]
    public void LongLivedCtorValidates()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState();
        var host = new SquidProcessHost(pool, types, kernel, state, () => null);
        Assert.Same(pool, host.Pool());
        Assert.Equal(188, host.TypeCount());
        types[9].Flags2 = 0x4u;
        Assert.Equal(0x4u, host.Type(9).Flags2);
        Assert.Throws<ArgumentException>(() => new SquidProcessHost(pool, [new OriginalTypeRecord()], kernel, state, () => null));
        Assert.Throws<ArgumentNullException>(() => new SquidProcessHost(pool, types, kernel, state, null!));
    }

    /// <summary>빈 월드를 합성하지 않고 미연결 처리기를 그 위치에서 보고한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    [Fact]
    public void RuntimeRequiresConnectedWorld()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState();
        ProcessHostRuntimeBindings? current = null;
        var host = new SquidProcessHost(pool, types, kernel, state, () => current);
        Assert.Throws<InvalidOperationException>(() => host.Runtime());
        current = new ProcessHostRuntimeBindings();
        Assert.Throws<InvalidOperationException>(() => host.Runtime());
        var destroy = DestroyFor(pool, types);
        current.Destroy = destroy;
        Assert.Same(current, host.Runtime());
        Assert.Same(destroy, host.Destroy());
        var foreign = DestroyFor(new RawSidPool(RawSurfaceStorage.PoolCapacity), Types188());
        current.Destroy = foreign;
        Assert.Throws<InvalidOperationException>(() => host.Runtime());
    }

    /// <summary>직접 폴백과 현재 대여를 구별해 읽고 교체는 현재 위치에 쓴다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    [Fact]
    public void SelectorsReadRuntimeOrFallback()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState();
        var destroy = DestroyFor(pool, types);
        RegularHandler fallback = (_, _, _, payload) => payload;
        var asset = new SquidDestroyHooks();
        var transmit = new ProcessFormTransmission();
        var host = new SquidProcessHost(pool, types, kernel, destroy, state, fallback, asset, transmit);
        Assert.Same(destroy, host.Destroy());
        Assert.NotNull(host.Regular());
        Assert.Same(asset, host.Asset());
        Assert.Same(transmit, host.Transmission());
        RegularHandler next = (_, _, _, payload) => payload + 1f;
        var nextTransmit = new ProcessFormTransmission();
        host.SetRegularHandler(next);
        host.SetFormTransmit(nextTransmit);
        Assert.Equal(2f, host.Regular()!(0, 0u, 0u, 1f));
        Assert.Same(nextTransmit, host.Transmission());
        var runtime = new ProcessHostRuntimeBindings { Destroy = destroy };
        var runtimeHost = new SquidProcessHost(pool, types, kernel, state, () => runtime);
        Assert.Same(destroy, runtimeHost.Destroy());
        Assert.Null(runtimeHost.Regular());
        runtime.Regular = next;
        Assert.Equal(3f, runtimeHost.Regular()!(0, 0u, 0u, 2f));
        runtimeHost.SetRegularHandler(fallback);
        Assert.Equal(5f, runtime.Regular!(0, 0u, 0u, 5f));
    }

    /// <summary>같은 수명의 직접 공장에만 연결하고 장기 대여·다른 풀을 거부한다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    [Fact]
    public void BindFormFactoryPinsDirectLifetime()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState();
        var destroy = DestroyFor(pool, types);
        var host = new SquidProcessHost(pool, types, kernel, destroy, state);
        var factory = new RawSquidFactory(pool, types);
        host.BindFormFactory(factory);
        host.BindFormFactory(factory);
        var otherFactory = new RawSquidFactory(pool, Types188());
        Assert.Throws<ArgumentException>(() => host.BindFormFactory(otherFactory));
        var foreignPool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var foreignFactory = new RawSquidFactory(foreignPool, Types188());
        Assert.Throws<ArgumentException>(() => host.BindFormFactory(foreignFactory));
        var runtimeHost = new SquidProcessHost(pool, types, kernel, state, () => null);
        Assert.Throws<ArgumentException>(() => runtimeHost.BindFormFactory(factory));
        Assert.Throws<ArgumentNullException>(() => host.BindFormFactory(null!));
    }

    /// <summary>판본 번호와 슬롯 주소 판정이 원문과 같다. 2026-11-07 F2d 프로세스 호스트 이식에서 추가됐다.</summary>
    [Fact]
    public void NumbersAndRegistrationMatchNative()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState { CurrentProcessTypeEnd = new UIntBox { Value = 70u } };
        var host = new SquidProcessHost(pool, types, kernel, DestroyFor(pool, types), state);
        Assert.Equal(70u, host.ProcessTypeEnd());
        Assert.Equal(SquidProcessHost.PatchFormVtable, host.FormVtable());
        var cdPool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, OriginalEdition.Cd1072);
        var cdTypes = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        // 삭제 몸체의 188개 검사는 기존 C# 한계라 188개 표를 쓴다. 호스트는 자기 171개 표를 읽는다.
        var cdHost = new SquidProcessHost(cdPool, cdTypes, new Kernel(8u), DestroyFor(cdPool, Types188()), new SquidProcessState());
        Assert.Equal(171, cdHost.TypeCount());
        Assert.Equal(62u, cdHost.ProcessTypeEnd());
        Assert.Equal(SquidProcessHost.CdFormVtable, cdHost.FormVtable());
        var process = new SquidProcess();
        Assert.False(host.IsRegistered(1u, process));
        Assert.False(host.IsRegistered(1u, null));
        kernel.Add(process);
        Assert.True(host.IsRegistered(1u, process));
        kernel.Remove(1u);
        Assert.False(host.IsRegistered(1u, process));
    }

    /// <summary>같은 호스트·풀·상태로 조회3종 장면을 만든다. 2026-11-07 F2d 프로세스 호스트 조회 이식에서 추가됐다.</summary>
    private static (RawSidPool Pool, Kernel Kernel, SquidProcessState State, SquidProcessHost Host) LookupScene(uint formType = 80u)
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState { CurrentProcessFormType = new UIntBox { Value = formType } };
        var host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types), state);
        return (pool, kernel, state, host);
    }

    /// <summary>PID DWORD를 쓰는 정적 보조 함수다. 2026-11-07 F2d 프로세스 호스트 조회 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="pid">쓸 번호다.</param>
    private static void PutPid(RawSidPool pool, int sid, uint pid)
    {
        Span<byte> raw = pool.RawSlot(sid);
        // 낮은 바이트부터 지정 폭만 쓰는 반복문이다. 2026-11-07 F2d 프로세스 호스트 조회 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            raw[18 + index] = unchecked((byte)(pid >> (index * 8)));
        }
    }

    /// <summary>정렬되지 않은 form의 현재 커널 번호를 읽는다. 2026-11-07 F2d 프로세스 호스트 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void FromFormReadsCurrentKernelNumber()
    {
        (RawSidPool pool, Kernel kernel, SquidProcessState _, SquidProcessHost host) = LookupScene();
        Assert.Null(host.FromForm(5));
        var process = new SquidProcess();
        kernel.Add(process);
        PutPid(pool, 5, 1u);
        Assert.Same(process, host.FromForm(5));
        PutPid(pool, 5, 2u);
        Assert.Null(host.FromForm(5));
        kernel.Add(new KernelDiagnosticStub());
        PutPid(pool, 5, 2u);
        Assert.Null(host.FromForm(5));
    }

    /// <summary>커널에 직접 올리지 않는 검사 전용 프로세스다. 2026-11-07 F2d 프로세스 호스트 조회 이식에서 추가됐다.</summary>
    private sealed class KernelDiagnosticStub : BaseProcess
    {
    }

    /// <summary>지정 슬롯 Take와 신규 생성을 같은 공장에서 고른다. 2026-11-07 F2d 프로세스 호스트 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void CreateOrTakeFormSelectsFactoryPath()
    {
        (RawSidPool pool, Kernel _, SquidProcessState _, SquidProcessHost host) = LookupScene();
        int created = host.CreateOrTakeForm(0u, 0);
        Assert.True(created >= RawSidPool.FirstOrdinary);
        Assert.Equal(80u, pool.RawSlot(created)[10]);
        int taken = host.CreateOrTakeForm(0u, 15005);
        Assert.Equal(15005, taken);
        Assert.Equal(80u, pool.RawSlot(15005)[10]);
        int client = host.CreateOrTakeForm(0x10u, 0);
        Assert.True(client >= RawSidPool.FirstOrdinary);
        Assert.NotEqual(created, client);
    }

    /// <summary>다른 풀 공장과 비어 있는 런타임 공장은 거부한다. 2026-11-07 F2d 프로세스 호스트 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void CreateOrTakeFormRejectsForeignOrMissingFactory()
    {
        (RawSidPool pool, Kernel kernel, SquidProcessState state, SquidProcessHost _) = LookupScene();
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types);
        var runtime = new ProcessHostRuntimeBindings { Destroy = destroy };
        var host = new SquidProcessHost(pool, types, kernel, state, () => runtime);
        Assert.Throws<InvalidOperationException>(() => host.CreateOrTakeForm(0u, 0));
        var foreignPool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var foreignTypes = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        runtime.Factory = new RawSquidFactory(foreignPool, foreignTypes);
        Assert.Throws<ArgumentException>(() => host.CreateOrTakeForm(0u, 0));
    }

    /// <summary>통지를 기록하는 검사 전용 프로세스다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    private sealed class NotifyProcess : SquidProcess
    {
        /// <summary>마지막 통지 인자다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
        public (uint Event, uint Info, uint Value)? Seen { get; private set; }

        /// <summary>가상 통지를 기록한다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
        /// <param name="ev">사건 번호다.</param>
        /// <param name="info">부가 정보다.</param>
        /// <param name="value">전달 값이다.</param>
        public override void Notify(uint ev, uint info, uint value)
        {
            Seen = (ev, info, value);
        }
    }

    /// <summary>종속 체인 슬롯을 만든다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="type">쓸 타입 바이트다.</param>
    /// <param name="next">쓸 다음 번호다.</param>
    /// <param name="pid">쓸 PID DWORD다.</param>
    private static void ChainSlot(RawSidPool pool, int sid, byte type, int next, uint pid)
    {
        Span<byte> raw = pool.RawSlot(sid);
        raw[10] = type;
        raw[4] = unchecked((byte)next);
        raw[5] = unchecked((byte)(next >> 8));
        PutPid(pool, sid, pid);
    }

    /// <summary>부모 머리를 잇는 정적 보조 함수다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="parent">부를 부모 번호다.</param>
    /// <param name="head">쓸 머리 번호다.</param>
    private static void LinkHead(RawSidPool pool, int parent, int head)
    {
        Span<byte> raw = pool.RawSlot(parent);
        raw[6] = unchecked((byte)head);
        raw[7] = unchecked((byte)(head >> 8));
    }

    /// <summary>실제 가상 34는 server가 아니면 통지하지 않고 PID가 비면 거부한다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    [Fact]
    public void NotifyDependencyCallsCurrentPidNotify()
    {
        (RawSidPool pool, Kernel kernel, SquidProcessState state, SquidProcessHost host) = LookupScene();
        state.Server = false;
        host.NotifyDependency(100, 200, 300);
        state.Server = true;
        Assert.Throws<InvalidOperationException>(() => host.NotifyDependency(100, 200, 300));
        var process = new NotifyProcess();
        kernel.Add(process);
        PutPid(pool, 100, 1u);
        host.NotifyDependency(100, 200, 300);
        Assert.Equal((2u, 300u, 200u), process.Seen);
    }

    /// <summary>부모 체인에서 사건·마스크가 맞는 첫 Regular를 찾는다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    [Fact]
    public void FindEventWalksChainSkippingAssets()
    {
        (RawSidPool pool, Kernel kernel, SquidProcessState _, SquidProcessHost host) = LookupScene();
        var first = new RegularProcess(0x5101u, 1f);
        var second = new RegularProcess(0x5102u, 2f);
        kernel.Add(first);
        kernel.Add(second);
        ChainSlot(pool, 100, 46, 101, 1u);
        ChainSlot(pool, 101, 1, 102, 2u);
        ChainSlot(pool, 102, 46, 0, 2u);
        LinkHead(pool, 50, 100);
        Assert.Same(first, host.FindEvent(50, 0x5101u));
        Assert.Same(second, host.FindEvent(50, 0x5102u));
        Assert.Same(first, host.FindEventMasked(50, 0xfff0u, 0x5100u));
        Assert.Null(host.FindEvent(50, 0x5203u));
        Assert.Null(host.FindEventMasked(50, 0xfff0u, 0x5200u));
        LinkHead(pool, 51, 0);
        Assert.Null(host.FindEvent(51, 0x5101u));
        Assert.Throws<ArgumentOutOfRangeException>(() => host.FindEvent(0, 0x5101u));
        Assert.Throws<ArgumentOutOfRangeException>(() => host.FindEvent(RawSurfaceStorage.PoolCapacity, 0x5101u));
    }

    /// <summary>끊긴 체인·빈 PID·다른 클래스는 빈 검색으로 숨기지 않는다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    [Fact]
    public void FindRejectsBrokenChainAndForeignPid()
    {
        (RawSidPool pool, Kernel kernel, SquidProcessState _, SquidProcessHost host) = LookupScene();
        var regular = new RegularProcess(0x5101u, 1f);
        kernel.Add(regular);
        ChainSlot(pool, 100, 46, 100, 1u);
        LinkHead(pool, 50, 100);
        Assert.Throws<InvalidOperationException>(() => host.FindEvent(50, 0x5203u));
        ChainSlot(pool, 100, 46, 0, 9u);
        Assert.Throws<InvalidOperationException>(() => host.FindEvent(50, 0x5101u));
        kernel.Add(new NotifyProcess());
        ChainSlot(pool, 100, 46, 0, 2u);
        Assert.Throws<InvalidOperationException>(() => host.FindEvent(50, 0x5101u));
    }

    /// <summary>가상 삭제 효과를 처리할 현재 Process 가상 표인지 판단한다. 2026-11-07 F2d 프로세스 호스트 통지·검색 이식에서 추가됐다.</summary>
    [Fact]
    public void IsProcessFormMatchesVtable()
    {
        (RawSidPool pool, Kernel _, SquidProcessState _, SquidProcessHost host) = LookupScene();
        Assert.False(host.IsProcessForm(0));
        Assert.False(host.IsProcessForm(RawSurfaceStorage.PoolCapacity));
        Span<byte> raw = pool.RawSlot(60);
        raw[0] = 0x98;
        raw[1] = 0x20;
        raw[2] = 0x51;
        raw[3] = 0x00;
        Assert.True(host.IsProcessForm(60));
        raw[0] = 0x10;
        Assert.False(host.IsProcessForm(60));
    }

    /// <summary>삭제 통지를 기록하는 검사 전용 프로세스다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
    private sealed class DestroyProcess : SquidProcess
    {
        /// <summary>쓸 PID DWORD를 고치는 콜백이다. 통지 중 재진입을 시험한다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
        public Action? OnDestroy { get; set; }

        /// <summary>마지막 부모 통지다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
        public int? DyingParent { get; private set; }

        /// <summary>마지막 삭제 플래그다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
        public uint? DestroyFlags { get; private set; }

        /// <summary>form 삭제 직전 통지를 기록한다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
        /// <param name="flags">삭제 플래그다.</param>
        public override void OnFormDestroy(uint flags)
        {
            DestroyFlags = flags;
            OnDestroy?.Invoke();
        }

        /// <summary>부모 삭제 중 통지를 기록한다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
        /// <param name="parent">죽는 부모다.</param>
        public override void OnParentDying(int parent)
        {
            DyingParent = parent;
        }
    }

    /// <summary>같은 호스트·삭제 몸체로 삭제 통지 장면을 만든다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
    private static (RawSidPool Pool, Kernel Kernel, RawSquidDestroy Destroy, SquidProcessHost Host) DestroyScene()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState();
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types);
        return (pool, kernel, destroy, new SquidProcessHost(pool, types, kernel, destroy, state));
    }

    /// <summary>프로세스에 통지하고 커널에서 제거한 뒤 삭제 깊이를 줄인다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
    [Fact]
    public void FormPreDestroyNotifiesRemovesAndCompletes()
    {
        (RawSidPool pool, Kernel kernel, RawSquidDestroy destroy, SquidProcessHost host) = DestroyScene();
        var process = new DestroyProcess();
        kernel.Add(process);
        PutPid(pool, 200, 1u);
        host.FormPreDestroy(200, 0x10u);
        Assert.Equal(0x10u, process.DestroyFlags);
        Assert.Null(kernel.Get(1u));
        Assert.Equal(0, kernel.Size());
        Assert.Equal(uint.MaxValue, destroy.PreDepth);
    }

    /// <summary>원본은 통지가 끝난 뒤 Form의 현재 PID를 다시 읽는다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
    [Fact]
    public void FormPreDestroyRereadsPidAfterNotify()
    {
        (RawSidPool pool, Kernel kernel, RawSquidDestroy _, SquidProcessHost host) = DestroyScene();
        var first = new DestroyProcess();
        var second = new DestroyProcess();
        kernel.Add(first);
        kernel.Add(second);
        PutPid(pool, 200, 1u);
        first.OnDestroy = () => PutPid(pool, 200, 2u);
        host.FormPreDestroy(200, 0u);
        Assert.Same(first, kernel.Get(1u));
        Assert.Null(kernel.Get(2u));
        Assert.Equal(1, kernel.Size());
    }

    /// <summary>비어 있는 번호·다른 종류는 통지 전에 거부한다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
    [Fact]
    public void FormPreDestroyRejectsMissing()
    {
        (RawSidPool pool, Kernel kernel, RawSquidDestroy _, SquidProcessHost host) = DestroyScene();
        PutPid(pool, 200, 1u);
        Assert.Throws<InvalidOperationException>(() => host.FormPreDestroy(200, 0u));
        kernel.Add(new BaseProcess());
        PutPid(pool, 201, 1u);
        Assert.Throws<InvalidOperationException>(() => host.FormPreDestroy(201, 0u));
    }

    /// <summary>NULL 조회는 빈 통지로 생략하고 살아 있는 프로세스에만 알린다. 2026-11-07 F2d 프로세스 삭제 통지 이식에서 추가됐다.</summary>
    [Fact]
    public void FormParentDyingNotifiesOrSkips()
    {
        (RawSidPool pool, Kernel kernel, RawSquidDestroy _, SquidProcessHost host) = DestroyScene();
        PutPid(pool, 200, 1u);
        host.FormParentDying(200, 77);
        var process = new DestroyProcess();
        kernel.Add(process);
        PutPid(pool, 200, 1u);
        host.FormParentDying(200, 77);
        Assert.Equal(77, process.DyingParent);
        kernel.Add(new BaseProcess());
        PutPid(pool, 201, 2u);
        Assert.Throws<InvalidOperationException>(() => host.FormParentDying(201, 77));
    }

    /// <summary>같은 호스트·커널로 잔류 조회 장면을 만든다. 2026-11-07 F2d 프로세스 호스트 잔류 이식에서 추가됐다.</summary>
    private static (Kernel Kernel, SquidProcessHost Host) ResidualScene()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState();
        var host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types), state);
        return (kernel, host);
    }

    /// <summary>잔류 객체가 실제로 이 Host를 빌리는지 조회한다. 2026-11-07 F2d 프로세스 호스트 잔류 이식에서 추가됐다.</summary>
    [Fact]
    public void OwnsPinsLinkedHost()
    {
        (Kernel _, SquidProcessHost host) = ResidualScene();
        var process = new SquidProcess();
        Assert.False(host.Owns(process));
        Assert.False(host.Owns(new BaseProcess()));
        process.HostLink = host;
        Assert.True(host.Owns(process));
        var other = new SquidProcessHost(
            new RawSidPool(RawSurfaceStorage.PoolCapacity),
            Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList(),
            new Kernel(8u),
            new SquidProcessState(),
            () => null);
        process.HostLink = other;
        Assert.False(host.Owns(process));
        Assert.Throws<ArgumentNullException>(() => host.Owns(null!));
    }

    /// <summary>현재 Host의 잔류 중 Regular 이외 수명은 거짓이다. 2026-11-07 F2d 프로세스 호스트 잔류 이식에서 추가됐다.</summary>
    [Fact]
    public void HasOnlyRegularProcessesGuardsNonRegular()
    {
        (Kernel kernel, SquidProcessHost host) = ResidualScene();
        Assert.True(host.HasOnlyRegularProcesses());
        var foreign = new RegularProcess(1u, 0f);
        kernel.Add(foreign);
        Assert.True(host.HasOnlyRegularProcesses());
        var owned = new RegularProcess(2u, 0f);
        owned.HostLink = host;
        kernel.Add(owned);
        Assert.True(host.HasOnlyRegularProcesses());
        var plain = new SquidProcess();
        plain.HostLink = host;
        kernel.Add(plain);
        Assert.False(host.HasOnlyRegularProcesses());
        kernel.Remove(3u);
        Assert.True(host.HasOnlyRegularProcesses());
    }

    /// <summary>독립 월드의 최종 fallback만 자기 현재 대여를 정리한다. 2026-11-07 F2d 프로세스 호스트 잔류 이식에서 추가됐다.</summary>
    [Fact]
    public void DrainRuntimeBindingsKeepsForeignResidual()
    {
        (Kernel kernel, SquidProcessHost host) = ResidualScene();
        var runtime = new ProcessHostRuntimeBindings();
        var owned = new RegularProcess(1u, 0f);
        owned.HostLink = host;
        owned.RuntimeLink = runtime;
        kernel.Add(owned);
        var foreign = new RegularProcess(2u, 0f);
        kernel.Add(foreign);
        var other = new ProcessHostRuntimeBindings();
        var stray = new RegularProcess(3u, 0f);
        stray.HostLink = host;
        stray.RuntimeLink = other;
        kernel.Add(stray);
        host.DrainRuntimeBindings(runtime);
        Assert.Null(kernel.Get(1u));
        Assert.NotNull(kernel.Get(2u));
        Assert.NotNull(kernel.Get(3u));
        Assert.Equal(2, kernel.Size());
        host.DrainRuntimeBindings(null);
        Assert.Equal(2, kernel.Size());
    }

    /// <summary>부모·폼 체인 슬롯을 만든다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="parent">부모 번호다.</param>
    /// <param name="form">폼 번호다.</param>
    private static void FormChain(RawSidPool pool, int parent, int form)
    {
        Span<byte> head = pool.RawSlot(parent);
        head.Fill(0);
        head[10] = 1;
        head[6] = unchecked((byte)form);
        head[7] = unchecked((byte)(form >> 8));
        Span<byte> raw = pool.RawSlot(form);
        raw.Fill(0);
        raw[10] = 2;
        raw[11] = 8;
        raw[16] = unchecked((byte)parent);
        raw[17] = unchecked((byte)(parent >> 8));
        // 타입 2의 생성자 가상 표다
        raw[0] = 0x98;
        raw[1] = 0x20;
        raw[2] = 0x51;
        raw[3] = 0x00;
    }

    /// <summary>독립 Host는 공통 순서로 폼을 풀고 체인을 끊는다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void UnpopFormFallbackReleasesChain()
    {
        (RawSidPool pool, Kernel _, RawSquidDestroy _, SquidProcessHost host) = DestroyScene();
        FormChain(pool, 100, 101);
        host.UnpopForm(101);
        Assert.Equal(4, pool.Slot(101)[11]);
        Assert.Equal(0, pool.Slot(100)[6] | (pool.Slot(100)[7] << 8));
        Assert.Throws<InvalidOperationException>(() => host.UnpopForm(101));
    }

    /// <summary>현재 공장이 비어 있는 런타임 분리는 거부한다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void UnpopFormRuntimeWithoutFactoryThrows()
    {
        (RawSidPool pool, Kernel kernel, SquidProcessState state, SquidProcessHost _) = LookupScene();
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types);
        var runtime = new ProcessHostRuntimeBindings { Destroy = destroy };
        var host = new SquidProcessHost(pool, types, kernel, state, () => runtime);
        Assert.Throws<InvalidOperationException>(() => host.UnpopForm(101));
    }

    /// <summary>직접 공장은 Form 분리를 아직 잇지 않아 명시 오류다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void UnpopFormDirectFactoryThrowsExplicit()
    {
        (RawSidPool pool, Kernel _, RawSquidDestroy _, SquidProcessHost host) = DestroyScene();
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        host.BindFormFactory(new RawSquidFactory(pool, types));
        Assert.Throws<InvalidOperationException>(() => host.UnpopForm(101));
    }

    /// <summary>권한이 없으면 서버 번호·플래그 없는 삭제는 조용히 둔다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void KillSkipsWithoutAuthority()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState { Boss = false };
        var host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types), state);
        var process = new SquidProcess();
        process.SetIdentity(16000, 0, 0u);
        host.Kill(process, 0u);
        Assert.Throws<ArgumentNullException>(() => host.Kill(null!, 0u));
    }

    /// <summary>0번·죽음·범위 밖 폼 삭제는 거부한다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void KillRejectsBadForm()
    {
        (RawSidPool pool, Kernel _, RawSquidDestroy _, SquidProcessHost host) = DestroyScene();
        var empty = new SquidProcess();
        empty.SetIdentity(0, 0, 0u);
        Assert.Throws<InvalidOperationException>(() => host.Kill(empty, 0u));
        pool.RawSlot(200)[11] = 2;
        var dead = new SquidProcess();
        dead.SetIdentity(200, 0, 0u);
        Assert.Throws<InvalidOperationException>(() => host.Kill(dead, 0u));
        var outside = new SquidProcess();
        outside.SetIdentity(pool.Capacity, 0, 0u);
        Assert.Throws<InvalidOperationException>(() => host.Kill(outside, 0u));
    }

    /// <summary>폼 뿌리 삭제는 효과 순서대로 풀고 반납한다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void KillDestroysRoot()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState();
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types);
        var seen = new List<SquidDestroyEvent>();
        var asset = new SquidDestroyHooks
        {
            Emit = e =>
            {
                seen.Add(e);
                if (e.Effect == SquidDestroyEffect.PreDestroy)
                {
                    destroy.CompletePreDestroy();
                }
                else if (e.Effect == SquidDestroyEffect.PostDestroy)
                {
                    destroy.CompletePostDestroy();
                }
            },
            Selected = () => 0,
        };
        var host = new SquidProcessHost(pool, types, kernel, destroy, state, null, asset);
        FormChain(pool, 100, 101);
        var process = new SquidProcess();
        kernel.Add(process);
        PutPid(pool, 101, 1u);
        process.SetIdentity(101, 100, 1u);
        host.Kill(process, 0u);
        Assert.Null(kernel.Get(1u));
        Assert.Equal(0u, destroy.PreDepth);
        Assert.Equal(0u, destroy.PostDepth);
        Assert.NotEqual(0, pool.Slot(101)[11] & 1);
    }

    /// <summary>선택 조회는 현재 자산·없음 기본값을 따른다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void HooksSelectedDefaults()
    {
        (RawSidPool pool, Kernel kernel, SquidProcessState state, SquidProcessHost bare) = LookupScene();
        Assert.Equal(0, bare.Hooks().Selected!());
        var asset = new SquidDestroyHooks { Selected = () => 7 };
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var direct = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types), state, null, asset);
        Assert.Equal(7, direct.Hooks().Selected!());
        var runtimeAsset = new SquidDestroyHooks { Selected = () => 9 };
        var runtime = new ProcessHostRuntimeBindings
        {
            Destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types),
            Asset = runtimeAsset,
        };
        var host = new SquidProcessHost(pool, types, kernel, state, () => runtime);
        Assert.Equal(9, host.Hooks().Selected!());
    }

    /// <summary>폼이 아니면 자산에 그대로 넘기고 없으면 거부한다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void HooksEmitForwardsOrRejects()
    {
        (RawSidPool pool, Kernel _, RawSquidDestroy _, SquidProcessHost bare) = DestroyScene();
        pool.RawSlot(300).Fill(0);
        var plain = new SquidDestroyEvent(SquidDestroyEffect.PreDestroy, 300, 0, 0u);
        Assert.Throws<InvalidOperationException>(() => bare.Hooks().Emit!(plain));
        SquidDestroyEvent? forwarded = null;
        var asset = new SquidDestroyHooks { Emit = e => forwarded = e, Selected = () => 0 };
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var kernel = new Kernel(8u);
        var host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types), new SquidProcessState(), null, asset);
        host.Hooks().Emit!(plain);
        Assert.Equal(plain, forwarded);
    }

    /// <summary>묶음의 분리 콜백은 호스트 분리로 이어진다. 2026-11-07 F2d 프로세스 호스트 삭제 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void HooksUnpopFormWired()
    {
        (RawSidPool pool, Kernel _, RawSquidDestroy _, SquidProcessHost host) = DestroyScene();
        Assert.NotNull(host.Hooks().UnpopForm);
        Span<byte> raw = pool.RawSlot(9999);
        raw.Fill(0);
        raw[10] = 2;
        raw[0] = 0x98;
        raw[1] = 0x20;
        raw[2] = 0x51;
        raw[3] = 0x00;
        Assert.Throws<InvalidOperationException>(() => host.Hooks().UnpopForm!(9999));
    }

    /// <summary>정기 실행 장면을 만든다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    /// <param name="handler">호출을 기록한 뒤 돌려줄 값을 정하는 처리기다.</param>
    /// <param name="now">현재 시각이다.</param>
    private static (RawSidPool Pool, Kernel Kernel, SquidProcessState State, SquidProcessHost Host, RegularProcess Process, uint Pid, List<(int Parent, uint Event, uint Count, float Payload)> Calls) RegularScene(Func<int, uint, uint, float, float> handler, double now = 10.0)
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState { Now = now };
        var calls = new List<(int, uint, uint, float)>();
        RegularHandler spy = (parent, ev, count, payload) =>
        {
            calls.Add((parent, ev, count, payload));
            return handler(parent, ev, count, payload);
        };
        var host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types), state, spy);
        Span<byte> parent = pool.RawSlot(100);
        parent.Fill(0);
        parent[10] = 80;
        var process = new RegularProcess(7u, 1.5f);
        process.ResetTask(7u, now);
        uint pid = kernel.Add(process);
        process.SetIdentity(0, 100, pid);
        return (pool, kernel, state, host, process, pid, calls);
    }

    /// <summary>죽음·빈 부모는 처리기를 부르지 않는다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    [Fact]
    public void RunRegularSkipsDeadParent()
    {
        (RawSidPool pool, Kernel _, SquidProcessState _, SquidProcessHost host, RegularProcess process, uint _, List<(int Parent, uint Event, uint Count, float Payload)> calls) = RegularScene((_, _, _, _) => 2.5f);
        pool.RawSlot(100)[11] = 1;
        host.RunRegular(process);
        pool.RawSlot(100)[11] = 2;
        host.RunRegular(process);
        Assert.Empty(calls);
        Assert.Equal(0u, process.Count());
    }

    /// <summary>미래 시각·void 부모는 처리기를 부르지 않는다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    [Fact]
    public void RunRegularSkipsFutureOrVoid()
    {
        (RawSidPool pool, Kernel _, SquidProcessState state, SquidProcessHost host, RegularProcess process, uint _, List<(int Parent, uint Event, uint Count, float Payload)> calls) = RegularScene((_, _, _, _) => 2.5f);
        process.ResetTask(7u, state.Now + 10.0);
        host.RunRegular(process);
        Assert.Empty(calls);
        process.ResetTask(7u, state.Now);
        pool.RawSlot(100)[11] = 4;
        host.RunRegular(process);
        Assert.Empty(calls);
        Assert.Equal(0u, process.Count());
    }

    /// <summary>간헐천의 먼 예약은 당겨서 실행한다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    [Fact]
    public void RunRegularClampsGeyserReservation()
    {
        (RawSidPool pool, Kernel _, SquidProcessState state, SquidProcessHost host, RegularProcess process, uint _, List<(int Parent, uint Event, uint Count, float Payload)> calls) = RegularScene((_, _, _, _) => 2.5f);
        pool.RawSlot(100)[10] = 122;
        process.ResetTask(7u, state.Now + 10.0);
        host.RunRegular(process);
        Assert.Single(calls);
        Assert.Equal(2.5f, process.Payload());
        Assert.Equal(2.5 + state.Now, process.Time());
        Assert.Equal(1u, process.Count());
    }

    /// <summary>양수 반환은 다음 payload·시각·횟수가 된다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    [Fact]
    public void RunRegularReschedulesPositive()
    {
        (RawSidPool _, Kernel _, SquidProcessState state, SquidProcessHost host, RegularProcess process, uint _, List<(int Parent, uint Event, uint Count, float Payload)> calls) = RegularScene((_, _, _, _) => 2.5f);
        host.RunRegular(process);
        Assert.Single(calls);
        Assert.Equal((100, 7u, 0u, 1.5f), calls[0]);
        Assert.Equal(2.5f, process.Payload());
        Assert.Equal(2.5 + state.Now, process.Time());
        Assert.Equal(1u, process.Count());
    }

    /// <summary>0 반환은 삭제 호출로 이어진다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    [Fact]
    public void RunRegularKillsOnZero()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var kernel = new Kernel(8u);
        var state = new SquidProcessState { Now = 10.0, Boss = false };
        var host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types), state, (_, _, _, _) => 0f);
        Span<byte> parent = pool.RawSlot(100);
        parent.Fill(0);
        parent[10] = 80;
        var process = new RegularProcess(7u, 1.5f);
        process.ResetTask(7u, 10.0);
        uint pid = kernel.Add(process);
        process.SetIdentity(16000, 100, pid);
        host.RunRegular(process);
        Assert.Same(process, kernel.Get(pid));
        Assert.Equal(0u, process.Count());
        Assert.Equal(1.5f, process.Payload());
        Assert.Throws<ArgumentNullException>(() => host.RunRegular(null!));
    }

    /// <summary>패치판 NaN은 재예약하고 CD판 NaN은 둔다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    [Fact]
    public void RunRegularReschedulesPatchNaNOnly()
    {
        (RawSidPool _, Kernel _, SquidProcessState _, SquidProcessHost host, RegularProcess process, uint _, List<(int Parent, uint Event, uint Count, float Payload)> _) = RegularScene((_, _, _, _) => float.NaN);
        host.RunRegular(process);
        Assert.Equal(1u, process.Count());
        Assert.Equal(float.NaN, process.Payload());
        var cdPool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, OriginalEdition.Cd1072);
        var cdTypes = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var cdKernel = new Kernel(8u);
        var cdState = new SquidProcessState { Now = 10.0 };
        var cdHost = new SquidProcessHost(cdPool, cdTypes, cdKernel, new RawSquidDestroy(cdPool, new RawUnpop(cdPool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), Types188()), cdState, (_, _, _, _) => float.NaN);
        Span<byte> parent = cdPool.RawSlot(100);
        parent.Fill(0);
        parent[10] = 80;
        var cdProcess = new RegularProcess(7u, 1.5f);
        cdProcess.ResetTask(7u, 10.0);
        cdProcess.SetIdentity(0, 100, cdKernel.Add(cdProcess));
        cdHost.RunRegular(cdProcess);
        Assert.Equal(0u, cdProcess.Count());
        Assert.Equal(1.5f, cdProcess.Payload());
    }

    /// <summary>추상 부모는 패치판만 건너뛴다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    [Fact]
    public void RunRegularSkipsAbstractPatchOnly()
    {
        (RawSidPool pool, Kernel _, SquidProcessState _, SquidProcessHost host, RegularProcess process, uint _, List<(int Parent, uint Event, uint Count, float Payload)> calls) = RegularScene((_, _, _, _) => 2.5f);
        pool.RawSlot(100)[40] = 1;
        host.RunRegular(process);
        Assert.Empty(calls);
        var cdPool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, OriginalEdition.Cd1072);
        var cdTypes = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var cdKernel = new Kernel(8u);
        var cdState = new SquidProcessState { Now = 10.0 };
        int cdCalls = 0;
        var cdHost = new SquidProcessHost(cdPool, cdTypes, cdKernel, new RawSquidDestroy(cdPool, new RawUnpop(cdPool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), Types188()), cdState, (_, _, _, _) => { ++cdCalls; return 2.5f; });
        Span<byte> parent = cdPool.RawSlot(100);
        parent.Fill(0);
        parent[10] = 80;
        parent[40] = 1;
        var cdProcess = new RegularProcess(7u, 1.5f);
        cdProcess.ResetTask(7u, 10.0);
        cdProcess.SetIdentity(0, 100, cdKernel.Add(cdProcess));
        cdHost.RunRegular(cdProcess);
        Assert.Equal(1, cdCalls);
    }

    /// <summary>처리기가 지운 프로세스는 건드리지 않는다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    [Fact]
    public void RunRegularSkipsWhenHandlerDeletes()
    {
        (RawSidPool _, Kernel kernel, SquidProcessState _, SquidProcessHost host, RegularProcess process, uint pid, List<(int Parent, uint Event, uint Count, float Payload)> _) = RegularScene((_, _, _, _) => 5f);
        bool ran = false;
        host.SetRegularHandler((_, _, _, _) =>
        {
            ran = true;
            kernel.Remove(pid);
            return 5f;
        });
        host.RunRegular(process);
        Assert.True(ran);
        Assert.Equal(0u, process.Count());
        Assert.Null(kernel.Get(pid));
    }

    /// <summary>연결된 RunFrame은 호스트 실행으로 이어진다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    [Fact]
    public void RunFrameRunsRegularWhenLinked()
    {
        (RawSidPool _, Kernel _, SquidProcessState _, SquidProcessHost host, RegularProcess process, uint _, List<(int Parent, uint Event, uint Count, float Payload)> calls) = RegularScene((_, _, _, _) => 2.5f);
        process.HostLink = host;
        process.RunFrame();
        Assert.Single(calls);
        Assert.Equal(1u, process.Count());
    }
}
