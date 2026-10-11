using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Netstorm.Game;

/// <summary>
/// Windows에서는 원본과 같은 timeGetTime 밀리초 시계를 읽고 DWORD 감김을 펼친다.
/// 다른 플랫폼은 단조 Stopwatch를 밀리초로 양자화한다. 게임 스레드에서만 사용한다.
/// </summary>
internal sealed class FrameWallClock
{
    /// <summary>비 Windows 환경의 단조 시계. Linux 전체 게임 검증은 후속이다.</summary>
    private readonly Stopwatch _fallback = Stopwatch.StartNew();

    /// <summary>직전 Windows DWORD 시각. 읽기 사이에는 한 번 이하의 49.7일 감김을 전제로 한다.</summary>
    private uint _previous;

    /// <summary>DWORD 시각 감김을 펼친 상위 밀리초 값.</summary>
    private long _epoch;

    /// <summary>원본이 쓰는 WINMM 시계. 별도로 시스템 타이머 해상도를 바꾸지 않는다.</summary>
    [DllImport("winmm.dll", EntryPoint = "timeGetTime", ExactSpelling = true)]
    private static extern uint TimeGetTime();

    /// <summary>실제 단조 밀리초 시각을 읽는다. 시작 시각의 큰 값은 루프 시계가 첫 갱신에서 제외한다.</summary>
    public long ReadMilliseconds()
    {
        if (!OperatingSystem.IsWindows()) return _fallback.ElapsedMilliseconds;
        uint now = TimeGetTime();
        if (now < _previous) _epoch += 1L << 32;
        _previous = now;
        return _epoch + now;
    }
}
