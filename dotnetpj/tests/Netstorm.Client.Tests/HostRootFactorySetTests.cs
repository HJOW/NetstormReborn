using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 실제 게임 Root의 네 공장 소유를 검사한다.

// RU-3b Root와 메시지 공장 집합의 실제 소유 계약이다. 0.987에서 추가됐다.
public sealed class HostRootFactorySetTests
{
    /// <summary>창이나 음향을 열지 않고 원본 자산과 같은 장기 Gameplay와 Surface와 파일 고리를 빌린다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>실제 현재 화면이다.</summary>
        public readonly Screen Screen = new(1024, 768);
        /// <summary>같은 실제 최초 자산 로더다.</summary>
        public readonly HostStartupScene Startup;
        /// <summary>공통 메뉴와 같은 노드 소유자다.</summary>
        public readonly HostMainMenu Menu;
        /// <summary>기존 HTML 소유자다.</summary>
        public readonly HostHtmlWindows Html;
        /// <summary>한 줄과 여러 줄 문자와 조각을 공유한다.</summary>
        public readonly HostHtmlText Text;
        /// <summary>실제 PlayerKnown과 Surface의 권한과 보조 owner다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>같은 실제 자산과 현재 모드 저장소다.</summary>
        public readonly RawSurfaceStorage Storage;
        /// <summary>현재 같은 NSInfo 전역이다.</summary>
        public readonly ClientNsInfoGlobals Globals = new();
        /// <summary>같은 실제 원문 파일 이름 작성기다. VFS와 소유권을 공유한다.</summary>
        public readonly RawFileName FileNames = new();
        /// <summary>실제 문자열 고리의 현재 내용이다.</summary>
        public readonly Dictionary<uint, string> Strings = [];
        /// <summary>같은 Root 실장치 갈고리다.</summary>
        public readonly HostNsInfoRootDevicesHooks RootHooks;
        /// <summary>같은 메시지 공장 갈고리다.</summary>
        public readonly UberMessageProbeHooks MessageHooks;
        /// <summary>같은 바깥 UI 장치 묶음이다.</summary>
        public readonly HostNsInfoDevices BaseDevices;
        /// <summary>검사할 실제 Root 공장 집합이다.</summary>
        public readonly HostRootFactorySet Set;

        /// <summary>기존 실제 메뉴와 문자와 자산을 조립한다. 선택하지 않은 입력과 메타와 특수 그림은 공급하지 않는다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); Startup = new HostStartupScene(data);
            Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); Startup.Draw(Screen);
            Menu = new(Startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            var queue = new InputQueue(); Html = new(Menu, queue, () => 1, _ => { });
            Html.BindParser(Startup, _ => { });
            Text = new(Menu, Html, Screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Html);
            var menus = new HostHtmlMenus(Menu, Html, Text, Startup, Screen, (_, _) => { }); menus.BindInput(queue, _ => { }, (_, _) => { });
            Storage = new(OriginalEdition.Patch1078, Game.Authority, Game.GameAux); var assets = new HostAssetTables(Startup, Storage); var shapes = new Netstorm.Client.Display.ShapeDatabase(assets.ShapeImage);
            var pictures = new MarkupGameAssets(assets.TypeTable, assets.Definitions, shapes); byte[] colors = SpriteColorMapBuilder.BuildSpriteColorMaps(Screen.LogicalPalette).Shadow; Text.BindPictures(pictures, () => colors, Startup.Configuration);
            Globals.ServerSelection.Value = -1; Globals.Zone.Value = 17;
            BaseDevices = new()
            {
                Read = address => address switch { 0x5ca920 => Menu.BackgroundType, 0x5ca924 => 1, 0x5411c8 => Menu.BackgroundType, 0x5bec2c => Screen.Colors().Basic[4], 0x5bec3c => Screen.Colors().Basic[2], 0x5b5e58 => Screen.Colors().Basic[3], 0x5c78d0 => Screen.Colors().Basic[0], 0x59afd8 => Screen.Colors().Basic[1], 0x5c85c8 => Screen.Colors().Basic[5], _ when address >= 0x5accf0u && address < 0x5accf0u + 32u => Screen.Colors().Basic[6], _ => throw new InvalidOperationException("검사에서 공급하지 않은 현재 전역:" + address.ToString("x")) },
                Translate = value => value,
                StoreText = value => { uint token = 0x2000000 + (uint)Strings.Count; Strings.Add(token, value); return token; },
                Text = token => Strings[token],
                FormatZone = (format, server, zone) => format.Replace("%s", server, StringComparison.Ordinal).Replace("%d", zone.ToString(System.Globalization.CultureInfo.InvariantCulture), StringComparison.Ordinal),
                Changed = () => { },
            };
            RootHooks = new()
            {
                SendStatus = _ => { },
                KickoffSound = () => { },
                BattleGuySound = () => { },
                ParticipantMenu = (_, _) => { },
                CenterOnSid = _ => { },
                MousePoint = () => new ScreenPoint(0, 0),
                BaseInput = (_, _, _) => 0u,
                MetaRootLayout = _ => { },
                MetaChildLayout = (_, _) => { },
                PlayerMenuBegin = () => { },
                PlayerMenuAttach = () => { },
                PlayerMenuTitle = _ => { },
                PlayerMenuItem = (_, _, _) => { },
                BattleSortMenu = _ => { },
                BattleOptionsMenu = (_, _) => { },
                BattleExcludeMenu = (_, _) => { },
                BattleOptionMenu = (_, _, _) => { },
                BattleRemoveMenu = _ => { },
            };
            MessageHooks = new()
            {
                BackgroundName = () => "testbg",
                StoredText = token => Strings[token],
                StoreText = value => { uint token = 0x3000000 + (uint)Strings.Count; Strings.Add(token, value); return token; },
                TryRead = _ => null,
                LogProbe = _ => { },
                ChangePalette = _ => { },
                Construct = (_, _) => { },
                MessageEvent = (_, _, _, _) => { },
            };
            Set = new(Menu.Tree, Text, Globals, Game, Storage, FileNames, new() { BaseDevices = BaseDevices, Root = RootHooks, Message = MessageHooks });
        }

        /// <summary>자기 공장과 기존 HTML의 실제 소유 자식만 반납한다.</summary>
        public void Dispose() { Set.Dispose(); Html.CloseAll(); }
    }

    /// <summary>첫 NSInfo창과 메시지 공장을 만들고 같은 파일 고리를 공유한다.</summary>
    [Fact]
    public void CreatesPrimaryAndMessageProbeSharingRing()
    {
        using var scene = new Scene();
        Assert.Equal(1, scene.Set.Count);
        Assert.NotNull(scene.Set.Primary);
        Assert.NotNull(scene.Set.MessageProbe);
        Assert.Same(scene.FileNames, scene.Set.FileNames);
        Assert.True(scene.Set.OwnsToken(scene.Globals.Current.Value));
    }

    /// <summary>State 지연 삭제가 실제 첫 창을 반납한 뒤 같은 공장·전역으로 새 첫 창을 만든다.</summary>
    [Fact]
    public void RecreatePrimaryAfterStateCleanupUsesSameFactoryAndUpdatesToken()
    {
        using var scene = new Scene();
        uint old = scene.Globals.Current.Value;
        var primary = scene.Set.Primary;
        Assert.Throws<InvalidOperationException>(() => scene.Set.RecreatePrimary());
        scene.Menu.Tree.KillStateGumps();
        scene.Menu.Tree.Lifetime.Cleanup();
        Assert.Null(scene.Menu.Tree.Node(old));
        uint made = scene.Set.RecreatePrimary();
        Assert.NotEqual(old, made);
        Assert.Same(primary, scene.Set.Primary);
        Assert.Equal(made, scene.Globals.Current.Value);
        Assert.True(scene.Set.OwnsToken(made));
        Assert.False(scene.Set.OwnsToken(old));
        Assert.Equal(1, scene.Set.Count);
    }

    /// <summary>둘째와 세째 NSInfo창을 만들고 세 토큰을 구별한다.</summary>
    [Fact]
    public void CreateExtraAddsSecondAndThirdWindows()
    {
        using var scene = new Scene();
        uint first = scene.Globals.Current.Value;
        int second = scene.Set.CreateExtra();
        uint secondToken = scene.Globals.Current.Value;
        int third = scene.Set.CreateExtra();
        uint thirdToken = scene.Globals.Current.Value;
        Assert.Equal(1, second);
        Assert.Equal(2, third);
        Assert.Equal(3, scene.Set.Count);
        Assert.NotEqual(first, secondToken);
        Assert.NotEqual(secondToken, thirdToken);
        Assert.NotEqual(first, thirdToken);
        Assert.True(scene.Set.OwnsToken(first));
        Assert.True(scene.Set.OwnsToken(secondToken));
        Assert.True(scene.Set.OwnsToken(thirdToken));
        Assert.Same(scene.Set.Primary, scene.Set.At(0));
        Assert.NotSame(scene.Set.At(0), scene.Set.At(1));
        Assert.NotSame(scene.Set.At(1), scene.Set.At(2));
    }

    /// <summary>F6 등 바깥 토큰은 어느 창도 소유하지 않는다.</summary>
    [Fact]
    public void ForeignTokenNotOwned()
    {
        using var scene = new Scene();
        scene.Set.CreateExtra();
        scene.Set.CreateExtra();
        Assert.False(scene.Set.OwnsToken(0x12345678u));
        Assert.False(scene.Set.OwnsToken(0u));
    }

    /// <summary>메시지 공장은 같은 공유 고리로 배경 경로를 만든다.</summary>
    [Fact]
    public void MessageProbeFormatsWithSharedRing()
    {
        using var scene = new Scene();
        uint token = scene.Set.MessageProbe.FormatMessage([0xfffffffeu, 0x503c8bu, 0xfffffffeu, 0x508ab4u, 0xfffffffeu, RawUberMessageProbe.SavedMessage, 0xfffffffeu, 0x51501cu, 0u]);
        string path = scene.Strings[token];
        Assert.Equal(@".\d\testbg.col", path);
    }

    /// <summary>닫으면 모든 토큰 소유가 풀린다.</summary>
    [Fact]
    public void CloseAllReleasesOwnership()
    {
        using var scene = new Scene();
        scene.Set.CreateExtra();
        uint token = scene.Globals.Current.Value;
        Assert.True(scene.Set.OwnsToken(token));
        scene.Set.CloseAll();
        Assert.Equal(0, scene.Set.Count);
        Assert.False(scene.Set.OwnsToken(token));
    }

    /// <summary>반납 뒤에는 창을 만들거나 빌릴 수 없다.</summary>
    [Fact]
    public void DisposeReleasesAll()
    {
        var scene = new Scene();
        scene.Set.CreateExtra();
        scene.Dispose();
        Assert.Throws<ObjectDisposedException>(() => scene.Set.CreateExtra());
        Assert.Throws<ObjectDisposedException>(() => scene.Set.At(0));
        Assert.False(scene.Set.OwnsToken(1u));
    }

    /// <summary>빈 갈고리로는 만들지 않는다.</summary>
    [Fact]
    public void NullHooksRejected()
    {
        using var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => new HostRootFactorySet(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, scene.FileNames, null!));
        Assert.Throws<ArgumentNullException>(() => new HostRootFactorySet(null!, scene.Text, scene.Globals, scene.Game, scene.Storage, scene.FileNames, new() { BaseDevices = scene.BaseDevices, Root = scene.RootHooks, Message = scene.MessageHooks }));
    }
}
