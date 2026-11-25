using Netstorm.Assets;
using Netstorm.Client.Host;

// 데모 3경로 진입의 스크립트 값과 --demo 강제 지정 경계를 검증한다. 0.998에서 추가됐다.
namespace Netstorm.Client.Tests;

/// <summary>R4-7a 데모 진입의 스크립트 demo 키와 명령줄 강제값을 검증한다.</summary>
public sealed class DemoEntryTests
{
    /// <summary>실제 데이터에서 시작 화면을 연다. 창은 열지 않는다.</summary>
    private static HostStartupScene Startup()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        return new HostStartupScene(data!);
    }

    /// <summary>데모 스크립트의 demo 키를 읽는다. 없으면 null이다.</summary>
    private static string? DemoKey(string name)
    {
        using var script = new Netstorm.Client.Mission.MissionScript(Startup().Configuration, name);
        Assert.True(script.Loaded);
        return script.Get("demo");
    }

    /// <summary>데모 폭풍은 스크립트 자체에 demo 1을 둔다.</summary>
    [Fact]
    public void StormDemoDeclaresDemoMode()
    {
        Assert.Equal("1", DemoKey("demostormrages"));
    }

    /// <summary>가드백·동맹 와해 대본에는 demo 키가 없어 진입 때 강제 지정이 필요하다.</summary>
    [Fact]
    public void GuardAndDissolvedNeedForcedDemo()
    {
        Assert.True(string.IsNullOrEmpty(DemoKey("guardmyback")));
        Assert.True(string.IsNullOrEmpty(DemoKey("dissolvedalliance")));
    }

    /// <summary>--demo는 데모 강제 진입을 켜고 기본값은 끈다.</summary>
    [Fact]
    public void DemoFlagParsesFromCommandLine()
    {
        Assert.True(HostOptions.Parse(["--mission", "guardmyback", "--demo"]).Demo);
        Assert.False(HostOptions.Parse(["--mission", "guardmyback"]).Demo);
        Assert.False(HostOptions.Parse([]).Demo);
    }
}
