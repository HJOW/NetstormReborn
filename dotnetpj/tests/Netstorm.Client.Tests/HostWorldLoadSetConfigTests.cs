using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>월드 적재 setConfig 전달이 실제 설정 경계에 NUL 절단·fort 객체 기록으로 연결되는지 검사한다.
/// 근거: Config.Set(004404e0)의 CutNul과 0.438 확정 계약. 시작 흐름 연결은 다음 단위다. (2026-11-12 0.439에서 추가했다)</summary>
public sealed class HostWorldLoadSetConfigTests
{
    /// <summary>전달 효과가 NUL 포함 핸들을 잘라 fort 객체에 쓰고 루트 저장을 촉발하지 않는지 확인한다.</summary>
    [Fact]
    public void SetConfigHookWritesTruncatedHandleWithoutRootSave()
    {
        var config = new ConfigInterface(null);
        int notifications = 0;
        config.OnChanged = () => { notifications++; };
        Action<string, byte[]> hook = HostSession.BuildSetConfigHook(config);
        hook("fort.mySubHandle", [(byte)'Y', (byte)'o', (byte)'u', 0, (byte)'s', (byte)'o', (byte)'n', 0]);
        Assert.Equal(1, notifications);
        Config? fort = config.Registry.FindByName("fort");
        Assert.NotNull(fort);
        Assert.Equal("You", fort!.GetRaw("mySubHandle"));
        Assert.True(fort.Changed);
        // 루트 변경 표시는 꺼져 호스트의 options 저장이 촉발되지 않는다.
        Assert.False(config.Configuration.Changed);
    }

    /// <summary>설정·이름·핸들이 없으면 전달 효과 만들기를 거절하는지 확인한다.</summary>
    [Fact]
    public void SetConfigHookRejectsMissingArguments()
    {
        var config = new ConfigInterface(null);
        Assert.Throws<ArgumentNullException>(() => HostSession.BuildSetConfigHook(null!));
        Action<string, byte[]> hook = HostSession.BuildSetConfigHook(config);
        Assert.Throws<ArgumentException>(() => hook("", [(byte)'Y', 0]));
        Assert.Throws<ArgumentNullException>(() => hook("fort.mySubHandle", null!));
    }
}
