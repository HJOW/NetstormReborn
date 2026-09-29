using Netstorm.Core.Display;

namespace Netstorm.Core.Tests;

/// <summary>화면비·전체화면 배치 계산 테스트 (AGENTS.md: 16:9, 16:10, 4:3 지원)</summary>
public sealed class ScreenLayoutTests
{
    /// <summary>4:3 창은 어떤 모드에서도 창 전체를 원본 해상도 그대로 쓴다 (배율 1)</summary>
    [Theory]
    [InlineData(WideScreenMode.Extend)]
    [InlineData(WideScreenMode.Letterbox)]
    public void FourByThree_UsesWholeWindow(WideScreenMode mode)
    {
        ScreenLayout layout = ScreenLayoutCalculator.Compute(1024, 768, 768, mode);
        Assert.Equal(new ScreenLayout(1024, 768, 0, 0, 1024, 768), layout);
        Assert.Equal(1.0, layout.Scale);
        Assert.True(layout.IsIntegerScale);
    }

    /// <summary>
    /// 원본 1024×768 을 16:9 모니터 1920×1080 에서 풀스크린으로 찍은 로컬 영상과 같은 배치:
    /// 가운데 1440×1080, 좌우 여백 각 240px, 배율 1080/768 = 1.40625 (docs/videos/README.md)
    /// </summary>
    [Fact]
    public void Letterbox_On1080p_MatchesOriginalRecording()
    {
        ScreenLayout layout = ScreenLayoutCalculator.Compute(1920, 1080, 768, WideScreenMode.Letterbox);
        Assert.Equal(new ScreenLayout(1024, 768, 240, 0, 1440, 1080), layout);
        Assert.Equal(1.40625, layout.Scale);
        Assert.False(layout.IsIntegerScale);
    }

    /// <summary>시야 확장: 16:9 는 창 전체를 쓰고 논리 폭만 늘어난다 (논리 높이 768, 배율 1.40625)</summary>
    [Fact]
    public void Extend_On16By9_WidensLogicalView()
    {
        ScreenLayout layout = ScreenLayoutCalculator.Compute(1920, 1080, 768, WideScreenMode.Extend);
        Assert.Equal(768, layout.LogicalHeight);
        Assert.Equal(1365, layout.LogicalWidth);
        Assert.Equal((0, 0, 1920, 1080), (layout.ViewportX, layout.ViewportY, layout.ViewportWidth, layout.ViewportHeight));
        Assert.Equal(1.40625, layout.Scale);
    }

    /// <summary>시야 확장: 16:10 (1920×1200) 은 창 전체, 논리 폭 = 768 × 1.6 = 1229 (반올림)</summary>
    [Fact]
    public void Extend_On16By10_WidensLogicalView()
    {
        ScreenLayout layout = ScreenLayoutCalculator.Compute(1920, 1200, 768, WideScreenMode.Extend);
        Assert.Equal(1229, layout.LogicalWidth);
        Assert.Equal(768, layout.LogicalHeight);
        Assert.Equal((0, 0, 1920, 1200), (layout.ViewportX, layout.ViewportY, layout.ViewportWidth, layout.ViewportHeight));
        Assert.Equal(1.5625, layout.Scale);
    }

    /// <summary>원본의 낮은 해상도(640×480)를 고른 경우: 16:9 에서도 같은 높이 480 을 기준으로 폭이 늘어난다</summary>
    [Fact]
    public void Extend_LowerOriginalResolution_KeepsHeight()
    {
        ScreenLayout layout = ScreenLayoutCalculator.Compute(1920, 1080, 480, WideScreenMode.Extend);
        Assert.Equal(480, layout.LogicalHeight);
        Assert.Equal(853, layout.LogicalWidth);
        Assert.Equal(2.25, layout.Scale);
        // 레터박스는 640×480 을 가운데 1440×1080 영역에 정수가 아닌 배율(2.25)로 늘린다.
        ScreenLayout classic = ScreenLayoutCalculator.Compute(1920, 1080, 480, WideScreenMode.Letterbox);
        Assert.Equal(new ScreenLayout(640, 480, 240, 0, 1440, 1080), classic);
    }

    /// <summary>16:9 보다 넓은 모니터(21:9)는 16:9 로 제한하고 좌우를 비운다</summary>
    [Fact]
    public void Extend_UltraWide_ClampsTo16By9()
    {
        ScreenLayout layout = ScreenLayoutCalculator.Compute(2560, 1080, 768, WideScreenMode.Extend);
        Assert.Equal((320, 0, 1920, 1080), (layout.ViewportX, layout.ViewportY, layout.ViewportWidth, layout.ViewportHeight));
        Assert.Equal(1365, layout.LogicalWidth);
    }

    /// <summary>4:3 보다 좁은 모니터(5:4, 1280×1024)는 4:3 으로 제한하고 위아래를 비운다</summary>
    [Fact]
    public void Extend_Narrow_ClampsTo4By3()
    {
        ScreenLayout layout = ScreenLayoutCalculator.Compute(1280, 1024, 768, WideScreenMode.Extend);
        Assert.Equal((0, 32, 1280, 960), (layout.ViewportX, layout.ViewportY, layout.ViewportWidth, layout.ViewportHeight));
        Assert.Equal(1024, layout.LogicalWidth);
        Assert.Equal(768, layout.LogicalHeight);
    }

    /// <summary>창 크기가 0 이하(최소화)여도 예외 없이 1×1 이상의 배치를 돌려준다</summary>
    [Fact]
    public void ZeroSizedWindow_DoesNotThrow()
    {
        ScreenLayout layout = ScreenLayoutCalculator.Compute(0, 0, 768, WideScreenMode.Extend);
        Assert.True(layout.LogicalWidth >= 1 && layout.ViewportWidth >= 1 && layout.ViewportHeight >= 1);
    }

    /// <summary>창 좌표 ↔ 논리 좌표 변환은 서로 역변환이고 레터박스 여백은 화면 밖으로 판정한다</summary>
    [Fact]
    public void CoordinateConversion_RoundTrips()
    {
        ScreenLayout layout = ScreenLayoutCalculator.Compute(1920, 1080, 768, WideScreenMode.Letterbox);
        // 게임 화면 왼쪽 위 모서리(240, 0) 는 논리 (0, 0), 오른쪽 아래(1680, 1080) 는 (1024, 768)
        Assert.Equal((0.0, 0.0), layout.ToLogical(240, 0));
        var (x, y) = layout.ToLogical(1680, 1080);
        Assert.Equal(1024.0, x, 6);
        Assert.Equal(768.0, y, 6);
        // 논리 (512, 384) = 화면 중앙 = 창 (960, 540)
        var (windowX, windowY) = layout.ToWindow(512, 384);
        Assert.Equal(960.0, windowX, 6);
        Assert.Equal(540.0, windowY, 6);
        // 왼쪽 여백은 게임 화면이 아니다
        Assert.False(layout.ContainsWindowPoint(100, 500));
        Assert.True(layout.ContainsWindowPoint(240, 0));
        Assert.False(layout.ContainsWindowPoint(1680, 500));
    }

    /// <summary>화면비 이름: 지원하는 세 화면비와 그 밖의 값</summary>
    [Theory]
    [InlineData(1024, 768, "4:3")]
    [InlineData(1600, 1200, "4:3")]
    [InlineData(1920, 1200, "16:10")]
    [InlineData(1280, 800, "16:10")]
    [InlineData(1920, 1080, "16:9")]
    [InlineData(1366, 768, "16:9")]
    [InlineData(2560, 1080, "2.37:1")]
    public void DescribeAspect_NamesSupportedRatios(int width, int height, string expected)
    {
        Assert.Equal(expected, ScreenLayoutCalculator.DescribeAspect(width, height));
    }

    /// <summary>원본 세 해상도의 논리 높이 목록과 기본값</summary>
    [Fact]
    public void ViewHeights_AreOriginalResolutions()
    {
        Assert.Equal(new[] { 480, 600, 768 }, ScreenLayoutCalculator.ViewHeights);
        Assert.Contains(ScreenLayoutCalculator.DefaultViewHeight, ScreenLayoutCalculator.ViewHeights);
    }
}
