using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>실제 옵션 파일의 인코딩과 저장 실패 수명을 검사한다.</summary>
public sealed class HostOptionsFileTests
{
    [Fact]
    public void SavedScreenOptionsApplyUnlessExplicitlyOverridden()
    {
        var config = new ConfigInterface(null); config.Startup(new());
        config.SetInt("SCREENW", 640); config.SetInt("SCREENH", 480); config.SetInt("maxFPS", 30); config.SetInt("startInFullScreen", 1);
        var restored = HostOptions.Parse([]); restored.ApplyStartupConfiguration(config);
        Assert.Equal(640, restored.Width); Assert.Equal(480, restored.Height); Assert.Equal(30, restored.FramesPerSecond); Assert.True(restored.Fullscreen);
        var explicitOptions = HostOptions.Parse(["--width", "800", "--set", "SCREENH=600;maxFPS=50", "--window"]); explicitOptions.ApplyStartupConfiguration(config);
        Assert.Equal(800, explicitOptions.Width); Assert.Equal(600, explicitOptions.Height); Assert.Equal(50, explicitOptions.FramesPerSecond); Assert.False(explicitOptions.Fullscreen);
    }

    [Fact]
    public void UnicodeAndOriginalBytesRoundTrip()
    {
        const string original = "mQdsTName=\"café €\"\r\n";
        Assert.Equal(ConfigInterface.EncodeConfigFile(original), HostOptionsFile.Encode(original));
        const string text = "mQdsTfirst=0\r\nInstallDir=\"H:\\한글\\😀\"\r\n";
        byte[] bytes = HostOptionsFile.Encode(text); byte[] plain = (byte[])bytes.Clone(); XorCipher.Apply(plain);
        Assert.Equal(new byte[] { 0xef, 0xbb, 0xbf }, plain[5..8]);
        var config = new Config(new ConfigRegistry()); config.LoadBytes(bytes);
        Assert.Equal("H:\\한글\\😀", config.Get("InstallDir"));
        Assert.ThrowsAny<Exception>(() => HostOptionsFile.Encode("mQdsTbroken=\"\ud800\""));
    }

    [Fact]
    public void FailedWriteKeepsChangedAndIsNotRetried()
    {
        var config = new ConfigInterface(null); config.Startup(new()); config.Set("InstallDir", "H:\\한글\\😀");
        int writes = 0;
        Assert.Throws<IOException>(() => HostOptionsFile.Save(config, (_, _) => { writes++; throw new IOException(); }));
        Assert.Equal(1, writes); Assert.True(config.Configuration.Changed);
        byte[]? output = null; HostOptionsFile.Save(config, (name, bytes) => { Assert.Equal("d\\options.cfg", name); output = bytes; });
        Assert.False(config.Configuration.Changed); Assert.NotNull(output);
        var restored = new Config(new ConfigRegistry()); restored.LoadBytes(output!);
        Assert.Equal("H:\\한글\\😀", restored.Get("InstallDir"));
    }
}
