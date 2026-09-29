namespace Netstorm.Core.Simulation;

/// <summary>
/// 화면 갱신(가변 간격)과 게임 시뮬레이션(고정 간격)을 분리하는 누적기.
/// 원본은 오브젝트별 "다음 시각 = 현재 시각 + 간격" 타이머를 화면 루프(최대 75fps)로 돌렸다 (docs/videos/animation-timing.md).
/// 클론은 멀티플레이·리플레이 결정론을 위해 고정 틱으로 진행한다.
/// </summary>
public sealed class FixedTimestep
{
    /// <summary>기본 틱 빈도 = 원본 0.04초 계열 애니메이션의 실제 체감 속도 24Hz (영상 측정)</summary>
    public const int DefaultTicksPerSecond = 24;

    /// <summary>한 번의 화면 갱신에서 따라잡을 최대 틱 수 (창 이동·중단 뒤 폭주 방지)</summary>
    public const int DefaultMaxCatchUpTicks = 8;

    /// <summary>아직 틱으로 바꾸지 못한 누적 시간(초)</summary>
    private double _accumulator;

    /// <summary>초당 틱 수</summary>
    public int TicksPerSecond { get; }

    /// <summary>한 틱의 길이(초)</summary>
    public double TickSeconds => 1.0 / TicksPerSecond;

    /// <summary>한 번에 따라잡을 최대 틱 수</summary>
    public int MaxCatchUpTicks { get; }

    /// <summary>지금까지 진행한 전체 틱 수 (게임 시각 = Tick × TickSeconds)</summary>
    public long Tick { get; private set; }

    /// <summary>다음 틱까지 진행한 비율 (0 이상 1 미만, 화면 보간용)</summary>
    public double Alpha => _accumulator * TicksPerSecond;

    /// <summary>틱 빈도와 따라잡기 한도를 정한다</summary>
    /// <param name="ticksPerSecond">초당 틱 수 (1 이상)</param>
    /// <param name="maxCatchUpTicks">한 번에 따라잡을 최대 틱 수 (1 이상)</param>
    public FixedTimestep(int ticksPerSecond = DefaultTicksPerSecond, int maxCatchUpTicks = DefaultMaxCatchUpTicks)
    {
        TicksPerSecond = Math.Max(1, ticksPerSecond);
        MaxCatchUpTicks = Math.Max(1, maxCatchUpTicks);
    }

    /// <summary>
    /// 흐른 실제 시간을 더하고 이번에 실행할 틱 수를 돌려준다. 한도를 넘는 밀린 시간은 버린다(게임이 느려질 뿐 결과는 같다).
    /// </summary>
    /// <param name="elapsedSeconds">지난 화면 갱신 이후 시간(초). 음수는 0 으로 본다</param>
    public int Advance(double elapsedSeconds)
    {
        _accumulator += Math.Max(0, elapsedSeconds);
        int ticks = (int)Math.Floor(_accumulator * TicksPerSecond);
        if (ticks > MaxCatchUpTicks)
        {
            // 밀린 시간은 한도만큼만 처리하고 나머지는 버린다.
            ticks = MaxCatchUpTicks;
            _accumulator = 0;
        }
        else
        {
            _accumulator -= ticks * TickSeconds;
        }
        Tick += ticks;
        return ticks;
    }

    /// <summary>초 단위 간격을 틱 수로 바꾼다 (올림, 최소 1틱). 원본의 "다음 갱신 때 실행" 성질과 같다.</summary>
    /// <param name="seconds">간격(초)</param>
    public long TicksFor(double seconds) => Math.Max(1, (long)Math.Ceiling(seconds * TicksPerSecond - 1e-9));
}
