using System.Diagnostics;
using System.Globalization;

namespace Netstorm.Game;

/// <summary>
/// <c>--perf</c> 로 켜는 성능 진단. 1초마다 그린 프레임 수와 갱신·그리기 평균/최대 시간(ms)을 콘솔에 적는다.
/// 그리기 시간은 CPU가 그리기 명령을 만드는 데 쓴 시간이며 수직 동기 대기·GPU 처리 시간은 포함하지 않는다.
/// </summary>
internal sealed class PerfMeter
{
    /// <summary>보고 간격(초).</summary>
    private const double ReportSeconds = 1.0;
    /// <summary>시간 측정 기준 타이머.</summary>
    private readonly Stopwatch _clock = Stopwatch.StartNew();
    /// <summary>이번 보고 구간의 갱신 호출 수.</summary>
    private int _updates;
    /// <summary>이번 보고 구간의 그리기 호출 수.</summary>
    private int _draws;
    /// <summary>이번 구간 갱신 시간 합과 최대(ms).</summary>
    private double _updateSum, _updateMax;
    /// <summary>이번 구간 그리기 시간 합과 최대(ms).</summary>
    private double _drawSum, _drawMax;
    /// <summary>마지막 보고 시각(초).</summary>
    private double _lastReport;

    /// <summary>가장 최근 1초 구간의 초당 그린 프레임 수 (화면 오버레이용).</summary>
    public double Fps { get; private set; }

    /// <summary>가장 최근 1초 구간의 평균 그리기 시간(ms) (화면 오버레이용).</summary>
    public double DrawMilliseconds { get; private set; }
    /// <summary>이번 구간의 그리기 세부 구간별 시간 합(ms). 구간 이름 → 합.</summary>
    private readonly SortedDictionary<string, double> _sections = [];

    /// <summary>지금 켜져 있는 성능 진단 (꺼져 있으면 null). 그리기 코드가 세부 구간을 보고할 때 쓴다.</summary>
    public static PerfMeter? Current { get; private set; }

    /// <summary>이 진단을 전역 <see cref="Current"/> 로 등록한다.</summary>
    public PerfMeter() => Current = this;

    /// <summary>그리기 세부 구간 하나의 소요 시간을 더한다.</summary>
    /// <param name="name">구간 이름</param>
    /// <param name="startTimestamp">구간을 시작한 <see cref="Stopwatch.GetTimestamp"/> 값</param>
    public void Section(string name, long startTimestamp)
    {
        _sections[name] = _sections.GetValueOrDefault(name) + Stopwatch.GetElapsedTime(startTimestamp).TotalMilliseconds;
    }

    /// <summary>갱신 한 번에 걸린 시간을 더한다.</summary>
    /// <param name="startTimestamp">갱신을 시작한 <see cref="Stopwatch.GetTimestamp"/> 값</param>
    public void EndUpdate(long startTimestamp)
    {
        double ms = Stopwatch.GetElapsedTime(startTimestamp).TotalMilliseconds;
        _updates++; _updateSum += ms; _updateMax = Math.Max(_updateMax, ms);
    }

    /// <summary>그리기 한 번에 걸린 시간을 더하고 보고 시각이 되면 한 줄을 출력한다.</summary>
    /// <param name="startTimestamp">그리기를 시작한 <see cref="Stopwatch.GetTimestamp"/> 값</param>
    public void EndDraw(long startTimestamp)
    {
        double ms = Stopwatch.GetElapsedTime(startTimestamp).TotalMilliseconds;
        _draws++; _drawSum += ms; _drawMax = Math.Max(_drawMax, ms);
        double now = _clock.Elapsed.TotalSeconds;
        if (now - _lastReport < ReportSeconds) return;
        double span = now - _lastReport;
        Fps = _draws / span;
        DrawMilliseconds = _draws == 0 ? 0 : _drawSum / _draws;
        // 문화권과 무관하게 점(.)을 소수점으로 쓰는 숫자 글자
        static string N(double value, string format) => value.ToString(format, CultureInfo.InvariantCulture);
        string line = $"[perf] t={N(now, "0.0")}s fps={N(_draws / span, "0.0")} updates/s={N(_updates / span, "0.0")} "
            + $"update avg={N(_updates == 0 ? 0 : _updateSum / _updates, "0.00")}ms max={N(_updateMax, "0.0")}ms "
            + $"draw avg={N(_draws == 0 ? 0 : _drawSum / _draws, "0.00")}ms max={N(_drawMax, "0.0")}ms";
        // 세부 구간의 프레임당 평균 시간(ms)을 덧붙인다.
        foreach ((string name, double sum) in _sections)
            line += $" {name}={N(_draws == 0 ? 0 : sum / _draws, "0.00")}";
        Console.WriteLine(line);
        _sections.Clear();
        _lastReport = now; _updates = _draws = 0; _updateSum = _updateMax = _drawSum = _drawMax = 0;
    }
}
