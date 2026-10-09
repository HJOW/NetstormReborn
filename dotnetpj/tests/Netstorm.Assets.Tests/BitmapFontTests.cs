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
