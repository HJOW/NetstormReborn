using System.Runtime.CompilerServices;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 장기 음향이 이전 미션을 붙잡지 않도록 실제 Host의 State 반납 관문을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>음소거 실제 자산에서 현재 미션 반납·배송 관문·늦은 반납과 장기 장치 보존을 검증한다.</summary>
public sealed class HostMissionAudioLifetimeTests
{
    /// <summary>원본 TYPE/SHP와 실제 Kernel·Host·Runtime을 공유하는 창 없는 검사 장면이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>같은 실제 Player·Kernel·장기 Host 대여를 제공하는 Client 표면이다.</summary>
        public ClientSurfaceGlobals Surface { get; }
        /// <summary>소리 프로세스가 빌릴 실제 세션이다.</summary>
        public HostSession Session { get; }
        /// <summary>배송 중 반납 거부에 사용하는 같은 실제 외곽 owner다.</summary>
        public HostMissionRuntime Runtime { get; }
        /// <summary>실제 자산을 설치하고 세션을 공개하되 게임 프레임이나 창은 만들지 않는다.</summary>
        public Scene()
        {
            var game = new ClientGameplayGlobals();
            var local = new ClientLocalStateGlobals(game, new());
            var clock = new ClientClockGlobals(local);
            clock.InitializeFullTime(1000);
            Surface = new(game, clock, new Kernel(), () => 1000);
            Session = new(Surface.Kernel, new GameRandom(17), new CrtRandom(23), sharedGameplay: game,
                sharedLocal: local, sharedPlayerRecords: game.PlayerRecords, pool: Surface.Storage.Pool,
                sharedLists: Surface.Storage.SidLists, sharedProcessTime: clock.GameTime, sharedPlayerProcesses: Surface.Processes);
            _ = HostPostPopRuntimeTests.Build(Session, Surface.Storage);
            Runtime = HostMissionRuntime.Create(Session, Surface.Storage);
        }
        /// <summary>반복 Drain 없이 실제 단일 Kernel Clear의 정상 복귀 증빙을 처리기 반납에 전달한다.</summary>
        public void ReturnProcesses() => Session.ReturnProcessBindingsForState(Session.ClearProcessesForState());
        /// <summary>검사 장면이 가진 실제 Player 소비자와 외곽 대여만 순서대로 반납한다.</summary>
        public void Dispose() { Session.ReleasePlayerRecordViews(); Runtime.Dispose(); }
    }

    /// <summary>외부 음향 장치를 열지 않는 실제 자산·장기 음악 상태를 준비한다.</summary>
    private static (HostStartupScene Startup, HostMissionAudio Audio) MutedAudio()
    {
        Assert.True(OperatingSystem.IsWindows(), "미션 음향 장치는 Windows 전용이다");
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        return (startup, new HostMissionAudio(startup, 0, true, () => 1));
    }

    /// <summary>Kernel Clear 전과 실제 Runtime 배송 중의 반납 실패는 현재 소리 공장 연결을 보존한다.</summary>
    [Fact]
    public void RequiresActualProcessReturnAndIdleRuntimeBeforeUnbinding()
    {
        var (startup, audio) = MutedAudio();
        using (audio)
        using (var scene = new Scene())
        {
            var definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
            _ = audio.BindPriest(scene.Session);
            Assert.Throws<InvalidOperationException>(() => audio.UnbindMissionForState(scene.Session));
            Assert.NotNull(audio.ConstructionHooks(scene.Session, definitions).AttachSound);
            var pass = scene.Session.ClearProcessesForState();
            Assert.Throws<InvalidOperationException>(() => audio.UnbindMissionForState(scene.Session));
            scene.Session.ReturnProcessBindingsForState(pass);
            using (scene.Session.PinRuntime())
            {
                Assert.Throws<InvalidOperationException>(() => audio.UnbindMissionForState(scene.Session));
                Assert.NotNull(audio.ConstructionHooks(scene.Session, definitions).AttachSound);
            }
            Assert.True(audio.UnbindMissionForState(scene.Session));
            Assert.False(audio.UnbindMissionForState(scene.Session));
            Assert.Throws<InvalidOperationException>(() => audio.ConstructionHooks(scene.Session, definitions));
            audio.SetFrame(91);
        }
    }

    /// <summary>다른 미션과 늦은 이전 미션의 반납은 다음 실제 소리 공장을 끊지 않는다.</summary>
    [Fact]
    public void ForeignAndLateOldReturnsPreserveCurrentMission()
    {
        var (startup, audio) = MutedAudio();
        using (audio)
        using (var previous = new Scene())
        using (var current = new Scene())
        {
            var definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
            Assert.Throws<ArgumentNullException>(() => audio.UnbindMissionForState(null!));
            _ = audio.BindPriest(previous.Session);
            Assert.False(audio.UnbindMissionForState(current.Session));
            Assert.NotNull(audio.ConstructionHooks(previous.Session, definitions).AttachSound);
            previous.ReturnProcesses();
            Assert.True(audio.UnbindMissionForState(previous.Session));
            _ = audio.BindPriest(current.Session);
            using (current.Session.PinRuntime()) Assert.False(audio.UnbindMissionForState(previous.Session));
            Assert.NotNull(audio.ConstructionHooks(current.Session, definitions).AttachSound);
            current.ReturnProcesses();
            Assert.True(audio.UnbindMissionForState(current.Session));
            audio.StopLoops();
            Assert.Equal(0u, audio.PlayAmbient("whale.wav", 1, 0, 0, 0, 0));
        }
    }

    /// <summary>같은 장기 음향만 남긴 뒤 실제 세션의 수집 가능성을 관찰하도록 지역 수명을 분리한다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference ReleaseCurrentMission(HostMissionAudio audio)
    {
        using var scene = new Scene();
        _ = audio.BindPriest(scene.Session);
        var weak = new WeakReference(scene.Session);
        scene.ReturnProcesses();
        Assert.True(audio.UnbindMissionForState(scene.Session));
        return weak;
    }

    /// <summary>직접 세션뿐 아니라 프로세스 상태의 진단 callback 등 간접 강한 참조도 장기 음향에 남지 않는다.</summary>
    [Fact]
    public void UnboundLongLivedAudioDoesNotRetainPreviousSession()
    {
        var (_, audio) = MutedAudio();
        using (audio)
        {
            var music = audio.Scene;
            music.Index = 2;
            WeakReference session = ReleaseCurrentMission(audio);
            // 종료한 지역의 실제 세션이 간접 소리 상태/공장을 통해 붙잡히는지 강제 수집으로 관찰한다.
            for (int attempt = 0; attempt < 3 && session.IsAlive; attempt++)
            {
                GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect();
            }
            Assert.False(session.IsAlive);
            Assert.Same(music, audio.Scene);
            Assert.Equal(2, audio.Scene.Index);
            audio.SetFrame(92);
            audio.StopLoops();
            Assert.Equal(0u, audio.PlayAmbient("whale.wav", 1, 0, 0, 0, 0));
        }
    }
}
