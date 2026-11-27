using System.Globalization;
using System.Text;
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
        /// <summary>같은 실제 소유자 조회다.</summary>
        public readonly Dictionary<uint, uint> Owners = [];
        /// <summary>실제 소유자 조회 호출이다.</summary>
        public readonly List<uint> OwnerCalls = [];
        /// <summary>실제 기술 합계 값이다.</summary>
        public uint TechWeightValue = 5;
        /// <summary>실제 기술 합계 호출 횟수다.</summary>
        public int TechWeightCalls;
        /// <summary>실제 랭크 그림 호출이다.</summary>
        public readonly List<int> RankPictureCalls = [];
        /// <summary>실제 번역으로 전달한 원문이다.</summary>
        public readonly List<string> Translations = [];
    /// <summary>실제 TYPE 설명 호출이다. 0.923에서 추가됐다.</summary>
    public readonly List<uint> DescribedTypes = [];
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
                Read = address => address switch { 0x5ca920 => Menu.BackgroundType, 0x5ca924 => 1, 0x5411c8 => ButtonType, 0x5bec2c => Screen.Colors().Basic[4], 0x5bec3c => Screen.Colors().Basic[2], 0x5b5e58 => Screen.Colors().Basic[3], 0x5c78d0 => Screen.Colors().Basic[0], 0x59afd8 => Screen.Colors().Basic[1], 0x5c85c8 => 0u, 0x5c78b0 => Screen.Colors().Basic[5], _ when address >= 0x5accf0u && address < 0x5accf0u + 32u => Screen.Colors().Basic[6], _ => throw new InvalidOperationException("검사에서 공급하지 않은 현재 전역:" + address.ToString("x")) },
                Translate = value => { Translations.Add(value); return value == "LAN Server" ? "Local" : value; },
                StoreText = value => { uint token = 0x2000000 + (uint)Strings.Count; Strings.Add(token, value); return token; }, Text = token => Strings[token],
                FormatZone = (format, server, zone) => FailFormat ? throw new IOException("실제 형식화 중단") : format.Replace("%s", server, StringComparison.Ordinal).Replace("%d", zone.ToString(CultureInfo.InvariantCulture), StringComparison.Ordinal),
                Changed = () => Changes++,
                ObjectOwner = sid => { OwnerCalls.Add(sid); return Owners.TryGetValue(sid, out uint owner) ? owner : 0u; },
                TechWeight = _ => { TechWeightCalls++; return TechWeightValue; },
                RankPicture = rank => { RankPictureCalls.Add(rank); return "R" + rank.ToString(CultureInfo.InvariantCulture); },
                Describe = type => { DescribedTypes.Add(type); return "D" + type.ToString(CultureInfo.InvariantCulture); },
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

    /// <summary>제품과 같은 전역 읽기로 생성한 여덟 참가자 창이 실제 fortGump TYPE을 쓴다.</summary>
    [Fact]
    public void ExternalReadCreatesPlayersWithActualRuntimeType()
    {
        using var scene = new Scene();
        var devices = new HostNsInfoDevices
        {
            Read = address => HostNsInfoExternalRead.TryRead(address, scene.Menu.BackgroundType, scene.Screen.Colors(), out uint value)
                ? value : scene.Devices.Read(address),
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText, Text = scene.Devices.Text,
            FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
        };
        using var windows = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, devices);
        uint token = windows.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        Assert.Equal((uint)new TypeCatalog(scene.Startup.Files).Find("fortGump")!.RuntimeIndex, scene.Menu.BackgroundType);
        // 실제 생성된 모든 참가자 버튼이 같은 런타임 TYPE 포인터를 보유한다.
        foreach (uint player in root.Players)
            Assert.Equal(0x100000u + scene.Menu.BackgroundType, Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(player)).ButtonType);
    }

    /// <summary>두 참가자 색상 배열과 기본색 주소의 정확한 항목 및 경계를 확인한다.</summary>
    [Fact]
    public void ExternalReadUsesEachNativePaletteAddress()
    {
        var colors = new PaletteColorTable();
        // 값이 중복되지 않는 표로 잘못된 항목 선택도 식별한다.
        for (int i = 0; i < colors.Basic.Length; i++) colors.Basic[i] = (uint)(100 + i);
        // 원본 팔레트의 전체 호출 순서를 그대로 구성한다.
        for (int i = 0; i < 58; i++) colors.Named.Add(new NamedPaletteColor(0, 0, 0, (uint)(200 + i)));
        Assert.True(HostNsInfoExternalRead.TryRead(0x5411c8, 92, colors, out uint type)); Assert.Equal(92u, type);
        foreach (var pair in new (uint Address, uint Value)[] { (0x5bec3c, 108), (0x5bec2c, 107), (0x5b5e58, 101), (0x59afd8, 105), (0x5c78d0, 236) })
        {
            Assert.True(HostNsInfoExternalRead.TryRead(pair.Address, 92, colors, out uint value)); Assert.Equal(pair.Value, value);
        }
        // 여덟 슬롯이 각각 원본의 두 색상 배열에 대응한다.
        for (uint i = 0; i < 8; i++)
        {
            Assert.True(HostNsInfoExternalRead.TryRead(0x5accfc + i * 4, 92, colors, out uint background)); Assert.Equal(242u + i, background);
            Assert.True(HostNsInfoExternalRead.TryRead(0x5c78b0 + i * 4, 92, colors, out uint name)); Assert.Equal(250u + i, name);
        }
        foreach (uint address in new uint[] { 0x5accf8, 0x5accfd, 0x5acd1c, 0x5c78af, 0x5c78b1, 0x5c78d4 })
            Assert.False(HostNsInfoExternalRead.TryRead(address, 92, colors, out _));
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

    /// <summary>같은 실제 CS 참가자 기록을 준비한다.</summary>
    private static void WriteParticipant(ClientChallengeGlobals challenge, uint index, uint battle, uint flags, string name, uint level = 0, uint sid = 0)
    {
        Span<byte> bytes = challenge.Participant(index).Span;
        BitConverter.GetBytes(flags).CopyTo(bytes.Slice(0x64, 4));
        bytes[0x58] = (byte)level;
        BitConverter.GetBytes(battle).CopyTo(bytes.Slice(0xa4, 4));
        BitConverter.GetBytes(sid).CopyTo(bytes.Slice(0xb8, 4));
        byte[] nameBytes = Encoding.Latin1.GetBytes(name + "\0");
        nameBytes.CopyTo(bytes.Slice(0x26, nameBytes.Length));
    }

    /// <summary>메타 후속까지 연결된 별도 호스트를 같은 트리/문자에 얹는다.</summary>
    private sealed class MetaHost : IDisposable
    {
        /// <summary>같은 실제 트리/문자 소유자다.</summary>
        public readonly Scene Scene = new();
        /// <summary>메타 후속이 연결된 검사할 실제 장기 공장이다.</summary>
        public readonly HostNsInfoWindows Ns;
        /// <summary>실제 메타 후속 호출이다.</summary>
        public readonly List<(HudNsInfoChild Kind, uint Token)> MetaCalls = [];

        /// <summary>같은 트리/문자에 메타 후속 장치를 더한 호스트를 조립한다.</summary>
        public MetaHost()
        {
            var devices = new HostNsInfoDevices
            {
                Read = Scene.Devices.Read, Translate = Scene.Devices.Translate, StoreText = Scene.Devices.StoreText,
                Text = Scene.Devices.Text, FormatZone = Scene.Devices.FormatZone, Changed = Scene.Devices.Changed,
                ObjectOwner = Scene.Devices.ObjectOwner, TechWeight = Scene.Devices.TechWeight,
                MetaChildLayout = (kind, token) => MetaCalls.Add((kind, token)),
            };
            Ns = new HostNsInfoWindows(Scene.Menu.Tree, Scene.Text, Scene.Globals, Scene.Game, Scene.Storage, devices);
        }

        /// <summary>별도 호스트와 기존 HTML 소유 자식을 반납한다.</summary>
        public void Dispose() { Ns.Dispose(); Scene.Dispose(); }
    }

    /// <summary>실전투 없이 명부·제목·도움말을 실제 저장소에 배치한다. 0.921에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaRootNoBattleRendersRosterAndHelp()
    {
        using var host = new MetaHost(); var scene = host.Scene;
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(challenge, owner, battle: 0, flags: 1, name: "me", sid: 7);
        WriteParticipant(challenge, 5, battle: 0, flags: 1, name: "foe", level: 3, sid: 8);
        scene.Owners[7] = owner;
        scene.Owners[8] = owner + 1;
        uint token = host.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        scene.Storage.OwnerMode.Challenge = true; scene.Game.Player.PlayerKnown = 1;
        root.BaseWidth = 300; root.BaseHeight = 200;
        host.Ns.LayoutMetaRoot(token, challenge);
        var title = Assert.IsType<ScrollingTextGumpNode>(scene.Menu.Tree.Node(root.Title));
        Assert.Equal(300, title.BaseWidth);
        Assert.Equal(root.Background, scene.Menu.Tree.Node(root.Roster)!.Parent);
        Assert.Equal(root.Background, title.Parent);
        Assert.NotEqual(0u, scene.Menu.Tree.Node(token)!.TextPointer);
        Assert.NotEqual(0u, scene.Menu.Tree.Node(token)!.AlternateTextPointer);
        Assert.Equal(0u, scene.Menu.Tree.Node(root.Battle)!.Flags & 1u);
        // 두 활성행의 자기 쌍교환 동률 2건×2회와 행 랭크 2건이다.
        Assert.Equal(6, scene.TechWeightCalls);
        Assert.Contains(7u, scene.OwnerCalls);
    }

    /// <summary>전투 경로는 마스터 0번 패널을 선택해 보인다. 0.921에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaRootBattlePathSelectsMaster()
    {
        using var host = new MetaHost(); var scene = host.Scene;
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(challenge, owner, battle: 2, flags: 5, name: "me", sid: 7);
        WriteParticipant(challenge, 3, battle: 2, flags: 5, name: "boss", sid: 9);
        scene.Owners[7] = owner;
        scene.Owners[9] = owner;
        var battle = challenge.Battle(2).Span;
        battle[0] = 1;
        BitConverter.GetBytes(3u).CopyTo(battle.Slice(0x38, 4));
        uint token = host.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        scene.Storage.OwnerMode.Challenge = true; scene.Game.Player.PlayerKnown = 1;
        root.BaseWidth = 300; root.BaseHeight = 200;
        host.Ns.LayoutMetaRoot(token, challenge);
        var first = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(root.Players[0]));
        Assert.NotEqual(0u, first.Flags & 1u);
        Assert.Equal(root.Background, first.Parent);
        Assert.Equal(3u, first.Player);
        var info = Assert.IsType<HudBattleInfoNode>(scene.Menu.Tree.Node(root.Battle));
        Assert.NotEqual(0u, info.Flags & 1u);
        Assert.Equal(2u, info.Battle);
        Assert.Equal(4u, info.Changed);
        Assert.NotEqual(0u, scene.Menu.Tree.Node(token)!.TextPointer);
    }

    /// <summary>빈 명부도 제목·도움말 부착까지 수행한다. 0.921에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaRootEmptyRosterStillAttaches()
    {
        using var host = new MetaHost(); var scene = host.Scene;
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(challenge, owner, battle: 0, flags: 0, name: "me", sid: 7);
        uint token = host.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        scene.Storage.OwnerMode.Challenge = true; scene.Game.Player.PlayerKnown = 1;
        root.BaseWidth = 300; root.BaseHeight = 200;
        host.Ns.LayoutMetaRoot(token, challenge);
        Assert.Equal(root.Background, scene.Menu.Tree.Node(root.Title)!.Parent);
        Assert.NotEqual(0u, scene.Menu.Tree.Node(token)!.AlternateTextPointer);
        Assert.Equal(0, scene.TechWeightCalls);
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.921에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaRootMissingDevicesFailExplicitly()
    {
        using var scene = new Scene();
        uint token = scene.Ns.Create();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        Assert.Throws<ArgumentNullException>(() => scene.Ns.LayoutMetaRoot(token, null!));
        var bare = new HostNsInfoDevices
        {
            Read = scene.Devices.Read, Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, bare);
        uint second = other.Create();
        var missing = new ClientChallengeGlobals(); missing.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(missing, owner, battle: 0, flags: 1, name: "me", sid: 7);
        scene.Storage.OwnerMode.Challenge = true; scene.Game.Player.PlayerKnown = 1;
        Assert.Throws<InvalidOperationException>(() => other.LayoutMetaRoot(second, missing));
        other.Dispose();
        Assert.Throws<ObjectDisposedException>(() => other.LayoutMetaRoot(second, missing));
    }

    /// <summary>빈 슬롯은 이름 없이 버튼을 끄고 기본 문맥 개행을 둔다. 0.922에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaPlayerEmptySlotDisablesButtons()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint token = scene.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        var panel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(root.Players[0]));
        Assert.Equal(0u, panel.Player);
        scene.Ns.LayoutMetaPlayer(root.Players[0], challenge);
        var button = Assert.IsType<HudPlayerButtonNode>(scene.Menu.Tree.Node(panel.Button));
        Assert.NotEqual(0u, button.ButtonFlags & 1u);
        var start = Assert.IsAssignableFrom<ButtonGumpNode>(scene.Menu.Tree.Node(panel.Start));
        Assert.NotEqual(0u, start.ButtonFlags & 1u);
        // 부모 0 부착은 기존 부모(플레이어 패널)를 유지한다.
        Assert.Equal(root.Players[0], scene.Menu.Tree.Node(panel.Text)!.Parent);
    }

    /// <summary>마스터는 모두 준비되면 시작 버튼을 켠다. 0.922에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaPlayerMasterEnablesStartWhenReady()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(challenge, owner, battle: 2, flags: 1 | 4 | 2 | 0x2000, name: "me", sid: 7);
        WriteParticipant(challenge, 5, battle: 2, flags: 1 | 2 | 0x2000, name: "ok", sid: 8);
        var battle = challenge.Battle(2).Span;
        BitConverter.GetBytes(owner).CopyTo(battle.Slice(0x38, 4));
        BitConverter.GetBytes(5u).CopyTo(battle.Slice(0x3c, 4));
        scene.Owners[7] = owner;
        scene.Owners[8] = owner;
        uint token = scene.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        var panel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(root.Players[0]));
        panel.Player = owner;
        scene.Ns.LayoutMetaPlayer(root.Players[0], challenge);
        var start = Assert.IsAssignableFrom<ButtonGumpNode>(scene.Menu.Tree.Node(panel.Start));
        Assert.NotEqual(0u, start.Flags & 1u);
        Assert.Equal(0u, start.ButtonFlags & 1u);
        var text = Assert.IsType<ScrollingTextGumpNode>(scene.Menu.Tree.Node(panel.Text));
        Assert.NotEqual(0u, text.TextFlags & 0x10u);
    }

    /// <summary>마스터는 미준비자가 있으면 시작 버튼을 끈다. 0.922에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaPlayerMasterDisablesStartWhenUnready()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(challenge, owner, battle: 2, flags: 1 | 4 | 2 | 0x2000, name: "me", sid: 7);
        WriteParticipant(challenge, 5, battle: 2, flags: 1 | 2, name: "notready", sid: 8);
        var battle = challenge.Battle(2).Span;
        BitConverter.GetBytes(owner).CopyTo(battle.Slice(0x38, 4));
        BitConverter.GetBytes(5u).CopyTo(battle.Slice(0x3c, 4));
        scene.Owners[7] = owner;
        scene.Owners[8] = owner;
        uint token = scene.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        var panel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(root.Players[0]));
        panel.Player = owner;
        scene.Ns.LayoutMetaPlayer(root.Players[0], challenge);
        var start = Assert.IsAssignableFrom<ButtonGumpNode>(scene.Menu.Tree.Node(panel.Start));
        Assert.NotEqual(0u, start.ButtonFlags & 1u);
        Assert.Equal(0u, start.Flags & 1u);
    }

    /// <summary>비마스터는 시작 버튼 없이 랭크 그림을 한 번 요청한다. 0.922에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaPlayerNonMasterShowsMarkAndRank()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(challenge, owner, battle: 2, flags: 1 | 4 | 2 | 0x2000, name: "boss", sid: 7);
        WriteParticipant(challenge, 5, battle: 2, flags: 1 | 2, name: "me", sid: 8);
        var battle = challenge.Battle(2).Span;
        BitConverter.GetBytes(5u).CopyTo(battle.Slice(0x38, 4));
        scene.Owners[7] = owner;
        scene.Owners[8] = owner;
        uint token = scene.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        var panel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(root.Players[0]));
        panel.Player = 5;
        scene.Ns.LayoutMetaPlayer(root.Players[0], challenge);
        var start = Assert.IsAssignableFrom<ButtonGumpNode>(scene.Menu.Tree.Node(panel.Start));
        Assert.Equal(0u, start.Flags & 1u);
        Assert.NotEqual(0u, start.ButtonFlags & 1u);
        var button = Assert.IsType<HudPlayerButtonNode>(scene.Menu.Tree.Node(panel.Button));
        Assert.Equal(1u, button.Value);
        Assert.Equal(0u, button.Entry);
        Assert.Equal([0], scene.RankPictureCalls);
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.922에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaPlayerMissingDevicesFailExplicitly()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint token = scene.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        var panel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(root.Players[0]));
        panel.Player = 0;
        var bare = new HostNsInfoDevices
        {
            Read = scene.Devices.Read, Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            ObjectOwner = scene.Devices.ObjectOwner, TechWeight = scene.Devices.TechWeight,
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, bare);
        uint second = other.Create();
        var otherPanel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(root.Players[0]));
        Assert.Equal(0u, otherPanel.Player);
        WriteParticipant(challenge, 5, battle: 2, flags: 1 | 2, name: "x", sid: 8);
        otherPanel.Player = 5;
        Assert.Throws<InvalidOperationException>(() => other.LayoutMetaPlayer(root.Players[0], challenge));
        other.Dispose();
        Assert.Throws<ArgumentNullException>(() => scene.Ns.LayoutMetaPlayer(root.Players[0], null!));
        Assert.Throws<ObjectDisposedException>(() => other.LayoutMetaPlayer(root.Players[0], challenge));
    }

    /// <summary>빈 전투는 내용을 생략하고 공통 스크롤/자식 배치를 수행한다. 0.923에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaBattleEmptySkipsContentButAttaches()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint token = scene.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        var panel = Assert.IsType<HudBattleInfoNode>(scene.Menu.Tree.Node(root.Battle));
        panel.BaseWidth = 300;
        panel.BaseHeight = 60;
        panel.Battle = 0;
        var title = Assert.IsType<ScrollingTextGumpNode>(scene.Menu.Tree.Node(panel.Title));
        title.TextFlags |= 8u;
        scene.Ns.LayoutMetaBattle(root.Battle, challenge);
        Assert.Equal(root.Battle, scene.Menu.Tree.Node(panel.Title)!.Parent);
        Assert.Equal(root.Battle, scene.Menu.Tree.Node(panel.Summary)!.Parent);
        Assert.Equal(root.Battle, scene.Menu.Tree.Node(panel.Body)!.Parent);
        Assert.Equal(1u, panel.Bytes[0]);
        Assert.Equal(0u, title.TextFlags & 8u);
        Assert.Equal(0, title.ScrollRequest);
    }

    /// <summary>활성 전투는 게임 종류·옵션 행과 제외 목록을 쓴다. 0.923에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaBattleActiveWritesRowsAndExcluded()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        var battle = challenge.Battle(2).Span;
        battle[0] = 1;
        battle[0x16] = 1;
        battle[0x17] = 5;
        uint token = scene.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        var panel = Assert.IsType<HudBattleInfoNode>(scene.Menu.Tree.Node(root.Battle));
        panel.BaseWidth = 300;
        panel.BaseHeight = 60;
        panel.Battle = 2;
        panel.Mode = 2;
        panel.Settings[1, 1] = 1;
        scene.Ns.LayoutMetaBattle(root.Battle, challenge);
        Assert.Equal([5u], scene.DescribedTypes);
        Assert.Equal(1u, panel.Bytes[0]);
        var title = Assert.IsType<ScrollingTextGumpNode>(scene.Menu.Tree.Node(panel.Title));
        Assert.NotEqual(0u, title.TextFlags & 8u);
        Assert.Equal(root.Battle, scene.Menu.Tree.Node(panel.Title)!.Parent);
    }

    /// <summary>일반 모드는 바뀐 활성행만 쓰고 추가 행을 둔다. 0.923에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaBattleNormalModeWritesChangedRows()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        var battle = challenge.Battle(2).Span;
        battle[0] = 1;
        uint token = scene.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        var panel = Assert.IsType<HudBattleInfoNode>(scene.Menu.Tree.Node(root.Battle));
        panel.BaseWidth = 300;
        panel.BaseHeight = 60;
        panel.Battle = 2;
        panel.Mode = 0;
        scene.Ns.LayoutMetaBattle(root.Battle, challenge);
        Assert.Equal(1u, panel.Bytes[0]);
        Assert.Contains("Click To Add", scene.Translations);
        Assert.Equal(root.Battle, scene.Menu.Tree.Node(panel.Body)!.Parent);
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.923에서 추가됐다.</summary>
    [Fact]
    public void LayoutMetaBattleMissingDevicesFailExplicitly()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint token = scene.Ns.Create(); var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(token));
        var panel = Assert.IsType<HudBattleInfoNode>(scene.Menu.Tree.Node(root.Battle));
        panel.Battle = 2;
        var battle = challenge.Battle(2).Span;
        battle[0] = 1;
        battle[0x16] = 1;
        battle[0x17] = 5;
        var bare = new HostNsInfoDevices
        {
            Read = scene.Devices.Read, Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, bare);
        uint second = other.Create();
        var otherRoot = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(second));
        var otherPanel = Assert.IsType<HudBattleInfoNode>(scene.Menu.Tree.Node(otherRoot.Battle));
        otherPanel.Battle = 2;
        Assert.Throws<InvalidOperationException>(() => other.LayoutMetaBattle(otherRoot.Battle, challenge));
        other.Dispose();
        Assert.Throws<ArgumentNullException>(() => scene.Ns.LayoutMetaBattle(root.Battle, null!));
        Assert.Throws<ObjectDisposedException>(() => other.LayoutMetaBattle(otherRoot.Battle, challenge));
    }

    /// <summary>활성 정렬행은 실제 참가자 메뉴를 열고 1을 반환한다. 0.925에서 추가됐다.</summary>
    [Fact]
    public void HandleRootInputActiveRosterOpensMenu()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        WriteParticipant(challenge, 9, battle: 0, flags: 1, name: "me", sid: 7);
        scene.Globals.RosterOrder[5] = 9;
        var menus = new List<(uint Index, ScreenPoint Point)>();
        var based = new List<(uint Event, uint First, uint Second)>();
        // 같은 실제 명령 주소를 시험 명령 코드로 덮어쓴다.
        var wired = new HostNsInfoDevices
        {
            Read = address => address switch { 0x542790 => 0x100u, 0x54278c => 0x200u, 0x5429dc => 0x300u, _ => scene.Devices.Read(address) },
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText, Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            MousePoint = () => new ScreenPoint(11, 22),
            ParticipantMenu = (index, point) => menus.Add((index, point)),
            CenterOnSid = _ => { },
            StatusSend = _ => { },
            KickoffSound = () => { },
            BaseInput = (@event, first, second) => { based.Add((@event, first, second)); return 7u; },
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint result = other.HandleRootInput(0x100, 1, 5, challenge);
        Assert.Equal(1u, result);
        Assert.Equal([(9u, new ScreenPoint(11, 22))], menus);
        Assert.Empty(based);
        other.Dispose();
    }

    /// <summary>활성 중심행은 실제 참가자 SID를 카메라에 전달하고 1을 반환한다. 0.925에서 추가됐다.</summary>
    [Fact]
    public void HandleRootInputActiveCenterSendsSid()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        WriteParticipant(challenge, 4, battle: 0, flags: 1, name: "me", sid: 0x1234);
        scene.Globals.RosterOrder[6] = 4;
        var centers = new List<uint>();
        var based = new List<(uint Event, uint First, uint Second)>();
        var wired = new HostNsInfoDevices
        {
            Read = address => address switch { 0x542790 => 0x100u, 0x54278c => 0x200u, 0x5429dc => 0x300u, _ => scene.Devices.Read(address) },
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText, Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            MousePoint = () => new ScreenPoint(11, 22),
            ParticipantMenu = (_, _) => { },
            CenterOnSid = sid => centers.Add(sid),
            StatusSend = _ => { },
            KickoffSound = () => { },
            BaseInput = (@event, first, second) => { based.Add((@event, first, second)); return 7u; },
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint result = other.HandleRootInput(0x200, 0, 6, challenge);
        Assert.Equal(1u, result);
        Assert.Equal([0x1234u], centers);
        Assert.Empty(based);
        other.Dispose();
    }

    /// <summary>직접 추방은 실제 기록 4번과 8을 패킷에 쓰고 추방음을 재생한다. 0.925에서 추가됐다.</summary>
    [Fact]
    public void HandleRootInputDirectKickFillsPacketAndPlaysSound()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        WriteParticipant(challenge, 8, battle: 0, flags: 1, name: "out", sid: 9);
        BitConverter.GetBytes(0xAABBCCDDu).CopyTo(challenge.Participant(8).Span.Slice(4, 4));
        var packets = new List<byte[]>();
        int sounds = 0;
        var based = new List<(uint Event, uint First, uint Second)>();
        var wired = new HostNsInfoDevices
        {
            Read = address => address switch { 0x542790 => 0x100u, 0x54278c => 0x200u, 0x5429dc => 0x300u, _ => scene.Devices.Read(address) },
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText, Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            MousePoint = () => new ScreenPoint(11, 22),
            ParticipantMenu = (_, _) => { },
            CenterOnSid = _ => { },
            StatusSend = fill => { byte[] packet = new byte[12]; fill(packet); packets.Add(packet); },
            KickoffSound = () => sounds++,
            BaseInput = (@event, first, second) => { based.Add((@event, first, second)); return 7u; },
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint result = other.HandleRootInput(0x300, 0, 8, challenge);
        Assert.Equal(1u, result);
        Assert.Single(packets);
        Assert.Equal(0xAABBCCDDu, BitConverter.ToUInt32(packets[0], 4));
        Assert.Equal(8u, BitConverter.ToUInt32(packets[0], 8));
        Assert.Equal(1, sounds);
        Assert.Empty(based);
        other.Dispose();
    }

    /// <summary>비활성과 둘째 0은 실제 기본 처리에 원래 인자를 전달한다. 0.925에서 추가됐다.</summary>
    [Fact]
    public void HandleRootInputInactiveFallsThroughToBase()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        WriteParticipant(challenge, 9, battle: 0, flags: 0, name: "off", sid: 7);
        scene.Globals.RosterOrder[5] = 9;
        var menus = new List<(uint Index, ScreenPoint Point)>();
        var based = new List<(uint Event, uint First, uint Second)>();
        var wired = new HostNsInfoDevices
        {
            Read = address => address switch { 0x542790 => 0x100u, 0x54278c => 0x200u, 0x5429dc => 0x300u, _ => scene.Devices.Read(address) },
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText, Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            MousePoint = () => new ScreenPoint(11, 22),
            ParticipantMenu = (index, point) => menus.Add((index, point)),
            CenterOnSid = _ => { },
            StatusSend = _ => { },
            KickoffSound = () => { },
            BaseInput = (@event, first, second) => { based.Add((@event, first, second)); return 7u; },
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint first = other.HandleRootInput(0x100, 3, 5, challenge);
        Assert.Equal(7u, first);
        Assert.Empty(menus);
        uint second = other.HandleRootInput(0x100, 9, 0, challenge);
        Assert.Equal(7u, second);
        Assert.Equal([(0x100u, 3u, 5u), (0x100u, 9u, 0u)], based);
        other.Dispose();
    }

    /// <summary>미연결 루트 입력 장치는 일반 성공으로 꾸미지 않는다. 0.925에서 추가됐다.</summary>
    [Fact]
    public void HandleRootInputMissingDevicesFailExplicitly()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        WriteParticipant(challenge, 9, battle: 0, flags: 1, name: "me", sid: 7);
        scene.Globals.RosterOrder[5] = 9;
        var bare = new HostNsInfoDevices
        {
            Read = address => address switch { 0x542790 => 0x100u, 0x54278c => 0x200u, 0x5429dc => 0x300u, _ => scene.Devices.Read(address) },
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, bare);
        Assert.Throws<InvalidOperationException>(() => other.HandleRootInput(0x100, 1, 5, challenge));
        Assert.Throws<ArgumentNullException>(() => other.HandleRootInput(0x100, 1, 5, null!));
        Assert.Throws<ArgumentOutOfRangeException>(() => other.HandleRootInput(0x100, 1, 99, challenge));
        other.Dispose();
        Assert.Throws<ObjectDisposedException>(() => other.HandleRootInput(0x100, 1, 5, challenge));
    }

    /// <summary>같은 실제 플레이어 입력 결선 장치를 기록하는 검사 장치다. 0.926에서 추가됐다.</summary>
    private sealed class PlayerInputWiring
    {
        /// <summary>실제 송신한 상태 패킷이다.</summary>
        public readonly List<byte[]> Packets = [];
        /// <summary>실제 송신한 옵션이다.</summary>
        public readonly List<(uint Battle, byte[] Options, uint Kind)> DataCalls = [];
        /// <summary>실제 쓴 보조 값이다.</summary>
        public readonly List<uint> Auxiliaries = [];
        /// <summary>실제 메뉴 제목이다.</summary>
        public readonly List<string> Titles = [];
        /// <summary>실제 메뉴 항목이다.</summary>
        public readonly List<(string Text, uint Command, uint Parameter)> Items = [];
        /// <summary>실제 메뉴 생성/부착 횟수다.</summary>
        public int Menus;
        /// <summary>같은 실제 외부 장치 묶음을 조립한다.</summary>
        public HostNsInfoDevices Build(Scene scene) => new()
        {
            Read = address => address switch { 0x5427a8 => 0x10u, 0x542790 => 0x20u, 0x54278c => 0x30u, 0x5429dc => 0x40u, _ => scene.Devices.Read(address) },
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            StatusSend = fill => { byte[] packet = new byte[12]; fill(packet); Packets.Add(packet); },
            PlayerSubscriber = () => 0x77u,
            PlayerAuxiliary = value => Auxiliaries.Add(value),
            PlayerData = (battle, options, kind) => DataCalls.Add((battle, options.ToArray(), kind)),
            PlayerMenuBegin = () => Menus++,
            PlayerMenuAttach = () => Menus++,
            PlayerMenuTitle = text => Titles.Add(text),
            PlayerMenuItem = (text, command, parameter) => Items.Add((text, command, parameter)),
        };
    }

    /// <summary>준비 버튼은 실제 버튼값으로 CS 플래그를 쓰고 상태 패킷을 송신한다. 0.926에서 추가됐다.</summary>
    [Fact]
    public void HandlePlayerInputReadyTogglesFlagAndSends()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(challenge, owner, battle: 2, flags: 0, name: "me", sid: 7);
        var wiring = new PlayerInputWiring();
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wiring.Build(scene));
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Players[0];
        var panel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(panelToken));
        panel.Player = owner;
        var button = Assert.IsType<HudPlayerButtonNode>(scene.Menu.Tree.Node(panel.Button));
        button.Value = 1;
        uint serial = scene.Menu.Tree.Node(panel.Button)!.Serial;
        uint result = other.HandlePlayerInput(panelToken, 0x10, serial, 0, challenge);
        Assert.Equal(1u, result);
        Assert.Equal(2u, BitConverter.ToUInt32(challenge.Participant(owner).Span.Slice(0x64, 4)));
        Assert.Single(wiring.Packets);
        Assert.Equal(0x77u, BitConverter.ToUInt32(wiring.Packets[0], 4));
        Assert.Equal(2u, BitConverter.ToUInt32(wiring.Packets[0], 8));
        other.Dispose();
    }

    /// <summary>시작 버튼은 최초 1회만 송신하고 예약을 남긴다. 0.926에서 추가됐다.</summary>
    [Fact]
    public void HandlePlayerInputStartReservesOnce()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(challenge, owner, battle: 2, flags: 1, name: "me", sid: 7);
        var wiring = new PlayerInputWiring();
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wiring.Build(scene));
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Players[0];
        var panel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(panelToken));
        panel.Player = owner;
        uint serial = scene.Menu.Tree.Node(panel.Start)!.Serial;
        uint first = other.HandlePlayerInput(panelToken, 0x10, serial, 0, challenge);
        Assert.Equal(1u, first);
        Assert.Equal(1u, panel.Reserved);
        Assert.Single(wiring.Packets);
        Assert.Equal(0x12u, BitConverter.ToUInt32(wiring.Packets[0], 8));
        uint second = other.HandlePlayerInput(panelToken, 0x10, serial, 0, challenge);
        Assert.Equal(1u, second);
        Assert.Single(wiring.Packets);
        other.Dispose();
    }

    /// <summary>좌행 슬롯 변경은 실제 전투 옵션과 87BYTE 송신을 함께 수행한다. 0.926에서 추가됐다.</summary>
    [Fact]
    public void HandlePlayerInputSlotChangeRankDownSendsData()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(challenge, owner, battle: 2, flags: 1, name: "me", sid: 7);
        var wiring = new PlayerInputWiring();
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wiring.Build(scene));
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Players[0];
        var panel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(panelToken));
        panel.Player = owner;
        uint serial = scene.Menu.Tree.Node(panel.Text)!.Serial;
        uint result = other.HandlePlayerInput(panelToken, 0x20, serial, owner + 300u, challenge);
        Assert.Equal(1u, result);
        Assert.Single(wiring.Packets);
        Assert.Single(wiring.DataCalls);
        Assert.Equal(2u, wiring.DataCalls[0].Battle);
        Assert.Equal(0x57u, wiring.DataCalls[0].Kind);
        Assert.Empty(wiring.Auxiliaries);
        other.Dispose();
    }

    /// <summary>추방 확인은 실제 이름 메뉴와 예/아니요 항목을 만든다. 0.926에서 추가됐다.</summary>
    [Fact]
    public void HandlePlayerInputKickoffOpensMenu()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(challenge, owner, battle: 2, flags: 1, name: "bob", sid: 7);
        var wiring = new PlayerInputWiring();
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wiring.Build(scene));
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Players[0];
        var panel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(panelToken));
        panel.Player = owner;
        uint serial = scene.Menu.Tree.Node(panel.Text)!.Serial;
        uint result = other.HandlePlayerInput(panelToken, 0x20, serial, owner + 400u, challenge);
        Assert.Equal(1u, result);
        Assert.Equal(2, wiring.Menus);
        Assert.Single(wiring.Titles);
        Assert.Contains("bob", wiring.Titles[0], StringComparison.Ordinal);
        Assert.Equal(2, wiring.Items.Count);
        Assert.Equal(0x40u, wiring.Items[0].Command);
        Assert.Equal(owner, wiring.Items[0].Parameter);
        Assert.Equal(0u, wiring.Items[1].Parameter);
        other.Dispose();
    }

    /// <summary>미연결 플레이어 입력 장치는 일반 성공으로 꾸미지 않는다. 0.926에서 추가됐다.</summary>
    [Fact]
    public void HandlePlayerInputMissingDevicesFailExplicitly()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(challenge, owner, battle: 2, flags: 1, name: "me", sid: 7);
        var bare = new HostNsInfoDevices
        {
            Read = address => address switch { 0x5427a8 => 0x10u, 0x542790 => 0x20u, 0x54278c => 0x30u, 0x5429dc => 0x40u, _ => scene.Devices.Read(address) },
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, bare);
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Players[0];
        var panel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(panelToken));
        panel.Player = owner;
        var button = Assert.IsType<HudPlayerButtonNode>(scene.Menu.Tree.Node(panel.Button));
        button.Value = 1;
        uint serial = scene.Menu.Tree.Node(panel.Button)!.Serial;
        Assert.Throws<InvalidOperationException>(() => other.HandlePlayerInput(panelToken, 0x10, serial, 0, challenge));
        Assert.Throws<ArgumentNullException>(() => other.HandlePlayerInput(panelToken, 0x10, serial, 0, null!));
        Assert.Throws<InvalidOperationException>(() => other.HandlePlayerInput(rootToken, 0x10, serial, 0, challenge));
        other.Dispose();
        Assert.Throws<ObjectDisposedException>(() => other.HandlePlayerInput(panelToken, 0x10, serial, 0, challenge));
    }

    /// <summary>같은 실제 배틀 입력 결선 장치를 기록하는 검사 장치다. 0.927에서 추가됐다.</summary>
    private sealed class BattleInputWiring
    {
        /// <summary>실제 송신한 옵션이다.</summary>
        public readonly List<(uint Battle, byte[] Options, uint Kind)> DataCalls = [];
        /// <summary>실제 정렬 메뉴 호출이다.</summary>
        public int SortCalls;
        /// <summary>실제 전체 옵션 메뉴 호출이다.</summary>
        public int OptionsCalls;
        /// <summary>실제 제외 메뉴 호출이다.</summary>
        public int ExcludeCalls;
        /// <summary>실제 개별 옵션 메뉴 호출이다.</summary>
        public readonly List<int> OptionCalls = [];
        /// <summary>실제 제거 확인 호출이다.</summary>
        public readonly List<uint> RemoveCalls = [];
        /// <summary>실제 재생한 음향 횟수다.</summary>
        public int GuySounds;
        /// <summary>같은 실제 외부 장치 묶음을 조립한다.</summary>
        public HostNsInfoDevices Build(Scene scene) => new()
        {
            Read = address => address switch { 0x54278c => 0x10u, 0x542b80 => 0x20u, 0x542b7c => 0x30u, 0x5429e0 => 0x40u, 0x5429e4 => 0x50u, _ => scene.Devices.Read(address) },
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            BattleGuySound = () => GuySounds++,
            BattleSortMenu = _ => SortCalls++,
            BattleOptionsMenu = (_, _) => OptionsCalls++,
            BattleExcludeMenu = (_, _) => ExcludeCalls++,
            BattleOptionMenu = (_, _, index) => OptionCalls.Add(index),
            BattleRemoveMenu = type => RemoveCalls.Add(type),
            BattleData = (battle, options, kind) => DataCalls.Add((battle, options.ToArray(), kind)),
        };
    }

    /// <summary>모드2 순환은 실제 옵션과 87BYTE 송신을 함께 수행한다. 0.927에서 추가됐다.</summary>
    [Fact]
    public void HandleBattleInputMode2CycleSendsData()
    {
        using var scene = new Scene();
        scene.Storage.OwnerMode.Challenge = true;
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        challenge.Battle(2).Span[0] = 1;
        var wiring = new BattleInputWiring();
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wiring.Build(scene));
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Battle;
        var panel = Assert.IsType<HudBattleInfoNode>(scene.Menu.Tree.Node(panelToken));
        panel.Battle = 2;
        panel.Changed = 1;
        panel.Mode = 2;
        uint serial = scene.Menu.Tree.Node(panel.Title)!.Serial;
        uint result = other.HandleBattleInput(panelToken, 0x10, serial, 5, challenge);
        Assert.Equal(1u, result);
        Assert.Single(wiring.DataCalls);
        Assert.Equal(2u, wiring.DataCalls[0].Battle);
        Assert.Equal(0x57u, wiring.DataCalls[0].Kind);
        uint parent = scene.Menu.Tree.Node(panelToken)!.Parent;
        Assert.NotEqual(0u, parent);
        Assert.Equal(panelToken, scene.Menu.Tree.Node(parent)!.Child);
        other.Dispose();
    }

    /// <summary>정렬 메뉴 선택은 변경 없이 메뉴만 열고 1을 반환한다. 0.927에서 추가됐다.</summary>
    [Fact]
    public void HandleBattleInputSortMenuOpensWithoutChange()
    {
        using var scene = new Scene();
        scene.Storage.OwnerMode.Challenge = true;
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        challenge.Battle(2).Span[0] = 1;
        var wiring = new BattleInputWiring();
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wiring.Build(scene));
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Battle;
        var panel = Assert.IsType<HudBattleInfoNode>(scene.Menu.Tree.Node(panelToken));
        panel.Battle = 2;
        uint serial = scene.Menu.Tree.Node(panel.Title)!.Serial;
        uint result = other.HandleBattleInput(panelToken, 0x10, serial, 24, challenge);
        Assert.Equal(1u, result);
        Assert.Equal(1, wiring.SortCalls);
        Assert.Empty(wiring.DataCalls);
        other.Dispose();
    }

    /// <summary>제외 추가는 실제 목록과 87BYTE 송신을 함께 수행한다. 0.927에서 추가됐다.</summary>
    [Fact]
    public void HandleBattleInputAddExcludedTypeSendsData()
    {
        using var scene = new Scene();
        scene.Storage.OwnerMode.Challenge = true;
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        challenge.Battle(2).Span[0] = 1;
        var wiring = new BattleInputWiring();
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wiring.Build(scene));
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Battle;
        var panel = Assert.IsType<HudBattleInfoNode>(scene.Menu.Tree.Node(panelToken));
        panel.Battle = 2;
        panel.Changed = 1;
        uint result = other.HandleBattleInput(panelToken, 0x40, 0, 5, challenge);
        Assert.Equal(1u, result);
        Assert.Single(wiring.DataCalls);
        Assert.Equal(2u, wiring.DataCalls[0].Battle);
        other.Dispose();
    }

    /// <summary>비활성 전투는 실제 메뉴·송신 없이 0을 반환한다. 0.927에서 추가됐다.</summary>
    [Fact]
    public void HandleBattleInputInactiveReturnsZero()
    {
        using var scene = new Scene();
        scene.Storage.OwnerMode.Challenge = true;
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        var wiring = new BattleInputWiring();
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wiring.Build(scene));
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Battle;
        var panel = Assert.IsType<HudBattleInfoNode>(scene.Menu.Tree.Node(panelToken));
        panel.Battle = 2;
        panel.Changed = 1;
        uint serial = scene.Menu.Tree.Node(panel.Title)!.Serial;
        uint result = other.HandleBattleInput(panelToken, 0x10, serial, 5, challenge);
        Assert.Equal(0u, result);
        Assert.Empty(wiring.DataCalls);
        Assert.Equal(0, wiring.SortCalls);
        uint fresh = other.HandleBattleInput(root.Battle, 0x10, serial, 5, challenge);
        panel.Battle = 0;
        Assert.Equal(0u, other.HandleBattleInput(panelToken, 0x10, serial, 5, challenge));
        Assert.Equal(0u, fresh);
        other.Dispose();
    }

    /// <summary>미연결 배틀 입력 장치는 일반 성공으로 꾸미지 않는다. 0.927에서 추가됐다.</summary>
    [Fact]
    public void HandleBattleInputMissingDevicesFailExplicitly()
    {
        using var scene = new Scene();
        scene.Storage.OwnerMode.Challenge = true;
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        challenge.Battle(2).Span[0] = 1;
        var bare = new HostNsInfoDevices
        {
            Read = address => address switch { 0x54278c => 0x10u, 0x542b80 => 0x20u, 0x542b7c => 0x30u, 0x5429e0 => 0x40u, 0x5429e4 => 0x50u, _ => scene.Devices.Read(address) },
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, bare);
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Battle;
        var panel = Assert.IsType<HudBattleInfoNode>(scene.Menu.Tree.Node(panelToken));
        panel.Battle = 2;
        panel.Changed = 1;
        panel.Mode = 2;
        uint serial = scene.Menu.Tree.Node(panel.Title)!.Serial;
        Assert.Throws<InvalidOperationException>(() => other.HandleBattleInput(panelToken, 0x10, serial, 5, challenge));
        Assert.Throws<ArgumentNullException>(() => other.HandleBattleInput(panelToken, 0x10, serial, 5, null!));
        Assert.Throws<InvalidOperationException>(() => other.HandleBattleInput(rootToken, 0x10, serial, 5, challenge));
        other.Dispose();
        Assert.Throws<ObjectDisposedException>(() => other.HandleBattleInput(panelToken, 0x10, serial, 5, challenge));
    }

    /// <summary>분배는 루트 토큰을 실제 참가자 메뉴로 보낸다. 0.929에서 추가됐다.</summary>
    [Fact]
    public void HandleDispatchRoutesRootToMenu()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        WriteParticipant(challenge, 9, battle: 0, flags: 1, name: "me", sid: 7);
        scene.Globals.RosterOrder[5] = 9;
        var menus = new List<(uint Index, ScreenPoint Point)>();
        var based = new List<(uint Event, uint First, uint Second)>();
        var wired = new HostNsInfoDevices
        {
            Read = address => address switch { 0x5427a8 => 0x10u, 0x542790 => 0x20u, 0x54278c => 0x30u, 0x5429dc => 0x40u, _ => scene.Devices.Read(address) },
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            MousePoint = () => new ScreenPoint(11, 22),
            ParticipantMenu = (index, point) => menus.Add((index, point)),
            CenterOnSid = _ => { },
            StatusSend = fill => { byte[] packet = new byte[12]; fill(packet); },
            KickoffSound = () => { },
            BaseInput = (@event, first, second) => { based.Add((@event, first, second)); return 7u; },
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint rootToken = other.Create();
        uint result = other.HandleDispatch(rootToken, 0x20, 1, 5, challenge);
        Assert.Equal(1u, result);
        Assert.Equal([(9u, new ScreenPoint(11, 22))], menus);
        Assert.Empty(based);
        other.Dispose();
    }

    /// <summary>분배는 플레이어 토큰을 실제 준비 플래그 송신으로 보낸다. 0.929에서 추가됐다.</summary>
    [Fact]
    public void HandleDispatchRoutesPlayerToReady()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        uint owner = scene.Game.Player.BattleOwner;
        WriteParticipant(challenge, owner, battle: 2, flags: 0, name: "me", sid: 7);
        var packets = new List<byte[]>();
        var based = new List<(uint Event, uint First, uint Second)>();
        var wired = new HostNsInfoDevices
        {
            Read = address => address switch { 0x5427a8 => 0x10u, 0x542790 => 0x20u, 0x54278c => 0x30u, 0x5429dc => 0x40u, _ => scene.Devices.Read(address) },
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            StatusSend = fill => { byte[] packet = new byte[12]; fill(packet); packets.Add(packet); },
            PlayerSubscriber = () => 0x77u,
            PlayerAuxiliary = _ => { },
            PlayerData = (_, _, _) => { },
            PlayerMenuBegin = () => { },
            PlayerMenuAttach = () => { },
            PlayerMenuTitle = _ => { },
            PlayerMenuItem = (_, _, _) => { },
            BaseInput = (@event, first, second) => { based.Add((@event, first, second)); return 7u; },
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Players[0];
        var panel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(panelToken));
        panel.Player = owner;
        var button = Assert.IsType<HudPlayerButtonNode>(scene.Menu.Tree.Node(panel.Button));
        button.Value = 1;
        uint serial = scene.Menu.Tree.Node(panel.Button)!.Serial;
        uint result = other.HandleDispatch(panelToken, 0x10, serial, 0, challenge);
        Assert.Equal(1u, result);
        Assert.Equal(2u, BitConverter.ToUInt32(challenge.Participant(owner).Span.Slice(0x64, 4)));
        Assert.Single(packets);
        Assert.Empty(based);
        other.Dispose();
    }

    /// <summary>분배는 배틀 토큰을 실제 정렬 메뉴로 보낸다. 0.929에서 추가됐다.</summary>
    [Fact]
    public void HandleDispatchRoutesBattleToSortMenu()
    {
        using var scene = new Scene();
        scene.Storage.OwnerMode.Challenge = true;
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        challenge.Battle(2).Span[0] = 1;
        int sorts = 0;
        var based = new List<(uint Event, uint First, uint Second)>();
        var wired = new HostNsInfoDevices
        {
            Read = address => address switch { 0x54278c => 0x10u, 0x542b80 => 0x20u, 0x542b7c => 0x30u, 0x5429e0 => 0x40u, 0x5429e4 => 0x50u, _ => scene.Devices.Read(address) },
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            BattleGuySound = () => { },
            BattleSortMenu = _ => sorts++,
            BattleOptionsMenu = (_, _) => { },
            BattleExcludeMenu = (_, _) => { },
            BattleOptionMenu = (_, _, _) => { },
            BattleRemoveMenu = _ => { },
            BattleData = (_, _, _) => { },
            BaseInput = (@event, first, second) => { based.Add((@event, first, second)); return 7u; },
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Battle;
        var panel = Assert.IsType<HudBattleInfoNode>(scene.Menu.Tree.Node(panelToken));
        panel.Battle = 2;
        uint serial = scene.Menu.Tree.Node(panel.Title)!.Serial;
        uint result = other.HandleDispatch(panelToken, 0x10, serial, 24, challenge);
        Assert.Equal(1u, result);
        Assert.Equal(1, sorts);
        Assert.Empty(based);
        other.Dispose();
    }

    /// <summary>분배는 알 수 없는 토큰을 기본 처리에 넘기고 미연결을 명시 실패한다. 0.929에서 추가됐다.</summary>
    [Fact]
    public void HandleDispatchFallsThroughAndFailsExplicitly()
    {
        using var scene = new Scene();
        var challenge = new ClientChallengeGlobals(); challenge.Prepare();
        var based = new List<(uint Event, uint First, uint Second)>();
        var wired = new HostNsInfoDevices
        {
            Read = scene.Devices.Read,
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            BaseInput = (@event, first, second) => { based.Add((@event, first, second)); return 7u; },
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint result = other.HandleDispatch(99999u, 0x10, 1, 2, challenge);
        Assert.Equal(7u, result);
        Assert.Equal([(0x10u, 1u, 2u)], based);
        Assert.Throws<ArgumentNullException>(() => other.HandleDispatch(99999u, 0x10, 1, 2, null!));
        var bare = new HostNsInfoDevices
        {
            Read = scene.Devices.Read,
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
        };
        using var missing = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, bare);
        Assert.Throws<InvalidOperationException>(() => missing.HandleDispatch(99999u, 0x10, 1, 2, challenge));
        missing.Dispose();
        other.Dispose();
        Assert.Throws<ObjectDisposedException>(() => other.HandleDispatch(99999u, 0x10, 1, 2, challenge));
    }

    /// <summary>메타맵 관문이 열리면 실제 루트 배치 장치에 전체 본문을 맡긴다. 0.930에서 추가됐다.</summary>
    [Fact]
    public void MetaRootLayoutFiresWhenMetamapKnown()
    {
        using var scene = new Scene();
        scene.Storage.OwnerMode.Challenge = true;
        scene.Game.Player.PlayerKnown = 1;
        var laidOut = new List<uint>();
        var childLaidOut = new List<(HudNsInfoChild Kind, uint Token)>();
        var wired = new HostNsInfoDevices
        {
            Read = scene.Devices.Read,
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            MetaRootLayout = token => laidOut.Add(token),
            MetaChildLayout = (kind, token) => childLaidOut.Add((kind, token)),
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        root.BaseWidth = 300;
        root.BaseHeight = 200;
        scene.Menu.Tree.TreeHooks.Attach!(rootToken, 10, 20, scene.Menu.Tree.Create.Entries[0], 0);
        Assert.Equal([rootToken], laidOut);
        Assert.Equal(9, childLaidOut.Count);
        Assert.Equal((HudNsInfoChild.Battle, root.Battle), childLaidOut[0]);
        other.Dispose();
    }

    /// <summary>미연결 루트 배치 장치는 일반 부착으로 꾸미지 않는다. 0.930에서 추가됐다.</summary>
    [Fact]
    public void MetaRootLayoutMissingFailsExplicitly()
    {
        using var scene = new Scene();
        scene.Storage.OwnerMode.Challenge = true;
        scene.Game.Player.PlayerKnown = 1;
        var childLaidOut = new List<(HudNsInfoChild Kind, uint Token)>();
        var wired = new HostNsInfoDevices
        {
            Read = scene.Devices.Read,
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            MetaChildLayout = (kind, token) => childLaidOut.Add((kind, token)),
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        root.BaseWidth = 300;
        root.BaseHeight = 200;
        Assert.Equal(9, childLaidOut.Count);
        Assert.Throws<InvalidOperationException>(() => scene.Menu.Tree.TreeHooks.Attach!(rootToken, 10, 20, scene.Menu.Tree.Create.Entries[0], 0));
        other.Dispose();
    }

    /// <summary>특수 버튼 그림은 실제 소유 판정을 통과한 뒤 장치에 전달된다. 0.930에서 추가됐다.</summary>
    [Fact]
    public void DrawPlayerButtonDispatchesToDevice()
    {
        using var scene = new Scene();
        var drawn = new List<uint>();
        var wired = new HostNsInfoDevices
        {
            Read = scene.Devices.Read,
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            DrawPlayerButton = (token, _, _, _) => drawn.Add(token),
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        var panel = Assert.IsType<HudPlayerInfoNode>(scene.Menu.Tree.Node(root.Players[0]));
        var clip = new GumpDrawState();
        var target = new GumpPixelTarget(new byte[16], 4, 4, clip);
        var background = new GumpBackgroundDraw(scene.Menu.Tree.Collection, clip, new(), new() { Node = id => scene.Menu.Tree.Node(id) as GumpBackgroundNode });
        bool handled = scene.Menu.Tree.ExternalDrawing.TryDraw(panel.Button, target, new(), background);
        Assert.True(handled);
        Assert.Equal([panel.Button], drawn);
        other.Dispose();
    }

    /// <summary>소유하지 않은 토큰의 그림 요청은 장치에 닿지 않고 실패한다. 0.930에서 추가됐다.</summary>
    [Fact]
    public void DrawPlayerButtonIgnoresUnownedToken()
    {
        using var scene = new Scene();
        var drawn = new List<uint>();
        var wired = new HostNsInfoDevices
        {
            Read = scene.Devices.Read,
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            DrawPlayerButton = (token, _, _, _) => drawn.Add(token),
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint rootToken = other.Create();
        var clip = new GumpDrawState();
        var target = new GumpPixelTarget(new byte[16], 4, 4, clip);
        var background = new GumpBackgroundDraw(scene.Menu.Tree.Collection, clip, new(), new() { Node = id => scene.Menu.Tree.Node(id) as GumpBackgroundNode });
        Assert.False(scene.Menu.Tree.ExternalDrawing.TryDraw(99999u, target, new(), background));
        Assert.False(scene.Menu.Tree.ExternalDrawing.TryDraw(rootToken, target, new(), background));
        Assert.Empty(drawn);
        other.Dispose();
    }

    /// <summary>이벤트 후크는 소유한 루트 토큰을 실제 입력 장치에 전달한다. 0.931에서 추가됐다.</summary>
    [Fact]
    public void HandleEventRoutesOwnedRootToDevice()
    {
        using var scene = new Scene();
        var handled = new List<(uint Token, uint Event, uint First, uint Second)>();
        var wired = new HostNsInfoDevices
        {
            Read = scene.Devices.Read,
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            Handle = (token, @event, first, second) => { handled.Add((token, @event, first, second)); return 7u; },
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint rootToken = other.Create();
        uint result = scene.Menu.Tree.EventHooks.Handle!(rootToken, 0x38, 1, 2);
        Assert.Equal(7u, result);
        Assert.Equal([(rootToken, 0x38u, 1u, 2u)], handled);
        other.Dispose();
    }

    /// <summary>입력 후크는 소유한 플레이어 토큰을 실제 입력 장치에 전달한다. 0.931에서 추가됐다.</summary>
    [Fact]
    public void HandleInputRoutesOwnedPlayerToDevice()
    {
        using var scene = new Scene();
        var handled = new List<(uint Token, uint Event, uint First, uint Second)>();
        var wired = new HostNsInfoDevices
        {
            Read = scene.Devices.Read,
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            Handle = (token, @event, first, second) => { handled.Add((token, @event, first, second)); return 7u; },
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint rootToken = other.Create();
        var root = Assert.IsType<HudNsInfoNode>(scene.Menu.Tree.Node(rootToken));
        uint panelToken = root.Players[0];
        uint result = scene.Menu.Tree.InputHooks.Handle!(panelToken, 0x38, 3, 4);
        Assert.Equal(7u, result);
        Assert.Equal([(panelToken, 0x38u, 3u, 4u)], handled);
        other.Dispose();
    }

    /// <summary>반납 뒤 입력 후크는 이전 소유 장치에 닿지 않고 기본 처리에 넘어간다. 0.931에서 추가됐다.</summary>
    [Fact]
    public void HandleAfterDisposeFallsThroughToBase()
    {
        using var scene = new Scene();
        var handled = new List<(uint Token, uint Event, uint First, uint Second)>();
        var wired = new HostNsInfoDevices
        {
            Read = scene.Devices.Read,
            Translate = scene.Devices.Translate, StoreText = scene.Devices.StoreText,
            Text = scene.Devices.Text, FormatZone = scene.Devices.FormatZone, Changed = scene.Devices.Changed,
            Handle = (token, @event, first, second) => { handled.Add((token, @event, first, second)); return 7u; },
        };
        using var other = new HostNsInfoWindows(scene.Menu.Tree, scene.Text, scene.Globals, scene.Game, scene.Storage, wired);
        uint rootToken = other.Create();
        uint alive = scene.Menu.Tree.EventHooks.Handle!(rootToken, 0x38, 1, 2);
        Assert.Equal(7u, alive);
        Assert.Single(handled);
        other.Dispose();
        uint result = scene.Menu.Tree.EventHooks.Handle!(rootToken, 0x38, 1, 2);
        Assert.Equal(0u, result);
        Assert.Single(handled);
    }
}
