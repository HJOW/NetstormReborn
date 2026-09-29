namespace Netstorm.Core.Bridges;

/// <summary>회전을 적용한 다리 조각의 한 칸: 조각 왼쪽 위 기준 칸 위치와 모양</summary>
/// <param name="Dx">조각 원점에서의 월드 x 칸 차이 (0 이상)</param>
/// <param name="Dy">조각 원점에서의 월드 y 칸 차이 (0 이상)</param>
/// <param name="Cell">회전된 방향 글자와 변형</param>
public readonly record struct PlacedBridgeCell(int Dx, int Dy, BridgeCell Cell);

/// <summary>
/// 생산 창·커서의 다리 조각 하나: 모양 번호 + 회전 번호 (원본 Canondecoder 오브젝트 FUN_00425c20).
/// 회전 1 은 시계 방향 90° 다. 칸 위치는 원본 FUN_00425860 의 순회(시작 칸 표 0x531570·0x531580,
/// 이동 표 0x531550·0x531560)와 같은 결과인 식으로 계산한다:
/// 회전 1 = (h−1−y, x), 회전 2 = (w−1−x, h−1−y), 회전 3 = (y, w−1−x).
/// </summary>
public sealed class BridgePiece
{
    /// <summary>모양</summary>
    public BridgePattern Pattern { get; }

    /// <summary>회전 번호 0~3 (1 = 시계 방향 90°)</summary>
    public int Rotation { get; private set; }

    /// <summary>회전 뒤 가로 칸 수</summary>
    public int Width => Rotation % 2 == 0 ? Pattern.Width : Pattern.Height;

    /// <summary>회전 뒤 세로 칸 수</summary>
    public int Height => Rotation % 2 == 0 ? Pattern.Height : Pattern.Width;

    /// <summary>조각을 만든다</summary>
    /// <param name="patternIndex">BridgePatternCatalog 의 모양 번호</param>
    /// <param name="rotation">회전 번호</param>
    public BridgePiece(int patternIndex, int rotation = 0)
    {
        Pattern = BridgePatternCatalog.Patterns[patternIndex];
        Rotation = BridgeDirections.NormalizeRotation(rotation);
    }

    /// <summary>시계 방향으로 90° 돌린다</summary>
    public void RotateClockwise() => Rotation = BridgeDirections.NormalizeRotation(Rotation + 1);

    /// <summary>반시계 방향으로 90° 돌린다 (원본 메뉴 "Counterclockwise Piece Rotation - C" 대응 후보)</summary>
    public void RotateCounterclockwise() => Rotation = BridgeDirections.NormalizeRotation(Rotation - 1);

    /// <summary>회전을 적용한 다리 칸 목록 (행 우선: 회전 뒤 y, x 순서)</summary>
    public IReadOnlyList<PlacedBridgeCell> Cells()
    {
        var cells = new List<PlacedBridgeCell>(Pattern.FilledCount);
        int w = Pattern.Width;
        int h = Pattern.Height;
        // 원본 모양의 각 칸을 회전 뒤 위치로 옮기고 방향 글자도 같은 만큼 돌린다
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                BridgeCell? source = Pattern.CellAt(x, y);
                if (source is not { } cell)
                {
                    continue;
                }
                (int dx, int dy) = Rotation switch
                {
                    1 => (h - 1 - y, x),
                    2 => (w - 1 - x, h - 1 - y),
                    3 => (y, w - 1 - x),
                    _ => (x, y),
                };
                cells.Add(new PlacedBridgeCell(dx, dy, cell with { Letter = BridgeDirections.Rotate(cell.Letter, Rotation) }));
            }
        }
        // 회전 뒤 행 우선 순서로 정렬해 표시·배치 순서를 일정하게 한다
        cells.Sort((a, b) => a.Dy != b.Dy ? a.Dy.CompareTo(b.Dy) : a.Dx.CompareTo(b.Dx));
        return cells;
    }

    /// <summary>개발용 표기 (예: "조각 4 회전 1")</summary>
    public override string ToString() => $"조각 {Pattern.Index} 회전 {Rotation}";
}
