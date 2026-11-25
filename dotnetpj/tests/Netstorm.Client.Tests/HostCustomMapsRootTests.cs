using Netstorm.Assets;
using Netstorm.Client.Host;

// 커스텀 맵의 설치·사용자 양쪽 읽기와 신규 저장을 사용자 CustomMaps 루트에서 검증한다.
// 2026-11-26 1.042에서 WP-5b 커스텀 맵 읽기·저장용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>설치 원본을 건드리지 않고 사용자 CustomMaps와 합친 목록·읽기·저장을 대조한다.</summary>
public sealed class HostCustomMapsRootTests
{
    /// <summary>사용자 CustomMaps를 작업 루트로 쓰는 편집 작업본 장치를 연다.</summary>
    private static EditorCatalogWorkspace Open(EditorTempRoot temp)
    {
        Directory.CreateDirectory(temp.Source);
        return new EditorCatalogWorkspace(new GameFileSystem(temp.Source, []), Path.Combine(temp.Root, "CustomMaps"));
    }

    /// <summary>사용자 CustomMaps 안에 검사용 파일을 둔다.</summary>
    private static void SeedUser(EditorTempRoot temp, string relative, byte[] bytes)
    {
        string path = Path.Combine(temp.Root, "CustomMaps", relative.Replace('/', Path.DirectorySeparatorChar));
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        File.WriteAllBytes(path, bytes);
    }

    /// <summary>설치 맵과 사용자 맵이 함께 목록에 오르고 설치본은 복사만 될 뿐 바뀌지 않는다.</summary>
    [Fact]
    public void CustomMapsListsInstallAndUserMaps()
    {
        using var temp = new EditorTempRoot();
        byte[] installed = [0x46, 0, 9, 0, 1, 2, 3, 4];
        byte[] authored = [0x46, 0, 9, 0, 9, 9, 9, 9];
        temp.Seed("d/Alpha.fort", installed);
        SeedUser(temp, "d/Beta.fort", authored);
        using var workspace = Open(temp);
        var listed = workspace.Enumerate("d/*.fort");
        Assert.Equal(2, listed.Count);
        Assert.Contains(listed, path => path.EndsWith("Alpha.fort", StringComparison.OrdinalIgnoreCase));
        Assert.Contains(listed, path => path.EndsWith("Beta.fort", StringComparison.OrdinalIgnoreCase));
        // 설치 원본은 바이트까지 그대로고, 사용자 작성본은 덮어쓰지 않는다.
        Assert.Equal(installed, File.ReadAllBytes(Path.Combine(temp.Source, "d", "Alpha.fort")));
        Assert.Equal(authored, File.ReadAllBytes(Path.Combine(temp.Root, "CustomMaps", "d", "Beta.fort")));
        Assert.Equal([.. installed], workspace.Read("d/Alpha.fort") ?? throw new InvalidOperationException("이관 맵 읽기 누락"));
        Assert.Equal(authored, workspace.Read("d/Beta.fort") ?? throw new InvalidOperationException("사용자 맵 읽기 누락"));
        Assert.False(File.Exists(Path.Combine(temp.Source, "d", "Beta.fort")));
    }

    /// <summary>새 맵은 사용자 CustomMaps에 저장되고 설치 폴더에는 생기지 않는다.</summary>
    [Fact]
    public void NewMapsAreSavedInCustomMaps()
    {
        using var temp = new EditorTempRoot();
        Directory.CreateDirectory(temp.Source);
        byte[] image = [0x46, 0, 9, 0, 7, 7, 7, 7];
        using var workspace = Open(temp);
        string saved = workspace.Files.Prepare("Gamma", image);
        Assert.StartsWith(Path.Combine(temp.Root, "CustomMaps"), saved, StringComparison.OrdinalIgnoreCase);
        Assert.Equal(image, File.ReadAllBytes(saved));
        Assert.False(File.Exists(Path.Combine(temp.Source, "d", "Gamma.fort")));
        Assert.Contains(workspace.Enumerate("d/*.fort"), path => path.EndsWith("Gamma.fort", StringComparison.OrdinalIgnoreCase));
    }

    /// <summary>같은 이름은 사용자 작성본이 이기고 원본 복사 없이 완료로 기록된다.</summary>
    [Fact]
    public void UserMapShadowsInstallWithoutCopying()
    {
        using var temp = new EditorTempRoot();
        byte[] installed = [0x46, 0, 9, 0, 1, 1, 1, 1];
        byte[] authored = [0x46, 0, 9, 0, 2, 2, 2, 2];
        temp.Seed("d/Same.fort", installed);
        SeedUser(temp, "d/Same.fort", authored);
        using var workspace = Open(temp);
        var listed = workspace.Enumerate("d/*.fort");
        Assert.Single(listed);
        Assert.Equal(authored, workspace.Read("d/Same.fort"));
        Assert.Equal(installed, File.ReadAllBytes(Path.Combine(temp.Source, "d", "Same.fort")));
        Assert.Equal(authored, File.ReadAllBytes(Path.Combine(temp.Root, "CustomMaps", "d", "Same.fort")));
    }

    /// <summary>사용자 루트를 정할 수 없어도 기본 작업본 동작은 그대로 둔다.</summary>
    [Fact]
    public void DefaultWorkspaceWithoutUserRootStillWorks()
    {
        using var temp = new EditorTempRoot();
        temp.Seed("d/Solo.fort", [0x46, 0]);
        Directory.CreateDirectory(temp.Source);
        using var workspace = new EditorCatalogWorkspace(new GameFileSystem(temp.Source, []), temp.Work);
        Assert.Single(workspace.Enumerate("d/*.fort"));
        Assert.False(File.Exists(Path.Combine(temp.Root, "CustomMaps", "d", "Solo.fort")));
    }
}
