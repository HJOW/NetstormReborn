using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제Host의공장/Form/Kernel/Regular를현재대여와외곽수명에연결해검사한다.
namespace Netstorm.Client.Tests;

/// <summary>직접Host호출과실제Kernel프레임의같은Runtime선택/반납을검증한다.</summary>
public sealed class HostProcessRuntimeTests
{
    /// <summary>같은실제Pool/Host와실자산부모를준비한다.</summary>
    private static (RawSurfaceStorage Storage, HostSession Session) Scene()
    {
        var storage = new RawSurfaceStorage(); var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), pool: storage.Pool);
        _ = HostPostPopRuntimeTests.Build(session); return (storage, session);
    }
    /// <summary>같은실제화신생성자에배치상태입력을공급하고원문Regular/Form/Kernel부착을실행한다.</summary>
    private static RegularProcess Attach(HostSession session)
    {
        int parent = session.BuildPlayerWorldHooks().Create!(152, 2); session.Pool.AllocatedBytes(parent)[RawSidPool.StateOffset] &= unchecked((byte)~RawSidPool.VoidBit);
        var process = new RegularProcess(7, 0.25f);
        Assert.Same(process, session.PlayerProcesses.AttachRegular(process, parent, false)); return process;
    }

    /// <summary>공개전후에조립한실제Host가같은본문/공장을쓰며Kernel콜백끝까지외곽owner를보유한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void ActualKernelUsesCurrentRuntimeRegardlessOfPublicationOrder(bool publishFirst)
    {
        var storage = new RawSurfaceStorage(); var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), pool: storage.Pool);
        HostMissionRuntime? owner = publishFirst ? HostMissionRuntime.Create(session, storage, []) : null;
        _ = HostPostPopRuntimeTests.Build(session); _ = session.BuildPlayerWorldHooks(); owner ??= HostMissionRuntime.Create(session, storage, []);
        ProcessHostRuntimeBindings runtime = session.PlayerProcesses.Runtime(); Assert.Same(session.Pool, runtime.Destroy!.Pool); Assert.Same(session.Pool, runtime.Factory!.Pool); Assert.Equal(session.PostPopDepth, runtime.PostPopDepth!());
        int calls = 0; session.PlayerProcesses.SetRegularHandler((_, ev, count, payload) =>
        {
            calls++; Assert.Equal(7u, ev); Assert.Equal(0u, count); Assert.Equal(0.25f, payload); Assert.True(runtime.ActiveCalls > 0); Assert.True(session.Gameplay.RuntimeCallActive(session));
            Assert.Throws<InvalidOperationException>(() => session.ReleasePlayerProcesses()); Assert.Throws<InvalidOperationException>(() => session.ReleasePlayerRecordViews()); return 0.25f;
        });
        RegularProcess attached = Attach(session); Assert.True(session.PlayerProcesses.Owns(attached)); session.AdvanceMissionFrame(0.1); Assert.Equal(1, calls); Assert.Equal(0ul, runtime.ActiveCalls); Assert.False(session.Gameplay.RuntimeCallActive(session));
        owner.Dispose(); session.ReleasePlayerRecordViews(); Assert.Throws<InvalidOperationException>(() => session.PlayerProcesses.Runtime()); GC.KeepAlive(runtime);
    }

    /// <summary>공개전직접호출도실제묶음의실행수를유지해앞선정리효과없이반납을거부한다.</summary>
    [Fact]
    public void PreparedKernelCallRejectsReleaseWithoutOuterPublication()
    {
        var (_, session) = Scene(); _ = session.BuildPlayerWorldHooks(); ProcessHostRuntimeBindings runtime = session.PlayerProcesses.Runtime();
        session.PlayerProcesses.SetRegularHandler((_, _, _, _) =>
        {
            Assert.True(runtime.ActiveCalls > 0); Assert.False(session.Gameplay.RuntimeCallActive(session)); Assert.False(session.Gameplay.Borrowed());
            Assert.Throws<InvalidOperationException>(() => session.ReleasePlayerRecordViews()); return 0.5f;
        });
        _ = Attach(session); session.AdvanceMissionFrame(0.1); Assert.Equal(0ul, runtime.ActiveCalls); session.ReleasePlayerRecordViews(); Assert.Equal(0, session.Kernel.Size()); Assert.Throws<InvalidOperationException>(() => session.PlayerProcesses.Runtime());
    }

    /// <summary>실제Regular효과예외가외곽종료를마지막복귀까지미루고현재호출수를누락하지않는다.</summary>
    [Fact]
    public void ActualRegularFailureReturnsRuntimeAndAllowsChildCleanup()
    {
        var (storage, session) = Scene(); _ = session.BuildPlayerWorldHooks(); var owner = HostMissionRuntime.Create(session, storage, []); ProcessHostRuntimeBindings runtime = session.PlayerProcesses.Runtime(); var failure = new IOException("실제Regular효과실패");
        session.PlayerProcesses.SetRegularHandler((_, _, _, _) => { Assert.True(session.Gameplay.RuntimeCallActive(session)); owner.Dispose(); Assert.False(owner.Released); throw failure; });
        _ = Attach(session); Assert.Same(failure, Assert.Throws<IOException>(() => session.AdvanceMissionFrame(0.1))); Assert.Equal(0ul, runtime.ActiveCalls); Assert.True(owner.Released); Assert.False(session.Gameplay.Borrowed());
        session.ReleasePlayerRecordViews(); Assert.Equal(0, session.Kernel.Size()); Assert.Throws<InvalidOperationException>(() => session.PlayerProcesses.Runtime()); GC.KeepAlive(runtime);
    }
}
