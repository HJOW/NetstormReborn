using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

// 같은 실제 트리/메뉴에 전체 HUD 부모 공장을 묶는다.
// 2026-11-26 1.074에서 R5-4a용으로 추가했다.
public sealed class HostUberRootTests
{
    private sealed class Scene
    {
        public Scene()
        {
            var fixture = new GumpFontFixture("gumptext");
            Tree = new GumpButtonTree(640, 480, GumpBackgroundTestAssets.Assets(), 82, 0, new(9, 243, 57), fixture.Fonts,
                new() { Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            Menus = new MenuGumpState();
            Globals = new HudDockGlobals(Tree.Create, Tree.TreeState, new UIntBox(), new UIntBox());
        }

        public GumpButtonTree Tree { get; }
        public MenuGumpState Menus { get; }
        public HudDockGlobals Globals { get; }
    }

    private static uint PlainNode(Scene scene) => scene.Tree.AllocateNode(new GumpCreateNode());

    private static (HostUberRootFactories Factories, HostUberRootDevices Devices, List<string> Calls) Stubs(Scene scene)
    {
        var calls = new List<string>();
        var factories = new HostUberRootFactories
        {
            CreateMinimap = () => PlainNode(scene),
            CreateProduction = () => PlainNode(scene),
            CreateKnowledge = () => PlainNode(scene),
            CreateChat = () => PlainNode(scene),
        };
        var devices = new HostUberRootDevices
        {
            Poll = _ => new(InputCode.Release, 0, 0),
            Cursor = _ => { },
            BaseHandle = (_, _, _, _) => 0,
            ClosePopups = () => { },
            Changed = () => calls.Add("Changed"),
            Replaced = token => calls.Add($"Replaced:{token}"),
            Read = address => address switch { 0x5c78f0 => 640, 0x5c78f4 => 480, _ => 0 },
            Write = (_, _) => { },
            Type = _ => 0,
            Translate = _ => 0,
            Help = (_, _) => { },
            HelpWord = (_, _) => { },
            SavedMessage = () => 0,
            EqualText = (_, _) => false,
            ConstructMessage = (_, _) => { },
            FormatMessage = _ => 0,
            TestMessage = _ => 0,
            VisitMessage = _ => { },
            MessageEvent = (_, _, _, _) => { },
            RefreshTechnology = () => calls.Add("Refresh"),
            DockInvalidate = (_, _, _, _, _) => { },
            DockFinishResize = () => { },
            DockChatVisible = _ => { },
            DockChatInput = _ => { },
            DockBaseHandle = (_, _, _) => 0,
        };
        return (factories, devices, calls);
    }

    /// <summary>조립한 전체 부모를 교체하면 루트·구분선·네 자식이 살아 있고 구분선 제한이 선다.</summary>
    [Fact]
    public void ReplaceCreatesRootAndDividers()
    {
        var scene = new Scene();
        var (factories, devices, calls) = Stubs(scene);
        var controls = HostUberRoot.Assemble(scene.Tree, scene.Menus, scene.Globals, factories, devices);
        Assert.False(controls.Current());
        controls.Replace();
        Assert.True(controls.Current());
        var root = Assert.IsType<UberRootNode>(scene.Tree.Node(scene.Tree.Create.Entries[0]));
        Assert.NotEqual(0u, root.Vertical);
        Assert.NotEqual(0u, root.MinimapDivider);
        Assert.NotEqual(0u, root.ChatDivider);
        Assert.NotEqual(0u, root.Minimap);
        Assert.NotEqual(0u, root.Production);
        Assert.NotEqual(0u, root.Knowledge);
        Assert.NotEqual(0u, root.Chat);
        var vertical = Assert.IsType<HudDividerNode>(scene.Tree.Node(root.Vertical));
        Assert.Equal(60, vertical.MinX);
        var minimap = Assert.IsType<HudDividerNode>(scene.Tree.Node(root.MinimapDivider));
        Assert.Equal(480 - 123, minimap.MinY);
        Assert.Equal(480, minimap.MaxY);
        var chat = Assert.IsType<HudDividerNode>(scene.Tree.Node(root.ChatDivider));
        Assert.Equal(25, chat.MinY);
        Assert.Equal(480 - 25, chat.MaxY);
        Assert.Contains("Refresh", calls);
        Assert.Contains(calls, call => call.StartsWith("Replaced:", StringComparison.Ordinal));
    }

    /// <summary>빠진 자식 공장은 교체 위치에서 거부된다.</summary>
    [Fact]
    public void MissingFactoryThrowsOnReplace()
    {
        var scene = new Scene();
        var (factories, devices, _) = Stubs(scene);
        factories.CreateChat = null;
        var controls = HostUberRoot.Assemble(scene.Tree, scene.Menus, scene.Globals, factories, devices);
        Assert.Throws<InvalidOperationException>(() => controls.Replace());
    }
}
