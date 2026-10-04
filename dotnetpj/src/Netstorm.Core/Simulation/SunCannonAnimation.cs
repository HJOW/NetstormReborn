namespace Netstorm.Core.Simulation;

/// <summary>원본 0x540550 전이표로 태양 캐논의 접기·회전·펼치기를 진행한다. 팬게임의 표와 원본 바이너리를 대조했다.</summary>
public static class SunCannonAnimation
{
    /// <summary>원본 회전 프레임 간격(초). 발사 그림의 0.07초와 별개다.</summary>
    public const double TurnSeconds = 0.2;

    /// <summary>원본 전이표에서 현재 자세가 목적 자세임을 표시하는 비트.</summary>
    private const int Ready = 0x8000;

    /// <summary>북·동·남·서 조준과 목표 없음(접기)의 전이표. 행마다 현재 그림 번호로 다음 그림을 찾는다.</summary>
    private static readonly int[][] Transitions =
    [
        [4, Ready, 12, 19, 5, 6, 7, 8, 9, 10, 11, 1, 13, 14, 15, 16, 20, 16, 17, 18, 21, 8, 8, 22],
        [4, 11, 12, Ready, 5, 6, 7, 8, 22, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 3, 16, 20, 23, 16],
        [Ready, 11, 12, 19, 0, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 20, 16, 17, 18, 21, 8, 8, 22],
        [4, 11, Ready, 19, 5, 6, 7, 8, 22, 8, 9, 10, 2, 12, 13, 14, 15, 16, 17, 18, 16, 20, 23, 16],
        [4, 11, 12, 19, 5, 6, 7, 8, Ready, 8, 9, 10, 13, 14, 15, 16, Ready, 16, 17, 18, Ready, 20, 23, 16],
    ];

    /// <summary>현재 그림을 유지하며 한 틱 갱신한다. 목표 변경은 진행 중인 자세에서 이어지고 재장전 중에도 회전한다.</summary>
    public static bool Advance(GameEntity entity, int? direction, long tick, int ticksPerSecond)
    {
        if (tick < entity.NextSunAnimationTick) return false;
        if (entity.SunCannonFrame >= 24)
            entity.SunCannonFrame -= 24;
        int row = direction ?? 4;
        int next = Transitions[row][entity.SunCannonFrame];
        if (next == Ready) return direction.HasValue;
        entity.SunCannonFrame = next;
        entity.NextSunAnimationTick = tick + Math.Max(1, (long)Math.Ceiling(TurnSeconds * ticksPerSecond));
        return false;
    }

    /// <summary>실제 발사와 같은 틱에 발사 자세로 바꾸고 원본의 짧은 발사 그림을 유지한다.</summary>
    public static void Fire(GameEntity entity, long tick, int ticksPerSecond)
    {
        entity.SunCannonFrame += 24;
        entity.NextSunAnimationTick = tick + Math.Max(1, (long)Math.Ceiling(CannonAnimation.SunFireSeconds * ticksPerSecond));
    }
}
