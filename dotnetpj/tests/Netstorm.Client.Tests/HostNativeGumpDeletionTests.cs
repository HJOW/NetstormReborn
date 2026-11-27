using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 공유 세션의 부모 삭제와 UI Form 중첩 삭제를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>공통 세션 삭제와 같은 UI 등록표의 수명을 함께 검증한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>부모 프로세스 Form 삭제가 부착 GumpForm과 등록 UI를 같은 삭제 깊이에서 정리한다.</summary>
    [Fact]
    public void NativeParentDeletionReleasesAttachedGumpThroughActualUi()
    {
        var (session, _, surface) = WorldDeletionRootScene(sharedNative: true);
        session.BuildWorldDeletionRootDevices();
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var screen = new Screen(1024, 768); Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new() { Now = () => 0, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
        var tree = menu.Tree;
        int changed = 0, destroyed = 0;
        using var ui = new HostGumpFormRuntime(surface.Storage, tree, (_, _) => false, () => changed++);
        using (var runtime = HostMissionRuntime.Create(session, surface.Storage))
        {
            int asset = session.BuildPlayerWorldHooks().Create!(152, 2);
            var process = new ObservedWorldDeletionProcess(_ => destroyed++);
            Assert.Same(process, session.PlayerProcesses.Attach(process, 10, asset, 0x50));
            int parent = process.Form();
            uint token = tree.AllocateNode(new GumpCreateNode()); tree.Factory.Construct(token, 0, 0, 0, 1);
            GumpCreateNode node = tree.Node(token)!;
            var forms = surface.Storage.GumpForms();
            int form = checked((int)HostProductionFormBinding.Bind(surface.Storage, tree, token, (uint)parent));
            Assert.Equal(node.Id, forms.Owner(form)); Assert.Equal(parent, forms.Parent(form));
            node.Object = (uint)parent; node.Form = (uint)form; node.Flags |= 0x40;
            Assert.True(session.DestroyResourceObject(parent, 0));
            Assert.Equal(1, destroyed); Assert.Equal(1, changed);
            Assert.NotEqual(0, session.Pool.Slot(parent)[11] & 1);
            Assert.NotEqual(0, session.Pool.Slot(form)[11] & 1);
            Assert.NotEqual(0u, node.Flags & 0x10000);
            Assert.Equal(0u, surface.Storage.BaseDestroy().PreDepth);
            Assert.Equal(0u, surface.Storage.BaseDestroy().PostDepth);
            tree.Lifetime.Cleanup(); Assert.Null(tree.Node(token));
            session.ReleasePlayerProcesses();
        }
    }
}
