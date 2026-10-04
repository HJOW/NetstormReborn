namespace Netstorm.Core.Bridges;

/// <summary>
/// 들고 있는 다리 조각의 위치 규칙 (2026-09-29 원본 실행으로 측정, docs/exe/bridge-pieces.md 4절).
/// 조각의 왼쪽 위 칸은 조각 크기·회전과 관계없이 커서로만 정해진다.
/// 월드 픽셀(칸 (cx, cy)의 기준점 = (16·cx, 11·cy), 스프라이트는 기준점 왼쪽 위로 그려짐) 기준으로
/// x 는 기준점 −7 ~ +8, y 는 기준점 ~ +10 범위의 커서가 그 칸을 가리킨다.
/// </summary>
public static class BridgeCursor
{
    /// <summary>칸 가로 픽셀</summary>
    public const int CellWidth = 16;

    /// <summary>칸 세로 픽셀</summary>
    public const int CellHeight = 11;

    /// <summary>x 방향 보정(픽셀): 원본은 기준점보다 7픽셀 왼쪽부터 그 칸으로 본다</summary>
    public const int HorizontalBias = 7;

    /// <summary>커서 월드 픽셀 → 조각 왼쪽 위 칸</summary>
    /// <param name="worldX">커서의 월드 x 픽셀</param>
    /// <param name="worldY">커서의 월드 y 픽셀</param>
    public static (int X, int Y) TopLeftCell(double worldX, double worldY) =>
        ((int)Math.Floor((worldX + HorizontalBias) / CellWidth), (int)Math.Floor(worldY / CellHeight));
}
