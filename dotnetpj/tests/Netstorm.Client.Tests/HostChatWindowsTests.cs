using System.Buffers.Binary;
using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Chat;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Client.Scripting;
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
        /// <summary>수신 옵션/HTML 생성기가 공유하는 실제 시작 설정이다.</summary>
        public readonly HostStartupScene Startup;
        /// <summary>같은 HTML/controlled 메뉴가 사용하는 실제 사건 큐다.</summary>
        public readonly InputQueue Queue = new();
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
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var startup = Startup = new HostStartupScene(data); Screen = new(1024, 768); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(Screen);
            Menu = new(startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            var queue = Queue; Html = new(Menu, queue, () => 1, _ => { }); Html.BindParser(startup, _ => { }); Text = new(Menu, Html, Screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Html); Entry = new(Menu, Html, Text, Screen, () => 1, (_, _) => { }); Menus = new(Menu, Html, Text, startup, Screen, (_, _) => { }); Menus.BindInput(queue, _ => { }, (_, _) => { }); Drawing = new(Menu, Text, Menus, Screen, Entry);
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

    /// <summary>실제 채팅 Enter와 부모 사건이 숨김·재열기·지도 영역을 왕복하며 라우터 반납 뒤에는 전달되지 않는다.</summary>
    [Fact]
    public void MissionHudRoutesChatOpenAndHideThroughRealTree()
    {
        using var scene = new Scene();
        var tree = scene.Tree;
        uint vertical = tree.AllocateNode(new HudDividerNode());
        uint divider = tree.AllocateNode(new HudDividerNode());
        uint chat = scene.Chat.Create();
        var globals = new HudDockGlobals(tree.Create, tree.TreeState, new UIntBox(), new UIntBox());
        var strings = new ScriptStringRing();
        using var htmlFactory = new HostHtmlFactory(scene.Menu, scene.Html, scene.Menus, scene.Startup, scene.Screen, scene.Queue, strings, () => 1, _ => 0, _ => { });
        var input = new HostMenuInput(scene.Menu, scene.Html, htmlFactory, scene.Queue, strings, () => { }, action => throw new InvalidOperationException(action.ToString()));
        input.BindChatEnter(scene.Chat.OpenInput);
        input.ChatToggleKey = scene.Chat.ToggleKey; // F3도 같은 실제 도킹/입력줄을 왕복한다.
        scene.Menu.IsMissionActive = () => true;
        HudDock? dock = null;
        HostMissionHudRouting? routing = null;
        var hooks = new HudDockHooks
        {
            Node = tree.Node,
            Divider = token => tree.Node(token) as HudDividerNode,
            BaseAttach = tree.Tree.Attach,
            Push = tree.Tree.Push,
            Attach = (token, x, y, parent, mode) =>
            {
                if (token == vertical || token == divider) dock!.Attach(token, x, y, parent, mode);
                else tree.TreeHooks.Attach!(token, x, y, parent, mode);
            },
            Move = tree.Tree.Move,
            ResolveSerial = tree.Reference,
            Bubble = (token, code, first, second) => tree.Events.Bubble(token, code, first, second),
            ClosePopups = () => scene.Menus.RequestClosePopups(() => { }),
            ConstructBackground = (token, width, height, flags, type, frame) => tree.Backgrounds.Construct(token, width, height, flags, 1, type, frame),
            Activate = (token, enabled) => tree.Lifetime.SetOverlay(token, enabled != 0),
            Invalidate = (_, _, _, _, _) => { },
            ChatVisible = visible => scene.Chat.Geometry.SetHelperVisible(chat, visible ? 1U : 0U),
            ChatInput = visible => scene.Chat.Geometry.SetInputVisible(chat, visible ? 1U : 0U),
            BaseHandle = (code, first, second) => routing!.HandleBase(code, first, second),
        };
        dock = new HudDock(part => part switch
        {
            HudDockPart.Root => tree.Create.Entries[0],
            HudDockPart.Vertical => vertical,
            HudDockPart.ChatDivider => divider,
            HudDockPart.Chat => chat,
            _ => 0,
        }, globals, () => 55, hooks);
        using (routing = new HostMissionHudRouting(tree, dock))
        {
            dock.Construct(vertical, 0, 5, 768, 0x03400000, scene.Menu.BackgroundType, 0);
            dock.Construct(divider, 18, 1024, 5, 0x02a00000, scene.Menu.BackgroundType, 0);
            tree.Tree.Attach(vertical, 80, 0, tree.Create.Entries[0], 0);
            tree.Tree.Attach(divider, 85, 672, tree.Create.Entries[0], 0);
            var chatNode = Assert.IsType<HudChatNode>(tree.Node(chat));
            chatNode.BaseWidth = chatNode.Width = 939;
            chatNode.BaseHeight = chatNode.Height = 91;
            tree.TreeHooks.Attach!(chat, 85, 677, tree.Create.Entries[0], 0);
            dock.RegisterSlave(vertical, chat, 4);
            dock.RegisterSlave(divider, chat, 8);
            dock.Handle(0x0200003a, tree.Node(vertical)!.Serial, 0);
            dock.Handle(0x02000088, 0, 0);
            Assert.Equal(768, tree.Node(divider)!.Y);
            Assert.Equal(768, tree.TreeState.MapView.Bottom);
            Assert.False(input.Dispatch(new InputEvent(13, 0, 0)));
            Assert.Equal(672, tree.Node(divider)!.Y);
            Assert.Equal(672, tree.TreeState.MapView.Bottom);
            Assert.NotEqual(0U, tree.Node(chatNode.Console)!.Flags & 1U);
            tree.EventHooks.Handle!(chat, 0x02000115, 0, scene.Globals.Store("draft message"));
            string draft = scene.Entry.ReadEntryText(chatNode.Console); // 숨겨도 보존해야 할 실제 입력 버퍼다.
            Assert.False(input.Dispatch(new InputEvent(0x720000, 0, 0)));
            Assert.Equal(768, tree.Node(divider)!.Y);
            Assert.Equal(768, tree.TreeState.MapView.Bottom);
            Assert.Equal(draft, scene.Entry.ReadEntryText(chatNode.Console));
            Assert.False(input.Dispatch(new InputEvent(0x720000 | InputCode.Release, 0, 0))); Assert.Equal(768, tree.Node(divider)!.Y);
            Assert.False(input.Dispatch(new InputEvent(0x720000, 0, 0))); Assert.Equal(672, tree.Node(divider)!.Y);
            Assert.Equal(draft, scene.Entry.ReadEntryText(chatNode.Console));
            Assert.False(input.Dispatch(new InputEvent(0x720000, 0, 0))); Assert.Equal(768, tree.Node(divider)!.Y);
        }
        scene.Chat.Show();
        Assert.Equal(768, tree.Node(divider)!.Y);
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

    /// <summary>실제 비정렬 패킷 필드를 지정 폭으로 기록한다.</summary>
    private static void Put(Span<byte> bytes, int at, uint value, int width = 4)
    {
        // 지정한 실제 필드 밖의 바이트를 보존한다.
        for (int index = 0; index < width; index++) bytes[at + index] = (byte)(value >> (index * 8));
    }

    /// <summary>고정41h 수신기에서 같은 실제 창/공유 로그/버튼으로 전달하고 현재 무시 목록을 적용한다.</summary>
    [Fact]
    public void ActualPacketReceiverAppendsSharedLogAndPulsesRegisteredButton()
    {
        using var scene = new Scene(); var subscribers = new ClientSubscriberGlobals(); subscribers.MySubscriber.Value = 100;
        var game = new ClientGameplayGlobals(subscribers); var local = new ClientLocalStateGlobals(game, new()); var connections = new ClientConnectionGlobals(local);
        var surface = new ClientSurfaceGlobals(game, new(local), new Kernel(), () => 1000); game.Authority.Server = false;
        var identity = new ClientChatIdentityRuntime(game, surface, subscribers, new(OriginalEdition.Patch1078, () => 100), () => "Youpson");
        var sounds = new List<string>(); var receiver = new HostChatReceive(game, surface, subscribers, scene.Globals, scene.Startup.Configuration, sender => identity.FindSenderOwner(sender), scene.Tree, scene.Screen, scene.Chat,
            () => throw new InvalidOperationException("비전투에서 미선택 월드"), new() { Sound = sounds.Add });
        var dispatch = new Netstorm.Net.RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 100, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });
        ClientChatPacketReceiver.Install(game, surface, connections, scene.Globals, dispatch, receiver.Receive);
        dispatch.InvokeDirect(0x41, 9, null, ReadOnlyMemory<byte>.Empty); Assert.Equal(0u, game.PlayerRecords.Base);
        uint root = scene.Chat.Create(); uint alert = scene.Tree.AddStone(20, 20, "Chat\0"u8.ToArray(), 10, 10); var button = Assert.IsType<StoneButtonGumpNode>(scene.Tree.Node(alert));
        scene.Tree.ChatAlertId.Value = button.Id; byte[] all = Enumerable.Repeat((byte)0xd3, 48).ToArray(); Memory<byte> packet = all.AsMemory(5, 36);
        Put(packet.Span, 0, 1, 1); Put(packet.Span, 5, 1, 1); Put(packet.Span, 6, 100); Put(packet.Span, 10, 0, 2); Put(packet.Span, 12, 7, 2); "hello\n!"u8.CopyTo(packet.Span[15..]);
        dispatch.InvokeDirect(0x41, 9, null, packet); Assert.Equal(1, scene.Globals.Log.LineCount); Assert.Equal(0, packet.Span[21]); Assert.Empty(sounds); Assert.Contains(alert, scene.Tree.LifeState.Idle); Assert.Equal(0x10u, button.ButtonFlags & 0x10);
        Put(packet.Span, 6, 200); dispatch.InvokeDirect(0x41, 9, null, packet); Assert.Equal(2, scene.Globals.Log.LineCount); Assert.Equal(new[] { "newChatMessage.wav" }, sounds);
        scene.Globals.Ignored.Add(200); dispatch.InvokeDirect(0x41, 9, null, packet); Assert.Equal(2, scene.Globals.Log.LineCount); Assert.Single(sounds);
        Assert.Equal(root, scene.Globals.Current.Value); Assert.Equal(0u, game.PlayerRecords.Base); Assert.All(all.Take(5), value => Assert.Equal(0xd3, value)); Assert.All(all.Skip(41), value => Assert.Equal(0xd3, value));
        button.ButtonFlags &= ~0x10u; scene.Tree.Lifetime.SetIdle(alert, false); button.Pressed = 1; Assert.False(scene.Tree.PulseRegisteredIfIdle(button.Id)); button.Pressed = 0; button.Value = 1; Assert.False(scene.Tree.PulseRegisteredIfIdle(button.Id)); button.Value = 0;
        uint saved = scene.Tree.Create.Entries[button.Id]; scene.Tree.Create.Entries[button.Id] = 0;
        try { Assert.False(scene.Tree.PulseRegisteredIfIdle(button.Id)); } finally { scene.Tree.Create.Entries[button.Id] = saved; }
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Tree.PulseRegisteredIfIdle(1000));
    }

    /// <summary>실제 원문 옵션0을 누락과 구별하고 뉴스 저장 후의 현재 본문을 같은 HTML 창으로 전달한다.</summary>
    [Fact]
    public void ActualReceptionUsesZeroOptionsAndCurrentNewsMapView()
    {
        using var scene = new Scene(); var subscribers = new ClientSubscriberGlobals(); var game = new ClientGameplayGlobals(subscribers); var local = new ClientLocalStateGlobals(game, new());
        var surface = new ClientSurfaceGlobals(game, new(local), new Kernel(), () => 1000); surface.Storage.OwnerMode.Battle = true;
        var identity = new ClientChatIdentityRuntime(game, surface, subscribers, new(OriginalEdition.Patch1078, () => 100), () => "Youpson");
        var factory = new HostHtmlFactory(scene.Menu, scene.Html, scene.Menus, scene.Startup, scene.Screen, scene.Queue, new(), () => 1, _ => 0, _ => { });
        uint root = scene.Chat.Create(); byte[] packet = new byte[64]; Put(packet, 0, 1, 1); Put(packet, 5, 1, 1); Put(packet, 6, 100); Put(packet, 10, 0, 2); Put(packet, 12, 9, 2); "008hello\0"u8.CopyTo(packet.AsSpan(15));
        // 숨김 메시지도 전체10옵션을 조회한다. 실제 값0을 모두 등록한다.
        foreach (string key in RawChatReceive.OptionKeys) scene.Startup.Configuration.Set(key, "0");
        int saved = 0, mapReads = 0; uint newsSerial = 0; ScreenRect map = new(100, 80, 900, 680);
        var receiver = new HostChatReceive(game, surface, subscribers, scene.Globals, scene.Startup.Configuration, sender => identity.FindSenderOwner(sender), scene.Tree, scene.Screen, scene.Chat,
            () => throw new InvalidOperationException("숨김에서 미선택 관계표"), new()
            {
                SaveNewsTime = () => { saved++; Assert.Equal(9u, scene.Globals.NewsTime.Value); "changed\0"u8.CopyTo(packet.AsSpan(24)); },
                News = (text, width, height) => { Assert.Equal("changed", text); Assert.Equal(400u, width); Assert.Equal(30u, height); scene.Html.SetMapView(() => { mapReads++; return map; }); newsSerial = factory.Factory.Show(text, 0, width, height); },
            });
        receiver.Receive(packet); Assert.Equal(0, scene.Globals.Log.LineCount);
        scene.Startup.Configuration.Set(RawChatReceive.OptionKeys[^1], ""); Assert.Throws<InvalidOperationException>(() => receiver.Receive(packet)); scene.Startup.Configuration.Set(RawChatReceive.OptionKeys[^1], "0");
        Put(packet, 5, 0x81, 1); Put(packet, 6, 0); Put(packet, 10, 9, 2); Put(packet, 12, 8, 2); packet[15] = 1; Put(packet, 19, 9); "before!\0"u8.CopyTo(packet.AsSpan(24));
        receiver.Receive(packet); Assert.Equal(1, saved); Assert.True(mapReads > 0); uint news = scene.Tree.Reference(newsSerial); var window = Assert.IsType<HtmlWindowGumpNode>(scene.Tree.Node(news)); Assert.Equal((800 - window.BaseWidth) * .5f + 100, window.X); Assert.Equal((600 - window.BaseHeight) * .5f + 80, window.Y); Assert.Equal(root, scene.Globals.Current.Value);
        int previousReads = mapReads; receiver.Receive(packet); Assert.Equal(1, saved); Assert.Equal(previousReads, mapReads);
        map = new(150, 100, 850, 600); scene.Tree.TreeHooks.Attach!(news, 0, 0, scene.Tree.Create.Entries[0], 0); Assert.Equal((700 - window.BaseWidth) * .5f + 150, window.X); Assert.Equal((500 - window.BaseHeight) * .5f + 100, window.Y);
        uint followup = scene.Tree.Reference(factory.Factory.Show("followup", 0, 400, 30)); var normal = Assert.IsType<HtmlWindowGumpNode>(scene.Tree.Node(followup)); Assert.Equal((700 - normal.BaseWidth) * .5f + 150, normal.X); Assert.Equal((500 - normal.BaseHeight) * .5f + 100, normal.Y); factory.Dispose();
    }
}
