using Netstorm.Assets;
using Netstorm.Client.Host;

// 설치 트리를 읽기 전용으로 두어도 시작·옵션·편집 흐름이 사용자 영역에서 동작하는지 검증한다.
// 2026-11-26 1.044에서 WP-5d 읽기 전용 설치 확인용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>읽기 전용 설치 복사본에서 게임 읽기·사용자 쓰기를 대조한다.</summary>
public sealed class HostReadOnlyInstallTests
{
    /// <summary>실제 게임 자료 디렉터리를 찾는다.</summary>
    private static string DataDirectory()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        return data;
    }

    /// <summary>임시 루트 아래 읽기 전용으로 복사된 파일의 속성을 풀고 지운다.</summary>
    private static void DeleteRoot(string root)
    {
        foreach (string file in Directory.EnumerateFiles(root, "*", SearchOption.AllDirectories))
        {
            File.SetAttributes(file, FileAttributes.Normal);
        }

        Directory.Delete(root, true);
    }

    /// <summary>디렉터리 아래 모든 파일의 상대 경로를 모은다.</summary>
    private static List<string> SnapshotFiles(string root)
    {
        return Directory.EnumerateFiles(root, "*", SearchOption.AllDirectories)
            .Select(path => Path.GetRelativePath(root, path)).OrderBy(name => name, StringComparer.Ordinal).ToList();
    }

    /// <summary>디렉터리 아래 모든 파일에 읽기 전용을 붙이고 복원 장치를 돌려준다.</summary>
    private static Action Freeze(string root)
    {
        var changed = new List<string>();
        foreach (string file in Directory.EnumerateFiles(root, "*", SearchOption.AllDirectories))
        {
            FileAttributes attributes = File.GetAttributes(file);
            if ((attributes & FileAttributes.ReadOnly) == 0)
            {
                File.SetAttributes(file, attributes | FileAttributes.ReadOnly);
                changed.Add(file);
            }
        }

        return () =>
        {
            foreach (string file in changed)
            {
                if (File.Exists(file)) File.SetAttributes(file, File.GetAttributes(file) & ~FileAttributes.ReadOnly);
            }
        };
    }

    /// <summary>읽기 전용 설치에서도 옵션 읽기·사용자 저장이 동작하고 설치본은 그대로다.</summary>
    [Fact]
    public void ReadOnlyInstallKeepsOptionsRunningInUserRoot()
    {
        string data = DataDirectory();
        List<string> before = SnapshotFiles(data);
        string installOptions = Path.Combine(data, "d", "options.cfg");
        byte[] installBytes = File.ReadAllBytes(installOptions);
        string root = Path.Combine(Path.GetTempPath(), "netstorm-dn-readonly-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        Action unfreeze = Freeze(data);
        try
        {
            string userOptions = Path.Combine(root, "Saves", "options.cfg");
            var startup = new HostStartupScene(data, "", null, userOptions);
            string installValue = startup.Configuration.Configuration.Get("soundVolume") ?? "";
            startup.Configuration.Set("soundVolume", installValue == "5" ? "4" : "5");
            startup.SaveOptions();
            Assert.True(File.Exists(userOptions));
            var reopened = new HostStartupScene(data, "", null, userOptions);
            Assert.Equal(startup.Configuration.Configuration.Get("soundVolume"), reopened.Configuration.Configuration.Get("soundVolume"));
        }
        finally
        {
            unfreeze();
            DeleteRoot(root);
        }

        Assert.Equal(before, SnapshotFiles(data));
        Assert.Equal(installBytes, File.ReadAllBytes(installOptions));
    }

    /// <summary>읽기 전용 설치에서도 Edit 목록·읽기·신규 저장이 사용자 영역에서 동작한다.</summary>
    [Fact]
    public void ReadOnlyInstallKeepsEditingRunningInUserRoot()
    {
        string data = DataDirectory();
        List<string> before = SnapshotFiles(data);
        string root = Path.Combine(Path.GetTempPath(), "netstorm-dn-readonly-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        Action unfreeze = Freeze(data);
        try
        {
            var customMaps = Path.Combine(root, "CustomMaps");
            using var workspace = new EditorCatalogWorkspace(new GameFileSystem(data, []), customMaps);
            var listed = workspace.Enumerate("d/*.fort");
            Assert.NotEmpty(listed);
            string first = listed[0];
            byte[]? content = workspace.Read(first);
            Assert.NotNull(content);
            string saved = workspace.Files.Prepare("ReadOnlyNew", [0x46, 0, 9, 0, 7, 7, 7, 7]);
            Assert.StartsWith(Path.Combine(customMaps), saved, StringComparison.OrdinalIgnoreCase);
            Assert.Contains(workspace.Enumerate("d/*.fort"), path => path.EndsWith("ReadOnlyNew.fort", StringComparison.OrdinalIgnoreCase));
            workspace.Files.Delete("d/ReadOnlyNew.fort");
        }
        finally
        {
            unfreeze();
            DeleteRoot(root);
        }

        Assert.Equal(before, SnapshotFiles(data));
    }
}
