using Netstorm.Core.Display;

namespace Netstorm.Core.Tests;

/// <summary>화면 루프 속도 규칙 테스트 (원본 메인 루프의 프레임 제한 방식, docs/exe/main-loop.md 4절)</summary>
public sealed class FramePacingTests
{
    /// <summary>원본 기본 상한 75 는 밀리초 눈금 시계에서 14ms 간격, 초당 약 71.43바퀴가 된다</summary>
    [Fact]
    public void OriginalMaxFps_GivesFourteenMillisecondFrames()
    {
        Assert.Equal(75, FramePacing.OriginalMaxFps);
        Assert.Equal(14, FramePacing.FrameMilliseconds(FramePacing.OriginalMaxFps));
        Assert.Equal(TimeSpan.FromMilliseconds(14), FramePacing.FrameInterval(FramePacing.OriginalMaxFps));
        Assert.Equal(1000.0 / 14, FramePacing.OriginalFramesPerSecond, 12);
    }

    /// <summary>간격은 1000 / maxFPS 를 밀리초로 올림한 값이다 (나누어떨어지면 그대로)</summary>
    [Theory]
    [InlineData(60, 17)]
    [InlineData(120, 9)]
    [InlineData(30, 34)]
    [InlineData(50, 20)]
    [InlineData(100, 10)]
    [InlineData(1000, 1)]
    public void FrameMilliseconds_RoundsUp(int maxFps, int expected)
    {
        Assert.Equal(expected, FramePacing.FrameMilliseconds(maxFps));
        Assert.Equal(1000.0 / expected, FramePacing.FramesPerSecond(maxFps), 12);
    }

    /// <summary>0 이하는 상한 없음이다: 간격이 없고 초당 바퀴 수는 무한대로 본다 (원본도 기다리지 않는다)</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(-1)]
    public void Unlimited_HasNoInterval(int maxFps)
    {
        Assert.Equal(FramePacing.Unlimited, FramePacing.Normalize(maxFps));
        Assert.Equal(0, FramePacing.FrameMilliseconds(maxFps));
        Assert.Null(FramePacing.FrameInterval(maxFps));
        Assert.True(double.IsPositiveInfinity(FramePacing.FramesPerSecond(maxFps)));
    }

    /// <summary>허용 범위를 벗어난 상한은 가장 가까운 허용 값으로 맞춘다</summary>
    [Theory]
    [InlineData(1, FramePacing.MinimumMaxFps)]
    [InlineData(14, FramePacing.MinimumMaxFps)]
    [InlineData(15, 15)]
    [InlineData(75, 75)]
    [InlineData(1000, 1000)]
    [InlineData(5000, FramePacing.MaximumMaxFps)]
    public void Normalize_ClampsToAllowedRange(int maxFps, int expected)
    {
        Assert.Equal(expected, FramePacing.Normalize(maxFps));
    }

    /// <summary>
    /// 원본의 "다음 시각 = 지금 + 간격" 타이머는 루프 주기의 배수로 올림된다: 14ms 루프에서 0.04초 타이머는 42ms(3바퀴),
    /// 0.08초 타이머는 84ms(6바퀴)다. 영상 측정값(41.9ms, 83.3ms)과 클론의 24Hz·12Hz 기준이 여기서 나온다.
    /// </summary>
    [Theory]
    [InlineData(0.04, 42)]
    [InlineData(0.08, 84)]
    public void OriginalTimers_QuantizeToFrameMultiples(double timerSeconds, int expectedMilliseconds)
    {
        int frame = FramePacing.FrameMilliseconds(FramePacing.OriginalMaxFps);
        int loops = (int)Math.Ceiling(timerSeconds * 1000 / frame);
        Assert.Equal(expectedMilliseconds, loops * frame);
    }
}
