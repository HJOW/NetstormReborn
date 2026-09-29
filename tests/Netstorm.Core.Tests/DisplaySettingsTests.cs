using Netstorm.Core.Display;

namespace Netstorm.Core.Tests;

/// <summary>표시 설정 저장·복구 테스트 (원본의 전체화면 저장 후 재실행 오류를 재현하지 않기 위한 안전장치 포함)</summary>
public sealed class DisplaySettingsTests : IDisposable
{
    /// <summary>테스트마다 새로 만드는 임시 폴더</summary>
    private readonly string _directory = Path.Combine(Path.GetTempPath(), "netstorm-settings-" + Guid.NewGuid().ToString("N"));

    /// <summary>임시 폴더를 지운다</summary>
    public void Dispose()
    {
        if (Directory.Exists(_directory))
        {
            Directory.Delete(_directory, recursive: true);
        }
    }

    /// <summary>저장한 값이 그대로 다시 읽힌다 (전체화면 설정 포함)</summary>
    [Fact]
    public void SaveThenLoad_RoundTrips()
    {
        string path = Path.Combine(_directory, "sub", DisplaySettings.FileName);
        var settings = new DisplaySettings
        {
            Fullscreen = true, ViewHeight = 600, WideScreen = WideScreenMode.Letterbox,
            EdgeScroll = false, EdgeScrollSpeed = 20, WindowWidth = 1280, WindowHeight = 720,
        };
        Assert.True(settings.Save(path));
        DisplaySettings loaded = DisplaySettings.Load(path);
        Assert.True(loaded.Fullscreen);
        Assert.Equal(600, loaded.ViewHeight);
        Assert.Equal(WideScreenMode.Letterbox, loaded.WideScreen);
        Assert.False(loaded.EdgeScroll);
        Assert.Equal(20, loaded.EdgeScrollSpeed);
        Assert.Equal((1280, 720), (loaded.WindowWidth, loaded.WindowHeight));
        Assert.False(File.Exists(path + ".tmp"));
    }

    /// <summary>파일이 없거나 JSON 이 깨져도 기본 설정으로 시작한다 (창 모드, 4:3 1024×768 기준)</summary>
    [Fact]
    public void Load_MissingOrCorrupt_GivesDefaults()
    {
        string path = Path.Combine(_directory, DisplaySettings.FileName);
        DisplaySettings missing = DisplaySettings.Load(path);
        Assert.False(missing.Fullscreen);
        Assert.Equal(768, missing.ViewHeight);
        Assert.Equal(WideScreenMode.Extend, missing.WideScreen);
        Assert.True(missing.EdgeScroll);
        Assert.Equal(35, missing.EdgeScrollSpeed);
        Directory.CreateDirectory(_directory);
        File.WriteAllText(path, "{ 깨진 JSON");
        Assert.False(DisplaySettings.Load(path).Fullscreen);
    }

    /// <summary>허용 범위를 벗어난 값은 가까운 값으로 고쳐 읽는다</summary>
    [Fact]
    public void Load_OutOfRangeValues_AreNormalized()
    {
        string path = Path.Combine(_directory, DisplaySettings.FileName);
        Directory.CreateDirectory(_directory);
        File.WriteAllText(path, """{ "viewHeight": 700, "windowWidth": 5, "windowHeight": 99999, "edgeScrollSpeed": -4, "wideScreen": "Letterbox" }""");
        DisplaySettings loaded = DisplaySettings.Load(path);
        Assert.Equal(768, loaded.ViewHeight);
        Assert.Equal(320, loaded.WindowWidth);
        Assert.Equal(16384, loaded.WindowHeight);
        Assert.Equal(0, loaded.EdgeScrollSpeed);
        Assert.Equal(WideScreenMode.Letterbox, loaded.WideScreen);
    }

    /// <summary>시작 처리 표식이 저장된 채 읽히면 직전 시작이 끝나지 못한 것으로 알 수 있다</summary>
    [Fact]
    public void StartupInProgress_SurvivesRoundTrip()
    {
        string path = Path.Combine(_directory, DisplaySettings.FileName);
        Assert.True(new DisplaySettings { Fullscreen = true, StartupInProgress = true }.Save(path));
        DisplaySettings loaded = DisplaySettings.Load(path);
        Assert.True(loaded.StartupInProgress);
        Assert.True(loaded.Fullscreen);
    }

    /// <summary>환경 변수로 설정 폴더를 바꾼다 (테스트·스크린샷이 사용자 설정을 건드리지 않게)</summary>
    [Fact]
    public void DefaultPath_HonorsEnvironmentOverride()
    {
        string? previous = Environment.GetEnvironmentVariable("NETSTORM_SETTINGS_DIR");
        try
        {
            Environment.SetEnvironmentVariable("NETSTORM_SETTINGS_DIR", _directory);
            Assert.Equal(Path.Combine(_directory, DisplaySettings.FileName), DisplaySettings.DefaultPath());
        }
        finally
        {
            Environment.SetEnvironmentVariable("NETSTORM_SETTINGS_DIR", previous);
        }
    }
}
