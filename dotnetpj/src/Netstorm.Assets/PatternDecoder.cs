namespace Netstorm.Assets;

/// <summary>원본 CanonDecoder 패턴의 한 칸. Label은 원본 번호 바이트에서 'a'를 뺀 값이다.</summary>
public readonly record struct PatternCell(char Side, int Variation, int Label = -97);

/// <summary>원본 패턴의 폭·높이와 행 우선 셀 목록. '.'은 빈 칸이다.</summary>
public sealed record CanonicalPattern(int Width, int Height, IReadOnlyList<PatternCell> Cells);

/// <summary>원본 반복자가 돌려주는 유효 프레임·단정밀도 좌표·번호·방향.</summary>
public readonly record struct DecodedPatternCell(int Frame, float X, float Y, int Label, char Side);

/// <summary>10.78 CanonDecoder 00425c20/00425860의 회전·누락 프레임·순회를 제공한다.</summary>
public static class PatternDecoder
{
    /// <summary>원본 00531590에서 읽은 네 회전의 방향 문자. 마지막 P열은 변형 방향이다.</summary>
    private static readonly string[] RotatedSides = ["ABCDEFGHIJKLMNOP", "ACDEBGHIFKJMNOLP", "ADEBCHIFGJKNOLMP", "AEBCDIFGHKJOLMNP"];

    /// <summary>조각을 회전해 원본과 같은 y·x 순서로 읽는다. 프레임 없는 칸은 목록에서 제외한다.</summary>
    public static IReadOnlyList<DecodedPatternCell> Decode(CanonicalPattern pattern, TypeFrameTable frames, int direction, float originX = 0, float originY = 0)
    {
        int rotation = direction / 2;
        if (direction < 0 || rotation > 3) throw new ArgumentOutOfRangeException(nameof(direction));
        if (pattern.Width <= 0 || pattern.Height <= 0 || pattern.Cells.Count != pattern.Width * pattern.Height)
            throw new InvalidDataException("원본 패턴의 크기와 셀 수가 다릅니다.");
        var result = new List<(int X, int Y, PatternCell Cell)>();
        // 원본 패턴의 각 행을 목적지 행·열로 옮긴다.
        for (int y = 0; y < pattern.Height; y++)
        {
            // 빈 칸도 좌표는 차지하므로 위치를 압축하지 않는다.
            for (int x = 0; x < pattern.Width; x++)
            {
                PatternCell cell = pattern.Cells[y * pattern.Width + x];
                if (cell.Side == '.') continue;
                if (cell.Side is < 'A' or > 'P' || cell.Variation is < 0 or > 10) throw new InvalidDataException("잘못된 원본 패턴 셀입니다.");
                (int rx, int ry) = rotation switch
                {
                    1 => (pattern.Height - 1 - y, x), 2 => (pattern.Width - 1 - x, pattern.Height - 1 - y),
                    3 => (y, pattern.Width - 1 - x), _ => (x, y),
                };
                result.Add((rx, ry, cell));
            }
        }
        var decoded = new List<DecodedPatternCell>();
        // 원본 반복자는 회전 뒤 행 우선 순서로 진행하고, 첫 일치 프레임만 반환한다.
        foreach (var item in result.OrderBy(item => item.Y).ThenBy(item => item.X))
        {
            char side = RotatedSides[rotation][item.Cell.Side - 'A'];
            int frame = frames.Find(side, RotatedSides[rotation][15], item.Cell.Variation);
            if (frame >= 0) decoded.Add(new DecodedPatternCell(frame, originX + (float)item.X, originY + (float)item.Y, item.Cell.Label, side));
        }
        return decoded;
    }
}
