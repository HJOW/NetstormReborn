using System.Globalization;

namespace Netstorm.AnalyzeManager;

/// <summary>자동 분석과 두 사용자 녹화 모드가 공유하는 초당 캡처 수.</summary>
public static class AnalysisFrameRate
{
    /// <summary>매개변수를 생략했을 때 사용하는 분석·녹화 속도.</summary>
    public const int Default = 30;
    /// <summary>더 짧은 동작을 관찰할 때 선택할 수 있는 분석·녹화 속도.</summary>
    public const int High = 60;

    /// <summary>지원하지 않는 FPS를 게임 실행·녹화 시작 전에 거부한다.</summary>
    public static int Validate(int fps) => fps is Default or High ? fps
        : throw new ArgumentException("fps는 30 또는 60이어야 합니다.");

    /// <summary>CLI의 선택적 --fps 값을 해석하고 생략한 값은 세션 설정에 맡긴다.</summary>
    public static int? Parse(string? value)
    {
        if (value == null) return null;
        if (!int.TryParse(value, NumberStyles.None, CultureInfo.InvariantCulture, out int fps))
            throw new ArgumentException("--fps는 30 또는 60이어야 합니다.");
        return Validate(fps);
    }
}

/// <summary>누적 시각으로 30/60FPS 간격을 계산하며 늦어진 프레임을 몰아서 캡처하지 않는다.</summary>
public sealed class FrameSchedule
{
    private readonly long _intervalTicks;
    private readonly int _fps;
    private long _nextFrame = 1;

    /// <summary>명시적인 pollMs가 있으면 기존 사용자 간격을 사용하고, 없으면 FPS를 따른다.</summary>
    public FrameSchedule(int fps, int pollMs = 0)
    {
        _fps = AnalysisFrameRate.Validate(fps);
        if (pollMs is < 0 or > 5000) throw new ArgumentException("pollMs는 0~5000이어야 합니다. 0은 fps 설정을 사용합니다.");
        _intervalTicks = pollMs * TimeSpan.TicksPerMillisecond;
    }

    /// <summary>33.333/16.667ms의 소수 간격을 유지하고 처리 지연으로 지나간 목표 시각을 건너뛴다.</summary>
    public TimeSpan DelayAfter(TimeSpan elapsed)
    {
        long passedFrames = _intervalTicks > 0 ? elapsed.Ticks / _intervalTicks
            : elapsed.Ticks * _fps / TimeSpan.TicksPerSecond;
        long frame = Math.Max(_nextFrame, passedFrames + 1);
        _nextFrame = frame + 1;
        long due = _intervalTicks > 0 ? frame * _intervalTicks
            : (frame * TimeSpan.TicksPerSecond + _fps - 1) / _fps;
        return TimeSpan.FromTicks(Math.Max(0, due - elapsed.Ticks));
    }
}
