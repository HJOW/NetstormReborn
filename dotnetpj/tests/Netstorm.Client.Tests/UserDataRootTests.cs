using Netstorm.Client.Host;

namespace Netstorm.Client.Tests;

/// <summary>
/// 사용자 데이터 루트 계산·하위 폴더·생성 보장을 검사한다
/// (cpppj <c>platform/UserDataRoot</c>의 계약을 직접 작성했다).
/// </summary>
/// <remarks>2026-11-11 0.359에서 5차 경로 이식과 함께 추가했다. 실제 사용자 홈을 건드리지 않고 재정의·임시 폴더만 쓴다.</remarks>
public sealed class UserDataRootTests
{
    /// <summary>환경 변수를 잠시 바꾸고 반드시 되돌린다.</summary>
    /// <param name="name">변수 이름이다.</param><param name="value">임시 값이다.</param><param name="body">실행할 검사다.</param>
    private static void WithEnvironment(string name, string? value, Action body)
    {
        string? saved = Environment.GetEnvironmentVariable(name);
        Environment.SetEnvironmentVariable(name, value);
        try
        {
            body();
        }
        finally
        {
            Environment.SetEnvironmentVariable(name, saved);
        }
    }

    /// <summary>하위 폴더·파일 이름이 요구 사양과 같다.</summary>
    [Fact]
    public void FolderAndFileNamesMatchSpec()
    {
        var root = new UserDataRoot(Path.Combine("x", "Netstorm"));
        Assert.Equal(Path.GetFullPath(Path.Combine("x", "Netstorm")), root.Root);
        Assert.Equal(Path.Combine(root.Root, "Saves"), root.Saves());
        Assert.Equal(Path.Combine(root.Root, "CustomMaps"), root.CustomMaps());
        Assert.Equal(Path.Combine(root.Root, "Editor"), root.Editor());
        Assert.Equal(Path.Combine(root.Root, "Captures"), root.Captures());
        Assert.Equal(Path.Combine(root.Root, "Cache"), root.Cache());
        Assert.Equal(Path.Combine(root.Root, "Diagnostics"), root.Diagnostics());
        Assert.Equal(Path.Combine(root.Root, "logs"), root.Logs());
        Assert.Equal(Path.Combine(root.Saves(), "options.cfg"), root.OptionsFile());
        Assert.Equal(Path.Combine(root.Saves(), "!state.dat"), root.SavedStateFile());
        Assert.Equal(Path.Combine(root.Saves(), "fullscreenStateFile.dat"), root.FullScreenStateFile());
    }

    /// <summary>환경 변수 재정의가 실제 홈보다 먼저 적용된다.</summary>
    [Fact]
    public void OverrideVariableWinsOverHome()
    {
        string temp = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());
        WithEnvironment(UserDataRoot.OverrideVariable, temp, () =>
        {
            UserDataRoot resolved = UserDataRoot.Resolve();
            Assert.Equal(Path.GetFullPath(temp), resolved.Root);
        });
        Assert.Null(Environment.GetEnvironmentVariable(UserDataRoot.OverrideVariable + "_Missing"));
    }

    /// <summary>없는 폴더를 상위까지 만들고 같은 이름의 파일이 있으면 거부한다.</summary>
    [Fact]
    public void EnsureDirectoryCreatesParentsAndRejectsFile()
    {
        string temp = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());
        try
        {
            string nested = Path.Combine(temp, "a", "logs");
            UserDataRoot.EnsureDirectory(nested);
            Assert.True(Directory.Exists(nested));
            UserDataRoot.EnsureDirectory(nested);
            string file = Path.Combine(temp, "blocked");
            File.WriteAllText(file, "x");
            Assert.Throws<IOException>(() => UserDataRoot.EnsureDirectory(Path.Combine(file, "child")));
        }
        finally
        {
            if (Directory.Exists(temp)) Directory.Delete(temp, true);
        }
    }
}
