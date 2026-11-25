using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

// 미션 HUD 구분선 세 개를 만들고 도크 배치로 실제 지도 영역을 정한다.
// 2026-11-26 1.075에서 R5-4b용으로 추가했다.
public sealed class HostMissionHudTests
{
    private sealed class Scene
    {
        public Scene(int width = 640, int height = 480)
        {
            var fixture = new GumpFontFixture("gumptext");
            Tree = new GumpButtonTree(width, height, GumpBackgroundTestAssets.Assets(), 82, 0, new(9, 243, 57), fixture.Fonts,
                new() { Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            Menus = new MenuGumpState();
            Globals = new HudDockGlobals(Tree.Create, Tree.TreeState, new UIntBox(), new UIntBox());
        }

        public GumpButtonTree Tree { get; }
        public MenuGumpState Menus { get; }
        public HudDockGlobals Globals { get; }
    }

    private static (HudDividerGump Dividers, HudDock Dock, List<string> Calls) Arrange(Scene scene, uint vertical, uint minimap, uint chat)
    {
        var calls = new List<string>();
        var hooks = new HudDockHooks
        {
            Node = scene.Tree.Node,
            Divider = token => scene.Tree.Node(token) as HudDividerNode,
            ClosePopups = () => { },
            BaseAttach = scene.Tree.Tree.Attach,
            Push = scene.Tree.Tree.Push,
            Attach = scene.Tree.Tree.Attach,
            Move = scene.Tree.Tree.Move,
            ResolveSerial = scene.Tree.Reference,
            Bubble = (token, code, first, second) => scene.Tree.Events.Bubble(token, code, first, second),
            ConstructBackground = (token, width, height, flags, type, frame) => scene.Tree.Backgrounds.Construct(token, width, height, flags, 1, type, frame),
            Activate = (token, enabled) => scene.Tree.Lifetime.SetOverlay(token, enabled != 0),
            Invalidate = (_, _, _, _, _) => calls.Add("invalidate"),
            FinishResize = () => calls.Add("resize"),
            ChatVisible = _ => throw new InvalidOperationException("예상 밖 채팅 표시"),
            ChatInput = _ => throw new InvalidOperationException("예상 밖 채팅 입력"),
            BaseHandle = (_, _, _) => throw new InvalidOperationException("예상 밖 기본 입력"),
        };
        var dividers = new HudDividerGump(() => scene.Tree.Create.ScreenWidth, () => scene.Tree.Create.ScreenHeight, () => (sbyte)55, hooks);
        var dock = new HudDock(part => part switch
        {
            HudDockPart.Vertical => vertical,
            HudDockPart.MinimapDivider => minimap,
            HudDockPart.ChatDivider => chat,
            _ => 0,
        }, scene.Globals, () => (sbyte)55, hooks);
        return (dividers, dock, calls);
    }

    /// <summary>구분선 기본 배치와 도크 배치로 지도 영역이 실제 HUD 배치를 가리킨다.</summary>
    [Fact]
    public void DividersAndArrangeSetMapView()
    {
        var scene = new Scene();
        uint root = scene.Tree.Create.Entries[0];
        uint vertical = scene.Tree.WithNodeOwner(this, () => scene.Tree.AllocateNode(new HudDividerNode()));
        uint minimap = scene.Tree.WithNodeOwner(this, () => scene.Tree.AllocateNode(new HudDividerNode()));
        uint chat = scene.Tree.WithNodeOwner(this, () => scene.Tree.AllocateNode(new HudDividerNode()));
        var (dividers, dock, _) = Arrange(scene, vertical, minimap, chat);
        dividers.Construct(vertical, 0, 5, 480, 0x03400000, 82, 0);
        scene.Tree.Tree.Attach(vertical, 80, 0, root, 0);
        dividers.Construct(minimap, 0, 80, 5, 0x02a00000, 82, 0);
        scene.Tree.Tree.Attach(minimap, 0, 400, root, 0);
        dividers.Construct(chat, 18, 640, 5, 0x02a00000, 82, 0);
        scene.Tree.Tree.Attach(chat, 85, 384, root, 0);
        dock.RegisterNormalLayout();
        GumpCreateNode verticalNode = scene.Tree.Node(vertical)!;
        GumpCreateNode chatNode = scene.Tree.Node(chat)!;
        dock.Handle(0x0200003a, verticalNode.Serial, 0);
        dock.Handle(0x0200003a, chatNode.Serial, 0);
        Assert.Equal(new ScreenRect(85, 0, 640, 384), scene.Tree.TreeState.MapView);
        Assert.Equal((85, 555, 384), (scene.Globals.MapLeft, scene.Globals.MapWidth, scene.Globals.MapHeight));
    }

    /// <summary>구분선 없이는 지도 영역이 비어 있다.</summary>
    [Fact]
    public void MissingDividersKeepMapEmpty()
    {
        var scene = new Scene();
        Assert.Equal(default, scene.Tree.TreeState.MapView);
    }
}
