using Netstorm.Client.Host;

namespace Netstorm.Client.Tests;

/// <summary>호스트 실행 인자의 미션 진입·음소거 해석을 검사한다.
/// 근거: cpppj/src/app/main.cpp의 --run 인자 중 --mission·--mute-audio. (2026-11-12 0.398에서 추가했다)</summary>
public sealed class HostMissionOptionsTests
{
    /// <summary>--mission·--handle·--mute-audio를 읽고 사용법에 싣는다.</summary>
    [Fact]
    public void ParsesMissionAndMuteFlags()
    {
        HostOptions options = HostOptions.Parse(["--window", "--mission", "thewarbegins", "--handle", "Youpson", "--mute-audio"]);
        Assert.Equal("thewarbegins", options.Mission);
        Assert.Equal("Youpson", options.Handle);
        Assert.True(options.MuteAudio);
        Assert.False(HostOptions.Parse([]).MuteAudio);
        Assert.Null(HostOptions.Parse([]).Mission);
        Assert.Null(HostOptions.Parse([]).Handle);
        Assert.Contains("--mission", HostOptions.Usage(), StringComparison.Ordinal);
        Assert.Contains("--mute-audio", HostOptions.Usage(), StringComparison.Ordinal);
        Assert.Throws<ArgumentException>(() => HostOptions.Parse(["--mission"]));
        Assert.Throws<ArgumentException>(() => HostOptions.Parse(["--handle"]));
    }
}
