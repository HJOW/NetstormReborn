using System.Diagnostics;
using System.Text.Json;
using Netstorm.Assets;

namespace Netstorm.AnalyzeManager.Tests;

/// <summary>추적 중인 원본의 보존과 AI 입력/증거의 경계 조건을 검사한다.</summary>
public sealed class SessionStoreTests : IDisposable
{
    private readonly string _root = Path.Combine(Path.GetTempPath(), "NetstormAnalyzeTests", Guid.NewGuid().ToString("N"));

    /// <summary>실행하지 않을 작은 가짜 원본과 실제 포맷의 설정 파일을 만든다.</summary>
    public SessionStoreTests()
    {
        string data = Path.Combine(_root, "originals", "d");
        Directory.CreateDirectory(data);
        File.WriteAllBytes(Path.Combine(_root, "originals", "Netstorm.exe"), [0x4d, 0x5a, 1]);
        var config = new ConfigFile("InstallDir = \"C:\\old-game\"\nInstallDir = \"C:\\duplicate\"\nstartInFullScreen = 1\nSCREENW=800\nSCREENH=600\n");
        File.WriteAllBytes(Path.Combine(data, "options.cfg"), config.Encode());
        File.WriteAllBytes(Path.Combine(data, "setup.cfg"), config.Encode());
    }

    /// <summary>복사 전 원본 설정을 보존하고 복사본만 창 모드로 바꾼다.</summary>
    [Fact]
    public async Task CopyPreservesOriginalAndRedirectsWrites()
    {
        var store = new SessionStore(_root);
        string original = Path.Combine(_root, "originals", "d", "options.cfg");
        byte[] initial = File.ReadAllBytes(original);
        AnalysisSession session = await store.CreateAsync("설정 격리", CancellationToken.None);
        ConfigFile copied = ConfigFile.Load(Path.Combine(store.SessionDirectory(session.Id), "game", "d", "options.cfg"));
        Assert.Equal(initial, File.ReadAllBytes(original));
        Assert.Equal("0", copied.Get("startInFullScreen"));
        Assert.Equal("1024", copied.Get("SCREENW"));
        Assert.Equal(Path.GetDirectoryName(store.GamePath(session)), copied.Get("InstallDir"));
        Assert.NotEmpty(session.ExeSha256);
        Assert.Equal(session.Id, store.Load(session.Id).Id);
    }

    /// <summary>세션 ID로 임의 경로에 접근하는 요청을 거부한다.</summary>
    [Theory]
    [InlineData("../originals")]
    [InlineData("C:\\Windows")]
    [InlineData("20260929T120000000Z-123456789abc/../..")]
    public void RejectsSessionTraversal(string id) => Assert.Throws<ArgumentException>(() => new SessionStore(_root).SessionDirectory(id));

    /// <summary>한 세션의 잠금이 다른 CLI/MCP 인스턴스에도 적용되는지 확인한다.</summary>
    [Fact]
    public void RejectsConcurrentDesktopOwners()
    {
        var first = new SessionStore(_root);
        var second = new SessionStore(_root);
        using FileStream locked = first.LockDesktop();
        Assert.Throws<IOException>(() => second.LockDesktop());
    }

    /// <summary>UTF-8 크기에 따라 파일을 나누어도 모든 이벤트와 순서가 남는다.</summary>
    [Fact]
    public async Task JournalRotatesAndContinuesAfterReload()
    {
        var store = new SessionStore(_root, 1600);
        AnalysisSession session = await store.CreateAsync("기록 분할", CancellationToken.None);
        // 실제 한글 메모를 사용해 문자 개수가 아니라 바이트 한도로 분할되는지 확인한다.
        for (int i = 0; i < 8; i++) store.Append(session, "observation", new { note = new string('한', 120), step = i });
        session = store.Load(session.Id);
        store.Append(session, "continued", new { step = 8 });
        string directory = store.SessionDirectory(session.Id);
        string[] files = Directory.GetFiles(directory, "events-*.jsonl").Order().ToArray();
        Assert.True(files.Length > 1);
        Assert.All(files, file => Assert.InRange(new FileInfo(file).Length, 1, 1600));
        int[] numbers = files.SelectMany(File.ReadAllLines).Select(line => JsonDocument.Parse(line).RootElement.GetProperty("number").GetInt32()).ToArray();
        Assert.Equal(Enumerable.Range(1, 9), numbers);
        Assert.True(File.Exists(Path.Combine(directory, "report.md")));
    }

    /// <summary>이미지 중복 제거가 관찰 시각이나 이벤트 자체를 없애지 않는지 검사한다.</summary>
    [Fact]
    public async Task DeduplicatesImagesButKeepsBothObservations()
    {
        var store = new SessionStore(_root);
        AnalysisSession session = await store.CreateAsync("중복", CancellationToken.None);
        var window = new GameWindow(1, "fixture", 0, 0, 1, 1, true);
        var frame = new CapturedFrame([1, 2, 3], [10, 20, 30], new(0, 0, 1, 1), window);
        ScreenshotEvidence first = store.StoreFrame(session, frame);
        ScreenshotEvidence second = store.StoreFrame(session, frame);
        store.Append(session, "first", new { }, first);
        store.Append(session, "second", new { }, second);
        Assert.False(first.Reused);
        Assert.True(second.Reused);
        Assert.Equal(first.Path, second.Path);
        Assert.Single(Directory.GetFiles(Path.Combine(store.SessionDirectory(session.Id), "screens")));
        Assert.Equal(2, store.Load(session.Id).EventCount);
    }

    /// <summary>손상된 해시 이름의 PNG를 정상 증거로 재사용하지 않는지 확인한다.</summary>
    [Fact]
    public async Task RejectsCorruptedStoredImage()
    {
        var store = new SessionStore(_root);
        AnalysisSession session = await store.CreateAsync("손상 검출", CancellationToken.None);
        var window = new GameWindow(1, "fixture", 0, 0, 1, 1, true);
        var frame = new CapturedFrame([1, 2, 3], [10, 20, 30], new(0, 0, 1, 1), window);
        ScreenshotEvidence evidence = store.StoreFrame(session, frame);
        string path = Path.Combine(store.SessionDirectory(session.Id), evidence.Path);
        File.WriteAllBytes(path, [1, 2, 4]);
        Assert.Throws<InvalidDataException>(() => store.StoreFrame(session, frame));
        File.WriteAllBytes(path, [1, 2]);
        Assert.Throws<InvalidDataException>(() => store.StoreFrame(session, frame));
        Assert.Equal(new byte[] { 1, 2 }, File.ReadAllBytes(path));
    }

    /// <summary>OS가 재사용한 PID나 다른 프로그램을 게임으로 취급하지 않는다.</summary>
    [Fact]
    public async Task RejectsUnownedProcessEvenWithMatchingPidAndStartTime()
    {
        var store = new SessionStore(_root);
        AnalysisSession session = await store.CreateAsync("PID 경계", CancellationToken.None);
        using Process process = Process.GetCurrentProcess();
        session.ProcessId = process.Id;
        session.ProcessStartedUtcTicks = process.StartTime.ToUniversalTime().Ticks;
        Assert.Null(store.OwnedProcess(session));
    }

    /// <summary>영역 밖과 정수 덧셈 범위를 넘는 ROI를 캡처 전에 거부한다.</summary>
    [Theory]
    [InlineData("-1,0,10,10")]
    [InlineData("0,0,0,10")]
    [InlineData("1000,0,50,10")]
    [InlineData("2147483647,0,2147483647,10")]
    public void RejectsInvalidRegions(string region) => Assert.Throws<ArgumentException>(() =>
        CaptureRegion.Parse(region, new GameWindow(1, "fixture", 0, 0, 1024, 768, true)));

    /// <summary>색상 잡음과 실제 변화 픽셀을 구분해 ROI 조건을 판정한다.</summary>
    [Fact]
    public void ComputesChangedPixelFraction()
    {
        var window = new GameWindow(1, "fixture", 0, 0, 2, 1, true);
        var first = new CapturedFrame([], [0, 0, 0, 0, 0, 0], new(0, 0, 2, 1), window);
        var second = first with { Pixels = [23, 0, 0, 24, 0, 0] };
        Assert.Equal(0.5, first.Difference(second));
        Assert.Equal(1, first.Difference(second with { Region = new(0, 1, 2, 1) }));
    }

    /// <summary>지침에서 금지한 전체화면 전환과 지원하지 않는 키 입력을 검사한다.</summary>
    [Theory]
    [InlineData("ALT+ENTER")]
    [InlineData("F11")]
    [InlineData("WIN")]
    [InlineData("")]
    public void RejectsUnsupportedKeys(string keys) => Assert.Throws<ArgumentException>(() => WindowsGame.ParseKeys(keys));

    /// <summary>오류가 발생해도 자신이 만든 임시 테스트 디렉토리만 정리한다.</summary>
    public void Dispose()
    {
        string resolved = Path.GetFullPath(_root);
        string expected = Path.Combine(Path.GetTempPath(), "NetstormAnalyzeTests") + Path.DirectorySeparatorChar;
        if (!resolved.StartsWith(expected, StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException("테스트 경로 이탈.");
        if (Directory.Exists(resolved)) Directory.Delete(resolved, true);
    }
}
