using Netstorm.Assets;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Rules;

/// <summary>설치 전에 정하는 고정 캐논·Crossbow의 방위와 Crossbow의 사격 부채꼴.</summary>
public static class EmplacementDirection
{
    /// <summary>원본 0046aa50의 Crossbow 사격 중심에서 한쪽 경계까지의 각도(도).</summary>
    public const double CrossbowHalfAngle = 30;

    /// <summary>sin·cos와 벡터 정규화의 반올림 때문에 정확한 경계각이 거부되는 것을 막는 내적 오차 허용값.</summary>
    private const double DirectionTolerance = 1e-12;

    /// <summary>배치 전 우클릭으로 방향을 정하는 타입. 썬 캐논은 설치 후 목표를 따라 회전하므로 제외한다.</summary>
    public static bool RequiresChoice(TypeInfo type) => CannonAnimation.IsFixed(type)
        || type.Name.Equals("windArcher", StringComparison.OrdinalIgnoreCase);

    /// <summary>저장된 Crossbow의 조준 그림(P 계열 포함)이 속한 L/M/N/O 방위 묶음을 찾는다.</summary>
    public static int SavedDirection(TypeInfo type, int? savedFrame)
    {
        int frame = savedFrame ?? type.Definition.Frames.DefaultFrame;
        if ((uint)frame >= (uint)type.Definition.Frames.Codes.Count) return 0;
        // 직전 기본 방향 글자까지 거슬러 올라가 같은 조준 묶음의 방위를 읽는다.
        for (int index = frame; index >= 0; index--)
        {
            char side = type.Definition.Frames.Codes[index].Side;
            if (side is >= 'L' and <= 'O') return side - 'L';
            if (side == 'A') break;
        }
        return 0;
    }

    /// <summary>북·동·남·서 방위의 Crossbow가 목표 중심을 60도 부채꼴 안에서 볼 수 있는지 판정한다.</summary>
    public static bool InsideCrossbowSector(int direction, double dx, double dy)
    {
        double distance = Math.Sqrt(dx * dx + dy * dy);
        if (distance <= 0) return false;
        double forward = ((direction % 4 + 4) % 4) switch { 0 => -dy, 1 => dx, 2 => dy, _ => -dx };
        return forward / distance + DirectionTolerance >= Math.Cos(CrossbowHalfAngle * Math.PI / 180);
    }
}
