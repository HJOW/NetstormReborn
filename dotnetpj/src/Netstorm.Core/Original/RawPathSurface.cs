namespace Netstorm.Core.Original;

/// <summary>
/// 0040eaf0·CD 0045b670의 실제 표면 번호를 같은 hash0에서 조회한다
/// (cpppj <c>o/RawPathSearch.cpp</c>의 자유 함수 <c>RawPathSurfaceAt</c>와 그 안의 도우미 <c>Cell</c>·<c>Inside</c>).
/// 점 정의(<c>RawPathPoint</c>)는 기존 <c>RawSquidPoolReset.cs</c>의 것을 빌린다.
/// BFS 거리·대기열·그래프 조회는 옮기지 않았다.
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 섬 목록의 거주 구역 owner 전파가 유일한 소비자이며 전체 탐색기는 R6에서 잇는다.</remarks>
public static class RawPathSurface
{
    /// <summary>원본 보드 한 변의 칸 수다.</summary>
    public const int BoardSide = 256;

    /// <summary>표면 셀 변환이 더하는 원본 +0.9999 편향이다.</summary>
    public const float CellBias = 0.9999f;

    /// <summary>좌표 실수를 셀 정수 아래쪽 절삭 없이 x87 원본 double 덧셈으로 바꾼다.</summary>
    /// <param name="value">바꿀 좌표다.</param>
    /// <returns>셀 좌표다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">BFS 표면 좌표 오류다.</exception>
    public static int Cell(float value)
    {
        double adjusted = value + CellBias;
        if (!double.IsFinite(adjusted) || adjusted < -2147483648.0 || adjusted > 2147483647.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), value, "BFS 표면 좌표 오류");
        }
        return (int)adjusted;
    }

    /// <summary>셀 좌표가 보드 안에 있는지 본다.</summary>
    /// <param name="x">셀 X 좌표다.</param>
    /// <param name="y">셀 Y 좌표다.</param>
    /// <returns>보드 안이면 참이다.</returns>
    public static bool Inside(int x, int y)
    {
        return x >= 0 && y >= 0 && x < BoardSide && y < BoardSide;
    }

    /// <summary>좌표에 0.9999를 더해 잘라 같은 hash0에서 실제 표면 SID를 조회하며 보드 밖은 0이다.</summary>
    /// <param name="hash">읽을 현재 hash0 단어 지도다.</param>
    /// <param name="position">조회할 좌표다.</param>
    /// <returns>표면 SID다. 없으면 0이다.</returns>
    public static int SurfaceAt(RawSquidHash hash, RawPathPoint position)
    {
        ArgumentNullException.ThrowIfNull(hash);
        int x = Cell(position.X);
        int y = Cell(position.Y);
        if (!Inside(x, y))
        {
            return 0;
        }
        return hash.Cell(0, x, y);
    }
}
