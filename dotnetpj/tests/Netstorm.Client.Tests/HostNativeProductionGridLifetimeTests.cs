using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 실제 생산 격자와 Native 자산의 부모 삭제 수명을 함께 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>제품과 같은 등록표·공장·세션에서 생산 격자의 부모 소실을 검증한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
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
            Assert.True(session.DestroyResourceObject(parent, 0));
            Assert.Equal(2, notices);
            Assert.NotEqual(0, session.Pool.Slot((int)bridgeForm)[11] & 1);
            Assert.NotEqual(0, session.Pool.Slot((int)teleportForm)[11] & 1);
            Assert.Equal((0u, 0u, 0u), (bridge.Object, bridge.Form, bridge.Flags & 0x40));
            Assert.Equal((0u, 0u, 0u), (teleport.Object, teleport.Form, teleport.Flags & 0x40));
            Assert.Equal(0u, surface.Storage.BaseDestroy().PreDepth); Assert.Equal(0u, surface.Storage.BaseDestroy().PostDepth);
            panel.Dispose(); panel = null; html.CloseAll(); session.ReleasePlayerProcesses();
        }
    }
}
