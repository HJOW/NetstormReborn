using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using Scene = Netstorm.Tests.GumpBackgroundTestAssets.Scene;

namespace Netstorm.Tests; // 전체 원본 배경 사건과 실제8비트 출력·생성 수명을 대조한다.

/// <summary>배경 분배/픽셀/자산의 원본 기록과 C++ 독립 계약이다.
/// 근거: cpppj/tests/GumpBackgroundDrawTests.cpp. 선택 실자산 Inspector 실행은 후속 자산 조립에서 수행한다.</summary>
public sealed class GumpBackgroundDrawTests
{
    /// <summary>signed 인자도 원본 DWORD의 부호 없는 관측 형식으로 기록한다.</summary>
    private static string Event(char kind, params uint[] values) => kind + "," + string.Join(',', values);
    /// <summary>화면 영역의 네 DWORD 비트를 그대로 기록한다.</summary>
    private static uint[] Coordinates(ScreenRect r) => [unchecked((uint)r.Left), unchecked((uint)r.Top), unchecked((uint)r.Right), unchecked((uint)r.Bottom)];
    /// <summary>fixture의 쉼표 signed 클립을 읽는다.</summary>
    private static ScreenRect Rect(string text) { int[] a = text.Split(',').Select(X86Fixture.Int).ToArray(); return new(a[0], a[1], a[2], a[3]); }

    /// <summary>판본별209행의 전체 사건·debug/shade·PE의768바이트 fill 픽셀을 전수 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void NativeBodiesAndSoftwarePixels(string edition)
    {
        string[][] rows = X86Fixture.Read("gumpbackgrounddraw", edition); Assert.Equal(209, rows.Length);
        // 매 입력은 실제 기본 루트/같은 파생 노드/새 보호 표면을 쓴다.
        foreach (string[] r in rows)
        {
            Assert.Equal(20, r.Length); var s = new Scene(edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            s.Node.X = X86Fixture.Int(r[2]); s.Node.Y = X86Fixture.Int(r[3]); s.Node.Width = X86Fixture.FloatBits(r[4]); s.Node.Height = X86Fixture.FloatBits(r[5]);
            s.Node.BackgroundFlags = X86Fixture.UInt(r[6]); s.Node.Type = X86Fixture.UInt(r[7]); s.Node.Frame = X86Fixture.UInt(r[8]); s.Node.Shape = 0xdeadbeef; s.Node.TileWidth = 123; s.Node.TileHeight = 456;
            var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, Rect(r[11]));
            var state = new GumpBackgroundDrawState { Debug = X86Fixture.UInt(r[12]) != 0, DebugColor = X86Fixture.UInt(r[13]), NearColor = X86Fixture.UInt(r[14]), FarColor = X86Fixture.UInt(r[15]) };
            byte[] guarded = Enumerable.Repeat((byte)77, 770).ToArray(); Memory<byte> pixels = guarded.AsMemory(1, 768);
            // 원본 PE에 공급한 명시 초기 픽셀을 사용한다.
            for (int i = 0; i < pixels.Length; i++) pixels.Span[i] = unchecked((byte)(i * 19 + 37));
            var target = new GumpPixelTarget(pixels, 32, 24, clip); var events = new List<string>(); uint current = 0;
            var hooks = new GumpBackgroundDrawHooks
            {
                Node = token => token == 2 ? s.Node : null,
                Frame = (type, frame) => { events.Add(Event('T', type, frame)); return new() { Width = (short)X86Fixture.Int(r[9]), Height = (short)X86Fixture.Int(r[10]) }; },
                Clip = area => { events.Add(Event('C', Coordinates(area))); GumpDraw.SetGumpDrawClip(clip, area); },
                Shade = shade => { current = shade; events.Add(Event('M', shade)); },
                Shape = q => events.Add(Event('D', q.Type, q.Frame, unchecked((uint)q.Position.X), unchecked((uint)q.Position.Y), q.Mapped ? current : 0)),
                Fill = (area, color, checker) => { events.Add(Event('F', [.. Coordinates(area), color, checker ? 1U : 0U])); target.Fill(area, color, checker); },
                Line = l => events.Add(Event('L', unchecked((uint)l.First.X), unchecked((uint)l.First.Y), unchecked((uint)l.Last.X), unchecked((uint)l.Last.Y), l.Color)),
            };
            var draw = new GumpBackgroundDraw(s.Collection, clip, state, hooks);
            if (r[1] == "Virtual") draw.Draw(2);
            else draw.Background(new(X86Fixture.Int(r[2]), X86Fixture.Int(r[3]), (int)((double)s.Node.X + s.Node.Width), (int)((double)s.Node.Y + s.Node.Height)), s.Node.BackgroundFlags, s.Node.Type, s.Node.Frame);
            Assert.Equal(r[16], events.Count == 0 ? "-" : string.Join(';', events)); Assert.Equal(X86Fixture.UInt(r[17]), state.DebugColor); Assert.Equal(X86Fixture.UInt(r[18]), current);
            Assert.Equal(X86Fixture.Bytes(r[19]), pixels.Span.ToArray()); Assert.Equal(77, guarded[0]); Assert.Equal(77, guarded[^1]); Assert.Equal(Rect(r[11]), clip.Clip);
        }
    }

    /// <summary>실제 헤더 공급은 두 판본 생성자와 같은 안정된 타입을 쓰고 type0의 미작성 추가 필드를 보존한다.</summary>
    [Fact]
    public void AssetHeadersBindBothConstructors()
    {
        GumpBackgroundAssets assets = GumpBackgroundTestAssets.Assets();
        // 원본 패치/CD 생성 순서만 다르고 같은 실제 자산을 사용한다.
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            var s = new Scene(edition); var create = new GumpBackgroundCreateHooks { Node = token => token == 2 ? s.Node : null }; var draw = new GumpBackgroundDrawHooks();
            GumpBackgroundAssets.BindGumpBackgroundAssets(create, draw, assets); var factory = new GumpBackgroundCreate(s.State, s.Factory, () => -7, create);
            Assert.Equal(2U, factory.Construct(2, 13, 9, GumpBackgroundFlags.Button, 1, 83, 1)); Assert.Equal(83U, s.Node.Shape); Assert.Equal(5, s.Node.TileWidth); Assert.Equal(4, s.Node.TileHeight); Assert.Equal(0xfffffff9U, s.Node.Command);
            Assert.Equal(13, s.Node.Width); Assert.Equal(9, s.Node.Height); Assert.Equal(2U, s.State.Count);
            GumpBackgroundType stable = create.Type!(83)!; GumpBackgroundFrame header = create.Frame!(stable.Token, 0, 1)!; Assert.Same(stable, create.Type(83)); Assert.Same(header, create.Frame(83, 0, 1)); Assert.NotSame(stable, create.Type(82));
            factory.Construct(2, 11, 8, 0, 0x100000, 0, 0); Assert.Equal(83U, s.Node.Shape); Assert.Equal(5, s.Node.TileWidth); Assert.Equal(4, s.Node.TileHeight); Assert.Equal(0U, s.Node.Type);
        }
    }

    /// <summary>콜백이 바꾼 현재 frame/shape와 signed 색/치수를 판본별 쓰기 시점에 읽는다. 실패 전 기본 등록도 남는다.</summary>
    [Fact]
    public void ConstructorCurrentFieldsAndPartialFailure()
    {
        // CD는 frame 조회 전에 shape를 저장하고 패치는 조회 뒤 현재 shape를 읽는다.
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            var s = new Scene(edition); var type = new GumpBackgroundType { Token = 93, Shape = 17 }; sbyte color = -128;
            var h = new GumpBackgroundCreateHooks { Node = t => t == 2 ? s.Node : null, Type = t => { Assert.Equal(82U, t); s.Node.Frame = 7; return type; }, Frame = (t, a, f) => { Assert.Equal((93U, 0U, 7U), (t, a, f)); type.Shape = 19; return new() { Width = -32768, Height = -3 }; } };
            var factory = new GumpBackgroundCreate(s.State, s.Factory, () => color, h); factory.Construct(2, 5, 6, 11, 1, 82, 4);
            Assert.Equal(edition == OriginalEdition.Patch1078 ? 19U : 17U, s.Node.Shape); Assert.Equal(-32768, s.Node.TileWidth); Assert.Equal(-3, s.Node.TileHeight); Assert.Equal(0xffffff80U, s.Node.Command);
            color = 7; h.Frame = (_, _, _) => throw new InvalidOperationException("프레임 입력 실패"); uint serial = s.State.Serial;
            Assert.Throws<InvalidOperationException>(() => factory.Construct(2, 9, 8, 21, 0x100000, 82, 3)); Assert.Equal(unchecked(serial + 1), s.State.Serial); Assert.Equal(7U, s.Node.Command); Assert.Equal(21U, s.Node.BackgroundFlags); Assert.Equal(-32768, s.Node.TileWidth);
            h.Node = _ => new(); Assert.Throws<InvalidOperationException>(() => factory.Construct(2, 0, 0, 0, 0, 0, 0));
        }
    }

    /// <summary>논리 수·추가 레이어·물리 표와 signed frame 범위/중복 바인딩을 구별한다.</summary>
    [Fact]
    public void AssetFrameContracts()
    {
        GumpBackgroundAssets assets = GumpBackgroundTestAssets.Assets(); Assert.Equal(3, assets.Frame(82, 0, 0).Width); Assert.Equal(5, assets.Frame(83, 0, 1).Width);
        Assert.Throws<ArgumentOutOfRangeException>(() => assets.Frame(82, 0, 1)); Assert.Throws<ArgumentOutOfRangeException>(() => assets.Frame(83, 0, 2)); Assert.Throws<ArgumentOutOfRangeException>(() => assets.Frame(83, 0, uint.MaxValue));
        Assert.Throws<ArgumentException>(() => assets.Frame(83, 1, 0)); Assert.Throws<ArgumentOutOfRangeException>(() => assets.Type(0)); Assert.Throws<ArgumentOutOfRangeException>(() => assets.Type(1000)); ShapeDatabase shapes = GumpBackgroundTestAssets.Shapes();
        Assert.Throws<ArgumentException>(() => new GumpBackgroundAssets(shapes, [new(82, 0, 1, 0), new(82, 0, 1, 0)]));
        // 각각의 잘못된 바인딩은 독립 생성에서 거부한다.
        foreach (GumpBackgroundAssetBinding b in new GumpBackgroundAssetBinding[] { new(0, 0, 1, 0), new(82, 1, 1, 0), new(82, 0, 0, 0), new(82, 0, 3, 0), new(82, -1, 1, 0) }) Assert.Throws<ArgumentException>(() => new GumpBackgroundAssets(shapes, [b]));
    }

    /// <summary>실제 RLE/메타 크기/투명 레이어와 현재 명암표의 비복구를 손계산 픽셀에 대조한다.</summary>
    [Fact]
    public void PixelsUseFreshAssetsAndPersistentShade()
    {
        GumpBackgroundAssets assets = GumpBackgroundTestAssets.Assets(); var s = new Scene(); s.Node.X = s.Node.Y = 1; s.Node.Width = s.Node.Height = 6;
        s.Node.Type = 82; s.Node.BackgroundFlags = GumpBackgroundFlags.Button; s.Node.Shape = 999; s.Node.TileWidth = s.Node.TileHeight = 99;
        var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 8, 8)); var globals = new GumpBackgroundDrawState(); GumpShadeMaps maps = GumpBackgroundTestAssets.Maps(); byte[] colors = Enumerable.Repeat((byte)77, 256).ToArray();
        byte[] guarded = Enumerable.Repeat((byte)9, 66).ToArray(); Memory<byte> pixels = guarded.AsMemory(1, 64); var target = new GumpPixelTarget(pixels, 8, 8, clip); var raster = new GumpBackgroundPixelTarget(target, assets, maps, colors);
        var hooks = new GumpBackgroundDrawHooks { Node = t => t == 2 ? s.Node : null }; GumpBackgroundAssets.BindGumpBackgroundAssets(new(), hooks, assets); var draw = new GumpBackgroundDraw(s.Collection, clip, globals, raster.Hooks(clip, hooks));
        draw.Draw(2); Assert.Equal(4, pixels.Span[27]); Assert.Equal(14, pixels.Span[11]); Assert.Equal(22, pixels.Span[51]); Assert.Equal(maps.Dark, colors); Assert.Equal(9, guarded[0]); Assert.Equal(9, guarded[^1]); Assert.Equal(new ScreenRect(0, 0, 8, 8), clip.Clip); Assert.Equal(7, clip.VfxClip.Right);
        pixels.Span.Fill(9); s.Node.BackgroundFlags |= GumpBackgroundFlags.Pressed; draw.Draw(2); Assert.Equal(24, pixels.Span[11]); Assert.Equal(12, pixels.Span[51]); Assert.Equal(maps.Bright, colors);
        pixels.Span.Fill(9); s.Node.Type = 83; s.Node.Frame = 1; s.Node.BackgroundFlags = GumpBackgroundFlags.Texture; draw.Draw(2);
        Assert.Equal(9, pixels.Span[9]); Assert.Equal(5, pixels.Span[28]); Assert.Equal(maps.Bright, colors); Assert.Equal(999U, s.Node.Shape); Assert.Equal(99, s.Node.TileWidth); Assert.Equal(new ScreenRect(0, 0, 8, 8), clip.Clip);
    }

    /// <summary>flat 우선순위/현재1픽셀 선과 debug 색 증가를 같은 실제 픽셀에서 확인한다.</summary>
    [Fact]
    public void FlatCheckerDebugAndCurrentColors()
    {
        var s = new Scene(); s.Node.X = s.Node.Y = 1; s.Node.Width = s.Node.Height = 6; s.Node.BackgroundFlags = 0x0fe40091;
        var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 8, 8)); byte[] pixels = Enumerable.Repeat((byte)9, 64).ToArray(); var target = new GumpPixelTarget(pixels, 8, 8, clip);
        var globals = new GumpBackgroundDrawState { DebugColor = uint.MaxValue, NearColor = 77, FarColor = 33 }; var hooks = new GumpBackgroundDrawHooks { Node = _ => s.Node, Frame = (_, _) => throw new InvalidOperationException("flat 자산 조회 금지"), Fill = target.Fill, Line = line => target.Lines([line]) };
        var draw = new GumpBackgroundDraw(s.Collection, clip, globals, hooks); draw.Draw(2); Assert.Equal(77, pixels[11]); Assert.Equal(33, pixels[51]); Assert.Equal(9, pixels[27]); Assert.Equal(145, pixels[28]);
        s.Node.BackgroundFlags = GumpBackgroundFlags.Solid | 42; pixels.AsSpan().Fill(9); draw.Draw(2); Assert.Equal(42, pixels[27]); Assert.Equal(9, pixels[0]); globals.Debug = true; draw.Draw(2);
        Assert.Equal(0U, globals.DebugColor); Assert.Equal(255, pixels[28]); Assert.Equal(42, pixels[27]); var palette = new PaletteColorTable(); palette.Basic[7] = 19; palette.Basic[8] = 21; GumpBackgroundDraw.SetGumpBackgroundColors(globals, palette);
        Assert.Equal(19U, globals.NearColor); Assert.Equal(21U, globals.FarColor); Assert.True(globals.Debug); Assert.Equal(0U, globals.DebugColor);
    }

    /// <summary>외부 조회/출력이 바꾼 현재 색·클립을 다음 호출에서 읽고 실패 전에 바뀐 색/횟수는 남긴다.</summary>
    [Fact]
    public void CurrentCallbacksAndTextureFailureRestoreClip()
    {
        var s = new Scene(); var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 8, 8)); var globals = new GumpBackgroundDrawState { NearColor = 11, FarColor = 13 }; var colors = new List<uint>(); uint mapped = 0; int draws = 0;
        var h = new GumpBackgroundDrawHooks { Node = _ => s.Node,
            Line = line => { colors.Add(line.Color); globals.NearColor = 17; globals.FarColor = 19; },
            Frame = (_, _) => { GumpDraw.SetGumpDrawClip(clip, new(2, 2, 6, 6)); return new() { Width = 3, Height = 3 }; }, Clip = r => GumpDraw.SetGumpDrawClip(clip, r), Shade = shade => mapped = shade,
            Shape = _ => { if (++draws == 2) throw new InvalidOperationException("배경 픽셀 출력 실패"); } };
        var draw = new GumpBackgroundDraw(s.Collection, clip, globals, h); draw.Background(new(1, 1, 7, 7), 0x01e00000, 0, 0); Assert.Equal(new uint[] { 11, 17, 19, 19 }, colors);
        Assert.Throws<InvalidOperationException>(() => draw.Background(new(1, 1, 7, 7), GumpBackgroundFlags.Texture | GumpBackgroundFlags.Dark, 82, 0)); Assert.Equal(2, draws); Assert.Equal(2U, mapped); Assert.Equal(new ScreenRect(2, 2, 6, 6), clip.Clip); Assert.Equal(5, clip.VfxClip.Right);
        globals.Debug = true; globals.DebugColor = 7; h.Fill = (_, color, checker) => { Assert.Equal(7U, color); Assert.True(checker); Assert.Equal(8U, globals.DebugColor); }; new GumpBackgroundDraw(s.Collection, clip, globals, h).Draw(2); Assert.Equal(8U, globals.DebugColor);
    }

    /// <summary>초기 clip 설정이 일부 쓴 뒤 실패해도 저장한 논리/VFX 전역으로 복구한다.</summary>
    [Fact]
    public void InitialClipFailureRestores()
    {
        var s = new Scene(); var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 8, 8)); int sets = 0;
        var h = new GumpBackgroundDrawHooks { Node = _ => s.Node, Frame = (_, _) => new() { Width = 3, Height = 3 }, Clip = r => { GumpDraw.SetGumpDrawClip(clip, r); if (++sets == 1) throw new InvalidOperationException("클립 설정 실패"); } };
        var draw = new GumpBackgroundDraw(s.Collection, clip, new(), h); Assert.Throws<InvalidOperationException>(() => draw.Texture(new(1, 1, 7, 7), GumpBackgroundFlags.Texture, 82, 0)); Assert.Equal(2, sets); Assert.Equal(new ScreenRect(0, 0, 8, 8), clip.Clip); Assert.Equal(7, clip.VfxClip.Right);
    }

    /// <summary>invalid 타일은 clip/map/shape 없이 반환하고 현재 분기의 필수 경계와 기존 fallback만 요구한다.</summary>
    [Fact]
    public void ContractsAndFallback()
    {
        var s = new Scene(); var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 8, 8)); var globals = new GumpBackgroundDrawState();
        var h = new GumpBackgroundDrawHooks { Node = t => t == 2 ? s.Node : null, Frame = (_, _) => new() { Width = 1, Height = 3 } }; var draw = new GumpBackgroundDraw(s.Collection, clip, globals, h);
        draw.Background(new(1, 1, 7, 7), GumpBackgroundFlags.Button, 82, 0); Assert.Equal(8, clip.Clip.Right); Assert.Throws<ArgumentException>(() => draw.Background(new(1, 1, 7, 7), GumpBackgroundFlags.Solid, 0, 0));
        Assert.Throws<InvalidOperationException>(() => draw.Draw(99)); Assert.Throws<ArgumentException>(() => new GumpBackgroundDraw(s.Collection, clip, globals, new()));
        var fallback = new List<uint>(); var dispatch = new GumpDrawHooks { Draw = t => fallback.Add(t) }; GumpBackgroundDraw.BindGumpBackgroundDraw(dispatch, draw, t => t == 2); dispatch.Draw!(1); dispatch.Draw(2); Assert.Equal(new uint[] { 1 }, fallback);
        Assert.Throws<ArgumentNullException>(() => GumpBackgroundDraw.BindGumpBackgroundDraw(dispatch, draw, null!));
    }
}
