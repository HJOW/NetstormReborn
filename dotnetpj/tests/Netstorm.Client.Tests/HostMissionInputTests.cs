using Netstorm.Client.Automation;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

/// <summary>호스트 프레임이 실제 입력 큐와 Guide 분배기를 같은 순서로 사용하는지 검사한다.</summary>
public sealed class HostMissionInputTests
{
    /// <summary>스크립트 입력과 실제 입력을 Guide 뒤 UI에 전달하고 처리 중 예약된 사건도 같은 프레임에 처리한다.</summary>
    [Fact]
    public void FrameDispatchesGuideBeforeUiAndDrainsNewEvents()
    {
        var input = new InputQueue();
        var screen = new Screen(64, 48);
        var runner = new UiScriptRunner(UiScript.Parse(["1\tesc"]), input, screen);
        var driver = new HostFrameDriver(screen, input, runner);
        var config = new ConfigInterface(_ => null); config.Startup(new ConfigInterface.StartupOptions());
        var trace = new List<string>();
        var guide = new GuideMissionOwner(config, input, new ScriptStringRing(),
            new PlayerParticipation(new byte[9 * PlayerParticipation.PlayerStride], new uint[81], new byte[9], new(), new()),
            new(), new(), new(), new() { Tell = text => trace.Add("tell:" + text) });
        var router = new HostMissionInput(guide, e => { trace.Add("ui:" + e.Code); return false; }, () => trace.Add("sync"));
        driver.BeginFrame();
        input.PushCommand(MissionGuideInput.AbortCommand, 0, 0);
        input.PushCommand(MissionGuideInput.AbortCommand, 0, 1);
        Assert.False(driver.ProcessInput());
        Assert.False(input.Empty); // 소비자를 설치하기 전 입력을 보존한다.
        driver.BindInputHandler(router.Dispatch);
        Assert.False(driver.ProcessInput());
        Assert.True(input.Empty);
        Assert.Equal(new[] { "ui:27", "sync", "tell:ABORT", "sync", "sync", "ui:33554629", "sync" }, trace);
    }

    /// <summary>UI가 종료를 요청하면 같은 큐의 후속 사건과 Tell 동기화를 다음 처리를 위해 남긴다.</summary>
    [Fact]
    public void QuitStopsImmediatelyAndPreservesRemainingEvents()
    {
        var input = new InputQueue(); var driver = new HostFrameDriver(new Screen(64, 48), input);
        var config = new ConfigInterface(_ => null); config.Startup(new ConfigInterface.StartupOptions());
        var guide = new GuideMissionOwner(config, input, new ScriptStringRing(),
            new PlayerParticipation(new byte[9 * PlayerParticipation.PlayerStride], new uint[81], new byte[9], new(), new()),
            new(), new(), new(), new());
        int synchronized = 0;
        var router = new HostMissionInput(guide, _ => true, () => synchronized++);
        driver.BindInputHandler(router.Dispatch);
        input.Push(42, 0, 0); input.Push(43, 0, 0);
        Assert.True(driver.ProcessInput());
        Assert.Equal(0, synchronized); Assert.Equal(43U, input.Pop(false).Code);
    }
}
