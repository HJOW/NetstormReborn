// 미션 HUD 미니맵 부착과 L4a 대조용 그리기·HELP를 검증한다.
// 2026-11-26 1.080에서 R5-4c3용으로 추가했다.
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>미니맵의 실제 HUD 왼쪽 아래 부착·지형 갱신·그리기를 같은 트리에서 대조한다.</summary>
public sealed class HostMissionMinimapDeckTests
{
    /// <summary>창이나 음향을 열지 않고 원본 자산과 같은 장기 저장소·공통 트리를 구성한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>동일 실제 팔레트와 8비트 화면이다.</summary>
        public readonly Screen Screen;
        /// <summary>수신 옵션과 HTML 생성기가 공유하는 실제 시작 설정이다.</summary>
        public readonly HostStartupScene Startup;
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
        /// <summary>같은 실제 그리기 장치다.</summary>
        public readonly HostGumpDrawing Drawing;
        /// <summary>실제 PlayerKnown과 Surface의 권한·보조 owner다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>같은 실제 자산·현재 모드 저장소다.</summary>
        public readonly RawSurfaceStorage Storage;
        /// <summary>같은 공유 bitmap·큐다.</summary>
        public readonly MetadisplayDirtyState State = new();
        /// <summary>같은 전용 팔레트·현재 토큰·serial이다.</summary>
        public readonly RawMinimapGlobals Globals = new();
        /// <summary>같은 실제 장기 미니맵 처리기다. Open 대신 Construct로 만든다.</summary>
        public readonly RawMinimapControls Mini;

        /// <summary>제품 설치와 같은 주인·저장소·전역·후크 모양으로 조립한다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); Startup = new HostStartupScene(data); Screen = new(1024, 768); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); Startup.Draw(Screen);
            Menu = new(Startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            Html = new(Menu, new Client.Input.InputQueue(), () => 1, _ => { }); Html.BindParser(Startup, _ => { }); Text = new(Menu, Html, Screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Html); Entry = new(Menu, Html, Text, Screen, () => 1, (_, _) => { }); Menus = new(Menu, Html, Text, Startup, Screen, (_, _) => { }); Menus.BindInput(new(), _ => { }, (_, _) => { }); Drawing = new(Menu, Text, Menus, Screen, Entry);
            Storage = new(OriginalEdition.Patch1078, Game.Authority, Game.GameAux); _ = new HostAssetTables(Startup, Storage);
            var baseHandle = Menu.Tree.EventHooks.Handle;
            Mini = new(Menu.Tree, Storage, State, Globals, Game.MapRevision, new()
            {
                View = () => new MinimapGeometryView(0, 0, Menu.CurrentMapView),
                Center = (_, _) => throw new InvalidOperationException("미니맵 중앙 카메라는 R5 카메라 연결에서 잇는다"),
                Move = (_, _) => throw new InvalidOperationException("미니맵 선택 이동은 R5 입력 연결에서 잇는다"),
                InputMode = () => throw new InvalidOperationException("미니맵 입력 모드는 R5 입력 연결에서 잇는다"),
                Poll = () => throw new InvalidOperationException("미니맵 입력 폴링은 R5 입력 연결에서 잇는다"),
                BaseHandle = (token, code, first, second) => (baseHandle ?? throw new InvalidOperationException("미니맵 실제 기본 입력 미설치"))(token, code, first, second),
                Invalidate = (_, _) => { },
                Changed = () => { },
            });
            Mini.Construct();
            Mini.SetPalette(Screen.Colors());
        }

        /// <summary>자기 UI와 기존 HTML·편집 저장소를 정리한다.</summary>
        public void Dispose() { Mini.Dispose(); Html.CloseAll(); Entry.Dispose(); }
    }

    /// <summary>미니맵을 HUD 왼쪽 아래 80×75에 부착하고 지형을 갱신해 그린다.</summary>
    [Fact]
    public void MinimapAttachesHudLeftBottomAndDraws()
    {
        using var scene = new Scene();
        var tree = scene.Menu.Tree;
        Assert.NotEqual(0u, scene.Mini.RootToken);
        var node = scene.Mini.Node(scene.Mini.RootToken)!;
        Assert.NotEqual(0u, node.TextPointer);
        Assert.NotEqual(0u, node.AlternateTextPointer);
        Assert.Equal("Sky Overview", tree.ReadOwnedText(node.TextPointer));
        Assert.Equal("minimapGump", tree.ReadOwnedText(node.AlternateTextPointer));
        var drawing = scene.Drawing;
        // 부착 전에는 수집돼도 bitmap이 비어 그려지지 않는다.
        byte[] before = scene.Screen.Pixels.ToArray();
        node.BaseWidth = node.Width = 80;
        node.BaseHeight = node.Height = 75;
        scene.Mini.Attach(scene.Mini.RootToken, 0, scene.Screen.Height - 75, scene.Menu.Tree.Create.Entries[0], 0);
        Assert.Equal(80f, node.BaseWidth);
        Assert.Equal(75f, node.BaseHeight);
        Assert.Equal(new ScreenPoint(0, scene.Screen.Height - 75), tree.Collection.Position(scene.Mini.RootToken));
        scene.Mini.Refresh();
        Assert.NotEmpty(scene.State.Bitmap);
        drawing.Draw();
        byte[] after = scene.Screen.Pixels.ToArray();
        Assert.False(before.AsSpan().SequenceEqual(after));
        string report = scene.Mini.Report();
        Assert.Contains("minimap", report);
        Assert.Contains("0\t693", report);
    }

    /// <summary>같은 자리 재부착은 같은 토큰·위치를 유지한다.</summary>
    [Fact]
    public void MinimapResizeIsIdempotent()
    {
        using var scene = new Scene();
        var tree = scene.Menu.Tree;
        uint token = scene.Mini.RootToken;
        var node = scene.Mini.Node(token)!;
        node.BaseWidth = node.Width = 80;
        node.BaseHeight = node.Height = 75;
        scene.Mini.Attach(token, 0, scene.Screen.Height - 75, tree.Create.Entries[0], 0);
        var first = tree.Collection.Position(token);
        scene.Mini.Resize(scene.Screen.Height);
        Assert.Equal(first, tree.Collection.Position(token));
        Assert.Equal(token, scene.Mini.RootToken);
        Assert.Equal(token, scene.Globals.Current);
    }
}
