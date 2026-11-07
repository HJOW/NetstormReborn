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
}
