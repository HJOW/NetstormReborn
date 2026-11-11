using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>월드 적재 setConfig 전달이 쓰는 Config 설정 계약(NUL 절단·객체별 변경 표시·알림)을 검사한다.
/// 근거: Config.Set(004404e0)의 "%s = \"%s\"" 서식과 CutNul, ConfigInterface.Set(004414d0)의 변경 알림. (2026-11-12 0.438에서 추가했다)</summary>
public sealed class ConfigSetContractTests
{
    /// <summary>NUL 포함 값을 쓰면 NUL 앞에서 자르고 fort 객체의 변경 표시와 알림을 남기는지 확인한다.</summary>
    [Fact]
    public void SetCutsValueAtNulAndMarksFortChanged()
    {
        var config = new ConfigInterface(null);
        int notifications = 0;
        config.OnChanged = () => { notifications++; };
        Assert.False(config.Set("fort.mySubHandle", "You\0son\0"));
        Assert.Equal(1, notifications);
        Config? fort = config.Registry.FindByName("fort");
        Assert.NotNull(fort);
        Assert.Equal("You", fort!.GetRaw("mySubHandle"));
        Assert.True(fort.Changed);
        // 저장 본문에도 NUL 바이트가 남지 않는다.
        Assert.DoesNotContain("\0", fort.Text, StringComparison.Ordinal);
        Assert.Contains("mySubHandle = \"You\"", fort.Text, StringComparison.Ordinal);
        // 루트 변경 표시는 켜지지 않아 호스트의 options 저장이 촉발되지 않는다.
        Assert.False(config.Configuration.Changed);
    }

    /// <summary>같은 키에 다시 쓰면 이전 값이 바뀌고 존재 표시가 참인지 확인한다.</summary>
    [Fact]
    public void SetSameKeyAgainReportsExisting()
    {
        var config = new ConfigInterface(null);
        Assert.False(config.Set("fort.mySubHandle", "You"));
        Assert.True(config.Set("fort.mySubHandle", "You\0son\0"));
        Config? fort = config.Registry.FindByName("fort");
        Assert.NotNull(fort);
        Assert.Equal("You", fort!.GetRaw("mySubHandle"));
    }
}
