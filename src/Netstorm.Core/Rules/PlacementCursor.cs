using Netstorm.Assets;

namespace Netstorm.Core.Rules;

/// <summary>
/// 커서가 가리키는 칸과, 들고 있는 유닛·건물이 놓일 칸의 규칙.
/// 화면에서 칸 (cx, cy) 는 월드 픽셀 (16·(cx−1), 16·cx] × (11·(cy−1), 11·cy] 를 차지한다 —
/// 지면 타일과 스프라이트가 기준점 (16·cx, 11·cy) 의 왼쪽 위로 그려지기 때문이다.
/// 2026-10-03 TEST01 녹화(입력 기록의 커서 좌표와 흰 발자국 사각형 대조)에서, 들고 있는 유닛의 발자국은
/// 오른쪽 아래 칸의 열이 커서 칸의 열과 같고 행은 커서 칸보다 (1 + 타입의 height) 만큼 아래였다:
/// Crossbow·Ice Cannon(height 없음)은 커서가 3×3 발자국의 오른쪽 열·가운데 행에, Thunder Cannon(height = 2)은
/// 커서가 발자국 위쪽 한 칸 밖(오른쪽 열)에 있었고, 캐논을 회전해도 사각형은 움직이지 않았다.
/// </summary>
public static class PlacementCursor
{
    /// <summary>칸 가로 픽셀</summary>
    public const int CellWidth = 16;

    /// <summary>칸 세로 픽셀</summary>
    public const int CellHeight = 11;

    /// <summary>커서의 월드 픽셀이 놓인 칸 (화면에 보이는 그 칸).</summary>
    /// <param name="worldX">커서의 월드 x 픽셀</param>
    /// <param name="worldY">커서의 월드 y 픽셀</param>
    public static (int X, int Y) CellUnder(double worldX, double worldY) =>
        ((int)Math.Ceiling(worldX / CellWidth), (int)Math.Ceiling(worldY / CellHeight));

    /// <summary>
    /// 들고 있는 타입의 발자국 기준 칸(오른쪽 아래 칸). 열은 커서 칸, 행은 커서 칸 + 1 + height 다.
    /// height 는 .type 의 속성으로 사제·골렘 1, Thunder Cannon·Sun 방벽 2, 나머지 방벽 3 이며 없으면 0 이다.
    /// 키가 큰 그림일수록 커서가 발밑이 아니라 몸통 쪽을 잡게 된다.
    /// </summary>
    /// <param name="worldX">커서의 월드 x 픽셀</param>
    /// <param name="worldY">커서의 월드 y 픽셀</param>
    /// <param name="type">들고 있는 타입</param>
    public static (int X, int Y) AnchorCell(double worldX, double worldY, TypeDefinition type)
    {
        (int x, int y) = CellUnder(worldX, worldY);
        return (x, y + 1 + Math.Max(0, type.GetInt("height") ?? 0));
    }
}
