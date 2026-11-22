using System.Globalization;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 설치 자산과 실제 공통 UI 장치의 NSInfo 결선을 검사한다.

/// <summary>실제 전체 생성/공통 수명과 아직 선택하지 않은 메타 후속의 경계를 확인한다.</summary>
public sealed class HostNsInfoWindowsTests
{
    /// <summary>창이나 음향을 열지 않고 원본 자산과 같은 장기 Gameplay/Surface를 빌린다.</summary>
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
        /// <summary>한 줄/여러 줄 문자와 조각을 공유한다.</summary>
        public readonly HostHtmlText Text;
        /// <summary>같은 실제 그림 분배다.</summary>
        public readonly HostGumpDrawing Drawing;
        /// <summary>실제 PlayerKnown과 Surface의 권한/보조 owner다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>같은 실제 자산/현재 모드 저장소다.</summary>
        public readonly RawSurfaceStorage Storage;
        /// <summary>현재 같은 NSInfo 전역이다.</summary>
        public readonly ClientNsInfoGlobals Globals = new();
        /// <summary>검사할 실제 장기 공장이다.</summary>
        public readonly HostNsInfoWindows Ns;
        /// <summary>실제 문자열 고리의 현재 내용이다.</summary>
        public readonly Dictionary<uint, string> Strings = [];
        /// <summary>현재 형식화 장치의 명시적 실패 관문이다.</summary>
        public bool FailFormat;
        /// <summary>실제 현재 PlayerInfo 버튼 TYPE 번호다.</summary>
        public uint ButtonType;
        /// <summary>실제 공통 변경 통지 횟수다.</summary>
        public int Changes;
        /// <summary>실제 번역으로 전달한 원문이다.</summary>
        public readonly List<string> Translations = [];
        /// <summary>같은 실제 외부 장치를 다른 owner 검증에도 제공한다.</summary>
        public readonly HostNsInfoDevices Devices;
        /// <summary>기존 실제 메뉴/문자/자산을 조립한다. 선택하지 않은 입력·메타·특수 그림은 공급하지 않는다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var startup = Startup = new HostStartupScene(data);
            Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(Screen);
            Menu = new(startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            var queue = new InputQueue(); Html = new(Menu, queue, () => 1, _ => { }); Html.BindParser(startup, _ => { });
            Text = new(Menu, Html, Screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Html);
            var menus = new HostHtmlMenus(Menu, Html, Text, startup, Screen, (_, _) => { }); menus.BindInput(queue, _ => { }, (_, _) => { }); Drawing = new(Menu, Text, menus, Screen);
            Storage = new(OriginalEdition.Patch1078, Game.Authority, Game.GameAux); var assets = new HostAssetTables(startup, Storage); var shapes = new Netstorm.Client.Display.ShapeDatabase(assets.ShapeImage);
            var pictures = new MarkupGameAssets(assets.TypeTable, assets.Definitions, shapes); byte[] colors = SpriteColorMapBuilder.BuildSpriteColorMaps(Screen.LogicalPalette).Shadow; Text.BindPictures(pictures, () => colors, startup.Configuration);
            Globals.ServerSelection.Value = -1; Globals.Zone.Value = 17;
            Devices = new()
            {
                Read = address => address switch { 0x5ca920 => Menu.BackgroundType, 0x5ca924 => 1, 0x5411c8 => ButtonType, 0x5bec3c => Screen.Colors().Basic[7], _ => throw new InvalidOperationException("검사에서 공급하지 않은 현재 전역:" + address.ToString("x")) },
                Translate = value => { Translations.Add(value); return value == "LAN Server" ? "Local" : value; },
                StoreText = value => { uint token = 0x2000000 + (uint)Strings.Count; Strings.Add(token, value); return token; }, Text = token => Strings[token],
                FormatZone = (format, server, zone) => FailFormat ? throw new IOException("실제 형식화 중단") : format.Replace("%s", server, StringComparison.Ordinal).Replace("%d", zone.ToString(CultureInfo.InvariantCulture), StringComparison.Ordinal),
                Changed = () => Changes++,
            };
            Ns = new(Menu.Tree, Text, Globals, Game, Storage, Devices);
        }
        /// <summary>자기 공장과 기존 HTML의 실제 소유 자식만 반납한다.</summary>
        public void Dispose() { Ns.Dispose(); Html.CloseAll(); }
    }

    /// <summary>원문이 숨긴 루트도 실제 파생/슬라이더/문자/돌 버튼을 전부 만들고 같은 그림에 연결한다.</summary>
    [Fact]
    public void FullCreationUsesActualAssetsAndSharedTextTree()
    {
        using var scene = new Scene(); var tree = scene.Menu.Tree; int fragments = scene.Text.Fragments.Fragments.Count; scene.Text.SingleState.Alignment = -3;
        uint token = scene.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(tree.Node(token)); Assert.Equal(token, scene.Globals.Current.Value); Assert.Equal(token, scene.Ns.Current()); Assert.Equal(0u, root.Flags & 1); Assert.Equal(6u, root.Command);
        var owned = tree.OwnedNodes(scene.Ns).Select(tree.Node).ToArray(); Assert.Equal(8, owned.OfType<HudPlayerInfoNode>().Count()); Assert.Single(owned.OfType<HudBattleInfoNode>()); Assert.Equal(8, owned.OfType<HudPlayerButtonNode>().Count()); Assert.Equal(8, owned.Count(node => node?.GetType() == typeof(StoneButtonGumpNode))); Assert.Equal(9, owned.OfType<TextGumpNode>().Count()); Assert.Equal(12, owned.OfType<ScrollingTextGumpNode>().Count()); Assert.NotEmpty(owned.OfType<ScrollGumpNode>());
        Assert.Contains("~w~ELocal\nPlayers in Zone 17\n", scene.Strings.Values); Assert.Contains("~r~EBATTLEMASTER", scene.Translations); Assert.True(scene.Text.Fragments.Fragments.Count > fragments);
        // 실제 자식 포인터/부모/버튼 파생과 공통 한 줄 종류 전역을 확인한다.
        foreach (uint player in root.Players)
        {
            var node = Assert.IsType<HudPlayerInfoNode>(tree.Node(player)); Assert.Equal(root.Background, node.Parent); Assert.Equal(0x100000u, node.ButtonType);
            var master = Assert.IsType<TextGumpNode>(tree.Node(node.Master)); Assert.NotEqual(0u, master.Head); Assert.Equal(unchecked((uint)-3), master.Command); Assert.Equal(player, master.Parent); Assert.True(master.BaseWidth > 0);
            var button = Assert.IsType<HudPlayerButtonNode>(tree.Node(node.Button)); Assert.Equal(6u, button.ButtonFlags & 6); Assert.Equal(player, button.Parent);
        }
        scene.Drawing.Draw(); scene.Html.CloseAll(); Assert.Same(root, tree.Node(token)); Assert.Same(tree, scene.Text.Tree);
        root.BaseWidth = 211; root.BaseHeight = 123; tree.TreeHooks.Attach!(token, 10, 20, tree.Create.Entries[0], 0);
        var background = Assert.IsType<GumpBackgroundNode>(tree.Node(root.Background)); Assert.Equal(211, background.BaseWidth); Assert.Equal(123, background.BaseHeight); Assert.Equal(token, background.Parent);
        scene.Ns.Close(); Assert.Empty(tree.OwnedNodes(scene.Ns)); Assert.Equal(fragments, scene.Text.Fragments.Fragments.Count); Assert.Equal(token, scene.Globals.Current.Value); Assert.Equal(0u, scene.Ns.Current()); Assert.Equal(1, scene.Changes); Assert.NotNull(tree.Node(tree.Create.Entries[0]));
        uint second = scene.Ns.Create(); Assert.NotEqual(token, second); Assert.Equal(second, scene.Ns.Current());
    }

    /// <summary>생성 도중 실제 예외는 current를 게시하지 않고 부분 자식/스크롤/조각을 같은 수명으로 정리한다.</summary>
    [Fact]
    public void InterruptedCreationKeepsCurrentAndReleasesPartialPhysicalState()
    {
        using var scene = new Scene(); var tree = scene.Menu.Tree; int fragments = scene.Text.Fragments.Fragments.Count; scene.Globals.Current.Value = 777; scene.FailFormat = true;
        Assert.Throws<IOException>(() => scene.Ns.Create()); Assert.Equal(777u, scene.Globals.Current.Value); Assert.NotEmpty(tree.OwnedNodes(scene.Ns)); scene.Ns.Close(); Assert.Empty(tree.OwnedNodes(scene.Ns)); Assert.Equal(fragments, scene.Text.Fragments.Fragments.Count); Assert.Equal(777u, scene.Globals.Current.Value);
        scene.FailFormat = false; Assert.NotEqual(0u, scene.Ns.Create());
    }

    /// <summary>현재 모드/known을 매번 같은 owner에서 읽고 선택된 메타 본체만 기본 부착 뒤에 요구한다.</summary>
    [Fact]
    public void CurrentModeSelectsMissingMetaOnlyAfterActualBaseEffects()
    {
        using var scene = new Scene(); var tree = scene.Menu.Tree; uint token = scene.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(tree.Node(token));
        scene.Storage.OwnerMode.Challenge = true; scene.Game.Player.PlayerKnown = 0; root.BaseWidth = 130; root.BaseHeight = 90; tree.TreeHooks.Attach!(token, 4, 5, tree.Create.Entries[0], 0);
        tree.Tree.Push(token); scene.Game.Player.PlayerKnown = 1; root.BaseWidth = 230; root.BaseHeight = 190;
        Assert.Throws<InvalidOperationException>(() => tree.TreeHooks.Attach!(token, 6, 7, tree.Create.Entries[0], 0)); Assert.Equal(tree.Create.Entries[0], root.Parent); Assert.Equal(230, tree.Node(root.Background)!.BaseWidth); Assert.Equal(190, tree.Node(root.Background)!.BaseHeight);
        uint player = root.Players[0]; tree.Tree.Push(player); Assert.Throws<InvalidOperationException>(() => tree.TreeHooks.Attach!(player, 8, 9, root.Background, 0)); Assert.Equal(root.Background, tree.Node(player)!.Parent);
    }

    /// <summary>선택한 입력 장치를 빈 성공으로 처리하지 않으며 그 실패 뒤 전체 수명을 정리할 수 있다.</summary>
    [Fact]
    public void MissingInputFailsAtSelectedParentWithoutChangingCurrent()
    {
        using var scene = new Scene(); uint token = scene.Ns.Create(); Assert.Throws<InvalidOperationException>(() => scene.Menu.Tree.EventHooks.Handle!(token, 0x02000038, 0, 0)); Assert.Equal(token, scene.Ns.Current());
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token)); var player = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(root.Players[0])); var clip = new GumpDrawState(); var target = new GumpPixelTarget(new byte[16], 4, 4, clip); var background = new GumpBackgroundDraw(scene.Menu.Tree.Collection, clip, new(), new() { Node = id => scene.Menu.Tree.Node(id) as GumpBackgroundNode });
        Assert.Throws<InvalidOperationException>(() => scene.Menu.Tree.ExternalDrawing.TryDraw(player.Button, target, new(), background));
        scene.Ns.Dispose(); Assert.Empty(scene.Menu.Tree.OwnedNodes(scene.Ns)); Assert.Equal(0, scene.Menu.Tree.ExternalDrawing.RegistrationCount); Assert.Throws<ObjectDisposedException>(() => scene.Ns.Create());
    }

    /// <summary>서로 다른 실제 Gameplay/Surface 혼용은 노드/가상 출력 등록 전에 거부한다.</summary>
    [Fact]
    public void RejectsDifferentPhysicalOwnerBeforeRegistration()
    {
        using var scene = new Scene(); int count = scene.Menu.Tree.ExternalDrawing.RegistrationCount; var other = new ClientGameplayGlobals();
        Assert.Throws<ArgumentException>(() => new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, other, scene.Storage, scene.Devices)); Assert.Equal(count, scene.Menu.Tree.ExternalDrawing.RegistrationCount); Assert.Empty(scene.Menu.Tree.OwnedNodes(scene.Ns));
        var withoutSource = new RawSurfaceStorage(OriginalEdition.Patch1078, scene.Game.Authority, scene.Game.GameAux); withoutSource.Types.AddRange(scene.Storage.Types);
        Assert.Throws<ArgumentException>(() => new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, withoutSource, scene.Devices)); Assert.Equal(count, scene.Menu.Tree.ExternalDrawing.RegistrationCount);
        var separatelyLoaded = new RawSurfaceStorage(OriginalEdition.Patch1078, scene.Game.Authority, scene.Game.GameAux); _ = new HostAssetTables(scene.Startup, separatelyLoaded);
        Assert.Equal(scene.Storage.Types.Count, separatelyLoaded.Types.Count); Assert.Throws<ArgumentException>(() => new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, separatelyLoaded, scene.Devices)); Assert.Equal(count, scene.Menu.Tree.ExternalDrawing.RegistrationCount);
    }

    /// <summary>월드 표가 늘어도 최초 로딩 TYPE 밖의 주소를 유효한 레코드로 만들지 않는다.</summary>
    [Fact]
    public void TypeLookupUsesInitialAssetBoundsAfterCurrentWorldTableChanges()
    {
        using var scene = new Scene(); scene.ButtonType = (uint)scene.Storage.Types.Count; scene.Storage.Types.AddRange([scene.Storage.Types[0].Clone()]);
        Assert.True(scene.ButtonType < scene.Storage.Types.Count); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Ns.Create()); Assert.Equal(0u, scene.Globals.Current.Value); Assert.NotEmpty(scene.Menu.Tree.OwnedNodes(scene.Ns)); scene.Ns.Close(); Assert.Empty(scene.Menu.Tree.OwnedNodes(scene.Ns));
    }
}
