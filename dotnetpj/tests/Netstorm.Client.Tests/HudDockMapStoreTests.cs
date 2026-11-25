using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// RU-4b 같은 지도 owner의 실제 HUD 작성자가 지도 저장소를 작성한다. 0.993에서 추가됐다.
public sealed class HudDockMapStoreTests
{
    /// <summary>제품 설치와 같은 주인·저장소·전역 모양으로 조립한다. 창이나 음향은 열지 않는다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>동일 실제 팔레트와 8비트 화면이다.</summary>
        public readonly Screen Screen;
        /// <summary>수신 옵션과 HTML 생성기가 공유하는 실제 시작 설정이다.</summary>
        public readonly HostStartupScene Startup;
        /// <summary>같은 실제 메뉴와 공통 트리다.</summary>
        public readonly HostMainMenu Menu;
        /// <summary>같은 실제 Deck/MapRevision owner다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>제품과 같은 HUD 전역이다. 같은 Create/TreeState/Deck/MapRevision을 빌린다.</summary>
        public readonly HudDockGlobals Globals;
        /// <summary>제품과 같은 HUD 도킹 본체다. 토큰은 실제 전체 부모에서 읽는다.</summary>
        public readonly HudDock Dock;
        /// <summary>제품 Invalidate의 같은 메뉴 dirty 통지 횟수다.</summary>
        public int Dirty;

        /// <summary>제품 ClientHostGame.HudDock.cs의 InitializeNativeHudDock과 같은 순서로 조립한다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); Startup = new HostStartupScene(data); Screen = new(1024, 768); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); Startup.Draw(Screen);
            Menu = new(Startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            Globals = new(Menu.Tree.Create, Menu.Tree.TreeState, Game.FullDeckBox, Game.MapRevision);
            Menu.Tree.Create.ScreenWidth = 1280;
            Assert.Equal(1280, Globals.ScreenWidth);
            Menu.Tree.Create.ScreenWidth = 1024;
            var hooks = new HudDockHooks
            {
                Node = Menu.Tree.Node,
                Divider = token => Menu.Tree.Node(token) as HudDividerNode,
                BaseAttach = Menu.Tree.Tree.Attach,
                Push = Menu.Tree.Tree.Push,
                Attach = (token, x, y, parent, mode) => Menu.Tree.TreeHooks.Attach!(token, x, y, parent, mode),
                Move = Menu.Tree.Tree.Move,
                ResolveSerial = Menu.Tree.Reference,
                Bubble = (token, code, first, second) => Menu.Tree.Events.Bubble(token, code, first, second),
                ClosePopups = () => { },
                Invalidate = (_, _, _, _, _) => Dirty++,
                FinishResize = () => throw new InvalidOperationException("HUD 화면 후처리는 R5 표시 연결에서 잇는다"),
                ChatVisible = _ => throw new InvalidOperationException("HUD 채팅 표시는 RU-5 Chat 명령 효과에서 잇는다"),
                ChatInput = _ => throw new InvalidOperationException("HUD 채팅 입력는 RU-5 Chat 명령 효과에서 잇는다"),
                BaseHandle = (_, _, _) => throw new InvalidOperationException("HUD 기본 입력은 R5 입력 연결에서 잇는다"),
            };
            Func<HudDockPart, uint> tokens = part =>
            {
                uint root = Menu.Tree.Create.Entries[0];
                if (Menu.Tree.Node(root) is not UberRootNode node) throw new InvalidOperationException("실제 HUD 부모 미설치(RU-3 나머지에서 잇는다)");
                if (part == HudDockPart.Root) return root;
                return part switch
                {
                    HudDockPart.Vertical => node.Vertical,
                    HudDockPart.MinimapDivider => node.MinimapDivider,
                    HudDockPart.ChatDivider => node.ChatDivider,
                    HudDockPart.Minimap => node.Minimap,
                    HudDockPart.Production => node.Production,
                    HudDockPart.Chat => node.Chat,
                    HudDockPart.Knowledge => node.Knowledge,
                    _ => throw new ArgumentOutOfRangeException(nameof(part), "Uber 현재 HUD 필드 종류 오류"),
                };
            };
            Dock = new(tokens, Globals, () => 55, hooks);
        }

        /// <summary>자기 UI는 메뉴 트리가 소유하므로 별도 반납이 없다.</summary>
        public void Dispose() { }
    }

    /// <summary>설치는 같은 Deck/MapRevision DWORD owner를 공유한다.</summary>
    [Fact]
    public void InstallSharesSameDeckAndRevisionOwners()
    {
        using var scene = new Scene();
        scene.Game.FullDeckBox.Value = 5;
        Assert.Equal(5u, scene.Globals.ShowHud);
        scene.Game.MapRevision.Value = 27;
        Assert.Equal(27u, scene.Globals.FullRefresh);
        scene.Globals.FullRefresh = 28;
        Assert.Equal(28u, scene.Game.MapRevision.Value);
    }

    /// <summary>HUD 전역의 지도 쓰기가 같은 메뉴 TreeState와 CurrentMapView에 보인다.</summary>
    [Fact]
    public void MapRectWritesSameStoreReadByCurrentMapView()
    {
        using var scene = new Scene();
        scene.Globals.MapRect = new(85, 0, 1024, 672);
        Assert.Equal(new ScreenRect(85, 0, 1024, 672), scene.Menu.Tree.TreeState.MapView);
        Assert.Equal(scene.Menu.Tree.TreeState.MapView, scene.Menu.CurrentMapView);
        scene.Menu.Tree.TreeState.MapView = new(103, 11, 900, 599);
        Assert.Equal(scene.Menu.Tree.TreeState.MapView, scene.Globals.MapRect);
        Assert.Equal(scene.Globals.MapRect, scene.Menu.CurrentMapView);
    }

    /// <summary>미설치 지도 영역을 전체 화면으로 대체하지 않는다.</summary>
    [Fact]
    public void NoFullViewportFallbackForUnsetMapView()
    {
        using var scene = new Scene();
        Assert.Equal(default(ScreenRect), scene.Menu.CurrentMapView);
        Assert.NotEqual(new ScreenRect(0, 0, scene.Screen.Width, scene.Screen.Height), scene.Menu.CurrentMapView);
    }

    /// <summary>다른 지도 owner는 같은 값이라도 별도 저장소이므로 같은 정렬기가 읽지 않는다.</summary>
    [Fact]
    public void RejectsDifferentViewOwnerBeforeBinding()
    {
        using var scene = new Scene();
        var separate = new GumpTreeState { MapView = scene.Menu.Tree.TreeState.MapView };
        var other = new HudDockGlobals(scene.Menu.Tree.Create, separate, new(), new());
        other.MapRect = new(1, 2, 3, 4);
        Assert.NotEqual(other.MapRect, scene.Menu.CurrentMapView);
        Assert.Equal(default(ScreenRect), scene.Menu.CurrentMapView);
    }

    /// <summary>전체 부모가 없으면 토큰 조회가 명시 실패한다.</summary>
    [Fact]
    public void MissingRootTokenFailsExplicitly()
    {
        using var scene = new Scene();
        var error = Assert.Throws<InvalidOperationException>(() => scene.Dock.Handle(0x0200003d, 3, 0));
        Assert.Contains("HUD", error.Message);
    }

    /// <summary>미연결 후단은 명시 실패하며 이전 지도 상태로 되돌리지 않는다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitlyWithoutRevertingMap()
    {
        using var scene = new Scene();
        scene.Globals.MapRect = new(85, 0, 1024, 672);
        Assert.Throws<InvalidOperationException>(() => scene.Dock.Handle(0x02000099, 0, 0));
        Assert.Equal(new ScreenRect(85, 0, 1024, 672), scene.Menu.CurrentMapView);
    }
}
