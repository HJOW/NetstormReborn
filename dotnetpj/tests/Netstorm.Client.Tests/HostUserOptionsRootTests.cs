using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

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

    /// <summary>실제 시작 자산의 최소 읽기 사본과 이 검사만의 설치/사용자 옵션을 소유한다.</summary>
    private sealed class OverlayInstall : IDisposable
    {
        /// <summary>검사가 직접 만든 절대 임시 루트다.</summary>
        private readonly string _root = TemporaryUserRoot();
        /// <summary>원본과 별개로 복사한 읽기 전용 입력의 설치 루트다.</summary>
        public string Data { get; }
        /// <summary>실제 사용자 홈과 별개인 이 검사의 저장 경로다.</summary>
        public string UserOptions { get; }
        /// <summary>저장 전후 불변을 확인할 독립 설치 옵션 경로다.</summary>
        public string InstallOptions => Path.Combine(Data, "d", "options.cfg");
        /// <summary>설치 옵션의 처음 실제 바이트다.</summary>
        public byte[] InstallBytes { get; }

        /// <summary>기본 키 누락과 oracle처럼 명시3인 두 입력을 실제 설치 파일로 준비한다.</summary>
        public OverlayInstall(bool explicitVolume)
        {
            Data = Path.Combine(_root, "Install");
            UserOptions = Path.Combine(_root, "Saves", "options.cfg");
            var source = new HostStartupScene(DataDirectory());
            Copy(source.Files, "netstorm.tarc");
            Copy(source.Files, "d/setup.cfg");
            Copy(source.Files, source.Configuration.PathSpec("GamePalSpec", source.Configuration.PathSpec("fortPal")));
            Copy(source.Files, source.Configuration.PathSpec("BackgroundGifSpec", "titleMenu"));
            string text = "mQdsT\r\n// independent install comment\r\n\r\n"
                + (explicitVolume ? "soundVolume = \"3\"\r\nSOUNDVOLUME = \"1\"\r\n" : "")
                + "musicVolume = \"2\"\r\noverlayKeep = \"install\"\r\noverlayHidden = \"install-hidden\"\r\n"
                + "overlayEmpty = \"install-empty\"\r\noverlayEscaped = \"install-escaped\"\r\n"
                + "overlayAlias = \"{overlayKeep}\"\r\noverlayReference = \"install-reference\"\r\n";
            InstallBytes = HostOptionsFile.Encode(text);
            File.WriteAllBytes(InstallOptions, InstallBytes);
        }

        /// <summary>원본 자료를 읽기만 하고 정규화한 자기 설치 루트 안으로만 복사한다.</summary>
        private void Copy(GameFileSystem source, string name)
        {
            string destination = Path.GetFullPath(Path.Combine(Data, GameFileSystem.NormalizePath(name)));
            Assert.StartsWith(Data + Path.DirectorySeparatorChar, destination, StringComparison.OrdinalIgnoreCase);
            Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
            File.WriteAllBytes(destination, source.ReadAllBytes(name));
        }

        /// <summary>원시 설정 문법을 같은 실제 저장 인코더로 이 검사 사용자 파일에만 쓴다.</summary>
        public void WriteUser(string text) => WriteUser(HostOptionsFile.Encode(text));
        /// <summary>손상/NUL 경계 입력도 직접 만든 사용자 파일에만 쓴다.</summary>
        public void WriteUser(byte[] bytes)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(UserOptions)!);
            File.WriteAllBytes(UserOptions, bytes);
        }
        /// <summary>직접 만든 고유 임시 루트만 반납한다.</summary>
        public void Dispose() => Directory.Delete(_root, true);
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

    /// <summary>설치 키 누락/명시3 양쪽에서 실제 사용자5 저장·재개·재저장이 설치 중복보다 우선한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void SavedUserValueOverridesExplicitInstallDuplicatesAndSurvivesAnotherSave(bool explicitVolume)
    {
        using var files = new OverlayInstall(explicitVolume);
        var first = new HostStartupScene(files.Data, "", null, files.UserOptions);
        Assert.Equal(explicitVolume ? "3" : null, first.Configuration.Configuration.Get("soundVolume"));
        first.Configuration.Set("soundVolume", "5");
        first.SaveOptions();
        int firstLength = File.ReadAllBytes(files.UserOptions).Length;
        var second = new HostStartupScene(files.Data, "", null, files.UserOptions);
        Assert.Equal("5", second.Configuration.Configuration.Get("soundVolume"));
        Assert.Equal("2", second.Configuration.Configuration.Get("musicVolume"));
        Assert.Equal("install", second.Configuration.Configuration.Get("overlayKeep"));
        second.SaveOptions();
        byte[] secondBytes = File.ReadAllBytes(files.UserOptions);
        Assert.Equal(firstLength, secondBytes.Length);
        var third = new HostStartupScene(files.Data, "", null, files.UserOptions);
        Assert.Equal("5", third.Configuration.Configuration.Get("soundVolume"));
        third.SaveOptions();
        Assert.Equal(secondBytes, File.ReadAllBytes(files.UserOptions));
        Assert.Equal(files.InstallBytes, File.ReadAllBytes(files.InstallOptions));
    }

    /// <summary>원문에서 options.cfg보다 앞선 명령 인자/user.cfg는 사용자5·설치3보다 우선하며 사용자 저장에는 섞이지 않는다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void EarlierCommandOrUserConfigKeepsPriorityOverUserOptions(bool commandLine)
    {
        using var files = new OverlayInstall(true);
        files.WriteUser("mQdsT\r\nsoundVolume = \"5\"\r\n");
        if (!commandLine)
            File.WriteAllBytes(Path.Combine(files.Data, "d", "user.cfg"), HostOptionsFile.Encode("mQdsT\r\nsoundVolume = \"1\"\r\n"));
        var startup = new HostStartupScene(files.Data, commandLine ? "soundVolume=1;" : "", null, files.UserOptions);
        Assert.Equal("1", startup.Configuration.Configuration.Get("soundVolume"));
        startup.SaveOptions();
        if (!commandLine) File.Delete(Path.Combine(files.Data, "d", "user.cfg"));
        var reopened = new HostStartupScene(files.Data, "", null, files.UserOptions);
        Assert.Equal("5", reopened.Configuration.Configuration.Get("soundVolume"));
        Assert.Equal(files.InstallBytes, File.ReadAllBytes(files.InstallOptions));
    }

    /// <summary>첫 사용자 중복·빈값·백틱 따옴표·주석·치환·Unicode와 원문 서명의 첫줄 가림을 함께 보존한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void OverlayPreservesRawDeclarationsAndFirstUserDuplicate(bool explicitVolume)
    {
        using var files = new OverlayInstall(explicitVolume);
        files.WriteUser("mQdsToverlayHidden = \"hidden-user\"\r\n"
            + "// overlayKeep = \"comment\"\r\noverlayKeep without equals\r\n"
            + "SOUNDVOLUME = \"5\"//first user value\r\nsoundVolume = \"4\"\r\n"
            + "overlayEmpty = \"\"\r\noverlayEscaped = \"a`\"b//c\"\r\n"
            + "overlayAlias = \"{overlayReference}\"\r\noverlayReference = \"user-reference\"\r\n"
            + "overlayUnicode = \"한글\"\r\n");
        var startup = new HostStartupScene(files.Data, "", null, files.UserOptions);
        Config config = startup.Configuration.Configuration;
        Assert.Equal("5", config.Get("soundVolume"));
        Assert.Equal("", config.GetRaw("overlayEmpty"));
        Assert.Equal("", config.Get("overlayEmpty"));
        Assert.Equal("a`\"b//c", config.GetRaw("overlayEscaped"));
        Assert.Equal("a\"b//c", config.Get("overlayEscaped"));
        Assert.Equal("{overlayReference}", config.GetRaw("overlayAlias"));
        Assert.Equal("user-reference", config.Get("overlayAlias"));
        Assert.Equal("한글", config.Get("overlayUnicode"));
        Assert.Equal("install", config.Get("overlayKeep"));
        Assert.Equal("install-hidden", config.Get("overlayHidden"));
        startup.SaveOptions();
        var reopened = new HostStartupScene(files.Data, "", null, files.UserOptions);
        Assert.Equal("5", reopened.Configuration.Configuration.Get("soundVolume"));
        Assert.Equal("a`\"b//c", reopened.Configuration.Configuration.GetRaw("overlayEscaped"));
        Assert.Equal("{overlayReference}", reopened.Configuration.Configuration.GetRaw("overlayAlias"));
        Assert.Equal(files.InstallBytes, File.ReadAllBytes(files.InstallOptions));
    }

    /// <summary>이전 설치 경로가 사용자 END에 저장되어도 현재 설치/CD 루트와 실제 자료 소유자는 바뀌지 않는다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void SavedUserEndCannotReplaceCurrentInstallAndCdRoots(bool explicitVolume)
    {
        using var files = new OverlayInstall(explicitVolume);
        files.WriteUser("mQdsT\r\n[END]\r\nInstallDir = \"X:\\old-install\"\r\nCDDir = \"Y:\\old-cd\"\r\nsoundVolume = \"5\"\r\n");
        var startup = new HostStartupScene(files.Data, "", null, files.UserOptions);
        Assert.Equal(files.Data, startup.Configuration.Configuration.Get("InstallDir"));
        Assert.Equal(files.Data, startup.Configuration.Configuration.Get("CDDir"));
        Assert.Equal(files.Data, startup.Files.BaseDirectory);
        Assert.Equal("5", startup.Configuration.Configuration.Get("soundVolume"));
        startup.SaveOptions();
        var reopened = new HostStartupScene(files.Data, "", null, files.UserOptions);
        Assert.Equal(files.Data, reopened.Configuration.Configuration.Get("InstallDir"));
        Assert.Equal(files.Data, reopened.Configuration.Configuration.Get("CDDir"));
        Assert.Equal(files.InstallBytes, File.ReadAllBytes(files.InstallOptions));
    }

    /// <summary>명시 UTF-8 본문 뒤 손상이 있으면 앞의 유효 사용자 선언도 적용하지 않고 설치 값을 전부 보존한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void InvalidUtf8UserBodyPreservesInstallBeforeAnyMutation(bool explicitVolume)
    {
        using var files = new OverlayInstall(explicitVolume);
        byte[] bytes = [.. Encoding.ASCII.GetBytes("mQdsT"), 0xef, 0xbb, 0xbf,
            .. Encoding.UTF8.GetBytes("\r\nsoundVolume = \"5\"\r\noverlayKeep = \"user\"\r\n"), 0xc3, 0x28];
        XorCipher.Apply(bytes);
        files.WriteUser(bytes);
        var baseline = new HostStartupScene(files.Data);
        var startup = new HostStartupScene(files.Data, "", null, files.UserOptions);
        Assert.Equal(baseline.Configuration.Configuration.Text, startup.Configuration.Configuration.Text);
        Assert.Equal(explicitVolume ? "3" : null, startup.Configuration.Configuration.Get("soundVolume"));
        Assert.Equal("install", startup.Configuration.Configuration.Get("overlayKeep"));
        Assert.Equal(bytes, File.ReadAllBytes(files.UserOptions));
        Assert.Equal(files.InstallBytes, File.ReadAllBytes(files.InstallOptions));
    }

    /// <summary>원문처럼 첫NUL 뒤의 잘못된 UTF-8은 무시하고 그 앞의 유효 사용자 선언만 적용한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void FirstNulIgnoresMalformedUtf8TailWithoutLosingValidUserPrefix(bool explicitVolume)
    {
        using var files = new OverlayInstall(explicitVolume);
        byte[] bytes = [.. Encoding.ASCII.GetBytes("mQdsT"), 0xef, 0xbb, 0xbf,
            .. Encoding.UTF8.GetBytes("\r\nsoundVolume = \"5\"\r\noverlayUnicode = \"한글\"\r\n"), 0, 0xc3, 0x28,
            .. Encoding.ASCII.GetBytes("\r\noverlayKeep = \"ignored-tail\"\r\n")];
        XorCipher.Apply(bytes);
        files.WriteUser(bytes);
        var startup = new HostStartupScene(files.Data, "", null, files.UserOptions);
        Assert.Equal("5", startup.Configuration.Configuration.Get("soundVolume"));
        Assert.Equal("한글", startup.Configuration.Configuration.Get("overlayUnicode"));
        Assert.Equal("install", startup.Configuration.Configuration.Get("overlayKeep"));
        Assert.Equal(files.InstallBytes, File.ReadAllBytes(files.InstallOptions));
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
