using System.Buffers.Binary;
using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Chat;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 전체 실제 공장에 기존 Console/조각/메뉴/수명을 연결하여 경계를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 설치 자산을 읽는 전체 Chat UI 공장의 조합 검사다.창/소리/파일 저장은 실행하지 않는다.</summary>
public sealed class HostChatWindowsTests
{
    /// <summary>기존 실제 UI 장치와 같은 장기 Chat 소유자를 구성한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>동일 실제 팔레트/8비트 화면이다.</summary>
        public readonly Screen Screen;
        /// <summary>같은 실제 메뉴와 공통 노드 트리다.</summary>
        public readonly HostMainMenu Menu;
        /// <summary>비교할 기존 HTML 닫기 소유자다.</summary>
        public readonly HostHtmlWindows Html;
        /// <summary>같은 실제 공유 조각/스크롤 장치다.</summary>
        public readonly HostHtmlText Text;
        /// <summary>같은 실제 Console/편집 버퍼다.</summary>
        public readonly HostTextEntry Entry;
        /// <summary>실제 controlled 스택이다.</summary>
        public readonly HostHtmlMenus Menus;
        /// <summary>실제 그리기 결선도 같은 저장소를 빌린다.</summary>
        public readonly HostGumpDrawing Drawing;
        /// <summary>창 교체 뒤에도 유지하는 실제 상태다.</summary>
        public readonly ClientChatWindowGlobals Globals = new();
        /// <summary>실제 정적 say와 clear/quit 인접 등록표다.</summary>
        public readonly ClientDotCommandGlobals Commands = new();
        /// <summary>같은 실제 전체 Chat 공장이다.</summary>
        public readonly HostChatWindows Chat;
        /// <summary>외부 제출에 전달된 실제 manager/편집 본문 관측이다.</summary>
        public readonly List<(uint Manager, string Text)> Sent = [];
        /// <summary>실제 전체 TYPE/정의/셰이프를 한 번 읽는 장기 자산표다.</summary>
        private readonly HostAssetTables _assets;
        /// <summary>같은 SHP 바이트를 읽는 실제 헤더 장치다.</summary>
        private readonly Netstorm.Client.Display.ShapeDatabase _shapes;
        /// <summary>현재 실제 공통 트리다.</summary>
        public GumpButtonTree Tree => Menu.Tree;
        /// <summary>기존 장치를 조립하고 실제 TYPE/SHP 프레임을 사용한다.</summary>
        public Scene(bool submit = true)
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var startup = new HostStartupScene(data); Screen = new(1024, 768); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(Screen);
            Menu = new(startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            var queue = new InputQueue(); Html = new(Menu, queue, () => 1, _ => { }); Text = new(Menu, Html, Screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Html); Entry = new(Menu, Html, Text, Screen, () => 1, (_, _) => { }); Menus = new(Menu, Html, Text, startup, Screen, (_, _) => { }); Menus.BindInput(queue, _ => { }, (_, _) => { }); Drawing = new(Menu, Text, Menus, Screen, Entry);
            _assets = new(startup, new(OriginalEdition.Patch1078)); _shapes = new(_assets.ShapeImage); var pictures = new MarkupGameAssets(_assets.TypeTable, _assets.Definitions, _shapes); byte[] pictureColors = SpriteColorMapBuilder.BuildSpriteColorMaps(Screen.LogicalPalette).Shadow; Text.BindPictures(pictures, () => pictureColors, startup.Configuration);
            var runtime = new ClientChatWindowRuntime(Globals, new(OriginalEdition.Patch1078, () => 100), value => value);
            var geometry = new HudChatGeometry(new() { Chat = token => Tree.Node(token) as HudChatNode, Node = Tree.Node, BaseAttach = Tree.Tree.Attach, Push = Tree.Tree.Push,
                Attach = (token, x, y, parent, mode) => Tree.TreeHooks.Attach!(token, x, y, parent, mode), BottomIndex = token => Text.Text.AtBottom(token) ? 1u : 0,
                RequestBottom = (token, extra) => Text.Text.RequestBottom(token, unchecked((int)extra)), Challenge = () => 0, Current = () => Globals.Current.Value });
            var help = new GumpHelp(Tree.Create, new(), new() { Node = Tree.Node });
            Chat = new(Menu, Html, Text, Entry, Menus, Screen, Globals, Commands, runtime, geometry, new()
            {
                Read = address => address switch { 0x541348 => (uint)_assets.TypeTable.Types.Count, 0x548714 => 0x61500001, 0x594fc8 or 0x594fbc => 0, 0x5cae94 => 300, 0x5cae98 => 300, 0x563e38 => Tree.Create.Entries[0], _ => throw new InvalidOperationException("검사에 없는 실제 전역:" + address.ToString("x")) },
                Text = token => throw new InvalidOperationException("안정 Chat 문자열이 아닌 포인터:" + token), Translate = value => value,
                FrameCount = type => type >= 70 && type - 70 < _assets.Definitions.Count ? _assets.Definitions[(int)type - 70].Clusters.Count : 0,
                FrameHeight = (type, frame) => BinaryPrimitives.ReadInt16LittleEndian(_shapes.SquidHeader((int)type - 70, (int)frame).AsSpan(0x1a, 2)),
                Recipient = value => value is "" or "allies", SendChat = submit ? (manager, value) => Sent.Add((manager, value)) : null,
                Config = startup.Configuration.GetInt, BaseHandle = help.Handle,
            });
        }
        /// <summary>최종 owner 반납 뒤 기존 실제 HTML/편집 저장소도 정리한다.</summary>
        public void Dispose() { Chat.Dispose(); Html.CloseAll(); Entry.Dispose(); }
    }
    /// <summary>전체 실제 자식 생성/가상 부착/그리기·입력·controlled payload와 HTML 닫기 경계를 확인한다.</summary>
    [Fact]
    public void RealFactoryCreatesAndRoutesSameTreeConsoleAndMenus()
    {
        using var scene = new Scene(); uint root = scene.Chat.Create(); var node = Assert.IsType<HudChatNode>(scene.Tree.Node(root)); Assert.Equal(root, scene.Globals.Current.Value); Assert.Equal(root, scene.Chat.CurrentRoot); Assert.Equal(3u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Commands.Bytes.AsSpan(0xa04, 4)));
        var console = Assert.IsType<ConsoleGumpNode>(scene.Tree.Node(node.Console)); var log = Assert.IsType<ScrollingTextGumpNode>(scene.Tree.Node(node.Text)); Assert.Equal(0x548758u, log.Buffer); Assert.Same(scene.Globals.Log, scene.Text.Buffers.Binding(log.Buffer).State); Assert.Equal("Chat Helper", scene.Tree.ReadOwnedText(scene.Tree.Node(node.Helper)!.TextPointer)); Assert.NotEmpty(scene.Globals.Window.Icons);
        scene.Tree.TreeHooks.Attach!(root, 40, 100, scene.Tree.Create.Entries[0], 0); Assert.Equal(scene.Tree.Create.Entries[0], node.Parent); scene.Drawing.Draw(); scene.Html.CloseAll(); Assert.Same(node, scene.Tree.Node(root)); Assert.NotNull(scene.Entry.FindBuffer(console.Buffer));
        uint payload = scene.Globals.Store("allies:hello"); scene.Tree.EventHooks.Handle!(root, 0x02000115, 0, payload); Assert.Equal("allies:hello", scene.Entry.ReadEntryText(node.Console)); Assert.Equal(1u, scene.Tree.EventHooks.Handle(root, 0x02000038, console.Serial, 0)); Assert.Single(scene.Sent); Assert.Equal(0x61500001u, scene.Sent[0].Manager); Assert.Contains("hello", scene.Sent[0].Text); Assert.Equal("allies:", scene.Entry.ReadEntryText(node.Console));
        scene.Tree.EventHooks.Handle(root, 0x0200002c, scene.Tree.Node(node.Helper)!.Serial, 0); Assert.NotEmpty(scene.Globals.Strings); Assert.Equal(0u, scene.Menus.Menu.Current()); Assert.True(scene.Globals.Strings.Count > 500); scene.Drawing.Draw(); Assert.Equal("allies:hello", scene.Globals.Strings[payload]); scene.Html.CloseAll(); Assert.Same(node, scene.Tree.Node(root));
    }
    /// <summary>표시 교체는 공유 로그/명령/후보를 유지하고 최종 owner만 실제 조각을 반납한다.</summary>
    [Fact]
    public void WindowReplacementPreservesSharedLogAndUnconditionalCurrentDestruction()
    {
        using var scene = new Scene(); uint first = scene.Chat.Create(); scene.Chat.Print("kept\n"); scene.Drawing.Draw(); Assert.Equal(1, scene.Globals.Log.LineCount); int icons = scene.Globals.Window.Icons.Count; uint second = scene.Chat.Create(); Assert.NotEqual(first, second); Assert.Equal(1, scene.Globals.Log.LineCount); Assert.Equal(icons, scene.Globals.Window.Icons.Count); Assert.Equal(3u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Commands.Bytes.AsSpan(0xa04, 4)));
        scene.Tree.Lifetime.Kill(first); scene.Tree.Lifetime.Cleanup(); Assert.NotNull(scene.Tree.Node(second)); Assert.Equal(0u, scene.Globals.Current.Value); scene.Chat.Close(); Assert.Empty(scene.Tree.OwnedNodes(scene.Chat)); Assert.True(scene.Globals.Log.Initialized); Assert.Equal(1, scene.Globals.Log.LineCount); Assert.Equal(0, scene.Tree.OwnedTextCount);
        uint third = scene.Chat.Create(); Assert.NotEqual(second, third); Assert.Equal(1, scene.Globals.Log.LineCount); scene.Chat.Dispose(); Assert.False(scene.Globals.Log.Initialized); Assert.Null(scene.Globals.UiOwner); Assert.Empty(scene.Tree.OwnedNodes(scene.Chat)); Assert.Throws<ObjectDisposedException>(() => scene.Chat.Create());
    }
    /// <summary>미선택 제출은 요구하지 않고 실제 Enter 도달에서 정상 성공으로 바꾸지 않는다.</summary>
    [Fact]
    public void MissingSubmissionFailsOnlyAfterRealEnter()
    {
        using var scene = new Scene(false); uint root = scene.Chat.Create(); var node = Assert.IsType<HudChatNode>(scene.Tree.Node(root)); var console = Assert.IsType<ConsoleGumpNode>(scene.Tree.Node(node.Console)); uint payload = scene.Globals.Store("hello"); scene.Tree.InputHooks.Handle!(root, 0x02000115, 0, payload); Assert.Equal("hello", scene.Entry.ReadEntryText(node.Console)); Assert.Throws<InvalidOperationException>(() => scene.Tree.EventHooks.Handle!(root, 0x02000038, console.Serial, 0)); Assert.Empty(scene.Sent); Assert.Equal(root, scene.Globals.Current.Value);
    }
}
