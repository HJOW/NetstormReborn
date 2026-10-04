using Netstorm.Assets;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Rules;

/// <summary>원본 Range.cpp의 표시 모양: 원형·직선·Crossbow의 60도 부채꼴.</summary>
public enum RangeShape
{
    /// <summary>원형 사거리.</summary>
    Circle,
    /// <summary>한 방향 또는 네 방향 직선 사거리.</summary>
    Ray,
    /// <summary>V 경계와 원호로 표시하는 부채꼴 사거리.</summary>
    Sector,
}

/// <summary>표시할 사거리(칸), 모양, 북·동·남·서 방향(0~3), range 타입의 그림 묶음.</summary>
public sealed record RangeDisplay(double Distance, RangeShape Shape, int Direction, char FrameSide);

/// <summary>발자국 중심에서 반짝임까지의 칸 단위 변위와 그림 위상을 구분하는 번호.</summary>
public readonly record struct RangePoint(double X, double Y, int Index);

/// <summary>원본 00474240·00474520·00474c30·00474ed0에서 확인한 사거리 표시 규칙. 공격 판정과 분리한다.</summary>
public static class RangeIndicator
{
    /// <summary>직선 반짝임 수를 구할 기준 간격(칸). 실제 간격은 사거리 ÷ 정수 반짝임 수다.</summary>
    public const double Spacing = 5;

    /// <summary>직선 반짝임 이동 및 원형 표시가 커지는 속도(칸/초).</summary>
    public const double GrowSpeed = 15;

    /// <summary>선택을 풀었을 때 원형 표시가 줄어드는 속도(칸/초).</summary>
    public const double ShrinkSpeed = 30;

    /// <summary>원형·부채꼴에 사용하는 반짝임 수.</summary>
    public const int CirclePointCount = 8;

    /// <summary>Crossbow 사거리 부채꼴의 중심에서 한쪽 경계까지의 각도(도). 원본 005409c0 = 30.</summary>
    public const double SectorHalfAngle = EmplacementDirection.CrossbowHalfAngle;

    /// <summary>선택·배치 타입의 표시 규격. 공격 그룹과 bomb 플래그만 대상으로 하고 비공격 건물은 제외한다.</summary>
    public static RangeDisplay? ForType(TypeInfo type, int direction = 0)
    {
        string group = type.Definition.GetString("group")?.ToLowerInvariant() ?? "";
        bool bomb = type.Definition.HasFlag("bomb");
        double distance = type.Definition.GetDouble("range") ?? 0;
        if (distance <= 0 || !(bomb || group is "archer" or "cannon" or "fence" or "aviary")) return null;
        direction = ((direction % 4) + 4) % 4;
        if (group == "cannon" && CannonAnimation.IsFixed(type))
            return new(distance, RangeShape.Ray, direction, 'A');
        // 썬 캐논·울타리는 네 방향을 동시에 표시하며 개별 조준 방향에 따라 표시가 회전하지 않는다.
        if (group is "cannon" or "fence") return new(distance, RangeShape.Ray, -1, 'A');
        if (type.Name.Equals("windArcher", StringComparison.OrdinalIgnoreCase))
            return new(distance, RangeShape.Sector, direction, 'A');
        return new(distance, RangeShape.Circle, -1, bomb ? 'B' : 'A');
    }

    /// <summary>게임 시각으로 반짝임 위치를 계산한다. 표시 반지름은 생성·선택 해제 연출을 위해 호출자가 전달한다.</summary>
    public static IReadOnlyList<RangePoint> Points(RangeDisplay display, double age, double radius, double clock)
    {
        var points = new List<RangePoint>();
        if (display.Distance <= 0 || radius <= 0 || age <= 0) return points;
        if (display.Shape == RangeShape.Ray)
        {
            int count = Math.Max(1, (int)(display.Distance / Spacing));
            double step = display.Distance / count;
            double phase = age * GrowSpeed % step;
            // 사용할 수 있는 북·동·남·서 광선마다 같은 속도로 반짝임을 바깥으로 흘린다.
            for (int direction = 0; direction < 4; direction++)
            {
                if (display.Direction >= 0 && display.Direction != direction) continue;
                double angle = direction * Math.PI / 2;
                // 원본은 처음 열릴 때 아직 이동 거리에 도달하지 못한 바깥 반짝임을 숨긴다.
                for (int index = 0; index < count; index++)
                {
                    double start = index * step;
                    if (start >= age * GrowSpeed) break;
                    double distance = start + phase;
                    points.Add(new(Math.Sin(angle) * distance, -Math.Cos(angle) * distance, direction * count + index));
                }
            }
            return points;
        }
        radius = Math.Clamp(radius, 0, display.Distance);
        // 최대 반지름에 도달한 뒤에는 원본의 전역 시각 소수부 × π만큼 원형 반짝임을 회전시킨다.
        double phaseAngle = radius >= display.Distance ? (clock - Math.Floor(clock)) * Math.PI : 0;
        // 원형의 여덟 점을 부채꼴 안에서는 원호에, 바깥에서는 가까운 V 경계에 접어 표시한다.
        for (int index = 0; index < CirclePointCount; index++)
        {
            double angle = 90 - (index * 2 * Math.PI / CirclePointCount + phaseAngle) * 180 / Math.PI;
            double distance = radius;
            if (display.Shape == RangeShape.Sector)
            {
                double relative = SignedAngle(angle - display.Direction * 90);
                if (Math.Abs(relative) > SectorHalfAngle)
                {
                    distance *= 1 - (Math.Abs(relative) - SectorHalfAngle) / (180 - SectorHalfAngle);
                    angle = display.Direction * 90 + Math.CopySign(SectorHalfAngle, relative);
                }
            }
            double radians = angle * Math.PI / 180;
            points.Add(new(Math.Sin(radians) * distance, -Math.Cos(radians) * distance, index));
        }
        return points;
    }

    /// <summary>각도를 -180~180도로 정규화해 북쪽 경계를 넘는 부채꼴도 같은 방법으로 처리한다.</summary>
    private static double SignedAngle(double angle) => ((angle + 180) % 360 + 360) % 360 - 180;
}
