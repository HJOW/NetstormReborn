using System.Runtime.CompilerServices;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 HostSession/저장소/장치 전체와 weak 공개 및 콜백 정리 관문을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>준비 대여는 게임 값을 보존하고 실제 owner pin은 배송 동안만 현재 정리를 막는다.</summary>
public sealed class HostMissionRuntimeTests
{
    /// <summary>실제 같은 풀을 빌리는 제품 세션과 물리를 구성한다.</summary>
    private static (HostSession Session, RawSurfaceStorage Storage) Scene(ClientGameplayGlobals? gameplay = null, ClientLocalStateGlobals? local = null, ClientConnectionGlobals? connections = null)
    {
        var storage = new RawSurfaceStorage(); var session = new HostSession(new Kernel(), new GameRandom(73), new CrtRandom(), sharedGameplay: gameplay, pool: storage.Pool, sharedLocal: local, sharedConnections: connections); return (session, storage);
    }

    /// <summary>준비 대여/공개/반납이 같은 현재 Player/옵션/모드/시계를 재초기화하지 않는다.</summary>
    [Fact]
    public void PreparedPublicationPreservesCurrentGameValuesAndRejectsDuplicates()
    {
        var (session, storage) = Scene(); ClientGameplayGlobals game = session.Gameplay; game.Authority.Server = false; game.Authority.Boss = false; game.FullDeckBox.Value = 17; game.SavedGameBox.Value = true; session.MissionPlayer.PlayerKnown = 9; session.LocalState.GameTicks.Value = 0x12345678;
        using (HostMissionRuntime runtime = HostMissionRuntime.Create(session, storage))
        {
            Assert.True(game.Borrowed()); Assert.Same(session, game.RequireSession()); Assert.Same(storage, runtime.Storage); Assert.Throws<InvalidOperationException>(() => game.AcquirePrepared());
            Assert.False(game.Authority.Server); Assert.False(game.Authority.Boss); Assert.Equal(17u, game.FullDeckBox.Value); Assert.True(game.SavedGameBox.Value); Assert.Equal(9u, session.MissionPlayer.PlayerKnown); Assert.Equal(0x12345678u, session.LocalState.GameTicks.Value);
        }
        Assert.False(game.Borrowed()); Assert.Throws<InvalidOperationException>(game.RequireSession); Assert.Equal(9u, session.MissionPlayer.PlayerKnown); Assert.True(game.SavedGameBox.Value); Assert.Equal(0x12345678u, session.LocalState.GameTicks.Value);
    }

    /// <summary>미공개/다른실제Pool과다른Session을성공owner로대신하지않으며실패는기존대여를보존한다.</summary>
    [Fact]
    public void MissingAndMismatchedOwnersFailWithoutChangingCurrentLease()
    {
        var (session, storage) = Scene(); Assert.Throws<InvalidOperationException>(session.PinRuntime); Assert.Throws<ArgumentException>(() => HostMissionRuntime.Create(session, new RawSurfaceStorage())); Assert.False(session.Gameplay.Borrowed());
        using HostMissionRuntime runtime = HostMissionRuntime.Create(session, storage); var (other, _) = Scene(); Assert.Throws<ArgumentException>(() => session.Gameplay.BindRuntimeOwner(other, runtime)); Assert.Same(session, session.Gameplay.RequireSession()); Assert.False(runtime.Released);
        Assert.Throws<InvalidOperationException>(() => HostMissionRuntime.Create(session, storage)); Assert.Same(session, session.Gameplay.RequireSession());
    }

    /// <summary>명시 준비 대여의공개/반납은현재정체성만바꾸고같은물리의값을보존한다.</summary>
    [Fact]
    public void PreparedUseSeparatesPublishWithdrawAndActualOwnerBinding()
    {
        var (session, _) = Scene(); var (other, _) = Scene(); ClientGameplayGlobals game = session.Gameplay; using var use = game.AcquirePrepared(); use.Publish(session); Assert.Same(session, game.RequireSession());
        Assert.Throws<InvalidOperationException>(() => game.PinCurrentRuntime(session)); Assert.Throws<InvalidOperationException>(() => use.Publish(session)); Assert.Throws<InvalidOperationException>(() => use.Withdraw(other)); use.Withdraw(session); Assert.Throws<InvalidOperationException>(game.RequireSession); use.Publish(session); use.Dispose(); Assert.False(game.Borrowed());
    }

    /// <summary>중첩pin은실제최후복귀까지owner를보유하고실행중전체월드반납을거부한다.</summary>
    [Fact]
    public void NestedCallsDeferActualOwnerReleaseAndProtectWorldCleanup()
    {
        var (session, storage) = Scene(); HostMissionRuntime runtime = HostMissionRuntime.Create(session, storage); IDisposable first = session.Gameplay.PinCurrentRuntime(session), second = session.PinRuntime(); Assert.True(session.Gameplay.RuntimeCallActive(session));
        Assert.Throws<InvalidOperationException>(session.ReleaseMissionAiBrains); Assert.Throws<InvalidOperationException>(session.ReleasePlayerProcesses); Assert.Throws<InvalidOperationException>(session.ReleaseConstructionWorld); Assert.Throws<InvalidOperationException>(session.ReleaseBurningWorld); Assert.Throws<InvalidOperationException>(() => session.Gameplay.BindRuntimeOwner(session, runtime));
        runtime.Dispose(); Assert.False(runtime.Released); Assert.Same(session, session.Gameplay.RequireSession()); first.Dispose(); first.Dispose(); Assert.False(runtime.Released); Assert.True(session.Gameplay.RuntimeCallActive(session)); second.Dispose(); Assert.True(runtime.Released); Assert.False(session.Gameplay.RuntimeCallActive(session)); Assert.False(session.Gameplay.Borrowed()); Assert.Throws<InvalidOperationException>(session.PinRuntime);
    }

    /// <summary>하위실패의finally에서실제호출수를반납하되외부owner가살아있으면현재공개를유지한다.</summary>
    [Fact]
    public void FailureUnwindsCallBeforeNormalOwnerCleanup()
    {
        var (session, storage) = Scene(); using HostMissionRuntime runtime = HostMissionRuntime.Create(session, storage); var expected = new InvalidOperationException("실제 하위 실패");
        Assert.Same(expected, Assert.Throws<InvalidOperationException>((Action)(() => { using IDisposable pin = session.PinRuntime(); Assert.True(session.Gameplay.RuntimeCallActive(session)); throw expected; })));
        Assert.False(session.Gameplay.RuntimeCallActive(session)); Assert.Same(session, session.Gameplay.RequireSession()); Assert.False(runtime.Released); session.ReleaseMissionAiBrains(); session.ReleasePlayerProcesses();
    }

    /// <summary>이전Session의저장된pin장치는현재Session이교체되면새owner로몰래재선택하지않는다.</summary>
    [Fact]
    public void OldSessionPinCannotRetargetNewActualSession()
    {
        var (first, firstStorage) = Scene(); HostMissionRuntime old = HostMissionRuntime.Create(first, firstStorage); old.Dispose(); var (second, secondStorage) = Scene(first.Gameplay, first.LocalState, first.Connections); using HostMissionRuntime current = HostMissionRuntime.Create(second, secondStorage);
        Assert.Throws<InvalidOperationException>(first.PinRuntime); using IDisposable pin = second.PinRuntime(); Assert.Same(second, first.Gameplay.RequireSession()); Assert.True(second.Gameplay.RuntimeCallActive(second)); Assert.False(first.Gameplay.RuntimeCallActive(first));
    }

    /// <summary>테스트호출스택이외부owner를불필요하게유지하지않도록생성과공개를분리한다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference<HostMissionRuntime> WeakOwner(HostSession session, RawSurfaceStorage storage)
    {
        HostMissionRuntime runtime = HostMissionRuntime.Create(session, storage); return new(runtime);
    }

    /// <summary>weak공개와세션의pin함수는실제외곽owner를강하게소유하지않는다.</summary>
    [Fact]
    public void WeakPublicationDoesNotPretendExpiredOwnerIsAlive()
    {
        var (session, storage) = Scene(); WeakReference<HostMissionRuntime> weak = WeakOwner(session, storage); GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); Assert.False(weak.TryGetTarget(out _)); Assert.Same(session, session.Gameplay.RequireSession()); Assert.Throws<InvalidOperationException>(session.PinRuntime);
    }

    /// <summary>외부owner를반납한뒤실제pin만전체세션/물리/외부장치를보유하는입력이다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (IDisposable Pin, WeakReference<HostMissionRuntime> Runtime, WeakReference<HostSession> Session, WeakReference<RawSurfaceStorage> Storage, WeakReference<MemoryStream> Device) OnlyDelivery(ClientGameplayGlobals game)
    {
        var (session, storage) = Scene(game); var device = new MemoryStream(new byte[] { 1, 2, 3 }); HostMissionRuntime runtime = HostMissionRuntime.Create(session, storage, device); IDisposable pin = session.PinRuntime(); runtime.Dispose(); return (pin, new(runtime), new(session), new(storage), new(device));
    }

    /// <summary>실제배송은외부참조가사라져도전체owner와실제장치를마지막복귀까지유지한다.</summary>
    [Fact]
    public void DeliveryPinsWholeSessionStorageAndActualDeviceAcrossCollection()
    {
        var game = new ClientGameplayGlobals(); var value = OnlyDelivery(game); GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); Assert.True(value.Runtime.TryGetTarget(out _)); Assert.True(value.Session.TryGetTarget(out _)); Assert.True(value.Storage.TryGetTarget(out _)); Assert.True(value.Device.TryGetTarget(out _)); Assert.True(game.Borrowed()); value.Pin.Dispose(); Assert.False(game.Borrowed()); Assert.Throws<InvalidOperationException>(game.RequireSession);
    }
}
