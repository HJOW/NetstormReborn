using System.Runtime.InteropServices;

namespace Netstorm.AnalyzeManager;

/// <summary>캡처 동안만 Windows 대기 타이머 정밀도를 높이고 끝나면 같은 요청을 해제한다.</summary>
public sealed class FrameTimerResolution : IDisposable
{
    /// <summary>30/60FPS의 소수 밀리초 주기를 기다리기 위한 최소 타이머 해상도.</summary>
    private const uint PeriodMs = 1;
    private bool _active;

    /// <summary>Windows의 거친 기본 대기 주기가 60FPS 샘플링을 제한하지 않도록 요청한다.</summary>
    public FrameTimerResolution() => _active = TimeBeginPeriod(PeriodMs) == 0;

    /// <summary>성공한 정밀도 요청을 한 번만 해제한다.</summary>
    public void Dispose()
    {
        if (!_active) return;
        _active = false;
        TimeEndPeriod(PeriodMs);
    }

    /// <summary>현재 프로세스의 타이머 해상도를 요청한다.</summary>
    [DllImport("winmm.dll", EntryPoint = "timeBeginPeriod")] private static extern uint TimeBeginPeriod(uint period);
    /// <summary>동일한 밀리초 값으로 타이머 해상도 요청을 해제한다.</summary>
    [DllImport("winmm.dll", EntryPoint = "timeEndPeriod")] private static extern uint TimeEndPeriod(uint period);
}
