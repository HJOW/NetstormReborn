namespace Netstorm.Assets.Tests;

/// <summary>원본 비트맵 글꼴 캐시(.chfnt) 판독기 테스트 (docs/formats/chfnt.md).</summary>
public sealed class BitmapFontTests
{
    /// <summary>원본 캐시 수.</summary>
    private const int ExpectedCaches = 18;

    /// <summary>캐시당 글자 수.</summary>
    private const int GlyphsPerCache = 256;

    /// <summary>원본 글자 불투명 픽셀의 팔레트 번호.</summary>
    private const byte FontInk = 100;

    /// <summary>d/ 폴더의 .chfnt 파일 18개를 전부 찾아 경로 순서로 돌려준다.</summary>
    private static string[] CacheFiles()
    {
        string dir = Path.Combine(OriginalData.RequireDirectory(), "d");
        string[] files = Directory.GetFiles(dir).Where(path => path.EndsWith(".chfnt", StringComparison.OrdinalIgnoreCase))
            .OrderBy(path => path, StringComparer.Ordinal).ToArray();
        Assert.Equal(ExpectedCaches, files.Length);
        return files;
    }

    /// <summary>원본 캐시 18개를 전수 판독한다: 머리말·256글리프·파일 끝 일치를 확인한다.</summary>
    [Fact]
    public void OriginalCaches_LoadAllEighteenFiles()
    {
        int total = 0;
        // 18개 캐시를 경로 순서대로 읽는다.
        foreach (string path in CacheFiles())
        {
            BitmapFont font = BitmapFont.Load(path);
            Assert.Equal(GlyphsPerCache, font.Glyphs.Count);
            Assert.InRange(font.Height, 1, 512);
            Assert.InRange(font.Ascent, 0, font.Height);
            Assert.InRange(font.Descent, 0, font.Height);
            total += font.Glyphs.Count;
        }
        Assert.Equal(ExpectedCaches * GlyphsPerCache, total);
    }

    /// <summary>4,608글리프 전수: 불투명 픽셀은 색 100이며, 모양 있는 글리프는 픽셀이 하나 이상 있다.</summary>
    [Fact]
    public void GlyphPixels_UseInkColorAndAdvance()
    {
        int pixels = 0;
        int blank = 0;
        // 18개 캐시의 모든 글리프를 본다.
        foreach (string path in CacheFiles())
        {
            BitmapFont font = BitmapFont.Load(path);
            // 캐시 안의 글리프를 코드 순서대로 본다.
            foreach (BitmapGlyph glyph in font.Glyphs)
            {
                if (glyph.Image is not { } image)
                {
                    blank++;
                    continue;
                }
                Assert.True(image.Width > 0 && image.Height > 0);
                pixels += image.Indices.Length;
                int ink = 0;
                // 해독된 픽셀을 행 우선으로 본다.
                for (int i = 0; i < image.Indices.Length; i++)
                {
                    if (!image.Opaque[i]) continue;
                    Assert.Equal(FontInk, image.Indices[i]);
                    ink++;
                }
                Assert.True(ink > 0);
            }
        }
        Assert.True(blank > 0);
        // C++ 독립 판독기의 전수 대조 픽셀 수와 같은 값이다.
        Assert.Equal(302480, pixels);
    }

    /// <summary>문자열 측정: 빈 열은 0이며 글자 폭의 합과 같다.</summary>
    [Fact]
    public void MeasureBytes_SumsFirstTable()
    {
        BitmapFont font = BitmapFont.Load(CacheFiles()[0]);
        Assert.Equal(0, font.MeasureBytes([]));
        byte[] word = "ABC"u8.ToArray();
        int expected = 0;
        // 세 글자의 측정 폭을 직접 더한다.
        foreach (byte code in word)
        {
            expected += font.Glyph(code).Advance;
        }
        Assert.Equal(expected, font.MeasureBytes(word));
    }

    /// <summary>
    /// 원본 슬롯의 캐시 경로가 실제 파일을 가리키고, UI 가 쓰는 세 슬롯의 높이·어센트·디센트가 원본 값이다
    /// (슬롯 0 본문 14/11/3, 슬롯 3 제목 19/15/4, 슬롯 6 작은 글자 12/9/3).
    /// </summary>
    /// <param name="slot">글꼴 슬롯</param>
    /// <param name="path">기대하는 캐시 경로</param>
    /// <param name="height">글꼴 높이</param>
    /// <param name="ascent">기준선 위 높이</param>
    /// <param name="descent">기준선 아래 높이</param>
    [Theory]
    [InlineData(BitmapFont.BodySlot, "d/!Arial.normal.14.700.chfnt", 14, 11, 3)]
    [InlineData(BitmapFont.TitleSlot, "d/!Arial.normal.20.700.chfnt", 19, 15, 4)]
    [InlineData(BitmapFont.SmallSlot, "d/!Arial.normal.12.0.chfnt", 12, 9, 3)]
    [InlineData(1, "d/!Courier New.normal.13.0.chfnt", 12, 9, 3)]
    public void CachePath_PointsAtOriginalSlotFiles(int slot, string path, int height, int ascent, int descent)
    {
        Assert.Equal(path, BitmapFont.CachePath(slot));
        BitmapFont font = BitmapFont.Load(OriginalData.RequireFile(path));
        Assert.Equal((height, ascent, descent), (font.Height, font.Ascent, font.Descent));
        Assert.Equal("d/!Arial.italic.14.0.chfnt", BitmapFont.CachePath(5, "italic"));
        Assert.Throws<ArgumentOutOfRangeException>(() => BitmapFont.CachePath(2));
        Assert.Throws<ArgumentOutOfRangeException>(() => BitmapFont.CachePath(7));
    }

    /// <summary>
    /// 캐시 18개 모두 측정 폭(첫 표)과 그리기 전진 폭(셋째 표)이 256글자에서 같고, 글리프 상자가 놓는 점의 오른쪽·아래에만 있으며
    /// 글꼴 높이를 넘지 않는다. 화면 글꼴이 폭 표 하나로 재고 그려도 원본과 같다는 전제다.
    /// </summary>
    [Fact]
    public void OriginalCaches_MeasureAndDrawAdvancesAgree()
    {
        // 18개 캐시를 경로 순서대로 본다.
        foreach (string path in CacheFiles())
        {
            BitmapFont font = BitmapFont.Load(path);
            // 캐시 안의 글리프를 코드 순서대로 본다.
            foreach (BitmapGlyph glyph in font.Glyphs)
            {
                Assert.Equal(glyph.Advance, glyph.DrawAdvance);
                Assert.True(glyph.OffsetX >= 0 && glyph.OffsetY >= 0);
                if (glyph.Image is { } image) Assert.True(glyph.OffsetY + image.Height <= font.Height);
            }
        }
    }

    /// <summary>
    /// 아틀라스: 코드 32~255 의 칸이 겹치지 않고, 칸 높이가 줄 높이로 같으며, 칸 안의 글자 픽셀이 글리프 모양을 상자 위치에 놓은 것과 같다.
    /// 문자열 폭(칸 전진 폭의 합)은 원본 측정과 같다.
    /// </summary>
    [Fact]
    public void Atlas_PlacesGlyphsAtTheirBoxOffsets()
    {
        BitmapFont font = BitmapFont.Load(OriginalData.RequireFile(BitmapFont.CachePath(BitmapFont.BodySlot)));
        var atlas = new BitmapFontAtlas(font);
        Assert.Equal(14, atlas.LineHeight);
        Assert.Equal(256 - BitmapFontAtlas.FirstCode, atlas.Cells.Count);
        var claimed = new bool[atlas.Width * atlas.Height];
        int ink = 0;
        // 칸마다 영역이 겹치지 않는지와 글자 픽셀 위치를 확인한다.
        foreach (BitmapFontCell cell in atlas.Cells)
        {
            BitmapGlyph glyph = font.Glyph(cell.Code);
            Assert.Equal(atlas.LineHeight, cell.Height);
            Assert.Equal(glyph.DrawAdvance, cell.Advance);
            Assert.True(cell.X >= 0 && cell.Y >= 0 && cell.X + cell.Width <= atlas.Width && cell.Y + cell.Height <= atlas.Height);
            // 칸의 행을 위에서부터 본다.
            for (int y = 0; y < cell.Height; y++)
            {
                // 칸의 열을 왼쪽부터 본다.
                for (int x = 0; x < cell.Width; x++)
                {
                    int index = (cell.Y + y) * atlas.Width + cell.X + x;
                    Assert.False(claimed[index]);
                    claimed[index] = true;
                    bool expected = glyph.Image is { } image && x >= glyph.OffsetX && y >= glyph.OffsetY
                        && x < glyph.OffsetX + image.Width && y < glyph.OffsetY + image.Height
                        && image.Opaque[(y - glyph.OffsetY) * image.Width + x - glyph.OffsetX];
                    Assert.Equal(expected, atlas.Coverage[index]);
                    if (expected) ink++;
                }
            }
        }
        // 칸 밖에는 글자 픽셀이 없다.
        Assert.Equal(ink, atlas.Coverage.Count(pixel => pixel));
        Assert.True(ink > 0);
        byte[] text = "Review Knowledge"u8.ToArray();
        Assert.Equal(106, font.MeasureBytes(text));
        Assert.Equal(106, text.Sum(code => atlas.Cells[code - BitmapFontAtlas.FirstCode].Advance));
        Assert.Equal('A', atlas.Cells['A' - BitmapFontAtlas.FirstCode].Character);
        Assert.Equal('€', atlas.Cells[0x80 - BitmapFontAtlas.FirstCode].Character);
    }

    /// <summary>손상된 캐시는 거부한다 (원본은 GDI 로 다시 만든다).</summary>
    [Fact]
    public void RejectsCorruptCaches()
    {
        // 높이 14·어센트 11·디센트 3, 글리프 블록 없는 최소 정상 캐시를 만든다.
        byte[] valid = new byte[4132];
        "BitmapFontData\x1A\0"u8.CopyTo(valid);
        WriteU32(valid, 16, 1);
        WriteU32(valid, 20, 14);
        WriteU32(valid, 24, 11);
        WriteU32(valid, 28, 3);
        WriteU32(valid, 32, 256);
        Assert.Equal(256, new BitmapFont(valid).Glyphs.Count);
        // 서명·버전·글자 수·높이가 다르면 거부한다.
        Assert.Throws<InvalidDataException>(() => new BitmapFont([]));
        byte[] bad = (byte[])valid.Clone();
        bad[0] = (byte)'X';
        Assert.Throws<InvalidDataException>(() => new BitmapFont(bad));
        bad = (byte[])valid.Clone();
        WriteU32(bad, 16, 2);
        Assert.Throws<InvalidDataException>(() => new BitmapFont(bad));
        bad = (byte[])valid.Clone();
        WriteU32(bad, 32, 255);
        Assert.Throws<InvalidDataException>(() => new BitmapFont(bad));
        bad = (byte[])valid.Clone();
        WriteU32(bad, 20, 0);
        Assert.Throws<InvalidDataException>(() => new BitmapFont(bad));
        // 잘린 파일과 끝에 남는 바이트도 거부한다.
        Assert.Throws<InvalidDataException>(() => new BitmapFont(valid[..4000]));
        Assert.Throws<InvalidDataException>(() => new BitmapFont([.. valid, (byte)0]));
    }

    /// <summary>리틀 엔디언 u32를 쓴다.</summary>
    /// <param name="raw">버퍼</param>
    /// <param name="offset">바이트 오프셋</param>
    /// <param name="value">쓸 값</param>
    private static void WriteU32(byte[] raw, int offset, uint value)
    {
        // 낮은 바이트부터 쓴다.
        for (int i = 0; i < 4; i++)
        {
            raw[offset + i] = (byte)(value >> (i * 8));
        }
    }
}
