using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text.Json;
using Netstorm.Assets;

namespace Netstorm.AnalyzeManager.Tests;

/// <summary>실제 게임 없이 FPS 설정·시간표·AVI·녹화 프로세스의 실패 경계를 검사한다.</summary>
public sealed class AnalysisFrameRateTests : IDisposable
{
    private readonly string _root = Path.Combine(Path.GetTempPath(), "NetstormFrameRateTests", Guid.NewGuid().ToString("N"));

    /// <summary>복사만 가능한 가짜 원본과 설정을 만들고 실행 파일을 시작하지 않는다.</summary>
    public AnalysisFrameRateTests()
    {
        string data = Path.Combine(_root, "originals", "d");
        Directory.CreateDirectory(data);
        File.WriteAllBytes(Path.Combine(_root, "originals", "Netstorm.exe"), [0x4d, 0x5a, 1]);
        byte[] config = new ConfigFile("InstallDir=\"fixture\"\nstartInFullScreen=1\nSCREENW=800\nSCREENH=600\n").Encode();
        File.WriteAllBytes(Path.Combine(data, "options.cfg"), config);
        File.WriteAllBytes(Path.Combine(data, "setup.cfg"), config);
    }

    /// <summary>30/60FPS에서 소수 밀리초를 누적해도 1초에 정확한 목표 시각 수를 유지한다.</summary>
    [Theory]
    [InlineData(30)]
    [InlineData(60)]
    public void ScheduleKeepsFractionalFrameIntervals(int fps)
    {
        var schedule = new FrameSchedule(fps);
        TimeSpan elapsed = TimeSpan.Zero;
        // 첫 1초의 목표 프레임 시각을 순서대로 계산한다.
        for (int i = 0; i < fps; i++) elapsed += schedule.DelayAfter(elapsed);
        Assert.Equal(TimeSpan.FromSeconds(1), elapsed);
    }

    /// <summary>느린 캡처 후에도 양수 대기 시간을 반환해 지나간 프레임을 몰아서 기록하지 않는다.</summary>
    [Theory]
    [InlineData(30)]
    [InlineData(60)]
    public void ScheduleSkipsMissedFrames(int fps)
    {
        var schedule = new FrameSchedule(fps);
        schedule.DelayAfter(TimeSpan.Zero);
        TimeSpan delay = schedule.DelayAfter(TimeSpan.FromMilliseconds(250));
        Assert.InRange(delay.Ticks, 1, TimeSpan.TicksPerSecond / fps + 1);
        Assert.Equal(2_666_667, (TimeSpan.FromMilliseconds(250) + delay).Ticks);
    }

    /// <summary>이미 사용하는 pollMs 매개변수가 새 FPS 기본값보다 우선할 수 있다.</summary>
    [Fact]
    public void ExplicitPollingIntervalRemainsSupported()
    {
        var schedule = new FrameSchedule(60, 200);
        Assert.Equal(TimeSpan.FromMilliseconds(200), schedule.DelayAfter(TimeSpan.Zero));
        Assert.Equal(TimeSpan.FromMilliseconds(50), schedule.DelayAfter(TimeSpan.FromMilliseconds(350)));
    }

    /// <summary>세션 값은 다시 읽어도 남고 기존 FPS 필드가 없는 세션은 30FPS로 열린다.</summary>
    [Fact]
    public async Task SessionPersistsFrameRateAndDefaultsLegacySessions()
    {
        var store = new SessionStore(_root);
        AnalysisSession session = await store.CreateAsync("60FPS 보존", CancellationToken.None, 60);
        Assert.Equal(60, store.Load(session.Id).Fps);
        using JsonDocument document = JsonDocument.Parse(File.ReadAllText(Path.Combine(store.SessionDirectory(session.Id), "session.json")));
        var legacy = document.RootElement.EnumerateObject().Where(property => property.Name != "fps")
            .ToDictionary(property => property.Name, property => property.Value.Clone());
        File.WriteAllText(Path.Combine(store.SessionDirectory(session.Id), "session.json"), JsonSerializer.Serialize(legacy));
        Assert.Equal(30, store.Load(session.Id).Fps);
        Assert.Null(new AnalysisRequest().Fps);
        Assert.Equal(0, new AnalysisRequest().PollMs);
    }

    /// <summary>잘못된 FPS의 실행 요청을 게임 복사·실행 전에 거부한다.</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(10)]
    [InlineData(45)]
    [InlineData(75)]
    public async Task InvalidFrameRateNeverPreparesOrStartsGame(int fps)
    {
        var engine = new AnalysisEngine(_root);
        AnalysisResult result = await engine.ExecuteAsync("start_session", new() { Fps = fps }, TestContext.Current.CancellationToken);
        Assert.True(result.IsError);
        Assert.Contains("30 또는 60", JsonSerializer.SerializeToElement(result.Data).GetProperty("error").GetString());
        Assert.Empty(engine.Store.Sessions());
    }

    /// <summary>AVI 재생 속도·메인 헤더 간격·분할 뒤 FPS와 과거 10FPS 읽기를 함께 검사한다.</summary>
    [Theory]
    [InlineData(10)]
    [InlineData(30)]
    [InlineData(60)]
    public void VideoHeadersAndRotatedSegmentsKeepFrameRate(int fps)
    {
        string directory = Path.Combine(_root, "video");
        using var bitmap = new Bitmap(10, 10);
        using var stream = new MemoryStream();
        bitmap.Save(stream, ImageFormat.Jpeg);
        byte[] jpeg = stream.ToArray();
        using (var video = new GuidedVideo(directory, 10, 10, fps, 1500))
        {
            // 작은 분할 한도로 여러 AVI 조각을 생성한다.
            for (int frame = 0; frame < 4; frame++) video.WriteFrame(jpeg, DateTimeOffset.UtcNow, frame * 1000d / fps);
        }
        string[] files = Directory.GetFiles(directory, "video-*.avi");
        Assert.True(files.Length > 1);
        // 모든 분할 파일에 같은 재생 간격·스트림 FPS가 있어야 한다.
        foreach (string file in files)
        {
            byte[] bytes = File.ReadAllBytes(file);
            Assert.Equal(fps, GuidedVideo.ReadFramesPerSecond(file));
            Assert.Equal(1_000_000 / fps, BitConverter.ToInt32(bytes, 32));
            Assert.Equal(1, BitConverter.ToInt32(bytes, 128));
            Assert.Equal(fps, BitConverter.ToInt32(bytes, 132));
        }
    }

    /// <summary>30→60FPS로 다시 녹화해도 기존 파일을 보존하고 색인에 각 파일의 실제 속도를 남긴다.</summary>
    [Fact]
    public async Task RecordingIndexDescribesMixedFrameRates()
    {
        var store = new SessionStore(_root);
        AnalysisSession session = await store.CreateAsync("혼합 FPS", CancellationToken.None);
        string directory = Path.Combine(store.SessionDirectory(session.Id), "recording");
        using var bitmap = new Bitmap(10, 10);
        using var jpeg = new MemoryStream();
        bitmap.Save(jpeg, ImageFormat.Jpeg);
        // 자동·수동 모드 전환과 같은 경로에 서로 다른 속도로 두 녹화 구간을 저장한다.
        foreach (int fps in new[] { 30, 60 })
        {
            using var video = new GuidedVideo(directory, 10, 10, fps);
            video.WriteFrame(jpeg.ToArray(), DateTimeOffset.UtcNow, fps);
        }
        using JsonDocument document = JsonDocument.Parse(File.ReadAllText(FreeplayRecordingIndex.Write(store, session.Id, false)));
        Assert.Equal(new[] { 30, 60 }, document.RootElement.GetProperty("frameRates").EnumerateArray().Select(item => item.GetInt32()));
        Assert.Equal(new[] { 30, 60 }, document.RootElement.GetProperty("videos").EnumerateArray().Select(item => item.GetProperty("fps").GetInt32()));
        Assert.Equal(2, Directory.GetFiles(directory, "video-*.avi").Length);
    }

    /// <summary>분석 마지막 화면은 재캡처하지 않고 판정에 사용한 BGR 픽셀·행 패딩으로 복원한다.</summary>
    [Fact]
    public void ComparisonFrameProducesIdenticalEvidencePixels()
    {
        var window = new GameWindow(1, "fixture", 0, 0, 3, 2, true);
        var frame = new CapturedFrame([], [0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 0, 0, 0, 20, 30, 40],
            new(0, 0, 3, 2), window, "wine-window-dc");
        CapturedFrame encoded = WindowsGame.EncodePng(frame);
        using var stream = new MemoryStream(encoded.Png);
        using var bitmap = new Bitmap(stream);
        Assert.Equal(Color.Red.ToArgb(), bitmap.GetPixel(0, 0).ToArgb());
        Assert.Equal(Color.Blue.ToArgb(), bitmap.GetPixel(2, 0).ToArgb());
        Assert.Equal(Color.White.ToArgb(), bitmap.GetPixel(0, 1).ToArgb());
        Assert.Equal(Color.FromArgb(40, 30, 20).ToArgb(), bitmap.GetPixel(2, 1).ToArgb());
        Assert.Equal(frame.Pixels, encoded.Pixels);
        Assert.Equal(frame.Method, encoded.Method);
    }

    /// <summary>독립 녹화 명령은 공백 경로를 인자로 보존하고 콘솔 창·프로토콜 핸들 상속을 막는다.</summary>
    [Fact]
    public void RecorderLaunchUsesHiddenProcessAndStructuredArguments()
    {
        ProcessStartInfo start = AutomaticRecording.CreateStartInfo("C:\\공백 있는 저장소", "20261003T120000000Z-123456789abc",
            Guid.NewGuid().ToString("N"), 60);
        Assert.False(start.UseShellExecute);
        Assert.True(start.CreateNoWindow);
        Assert.Equal(ProcessWindowStyle.Hidden, start.WindowStyle);
        Assert.True(start.RedirectStandardInput && start.RedirectStandardOutput && start.RedirectStandardError);
        Assert.Contains("C:\\공백 있는 저장소", start.ArgumentList);
        Assert.Equal("60", start.ArgumentList[^1]);
    }

    /// <summary>게임이 없는 녹화 프로세스는 오류 상태를 남기며 세션 이벤트 번호를 변경하지 않는다.</summary>
    [Fact]
    public async Task RecorderFailureDoesNotCorruptCliSessionJournal()
    {
        var store = new SessionStore(_root);
        AnalysisSession session = await store.CreateAsync("실행 없는 실패 검사", CancellationToken.None);
        var state = new AutomaticRecordingState { SessionId = session.Id, RunId = Guid.NewGuid().ToString("N"), Fps = 30 };
        string path = Path.Combine(store.SessionDirectory(session.Id), "automatic-recording.json");
        File.WriteAllText(path, JsonSerializer.Serialize(state, SessionStore.Json));
        Assert.Equal(2, await AutomaticRecording.RunAsync(store, session.Id, state.RunId, 30));
        AutomaticRecordingState? stopped = await AutomaticRecording.StopAsync(store, session.Id, TestContext.Current.CancellationToken);
        Assert.Equal("failed", stopped!.State);
        Assert.Equal(0, stopped.Frames);
        Assert.NotNull(stopped.Error);
        Assert.Equal(0, store.Load(session.Id).EventCount);
    }

    /// <summary>다른 프로세스가 상태 파일을 잠시 독점해 교체가 거부돼도 저장을 재시도해 결국 최신 내용으로 바꾼다.</summary>
    [Fact]
    public async Task StateSaveRetriesWhileAnotherHandleBlocksReplacement()
    {
        var store = new SessionStore(_root);
        AnalysisSession session = await store.CreateAsync("상태 파일 충돌 검사", CancellationToken.None);
        string path = Path.Combine(store.SessionDirectory(session.Id), "automatic-recording.json");
        var first = new AutomaticRecordingState { SessionId = session.Id, RunId = Guid.NewGuid().ToString("N"), Fps = 30 };
        AutomaticRecording.SaveState(store, first);
        // 삭제 공유 없이 열어 두어 File.Move 교체를 거부시키고 약 150ms 뒤에 놓는다.
        var blocker = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        Task release = Task.Run(async () =>
        {
            await Task.Delay(150, TestContext.Current.CancellationToken);
            blocker.Dispose();
        }, TestContext.Current.CancellationToken);
        AutomaticRecording.SaveState(store, first with { State = "recording", Frames = 7 });
        await release;
        AutomaticRecordingState saved = JsonSerializer.Deserialize<AutomaticRecordingState>(File.ReadAllBytes(path), SessionStore.Json)!;
        Assert.Equal("recording", saved.State);
        Assert.Equal(7, saved.Frames);
    }

    /// <summary>자신이 만든 임시 폴더의 절대 경로를 확인한 뒤 테스트 파일을 정리한다.</summary>
    public void Dispose()
    {
        string resolved = Path.GetFullPath(_root);
        string expected = Path.Combine(Path.GetTempPath(), "NetstormFrameRateTests") + Path.DirectorySeparatorChar;
        if (!resolved.StartsWith(expected, StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException("테스트 경로 이탈.");
        if (Directory.Exists(resolved)) Directory.Delete(resolved, true);
    }
}
