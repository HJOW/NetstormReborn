using System.Buffers.Binary;
using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.InventoryRawSupport;

namespace Netstorm.Client.Tests; // 실제 설치 자산·공통 노드/문자/수명으로 생산 하위 전체를 조합한다.

/// <summary>실제 픽셀·내용물 전달·목록 재부착·문자열 반납과 기존 UI 보존을 검사한다.</summary>
public sealed class ProductionInventoryTreeTests
{
    /// <summary>창/소리 없이 실제 파일/글꼴과 같은 공통 트리를 만든다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>같은 실제 메뉴/트리 소유자다.</summary>
        public readonly HostMainMenu Menu;
        /// <summary>기존 HTML의 별도 소유자다.</summary>
        public readonly HostHtmlWindows Html;
        /// <summary>같은 실제 스타일/조각/한 줄 문자다.</summary>
        public readonly HostHtmlText Text;
        /// <summary>같은 현재 내용물/TYPE/프레임/셰이프다.</summary>
        public readonly RawSurfaceStorage Storage = new();
        /// <summary>실제 최초 자산 owner다.</summary>
        public readonly HostAssetTables Assets;
        /// <summary>현재 실제 공통 트리다.</summary>
        public GumpButtonTree Tree => Menu.Tree;
        /// <summary>실제 같은 원문 자산과 팔레트를 읽는다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var startup = new HostStartupScene(data); var screen = new Screen(1024, 768); Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
            Menu = new(startup, screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 }); Html = new(Menu, new InputQueue(), () => 1, _ => { }); Html.BindParser(startup, _ => { }); Text = new(Menu, Html, screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Html);
            Assets = new(startup, Storage); Text.BindPictures(new(Assets.TypeTable, Assets.Definitions, new(Assets.ShapeImage)), () => SpriteColorMapBuilder.BuildSpriteColorMaps(screen.LogicalPalette).Shadow, startup.Configuration);
        }
        /// <summary>자기 등록 자식만 공통 수명으로 반납한다.</summary>
        public void CloseOwned(object owner)
        {
            // 같은 등록표 순서에서 이미 죽은 자식은 다시 Kill하지 않는다.
            foreach (uint token in Tree.Create.Entries.Skip(1).ToArray()) if (token != 0 && ReferenceEquals(Tree.NodeOwner(token), owner) && Tree.Node(token) is { } node && (node.Flags & 0x10000) == 0) Tree.Lifetime.Kill(token);
            Tree.Lifetime.Cleanup(); Assert.Empty(Tree.OwnedNodes(owner));
        }
        /// <summary>같은 실제 자식 우선 Kill이다.</summary>
        public void Clear(uint token) { while (Tree.Node(token)!.Child != 0) Tree.Lifetime.Kill(Tree.Node(token)!.Child); }
        /// <summary>기존 HTML 소유 자식만 정리한다.</summary>
        public void Dispose() => Html.CloseAll();
    }
    /// <summary>실제 골렘/썬 캐논의 전체 측정/decoder→이미지→픽셀/클릭과 재부착 수명을 확인한다.</summary>
    [Theory]
    [InlineData(133u)]
    [InlineData(128u)]
    public void ActualAssetsUseSameTreePixelsContentInputAndReplacement(uint kind)
    {
        using var s = new Scene(); var tree = s.Tree; object owner = new(); var storage = s.Storage; var frames = storage.Frames.Select((value, index) => new PriestPlainCanonType { Frames = value, DefaultFrame = index < 70 || index - 70 >= s.Assets.Definitions.Count ? 0 : s.Assets.Definitions[index - 70].SpecialFrames.DefaultFrame }).ToArray();
        var geometry = new PriestPlacementGeometryState(); var measure = new RawCanonPixelShape(storage.Pool, storage.Types, frames, storage.Shapes, storage.Types.Select(t => new PriestTypeHotspot(t.HotspotX, t.HotspotY)).ToArray(), geometry, new());
        var bindings = new List<ImageGumpAssetBinding>();
        // 실제 최초 TYPE/프레임과 같은 SHP 블록만 연결한다.
        foreach (uint type in new[] { 128u, 133u }) { var record = storage.Types[(int)type]; bindings.Add(new() { Type = type, Block = (int)type - 70, FrameCount = storage.Frames[(int)type].Codes.Count, Flags = record.Flags1, HotspotX = record.HotspotX, HotspotY = record.HotspotY }); }
        var assets = new ImageGumpAssets(new(s.Assets.ShapeImage), bindings); var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 200, 300)); byte[] pixels = Enumerable.Repeat((byte)255, 60032).ToArray(); var target = new GumpPixelTarget(pixels, 200, 300, clip);
        var imageHooks = new ImageGumpPixelTarget(target, assets, new byte[256], _ => throw new InvalidOperationException("검사에서 선택하지 않는 변환 색표")).Hooks(new() { Node = t => tree.Node(t) as ImageGumpNode, Position = tree.Collection.Position, Attach = tree.Tree.Attach }); ImageGumpAssets.BindImageGumpAssets(imageHooks, assets); var image = new ImageGump(tree.Create, tree.Factory, new() { LocalOwner = 1, OwnerMaps = new uint[188] }, imageHooks);
        ImageGump.BindImageGump(tree.TreeHooks, new(), t => ReferenceEquals(tree.NodeOwner(t), owner) && tree.Node(t) is ImageGumpNode ? image : null);
        uint gridToken = tree.WithNodeOwner(owner, () => tree.AllocateNode(new InventoryGridNode())); var finder = new ContainedFinderState(); InventoryGump? grid = null; InventoryItemGump? item = null; (uint Event, uint Sid, uint Serial) selected = default;
        var itemHooks = new InventoryItemHooks
        {
            Node = t => ReferenceEquals(tree.NodeOwner(t), owner) ? tree.Node(t) as InventoryItemNode : null, Base = tree.Node, BaseAttach = tree.Tree.Attach, ClearChildren = s.Clear, Cleanup = tree.Lifetime.Cleanup,
            Robust = (sid, allow) => sid == 128 && allow, Assertion = text => throw new InvalidOperationException(text),
            Allocate = bytes => tree.AllocateNode(bytes == 180 ? new ImageGumpNode() : bytes == 164 ? new TextGumpNode() : throw new ArgumentOutOfRangeException(nameof(bytes))),
            Image = (t, type, frame, mode) => image.Construct(t, type, unchecked((uint)frame), mode), Text = t => s.Text.Single.Construct(t, 0, 0, 0, 0), StyledText = (t, text) => { var node = (TextGumpNode)tree.Node(t)!; s.Text.Style.Parse(ref node.Head, Encoding.Latin1.GetBytes(text), node.TextContext); }, FitText = t => s.Text.Single.Fit(t), Attach = (t, x, y, parent, mode) => tree.TreeHooks.Attach!(t, x, y, parent, mode),
            CostUnits = _ => 800, Money = _ => 99999, Local = (t, _, x, y) => tree.Collection.ScreenToLocal(t, new(unchecked((int)x), unchecked((int)y))), KnowledgeOpen = () => 0, ImageHit = image.Hit, Denied = (_, _) => 0, Tell = _ => throw new InvalidOperationException("검사에서 선택하지 않는 알림"),
            Forward = (_, code, sid, serial) => { selected = (code, sid, serial); return 17; }, About = (_, _, _) => throw new InvalidOperationException("검사에서 선택하지 않는 정보 메뉴"),
        };
        InventoryItemGump.BindAssets(itemHooks, OriginalEdition.Patch1078, s.Assets.Definitions, frames, geometry, measure);
        item = new(storage.Pool, new() { LocalOwner = () => 1, Editor = () => false, StockEnabled = () => 0, InputDisabled = () => 0, LastType = new(), BridgeType = () => 82, PixelsX = () => 16, PixelsY = () => 11, BackgroundType = () => s.Menu.BackgroundType, BackgroundFrame = () => 1 }, itemHooks);
        grid = new(tree.Create, tree.Factory, storage.Pool, finder, new() { CursorPoint = () => default, CursorFlags = () => 0 }, new()
        {
            Grid = t => t == gridToken ? tree.Node(t) as InventoryGridNode : null, Item = t => tree.Node(t) as InventoryItemNode, ClearChildren = s.Clear, BaseAttach = tree.Tree.Attach, AllocateItem = bytes => bytes == 192 ? tree.AllocateNode(new InventoryItemNode()) : throw new ArgumentOutOfRangeException(nameof(bytes)), Attach = (t, x, y, parent, mode) => tree.TreeHooks.Attach!(t, x, y, parent, mode),
        });
        ProductionInventoryBindings.BindInventory(tree.TreeHooks, tree.EventHooks, tree.InputHooks, tree.FrameHooks, t => t == gridToken ? grid : null, t => ReferenceEquals(tree.NodeOwner(t), owner) && tree.Node(t) is InventoryItemNode ? item : null, t => ReferenceEquals(tree.NodeOwner(t), owner) && tree.Node(t) is InventoryItemNode ? grid : null);
        var raw = Raw(storage.Pool, 50); BinaryPrimitives.WriteUInt16LittleEndian(raw[6..], 128); raw = Raw(storage.Pool, 128); raw[10] = 6; BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], 0); BinaryPrimitives.WriteUInt16LittleEndian(raw[18..], (ushort)kind); BinaryPrimitives.WriteUInt16LittleEndian(raw[20..], 8); raw[23] = 0; raw[30] = 3;
        grid.ConstructGrid(gridToken, 1, 6, 8); var root = (InventoryGridNode)tree.Node(gridToken)!; root.Object = 50; root.Width = root.BaseWidth = 40; root.Height = root.BaseHeight = 250; tree.WithNodeOwner(owner, () => { tree.TreeHooks.Attach!(gridToken, 0, 0, tree.Create.Entries[0], 0); return 0; });
        uint itemToken = root.Child; var child = Assert.IsType<InventoryItemNode>(tree.Node(itemToken)); Assert.Equal(128u, child.Content); Assert.Equal(gridToken, child.Parent); var images = tree.OwnedNodes(owner).Where(t => tree.Node(t) is ImageGumpNode).ToArray(); Assert.NotEmpty(images);
        // 실제 전체 그림을 같은 픽셀/클립에 그리고 행 밖 가드를 보존한다.
        foreach (uint token in images) image.Draw(token);
        Assert.All(pixels.AsSpan(60000).ToArray(), value => Assert.Equal(255, value)); ScreenPoint? selectedPoint = null;
        // 원문 Draw/Hit의 별도 절삭 경계를 유지하며 실제 그림과 적중이 있는 클릭점을 찾는다.
        for (int pixel = 0; pixel < 60000 && selectedPoint is null; pixel++) if (pixels[pixel] != 255) { var candidate = new ScreenPoint(pixel % 200, pixel / 200); var local = tree.Collection.ScreenToLocal(itemToken, candidate); if (images.Any(token => image.Hit(token, local.X, local.Y) != 0)) selectedPoint = candidate; }
        Assert.NotNull(selectedPoint); var point = selectedPoint.Value;
        Assert.Equal(17u, tree.EventHooks.Handle!(itemToken, 0x10000, unchecked((uint)point.X), unchecked((uint)point.Y))); Assert.Equal((0x02000008u, 128u, child.Serial), selected);
        BinaryPrimitives.WriteUInt16LittleEndian(Raw(storage.Pool, 50)[6..], 0); tree.Tree.Push(gridToken); tree.WithNodeOwner(owner, () => { tree.TreeHooks.Attach!(gridToken, 0, 0, tree.Create.Entries[0], 0); return 0; }); tree.Lifetime.Cleanup(); Assert.Equal(0u, root.Child); Assert.Null(tree.Node(itemToken)); s.CloseOwned(owner); Assert.NotNull(tree.Node(tree.Create.Entries[0]));
    }
    /// <summary>실제 공유 Text/Style 조각·Idle·문자열→Text 소멸과 다른 UI 수명을 확인한다.</summary>
    [Fact]
    public void HintUsesSharedTextIdleAndOwnedStringLifetime()
    {
        using var s = new Scene(); object owner = new(); var tree = s.Tree; var strings = new Dictionary<uint, string>(); uint next = 0; double now = 3; int fragments = s.Text.Fragments.Fragments.Count; var state = new ProductionHintState(); var hint = new ProductionHintGump(tree.Create, state, new()
        {
            Node = t => ReferenceEquals(tree.NodeOwner(t), owner) ? tree.Node(t) as ProductionHintNode : null, Now = () => now, Allocate = bytes => bytes == 184 ? tree.AllocateNode(new ProductionHintNode()) : throw new ArgumentOutOfRangeException(nameof(bytes)), ConstructText = t => s.Text.Single.Construct(t, 0, 0, 0, 0), Kill = tree.Lifetime.Kill,
            Duplicate = text => { uint t = ++next; strings.Add(t, text); return t; }, String = t => strings[t], Free = t => Assert.True(strings.Remove(t)), Translate = text => text,
            Style = (t, value) => { var node = (ProductionHintNode)tree.Node(t)!; s.Text.Style.Parse(ref node.Head, Encoding.Latin1.GetBytes(value), node.TextContext); }, Fit = s.Text.Single.Fit, Idle = tree.Lifetime.SetIdle, Bounds = tree.Collection.Bounds, Position = tree.Collection.Position,
            Draw = (_, _) => throw new InvalidOperationException("검사에서 선택하지 않는 출력 문맥"), Attach = (t, x, y, parent, mode) => tree.TreeHooks.Attach!(t, x, y, parent, mode), DeleteText = s.Text.Single.DeleteScalar,
        });
        ProductionInventoryBindings.BindHint(tree.FrameHooks, tree.LifetimeHooks, t => ReferenceEquals(tree.NodeOwner(t), owner) && tree.Node(t) is ProductionHintNode ? hint : null);
        uint token = tree.WithNodeOwner(owner, () => hint.Show(true, 40, 50, tree.Create.Entries[0])); var node = Assert.IsType<ProductionHintNode>(tree.Node(token)); Assert.Equal(7, node.Until); Assert.Equal(node.Serial, state.LastSerial); Assert.Single(strings); Assert.True(node.Head != 0); Assert.True(node.BaseWidth > 6); Assert.True(s.Text.Fragments.Fragments.Count > fragments);
        now = 6.99; tree.Frame.Idle(); Assert.Equal(0u, node.Flags & 0x10000); now = 7; tree.Frame.Idle(); tree.Lifetime.Cleanup(); Assert.Null(tree.Node(token)); Assert.Empty(strings); Assert.Equal(fragments, s.Text.Fragments.Fragments.Count); Assert.NotNull(tree.Node(tree.Create.Entries[0]));
    }
}
