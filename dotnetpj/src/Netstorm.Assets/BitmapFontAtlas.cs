namespace Netstorm.Assets;

/// <summary>글꼴 아틀라스 안의 글자 한 칸.</summary>
/// <param name="Code">Windows-1252 바이트 코드</param>
/// <param name="Character">그 코드의 유니코드 문자</param>
/// <param name="X">아틀라스 안 칸의 왼쪽</param>
/// <param name="Y">아틀라스 안 칸의 위쪽</param>
/// <param name="Width">칸의 폭 (글자 모양이 놓는 점에서 뻗는 폭, 최소 1)</param>
/// <param name="Height">칸의 높이 (모든 칸이 같다)</param>
/// <param name="Advance">다음 글자로 넘어가는 폭 (원본의 그리기 전진 폭)</param>
public readonly record struct BitmapFontCell(byte Code, char Character, int X, int Y, int Width, int Height, int Advance);

/// <summary>
/// 원본 비트맵 글꼴(<see cref="BitmapFont"/>)의 글자 모양을 한 장의 그림으로 모은 아틀라스.
/// 화면 쪽은 <see cref="Coverage"/> 를 흰색 + 알파 텍스처로 올리고 <see cref="Cells"/> 로 글자 칸을 찾는다.
/// 칸은 **글자를 놓는 점(줄의 왼쪽 위)을 왼쪽 위 모서리로** 잡고 높이를 모두 <see cref="LineHeight"/> 로 맞췄다.
/// 그래서 칸을 놓는 점에 그대로 그리면 원본 출력(004a3430: 놓는 점 + 글리프 상자 위치, 전진은 셋째 표)과 같은 픽셀이 되고,
/// 글자열의 높이가 어떤 글자든 줄 높이로 일정하다. 사용법: <c>var atlas = new BitmapFontAtlas(font);</c>
/// </summary>
/// <remarks>
/// 코드 32 미만(제어 문자)은 넣지 않는다. 원본은 측정에 첫째 표, 출력에 셋째 표를 쓰지만 실제 캐시 18개에서 두 표는 모든 글자가 같다
/// (테스트가 확인한다). 2026-10-10 추가 (LEFT_JOBS.dotnetpj.md 5-3).
/// </remarks>
public sealed class BitmapFontAtlas
{
    /// <summary>아틀라스에 넣는 첫 코드 (공백).</summary>
    public const int FirstCode = 32;

    /// <summary>아틀라스의 최대 폭. 이 폭을 넘으면 다음 줄로 넘긴다.</summary>
    private const int MaxWidth = 512;

    /// <summary>칸 사이에 비워 두는 픽셀 (확대 표시에서 옆 글자가 번지지 않게 한다).</summary>
    private const int Padding = 1;

    /// <summary>아틀라스 폭.</summary>
    public int Width { get; }

    /// <summary>아틀라스 높이.</summary>
    public int Height { get; }

    /// <summary>줄 높이 = 모든 칸의 높이. 원본 글꼴 높이와, 글리프가 그보다 아래로 내려가면 그 끝까지.</summary>
    public int LineHeight { get; }

    /// <summary>행 우선 불투명 여부 (true = 글자 픽셀).</summary>
    public bool[] Coverage { get; }

    /// <summary>코드 32~255 의 글자 칸 (코드 순서).</summary>
    public IReadOnlyList<BitmapFontCell> Cells { get; }

    /// <summary>글꼴의 글자 모양을 아틀라스로 배치한다.</summary>
    /// <param name="font">원본 비트맵 글꼴</param>
    public BitmapFontAtlas(BitmapFont font)
    {
        int lineHeight = font.Height;
        // 글리프 상자가 글꼴 높이를 넘는 경우(없어야 하지만)에도 잘리지 않게 줄 높이를 정한다.
        foreach (BitmapGlyph glyph in font.Glyphs)
        {
            if (glyph.Image is { } image)
            {
                if (glyph.OffsetX < 0 || glyph.OffsetY < 0)
                {
                    throw new InvalidDataException("비트맵 글리프 상자가 놓는 점의 왼쪽·위로 나갑니다.");
                }
                lineHeight = Math.Max(lineHeight, glyph.OffsetY + image.Height);
            }
        }
        LineHeight = lineHeight;
        var cells = new List<BitmapFontCell>();
        int x = Padding, y = Padding, width = 0;
        // 코드 순서대로 칸을 왼쪽에서 오른쪽으로 놓고 폭이 차면 다음 줄로 넘긴다.
        for (int code = FirstCode; code < font.Glyphs.Count; code++)
        {
            BitmapGlyph glyph = font.Glyphs[code];
            int cellWidth = Math.Max(1, glyph.Image is { } image ? glyph.OffsetX + image.Width : 0);
            if (x + cellWidth + Padding > MaxWidth)
            {
                x = Padding;
                y += lineHeight + Padding;
            }
            char character = OriginalText.DecodeWindows1252([(byte)code])[0];
            cells.Add(new BitmapFontCell((byte)code, character, x, y, cellWidth, lineHeight, glyph.DrawAdvance));
            x += cellWidth + Padding;
            width = Math.Max(width, x);
        }
        Width = width;
        Height = y + lineHeight + Padding;
        Coverage = new bool[Width * Height];
        // 칸마다 글리프 모양을 상자 위치에 옮긴다.
        foreach (BitmapFontCell cell in cells)
        {
            BitmapGlyph glyph = font.Glyphs[cell.Code];
            if (glyph.Image is not { } image) continue;
            // 모양의 행을 위에서부터 옮긴다.
            for (int row = 0; row < image.Height; row++)
            {
                // 행 안의 불투명 픽셀만 표시한다.
                for (int column = 0; column < image.Width; column++)
                {
                    if (image.Opaque[row * image.Width + column])
                    {
                        Coverage[(cell.Y + glyph.OffsetY + row) * Width + cell.X + glyph.OffsetX + column] = true;
                    }
                }
            }
        }
        Cells = cells;
    }
}
