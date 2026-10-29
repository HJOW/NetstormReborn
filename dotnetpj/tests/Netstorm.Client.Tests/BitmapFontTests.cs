using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 실제 글꼴 캐시/생성/저장소의 명시 안전 계약을 검사한다.

/// <summary>원본 네 표와 독립 포인터·손상 파일·플랫폼 GDI 생성의 검사다.</summary>
public sealed class BitmapFontTests
{
    /// <summary>클론 자료의 실제18개 캐시/4,608글리프를 모두 읽고 C++ 독립 판독기의302,480픽셀 합계 및 실제 슬롯 메트릭을 대조한다.</summary>
    [Fact]
    public void AllActualCacheFilesAndSlots()
    {
        string? directory = Netstorm.Assets.GameDataLocator.FindDataDirectory(); Assert.NotNull(directory);
        string[] files = Directory.GetFiles(Path.Combine(directory!, "d"), "*.chfnt"); Assert.Equal(18, files.Length); int pixels = 0, glyphs = 0;
        // 실제 파일을 읽기만 하며 원본/클론 자료에 쓰지 않는다.
        foreach (string path in files)
        {
            var font = new BitmapFont(File.ReadAllBytes(path));
            // 특별 빈 프레임과 포인터 없음은 구별해 실제 그림만 합산한다.
            for (int code = 0; code < 256; code++)
            {
                glyphs++; ShapeDatabase? shape = font.Glyph((byte)code).Shape; if (shape is null || shape.Blocks()[0].Frames[0].IsSpecial()) continue;
                IndexedImage image = shape.Decode(0, 0); pixels += image.Indices.Length;
                // 실제 불투명 글자 번호는 원본처럼100이다.
                for (int i = 0; i < image.Indices.Length; i++) if (image.Opacity[i] != 0) Assert.Equal(100, image.Indices[i]);
            }
        }
        Assert.Equal(4608, glyphs); Assert.Equal(302480, pixels); int generated = 0;
        var store = new FontStore(path => { string? actual = Netstorm.Assets.GameDataLocator.FindFile(directory!, path); return actual is null ? null : File.ReadAllBytes(actual); }, "Arial", (_, _, _, _) => { generated++; throw new InvalidOperationException("실제 캐시 누락"); });
        Assert.Equal((14, 11, 3), new GumpText(store).Metrics(new())); Assert.Equal((19, 15, 4), new GumpText(store).Metrics(new() { Font = 3 })); Assert.Equal((12, 9, 3), new GumpText(store).Metrics(new() { Font = 6 })); Assert.Equal((12, 9, 3), new GumpText(store).Metrics(new() { Font = 1 })); Assert.Equal(0, generated);
    }

    /// <summary>포인터 없는 측정 폭과 특별 빈 프레임의 출력 폭을 구별하며 안전 Measure는 overflow를 거부한다.</summary>
    [Fact]
    public void TablesSpecialFrameAndMeasureOverflow()
    {
        BitmapFont pixel = GumpFontFixture.PixelFont(); Assert.Equal(99, pixel.Glyph(65).Advance); Assert.Equal(-31, pixel.Glyph(65).Bearing); Assert.Equal(2, pixel.Glyph(65).DrawAdvance); Assert.Null(pixel.Glyph(66).Shape); Assert.NotNull(pixel.Glyph(32).Shape); Assert.True(pixel.Glyph(32).Shape!.Blocks()[0].Frames[0].IsSpecial()); Assert.Equal(198, pixel.Measure([65, 32]));
        byte[] cache = GumpFontFixture.Header(31); GumpFontFixture.Put(cache, 36 + 65 * 4, int.MaxValue); var font = new BitmapFont(cache); Assert.Throws<OverflowException>(() => font.Measure([65, 65])); Assert.Equal(int.MaxValue, font.Measure([65]));
    }

    /// <summary>헤더/메트릭/꼬리/잘린 글리프를 현재 캐시 파서가 끝까지 검사한다.</summary>
    [Fact]
    public void InvalidCachesAreRejected()
    {
        byte[] cache = GumpFontFixture.Header(31); Assert.Throws<InvalidDataException>(() => new BitmapFont(cache.AsSpan(0, 4131))); Assert.Throws<InvalidDataException>(() => new BitmapFont([.. cache, 0]));
        cache[0] = 0; Assert.Throws<InvalidDataException>(() => new BitmapFont(cache)); cache = GumpFontFixture.Header(31); GumpFontFixture.Put(cache, 20, uint.MaxValue); Assert.Throws<InvalidDataException>(() => new BitmapFont(cache));
        cache = GumpFontFixture.Header(31); GumpFontFixture.Put(cache, 36 + 3072, 43); Assert.Throws<InvalidDataException>(() => new BitmapFont(cache));
    }

    /// <summary>같은 슬롯은 파일/생성을 반복하지 않고 손상 캐시는 실제 원본 굵기의 메모리 생성으로 대체한다.</summary>
    [Fact]
    public void StorePathsFallbackAndStableIdentity()
    {
        var reads = new List<string>(); var generated = new List<(string, int, int, FontStyle)>(); BitmapFont font = GumpFontFixture.PixelFont();
        var fonts = new FontStore(path => { reads.Add(path); return [0]; }, "", (face, height, weight, style) => { generated.Add((face, height, weight, style)); return font; });
        Assert.Same(font, fonts.Get(0, FontStyle.Bold)); Assert.Same(font, fonts.Get(0, FontStyle.Bold)); Assert.Equal(new[] { "d/!Arial.bold.14.700.chfnt" }, reads); Assert.Equal(new[] { ("Arial", 14, 800, FontStyle.Bold) }, generated);
        Assert.Equal("d/!Courier New.normal.13.0.chfnt", fonts.CachePath(1)); fonts.Get(5, FontStyle.Bold); Assert.Equal(("Arial", 14, 700, FontStyle.Bold), generated[^1]);
        Assert.Throws<ArgumentException>(() => fonts.Get(2)); Assert.Throws<ArgumentException>(() => fonts.Get(3, FontStyle.Bold)); Assert.Throws<ArgumentException>(() => fonts.Get(0, (FontStyle)(-1))); Assert.Throws<ArgumentException>(() => FontStore.LogicalHeight(7));
    }

    /// <summary>cpppj RendererTests의 GDI fallback과 같은 실제 창 없는 메모리 생성 계약이다. Linux에서는 캐시 사용 경로만 대상이다.</summary>
    [Fact]
    public void WindowsGdiFallbackProducesReadableGlyphs()
    {
        if (!OperatingSystem.IsWindows()) return; BitmapFont font = WindowsBitmapFont.Generate("Arial", 14, 700, FontStyle.Normal);
        Assert.InRange(font.Height(), 1, 512); Assert.True(font.Ascent() > 0); Assert.True(font.Measure("NetStorm"u8) > font.Measure("Net"u8)); Assert.NotNull(font.Glyph(65).Shape); ShapeDatabase shape = font.Glyph(65).Shape!; Assert.False(shape.Blocks()[0].Frames[0].IsSpecial()); IndexedImage image = shape.Decode(0, 0); Assert.Contains((byte)100, image.Indices); Assert.NotNull(font.Glyph(32).Shape);
        var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 160, 40)); byte[] pixels = new byte[6400]; GumpGlyphHooks hooks = GumpTextDraw.MakeGumpGlyphPixelHooks(new(pixels, 160, 40, clip), new());
        GumpTextDraw.DrawBitmapGlyphText("NetStorm\0"u8, font, new(2, 2), new(6, 0, 255, 0, 0, 0, 1, 1), hooks); Assert.True(pixels.Count(p => p == 255) > 40);
        Assert.Throws<ArgumentException>(() => WindowsBitmapFont.Generate("Arial", 0, 700, FontStyle.Normal));
    }
}
