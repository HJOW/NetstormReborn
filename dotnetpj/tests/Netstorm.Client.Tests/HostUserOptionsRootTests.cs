using Netstorm.Assets;
using Netstorm.Client.Host;

// 옵션 파일의 사용자 루트 읽기·쓰기를 설치 경로와 대조해 검증한다.
// 2026-11-26 1.041에서 WP-5a 사용자 옵션 경로용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>사용자 옵션 경로의 저장·덮어쓰기·폴백을 같은 실제 자료에서 대조한다.</summary>
public sealed class HostUserOptionsRootTests
{
    /// <summary>실제 게임 자료 디렉터리를 찾는다.</summary>
    private static string DataDirectory()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        return data;
    }

    /// <summary>검사 뒤 지우는 임시 사용자 루트를 만든다.</summary>
    private static string TemporaryUserRoot()
    {
        string root = Path.Combine(Path.GetTempPath(), "netstorm-dn-user-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        return root;
    }

    /// <summary>저장은 사용자 파일에만 하고 설치 파일은 바꾸지 않는다.</summary>
    [Fact]
    public void SaveOptionsWritesUserRootAndPreservesInstallFile()
    {
        string data = DataDirectory();
        string installOptions = Path.Combine(Path.GetFullPath(data), "d", "options.cfg");
        Assert.True(File.Exists(installOptions));
        byte[] before = File.ReadAllBytes(installOptions);
        string root = TemporaryUserRoot();
        try
        {
            string userOptions = Path.Combine(root, "Saves", "options.cfg");
            var startup = new HostStartupScene(data, "", null, userOptions);
            startup.Configuration.Set("soundVolume", "5");
            startup.SaveOptions();
            Assert.True(File.Exists(userOptions));
            Assert.Equal(before, File.ReadAllBytes(installOptions));
        }
        finally
        {
            Directory.Delete(root, true);
        }
    }

    /// <summary>사용자 값이 있으면 설치 값보다 이긴다.</summary>
    [Fact]
    public void UserOverlayWinsOverInstallValues()
    {
        string data = DataDirectory();
        string root = TemporaryUserRoot();
        try
        {
            string userOptions = Path.Combine(root, "Saves", "options.cfg");
            var first = new HostStartupScene(data, "", null, userOptions);
            first.Configuration.Set("soundVolume", "5");
            first.SaveOptions();
            var second = new HostStartupScene(data, "", null, userOptions);
            Assert.Equal("5", second.Configuration.Configuration.Get("soundVolume"));
        }
        finally
        {
            Directory.Delete(root, true);
        }
    }

    /// <summary>사용자 파일이 없으면 설치 값을 그대로 쓰고 저장 때 폴더를 만든다.</summary>
    [Fact]
    public void MissingUserFileFallsBackToInstallValues()
    {
        string data = DataDirectory();
        string root = TemporaryUserRoot();
        try
        {
            string userOptions = Path.Combine(root, "Saves", "options.cfg");
            Assert.False(File.Exists(userOptions));
            var plain = new HostStartupScene(data);
            var overlaid = new HostStartupScene(data, "", null, userOptions);
            Assert.Equal(
                plain.Configuration.Configuration.Get("soundVolume"),
                overlaid.Configuration.Configuration.Get("soundVolume"));
            overlaid.SaveOptions();
            Assert.True(File.Exists(userOptions));
        }
        finally
        {
            Directory.Delete(root, true);
        }
    }

    /// <summary>읽을 수 없는 사용자 경로는 설치 값을 보존하고 무시한다.</summary>
    [Fact]
    public void UnreadableUserPathFallsBackToInstallValues()
    {
        string data = DataDirectory();
        string root = TemporaryUserRoot();
        try
        {
            // 디렉터리 경로를 파일처럼 주면 읽기가 실패한다.
            var plain = new HostStartupScene(data);
            var overlaid = new HostStartupScene(data, "", null, root);
            Assert.Equal(
                plain.Configuration.Configuration.Get("soundVolume"),
                overlaid.Configuration.Configuration.Get("soundVolume"));
        }
        finally
        {
            Directory.Delete(root, true);
        }
    }
}
