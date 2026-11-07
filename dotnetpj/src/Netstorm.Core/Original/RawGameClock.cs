// 프레임의 게임 시각·벽 시각·시간차를 고정하고 중첩 정지를 계산하는 시계를 공유한다.
namespace Netstorm.Core.Original;

// 게임 시계 모듈을 담는 네임스페이스다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.

// 프레임의 게임 시각과 벽 시각 및 시간차다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.
public struct FrameTime
{
    /// <summary>프레임 안에서 고정된 게임 시각이다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    public double Game;

    /// <summary>정지 중에도 증가하는 벽시계다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    public double Wall;

    /// <summary>직전 프레임과의 게임 시간차다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    public double Delta;

    /// <summary>프레임 카운터다. 새 빌드는 폭을 늘린다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    public ulong Number;
}

// 게임 시각과 프레임 간격을 계산하고 중첩 정지 시간을 제외하는 시계다. OS 시계는 외부에서 공급한다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.
public sealed class GameClock
{
    /// <summary>원본 밀리초 나머지를 초로 환산하는 배율이다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    private const double MillisecondSeconds = 0.001;

    /// <summary>시작 밀리초 기준이다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    private readonly uint _origin;

    /// <summary>실제 장기 소유자에서 빌린 보정 DWORD를 읽는 장치다. null이면 기존 0 보정을 유지한다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    private Func<uint>? _synchronizationTicks;

    /// <summary>외부 공유 시계의 주소 대여가 끝까지 유지되도록 실제 장기 소유자를 보유한다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    private object? _synchronizationOwner;

    /// <summary>중첩된 시계 정지 요청의 수다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    private uint _pauseDepth;

    /// <summary>정지 시점에 저장한 게임 시각(초)이다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    private double _pausedGame;

    /// <summary>재개 때 게임 시각에서 제외하는 누적 정지 시간(초)이다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    private double _pausedTotal;

    /// <summary>현재 프레임이다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    private FrameTime _frame;

    /// <summary>원본 시작 밀리초 기준을 기록한다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    /// <param name="originMilliseconds">시작 밀리초다.</param>
    public GameClock(uint originMilliseconds = 0u)
    {
        _origin = originMilliseconds;
    }

    /// <summary>원본 나눗셈·나머지 순서대로 초를 계산한다. 시작 밀리초와의 차이는 원본 부호 없는 32비트 연산이다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    /// <param name="milliseconds">현재 밀리초다.</param>
    /// <returns>벽 시각(초)이다.</returns>
    public double WallSeconds(uint milliseconds)
    {
        uint elapsed = unchecked(milliseconds - _origin);
        return (double)(elapsed / 1000u) + (double)(elapsed % 1000u) * MillisecondSeconds;
    }

    /// <summary>정지 중이면 저장값, 아니면 누적 정지 시간을 뺀 값이다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    /// <param name="milliseconds">현재 밀리초다.</param>
    /// <returns>게임 시각(초)이다.</returns>
    public double GameSeconds(uint milliseconds)
    {
        if (_pauseDepth != 0u)
        {
            return _pausedGame;
        }

        // 원문은 부호 있는 보정 비트도 부호 없는 밀리초 감김으로 감산한다
        uint elapsed = unchecked((milliseconds - _origin) - (_synchronizationTicks is not null ? _synchronizationTicks() : 0u));
        return ((double)(elapsed / 1000u) + (double)(elapsed % 1000u) * MillisecondSeconds) - _pausedTotal;
    }

    /// <summary>같은 보정 DWORD와 실제 장기 소유자를 빌린다. 독립 호출은 caller가 주소 수명을 보장하며 값 자체는 변경하지 않는다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    /// <param name="ticks">현재 보정 DWORD를 읽는 장치다.</param>
    /// <param name="owner">대여가 끝까지 유지되도록 보유할 실제 장기 소유자다.</param>
    public void BindSynchronizationTicks(Func<uint> ticks, object? owner = null)
    {
        ArgumentNullException.ThrowIfNull(ticks);
        _synchronizationOwner = owner;
        _synchronizationTicks = ticks;
    }

    /// <summary>첫 정지에서만 게임 시각을 저장한다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    /// <param name="milliseconds">현재 밀리초다.</param>
    public void Pause(uint milliseconds)
    {
        if (_pauseDepth == uint.MaxValue)
        {
            throw new OverflowException("게임 시계 정지 깊이 넘침");
        }

        if (_pauseDepth == 0u)
        {
            _pausedGame = GameSeconds(milliseconds);
        }

        ++_pauseDepth;
    }

    /// <summary>마지막 정지가 풀릴 때만 정지 시간을 누적한다. 정지되지 않았으면 아무 작업도 하지 않는다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    /// <param name="milliseconds">현재 밀리초다.</param>
    public void Resume(uint milliseconds)
    {
        if (_pauseDepth != 0u && --_pauseDepth == 0u)
        {
            _pausedTotal = (GameSeconds(milliseconds) - _pausedGame) + _pausedTotal;
        }
    }

    /// <summary>누적 정지 시간 double만 0으로 쓴다. 원점·정지 깊이·저장 게임 시각·현재 프레임은 변경하지 않는다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    public void Reset()
    {
        _pausedTotal = 0.0;
    }

    /// <summary>중첩 정지 카운터가 양수인지 확인한다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    /// <returns>정지 중이면 참이다.</returns>
    public bool IsPaused() => _pauseDepth != 0u;

    /// <summary>갱신 전체가 같은 시각을 사용하도록 한 번 계산해 스냅샷을 반환한다. 시각 고정·delta·프레임 증가 부분이다. 2026-11-07 F2d 게임 시계 이식에서 추가됐다.</summary>
    /// <param name="milliseconds">현재 밀리초다.</param>
    /// <returns>고정한 프레임 시각이다.</returns>
    public FrameTime Capture(uint milliseconds)
    {
        double game = GameSeconds(milliseconds);
        _frame.Delta = game - _frame.Game;
        _frame.Game = game;
        _frame.Wall = WallSeconds(milliseconds);
        ++_frame.Number;
        return _frame;
    }
}
