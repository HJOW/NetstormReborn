using Netstorm.Client.Automation;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 실제Client프레임주소와Kernel/입력스크립트의동일시각을검사한다.
namespace Netstorm.Client.Tests;

/// <summary>프레임캡처순서/시간별칭/명시독립전진의서로다른계약을대조한다.</summary>
public sealed class ClientSharedFrameTests
{
    /// <summary>실제Kernel슬롯에서현재공유게임시각을관측한다.</summary>
    private sealed class Probe(Action run) : BaseProcess
    {
        /// <summary>원문Kernel의실제프레임호출에서관측효과를실행한다.</summary>
        public override void RunFrame() => run();
    }
    /// <summary>같은Client의실제보정/Clock/Frame소유자를구성한다.</summary>
    private static (ClientGameplayGlobals Game, ClientLocalStateGlobals Local, ClientClockGlobals Clock) Owner()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var clock = new ClientClockGlobals(local); clock.InitializeFullTime(1000); return (game, local, clock);
    }
    /// <summary>안정된게임시각상자는같은Frame주소를양방향으로빌리고새Capture는내부Clock결과를대입한다.</summary>
    [Fact]
    public void GameTimeBoxAliasesSameFrameWithoutCopy()
    {
        var (_, _, clock) = Owner(); DoubleBox alias = clock.GameTime; alias.Value = 17.25; Assert.Equal(17.25, clock.Frame.Value.Game);
        clock.Frame.Value.Game = 19.5; Assert.Equal(19.5, alias.Value); FrameTime first = clock.CaptureFrame(1025); Assert.Same(alias, clock.GameTime); Assert.Equal(0.025, alias.Value); Assert.Equal(0.025, first.Delta);
        alias.Value = 99; FrameTime second = clock.CaptureFrame(1050); Assert.Equal(0.05, alias.Value); Assert.Equal(0.025, second.Delta);
    }
    /// <summary>호스트생성은현재시간을덮지않고Kernel은캡처한같은주소를읽으며추가전진하지않는다.</summary>
    [Fact]
    public void SessionKernelConsumesCapturedClientTimeDirectly()
    {
        var (game, local, clock) = Owner(); _ = clock.CaptureFrame(1025); var kernel = new Kernel();
        var session = new HostSession(kernel, new GameRandom(17), new CrtRandom(23), sharedGameplay: game, sharedLocal: local, sharedProcessTime: clock.GameTime); var seen = new List<double>(); kernel.Add(new Probe(() => seen.Add(session.ProcessTime.Value)));
        Assert.Same(clock.GameTime, session.ProcessTime); Assert.Equal(0.025, clock.GameTime.Value); session.RunMissionFrame(); Assert.Equal(0.025, clock.GameTime.Value);
        local.GameTicks.Value = 10; _ = clock.CaptureFrame(1050); session.RunMissionFrame(); Assert.Equal(new[] { 0.025, 0.04 }, seen); Assert.Equal(0.04, session.ProcessTime.Value); session.ReleasePlayerRecordViews();
    }
    /// <summary>기존독립호출자는명시Advance로만시간을늘리고Run은같은현재시간을유지한다.</summary>
    [Fact]
    public void IndependentSessionRetainsExplicitAdvanceContract()
    {
        var kernel = new Kernel(); var session = new HostSession(kernel, new GameRandom(17), new CrtRandom(23)); var seen = new List<double>(); kernel.Add(new Probe(() => seen.Add(session.ProcessTime.Value)));
        session.AdvanceMissionFrame(0.25); session.RunMissionFrame(); Assert.Equal(new[] { 0.25, 0.25 }, seen); Assert.Equal(0.25, session.ProcessTime.Value); session.ReleasePlayerRecordViews();
    }
    /// <summary>고정시각을전진하고캡처한뒤입력해석이실패하더라도현재Frame은이미고정돼있다.</summary>
    [Fact]
    public void DriverCapturesAfterClockAdvanceBeforeScriptInput()
    {
        var (game, local, clock) = Owner(); var source = new HostTimeSource(); source.EnableFixed(new(25, 1000)); var screen = new Screen(8, 8); var input = new InputQueue();
        var runner = new UiScriptRunner(UiScript.Parse(["1\tkey\tnope"]), input, screen); var driver = new HostFrameDriver(screen, input, runner, source);
        Assert.Throws<ArgumentException>(() => driver.BeginFrame(() => clock.CaptureFrame(source.Milliseconds))); Assert.Equal(1ul, driver.Frame); Assert.Equal(1ul, clock.Frame.Value.Number); Assert.Equal(0.025, clock.GameTime.Value); GC.KeepAlive(game); GC.KeepAlive(local);
    }
    /// <summary>캡처예외는앞선시각/프레임전진을보존하고후행스크립트입력을실행하지않는다.</summary>
    [Fact]
    public void CaptureFailureStopsScriptInputAfterPreservingAdvance()
    {
        var source = new HostTimeSource(); source.EnableFixed(new(25, 1000)); var screen = new Screen(8, 8); var input = new InputQueue();
        var runner = new UiScriptRunner(UiScript.Parse(["1\tkey\tnope"]), input, screen); var driver = new HostFrameDriver(screen, input, runner, source); var failure = new IOException("실제프레임캡처실패");
        Assert.Same(failure, Assert.Throws<IOException>(() => driver.BeginFrame(() => throw failure))); Assert.Equal(1ul, driver.Frame); Assert.Equal(1025u, source.Milliseconds);
    }
}
