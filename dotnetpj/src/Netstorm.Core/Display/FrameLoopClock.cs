namespace Netstorm.Core.Display;

/// <summary>
/// 화면마다 실제 밀리초 시각을 고정하고 직전 그리기 시각을 기준으로 제한한다.
/// 지연을 여러 합성 갱신으로 쪼개거나 버리지 않는다. 게임 규칙의 남은 고정 틱과는 별개다.
/// 원본 00460e90·00436450 및 cpppj Client::Frame의 호출 순서를 따른다.
/// </summary>
public sealed class FrameLoopClock
{
    /// <summary>초를 밀리초로 바꾸는 배율.</summary>
    private const double MillisecondsPerSecond = 1000;

    /// <summary>직전 화면 갱신의 실제 시각. 첫 갱신에서는 로딩 시간을 제외한다.</summary>
    private long? _lastUpdateMilliseconds;

    /// <summary>직전 그리기 직전의 실제 시각. 그리기·표시·다음 갱신 비용 모두 제한 간격에 포함한다.</summary>
    private long? _lastDrawMilliseconds;

    /// <summary>원본 DWORD 오프셋/몫·나머지 변환을 제공하는 벽시계.</summary>
    private readonly OriginalGameClock _originalClock;

    /// <summary>직전 그리기 직전의 원본 double 벽시각. 제한에서 두 시각의 감산 비트를 보존한다.</summary>
    private double _lastDrawWallSeconds;

    /// <summary>지금까지 관찰한 시각. 단조 시계의 역행을 상태 변경 전에 거부한다.</summary>
    private long _lastObservedMilliseconds;

    /// <summary>이번 화면 갱신에 전달할 실제 경과 초. 첫 화면은 0이다.</summary>
    public double ElapsedSeconds { get; private set; }

    /// <summary>긴 지연이 상한 없이 전달됐는지 자동 창 검사에서 확인할 최대 경과 초.</summary>
    public double LongestElapsedSeconds { get; private set; }

    /// <summary>캡처한 원본 시계 설정을 주입한다. 단위 검사에서는 원점/epoch가 0인 시계를 쓴다.</summary>
    public FrameLoopClock(OriginalGameClock? originalClock = null)
    {
        _originalClock = originalClock ?? new OriginalGameClock(0);
    }

    /// <summary>화면 갱신 시작 시각을 한 번 읽어 고정하고 경과 시간을 돌려준다.</summary>
    /// <param name="milliseconds">단조 증가하는 실제 시계의 밀리초 값.</param>
    public double BeginFrame(long milliseconds)
    {
        Observe(milliseconds);
        ElapsedSeconds = _lastUpdateMilliseconds is long previous
            ? (milliseconds - previous) / MillisecondsPerSecond : 0;
        _lastUpdateMilliseconds = milliseconds;
        LongestElapsedSeconds = Math.Max(LongestElapsedSeconds, ElapsedSeconds);
        return ElapsedSeconds;
    }

    /// <summary>
    /// 그리기 전 남은 대기 밀리초를 구한다. 늦은 프레임·정지·무제한은 0이다.
    /// 1/maxFPS와 원본 double 벽시각의 차이를 비교한다. 나누어떨어지는 경계의 반올림도 보존한다.
    /// </summary>
    public int RemainingWaitMilliseconds(long milliseconds, int maxFps, bool paused)
    {
        Observe(milliseconds);
        int normalized = FramePacing.Normalize(maxFps);
        if (paused || normalized == FramePacing.Unlimited || _lastDrawMilliseconds is not long previous)
            return 0;
        double elapsed = _originalClock.WallSeconds(unchecked((uint)milliseconds)) - _lastDrawWallSeconds;
        // DWORD 결과가 감긴 경계에서는 호스트의 펼친 단조 간격으로 제한해 장시간 대기를 방지한다.
        if (elapsed < 0 || milliseconds - previous >= (1L << 32))
            elapsed = (milliseconds - previous) / MillisecondsPerSecond;
        double remaining = 1.0 / normalized - elapsed;
        return remaining <= 0 ? 0 : (int)Math.Ceiling(remaining * MillisecondsPerSecond);
    }

    /// <summary>대기 종료 뒤 그리기 직전 시각을 저장한다. 밀린 옛 예정 시각을 계속 더하지 않는다.</summary>
    public void BeginDraw(long milliseconds)
    {
        Observe(milliseconds);
        _lastDrawMilliseconds = milliseconds;
        _lastDrawWallSeconds = _originalClock.WallSeconds(unchecked((uint)milliseconds));
    }

    /// <summary>잘못된 시계를 거부하고 다음 호출의 역행 검사 기준을 저장한다.</summary>
    private void Observe(long milliseconds)
    {
        if (milliseconds < _lastObservedMilliseconds)
            throw new ArgumentOutOfRangeException(nameof(milliseconds));
        _lastObservedMilliseconds = milliseconds;
    }
}
