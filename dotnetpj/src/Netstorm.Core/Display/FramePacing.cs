namespace Netstorm.Core.Display;

/// <summary>
/// 화면 루프의 속도 규칙 (원본 <c>Netstorm.exe</c> 의 메인 루프 분석, docs/exe/main-loop.md).
/// <para>
/// 원본은 고정 틱이 아니라 한 바퀴마다 "시각 고정 → 입력·명령 → 갱신 → 그리기"를 한 번씩 돌고, 그리기 직전에
/// <c>timeGetTime()</c>(밀리초 눈금)으로 "직전 그리기 뒤 <c>1 / maxFPS</c> 초가 지났는지"를 반복해서 확인하며 기다린다
/// (<c>FUN_00436450</c>). 시계가 1ms 단위라서 실제 간격은 <c>1000 / maxFPS</c> 를 밀리초로 올림한 값이 된다.
/// 기본 <c>maxFPS = 75</c> 이면 14ms, 초당 약 71.4바퀴다. 영상에서 잰 0.04초 타이머의 실제 주기 41.9ms(= 14ms × 3)와 맞는다.
/// </para>
/// <para>
/// AGENTS.md 의 "기존 게임 수준의 프레임으로 먼저 만든다"에 따라 클론의 기본 화면 루프도 이 간격으로 돈다.
/// 규칙(<c>BattleSession</c>)은 24Hz 고정 틱 그대로이고, 화면은 틱 사이를 보간해 그린다.
/// </para>
/// </summary>
public static class FramePacing
{
    /// <summary>원본 설정 <c>maxFPS</c> 의 코드 기본값 (exe <c>DAT_005318d8 = 0x4b</c>, 두 판본 공통)</summary>
    public const int OriginalMaxFps = 75;

    /// <summary>상한 없음을 뜻하는 값. 원본도 <c>maxFPS</c> 가 0 이하면 기다리지 않는다.</summary>
    public const int Unlimited = 0;

    /// <summary>허용하는 가장 낮은 상한 (이보다 낮으면 조작이 어렵다)</summary>
    public const int MinimumMaxFps = 15;

    /// <summary>허용하는 가장 높은 상한 (밀리초 눈금에서 간격이 1ms 가 되는 값)</summary>
    public const int MaximumMaxFps = 1000;

    /// <summary>1초의 밀리초 수</summary>
    private const int MillisecondsPerSecond = 1000;

    /// <summary>
    /// 자동 검사 스크립트(<c>--ui-script-file</c>)의 <c>wait N</c> 과 <c>--screenshot-frames N</c> 의 단위: 1/60초.
    /// 화면 루프 속도를 바꿔도 같은 스크립트가 같은 게임 시간만큼 기다리도록 프레임 수가 아니라 시간으로 센다.
    /// </summary>
    public const int ScriptFramesPerSecond = 60;

    /// <summary>원본 기본 설정에서의 실제 루프 속도: 1000 / 14 ≈ 71.43 (초당 바퀴 수)</summary>
    public static readonly double OriginalFramesPerSecond = FramesPerSecond(OriginalMaxFps);

    /// <summary>설정 값을 허용 범위로 맞춘다. 0 이하는 상한 없음이다.</summary>
    /// <param name="maxFps">초당 바퀴 수 상한</param>
    public static int Normalize(int maxFps) =>
        maxFps <= 0 ? Unlimited : Math.Clamp(maxFps, MinimumMaxFps, MaximumMaxFps);

    /// <summary>
    /// 원본의 제한 방식으로 정해지는 한 바퀴의 길이(밀리초): <c>1000 / maxFPS</c> 를 올림한 값. 상한이 없으면 0 이다.
    /// </summary>
    /// <param name="maxFps">초당 바퀴 수 상한 (정규화하지 않은 값도 받는다)</param>
    public static int FrameMilliseconds(int maxFps)
    {
        int normalized = Normalize(maxFps);
        return normalized == Unlimited ? 0 : (MillisecondsPerSecond + normalized - 1) / normalized;
    }

    /// <summary>한 바퀴의 길이. 상한이 없으면 null 이다.</summary>
    /// <param name="maxFps">초당 바퀴 수 상한</param>
    public static TimeSpan? FrameInterval(int maxFps)
    {
        int milliseconds = FrameMilliseconds(maxFps);
        return milliseconds == 0 ? null : TimeSpan.FromMilliseconds(milliseconds);
    }

    /// <summary>그 상한에서 실제로 도는 초당 바퀴 수. 상한이 없으면 양의 무한대다.</summary>
    /// <param name="maxFps">초당 바퀴 수 상한</param>
    public static double FramesPerSecond(int maxFps)
    {
        int milliseconds = FrameMilliseconds(maxFps);
        return milliseconds == 0 ? double.PositiveInfinity : (double)MillisecondsPerSecond / milliseconds;
    }
}
