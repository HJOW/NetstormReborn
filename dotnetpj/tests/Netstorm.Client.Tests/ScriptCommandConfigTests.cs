using Netstorm.Client.Input;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

/// <summary>
/// 스크립트 명령→Config 쓰기 왕복을 실제 Config 객체로 검사한다.
/// 근거: cpppj/tests/ScriptCommandTests.cpp 의 ConfigOverwriteAndQueueLifetime 중 Config 부분.
/// 대화창 분리(ParseDialogPage) 부분은 대화창 단위에서 잇는다. (R3k-11, 2026-10-31)
/// </summary>
public sealed class ScriptCommandConfigTests
{
    /// <summary>같은 키를 덮어쓰고 같은 입력으로는 다른 명령을 버리지 않는다.</summary>
    [Fact]
    public void ConfigOverwriteAndQueueLifetime()
    {
        var config = new ConfigInterface(null);
        config.Configuration.Assign("[END]\r\n");
        config.Set("DoneTutorial1", "0");
        int changed = 0;
        config.OnChanged = () => { changed++; };
        var strings = new ScriptStringRing();
        var queue = new InputQueue();
        var hooks = new ScriptCommandHooks { Lookup = _ => ScriptCommand.ScriptConfig };
        ScriptCommand.QueueScriptCommand("Config,DoneTutorial1=1", queue, strings, hooks);
        ScriptCommand.QueueScriptCommand("Config,DoneTutorial2=1", queue, strings, hooks);
        Assert.Equal(0, config.GetInt("DoneTutorial1"));
        Assert.Equal(0, changed);
        // 실제 입력 큐에서 꺼낸 순서대로 적용하며 첫 세대 전환만 보존한다(별도 큐는 사용하지 않는다).
        while (!queue.Empty)
        {
            InputEvent ev = queue.Pop(false);
            Assert.Equal(ScriptCommand.ScriptConfig, ev.Code);
            ScriptCommand.ApplyScriptConfig(strings.Resolve(unchecked((uint)ev.X)), config);
        }
        Assert.Equal(1, config.GetInt("DoneTutorial1"));
        Assert.Equal(1, config.GetInt("DoneTutorial2"));
        Assert.Equal(2, changed);
        Assert.Equal("1", new ConfigText(config.SaveText("options.cfg")).GetRaw("DoneTutorial1"));
        bool rejected = false;
        try
        {
            strings.Resolve(0);
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }
        Assert.True(rejected);
    }
}
