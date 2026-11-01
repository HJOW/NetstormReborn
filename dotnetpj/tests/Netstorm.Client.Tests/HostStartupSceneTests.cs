using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Client.Tests;

/// <summary>실제 배포 자산의 설정·팔레트·메뉴 GIF가 논리 화면에 연결되는지 검사한다.</summary>
public sealed class HostStartupSceneTests
{
    /// <summary>저장소의 게임 데이터에서 시작 자산을 찾아 팔레트와 화면을 채운다.</summary>
    [Fact]
    public void LoadsRealTitleAndPaletteIntoLogicalScreen()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet));
        startup.Draw(screen);
        Assert.Equal(1U, screen.PaletteLoaded);
        Assert.Contains(screen.Pixels.ToArray(), pixel => pixel != 0);
        Assert.NotEmpty(startup.Configuration.PathSpec("BackgroundGifSpec", "titleMenu"));
        string path = startup.Configuration.PathSpec("CloudFileSpec");
        var cloud = Netstorm.Client.Display.GifImage.DecodeBackgroundGif(startup.Files.ReadAllBytes(path));
        Assert.Equal(512, cloud.Width); Assert.Equal(512, cloud.Height);
        Assert.Equal(cloud.Indices[0], screen.Pixels[0]);
        Assert.Equal(cloud.Indices[0], screen.Pixels[512]);
        Assert.Equal(cloud.Indices[0], screen.Pixels[512 * screen.Pitch]);
    }
    /// <summary>호스트 설정은 실제 Config의 시작 인자로 전달되어 파일 설정에 우선한다.</summary>
    [Fact]
    public void HostOverridesReachActualConfiguration()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var options = HostOptions.Parse(["--set", "tellTips=1;maxFPS=35", "--set", "tellTips=0"]);
        var startup = new HostStartupScene(data, options.SettingsText);
        Assert.Equal(0, startup.Configuration.GetInt("tellTips"));
        Assert.Equal(35, startup.Configuration.GetInt("maxFPS"));
        Assert.Equal(35, options.FramesPerSecond);
    }

    /// <summary>실제 fortGump·글꼴을 사용해 메인 메뉴 여덟 버튼이 각각 타이틀 화면을 변경한다.</summary>
    [Fact]
    public void MainMenuDrawsEightRealAssetButtons()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        byte[] before = screen.Pixels.ToArray();
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => throw new InvalidOperationException("입력 전용"), Poll = _ => throw new InvalidOperationException("입력 전용"),
            Sound = () => throw new InvalidOperationException("입력 전용"), Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        });
        menu.Draw(screen);
        // 버튼마다 해당 사각형 안에서 적어도 하나의 픽셀이 실제 자산 출력으로 달라진다.
        for (int index = 0; index < 8; index++)
        {
            int x = 1024 / 2 - 156 + (index % 4) * 79, y = 768 / 2 - 73 + (index / 4) * 23;
            bool changed = false;
            // 각 버튼의 표시 영역만 비교하며 다른 버튼의 변경은 세지 않는다.
            for (int row = y; row < y + 19 && !changed; row++)
                for (int col = x; col < x + 75; col++)
                    if (before[row * screen.Pitch + col] != screen.Pixels[row * screen.Pitch + col]) { changed = true; break; }
            Assert.True(changed, $"버튼 {index} 출력이 없다");
        }
    }

    /// <summary>여덟 실제 버튼의 눌림·해제를 원본 상위 명령과 인자로 전달한다.</summary>
    [Fact]
    public void MainMenuRoutesEachButtonToItsOriginalAction()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var actions = new List<MainMenuAction>(); int sounds = 0;
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => sounds++,
            Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        }, actions.Add);
        // 실제 논리 화면의 각 버튼 내부에서 누른 뒤 같은 위치에서 뗀다.
        for (int index = 0; index < 8; index++)
        {
            int x = 1024 / 2 - 156 + (index % 4) * 79 + 3, y = 768 / 2 - 73 + (index / 4) * 23 + 3;
            Assert.NotEqual(0U, menu.Route(new(InputCode.LeftButton, x, y)));
            Assert.NotEqual(0U, menu.Route(new(InputCode.LeftButton | InputCode.Release, x, y)));
        }
        Assert.Equal(new MainMenuAction[] { new("Tell", "UCampaign"), new("Tell", "NotImplemented"), new("Tell", "DEMOVILLE"),
            new("Menu", "help"), new("AskEditFort", "0"), new("Tell", "Credits"), new("Menu", "options"), new("RequestQuitApp", "0") }, actions);
        Assert.Equal(8, sounds);
        // 버튼 밖에서 떼면 캡처를 풀고 명령은 발생하지 않는다.
        Assert.NotEqual(0U, menu.Route(new(InputCode.LeftButton, 360, 315)));
        Assert.NotEqual(0U, menu.Route(new(InputCode.LeftButton | InputCode.Release, 2, 2)));
        Assert.Equal(8, actions.Count);
    }

}
