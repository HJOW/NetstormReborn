namespace Netstorm.Assets.Tests;

/// <summary>분석용 원본이 없어도 배포 데이터와 클론 소스를 찾는지 검사한다.</summary>
public sealed class GameDataLocatorTests
{
    /// <summary>배포 폴더의 데이터를 사용하며 상위 저장소·분석 원본의 데이터와 섞지 않는다.</summary>
    [Fact]
    public void PackagedData_WinsOverRepositoryAndAnalysisFiles()
    {
        using var directory = new DataDirectory();
        string packaged = directory.CreateData("publish/game-data");
        directory.CreateData("assets/game-data");
        directory.CreateData("originals");

        Assert.Equal(packaged, GameDataLocator.FindDataDirectory(
            Path.Combine(directory.Root, "publish"), directory.Root));
    }

    /// <summary>실행 파일 경로에 데이터가 없으면 작업 폴더의 상위에서 클론 소스를 찾는다.</summary>
    [Fact]
    public void RepositoryData_IsFoundFromWorkingDirectory()
    {
        using var directory = new DataDirectory();
        string source = directory.CreateData("project/assets/game-data");

        Assert.Equal(source, GameDataLocator.FindDataDirectory(
            Path.Combine(directory.Root, "external-host"),
            Path.Combine(directory.Root, "project/src/game")));
    }

    /// <summary>명시적으로 지정한 데이터는 기본 배포 데이터보다 우선하며 파일 대소문자를 무시한다.</summary>
    [Fact]
    public void ExplicitDataDirectory_WinsAndIgnoresMarkerCase()
    {
        using var directory = new DataDirectory();
        string custom = directory.CreateData("custom", "NETSTORM.TARC");
        directory.CreateData("game-data");

        Assert.Equal(custom, GameDataLocator.FindDataDirectory(directory.Root, directory.Root, custom));
    }

    /// <summary>분석용 originals/와 이름 없는 상위 폴더의 아카이브는 자동 실행 데이터로 선택하지 않는다.</summary>
    [Fact]
    public void AnalysisDirectory_IsNeverAnAutomaticFallback()
    {
        using var directory = new DataDirectory();
        directory.CreateData("originals");
        directory.CreateData("");

        Assert.Null(GameDataLocator.FindDataDirectory(directory.Root, directory.Root));
    }

    /// <summary>유효하지 않은 별도 경로가 남아 있어도 배포에 포함된 기본 데이터를 사용할 수 있다.</summary>
    [Fact]
    public void MissingOverride_FallsBackToPackagedData()
    {
        using var directory = new DataDirectory();
        string packaged = directory.CreateData("game-data");

        Assert.Equal(packaged, GameDataLocator.FindDataDirectory(
            directory.Root, directory.Root, Path.Combine(directory.Root, "missing")));
    }

    /// <summary>클론 데이터는 일반 파일이며 분석용 프로그램·DLL·서버 스크립트를 포함하지 않는다.</summary>
    [Fact]
    public void CloneData_ContainsNoOriginalProgramsOrSymbolicLinks()
    {
        string source = OriginalData.RequireDirectory();
        // 게임 데이터 전체에서 원본 프로그램이나 원본 폴더를 가리키는 링크가 섞이지 않았는지 검사한다.
        foreach (string path in Directory.EnumerateFiles(source, "*", SearchOption.AllDirectories))
        {
            Assert.DoesNotContain(Path.GetExtension(path).ToLowerInvariant(), new[] { ".exe", ".dll", ".php" });
            Assert.Null(new FileInfo(path).LinkTarget);
        }
    }

    /// <summary>실제 저장소·전역 환경 변수를 변경하지 않는 독립적인 임시 탐색 자료.</summary>
    private sealed class DataDirectory : IDisposable
    {
        /// <summary>검사 전용 임시 폴더.</summary>
        public string Root { get; } = Path.Combine(Path.GetTempPath(), "netstorm-data-" + Guid.NewGuid().ToString("N"));

        /// <summary>표지 파일만 가진 데이터 폴더를 만들고 절대 경로를 반환한다.</summary>
        public string CreateData(string relativePath, string markerName = "netstorm.tarc")
        {
            string path = Path.Combine(Root, relativePath);
            Directory.CreateDirectory(path);
            File.WriteAllBytes(Path.Combine(path, markerName), []);
            return Path.GetFullPath(path);
        }

        /// <summary>검사에서 만든 임시 폴더만 제거한다.</summary>
        public void Dispose() => Directory.Delete(Root, recursive: true);
    }
}
