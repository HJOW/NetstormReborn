using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;

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

}
