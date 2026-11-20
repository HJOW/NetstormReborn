using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제전체HostRuntime의짧은대여를Core호출guard가끝까지유지하고결정적으로반납하는지확인한다.
namespace Netstorm.Client.Tests;

/// <summary>실제Pool/Session/Gameplay대여를marker소비자에연결한다.</summary>
public sealed class HostRuntimeScopedLeaseTests
{
    /// <summary>같은실제저장소를빌리는Session과장치묶음을함께소유하는Runtime이다.</summary>
    private static HostMissionRuntime Runtime()
    {
        var storage = new RawSurfaceStorage(); var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), pool: storage.Pool);
        return HostMissionRuntime.Create(session, storage, [new MemoryStream()]);
    }
    /// <summary>중첩된실제배송호출의마지막반납까지외부Runtime종료를보류한다.</summary>
    [Fact]
    public void ActualRuntimeWaitsUntilLastProcessLeaseReturns()
    {
        var owner = Runtime(); var session = owner.Session; var runtime = new ProcessHostRuntimeBindings { Pin = () => session.PinRuntime() };
        var first = new RuntimeCallGuard(runtime); var second = new RuntimeCallGuard(runtime);
        Assert.True(session.Gameplay.RuntimeCallActive(session)); owner.Dispose(); Assert.False(owner.Released);
        second.Dispose(); Assert.False(owner.Released); Assert.True(session.Gameplay.RuntimeCallActive(session));
        first.Dispose(); Assert.True(owner.Released); Assert.False(session.Gameplay.RuntimeCallActive(session)); Assert.False(session.Gameplay.Borrowed());
        first.Dispose(); second.Dispose();
    }
    /// <summary>실제효과예외복귀가대여를누락하지않고이후새실제Runtime을다시공개할수있다.</summary>
    [Fact]
    public void ActualExceptionUnwindReturnsWholeRuntimeAndCurrentPublication()
    {
        var owner = Runtime(); var session = owner.Session; var runtime = new ProcessHostRuntimeBindings { Pin = () => session.PinRuntime() };
        Assert.Throws<IOException>((Action)(() => { using var call = new RuntimeCallGuard(runtime); owner.Dispose(); Assert.False(owner.Released); throw new IOException("실제배송효과실패"); }));
        Assert.True(owner.Released); Assert.False(session.Gameplay.Borrowed()); Assert.False(session.Gameplay.RuntimeCallActive(session));
        using var next = HostMissionRuntime.Create(session, owner.Storage, []); using var pin = session.PinRuntime(); Assert.IsAssignableFrom<IRawRuntimeLease>(pin);
    }
}
