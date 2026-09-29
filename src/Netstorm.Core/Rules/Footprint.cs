using Netstorm.Assets;

namespace Netstorm.Core.Rules;

/// <summary>
/// 오브젝트가 차지하는 칸 사각형. 기준점(저장·배치 좌표)은 발자국의 **오른쪽 아래 칸**이다.
/// 원본 FUN_0049ae80: 사각형 = (x − (foot_x − 1), y − (foot_y − 1)) ~ (x, y).
/// </summary>
/// <param name="AnchorX">기준점 칸 x (오른쪽 끝 칸)</param>
/// <param name="AnchorY">기준점 칸 y (아래 끝 칸)</param>
/// <param name="Width">가로 칸 수 (.type foot_x)</param>
/// <param name="Height">세로 칸 수 (.type foot_y)</param>
public readonly record struct Footprint(int AnchorX, int AnchorY, int Width, int Height)
{
    /// <summary>왼쪽 끝 칸 x</summary>
    public int Left => AnchorX - (Width - 1);

    /// <summary>위쪽 끝 칸 y</summary>
    public int Top => AnchorY - (Height - 1);

    /// <summary>사각형 중심 x (칸 좌표, 원본 (x0 + x1) × 0.5)</summary>
    public double CenterX => (Left + AnchorX) * 0.5;

    /// <summary>사각형 중심 y (칸 좌표)</summary>
    public double CenterY => (Top + AnchorY) * 0.5;

    /// <summary>칸이 사각형 안인지</summary>
    /// <param name="x">칸 x</param>
    /// <param name="y">칸 y</param>
    public bool Contains(int x, int y) => x >= Left && x <= AnchorX && y >= Top && y <= AnchorY;

    /// <summary>두 사각형이 한 칸이라도 겹치는지</summary>
    /// <param name="other">다른 발자국</param>
    public bool Overlaps(Footprint other) =>
        Left <= other.AnchorX && other.Left <= AnchorX && Top <= other.AnchorY && other.Top <= AnchorY;

    /// <summary>사각형이 차지하는 모든 칸 (y·x 순서)</summary>
    public IEnumerable<(int X, int Y)> Cells()
    {
        // 위쪽 행부터 왼쪽 칸 순서로 나열한다.
        for (int y = Top; y <= AnchorY; y++)
        {
            // 한 행의 칸을 왼쪽부터 나열한다.
            for (int x = Left; x <= AnchorX; x++)
            {
                yield return (x, y);
            }
        }
    }

    /// <summary>.type 의 foot_x·foot_y 로 발자국을 만든다. 값이 없거나 1 미만이면 1칸으로 본다.</summary>
    /// <param name="definition">.type 정의</param>
    /// <param name="anchorX">기준점 칸 x</param>
    /// <param name="anchorY">기준점 칸 y</param>
    public static Footprint ForType(TypeDefinition definition, int anchorX, int anchorY) =>
        new(anchorX, anchorY, Math.Max(1, definition.GetInt("foot_x") ?? 1), Math.Max(1, definition.GetInt("foot_y") ?? 1));
}
