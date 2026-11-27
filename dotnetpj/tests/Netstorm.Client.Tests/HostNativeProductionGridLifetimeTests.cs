using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 생산 격자와 Native 자산의 부모 삭제 수명을 함께 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>제품과 같은 등록표·공장·세션에서 생산 격자의 부모 소실을 검증한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>검사 전용 적재 관문 뒤 실제 내용물·메뉴·Form과 현재 비용 및 도움말을 연결한다.</summary>
    [Fact]
    public void ProductionAboutUsesActualSessionMenuAndForm()
    {
        var (session, types, surface) = WorldDeletionRootScene(sharedNative: true);
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var screen = new Screen(1024, 768); Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new() { Now = () => 0, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
        var tree = menu.Tree; var html = new HostHtmlWindows(menu, new InputQueue(), () => 0, _ => { }); html.BindParser(startup, _ => { });
        var text = new HostHtmlText(menu, html, screen, _ => default, (_, _) => { }, _ => { }, _ => { }); text.Bind(html);
        var assets = Assert.IsType<HostAssetTables>(surface.Storage.Source());
        text.BindPictures(new(assets.TypeTable, assets.Definitions, new(assets.ShapeImage)), () => SpriteColorMapBuilder.BuildSpriteColorMaps(screen.LogicalPalette).Shadow, startup.Configuration);
        var menus = new HostHtmlMenus(menu, html, text, startup, screen, (_, _) => { }); menus.BindInput(new(), _ => { }, (_, _) => { });
        using var ui = new HostGumpFormRuntime(surface.Storage, tree, (_, _) => false, () => { });
        menus.BindObjectForms(surface.Storage.GumpForms());
        int content = session.BuildPlayerWorldHooks().Create!(2, 2);
        BinaryPrimitives.WriteUInt16LittleEndian(session.Pool.AllocatedBytes(content)[18..], 71);
        var item = new InventoryItemNode { Content = (uint)content };
        var sounds = new List<string>(); int changes = 0, moneyChecks = 0;
        var devices = new HostProductionAboutDevices
        {
            Description = type => types[(int)type].Description, OwnerName = owner => "Player" + owner,
            Translate = value => value, Substitute = value => value, HelpWord = () => "Help",
            Sound = sounds.Add, Changed = () => changes++,
        };
        uint Open() => HostProductionAbout.Open(session, assets.Definitions, menus, item, new(30, 40), _ => { moneyChecks++; return true; }, devices);
        Assert.Throws<InvalidOperationException>(() => Open()); Assert.Equal(0, moneyChecks);
        // 실제 완전 적재를 대신하지 않으며 이 독립 검사에서만 관문을 연다.
        var loaded = typeof(HostSession).GetField("_nativeLoaded", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance)!;
        loaded.SetValue(session, true);
        try
        {
            Assert.Equal(1u, Open()); uint token = menus.Popup;
            var node = Assert.IsType<MenuGumpNode>(tree.Node(token));
            Assert.Equal(tree.Create.Entries[0], node.Parent); Assert.Equal((uint)content, node.Object);
            Assert.NotEqual(0u, node.Form); Assert.Equal(node.Id, surface.Storage.GumpForms().Owner((int)node.Form));
            Assert.Equal(content, surface.Storage.GumpForms().Parent((int)node.Form));
            Assert.True(node.ItemCount > 0); Assert.Equal(1, moneyChecks); Assert.Equal(new[] { "opengump.wav" }, sounds);
            uint form = node.Form; tree.Lifetime.Kill(token); tree.Lifetime.Cleanup();
            Assert.Null(tree.Node(token)); Assert.NotEqual(0, session.Pool.Slot((int)form)[11] & 1);
            Assert.Equal(3, changes);
        }
        finally { loaded.SetValue(session, false); html.CloseAll(); session.ReleasePlayerProcesses(); }
    }

    /// <summary>생산 사건으로 두 격자를 연결한 뒤 공간 자산 삭제가 파생 통지에 도달한다.</summary>
    [Theory]
    [InlineData(80u)]
    [InlineData(152u)]
    public void NativeProductionGridsReceiveActualParentDeletion(uint type)
    {
        var (session, _, surface) = WorldDeletionRootScene(sharedNative: true);
        session.BuildWorldDeletionRootDevices();
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var screen = new Screen(1024, 768); Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new() { Now = () => 0, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
        var tree = menu.Tree;
        var html = new HostHtmlWindows(menu, new InputQueue(), () => 0, _ => { }); html.BindParser(startup, _ => { });
        var text = new HostHtmlText(menu, html, screen, _ => default, (_, _) => { }, _ => { }, _ => { }); text.Bind(html);
        var assets = Assert.IsType<HostAssetTables>(surface.Storage.Source());
        text.BindPictures(new(assets.TypeTable, assets.Definitions, new(assets.ShapeImage)), () => SpriteColorMapBuilder.BuildSpriteColorMaps(screen.LogicalPalette).Shadow, startup.Configuration);
        var menus = new HostHtmlMenus(menu, html, text, startup, screen, (_, _) => { }); menus.BindInput(new(), _ => { }, (_, _) => { });
        var context = new ProductionControlContext { Storage = surface.Storage, Gameplay = session.Gameplay, Players = session.Gameplay.PlayerRecords, Options = new byte[87] };
        HostProductionWindows? panel = null; int notices = 0;
        using var ui = new HostGumpFormRuntime(surface.Storage, tree, (token, parent) =>
        {
            if (panel?.Node(token) is null) return false;
            notices++; panel.ParentGone(token, unchecked((uint)parent)); return true;
        }, () => { });
        menus.BindObjectForms(surface.Storage.GumpForms());
        using (var runtime = HostMissionRuntime.Create(session, surface.Storage))
        {
            panel = new HostProductionWindows(tree, text, context, new() { BackgroundType = menu.BackgroundType }, new()
            {
                Now = () => 0, Sound = _ => { }, Tell = _ => { }, ConfigInt = (_, fallback) => fallback, Translate = value => value,
                Changed = () => { }, BaseHandle = (_, _, _, _) => 0, Redraw = (_, _) => { },
                ClosePopups = () => menus.RequestClosePopups(() => { }), MenuInstances = () => menus.State.Instances,
                BindForm = (token, parent) => HostProductionFormBinding.Bind(surface.Storage, tree, token, parent),
            });
            uint rootToken = panel.Construct(); var root = Assert.IsType<ProductionRootNode>(panel.Node(rootToken));
            int parent = session.BuildPlayerWorldHooks().Create!(type, 2);
            if (type == 80) session.PopWorldObject(parent, 50, 60, 0);
            else session.PriestWorld.SetOwner(parent, 1);
            session.MissionPlayer.PlayerKnown = 1; session.MissionPlayer.BattleOwner = 1;
            BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x9c), (uint)parent);
            Assert.Equal(0u, panel.Handle(rootToken, 0x0200003d, 0, 0));
            var bridge = Assert.IsType<InventoryGridNode>(panel.Node(root.BridgeGrid));
            var teleport = Assert.IsType<InventoryGridNode>(panel.Node(root.TeleportGrid));
            uint bridgeForm = bridge.Form, teleportForm = teleport.Form;
            Assert.NotEqual(0u, bridgeForm); Assert.NotEqual(0u, teleportForm); Assert.NotEqual(bridgeForm, teleportForm);
            uint popup = menus.Widgets.BeginFancy();
            menus.Widgets.Reference(popup, (uint)parent);
            menus.Menu.AttachCurrent(20, 20, tree.Create.Collection.Root);
            var popupNode = Assert.IsType<MenuGumpNode>(tree.Node(popup)); uint popupForm = popupNode.Form;
            Assert.NotEqual(0u, popupForm);
            Assert.Equal((uint)parent, popupNode.Object);
            Assert.Equal(popupNode.Id, surface.Storage.GumpForms().Owner((int)popupForm));
            Assert.Equal(parent, surface.Storage.GumpForms().Parent((int)popupForm));
            Assert.True(session.DestroyResourceObject(parent, 0));
            Assert.Equal(2, notices);
            Assert.NotEqual(0, session.Pool.Slot((int)bridgeForm)[11] & 1);
            Assert.NotEqual(0, session.Pool.Slot((int)teleportForm)[11] & 1);
            Assert.Equal((0u, 0u, 0u), (bridge.Object, bridge.Form, bridge.Flags & 0x40));
            Assert.Equal((0u, 0u, 0u), (teleport.Object, teleport.Form, teleport.Flags & 0x40));
            Assert.NotEqual(0, session.Pool.Slot((int)popupForm)[11] & 1);
            tree.Lifetime.Cleanup(); Assert.Null(tree.Node(popup));
            Assert.Equal(0u, surface.Storage.BaseDestroy().PreDepth); Assert.Equal(0u, surface.Storage.BaseDestroy().PostDepth);
            var borrowed = new List<int>(); var reused = new HashSet<int>();
            // 실제 FIFO 풀을 순회해 반환된 두 Form 번호가 다른 할당에 재사용될 때까지 보유한다.
            while (reused.Count < 2 && borrowed.Count < 15000)
            {
                int sid = session.Pool.Allocate(SidAllocation.Client); borrowed.Add(sid);
                if (sid == bridgeForm || sid == teleportForm)
                {
                    reused.Add(sid); session.Pool.AllocatedBytes(sid)[10] = 80;
                }
            }
            Assert.Equal(2, reused.Count);
            panel.Dispose(); panel = null; html.CloseAll(); session.ReleasePlayerProcesses();
            // 이전 격자 정리가 재사용된 다른 객체의 슬롯을 건드리지 않아야 한다.
            foreach (int sid in reused)
            {
                Assert.Equal(80, session.Pool.AllocatedBytes(sid)[10]);
            }
            // 검사에서 보유한 임시 슬롯은 같은 풀에 반환한다.
            foreach (int sid in borrowed) session.Pool.Release(sid);
        }
    }
}
