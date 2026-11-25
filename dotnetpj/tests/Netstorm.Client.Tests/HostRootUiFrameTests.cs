using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;

// 미션 Root UI가 Native 프레임 경로에서 수집·그려지는지 검증한다.
// 2026-11-26 1.045에서 DI-1 Native Frame/Root UI 수집·그리기용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>Root UI 수집과 그리기 분배를 같은 트리·화면에서 대조한다.</summary>
public sealed class HostRootUiFrameTests
{
    /// <summary>창이나 음향을 열지 않고 원본 자산과 같은 장기 Chat 소유자를 구성한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>동일 실제 팔레트와 8비트 화면이다.</summary>
        public readonly Screen Screen;
        /// <summary>수신 옵션과 HTML 생성기가 공유하는 실제 시작 설정이다.</summary>
        public readonly HostStartupScene Startup;
        /// <summary>같은 HTML과 controlled 메뉴가 사용하는 실제 사건 큐다.</summary>
        public readonly InputQueue Queue = new();
        /// <summary>같은 실제 메뉴와 공통 노드 트리다.</summary>
        public readonly HostMainMenu Menu;
        /// <summary>비교할 기존 HTML 닫기 소유자다.</summary>
        public readonly HostHtmlWindows Html;
        /// <summary>같은 실제 공유 조각과 스크롤 장치다.</summary>
        public readonly HostHtmlText Text;
        /// <summary>같은 실제 Console과 편집 버퍼다.</summary>
        public readonly HostTextEntry Entry;
        /// <summary>실제 controlled 스택이다.</summary>
        public readonly HostHtmlMenus Menus;
        /// <summary>창 교체 뒤에도 유지하는 실제 상태다.</summary>
        public readonly ClientChatWindowGlobals Globals = new();
        /// <summary>실제 정적 say와 clear와 quit 인접 등록표다.</summary>
        public readonly ClientDotCommandGlobals Commands = new();
        /// <summary>같은 실제 전체 Chat 공장이다.</summary>
        public readonly HostChatWindows Chat;
        /// <summary>같은 실제 지오메트리다. 입력 표시 판정에 쓴다.</summary>
        public readonly HudChatGeometry Geometry;
        /// <summary>실제 전체 TYPE과 정의와 셰이프를 한 번 읽는 장기 자산표다.</summary>
        private readonly HostAssetTables _assets;
        /// <summary>같은 SHP 바이트를 읽는 실제 헤더 장치다.</summary>
        private readonly Netstorm.Client.Display.ShapeDatabase _shapes;

        /// <summary>기존 장치를 조립하고 실제 TYPE과 SHP 프레임을 사용한다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); Startup = new HostStartupScene(data); Screen = new(1024, 768); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); Startup.Draw(Screen);
            Menu = new(Startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            Html = new(Menu, Queue, () => 1, _ => { }); Html.BindParser(Startup, _ => { }); Text = new(Menu, Html, Screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Html); Entry = new(Menu, Html, Text, Screen, () => 1, (_, _) => { }); Menus = new(Menu, Html, Text, Startup, Screen, (_, _) => { }); Menus.BindInput(Queue, _ => { }, (_, _) => { });
            _assets = new(Startup, new(OriginalEdition.Patch1078)); _shapes = new(_assets.ShapeImage); var pictures = new MarkupGameAssets(_assets.TypeTable, _assets.Definitions, _shapes); byte[] pictureColors = SpriteColorMapBuilder.BuildSpriteColorMaps(Screen.LogicalPalette).Shadow; Text.BindPictures(pictures, () => pictureColors, Startup.Configuration);
            var runtime = new ClientChatWindowRuntime(Globals, new(OriginalEdition.Patch1078, () => 100), value => value);
            Geometry = new(new() { Chat = token => Menu.Tree.Node(token) as HudChatNode, Node = Menu.Tree.Node, BaseAttach = Menu.Tree.Tree.Attach, Push = Menu.Tree.Tree.Push,
                Attach = (token, x, y, parent, mode) => Menu.Tree.TreeHooks.Attach!(token, x, y, parent, mode), BottomIndex = token => Text.Text.AtBottom(token) ? 1u : 0,
                RequestBottom = (token, extra) => Text.Text.RequestBottom(token, unchecked((int)extra)), Challenge = () => 0, Current = () => Globals.Current.Value });
            var help = new GumpHelp(Menu.Tree.Create, new(), new() { Node = Menu.Tree.Node });
            Chat = new(Menu, Html, Text, Entry, Menus, Screen, Globals, Commands, runtime, Geometry, new()
            {
                Read = address => address switch { 0x541348 => (uint)_assets.TypeTable.Types.Count, 0x548714 => 0x61500001, 0x594fc8 or 0x594fbc => 0, 0x5cae94 => 300, 0x5cae98 => 300, 0x563e38 => Menu.Tree.Create.Entries[0], _ => throw new InvalidOperationException("검사에 없는 실제 전역:" + address.ToString("x")) },
                Text = token => throw new InvalidOperationException("안정 Chat 문자열이 아닌 포인터:" + token), Translate = value => value,
                FrameCount = type => type >= 70 && type - 70 < _assets.Definitions.Count ? _assets.Definitions[(int)type - 70].Clusters.Count : 0,
                FrameHeight = (type, frame) => BinaryPrimitives.ReadInt16LittleEndian(_shapes.SquidHeader((int)type - 70, (int)frame).AsSpan(0x1a, 2)),
                Recipient = value => value is "" or "allies", SendChat = null,
                Config = Startup.Configuration.GetInt, BaseHandle = help.Handle,
            });
        }

        /// <summary>최종 owner 반납 뒤 기존 실제 HTML과 편집 저장소도 정리한다.</summary>
        public void Dispose() { Chat.Dispose(); Html.CloseAll(); Entry.Dispose(); }

        /// <summary>실제 Root처럼 표시 창을 만들고 같은 트리 루트에 부착한다.</summary>
        public uint OpenRootUi()
        {
            uint token = Chat.Create();
            Menu.Tree.TreeHooks.Attach!(token, 0, Screen.Height - 100, Menu.Tree.Create.Entries[0], 0);
            return token;
        }
    }

    /// <summary>Root UI는 같은 Native 프레임 수집에 들어간다.</summary>
    [Fact]
    public void RootUiIsCollectedFromNativeFrame()
    {
        using var scene = new Scene();
        uint token = scene.OpenRootUi();
        var entries = new RenderCollectionEntry[5000];
        int count = scene.Menu.Tree.Collection.Collect(new ScreenRect(0, 0, scene.Screen.Width, scene.Screen.Height), entries, 5000);
        Assert.True(count > 0);
        Assert.Contains(entries[..count], entry => entry.Token == token);
    }

    /// <summary>Root UI가 붙은 트리 그리기는 미연결 진단 없이 끝난다.</summary>
    [Fact]
    public void RootUiDrawsWithoutMissingDiagnostics()
    {
        using var scene = new Scene();
        scene.OpenRootUi();
        var drawing = new HostGumpDrawing(scene.Menu, scene.Text, scene.Menus, scene.Screen, scene.Entry);
        var failure = Record.Exception(drawing.Draw);
        Assert.Null(failure);
    }

    /// <summary>빈 Root 수집은 미연결 진단 없이 끝난다.</summary>
    [Fact]
    public void EmptyRootCollectsWithoutMissingDiagnostics()
    {
        using var scene = new Scene();
        var entries = new RenderCollectionEntry[5000];
        int count = scene.Menu.Tree.Collection.Collect(new ScreenRect(0, 0, scene.Screen.Width, scene.Screen.Height), entries, 5000);
        Assert.True(count >= 0);
        var drawing = new HostGumpDrawing(scene.Menu, scene.Text, scene.Menus, scene.Screen, scene.Entry);
        Assert.Null(Record.Exception(drawing.Draw));
    }
}
