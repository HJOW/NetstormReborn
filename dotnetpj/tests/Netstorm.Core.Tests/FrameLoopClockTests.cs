using Netstorm.Core.Display;

namespace Netstorm.Core.Tests;

/// <summary>원본 그리기 제한 분기와 긴 실제 시간열을 같은 입력으로 검사한다.</summary>
public sealed class FrameLoopClockTests
{
    /// <summary>첫 갱신은 로딩 시간을 제외하고 이후 프레임은 실제 불규칙 간격을 그대로 전달한다.</summary>
    [Fact]
    public void BeginFrame_PreservesIrregularAndLongIntervals()
    {
        var clock = new FrameLoopClock();
        Assert.Equal(0, clock.BeginFrame(10000));
        Assert.Equal(0.014, clock.BeginFrame(10014));
        Assert.Equal(0.009, clock.BeginFrame(10023));
        Assert.Equal(1.2, clock.BeginFrame(11223));
        Assert.Equal(0, clock.BeginFrame(11223));
        Assert.Equal(1.2, clock.LongestElapsedSeconds);
    }

    /// <summary>정지·상한 없음·첫 그리기는 기다리지 않는 원본 분기다.</summary>
    [Theory]
    [InlineData(75, true)]
    [InlineData(0, false)]
    [InlineData(-1, false)]
    public void RemainingWait_PauseAndUnlimitedBypassLimit(int maxFps, bool paused)
    {
        var clock = new FrameLoopClock();
        Assert.Equal(0, clock.RemainingWaitMilliseconds(100, 75, false));
        clock.BeginDraw(100);
        Assert.Equal(0, clock.RemainingWaitMilliseconds(100, maxFps, paused));
    }

    /// <summary>직전 그리기부터 센 밀리초 눈금은 정확한 1/maxFPS 경계에서 제한을 푼다.</summary>
    [Theory]
    [InlineData(75, 0, 14)]
    [InlineData(75, 13, 1)]
    [InlineData(75, 14, 0)]
    [InlineData(60, 16, 1)]
    [InlineData(60, 17, 0)]
    [InlineData(120, 8, 1)]
    [InlineData(120, 9, 0)]
    [InlineData(1000, 1, 0)]
    public void RemainingWait_MatchesOriginalMillisecondBoundary(int maxFps, int elapsed, int expected)
    {
        var clock = new FrameLoopClock();
        clock.BeginDraw(100);
        Assert.Equal(expected, clock.RemainingWaitMilliseconds(100 + elapsed, maxFps, false));
    }

    /// <summary>긴 처리 비용 뒤에는 즉시 그리며 그 실제 시각부터 새 간격을 센다. 옛 마감 시각을 따라잡지 않는다.</summary>
    [Fact]
    public void LateFrame_RestartsLimitFromActualDraw()
    {
        var clock = new FrameLoopClock();
        clock.BeginFrame(100);
        clock.BeginDraw(100);
        Assert.Equal(0.8, clock.BeginFrame(900));
        Assert.Equal(0, clock.RemainingWaitMilliseconds(930, 75, false));
        clock.BeginDraw(930);
        Assert.Equal(0.035, clock.BeginFrame(935));
        Assert.Equal(9, clock.RemainingWaitMilliseconds(935, 75, false));
    }

    /// <summary>정지 중 그리기에도 기준을 갱신하므로 재개가 과거 대기 시각을 이어받지 않는다.</summary>
    [Fact]
    public void PausedDraw_UpdatesResumeReference()
    {
        var clock = new FrameLoopClock();
        clock.BeginDraw(100);
        Assert.Equal(0, clock.RemainingWaitMilliseconds(102, 75, true));
        clock.BeginDraw(102);
        Assert.Equal(13, clock.RemainingWaitMilliseconds(103, 75, false));
    }

    /// <summary>24Hz 따라잡기와 달리 밀리초 시계는 큰 값·음수·역행을 안전하게 처리한다.</summary>
    [Fact]
    public void Clock_RejectsReversalWithoutChangingElapsed()
    {
        var clock = new FrameLoopClock();
        Assert.Throws<ArgumentOutOfRangeException>(() => clock.BeginFrame(-1));
        clock.BeginFrame(long.MaxValue - 1000);
        Assert.Equal(1, clock.BeginFrame(long.MaxValue));
        Assert.Throws<ArgumentOutOfRangeException>(() => clock.BeginFrame(long.MaxValue - 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => clock.BeginDraw(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => clock.RemainingWaitMilliseconds(0, 75, false));
        Assert.Equal(1, clock.ElapsedSeconds);
    }

    /// <summary>1.001−1.0의 실제 double 값이 1ms보다 작으면 원본처럼 다음 눈금까지 기다린다.</summary>
    [Fact]
    public void Limit_UsesConvertedDoubleDifferenceAtExactBoundary()
    {
        var clock = new FrameLoopClock(new OriginalGameClock(100));
        clock.BeginDraw(1100);
        Assert.Equal(1, clock.RemainingWaitMilliseconds(1101, 1000, false));
        Assert.Equal(0, clock.RemainingWaitMilliseconds(1102, 1000, false));
    }

    /// <summary>epoch 밀리초 합의 감김에서 음수 벽시각 차이로 긴 대기를 만드는 것을 방지한다.</summary>
    [Fact]
    public void Limit_WrappedWallUsesHostMonotonicInterval()
    {
        var clock = new FrameLoopClock(new OriginalGameClock(0, 4294967));
        clock.BeginDraw(295);
        Assert.Equal(13, clock.RemainingWaitMilliseconds(296, 75, false));
        Assert.Equal(0, clock.RemainingWaitMilliseconds(309, 75, false));
        clock.BeginDraw(309);
        Assert.Equal(14, clock.RemainingWaitMilliseconds(309, 75, false));
        Assert.Equal(0, clock.RemainingWaitMilliseconds(309 + (1L << 32), 75, false));
    }
}
