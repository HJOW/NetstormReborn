using Netstorm.Assets;
using Netstorm.Client.Chat;
using Netstorm.Client.Host;

namespace Netstorm.Client.Tests; // 실제 Config 변경과 OS 달력 경계를 분리해 검증한다.

/// <summary>같은 옵션/기한 수명과 실제 Windows CRT 달력만 검사한다. 게임 창·통신은 실행하지 않는다.</summary>
public sealed class ClientMonitorOptionsTests
{
    /// <summary>전체 signed 옵션·기존 기한·초기화 비트와 다른 구독자를 보존한다.</summary>
    [Fact]
    public void ActualConfigurationKeepsDeadlineAndOnlyReleasesItsSubscription()
    {
        var startup = new HostStartupScene(GameDataLocator.FindDataDirectory() ?? throw new InvalidOperationException("시작 자산 부재"));
        var state = new MonitorTimeState { Initialized = 0x80000000, Deadline = 123 }; var activity = new ClientConnectionActivityGlobals();
        int changes = 0; Action listener = () => changes++; startup.Configuration.OnChanged += listener;
        using var options = new ClientMonitorOptions(state, activity, startup.Configuration);
        Assert.Equal(1u, activity.AutoRestart.Value);
        startup.Configuration.Set("monitor", "-2147483648"); startup.Configuration.SetInt("alert", 1);
        Assert.Equal(int.MinValue, state.Monitor); Assert.Equal(1, state.Alert); Assert.Equal(123.0, state.Deadline); Assert.Equal(0x80000000u, state.Initialized);
        RawMonitorTime.Step(state, new() { WallSeconds = () => 100 });
        Assert.Equal(0x80000001u, state.Initialized); Assert.Equal(280.0, state.Deadline);
        startup.Configuration.SetInt("monitor", 0); startup.Configuration.SetInt("alert", 0);
        RawMonitorTime.Step(state, new() { WallSeconds = () => throw new IOException("비활성 시계 조회") });
        Assert.Equal(280.0, state.Deadline); int before = changes;
        options.Dispose(); activity.AutoRestart.Value = 7; startup.Configuration.SetInt("monitor", 9);
        Assert.Equal(0, state.Monitor); Assert.Equal(7u, activity.AutoRestart.Value); Assert.Equal(before + 1, changes);
    }

    /// <summary>실제 CRT의32비트 시간→지역 달력→영문25BYTE/개행을 검증한다. CPU fixture의 고정 달력 입력과 구별한다.</summary>
    [Fact]
    public void ActualWindowsCalendarKeepsOriginalAsctimeShape()
    {
        if (!OperatingSystem.IsWindows()) return;
        string calendar = WindowsMonitorCalendar.Read(); Assert.Equal(25, calendar.Length); Assert.Equal('\n', calendar[^1]);
        Assert.Matches("^[A-Z][a-z]{2} [A-Z][a-z]{2} [ 0-9][0-9] [0-9]{2}:[0-9]{2}:[0-9]{2} [0-9]{4}\\n$", calendar);
    }
}
