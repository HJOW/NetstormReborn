using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;

namespace Netstorm.Client.Tests;

// RU-3d 알림 등록 ID의 실제 HUD 작성자 계약이다. 0.990에서 추가됐다.
public sealed class ChatAlertRegisterTests
{
    /// <summary>창이나 음향을 열지 않고 원본 자산과 같은 메뉴 트리를 구성한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>동일 실제 팔레트와 8비트 화면이다.</summary>
        public readonly Screen Screen;
        /// <summary>수신 옵션과 HTML 생성기가 공유하는 실제 시작 설정이다.</summary>
        public readonly HostStartupScene Startup;
        /// <summary>같은 실제 메뉴와 공통 노드 트리다.</summary>
        public readonly HostMainMenu Menu;

        /// <summary>같은 실제 파일 자산으로 메뉴 트리를 만든다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); Startup = new HostStartupScene(data); Screen = new(1024, 768); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); Startup.Draw(Screen);
            Menu = new(Startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
        }

        /// <summary>최종 owner 반납 없이 같은 실제 트리를 정리한다.</summary>
        public void Dispose() { }
    }

    /// <summary>HudChatNode가 아닌 실제 돌 버튼을 만든다.</summary>
    private static StoneButtonGumpNode AddChatStone(Scene scene)
    {
        uint alert = scene.Menu.Tree.AddStone(20, 20, "Chat\0"u8.ToArray(), 10, 10);
        return Assert.IsType<StoneButtonGumpNode>(scene.Menu.Tree.Node(alert));
    }

    /// <summary>등록 ID가 실제 HUD 작성자에 닿아 같은 칸에 남는다.</summary>
    [Fact]
    public void RegisterChatAlertStoresButtonId()
    {
        using var scene = new Scene();
        var button = AddChatStone(scene);
        scene.Menu.Tree.RegisterChatAlert(button.Id);
        Assert.Equal(button.Id, scene.Menu.Tree.ChatAlertId.Value);
    }

    /// <summary>등록한 버튼에 실제 수신 wake가 닿아 점멸한다.</summary>
    [Fact]
    public void WakeReachesRegisteredButton()
    {
        using var scene = new Scene();
        var button = AddChatStone(scene);
        scene.Menu.Tree.RegisterChatAlert(button.Id);
        Assert.True(scene.Menu.Tree.PulseRegisteredIfIdle(scene.Menu.Tree.ChatAlertId.Value));
        Assert.NotEqual(0u, button.ButtonFlags & 0x10u);
    }

    /// <summary>범위 밖 등록은 거부한다.</summary>
    [Fact]
    public void RegisterChatAlertRejectsOutOfRange()
    {
        using var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Menu.Tree.RegisterChatAlert(100000));
    }

    /// <summary>빈 칸 등록은 거부한다.</summary>
    [Fact]
    public void RegisterChatAlertRejectsEmptySlot()
    {
        using var scene = new Scene();
        var button = AddChatStone(scene);
        uint saved = scene.Menu.Tree.Create.Entries[button.Id];
        scene.Menu.Tree.Create.Entries[button.Id] = 0;
        try { Assert.Throws<InvalidOperationException>(() => scene.Menu.Tree.RegisterChatAlert(button.Id)); }
        finally { scene.Menu.Tree.Create.Entries[button.Id] = saved; }
    }

    /// <summary>버튼이 아닌 노드 등록은 거부한다.</summary>
    [Fact]
    public void RegisterChatAlertRejectsNonButtonNode()
    {
        using var scene = new Scene();
        AddChatStone(scene);
        Assert.Throws<InvalidOperationException>(() => scene.Menu.Tree.RegisterChatAlert(0));
    }
}
