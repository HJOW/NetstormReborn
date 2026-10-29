using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 원본 MetaImage의 독립 기하 관찰과 실제 동적 그림/공통 수명 검사다.

/// <summary>실제 RawImage를 빌리는 경로를 같은 원문 상태/효과와 비교한다.</summary>
public sealed class MetaImageGumpTests
{
    /// <summary>원문float32의 현재 비트.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>고정색 실제 bitmap을 소비하는 동적 Image 입력.</summary>
    private static RawImage Image(int width = 6, int height = 4, byte color = 7) { var image = new RawImage(); image.Convert(new(width, height, Enumerable.Repeat(color, width * height).ToArray()), false); return image; }
    /// <summary>독립 원본326행의 생성/기하·등록/현재Image/가상효과순서를대조한다.</summary>
    [Fact]
    public void All326OriginalConstructorGeometryAndDrawObservationsMatch()
    {
        int count = 0;
        // 앞12열의 입력만 사용하고 뒤 관측은 비교에만 사용한다.
        foreach (string[] row in X86Fixture.Read("meta-image"))
        {
            if (row[0] == "op") continue; Assert.Equal(24, row.Length); int width = X86Fixture.Int(row[1]), height = X86Fixture.Int(row[2]); var image = new RawImage(); if (X86Fixture.UInt(row[9]) != 0 && width > 0 && height > 0) image.Convert(new(width, height, Enumerable.Repeat((byte)7, width * height).ToArray()), false); var node = new MetaImageGumpNode { Image = image, ColorMode = X86Fixture.UInt(row[6]), Scale = X86Fixture.FloatBits(row[3]), BaseWidth = X86Fixture.FloatBits(row[4]), BaseHeight = X86Fixture.FloatBits(row[5]) }; var create = new GumpCreateState(); var factory = new GumpCreate(create, new() { Node = token => token == 1 ? node : null }); var trace = new List<string>();
            var hooks = new MetaImageGumpHooks
            {
                Node = token => token == 1 ? node : null, Position = _ => { trace.Add("position"); return new(X86Fixture.Int(row[7]), X86Fixture.Int(row[8])); },
                Attach = (_, x, y, parent, mode) => { trace.Add($"attach:{Bits(x)}:{Bits(y)}:{parent}:{mode}"); node.BaseWidth = X86Fixture.FloatBits(row[4]); node.BaseHeight = X86Fixture.FloatBits(row[5]); if (X86Fixture.UInt(row[10]) != 0) image.Convert(new(37, 19, Enumerable.Repeat((byte)7, 37 * 19).ToArray()), false); }, Color = token => trace.Add("color:" + token),
                Draw = request => { Assert.Same(image, request.Image); trace.Add($"draw:{(request.Scaled ? "scaled" : request.Mapped ? "mapped" : "plain")}:{unchecked((uint)request.Position.X)}:{unchecked((uint)request.Position.Y)}:{unchecked((uint)request.Scale)}"); }
            }; var body = new MetaImageGump(create, factory, hooks);
            switch (row[0]) { case "Construct": body.Construct(1, image); break; case "Resize": body.Resize(1); break; case "Attach": body.Attach(1, 1.25f, -2.5f, 0xaabbccdd, 0x80000000); break; case "Draw": body.Draw(1); break; default: throw new ArgumentException("MetaImage 원본 진입 이름"); }
            uint[] actual = [Bits(node.Scale), Bits(node.BaseWidth), Bits(node.BaseHeight), Bits(node.Width), Bits(node.Height), node.Command, node.Vtable, node.Flags, create.Count, create.Serial]; Assert.Equal(row.Skip(12).Take(10).Select(X86Fixture.UInt), actual); Assert.Equal(row[22], trace.Count == 0 ? "-" : string.Join(';', trace)); Assert.True(X86Fixture.UInt(row[23]) > 0); count++;
        }
        Assert.Equal(326, count);
    }
    /// <summary>원문동적shape의현재내용을실제표면/공유색표/클립에매번다시그린다.</summary>
    [Fact]
    public void ActualRawShapePixelsSharedMapClipAndReplacement()
    {
        byte[] pixels = Enumerable.Repeat((byte)0x5a, 643).ToArray(), current = new byte[256], disabled = Enumerable.Range(0, 256).Select(v => unchecked((byte)(v + 30))).ToArray(); var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 32, 20)); var target = new GumpPixelTarget(pixels, 32, 20, clip); var raw = Image(); var node = new MetaImageGumpNode(); var create = new GumpCreateState(); var factory = new GumpCreate(create, new() { Node = _ => node }); var hooks = MetaImageGump.BindMetaImagePixels(new() { Node = _ => node, Position = _ => new(2, 3) }, target, current, disabled); var body = new MetaImageGump(create, factory, hooks); body.Construct(2, raw); body.Draw(2); Assert.Equal(7, pixels[4 * 32 + 3]); Assert.Equal(7, pixels[7 * 32 + 8]); Assert.Equal(0x5a, pixels[7 * 32 + 9]); node.ColorMode = 1; Array.Fill(pixels, (byte)0x5a); body.Draw(2); Assert.Equal(37, pixels[4 * 32 + 3]); Assert.Equal(disabled, current); disabled[7] = 42; body.Draw(2); Assert.Equal(42, pixels[4 * 32 + 3]); raw.Convert(new(2, 1, new byte[] { 8, 0 }), false); node.ColorMode = 0; Array.Fill(pixels, (byte)0x5a); body.Draw(2); Assert.Equal(8, pixels[4 * 32 + 3]); Assert.Equal(0x5a, pixels[4 * 32 + 4]); GumpDraw.SetGumpDrawClip(clip, new(3, 3, 4, 4)); Array.Fill(pixels, (byte)0x5a); body.Draw(2); Assert.All(pixels, color => Assert.Equal(0x5a, color)); Assert.Throws<ArgumentOutOfRangeException>(() => hooks.Color!(0));
    }
    /// <summary>배율그림은colorMode와무관한plain변환이고정확한단위만mapped를사용한다.</summary>
    [Fact]
    public void ActualScaledImageIgnoresColorModeAndPreservesPadding()
    {
        byte[] pixels = Enumerable.Repeat((byte)0x5a, 323).ToArray(), current = Enumerable.Repeat((byte)99, 256).ToArray(); var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 16, 20)); var node = new MetaImageGumpNode { Image = Image(), Scale = .5f, ColorMode = 1 }; var create = new GumpCreateState(); var hooks = MetaImageGump.BindMetaImagePixels(new() { Node = _ => node, Position = _ => new(5, 6) }, new(pixels, 16, 20, clip), current, Enumerable.Repeat((byte)88, 256).ToArray()); var body = new MetaImageGump(create, new(create, new() { Node = _ => node }), hooks); body.Draw(2); Assert.Contains((byte)7, pixels); Assert.DoesNotContain((byte)88, pixels); Assert.All(current, c => Assert.Equal(99, c)); Assert.All(pixels.AsSpan(320).ToArray(), c => Assert.Equal(0x5a, c)); node.Scale = 0; Array.Fill(pixels, (byte)0x5a); body.Draw(2); Assert.All(pixels, c => Assert.Equal(0x5a, c));
    }
    /// <summary>기본Attach와위치/색표callback중바뀐현재Image/기하를재조회한다.</summary>
    [Fact]
    public void CurrentImageAfterBaseAttachPositionAndColorEffects()
    {
        RawImage first = Image(), second = Image(13, 7), third = Image(3, 2); var n = new MetaImageGumpNode { Image = first }; var create = new GumpCreateState(); var requests = new List<MetaImageDrawRequest>(); var h = new MetaImageGumpHooks { Node = _ => n, Attach = (_, _, _, _, _) => { n.Image = second; n.BaseWidth = 6.5f; n.BaseHeight = 3.5f; }, Position = _ => { n.Image = third; return new(10, 20); }, Color = _ => { n.Image = second; n.ColorMode = 0; }, Draw = r => requests.Add(r) }; var body = new MetaImageGump(create, new(create, new() { Node = _ => n }), h); body.Attach(2, 0, 0, 1, 0); Assert.Equal((.5f, 3.25f, 1.75f), (n.Scale, n.Width, n.Height)); body.Draw(2); Assert.Equal(new MetaImageDrawRequest(third, new(11, 21), 32768, true, false), requests[0]); n.Scale = 1; n.ColorMode = 1; body.Draw(2); Assert.Equal(new MetaImageDrawRequest(second, new(23, 27), 65536, false, false), requests[1]);
    }
    /// <summary>빈Image는위치조회/색/그리기없이반환하고없는Image/판본은명시적으로거부한다.</summary>
    [Fact]
    public void EmptyShapeDoesNotReachDevicesAndNullImageRemainsExplicit()
    {
        var create = new GumpCreateState(); var n = new MetaImageGumpNode { Image = new() }; var factory = new GumpCreate(create, new() { Node = _ => n }); var body = new MetaImageGump(create, factory, new() { Node = _ => n }); body.Draw(2); n.Image = null; Assert.Throws<InvalidOperationException>(() => body.Draw(2)); Assert.Throws<InvalidOperationException>(() => body.Resize(2)); var cd = new GumpCreateState(); cd.Collection.Edition = OriginalEdition.Cd1072; Assert.Throws<ArgumentException>(() => new MetaImageGump(cd, factory, new())); Assert.Throws<ArgumentException>(() => MetaImageGump.BindMetaImagePixels(new(), new(new byte[4], 2, 2, new()), new byte[255], new byte[256]));
    }
    /// <summary>같은실제공통생성/트리/가상Draw/기본수명은빌린Image를반납하지않는다.</summary>
    [Fact]
    public void ActualCommonTreeDrawAndBorrowedImageLifetime()
    {
        var scene = new ScrollWidgetScene(); var n = new MetaImageGumpNode(); scene.Nodes.Add(30, n); var image = Image(); var hooks = MetaImageGump.BindMetaImagePixels(new() { Node = token => token == 30 ? n : null, Attach = scene.Tree.Attach, Position = scene.Collection.Position }, new(scene.Pixels, 64, 128, scene.Clip), new byte[256], new byte[256]); var body = new MetaImageGump(scene.Create, scene.Factory, hooks); MetaImageGump.BindMetaImageGump(scene.TreeHooks, scene.DrawHooks, t => t == 30 ? body : null); Assert.Equal(30U, body.Construct(30, image)); Assert.Equal((0f, 0f), (n.BaseWidth, n.BaseHeight)); body.Resize(30); Assert.Equal((7f, 5f), (n.BaseWidth, n.BaseHeight)); scene.TreeHooks.Attach!(30, 2, 3, 1, 0); Assert.Equal((1U, 1f), (n.Parent, n.Scale)); scene.Pixels.Span.Fill(0x5a); scene.Draw.Draw(30); Assert.Equal(7, scene.Pixels.Span[4 * 64 + 3]); Assert.Equal(0x5a, scene.Pixels.Span[3 * 64 + 2]); scene.Lifetime.Kill(30); scene.Lifetime.Cleanup(); Assert.Equal(1U, scene.Create.Count); Assert.Contains(30U, scene.Released); Assert.False(image.Shape().IsEmpty); Assert.Same(image, n.Image); image.Destroy(); Assert.True(n.Image!.Shape().IsEmpty);
    }
}
