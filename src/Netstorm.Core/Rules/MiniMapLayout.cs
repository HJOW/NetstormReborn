namespace Netstorm.Core.Rules;

/// <summary>
/// 원본 미니맵의 좌표 규칙. 미니맵은 월드 전체가 아니라 현재 화면 중심을 가운데 둔 창을 1픽셀 = 2칸 배율로 보여 주며,
/// 창이 월드 밖으로 나가지 않게 가장자리에서만 멈춘다. 그래서 흰 시야 사각형은 평소 상자 한가운데에 있고
/// 월드 끝에서만 상자 안을 움직인다.
/// 근거: 1-1 시작 캡처(창 원점 (17.6, 33.6), 사각형 중심 = 상자 중심), 2026-10-03 TEST01 녹화 37.5초
/// (화면 중심 (172.7, 201.2) → 창 원점 (93.2, 106.8): 가로는 중심 − 78, 세로는 월드 아래 끝 256 − 148 에 걸림).
/// </summary>
public static class MiniMapLayout
{
    /// <summary>미니맵 1픽셀이 나타내는 칸 수 (가로·세로 같음). 가이저 13개의 미니맵 점과 월드 칸을 맞춰 확정했다.</summary>
    public const int CellsPerPixel = 2;

    /// <summary>
    /// 미니맵 창의 왼쪽(또는 위쪽) 끝 칸. 화면 중심을 가운데 두되 월드 범위 [0, worldCells] 안으로 제한한다.
    /// 창이 월드보다 크면 0이다.
    /// </summary>
    /// <param name="viewCenterCell">현재 화면 중심의 월드 칸 좌표 (한 축)</param>
    /// <param name="boxPixels">미니맵 상자의 픽셀 크기 (같은 축)</param>
    /// <param name="worldCells">월드의 칸 수 (같은 축)</param>
    public static double Origin(double viewCenterCell, int boxPixels, int worldCells)
    {
        double window = boxPixels * CellsPerPixel;
        double limit = Math.Max(0, worldCells - window);
        return Math.Clamp(viewCenterCell - window / 2, 0, limit);
    }

    /// <summary>월드 칸 좌표를 미니맵 상자 안 픽셀 좌표로 바꾼다 (상자 왼쪽 위가 0).</summary>
    /// <param name="cell">월드 칸 좌표 (한 축)</param>
    /// <param name="origin">미니맵 창의 시작 칸 (<see cref="Origin"/>)</param>
    public static double ToPixel(double cell, double origin) => (cell - origin) / CellsPerPixel;

    /// <summary>미니맵 상자 안 픽셀 좌표를 월드 칸 좌표로 바꾼다. 상자 밖 좌표는 상자 끝으로 당긴다.</summary>
    /// <param name="pixel">상자 왼쪽 위 기준 픽셀 좌표 (한 축)</param>
    /// <param name="origin">미니맵 창의 시작 칸</param>
    /// <param name="boxPixels">미니맵 상자의 픽셀 크기 (같은 축)</param>
    public static double ToCell(double pixel, double origin, int boxPixels) =>
        origin + Math.Clamp(pixel, 0, boxPixels) * CellsPerPixel;
}
