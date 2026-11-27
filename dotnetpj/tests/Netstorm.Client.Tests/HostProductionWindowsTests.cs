using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 실제 자산/공통 트리로 생산 부모의 전체 초기 생성과 수명을 검사한다.

/// <summary>Source ProductionInitialContext의 실제8노드/현재 예산/부분 실패를 재현한다.</summary>
public sealed class HostProductionWindowsTests
{
    /// <summary>직접 부착한 객체 메뉴를 중복 부착하지 않고 이전 sticky 팝업을 정리한다.</summary>
    [Fact]
    public void AttachedMenuUsesActualRootAndClosesPreviousPopup()
    {
        using var scene = new Scene(); var menus = scene.Menus; var tree = scene.Menu.Tree; int changes = 0;
        uint first = 0; float attachedX = 0, attachedY = 0;
        menus.OpenAttached(() =>
        {
            first = menus.Widgets.BeginFancy();
            menus.Menu.AttachCurrent(14, 19, tree.Create.Collection.Root);
            attachedX = tree.Node(first)!.X; attachedY = tree.Node(first)!.Y;
            return first;
        }, () => changes++);
        Assert.Equal(first, menus.Popup); Assert.Equal(0u, menus.State.Index);
        Assert.Equal(tree.Create.Collection.Root, tree.Node(first)!.Parent);
        Assert.Equal(attachedX, tree.Node(first)!.X); Assert.Equal(attachedY, tree.Node(first)!.Y);
        tree.Node(first)!.Flags |= 0x4000;
        uint second = 0;
        menus.OpenAttached(() =>
        {
            second = menus.Widgets.BeginFancy();
            menus.Menu.AttachCurrent(30, 40, tree.Create.Collection.Root);
            return second;
        }, () => changes++);
        Assert.Null(tree.Node(first)); Assert.Equal(second, menus.Popup); Assert.Equal(0u, menus.State.Index); Assert.Equal(4, changes);
        Assert.Throws<InvalidOperationException>(() => menus.OpenAttached(() => menus.Widgets.BeginFancy(), () => changes++, false));
        Assert.NotEqual(0u, menus.State.Index);
    }

    /// <summary>World/Session 없이 실제 Client owner와 공통 글꼴/자산을 준비한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>실제 장기 게임 전역이다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>실제 현재 동일 물리다.</summary>
        public readonly RawSurfaceStorage Storage;
        /// <summary>실제 같은 화면이다.</summary>
        public readonly Screen Screen;
        /// <summary>실제 같은 메뉴/트리다.</summary>
        public readonly HostMainMenu Menu;
        /// <summary>실제 일반 UI 소유자다.</summary>
        public readonly HostHtmlWindows Html;
        /// <summary>같은 실제 한 줄/스타일/그림 자산이다.</summary>
        public readonly HostHtmlText Text;
        /// <summary>같은 실제 공통 출력이다.</summary>
        public readonly HostGumpDrawing Drawing;
        /// <summary>같은 실제 controlled 메뉴 입력/팝업 장치다.</summary>
        public readonly HostHtmlMenus Menus;
        /// <summary>실제 현재 옵션이다.</summary>
        public readonly byte[] Options = new byte[87];
        /// <summary>여러 생성 세대가 같은 장기 전역을 빌린다.</summary>
        public readonly ProductionMaintenanceGlobals Globals = new();
        /// <summary>World 없는 실제 현재 SP owner다.</summary>
        public readonly ClientSpGlobals Sp = new(new GameRandom(17), new() { Value = 23 }, new(), () => 1234567);
        /// <summary>현재 원문 벽시계다.</summary>
        public double Wall = 123.25;
        /// <summary>현재 생성/설정 조회 관측이다.</summary>
        public int TimeReads, ConfigReads;
        /// <summary>실제 번역 호출의 입력 순서다.</summary>
        public readonly List<string> Translations = [];
        /// <summary>실제 생산 처리기의 현재 세대다.</summary>
        public HostProductionWindows? Panel;
        /// <summary>새 파일 적재 없이 같은 최초 자산/문자/물리에 모든 UI를 연결한다.</summary>
        public Scene(int width = 640, int height = 480)
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var startup = new HostStartupScene(data); Screen = new(width, height); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(Screen);
            Menu = new(startup, Screen, new() { Now = () => Wall, Poll = _ => default, Sound = () => throw new InvalidOperationException("미선택 버튼 음향"), Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 }); Html = new(Menu, new InputQueue(), () => Wall, _ => throw new InvalidOperationException("미선택 UI 명령")); Html.BindParser(startup, _ => { }); Text = new(Menu, Html, Screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Html);
            Storage = new(authority: Game.Authority, gameAux: Game.GameAux); var assets = new HostAssetTables(startup, Storage); Text.BindPictures(new(assets.TypeTable, assets.Definitions, new(assets.ShapeImage)), () => SpriteColorMapBuilder.BuildSpriteColorMaps(Screen.LogicalPalette).Shadow, startup.Configuration); Menus = new(Menu, Html, Text, startup, Screen, (_, _) => { }); Menus.BindInput(new(), _ => throw new InvalidOperationException("미선택 메뉴 음향"), (_, _) => { }); Drawing = new(Menu, Text, Menus, Screen); Globals.BackgroundType = Menu.BackgroundType;
        }
        /// <summary>실제 원문 입력/효과만 연결하고 선택 안 된 Runtime 장치는 설치하지 않는다.</summary>
        public HostProductionWindows NewPanel(bool translate = true)
        {
            return Panel = new(Menu.Tree, Text, new() { Storage = Storage, Gameplay = Game, Players = Game.PlayerRecords, Options = Options, Sp = Sp }, Globals, new()
            {
                Now = () => { TimeReads++; return Wall; }, Sound = _ => throw new InvalidOperationException("미선택 생산 음향"), Tell = _ => throw new InvalidOperationException("미선택 알림"), ConfigInt = (_, fallback) => { ConfigReads++; return fallback; }, Translate = translate ? value => { Translations.Add(value); return value; } : null, Changed = () => { }, BaseHandle = (_, _, _, _) => 0, Redraw = (_, _) => { }, ClosePopups = () => Menus.RequestClosePopups(() => { }), MenuInstances = () => Menus.State.Instances,
            });
        }
        /// <summary>실제 화면 잠금으로 현재 전체 픽셀/행 padding을 읽는다.</summary>
        public byte[] Pixels() { byte[] pixels = Screen.Lock(); try { return pixels.ToArray(); } finally { Screen.Unlock(); } }
        /// <summary>자기 생산 세대와 실제 기존 UI를 각각 반납한다.</summary>
        public void Dispose() { Panel?.Dispose(); Html.CloseAll(); }
    }
    /// <summary>원문8노드/40폭/화면높이·도움말/시계와 실제 공통 출력/클립을 확인한다.</summary>
    [Theory]
    [InlineData(640, 480)]
    [InlineData(643, 480)]
    [InlineData(1024, 768)]
    public void ConstructUsesActualEightNodesAssetsTextAndSharedDrawing(int width, int height)
    {
        using var s = new Scene(width, height); var tree = s.Menu.Tree; int before = tree.Create.Entries.Count(value => value != 0), strings = tree.OwnedTextCount; uint[] old = tree.Create.Entries.Where(value => value != 0).ToArray(); s.Options[1] = 3; var panel = s.NewPanel(); uint token = panel.Construct(); var root = Assert.IsType<ProductionRootNode>(panel.Node(token));
        Assert.Equal(before + 8, tree.Create.Entries.Count(value => value != 0)); Assert.Equal(8, tree.OwnedNodes(panel).Length); Assert.Equal(strings + 8, tree.OwnedTextCount); Assert.Equal(0u, root.Parent); Assert.Equal(40, root.BaseWidth); Assert.Equal(height, root.BaseHeight); Assert.Equal(1, s.TimeReads); Assert.Equal(0, s.ConfigReads); Assert.Equal(0u, s.Game.PlayerRecords.Base); Assert.False(s.Game.Borrowed()); Assert.Equal(new[] { "Storm Power Available", "Production Window", "Production Window", "Production Window" }, s.Translations);
        var bridge = Assert.IsType<InventoryGridNode>(panel.Node(root.BridgeGrid)); var teleport = Assert.IsType<InventoryGridNode>(panel.Node(root.TeleportGrid)); Assert.Equal((2, 4, 1u, 144f, 26f), (bridge.MinimumColumns, bridge.MinimumRows, bridge.Fixed, bridge.BaseHeight, bridge.Y)); Assert.Equal((1, 6, 250f, 170f), (teleport.MinimumColumns, teleport.MinimumRows, teleport.BaseHeight, teleport.Y)); var line = Assert.IsType<HudDividerNode>(panel.Node(root.Separator)); Assert.Equal((0u, 40f, 5f, 21f), (line.Axis, line.BaseWidth, line.BaseHeight, line.Y)); var budget = Assert.IsType<ProductionBudgetNode>(panel.Node(root.Budget)); Assert.NotEqual(0u, budget.Head); Assert.NotEqual(0u, budget.TextPointer); Assert.NotEqual(0u, budget.AlternateTextPointer); Assert.Equal("Storm Power Available", tree.ReadOwnedText(budget.TextPointer));
        s.Drawing.Draw(); byte[] pixels = s.Pixels(); panel.SetPalette(s.Screen.LogicalPalette); panel.Attach(token, 0, 0, tree.Create.Collection.Root, 0); s.Drawing.Draw(); byte[] drawn = s.Pixels(); Assert.False(pixels.AsSpan().SequenceEqual(drawn));
        // 모든 행의 실제 화면 밖 padding은 각각 그대로여야 한다.
        for (int row = 0; row < height; row++) for (int column = width; column < s.Screen.Pitch; column++) Assert.Equal(pixels[row * s.Screen.Pitch + column], drawn[row * s.Screen.Pitch + column]);
        panel.Close(); Assert.False(panel.HasNodes); Assert.Equal(before, tree.Create.Entries.Count(value => value != 0)); Assert.Equal(strings, tree.OwnedTextCount); Assert.All(old, value => Assert.NotNull(tree.Node(value))); Assert.NotNull(tree.Node(tree.Create.Collection.Root));
    }
    /// <summary>실제 Client SP와 예산 완급/fort 즉시 반영·일반 재부착의 같은 토큰을 검사한다.</summary>
    [Fact]
    public void BudgetUsesRealClientSpAndResizeKeepsSamePanel()
    {
        using var s = new Scene(); var panel = s.NewPanel(); uint token = panel.Construct(); var root = Assert.IsType<ProductionRootNode>(panel.Node(token)); var budget = Assert.IsType<ProductionBudgetNode>(panel.Node(root.Budget)); s.Sp.Store.Initialize(); s.Sp.Store.Set(1, BitConverter.SingleToUInt32Bits(5000)); s.Game.Player.PlayerKnown = 1; panel.Idle(root.Budget); Assert.Equal(100, budget.Amount); s.Storage.OwnerMode.Fort = true; panel.Idle(root.Budget); Assert.Equal(5000, budget.Amount); s.Sp.Store.Set(1, BitConverter.SingleToUInt32Bits(0)); s.Storage.OwnerMode.Fort = false; panel.Idle(root.Budget); Assert.Equal(4900, budget.Amount); Assert.False(s.Game.Borrowed()); Assert.Equal(0u, s.Game.PlayerRecords.Base);
        s.Game.Player.PlayerKnown = 0; uint budgetToken = root.Budget; uint serial = root.Serial; panel.Attach(token, 0, 0, s.Menu.Tree.Create.Collection.Root, 0); panel.Resize(700); Assert.Equal(620, root.BaseHeight); Assert.Equal(token, panel.RootToken); Assert.Equal(budgetToken, root.Budget); Assert.Equal(serial, root.Serial); var depths = s.Menu.Tree.OwnedNodes(panel).Select(current => (current, panel.Node(current)!.Depth)).ToArray(); panel.Resize(700); Assert.Equal(depths, s.Menu.Tree.OwnedNodes(panel).Select(current => (current, panel.Node(current)!.Depth)).ToArray());
    }
    /// <summary>중간 번역 실패의 실제2등록과 부분 owner를 Close하고 세 생산 세대를 독립 반납한다.</summary>
    [Fact]
    public void PartialFailureAndRepeatedGenerationsRetireActualNodes()
    {
        using var s = new Scene(); var tree = s.Menu.Tree; int before = tree.Create.Entries.Count(value => value != 0), texts = tree.OwnedTextCount; var failed = s.NewPanel(false); Assert.Throws<InvalidOperationException>(() => failed.Construct()); Assert.Equal(before + 2, tree.Create.Entries.Count(value => value != 0)); Assert.Equal(0u, s.Globals.RootSerial); failed.Dispose(); Assert.Equal(before, tree.Create.Entries.Count(value => value != 0)); Assert.Equal(texts, tree.OwnedTextCount);
        List<uint> retired = [];
        // 같은 장기 UI/전역에서 세 실제 생성/소멸을 수행한다.
        for (int generation = 0; generation < 3; generation++) { var panel = s.NewPanel(); uint token = panel.Construct(); Assert.DoesNotContain(token, retired); panel.Dispose(); retired.Add(token); Assert.Equal(before, tree.Create.Entries.Count(value => value != 0)); Assert.Equal(texts, tree.OwnedTextCount); Assert.False(panel.HasNodes); }
        Assert.Equal(0, tree.ExternalDrawing.RegistrationCount);
    }
}
