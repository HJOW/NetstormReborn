// 미션 HUD 생산 덱 틀 부착과 DN-MP-01 우클릭 도움말을 검증한다.
// 2026-11-26 1.079에서 R5-4c2용으로 추가했다.
using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>생산 덱 틀의 실제 8노드·예산 HELP·우클릭 메뉴를 같은 트리에서 대조한다.</summary>
public sealed class HostMissionProductionDeckTests
{
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
        /// <summary>같은 실제 controlled 메뉴 입력/팝업 장치다.</summary>
        public readonly HostHtmlMenus Menus;
        /// <summary>실제 현재 옵션이다.</summary>
        public readonly byte[] Options = new byte[87];
        /// <summary>여러 생성 세대가 같은 장기 전역을 빌린다.</summary>
        public readonly ProductionMaintenanceGlobals Globals = new();
        /// <summary>World 없는 실제 현재 SP owner다.</summary>
        public readonly ClientSpGlobals Sp = new(new GameRandom(17), new() { Value = 23 }, new(), () => 1234567);
        /// <summary>현재 원본 벽시각이다.</summary>
        public double Wall = 123.25;
        /// <summary>도움말 메뉴의 실제 기록이다.</summary>
        public readonly List<string> HelpLog = [];
        /// <summary>실제 생산 처리기의 현재 세대다.</summary>
        public HostProductionWindows? Panel;
        /// <summary>실제 우클릭 도움말 처리기다.</summary>
        public GumpHelp? Help;
        /// <summary>새 파일 적재 없이 같은 최초 자산/문자/물리에 모든 UI를 연결한다.</summary>
        public Scene(int width = 1024, int height = 768, bool nativeForms = false)
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var startup = new HostStartupScene(data); Screen = new(width, height); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(Screen);
            Menu = new(startup, Screen, new() { Now = () => Wall, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 }); Html = new(Menu, new Input.InputQueue(), () => Wall, _ => { }); Html.BindParser(startup, _ => { }); Text = new(Menu, Html, Screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Html);
            Storage = new(authority: Game.Authority, gameAux: Game.GameAux);
            if (nativeForms)
            {
                Storage.BindTransmissionDevices(new() { Broadcast = _ => { }, Connected = _ => false, Send = (_, _) => { } });
                Storage.BindDisplayDevices(new() { View = () => new() { Zoom = 65536 }, Suppressed = () => false, Invalidate = (_, _) => { } });
            }
            var assets = new HostAssetTables(startup, Storage); Text.BindPictures(new(assets.TypeTable, assets.Definitions, new(assets.ShapeImage)), () => SpriteColorMapBuilder.BuildSpriteColorMaps(Screen.LogicalPalette).Shadow, startup.Configuration); Menus = new(Menu, Html, Text, startup, Screen, (_, _) => { }); Menus.BindInput(new(), _ => { }, (_, _) => { }); Globals.BackgroundType = Menu.BackgroundType;
        }
        /// <summary>실제 원문 입력/효과와 우클릭 도움말을 연결한다.</summary>
        public HostProductionWindows NewPanel(Func<uint, uint, uint>? bindForm = null, Action? synchronize = null)
        {
            var tree = Menu.Tree;
            Help = new(tree.Create, new GumpHelpState(), new GumpHelpHooks
            {
                Node = tree.Node,
                Text = token => tree.ReadOwnedText(token),
                BeginMenu = _ => { HelpLog.Add("begin"); Menus.Widgets.BeginVertical(0); },
                Title = token => { HelpLog.Add("title:" + tree.ReadOwnedText(token)); Menus.Controls.AddTitle(System.Text.Encoding.Latin1.GetBytes(tree.ReadOwnedText(token))); },
                Entry = (_, _, topic, _, _, _) => { HelpLog.Add("entry:" + topic); Menus.Controls.AddText(System.Text.Encoding.Latin1.GetBytes(topic), 0, 0); },
                AttachMenu = (x, y, parent) => { HelpLog.Add($"attach:{x},{y}"); uint current = Menus.Menu.TakeCurrent(); Menus.Widgets.Attach(current, x, y, parent, 0); },
                Sound = name => HelpLog.Add("sound:" + name),
            });
            var help = Help;
            return Panel = new(tree, Text, new() { Storage = Storage, Gameplay = Game, Players = Game.PlayerRecords, Options = Options, Sp = Sp }, Globals, new()
            {
                Now = () => Wall, Sound = _ => { }, Tell = _ => { }, ConfigInt = (_, fallback) => fallback, Translate = value => value, Changed = () => { }, BaseHandle = (token, code, first, second) => help.Handle(token, code, first, second), Redraw = (_, _) => { }, ClosePopups = () => Menus.RequestClosePopups(() => { }), MenuInstances = () => Menus.State.Instances,
                BindForm = bindForm, Synchronize = synchronize,
            });
        }
        /// <summary>자기 생산 세대와 실제 기존 UI를 각각 반납한다.</summary>
        public void Dispose() { Panel?.Dispose(); Html.CloseAll(); }
    }

    /// <summary>생산 사건은 현재 기술 부모로 두 격자를 연결한 뒤 동기화를 실행한다.</summary>
    [Fact]
    public void ProductionModeEventBindsBothGridsBeforeSynchronization()
    {
        using var s = new Scene();
        ClientPlayerRuntime.Initialize(s.Game);
        s.Game.Player.PlayerKnown = 1; s.Game.Player.BattleOwner = 1;
        BinaryPrimitives.WriteUInt32LittleEndian(s.Game.PlayerRecords.Records.Span[(180 + 0x9c)..], 117);
        var calls = new List<(uint Token, uint Parent)>();
        HostProductionWindows? panel = null; ProductionRootNode? root = null; int synchronized = 0;
        s.Menu.Tree.LifetimeHooks.DeleteForm = (_, _) => { };
        panel = s.NewPanel((token, parent) => { calls.Add((token, parent)); return unchecked((uint)(500 + calls.Count)); }, () =>
        {
            Assert.Equal(2, calls.Count);
            Assert.Equal(501u, panel!.Node(root!.BridgeGrid)!.Form);
            Assert.Equal(502u, panel.Node(root.TeleportGrid)!.Form);
            synchronized++;
        });
        uint token = panel.Construct(); root = Assert.IsType<ProductionRootNode>(panel.Node(token));
        Assert.Equal(0u, panel.Handle(token, 0x0200003d, 1, 0));
        Assert.Equal(new[] { (root.BridgeGrid, 117u), (root.TeleportGrid, 117u) }, calls);
        Assert.Equal(1, synchronized);
        Assert.Equal(117u, panel.Node(root.BridgeGrid)!.Object);
        Assert.Equal(117u, panel.Node(root.TeleportGrid)!.Object);
    }

    /// <summary>월드가 없는 생산 사건도 실제 Client 동기화로 현재 Deck을 마련한다.</summary>
    [Fact]
    public void ProductionModeEventRunsActualWorldlessSynchronization()
    {
        using var s = new Scene();
        ClientPlayerRuntime.Initialize(s.Game);
        s.Game.Player.TemplateOwner = 2;
        var context = new ProductionControlContext { Storage = s.Storage, Gameplay = s.Game, Players = s.Game.PlayerRecords, Options = s.Options, Sp = s.Sp };
        var panel = s.NewPanel(synchronize: () => ClientProductionSync.SynchronizeProductionControlContents(context));
        uint token = panel.Construct();
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(s.Game.PlayerRecords.Records.Span[(2 * 180 + 0x68)..]));
        Assert.Equal(0u, panel.Handle(token, 0x0200003d, 1, 0));
        Assert.NotEqual(0u, BinaryPrimitives.ReadUInt32LittleEndian(s.Game.PlayerRecords.Records.Span[(2 * 180 + 0x68)..]));
    }

    /// <summary>덱 틀 8노드·예산 HELP·생산창 HELP와 우클릭 메뉴를 확인한다.</summary>
    [Fact]
    public void DeckFrameSetsBudgetAndWindowHelp()
    {
        using var s = new Scene();
        var tree = s.Menu.Tree;
        int before = tree.Create.Entries.Count(value => value != 0);
        s.Options[1] = 3;
        var panel = s.NewPanel();
        uint token = panel.Construct();
        var root = Assert.IsType<ProductionRootNode>(panel.Node(token));
        Assert.Equal(before + 8, tree.Create.Entries.Count(value => value != 0));
        var budget = Assert.IsType<ProductionBudgetNode>(panel.Node(root.Budget));
        Assert.NotEqual(0u, budget.TextPointer);
        Assert.NotEqual(0u, budget.AlternateTextPointer);
        Assert.Equal("Storm Power Available", tree.ReadOwnedText(budget.TextPointer));
        Assert.Equal("moneyGump", tree.ReadOwnedText(budget.AlternateTextPointer));
        Assert.NotEqual(0u, budget.Flags & 0x1000U);
        var bridge = Assert.IsType<InventoryGridNode>(panel.Node(root.BridgeGrid));
        Assert.NotEqual(0u, bridge.TextPointer);
        Assert.NotEqual(0u, bridge.AlternateTextPointer);
        Assert.Equal("Production Window", tree.ReadOwnedText(bridge.TextPointer));
        Assert.Equal("teleportView", tree.ReadOwnedText(bridge.AlternateTextPointer));
        var middle = Assert.IsType<GumpBackgroundNode>(panel.Node(root.Middle));
        Assert.NotEqual(0u, middle.TextPointer);
        Assert.NotEqual(0u, middle.AlternateTextPointer);
        // DN-MP-01 우클릭 메뉴 표시를 같은 HELP 포인터로 검증한다.
        Assert.NotNull(s.Help);
        s.HelpLog.Clear();
        uint handled = s.Help.Handle(root.Budget, Input.InputCode.RightButton, 40, 12);
        Assert.Equal(1u, handled);
        Assert.Contains("title:Storm Power Available", s.HelpLog);
        Assert.Contains(s.HelpLog, entry => entry.StartsWith("entry:") && entry.Contains("moneyGumpHelp"));
        Assert.Contains(s.HelpLog, entry => entry.StartsWith("attach:"));
        Assert.Contains(s.HelpLog, entry => entry.StartsWith("sound:"));
        s.HelpLog.Clear();
        handled = s.Help.Handle(root.BridgeGrid, Input.InputCode.RightButton, 60, 42);
        Assert.Equal(1u, handled);
        Assert.Contains("title:Production Window", s.HelpLog);
        Assert.Contains(s.HelpLog, entry => entry.StartsWith("entry:") && entry.Contains("teleportViewHelp"));
        // 빈 HELP는 우클릭을 소비하지 않는다.
        Assert.Equal(0u, s.Help.Handle(root.Budget, Input.InputCode.RightButton | Input.InputCode.Release, 40, 12));
        panel.Close();
    }

    /// <summary>덱 틀을 미션 HUD 왼쪽에 부착하면 그리기 픽셀이 바뀐다.</summary>
    [Fact]
    public void DeckFrameAttachesLeftSidebarAndDraws()
    {
        using var s = new Scene();
        var tree = s.Menu.Tree;
        s.Options[1] = 3;
        var panel = s.NewPanel();
        uint token = panel.Construct();
        panel.SetPalette(s.Screen.LogicalPalette);
        var root = Assert.IsType<ProductionRootNode>(panel.Node(token));
        root.BaseWidth = root.Width = 80;
        float deckHeight = s.Screen.Height - 80;
        root.BaseHeight = root.Height = deckHeight;
        var drawing = new HostGumpDrawing(s.Menu, s.Text, s.Menus, s.Screen);
        byte[] before = s.Screen.Pixels.ToArray();
        panel.Attach(token, 0, 0, tree.Create.Entries[0], 0);
        panel.Idle(root.Budget);
        drawing.Draw();
        byte[] after = s.Screen.Pixels.ToArray();
        Assert.False(before.AsSpan().SequenceEqual(after));
        var position = tree.Collection.Position(token);
        Assert.Equal(new ScreenPoint(0, 0), position);
        Assert.Equal(80f, root.BaseWidth);
        Assert.Equal(deckHeight, root.BaseHeight);
        Assert.Equal(tree.Create.Entries[0], root.Parent);
        panel.Close();
    }

    /// <summary>장기 Form 통지가 실제 생산 노드와 공통 노드의 수명으로 분배되고 기존 삭제 경계는 복구된다.</summary>
    [Fact]
    public void GumpFormNoticeUsesActualRegistryAndRestoresPreviousDelete()
    {
        using var s = new Scene(nativeForms: true); var tree = s.Menu.Tree; var panel = s.NewPanel();
        uint rootToken = panel.Construct(); var root = Assert.IsType<ProductionRootNode>(panel.Node(rootToken));
        uint budget = root.Budget; uint id = panel.Node(budget)!.Id; int changed = 0; int previous = 0;
        Action<uint, uint> fallback = (_, _) => previous++; tree.LifetimeHooks.DeleteForm = fallback;
        using var runtime = new HostGumpFormRuntime(s.Storage, tree, (token, parent) =>
        {
            if (panel.Node(token) is null) return false;
            panel.ParentGone(token, unchecked((uint)parent)); return true;
        }, () => changed++);
        var forms = s.Storage.GumpForms(); int form = s.Storage.Pool.Allocate(); Span<byte> raw = s.Storage.Pool.AllocatedBytes(form); raw.Clear(); raw[10] = 3; forms.SetOwner(form, id);
        forms.Notify(form); Assert.Equal(1, changed); Assert.NotEqual(0u, panel.Node(budget)!.Flags & 0x10000); tree.Lifetime.Cleanup(); Assert.Null(panel.Node(budget));
        uint plain = tree.AllocateNode(new GumpCreateNode()); tree.Factory.Construct(plain, 0, 0, 0, 1); forms.SetOwner(form, tree.Node(plain)!.Id);
        forms.Notify(form); Assert.Equal(2, changed); Assert.NotEqual(0u, tree.Node(plain)!.Flags & 0x10000); tree.Lifetime.Cleanup(); Assert.Null(tree.Node(plain));
        runtime.Dispose(); Assert.Same(fallback, tree.LifetimeHooks.DeleteForm); tree.LifetimeHooks.DeleteForm!(7, 0); Assert.Equal(1, previous);
        Assert.Throws<InvalidOperationException>(() => forms.Notify(form));
    }
}
