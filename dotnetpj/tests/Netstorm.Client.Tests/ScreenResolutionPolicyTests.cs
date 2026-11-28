using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 공통1280 폭 정책의 출력/논리 분리와 할당 전 복구를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 GPU 성공과 구별해 목록/설정/입력 크기의 정책만 검사한다.</summary>
public sealed class ScreenResolutionPolicyTests
{
    /// <summary>FSR12개는 실제 가용성에 따라 허용하고 원래 네 화면비의1280 입력으로 바꾼다.</summary>
    [Theory]
    [InlineData(1920, 1080, 720)]
    [InlineData(1920, 1200, 800)]
    [InlineData(1920, 1440, 960)]
    [InlineData(1920, 1536, 1024)]
    [InlineData(2560, 1440, 720)]
    [InlineData(2560, 1600, 800)]
    [InlineData(2560, 1920, 960)]
    [InlineData(2560, 2048, 1024)]
    [InlineData(3840, 2160, 720)]
    [InlineData(3840, 2400, 800)]
    [InlineData(3840, 2880, 960)]
    [InlineData(3840, 3072, 1024)]
    public void OutputRequiresActualFsrAndKeepsAspect(int width, int height, uint logicalHeight)
    {
        Assert.True(ScreenResolutions.IsFsr(width, height)); Assert.False(ScreenResolutions.IsSupported(width, height));
        Assert.True(ScreenResolutions.IsSupported(width, height, true)); Assert.Equal((1280u, logicalHeight), ScreenResolutions.LogicalSize(width, height));
        Assert.Equal(((uint)width, (uint)height), ScreenResolutions.Normalize(width, height, true));
        Assert.Equal((1280u, logicalHeight), ScreenResolutions.Normalize(width, height, false));
    }
    /// <summary>옛 큰 크기/잘못된 작은 크기/거리 동률을 CPP 정책의 정해진 결과와 대조한다.</summary>
    [Theory]
    [InlineData(1366, 768, 1280, 720)]
    [InlineData(1600, 900, 1280, 720)]
    [InlineData(1600, 1000, 1280, 800)]
    [InlineData(1600, 1200, 1280, 960)]
    [InlineData(1600, 1280, 1280, 1024)]
    [InlineData(1600, 950, 1280, 720)]
    [InlineData(1000, 700, 1024, 768)]
    [InlineData(0, 768, 1024, 768)]
    [InlineData(-1, 1080, 1024, 768)]
    [InlineData(1920, -1, 1024, 768)]
    public void UnsupportedSavedSizesRecoverBeforeAllocation(long width, long height, uint expectedWidth, uint expectedHeight) =>
        Assert.Equal((expectedWidth, expectedHeight), ScreenResolutions.Normalize(width, height, false));
    /// <summary>최초 호스트가 옛 설정을 실제 Screen 할당 전 복구하며 정상 명시 축의 우선순위도 보존한다.</summary>
    [Fact]
    public void StartupUsesOnePolicyForAllocatedAndSavedDimensions()
    {
        var config = new ConfigInterface(null); config.SetInt("SCREENW", 3840); config.SetInt("SCREENH", 2160);
        var options = HostOptions.Parse([]); options.ApplyStartupConfiguration(config);
        Assert.Equal(1280, options.Width); Assert.Equal(720, options.Height); Assert.Equal(1280, config.GetInt("SCREENW")); Assert.Equal(720, config.GetInt("SCREENH"));
        var explicitOptions = HostOptions.Parse(["--width", "800", "--height", "600"]); explicitOptions.ApplyStartupConfiguration(config);
        Assert.Equal(800, explicitOptions.Width); Assert.Equal(600, explicitOptions.Height); Assert.Equal(800, config.GetInt("SCREENW")); Assert.Equal(600, config.GetInt("SCREENH"));
    }
}
