// 게임 보조 절차 번호와 프로세스 호스트 저장소 껍데기의 배선을 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 프로세스 호스트 저장소 껍데기를 검증하는 테스트 클래스다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.
public sealed class RawProcessHostStorageTests
{
    /// <summary>세 자산 목록으로 실제 저장소를 만든다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private static RawSurfaceStorage FullStorage()
    {
        var types = new List<OriginalTypeRecord> { new(), new(), new() };
        var frames = new List<RiftTypeFrames> { new([]), new([]), new([]) };
        var shapes = new List<SquidDisplayShape> { new(), new(), new() };
        return new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
    }

    /// <summary>절차 번호 6종은 원문 PE 초기값으로 시작하고 상자를 공유한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void GameAuxProcessNumbersShareBoxes()
    {
        var gameAux = new RawGameAuxState();
        Assert.Equal(2u, gameAux.ProcessFormType);
        Assert.Equal(46u, gameAux.RegularProcessType);
        Assert.Equal(61u, gameAux.SharedRegularProcessType);
        Assert.Equal(63u, gameAux.ProcessTypeEnd);
        Assert.Equal(35u, gameAux.FireAnimationProcessType);
        Assert.Equal(36u, gameAux.RotateAnimationProcessType);
        gameAux.ProcessFormType = 9u;
        Assert.Equal(9u, gameAux.ProcessFormTypeBox.Value);
        gameAux.RegularProcessTypeBox.Value = 48u;
        Assert.Equal(48u, gameAux.RegularProcessType);
        gameAux.ResetForGeneralWorld();
        Assert.Equal(9u, gameAux.ProcessFormType);
        Assert.Equal(48u, gameAux.RegularProcessType);
        Assert.Equal(122u, gameAux.GeyserType);
    }

    /// <summary>생성자는 장기 권한·타입 주소를 빌리고 절차 번호 상자를 pin한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void CtorPinsSharedOwnersAndNumbers()
    {
        RawSurfaceStorage storage = FullStorage();
        RawProcessHostStorage host = storage.Processes();
        Assert.Same(storage.Processes(), host);
        SquidProcessState state = host.State();
        Assert.True(state.Boss);
        Assert.True(state.Server);
        Assert.Equal(122u, state.PollType);
        Assert.Equal(2u, state.ProcessFormType());
        Assert.Equal(46u, state.RegularType());
        Assert.Equal(61u, state.SharedRegularType());
        Assert.Equal(63u, state.ProcessTypeEnd());
        Assert.Equal(35u, state.FireAnimationType());
        Assert.Equal(36u, state.RotateAnimationType());
        storage.Authority().Boss = false;
        Assert.False(state.Boss);
        state.Server = false;
        Assert.False(storage.Authority().Server);
        storage.GameAux().ProcessFormType = 11u;
        Assert.Equal(11u, state.ProcessFormType());
        state.PollType = 131u;
        Assert.Equal(131u, storage.GameAux().GeyserType);
        Assert.False(host.HasDevices());
        Assert.False(host.UsesKernel(null));
    }

    /// <summary>CD 판본은 일반·공유·상한·발사·회전 빌림을 두지 않는다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void CdSkipsPatchOnlyNumberBorrowing()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, OriginalEdition.Cd1072);
        var host = new RawProcessHostStorage(pool, [], null, null, new RawGameAuxState());
        SquidProcessState state = host.State();
        Assert.Equal(2u, state.ProcessFormType());
        Assert.Equal(46u, state.RegularType());
        Assert.Null(state.CurrentRegularType);
        Assert.Null(state.CurrentFireAnimationType);
        Assert.False(host.HasDevices());
    }

    /// <summary>같은 장기 장치의 재확인은 바꾸지 않고 다른 장치는 거부한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void BindDevicesPinsContract()
    {
        RawSurfaceStorage storage = FullStorage();
        RawProcessHostStorage host = storage.Processes();
        var kernel = new Kernel(7u);
        var clock = new FrameTimeBox();
        var poll = new UIntBox { Value = 122u };
        host.BindDevices(kernel, clock, poll);
        Assert.True(host.HasDevices());
        Assert.True(host.UsesKernel(kernel));
        Assert.False(host.UsesKernel(new Kernel(7u)));
        host.BindDevices(kernel, clock, poll);
        Assert.True(host.HasDevices());
        Assert.Throws<ArgumentException>(() => host.BindDevices(new Kernel(7u), clock, poll));
        Assert.Throws<ArgumentException>(() => host.BindDevices(kernel, new FrameTimeBox(), poll));
        Assert.Throws<ArgumentNullException>(() => host.BindDevices(null!, clock, poll));
        Assert.Throws<ArgumentNullException>(() => host.BindDevices(kernel, null!, poll));
    }

    /// <summary>묶은 시계·타입은 상태가 같은 주소를 직접 읽고 쓴다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundDevicesForwardToState()
    {
        RawSurfaceStorage storage = FullStorage();
        RawProcessHostStorage host = storage.Processes();
        var kernel = new Kernel(7u);
        var clock = new FrameTimeBox { Value = new FrameTime { Game = 10.0 } };
        var poll = new UIntBox { Value = 122u };
        host.BindDevices(kernel, clock, poll);
        SquidProcessState state = host.State();
        Assert.Equal(10.0, state.Now);
        clock.Value = new FrameTime { Game = 20.0 };
        Assert.Equal(20.0, state.Now);
        state.Now = 30.0;
        Assert.Equal(30.0, clock.Value.Game);
        Assert.Equal(122u, state.PollType);
        poll.Value = 133u;
        Assert.Equal(133u, state.PollType);
    }

    /// <summary>외부 장치가 없으면 판본 capacity의 독립 커널을 한 번만 만든다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void LazyKernelKeepsEditionCapacity()
    {
        RawSurfaceStorage storage = FullStorage();
        Assert.Same(storage.Processes().Kernel(), storage.Processes().Kernel());
        Assert.Equal(KernelShape.ProcessCapacity, storage.Processes().Kernel().Capacity());
        var cdPool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, OriginalEdition.Cd1072);
        var cdHost = new RawProcessHostStorage(cdPool, [], null);
        Assert.Equal(KernelShape.CdProcessCapacity, cdHost.Kernel().Capacity());
        Assert.True(cdHost.UsesKernel(cdHost.Kernel()));
    }

    /// <summary>같은 188개 타입으로 대여 장면을 만든다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    private static (RawSidPool Pool, RawProcessHostStorage Storage, ProcessHostRuntimeBindings Runtime) LeaseScene()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var storage = new RawProcessHostStorage(pool, types, new RawAuthorityState(), new UIntBox { Value = 122u }, new RawGameAuxState());
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), types);
        var runtime = new ProcessHostRuntimeBindings { Destroy = destroy };
        return (pool, storage, runtime);
    }

    /// <summary>첫 사용에 안정된 Host를 만들고 같은 주소를 유지한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    [Fact]
    public void HostIsStableAndWired()
    {
        (RawSidPool pool, RawProcessHostStorage storage, ProcessHostRuntimeBindings _) = LeaseScene();
        Assert.Same(storage.Host(), storage.Host());
        Assert.Same(pool, storage.Host().Pool());
        Assert.True(storage.Host().HasOnlyRegularProcesses());
    }

    /// <summary>중복·다른 풀·빈 묶음은 현재 바인딩을 바꾸기 전에 거부한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    [Fact]
    public void BindGuardsSingleLease()
    {
        (RawSidPool pool, RawProcessHostStorage storage, ProcessHostRuntimeBindings runtime) = LeaseScene();
        Assert.Throws<ArgumentException>(() => storage.Bind(null!));
        Assert.Throws<ArgumentException>(() => storage.Bind(new ProcessHostRuntimeBindings()));
        var foreignPool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var foreignTypes = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var foreignDestroy = new RawSquidDestroy(foreignPool, new RawUnpop(foreignPool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]), foreignTypes);
        Assert.Throws<ArgumentException>(() => storage.Bind(new ProcessHostRuntimeBindings { Destroy = foreignDestroy }));
        runtime.Factory = new RawSquidFactory(foreignPool, foreignTypes);
        Assert.Throws<ArgumentException>(() => storage.Bind(runtime));
        runtime.Factory = null;
        using (RawProcessHostStorage.Use first = storage.Bind(runtime))
        {
            Assert.Throws<ArgumentException>(() => storage.Bind(runtime));
            Assert.Throws<ArgumentException>(() => storage.BindLegacyAllocationReclaim(_ => { }));
        }
    }

    /// <summary>같은 대여의 증빙과 현재 잔류 종류를 확인하고 실제 바인딩을 반납한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    [Fact]
    public void ClearAndReturnPassProof()
    {
        (RawSidPool _, RawProcessHostStorage storage, ProcessHostRuntimeBindings runtime) = LeaseScene();
        using (RawProcessHostStorage.Use lease = storage.Bind(runtime))
        {
            RawProcessHostStorage.ClearPass pass = lease.ClearForState();
            RawProcessHostStorage.Returned returned = lease.ReturnForState(pass);
            Assert.Same(storage.Kernel(), returned.Kernel);
            Assert.Throws<ArgumentException>(() => lease.ReturnForState(pass));
        }

        using (RawProcessHostStorage.Use next = storage.Bind(runtime))
        {
            Assert.NotNull(next);
        }
    }

    /// <summary>대여 없는 Clear와 진행 중 호출·비정기 잔류의 반납을 거부한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    [Fact]
    public void ReturnGuardsProofAndResiduals()
    {
        (RawSidPool _, RawProcessHostStorage storage, ProcessHostRuntimeBindings runtime) = LeaseScene();
        var stray = new RawProcessHostStorage.Use(storage, runtime);
        stray.Dispose();
        Assert.Throws<InvalidOperationException>(() => stray.ClearForState());
        using (RawProcessHostStorage.Use lease = storage.Bind(runtime))
        {
            RawProcessHostStorage.ClearPass pass = lease.ClearForState();
            using (new RuntimeCallGuard(runtime))
            {
                Assert.Throws<InvalidOperationException>(() => lease.ReturnForState(pass));
            }

            var plain = new SquidProcess();
            plain.HostLink = storage.Host();
            storage.Kernel().Add(plain);
            Assert.Throws<InvalidOperationException>(() => lease.ReturnForState(pass));
            storage.Kernel().Remove(1u);
            RawProcessHostStorage.Returned returned = lease.ReturnForState(pass);
            Assert.Same(storage.Kernel(), returned.Kernel);
        }
    }

    /// <summary>raw 외부 커널의 현재 월드만 회수 callable을 소유한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    [Fact]
    public void LegacyReclaimBindsSingleWorld()
    {
        (RawSidPool _, RawProcessHostStorage storage, ProcessHostRuntimeBindings _) = LeaseScene();
        Assert.Throws<ArgumentNullException>(() => storage.BindLegacyAllocationReclaim(null!));
        int seen = -1;
        Action<int> reclaim = storage.BindLegacyAllocationReclaim(sid => seen = sid);
        Assert.Throws<ArgumentException>(() => storage.BindLegacyAllocationReclaim(_ => { }));
        reclaim(0);
        storage.ReclaimForAllocation(9);
        Assert.Equal(9, seen);
    }

    /// <summary>현재 가상 삭제가 없으면 소진 복구를 명시 실패한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    [Fact]
    public void ReclaimRequiresCurrentDestroy()
    {
        (RawSidPool _, RawProcessHostStorage storage, ProcessHostRuntimeBindings runtime) = LeaseScene();
        Assert.Throws<InvalidOperationException>(() => storage.ReclaimForAllocation(5));
        string seen = string.Empty;
        runtime.Asset = new SquidDestroyHooks
        {
            Emit = @event =>
            {
                Assert.Equal(SquidDestroyEffect.DestroyDependent, @event.Effect);
                Assert.Equal(5, @event.Sid);
                Assert.Equal(1ul, runtime.ActiveCalls);
                seen = "emitted";
            },
        };
        using (RawProcessHostStorage.Use lease = storage.Bind(runtime))
        {
            storage.ReclaimForAllocation(5);
            Assert.Equal("emitted", seen);
        }

        Assert.Equal(0ul, runtime.ActiveCalls);
    }

    /// <summary>생성 실패·일반 종료에서 자기 대여가 만든 객체만 반납한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    [Fact]
    public void UseDrainReturnsOwnedOnly()
    {
        (RawSidPool _, RawProcessHostStorage storage, ProcessHostRuntimeBindings runtime) = LeaseScene();
        var use = storage.Bind(runtime);
        var owned = new SquidProcess();
        owned.HostLink = storage.Host();
        owned.RuntimeLink = runtime;
        storage.Kernel().Add(owned);
        var foreign = new SquidProcess();
        storage.Kernel().Add(foreign);
        use.DrainOwnedForShutdown();
        Assert.Null(storage.Kernel().Get(1u));
        Assert.NotNull(storage.Kernel().Get(2u));
        use.Dispose();
        using (RawProcessHostStorage.Use next = storage.Bind(runtime))
        {
            Assert.NotNull(next);
        }
    }
}
