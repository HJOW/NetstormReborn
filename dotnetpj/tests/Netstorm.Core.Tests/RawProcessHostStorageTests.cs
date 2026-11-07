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
}
