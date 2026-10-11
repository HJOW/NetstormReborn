namespace Netstorm.Core.Display;

/// <summary>
/// 10.78의 DWORD 벽시계·게임 시계와 중첩 정지 누계를 복원한다.
/// 밀리초 공급과 초기 원점/epoch는 호출자가 맡고, 부동소수 연산 순서는 원본 53비트 x87을 따른다.
/// </summary>
public sealed class OriginalGameClock
{
    /// <summary>원본 epoch 초를 DWORD 밀리초에 더하는 배율.</summary>
    private const uint MillisecondsPerSecond = 1000;

    /// <summary>원본의 double 상수와 같은 밀리초 나머지 배율. 전체 값을 1000으로 나누지 않는다.</summary>
    private const double SecondsPerMillisecond = 0.001;

    /// <summary>원본 초기화가 _time32에서 빼는 Unix 초 기준값.</summary>
    public const uint EpochReferenceSeconds = 0x32c85780;

    /// <summary>초기 timeGetTime 값. DWORD 뺄셈의 원점으로 사용한다.</summary>
    public uint OriginMilliseconds { get; }

    /// <summary>초기 Unix 초에서 기준값을 뺀 원본 epoch 오프셋.</summary>
    public uint EpochSeconds { get; }

    /// <summary>게임 시계에만 먼저 빼는 DWORD 밀리초 보정값.</summary>
    public uint GameAdjustmentMilliseconds { get; }

    /// <summary>중첩 정지 횟수. 대응하는 Resume이 모두 실행되어야 게임 시계가 재개된다.</summary>
    public int PauseDepth { get; private set; }

    /// <summary>최초 Pause 직전 게임 시각의 저장 비트. 중첩 Pause에서 덮어쓰지 않는다.</summary>
    public double HeldGameSeconds { get; private set; }

    /// <summary>마지막 중첩 Resume에서만 갱신되는 정지 시간 누계.</summary>
    public double PausedTotalSeconds { get; private set; }

    /// <summary>유효한 정지 깊이가 양수인지 확인한다. 원본 signed 비교와 같다.</summary>
    public bool IsPaused => PauseDepth > 0;

    /// <summary>OS 초기화와 분리하여 이미 캡처한 원점·epoch·게임 보정값으로 시계를 만든다.</summary>
    public OriginalGameClock(uint originMilliseconds, uint epochSeconds = 0, uint gameAdjustmentMilliseconds = 0)
    {
        OriginMilliseconds = originMilliseconds;
        EpochSeconds = epochSeconds;
        GameAdjustmentMilliseconds = gameAdjustmentMilliseconds;
    }

    /// <summary>보정/정지 누계와 무관하게 현재 DWORD 벽시각을 double 초로 변환한다.</summary>
    public double WallSeconds(uint milliseconds) => ConvertSeconds(milliseconds, 0);

    /// <summary>정지 중에는 저장 시각을 반환하고 실행 중에는 보정과 정지 누계를 뺀다.</summary>
    public double GameSeconds(uint milliseconds) => IsPaused
        ? HeldGameSeconds : ConvertSeconds(milliseconds, GameAdjustmentMilliseconds) - PausedTotalSeconds;

    /// <summary>첫 정지만 현재 게임 시각을 저장한다. 잘못된 깊이 넘침은 상태 변경 전에 거부한다.</summary>
    public void Pause(uint milliseconds)
    {
        if (PauseDepth == int.MaxValue) throw new InvalidOperationException("게임 시계 정지 깊이가 너무 큽니다.");
        if (PauseDepth == 0) HeldGameSeconds = GameSeconds(milliseconds);
        PauseDepth++;
    }

    /// <summary>짝 없는 재개는 무시하며 마지막 재개에서 원본 순서로 정지 누계를 더한다.</summary>
    public void Resume(uint milliseconds)
    {
        if (PauseDepth == 0) return;
        PauseDepth--;
        if (PauseDepth == 0)
            PausedTotalSeconds = (GameSeconds(milliseconds) - HeldGameSeconds) + PausedTotalSeconds;
    }

    /// <summary>DWORD 감김을 먼저 적용한 뒤 몫과 나머지를 별도로 double에 올려 계산한다.</summary>
    private double ConvertSeconds(uint milliseconds, uint adjustment)
    {
        uint elapsed = unchecked(milliseconds - OriginMilliseconds - adjustment + EpochSeconds * MillisecondsPerSecond);
        double fraction = (elapsed % MillisecondsPerSecond) * SecondsPerMillisecond;
        return (elapsed / MillisecondsPerSecond) + fraction;
    }
}
