using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 원본 글리프 사건 전수와 실제 픽셀/현재 문맥을 검사한다.

/// <summary>cpppj/tests/GumpGlyphTests.cpp의 원본 재생과 독립 픽셀 계약이다. Renderer 통합은 R5에서 잇는다.</summary>
public sealed class GumpGlyphTests
{
    /// <summary>판본별256행의 실제 선택 글꼴·BYTE 색·DWORD 좌표·순서/빈 표/폭/가상 위임을 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void NativeEditions(string edition)
    {
        var fixture = new GumpFontFixture("gumpglyph"); string[][] rows = fixture.Cases.Where(r => r[1] == edition).ToArray(); Assert.Equal(256, rows.Length);
        // 매 관측의 전체 원본 사건만 기대값으로 사용한다.
        foreach (string[] r in rows)
        {
            Assert.Equal(15, r.Length); var trace = new List<string>(); byte color = 0;
            var hooks = new GumpGlyphHooks { Color = c => { color = c; trace.Add("C," + c); }, IllegalStyle = () => trace.Add("E"), Glyph = (font, code, at) => { var key = fixture.Identities[font]; trace.Add($"G,{key.Slot},{key.Style},{code},{unchecked((uint)at.X)},{unchecked((uint)at.Y)},{color}"); } };
            var context = new GumpTextContext(X86Fixture.UInt(r[5]), X86Fixture.UInt(r[3]), X86Fixture.UInt(r[6]), X86Fixture.UInt(r[4]), 0xabcdef01, X86Fixture.UInt(r[7]), X86Fixture.Int(r[8]), X86Fixture.Int(r[9]));
            byte[] bytes = X86Fixture.Bytes(r[13]); var offset = new ScreenPoint(X86Fixture.Int(r[10]), X86Fixture.Int(r[11])); var draw = new GumpTextDraw(fixture.Fonts, hooks, edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            if (r[2] == "Direct") draw.Draw(bytes, offset, context, X86Fixture.Int(r[12]));
            else
            {
                var root = new GumpCreateNode(); var node = new GumpCreateNode { X = -13, Y = 27, Parent = 1 }; uint[] words = [context.Flags, context.Font, context.Color, context.Style, context.Extra, context.ShadowColor, unchecked((uint)context.ShadowX), unchecked((uint)context.ShadowY)]; words.CopyTo(node.TextContext, 0);
                var collection = new GumpCollection(new() { Root = 1 }, new() { Node = t => t == 1 ? root : node });
                if (r[2] == "Default") draw.DrawGump(collection, 2, bytes, offset, node.TextContext); else draw.DrawGump(collection, 2, bytes, offset, node.TextContext, in context);
                Assert.Equal(new ScreenPoint(-13, 27), node.Cached);
            }
            Assert.Equal(r[14], string.Join(';', trace));
        }
    }

    /// <summary>특수 빈 공백은 전진하고 포인터 없는B는 전진하지 않는다. 첫 폭/ABC A와 독립인 픽셀 기대값이다.</summary>
    [Fact]
    public void PixelsUseDrawAdvanceAndCurrentClip()
    {
        BitmapFont font = GumpFontFixture.PixelFont(); var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 9, 3)); byte[] pixels = Enumerable.Repeat((byte)7, 30).ToArray(); var target = new GumpPixelTarget(pixels, 10, 3, clip); var colors = new GumpGlyphColorState(); colors.FontColors[17] = 81;
        GumpGlyphHooks h = GumpTextDraw.MakeGumpGlyphPixelHooks(target, colors); byte[] bytes = [65, 32, 66, 65, 0];
        GumpTextDraw.DrawBitmapGlyphText(bytes, font, new(1, 1), new(0, 0, 0x123456ab, 0, 0, 0, 0, 0), h); Assert.Equal(171, pixels[11]); Assert.Equal(171, pixels[16]); Assert.Equal(2, pixels.Count(p => p == 171)); Assert.Equal(171, colors.VfxColors[100]); Assert.Equal(81, colors.VfxColors[17]);
        GumpDraw.SetGumpDrawClip(clip, new(2, 1, 7, 2)); pixels.AsSpan().Fill(7); GumpTextDraw.DrawBitmapGlyphText(bytes, font, new(1, 1), new(0, 0, 44, 0, 0, 0, 0, 0), h); Assert.Equal(7, pixels[11]); Assert.Equal(44, pixels[16]); Assert.Equal(1, pixels.Count(p => p == 44));
    }

    /// <summary>한 픽셀 글자의 그림자/고정4방향/본문과 빈 문자열의 마지막 색 설치를 확인한다.</summary>
    [Fact]
    public void ShadowOutlineAndEmptyText()
    {
        BitmapFont font = GumpFontFixture.PixelFont(); var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 5, 5)); byte[] pixels = Enumerable.Repeat((byte)7, 25).ToArray(); var state = new GumpGlyphColorState(); GumpGlyphHooks h = GumpTextDraw.MakeGumpGlyphPixelHooks(new(pixels, 5, 5, clip), state);
        GumpTextDraw.DrawBitmapGlyphText([65, 0], font, new(2, 2), new(6, 0, 211, 0, 0, 33, 1, 1), h); Assert.Equal(211, pixels[12]);
        // 독립 손계산한 네 이웃과 오른쪽 아래 그림자다.
        foreach (int at in new[] { 7, 13, 17, 11, 18 }) Assert.Equal(33, pixels[at]);
        Assert.Equal(211, state.VfxColors[100]); byte[] before = (byte[])pixels.Clone(); GumpTextDraw.DrawBitmapGlyphText([0], null, new(), new(6, 0, 77, 0, 0, 55, 1, 1), h); Assert.Equal(before, pixels); Assert.Equal(77, state.VfxColors[100]);
    }

    /// <summary>명시 길이의NUL도 출력하고 잘못된 범위는 색/픽셀 변경 전에 거부한다.</summary>
    [Fact]
    public void ExplicitNulAndInvalidRanges()
    {
        BitmapFont font = GumpFontFixture.PixelFont(); var colors = new List<byte>(); var points = new List<ScreenPoint>(); var h = new GumpGlyphHooks { Color = c => colors.Add(c), Glyph = (_, _, at) => points.Add(at) }; byte[] bytes = [65, 0, 65]; var context = new GumpTextContext { Color = 99 };
        GumpTextDraw.DrawBitmapGlyphText(bytes, font, new(), context, h, 3); Assert.Equal(new[] { new ScreenPoint(0, 0), new(2, 0), new(3, 0) }, points); points.Clear(); colors.Clear();
        Assert.Throws<ArgumentException>(() => GumpTextDraw.DrawBitmapGlyphText([65], font, new(), context, h)); Assert.Throws<ArgumentException>(() => GumpTextDraw.DrawBitmapGlyphText(bytes, font, new(), context, h, 4)); Assert.Throws<ArgumentException>(() => GumpTextDraw.DrawBitmapGlyphText(bytes, font, new(), context, h, -2)); Assert.Empty(points); Assert.Empty(colors);
    }

    /// <summary>원본처럼 Position 뒤 기본/명시 참조 문맥을 읽고 빈 슬롯도 현재 색은 설치한다.</summary>
    [Fact]
    public void ProjectionReadsCurrentContexts()
    {
        var fixture = new GumpFontFixture("gumpglyph"); var node = new GumpCreateNode { Flags = 0x2000, Parent = 1, Object = 50 }; node.TextContext[1] = 2; node.TextContext[2] = 11; var explicitContext = new GumpTextContext { Font = 2, Color = 33 };
        var collection = new GumpCollection(new() { Root = 1 }, new() { Node = _ => node, Project = sid => { Assert.Equal(50U, sid); node.TextContext[2] = 77; explicitContext.Color = 55; return new(4, 8); } });
        var installed = new List<byte>(); var h = new GumpGlyphHooks { Color = c => installed.Add(c), Glyph = (_, _, _) => Assert.Fail("빈 슬롯의 글리프 출력 금지") }; var draw = new GumpTextDraw(fixture.Fonts, h);
        draw.DrawGump(collection, 2, [65, 0], new(2, 3), node.TextContext); draw.DrawGump(collection, 2, [65, 0], new(2, 3), node.TextContext, in explicitContext); Assert.Equal(new byte[] { 77, 55 }, installed);
    }
}
