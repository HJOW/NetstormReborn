using System.Text;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

/// <summary>Guide 소유자가 실제 Config·입력 큐·문자열 고리와 Player 수명을 공유하는지 검사한다.</summary>
public sealed class GuideMissionOwnerTests
{
    /// <summary>명시 source와 큐의 명령으로 시작·분기·재시작·종료할 때 현재 등록과 미션 필드를 유지한다.</summary>
    [Fact]
    public void CommandsOwnScriptAndPreserveSourceAcrossTransitions()
    {
        var files = new Dictionary<string, byte[]>(StringComparer.OrdinalIgnoreCase)
        {
            ["d\\setup.cfg"] = Encoding.Latin1.GetBytes("DataDir = \"\\D\"\r\ncurrentLanguage=\"english\"\r\nmissionSpec = \"{DataDir}\\{local.1}.{currentLanguage}\"\r\n"),
            ["\\D\\one.english"] = Encoding.Latin1.GetBytes("missionType=\"Tutorial\"\r\nfork1=\"two\"\r\n"),
            ["\\D\\two.english"] = Encoding.Latin1.GetBytes("missionType=\"Tutorial\"\r\n"),
        };
        byte[]? Read(string path) => files.GetValueOrDefault(path);
        var config = new ConfigInterface(Read);
        config.Startup(new ConfigInterface.StartupOptions());
        var input = new InputQueue(); var strings = new ScriptStringRing();
        var players = new PlayerParticipation(new byte[9 * PlayerParticipation.PlayerStride], new uint[81], new byte[9], new(), new());
        var globals = new GuideMissionGlobals(); var trace = new List<string>();
        var owner = new GuideMissionOwner(config, input, strings, players, new(), new(), globals, new()
        {
            ReleaseFortRequest = () => trace.Add("request"), SetAllowAll = _ => trace.Add("allow"),
            Created = runtime => trace.Add("created:" + runtime.SaveToken), Tell = text => trace.Add(text),
            ShowStage = () => trace.Add("stage")
        });
        owner.Begin("one", demo: 4, mode: 9, source: Read);
        Assert.True(globals.MissionMode); Assert.Equal("one", owner.Mission!.Name);
        Assert.True(MissionGuideInput.QueueControlAction("missionfork", "1", input, strings));
        Assert.True(owner.Dispatch(input.Pop(false)));
        Assert.Equal("two", owner.Mission!.Name); Assert.Equal(2U, owner.Runtime!.SaveToken);
        owner.Runtime.Mode = 8; owner.Runtime.Gate.Demo = 3;
        Assert.True(MissionGuideInput.QueueControlAction("missionrestart", "", input, strings));
        Assert.True(owner.Dispatch(input.Pop(false)));
        Assert.Equal("two", owner.Mission!.Name); Assert.Equal(1U, owner.Runtime!.Mode); Assert.Equal(1U, owner.Runtime.Gate.Demo);
        Assert.True(MissionGuideInput.QueueControlAction("missionend", "", input, strings));
        Assert.True(owner.Dispatch(input.Pop(false)));
        Assert.Null(owner.Mission); Assert.Null(owner.Runtime); Assert.False(globals.MissionMode);
        Assert.Equal(new[] { "created:1", "request", "allow", "created:2", "request", "allow", "created:3", "request", "allow" }, trace);
        Assert.True(MissionGuideInput.QueueControlAction("missionbegin", "one", input, strings));
        Assert.True(owner.Dispatch(input.Pop(false)));
        Assert.Equal("one", owner.Mission!.Name);
        Assert.Equal(4U, owner.Runtime!.SaveToken);
        owner.Release();

    }
    /// <summary>미션 생성 실패 후에도 등록된 설정을 소유자가 명시적으로 반납한다.</summary>
    [Fact]
    public void MissingMissionCanBeReleasedAfterFactoryFailure()
    {
        var config = new ConfigInterface(_ => null);
        config.Startup(new ConfigInterface.StartupOptions());
        int initialCount = config.Registry.Objects.Count;
        var globals = new GuideMissionGlobals();
        var owner = new GuideMissionOwner(config, new(), new(),
            new PlayerParticipation(new byte[9 * PlayerParticipation.PlayerStride], new uint[81], new byte[9], new(), new()),
            new(), new(), globals, new() { ReleaseFortRequest = () => { } });
        Assert.Throws<InvalidOperationException>(() => owner.Begin("missing"));
        Assert.NotNull(owner.Mission); Assert.Null(owner.Runtime); Assert.True(globals.MissionMode);
        Assert.Equal(initialCount + 1, config.Registry.Objects.Count);
        owner.Release();
        Assert.Null(owner.Mission); Assert.False(globals.MissionMode);
        Assert.Equal(initialCount, config.Registry.Objects.Count);
    }

}
