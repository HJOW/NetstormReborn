using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 원본 이미지 UI를 실제 자산/픽셀/공통 트리에 연결한 검사다.

/// <summary>독립 원본 관찰과 별도로 실제 SHP/배율/색표/적중·공통 수명 연결을 검증한다.</summary>
public sealed class ImageGumpTests
{
    /// <summary>보호 원본이 있는 저장소 루트를 찾으며 파일은 읽기만 한다.</summary>
    private static string Root() { var dir = new DirectoryInfo(AppContext.BaseDirectory);
        // 상위 경로만 방문하고 원본 실행 파일을 실행하거나 수정하지 않는다.
        while (dir is not null) { if (File.Exists(Path.Combine(dir.FullName, "originals", "Netstorm.exe"))) return dir.FullName; dir = dir.Parent; } throw new InvalidOperationException("이미지 UI 보호 자료 경로 없음");
    }
    /// <summary>원문 TYPE hotFootRatio의 float32 입력과16/11 배율을 실제 검사 입력으로 읽는다.</summary>
    private static int Hotspot(Netstorm.Assets.TypeDefinition type, bool vertical) => (int)((double)(float)(type.GetDouble(vertical ? "hotFootRatioY" : "hotFootRatioX") ?? 0) * (vertical ? 11 : 16));
    /// <summary>실제 썬 캐논/골렘의 일반/축소/반전/0배율·선택색·클립/guard와 투명 적중을 대조한다.</summary>
    [Fact]
    public void OriginalCannonAndGolemActualPixelsHitAndColor()
    {
        var files = Netstorm.Assets.GameFileSystem.Open(Path.Combine(Root(), "originals")); var catalog = new Netstorm.Assets.TypeCatalog(files); var shapes = new ShapeDatabase(files.ReadAllBytes("d/_shapes.shp")); var palette = new GamePalette(files.ReadAllBytes("d/gifcloud.col")); uint[] logical = new uint[256];
        // 원본 파일의 실제RGB를 기존 원본 명암표 생성기에 공급한다.
        for (int i = 0; i < logical.Length; i++) { var c = palette.Color((byte)i); logical[i] = (uint)c.Red | ((uint)c.Green << 8) | ((uint)c.Blue << 16); }
        var maps = PaletteShade.BuildGumpShadeMaps(logical); var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 200, 200)); byte[] pixels = new byte[40032], current = new byte[256]; var target = new GumpPixelTarget(pixels, 200, 200, clip); var create = new GumpCreateState(); var node = new ImageGumpNode(); var factory = new GumpCreate(create, new() { Node = t => t == 2 ? node : null }); var state = new ImageGumpState { OwnerMaps = new uint[188] };
        var bindings = new List<ImageGumpAssetBinding>();
        // 원본128은썬캐논,133은골렘이다.현재TYPE/논리수와실제SHP블록을구별한다.
        foreach (uint number in new[] { 128U, 133U }) { var type = catalog.Types[(int)number - 70]; Assert.Equal(number, (uint)type.RuntimeIndex); bindings.Add(new() { Type = number, Block = type.LoadIndex, FrameCount = type.Definition.Frames.Codes.Count, Flags = type.Flags1, HotspotX = Hotspot(type.Definition, false), HotspotY = Hotspot(type.Definition, true) }); }
        var assets = new ImageGumpAssets(shapes, bindings); var raster = new ImageGumpPixelTarget(target, assets, current, token => token == 0x556438 ? maps.Dark : throw new InvalidOperationException("검사 미연결 색표")); var hooks = raster.Hooks(new() { Node = t => t == 2 ? node : null, Position = _ => new(70, 70) }); ImageGumpAssets.BindImageGumpAssets(hooks, assets); var image = new ImageGump(create, factory, state, hooks);
        // 같은 실제 아이콘의단위/축소/반전/0배율을현재VFX와적중에모두사용한다.
        foreach (uint type in new[] { 128U, 133U })
        {
            uint frame = (uint)catalog.Types[(int)type - 70].Definition.Frames.DefaultFrame; image.Construct(2, type, frame, 0); node.X = node.Y = 70;
            // 배율마다출력과로컬적중의실제255판정이같아야한다.
            foreach (float scale in new[] { 1f, .63f, -1f, 0f })
            {
                node.Scale = scale; Array.Fill(pixels, (byte)255); image.Draw(2); if (scale > 0) Assert.Contains(pixels.AsSpan(0, 40000).ToArray(), color => color != 255);
                // 실제 투명/불투명 영역을 양 축에서 고르게 샘플링한다.
                for (int y = 0; y < 196; y += 7)
                {
                    // 출력 버퍼의 현재색은 적중의 독립 기대값으로만 읽는다.
                    for (int x = 0; x < 196; x += 7) Assert.Equal(pixels[y * 200 + x] != 255 ? 1U : 0U, image.Hit(2, x, y));
                }
                Assert.All(pixels.AsSpan(40000).ToArray(), color => Assert.Equal(255, color));
            }
            node.Scale = 1; node.Mode = 0; Array.Fill(pixels, (byte)255); image.Draw(2); byte[] plain = (byte[])pixels.Clone(); node.Mode = 1; Array.Fill(pixels, (byte)255); image.Draw(2); Assert.False(plain.SequenceEqual(pixels)); Assert.Equal(maps.Dark, current); GumpDraw.SetGumpDrawClip(clip, new(60, 60, 90, 90)); Array.Fill(pixels, (byte)255); image.Draw(2);
            // 공유 클립 밖과 표면 끝의padding은 그대로다.
            for (int y = 0; y < 200; y++) for (int x = 0; x < 200; x++) if (x < 60 || x >= 90 || y < 60 || y >= 90) Assert.Equal(255, pixels[y * 200 + x]); Assert.All(pixels.AsSpan(40000).ToArray(), color => Assert.Equal(255, color)); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 200, 200));
        }
    }
    /// <summary>3×3 실제RLE의투명칸과불투명255를구별하는독립입력이다.</summary>
    private static ShapeDatabase SmallShapes()
    {
        byte[] raw = new byte[16 + 36 + 24 + 21]; BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x30312e31); BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(4), 1); BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(8), 52); BinaryPrimitives.WriteInt16LittleEndian(raw.AsSpan(40), 3); BinaryPrimitives.WriteInt16LittleEndian(raw.AsSpan(42), 3); BinaryPrimitives.WriteUInt16LittleEndian(raw.AsSpan(52), 2); BinaryPrimitives.WriteUInt16LittleEndian(raw.AsSpan(54), 2); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(68), 2); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(72), 2); new byte[] { 2, 7, 1, 1, 2, 8, 0, 2, 9, 2, 255, 2, 10, 0, 2, 11, 2, 12, 2, 13, 0 }.CopyTo(raw, 76); return new(raw);
    }
    /// <summary>선택적출력과같은실제한블록자산.</summary>
    private static ImageGumpAssets SmallAssets() => new(SmallShapes(), [new() { Type = 128, Block = 0, FrameCount = 1 }]);
    /// <summary>원문 선형index는x/y를개별클램프하지않는다.기준점보정/255/범위밖읽기도보존한다.</summary>
    [Fact]
    public void ActualHitRawLinearIndexOriginClampAndOpaque255()
    {
        var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 200, 200)); var hooks = new ImageGumpPixelTarget(new(new byte[40000], 200, 200, clip), SmallAssets(), new byte[256], _ => throw new InvalidOperationException()).Hooks(); var request = new ImageGumpHitRequest(128, 1, 0, new(0, 0), new(0, 0), 65536); Assert.Equal(1U, hooks.Hit!(request)); Assert.Equal(0U, hooks.Hit(request with { Point = new(1, 0) })); Assert.Equal(0U, hooks.Hit(request with { Point = new(1, 1) })); Assert.Equal(1U, hooks.Hit(request with { Point = new(200, 0) })); Assert.Equal(0U, hooks.Hit(request with { Point = new(-1, 1) })); Assert.Equal(1U, hooks.Hit(request with { Point = new(-3, -4), Origin = new(-3, -4) })); Assert.Equal(1U, hooks.Hit(request with { Point = new(205, 0), Origin = new(205, 0) })); Assert.Throws<ArgumentOutOfRangeException>(() => hooks.Hit(request with { Point = new(-1, 0) })); Assert.Throws<ArgumentOutOfRangeException>(() => hooks.Hit(request with { Point = new(0, 200) })); Assert.Equal(0U, hooks.Hit(request with { Scale = 0 }));
    }
    /// <summary>논리수/추가레이어와실제물리범위를분리하고현재TYPE필드를재조회한다.</summary>
    [Fact]
    public void ActualAssetsCurrentTypeLogicalLayersAndPhysicalBounds()
    {
        var binding = new ImageGumpAssetBinding { Type = 128, Block = 0, FrameCount = 1, HotspotX = 17, HotspotY = -19 }; var assets = new ImageGumpAssets(GumpBackgroundTestAssets.Shapes(), [binding]); Assert.Equal(new ImageGumpType(1, 17, -19), assets.Type(128)); Assert.Equal(new ImageGumpHeader(3, 3, 0, 0), assets.Header(128, 0)); Assert.Throws<ArgumentOutOfRangeException>(() => assets.Header(128, 1)); Assert.Equal(0, assets.Shape(128, 1).Block); binding.Flags = 0x400000; binding.HotspotX = 23; Assert.Equal(23, assets.Type(128).HotspotX); Assert.Equal((short)5, assets.Header(128, 1).Width); Assert.Throws<ArgumentOutOfRangeException>(() => assets.Shape(128, 2)); Assert.Throws<ArgumentOutOfRangeException>(() => assets.Type(69)); Assert.Throws<ArgumentException>(() => new ImageGumpAssets(SmallShapes(), [], OriginalEdition.Cd1072));
    }
    /// <summary>색표설치콜백뒤현재필드/전역을읽되최초위치/단위분기/TYPE사본은보존한다.</summary>
    [Fact]
    public void CurrentColorEffectsKeepOriginalStoredValues()
    {
        var create = new GumpCreateState(); var node = new ImageGumpNode { Type = 128, Frame = 7, Scale = 1, Mode = 4, HotspotX = 2, HotspotY = 3 }; var factory = new GumpCreate(create, new() { Node = _ => node }); var state = new ImageGumpState { OwnerMaps = new uint[188], ShadowOffset = new(5, 6) }; var paints = new List<ImageGumpDrawRequest>(); var colors = new List<uint>(); var hooks = new ImageGumpHooks { Node = _ => node, Type = _ => new(71, 0, 0), Position = _ => new(10, 20), Draw = r => paints.Add(r), Color = token => { colors.Add(token); if (colors.Count == 1) { node.Type = 129; node.Frame = 9; node.Mode = 2; node.Scale = .5f; node.HotspotX = 7; state.ShadowOffset = new(8, 9); } } }; var image = new ImageGump(create, factory, state, hooks); image.Draw(2); Assert.Equal(new uint[] { 0x556338, 0x556038 }, colors); Assert.Equal(new ImageGumpDrawRequest(129, 71, 9, new(25, 31), 65536, false, true), paints[0]); Assert.Equal(new ImageGumpDrawRequest(129, 71, 9, new(17, 23), 65536, false, true), paints[1]);
    }
    /// <summary>미연결/잘못된색표를항등표로대체하지않고앞선현재색표를보존한다.</summary>
    [Fact]
    public void MissingColorAndPartialConstructorRemainExplicit()
    {
        var clip = new GumpDrawState(); byte[] current = Enumerable.Repeat((byte)7, 256).ToArray(); var target = new GumpPixelTarget(new byte[4], 2, 2, clip); var hooks = new ImageGumpPixelTarget(target, SmallAssets(), current, _ => new byte[255]).Hooks(); Assert.Throws<ArgumentException>(() => hooks.Color!(0x556438)); Assert.All(current, b => Assert.Equal(7, b)); var create = new GumpCreateState(); var node = new ImageGumpNode(); var factory = new GumpCreate(create, new() { Node = _ => node }); var image = new ImageGump(create, factory, new(), new() { Node = _ => node, Type = _ => throw new InvalidOperationException("명시 입력 거부") }); Assert.Throws<InvalidOperationException>(() => image.Construct(2, 128, 3, 4)); Assert.Equal((1U, 0x501bb0U, 3U, 128U, 3U, 4U), (create.Count, node.Vtable, node.Command, node.Type, node.Frame, node.Mode)); var cd = new GumpCreateState(); cd.Collection.Edition = OriginalEdition.Cd1072; Assert.Throws<ArgumentException>(() => new ImageGump(cd, factory, new(), new()));
    }
    /// <summary>실제기본생성/가상부착·Draw와자식우선수명을같은공통저장소에연결한다.</summary>
    [Fact]
    public void ActualCommonTreeDrawAndLifetime()
    {
        var scene = new ScrollWidgetScene(); var node = new ImageGumpNode(); scene.Nodes.Add(30, node); var assets = SmallAssets(); var target = new GumpPixelTarget(scene.Pixels, 64, 128, scene.Clip); var hooks = new ImageGumpPixelTarget(target, assets, new byte[256], _ => throw new InvalidOperationException()).Hooks(new() { Node = t => t == 30 ? node : null, Position = scene.Collection.Position, Attach = scene.Tree.Attach }); ImageGumpAssets.BindImageGumpAssets(hooks, assets); var image = new ImageGump(scene.Create, scene.Factory, new() { OwnerMaps = new uint[188] }, hooks); ImageGump.BindImageGump(scene.TreeHooks, scene.DrawHooks, t => t == 30 ? image : null); image.Construct(30, 128, 0, 0); scene.TreeHooks.Attach!(30, 4, 5, 1, 1); Assert.Equal((1U, 4f, 5f), (node.Parent, node.X, node.Y)); scene.Pixels.Span.Fill(0x5a); scene.Draw.Draw(30); Assert.Equal(7, scene.Pixels.Span[5 * 64 + 4]); Assert.Equal(0x5a, scene.Pixels.Span[5 * 64 + 5]); Assert.Equal(255, scene.Pixels.Span[6 * 64 + 5]); scene.Lifetime.Kill(30); scene.Lifetime.Cleanup(); Assert.Equal(1U, scene.Create.Count); Assert.Contains(30U, scene.Released);
    }
}
