using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 돌 버튼 라벨/글꼴/배경/입력/수명 통합 검사다.

/// <summary>cpppj/tests/GumpTextTests.cpp의 돌 버튼 계약을 같은 C# 저장소에 연결한다. 공통 CRT 정렬/분배는 R5에서 잇는다.</summary>
public sealed class StoneButtonGumpTests
{
    /// <summary>원본 WriteStonePixelFont 입력의 한 픽셀A·측정/출력 폭2·실제 메트릭14/11/3을 캐시로 공급한다.</summary>
    private static FontStore PixelFonts()
    {
        byte[] cache = GumpFontFixture.Header(14); GumpFontFixture.Put(cache, 24, 11); GumpFontFixture.Put(cache, 28, 3); GumpFontFixture.Put(cache, 36 + 65 * 4, 2); GumpFontFixture.Put(cache, 36 + 2048 + 65 * 4, 2); GumpFontFixture.Put(cache, 36 + 3072 + 65 * 4, 43);
        byte[] shape = new byte[40]; GumpFontFixture.Put(shape, 0, 0x30312e31); GumpFontFixture.Put(shape, 4, 1); GumpFontFixture.Put(shape, 8, 16); byte[] data = [.. cache, .. shape, 2, 100, 0];
        return new(path => path == "d/!Arial.normal.14.700.chfnt" ? data : null, "Arial", (_, _, _, _) => throw new InvalidOperationException("픽셀 검사 캐시 누락"));
    }

    /// <summary>기본/미작성 문맥·라벨 꼬리·실제 측정→가상Attach→자식 우선Cleanup을 두 판본에서 연결한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void FontMeasureAttachAndChildFirstDestroy(OriginalEdition edition)
    {
        var fixture = new GumpFontFixture("gumptext"); var s = new StoneButtonFixture(new(fixture.Fonts, edition), edition); uint[] context = [2, 5, 7, 4, 0, 9, 1, 1]; context.CopyTo(s.State.DefaultContext, 0);
        // 생성자가 쓰지 않는 DWORD와 라벨 꼬리를 독립 입력으로 남긴다.
        for (int i = 0; i < s.Node.Contexts.Length; i++) s.Node.Contexts[i] = new(0, 0, 0, 0, 0x81726354, 29, 37, 43);
        s.Node.Label[63] = 90; s.Node.TextWidth = 81; s.Node.TextHeight = 82; Assert.Equal(2U, s.Stones.Construct(2, 75, 19, [0], uint.MaxValue));
        Assert.Equal(3U, s.Create.Count); Assert.Equal(3U, s.Create.Serial); Assert.Equal(3U, s.Node.Background); Assert.Equal(90, s.Node.Label[63]); Assert.Equal((81, 82), (s.Node.TextWidth, s.Node.TextHeight)); Assert.Equal(0U, s.Node.ButtonFlags); Assert.Equal(0xfffffff9U, s.Node.Command);
        Assert.Equal(0x81726354U, s.Node.Contexts[0].Extra); Assert.Equal((37, 43), (s.Node.Contexts[2].ShadowX, s.Node.Contexts[2].ShadowY)); Assert.Equal(2U, s.Background.Parent); Assert.Equal((73f, 17f), (s.Background.Width, s.Background.Height)); Assert.Equal(context, s.Node.TextContext);
        s.Stones.FitToText(2); Assert.Equal((1, 15), (s.Node.TextWidth, s.Node.TextHeight)); Assert.Equal((13f, 21f), (s.Node.Width, s.Node.Height)); Assert.Equal((73f, 17f), (s.Background.Width, s.Background.Height));
        s.Tree.Move(2, 40, 50, 1); Assert.Equal(1U, s.Node.Parent); Assert.Equal((40f, 50f), (s.Node.X, s.Node.Y)); Assert.Equal((11f, 19f), (s.Background.Width, s.Background.Height)); Assert.Equal(unchecked(s.Node.Depth + 1), s.Background.Depth); Assert.Equal(2U, s.Root.Child);
        s.Lifetime.Kill(2); Assert.Equal(2U, s.LifeState.KillCount); s.Lifetime.Cleanup(); Assert.Equal(1U, s.Create.Count); Assert.Equal(1U, s.Create.Entries[0]); Assert.Equal(new uint[] { 3, 2 }, s.Released);
    }

    /// <summary>라벨 변경은 측정만 한다. null 생성/짧은 복사/긴 라벨의 미작성 마지막 바이트를 구별한다.</summary>
    [Fact]
    public void LabelsDoNotResizeOrInventTerminator()
    {
        var fixture = new GumpFontFixture("gumptext"); var s = new StoneButtonFixture(new(fixture.Fonts)); s.Node.Label.AsSpan().Fill(90); s.Stones.Construct(2, 75, 19, null); Assert.Equal(0, s.Node.Label[0]); Assert.Equal(90, s.Node.Label[1]); Assert.Equal(90, s.Node.Label[63]);
        s.Stones.SetLabel(2, [0]); Assert.Equal((0, 14), (s.Node.TextWidth, s.Node.TextHeight)); Assert.Equal(75, s.Node.Width); Assert.Equal(73, s.Background.Width); Assert.Equal(0, s.Node.Label[62]); Assert.Equal(90, s.Node.Label[63]);
        string[] native = fixture.Cases[0]; Assert.Equal(new[] { "originals", "0", "0", "0", "0" }, new[] { native[1], native[3], native[4], native[5], native[9] }); int expected = X86Fixture.Int(native[11]);
        s.Stones.SetLabel(2, [65, 98, 0]); Assert.Equal((expected, 14), (s.Node.TextWidth, s.Node.TextHeight)); Assert.Equal(75, s.Node.Width);
        Assert.Throws<ArgumentException>(() => s.Stones.SetLabel(2, Enumerable.Repeat((byte)65, 80).ToArray())); Assert.Equal(65, s.Node.Label[62]); Assert.Equal(90, s.Node.Label[63]); Assert.Equal((expected, 14), (s.Node.TextWidth, s.Node.TextHeight));
    }

    /// <summary>할당 실패 전에 완료한 부모 등록/라벨 쓰기는 되돌리지 않는다.</summary>
    [Fact]
    public void AllocationFailurePreservesRegisteredParent()
    {
        var s = new StoneButtonFixture(new(PixelFonts())) { FailAllocation = true }; Assert.Throws<InvalidOperationException>(() => s.Stones.Construct(2, 75, 19, "Fail\0"u8.ToArray())); Assert.Equal(2U, s.Create.Count); Assert.Equal(0U, s.Node.Background); Assert.Equal(70, s.Node.Label[0]); Assert.Equal(2U, s.Create.Entries[s.Node.Id]); Assert.Equal(0U, s.Node.Parent);
    }

    /// <summary>현재 저장 Draw 문맥과 눌림/비활성/일시클립이 실제 한 픽셀 글자를 원본 위치에 출력한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void StoredContextsAndPixels(OriginalEdition edition)
    {
        FontStore fonts = PixelFonts(); var s = new StoneButtonFixture(new(fonts, edition), edition); s.Stones.Construct(2, 75, 19, [65, 0]); s.Stones.SetLabel(2, [65, 0]); s.Tree.Move(2, 10, 10, 1); s.Node.Flags |= GumpDraw.DrawClip; s.Node.TextContext[1] = 2;
        var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 100, 40)); byte[] guarded = Enumerable.Repeat((byte)7, 4002).ToArray(); Memory<byte> pixels = guarded.AsMemory(1, 4000); var target = new GumpPixelTarget(pixels, 100, 40, clip); var colors = new GumpGlyphColorState(); var text = new GumpTextDraw(fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(target, colors), edition);
        var fallback = new List<uint>(); var hooks = new GumpDrawHooks { Draw = t => fallback.Add(t) }; StoneButtonGump.BindStoneButtonDraw(hooks, s.Collection, text, target, t => t == 2 ? s.Stones : null); var draw = new GumpDraw(s.Collection, clip, hooks); draw.Draw(3); draw.Draw(2); Assert.Equal(new uint[] { 3 }, fallback);
        Assert.Equal(243, pixels.Span[1247]); Assert.Equal(9, pixels.Span[1348]); Assert.Equal(57, pixels.Span[1011]); Assert.Equal(243, colors.VfxColors[100]); Assert.Equal(new ScreenRect(0, 0, 100, 40), clip.Clip); Assert.Equal(99, clip.VfxClip.Right); Assert.Equal(7, guarded[0]); Assert.Equal(7, guarded[^1]);
        pixels.Span.Fill(7); s.Node.Pressed = 1; draw.Draw(2); Assert.Equal(243, pixels.Span[1348]); Assert.Equal(9, pixels.Span[1449]); Assert.Equal(7, pixels.Span[1247]);
        pixels.Span.Fill(7); s.Node.Pressed = 0; s.Node.ButtonFlags |= 1; draw.Draw(2); Assert.Equal(9, pixels.Span[1247]); Assert.Equal(7, pixels.Span[1348]); Assert.Equal(9, colors.VfxColors[100]);
        pixels.Span.Fill(7); GumpDraw.SetGumpDrawClip(clip, new(47, 12, 48, 13)); s.Node.ButtonFlags = 0; draw.Draw(2); Assert.Equal(243, pixels.Span[1247]); Assert.Equal(1, pixels.Span.ToArray().Count(p => p == 243)); Assert.Equal(new ScreenRect(47, 12, 48, 13), clip.Clip);
    }

    /// <summary>라벨 범위 실패에도 임시 클립을 복구하며 앞서 출력한 부모 선은 보존한다.</summary>
    [Fact]
    public void DrawFailureRestoresClipAfterLines()
    {
        FontStore fonts = PixelFonts(); var s = new StoneButtonFixture(new(fonts)); s.Stones.Construct(2, 75, 19, [65, 0]); s.Tree.Move(2, 10, 10, 1); s.Node.Flags |= GumpDraw.DrawClip; s.Node.Label.AsSpan().Fill(65);
        var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 100, 40)); byte[] pixels = Enumerable.Repeat((byte)7, 4000).ToArray(); var target = new GumpPixelTarget(pixels, 100, 40, clip); var colors = new GumpGlyphColorState(); var text = new GumpTextDraw(fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(target, colors)); var h = new GumpDrawHooks(); StoneButtonGump.BindStoneButtonDraw(h, s.Collection, text, target, _ => s.Stones); var draw = new GumpDraw(s.Collection, clip, h);
        Assert.Throws<ArgumentException>(() => draw.Draw(2)); Assert.Equal(new ScreenRect(0, 0, 100, 40), clip.Clip); Assert.Equal(99, clip.VfxClip.Right); Assert.Equal(57, pixels[1011]); Assert.Equal(0, colors.VfxColors[100]);
    }

    /// <summary>실제 자산→배경/라벨→현재 입력 눌림/뗌→부모 통지→자식 우선 삭제를 같은 두 판본에서 연결한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void BackgroundGlyphInputAndLifetime(OriginalEdition edition)
    {
        FontStore fonts = PixelFonts(); var assets = new GumpBackgroundAssets(GumpBackgroundTestAssets.Shapes(), [new(82, 0, 1, 0)]); var s = new StoneButtonFixture(new(fonts, edition), edition, assets); s.Stones.Construct(2, 75, 19, [65, 0]); s.Stones.SetLabel(2, [65, 0]); s.Tree.Move(2, 10, 10, 1);
        Assert.Equal((82U, 82U, 3, 3), (s.Background.Type, s.Background.Shape, s.Background.TileWidth, s.Background.TileHeight));
        var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 100, 40)); byte[] guarded = Enumerable.Repeat((byte)7, 4002).ToArray(); Memory<byte> pixels = guarded.AsMemory(1, 4000); var target = new GumpPixelTarget(pixels, 100, 40, clip); var colors = new GumpGlyphColorState(); var text = new GumpTextDraw(fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(target, colors), edition);
        var raster = new GumpBackgroundPixelTarget(target, assets, GumpBackgroundTestAssets.Maps(), colors.VfxColors); var bg = new GumpBackgroundDrawHooks { Node = t => t == 3 ? s.Background : null }; GumpBackgroundAssets.BindGumpBackgroundAssets(new(), bg, assets); var background = new GumpBackgroundDraw(s.Collection, clip, new(), raster.Hooks(clip, bg));
        var h = new GumpDrawHooks { Draw = _ => Assert.Fail("없는 가상Draw") }; StoneButtonGump.BindStoneButtonDraw(h, s.Collection, text, target, t => t == 2 ? s.Stones : null); GumpBackgroundDraw.BindGumpBackgroundDraw(h, background, t => t == 3); var draw = new GumpDraw(s.Collection, clip, h); var work = new RenderCollectionEntry[4]; Assert.Equal(2, s.Collection.Collect(new(0, 0, 100, 40), work, 4));
        // 이 검사의 명시 출력 입력은 배경→부모다. CRT 정렬/공통kind 분배를 대신 구현하거나 완료로 세지 않는다.
        draw.Draw(3); draw.Draw(2); Assert.Equal(4, pixels.Span[1513]); Assert.Equal(14, pixels.Span[1113]); Assert.Equal(22, pixels.Span[2613]); Assert.Equal(243, pixels.Span[1247]); Assert.Equal(9, pixels.Span[1348]); Assert.Equal(57, pixels.Span[1011]); Assert.Equal(243, colors.VfxColors[100]);
        Assert.Equal(2U, s.Input.Route(InputCode.LeftButton, 13, 15)); Assert.NotEqual(0U, s.Background.BackgroundFlags & GumpBackgroundFlags.Pressed); pixels.Span.Fill(7); draw.Draw(3); draw.Draw(2);
        Assert.Equal(24, pixels.Span[1113]); Assert.Equal(12, pixels.Span[2613]); Assert.Equal(243, pixels.Span[1348]); Assert.Equal(9, pixels.Span[1449]); Assert.Equal(new ScreenRect(0, 0, 100, 40), clip.Clip); Assert.Equal(7, guarded[0]); Assert.Equal(7, guarded[^1]);
        Assert.Equal(2U, s.Input.Route(InputCode.Release | InputCode.LeftButton, 13, 15)); Assert.Equal(0U, s.LifeState.Focus); Assert.Equal(0U, s.Background.BackgroundFlags & GumpBackgroundFlags.Pressed); Assert.Contains((0x12346U, s.Node.Serial, 0U), s.Notifications);
        s.Lifetime.Kill(2); s.Lifetime.Cleanup(); Assert.Equal(new uint[] { 3, 2 }, s.Released); Assert.Equal(1U, s.Create.Count); Assert.Equal(0, s.Collection.Collect(new(0, 0, 100, 40), work, 4));
    }

    /// <summary>측정 콜백이 바꾼 현재scale와 출력 DWORD의 감김을 실제FitToText가 사용한다.</summary>
    [Fact]
    public void FitUsesCurrentScaleAndDwordWrap()
    {
        var s = new StoneButtonFixture(new(PixelFonts())); s.Stones.Construct(2, 75, 19, [0]);
        s.StoneHooks.Measure = (uint t, byte[] label, uint[] context, ref int width, ref int height, uint length) => { Assert.Equal(2U, t); Assert.Equal(uint.MaxValue, length); width = int.MaxValue; height = int.MaxValue; s.Node.Scale = .5f; };
        s.Stones.FitToText(2); Assert.Equal((float)unchecked(int.MaxValue + 12), s.Node.BaseWidth); Assert.Equal((float)((double)unchecked(int.MaxValue + 12) * .5f), s.Node.Width); Assert.Equal((float)unchecked(int.MaxValue + 6), s.Node.BaseHeight); Assert.Equal(73, s.Background.Width);
    }
}
