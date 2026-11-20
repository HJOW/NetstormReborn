using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 장기 Host가 현재 Runtime만 선택하고 자기 프로세스만 반납하는지 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>같은 Kernel/Frame/Pool에서 세션 대여를 바꾸는 실제 결선을 검증한다.</summary>
public sealed class HostLongProcessStorageTests
{
    /// <summary>특정 Runtime이 소유하지 않는 장기 커널 슬롯이다.</summary>
    private sealed class ForeignProcess : BaseProcess
    {
        /// <summary>외부 슬롯의 생존만 검사하며 게임값은 변경하지 않는다.</summary>
        public override void RunFrame() { }
    }

    /// <summary>같은 Client의 장기 물리/시계 장치를 준비한다.</summary>
    private static ClientSurfaceGlobals Owner()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var clock = new ClientClockGlobals(local); clock.InitializeFullTime(1000);
        return new ClientSurfaceGlobals(game, clock, new Kernel(), () => 1000);
    }

    /// <summary>같은 장기 Pool/TYPE/Kernel/Frame을 빌리고 실제 자산 부모를 조립한다.</summary>
    private static HostSession Session(ClientSurfaceGlobals owner)
    {
        var session = new HostSession(owner.Kernel, new GameRandom(17), new CrtRandom(23), pool: owner.Storage.Pool,
            sharedGameplay: owner.Gameplay, sharedProcessTime: owner.Clock.GameTime, sharedPlayerProcesses: owner.Processes);
        _ = HostPostPopRuntimeTests.Build(session, owner.Storage); return session;
    }

    /// <summary>실제 Player/공장 경로로 부모를 생성하고 같은 장기 Host에 Regular를 붙인다.</summary>
    private static RegularProcess Attach(HostSession session)
    {
        int sid = session.BuildPlayerWorldHooks().Create!(152, 2); session.Pool.AllocatedBytes(sid)[RawSidPool.StateOffset] &= unchecked((byte)~RawSidPool.VoidBit);
        var process = new RegularProcess(7, 0.25f); Assert.Same(process, session.PlayerProcesses.AttachRegular(process, sid, false)); return process;
    }

    /// <summary>외곽 공개 전후 모두 같은 장기 Host/현재 Runtime이 캡처된 실제 게임시각을 읽는다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void SharedHostBindsCurrentRuntimeBeforeOrAfterPublication(bool publishFirst)
    {
        var owner = Owner(); var session = Session(owner); HostMissionRuntime? lifetime = publishFirst ? HostMissionRuntime.Create(session, owner.Storage, []) : null;
        _ = session.BuildPlayerWorldHooks(); lifetime ??= HostMissionRuntime.Create(session, owner.Storage, []);
        Assert.Same(owner.Processes.Host(), session.PlayerProcesses); ProcessHostRuntimeBindings runtime = session.PlayerProcesses.Runtime();
        int calls = 0; session.PlayerProcesses.SetRegularHandler((_, _, _, _) =>
        {
            calls++; Assert.Equal(0.025, session.ProcessTime.Value); Assert.Equal(0.025, owner.Processes.State().SharedNow!.Value);
            Assert.True(runtime.ActiveCalls > 0); Assert.True(owner.Gameplay.RuntimeCallActive(session));
            Assert.Throws<InvalidOperationException>(() => session.ReleasePlayerRecordViews()); return 0.25f;
        });
        _ = Attach(session); _ = owner.Clock.CaptureFrame(1025); session.RunMissionFrame(); Assert.Equal(1, calls); Assert.Equal(0ul, runtime.ActiveCalls);
        lifetime.Dispose(); session.ReleasePlayerRecordViews(); Assert.Throws<InvalidOperationException>(() => owner.Processes.Host().Runtime());
    }

    /// <summary>현재 대여의 정리는 외부 커널 슬롯을 보존하고 다음 세션도 같은 Host 주소를 재사용한다.</summary>
    [Fact]
    public void ReborrowKeepsSameHostAndPreservesForeignKernelProcess()
    {
        var owner = Owner(); var foreign = new ForeignProcess(); owner.Kernel.Add(foreign);
        var first = Session(owner); _ = first.BuildPlayerWorldHooks(); SquidProcessHost host = first.PlayerProcesses; ProcessHostRuntimeBindings previous = host.Runtime(); _ = Attach(first);
        first.ReleasePlayerRecordViews(); Assert.Same(foreign, owner.Kernel.Get(1)); Assert.Equal(1, owner.Kernel.Size()); Assert.Throws<InvalidOperationException>(() => host.Runtime());
        var second = Session(owner); _ = second.BuildPlayerWorldHooks(); Assert.Same(host, second.PlayerProcesses); Assert.NotSame(previous, host.Runtime());
        int calls = 0; second.PlayerProcesses.SetRegularHandler((_, _, _, _) => { calls++; return 0.25f; }); _ = Attach(second); _ = owner.Clock.CaptureFrame(1025); second.RunMissionFrame(); Assert.Equal(1, calls);
        second.ReleasePlayerRecordViews(); Assert.Same(foreign, owner.Kernel.Get(1)); Assert.Equal(1, owner.Kernel.Size()); Assert.Throws<InvalidOperationException>(() => host.Runtime()); GC.KeepAlive(previous);
    }

    /// <summary>실제 Regular 예외가 외곽 Runtime 반납을 마지막 복귀까지 미루고 장기 Host의 현재 선택도 끝에 해제한다.</summary>
    [Fact]
    public void CallbackFailureReturnsWholeRuntimeBeforeFinalHostUse()
    {
        var owner = Owner(); var session = Session(owner); _ = session.BuildPlayerWorldHooks(); var lifetime = HostMissionRuntime.Create(session, owner.Storage, []);
        ProcessHostRuntimeBindings binding = session.PlayerProcesses.Runtime(); var failure = new IOException("장기Host정기실패");
        session.PlayerProcesses.SetRegularHandler((_, _, _, _) => { lifetime.Dispose(); Assert.False(lifetime.Released); Assert.Throws<InvalidOperationException>(() => session.ReleasePlayerProcesses()); throw failure; });
        _ = Attach(session); _ = owner.Clock.CaptureFrame(1025); Assert.Same(failure, Assert.Throws<IOException>(() => session.RunMissionFrame()));
        Assert.Equal(0ul, binding.ActiveCalls); Assert.True(lifetime.Released); Assert.False(owner.Gameplay.Borrowed()); session.ReleasePlayerRecordViews(); Assert.Throws<InvalidOperationException>(() => owner.Processes.Host().Runtime());
    }

    /// <summary>다른 Kernel을 가진 장기 Host는 미션 장치나 Player 고정뷰를 구성하기 전에 거부한다.</summary>
    [Fact]
    public void RejectsDifferentKernelBeforeMissionInitialization()
    {
        var owner = Owner(); Assert.Throws<ArgumentException>(() => new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), pool: owner.Storage.Pool,
            sharedGameplay: owner.Gameplay, sharedProcessTime: owner.Clock.GameTime, sharedPlayerProcesses: owner.Processes));
        Assert.Equal(0, owner.Kernel.Size()); Assert.False(owner.Gameplay.Borrowed());
    }
}
