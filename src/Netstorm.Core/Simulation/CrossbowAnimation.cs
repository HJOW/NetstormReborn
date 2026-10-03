namespace Netstorm.Core.Simulation;

/// <summary>원본 Gunanim.cpp의 석궁 조준 각도표와 회전·장전·발사 그림을 논리 틱으로 진행한다.</summary>
public static class CrossbowAnimation
{
    /// <summary>원본 VA 0x540940의 북쪽 기준 시계 방향 각도. 그림 간 각도는 균등하지 않다.</summary>
    private static readonly double[] Angles =
        [0, 21, 35, 52, 71, 86, 97, 115, 121, 148, 180, 205, 237, 256, 272, 282, 291, 314, 328, 345];

    /// <summary>원본 FUN_00468820의 조준 그림 간격(초).</summary>
    public const double TurnSeconds = 0.1;

    /// <summary>원본 FUN_00468320의 장전 그림 간격(초).</summary>
    public const double LoadSeconds = 0.2;

    /// <summary>한 조준 각도에 속하는 장전 그림 수.</summary>
    private const int FramesPerAngle = 5;

    /// <summary>원본 FUN_004676c0처럼 가장 가까운 실제 조준 그림을 찾고 북쪽 경계를 연결한다.</summary>
    public static int Orientation(double dx, double dy)
    {
        double angle = (Math.Atan2(dx, -dy) * 180 / Math.PI + 360) % 360;
        int result = 0;
        double closest = double.PositiveInfinity;
        // 실제 각도표를 검색해 균등한 18도 간격의 탄 그림과 본체 조준을 구분한다.
        for (int index = 0; index < Angles.Length; index++)
        {
            double distance = Math.Abs(Angles[index] - angle);
            if (distance >= closest) continue;
            closest = distance;
            result = index;
        }
        return result == Angles.Length - 1 && 360 - angle < closest ? 0 : result;
    }

    /// <summary>원본 FUN_00467610처럼 저장 그림의 실제 각도로 설치 방위를 복원한다.</summary>
    public static int SavedDirection(int frame)
    {
        double angle = Angles[Math.Clamp(frame / FramesPerAngle, 0, Angles.Length - 1)];
        return angle < 45 || angle >= 315 ? 0 : angle < 135 ? 1 : angle < 225 ? 2 : 3;
    }

    /// <summary>배치 방위에서 가장 가까운 원본 조준 그림을 고른다.</summary>
    public static int PlacementFrame(int direction) => Orientation(
        direction == 1 ? 1 : direction == 3 ? -1 : 0,
        direction == 0 ? -1 : direction == 2 ? 1 : 0) * FramesPerAngle;

    /// <summary>조준과 장전을 진행하며 실제 탄을 놓는 그림에서만 true를 돌려준다. 목표 상실은 발사를 취소한다.</summary>
    public static bool Advance(GameEntity entity, int? orientation, bool mayFire, long tick, int ticksPerSecond)
    {
        if (!orientation.HasValue)
        {
            entity.CrossbowFiring = false;
            return false;
        }
        if (tick < entity.NextCrossbowAnimationTick) return false;
        int current = entity.CrossbowFrame / FramesPerAngle;
        if (current != orientation.Value)
        {
            entity.CrossbowFiring = false;
            int clockwise = (orientation.Value - current + Angles.Length) % Angles.Length;
            int anticlockwise = (current - orientation.Value + Angles.Length) % Angles.Length;
            int step = clockwise < anticlockwise ? 1 : -1;
            entity.CrossbowFrame = (current + step + Angles.Length) % Angles.Length * FramesPerAngle;
            entity.NextCrossbowAnimationTick = tick + Math.Max(1, (long)Math.Ceiling(TurnSeconds * ticksPerSecond));
            return false;
        }
        if (!entity.CrossbowFiring && !mayFire) return false;
        entity.CrossbowFiring = true;
        if (entity.CrossbowFrame % FramesPerAngle != 0)
        {
            entity.CrossbowFrame--;
            entity.NextCrossbowAnimationTick = tick + Math.Max(1, (long)Math.Ceiling(LoadSeconds * ticksPerSecond));
            return false;
        }
        entity.CrossbowFrame += FramesPerAngle - 1;
        entity.CrossbowFiring = false;
        return true;
    }
}
