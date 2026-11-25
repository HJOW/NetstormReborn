using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 미션 프레임의 월드·건설 효과가 현재 Runtime pin 안에서 실행되는지 검증한다.
// 2026-11-26 1.040에서 WP-4d World/건설 효과 pin 전달용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>월드 프레임 pin 전달과 미공개 폴백을 같은 세션에서 대조한다.</summary>
public sealed class HostWorldFramePinTests
{
    /// <summary>실제 같은 풀을 빌리는 제품 세션과 물리를 구성한다.</summary>
    private static (HostSession Session, RawSurfaceStorage Storage) Scene()
    {
        var storage = new RawSurfaceStorage();
        var session = new HostSession(new Kernel(), new GameRandom(73), new CrtRandom(), pool: storage.Pool, sharedGameplay: new ClientGameplayGlobals());
        return (session, storage);
    }

    /// <summary>미션 Runtime 공개 뒤 프레임 효과는 현재 pin 안에서 실행된다.</summary>
    [Fact]
    public void PinnedWorldFrameHoldsPinDuringEffects()
    {
        var (session, storage) = Scene();
        using var runtime = HostMissionRuntime.Create(session, storage);
        bool observed = false;
        bool active = false;
        session.RunPinnedWorldFrame(() =>
        {
            active = session.Gameplay.RuntimeCallActive(session);
            observed = true;
        });
        Assert.True(observed);
        Assert.True(active);
        Assert.False(session.Gameplay.RuntimeCallActive(session));
        Assert.False(runtime.Released);
        session.ReleasePlayerRecordViews();
    }

    /// <summary>미션 Runtime 미공개면 pin 없이 그대로 실행한다.</summary>
    [Fact]
    public void UnpinnedWorldFrameRunsWithoutRuntime()
    {
        var (session, _) = Scene();
        Assert.Throws<InvalidOperationException>(session.PinRuntime);
        bool observed = false;
        session.RunPinnedWorldFrame(() =>
        {
            observed = true;
            Assert.False(session.Gameplay.RuntimeCallActive(session));
        });
        Assert.True(observed);
        session.ReleasePlayerRecordViews();
    }

    /// <summary>효과 예외가 나도 pin은 반납된다.</summary>
    [Fact]
    public void PinnedWorldFrameReleasesPinOnFailure()
    {
        var (session, storage) = Scene();
        using var runtime = HostMissionRuntime.Create(session, storage);
        var failure = new InvalidOperationException("월드 효과 실패");
        Assert.Same(failure, Assert.Throws<InvalidOperationException>(() => session.RunPinnedWorldFrame(() => throw failure)));
        Assert.False(session.Gameplay.RuntimeCallActive(session));
        Assert.False(runtime.Released);
        session.ReleasePlayerRecordViews();
    }
}
