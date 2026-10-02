using System.Diagnostics;
using System.Text;
using System.Text.Json;

namespace Netstorm.AnalyzeManager;

/// <summary>CLI 재호출 사이에도 살아 있는 자동 녹화 프로세스와 출력 파일의 상태.</summary>
public sealed record AutomaticRecordingState
{
    public string SessionId { get; init; } = "";
    public string RunId { get; init; } = "";
    public int Fps { get; init; }
    public int ProcessId { get; init; }
    public long ProcessStartedUtcTicks { get; init; }
    public string ProcessPath { get; init; } = "";
    public string State { get; init; } = "starting";
    public int Frames { get; init; }
    public double ElapsedMs { get; init; }
    public string? Error { get; init; }
    public DateTimeOffset UpdatedUtc { get; init; } = DateTimeOffset.UtcNow;
}

/// <summary>자동 분석 세션의 녹화를 독립 프로세스로 유지하고 모드 전환·게임 종료 때 파일을 닫는다.</summary>
public static class AutomaticRecording
{
    /// <summary>오디오·입력 훅 초기화 또는 파일 마감을 기다리는 최대 시간.</summary>
    private static readonly TimeSpan TransitionTimeout = TimeSpan.FromSeconds(10);
    /// <summary>프레임 수·오류 상태 파일과 종료 신호를 확인하는 주기.</summary>
    private static readonly TimeSpan StatusInterval = TimeSpan.FromMilliseconds(200);

    /// <summary>dotnet으로 호출했더라도 배포된 apphost가 있으면 같은 실행 파일로 녹화 프로세스를 관리한다.</summary>
    private static string RecorderProcessPath()
    {
        string appHost = Path.Combine(Path.GetDirectoryName(typeof(AutomaticRecording).Assembly.Location)!, "Netstorm.AnalyzeManager.exe");
        return File.Exists(appHost) ? appHost
            : Environment.ProcessPath ?? throw new InvalidOperationException("분석기 실행 경로를 찾지 못했습니다.");
    }

    /// <summary>고정된 세션 폴더 안의 자동 녹화 상태 파일을 반환한다.</summary>
    private static string StatePath(SessionStore store, string sessionId) => Path.Combine(store.SessionDirectory(sessionId), "automatic-recording.json");

    /// <summary>임의 경로 문자열 대신 녹화 구간의 GUID에만 종료 신호를 만든다.</summary>
    private static string StopPath(SessionStore store, string sessionId, string runId)
    {
        if (!Guid.TryParseExact(runId, "N", out _)) throw new InvalidDataException("자동 녹화 구간 ID가 잘못되었습니다.");
        return Path.Combine(store.SessionDirectory(sessionId), $"automatic-recording-stop-{runId}");
    }

    /// <summary>동시에 교체되는 상태 파일을 읽어도 파일 교체를 막지 않으며 크기·세션 ID를 확인한다.</summary>
    public static AutomaticRecordingState? ReadState(SessionStore store, string sessionId)
    {
        string path = StatePath(store, sessionId);
        SessionStore.RejectReparse(path);
        if (!File.Exists(path)) return null;
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        if (stream.Length > 64_000) throw new InvalidDataException("자동 녹화 상태 파일이 너무 큽니다.");
        AutomaticRecordingState state = JsonSerializer.Deserialize<AutomaticRecordingState>(stream, SessionStore.Json)
            ?? throw new InvalidDataException("빈 자동 녹화 상태 파일입니다.");
        if (state.SessionId != sessionId) throw new InvalidDataException("자동 녹화 상태의 세션 ID가 다릅니다.");
        AnalysisFrameRate.Validate(state.Fps);
        _ = StopPath(store, sessionId, state.RunId);
        return state;
    }

    /// <summary>독립 녹화 상태는 세션 이벤트 파일을 건드리지 않고 완성된 JSON으로 교체한다.</summary>
    private static void SaveState(SessionStore store, AutomaticRecordingState state)
    {
        string path = StatePath(store, state.SessionId);
        string temporary = path + ".tmp";
        SessionStore.RejectReparse(path);
        SessionStore.WriteSmallFile(temporary, JsonSerializer.SerializeToUtf8Bytes(state with { UpdatedUtc = DateTimeOffset.UtcNow }, SessionStore.Json));
        File.Move(temporary, path, true);
    }

    /// <summary>별도 콘솔 창이나 MCP 표준 핸들 상속 없이 같은 분석기에서 녹화 전용 명령을 실행한다.</summary>
    public static ProcessStartInfo CreateStartInfo(string repository, string sessionId, string runId, int fps)
    {
        AnalysisFrameRate.Validate(fps);
        if (!Guid.TryParseExact(runId, "N", out _)) throw new ArgumentException("녹화 구간 ID가 잘못되었습니다.");
        string processPath = RecorderProcessPath();
        var start = new ProcessStartInfo(processPath)
        {
            WorkingDirectory = repository,
            UseShellExecute = false,
            CreateNoWindow = true,
            WindowStyle = ProcessWindowStyle.Hidden,
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
        };
        if (Path.GetFileNameWithoutExtension(processPath).Equals("dotnet", StringComparison.OrdinalIgnoreCase))
            start.ArgumentList.Add(typeof(AutomaticRecording).Assembly.Location);
        // ArgumentList는 공백·따옴표가 있는 저장소 경로도 쉘을 거치지 않고 그대로 전달한다.
        foreach (string argument in new[] { "--repo", repository, "record-auto", "--session", sessionId,
            "--run", runId, "--fps", fps.ToString(System.Globalization.CultureInfo.InvariantCulture) })
            start.ArgumentList.Add(argument);
        return start;
    }

    /// <summary>첫 프레임을 저장한 상태까지 확인하고 CLI가 끝나도 녹화 프로세스를 유지한다.</summary>
    public static async Task<AutomaticRecordingState> StartAsync(SessionStore store, AnalysisSession session, CancellationToken cancellation)
    {
        var initial = new AutomaticRecordingState { SessionId = session.Id, RunId = Guid.NewGuid().ToString("N"), Fps = session.Fps };
        SaveState(store, initial);
        using Process process = Process.Start(CreateStartInfo(store.Repository, session.Id, initial.RunId, session.Fps))
            ?? throw new InvalidOperationException("자동 녹화 프로세스를 시작하지 못했습니다.");
        process.StandardInput.Close();
        process.BeginOutputReadLine();
        process.BeginErrorReadLine();
        var timer = Stopwatch.StartNew();
        try
        {
            // 초기화 실패와 0프레임 녹화를 성공한 세션으로 반환하지 않는다.
            while (timer.Elapsed < TransitionTimeout)
            {
                cancellation.ThrowIfCancellationRequested();
                AutomaticRecordingState? state = ReadState(store, session.Id);
                if (state?.RunId != initial.RunId) throw new InvalidOperationException("자동 녹화 구간이 바뀌었습니다.");
                if (state.State == "recording" && state.Frames > 0) return state;
                if (state.State == "failed" || process.HasExited)
                    throw new InvalidOperationException("자동 녹화 시작 실패: " + (state.Error ?? "녹화 프로세스가 종료되었습니다."));
                await Task.Delay(StatusInterval, cancellation);
            }
            throw new TimeoutException("자동 녹화의 첫 프레임을 기다리는 시간이 초과되었습니다.");
        }
        catch
        {
            SessionStore.WriteSmallFile(StopPath(store, session.Id, initial.RunId), Encoding.UTF8.GetBytes("stop"));
            // 요청 취소 뒤에도 이미 연 파일을 닫을 시간을 주고 원본 게임은 그대로 둔다.
            try { await process.WaitForExitAsync().WaitAsync(TransitionTimeout); }
            catch (TimeoutException) { }
            throw;
        }
    }

    /// <summary>PID 재사용이나 다른 실행 파일을 자동 녹화 프로세스로 오인하지 않는다.</summary>
    private static Process? OwnedProcess(AutomaticRecordingState state)
    {
        if (state.ProcessId <= 0 || state.ProcessStartedUtcTicks <= 0
            || !string.Equals(state.ProcessPath, RecorderProcessPath(), StringComparison.OrdinalIgnoreCase)) return null;
        Process? process = null;
        try
        {
            process = Process.GetProcessById(state.ProcessId);
            if (!process.HasExited && process.StartTime.ToUniversalTime().Ticks == state.ProcessStartedUtcTicks
                && string.Equals(process.MainModule?.FileName, state.ProcessPath, StringComparison.OrdinalIgnoreCase)) return process;
        }
        catch (ArgumentException) { }
        catch (InvalidOperationException) { }
        catch (System.ComponentModel.Win32Exception) { }
        process?.Dispose();
        return null;
    }

    /// <summary>녹화 프로세스가 예기치 않게 사라지면 마지막 recording 상태를 진행 중으로 보고하지 않는다.</summary>
    public static AutomaticRecordingState? Status(SessionStore store, string sessionId)
    {
        AutomaticRecordingState? state = ReadState(store, sessionId);
        if (state == null || state.ProcessId <= 0 || state.State is "stopped" or "failed") return state;
        using Process? process = OwnedProcess(state);
        return process != null ? state : state with { State = "interrupted", Error = state.Error ?? "자동 녹화 프로세스를 확인할 수 없습니다." };
    }

    /// <summary>자신이 시작한 녹화 구간에 종료 신호를 보내고 AVI·WAV·입력 로그의 마감을 기다린다.</summary>
    public static async Task<AutomaticRecordingState?> StopAsync(SessionStore store, string sessionId, CancellationToken cancellation = default)
    {
        AutomaticRecordingState? state = ReadState(store, sessionId);
        if (state == null || state.State is "stopped" or "failed") return state;
        using Process? process = OwnedProcess(state);
        if (process == null) return Status(store, sessionId);
        SessionStore.WriteSmallFile(StopPath(store, sessionId, state.RunId), Encoding.UTF8.GetBytes("stop"));
        await process.WaitForExitAsync(cancellation).WaitAsync(TransitionTimeout, cancellation);
        return ReadState(store, sessionId);
    }

    /// <summary>숨겨진 녹화 전용 명령으로 세션 게임의 생존과 종료 신호를 감시한다.</summary>
    public static async Task<int> RunAsync(SessionStore store, string sessionId, string runId, int fps)
    {
        AnalysisFrameRate.Validate(fps);
        string stopPath = StopPath(store, sessionId, runId);
        using Process self = Process.GetCurrentProcess();
        var state = new AutomaticRecordingState { SessionId = sessionId, RunId = runId, Fps = fps, ProcessId = self.Id,
            ProcessStartedUtcTicks = self.StartTime.ToUniversalTime().Ticks, ProcessPath = Environment.ProcessPath! };
        GuidedRecorder? recorder = null;
        FileStream? recordingLock = null;
        try
        {
            if (ReadState(store, sessionId)?.RunId != runId) throw new InvalidOperationException("등록되지 않은 자동 녹화 구간입니다.");
            SaveState(store, state);
            if (!File.Exists(stopPath))
            {
                AnalysisSession session = store.Load(sessionId);
                using Process game = store.OwnedProcess(session) ?? throw new InvalidOperationException("녹화할 게임 세션이 종료되었습니다.");
                recordingLock = store.LockRecording(sessionId);
                recorder = new GuidedRecorder(store, session, fps: fps, writeSessionEvents: false, stopOnGameExit: true);
                recorder.Start();
                state = state with { State = "recording" };
                // 제어 CLI가 종료되어도 게임 종료·녹화 오류·명시적인 중단 신호까지 계속 기록한다.
                while (!game.HasExited && !File.Exists(stopPath))
                {
                    state = state with { Frames = recorder.FrameCount, ElapsedMs = recorder.Elapsed.TotalMilliseconds };
                    SaveState(store, state);
                    if (recorder.Error != null) throw new InvalidOperationException("자동 녹화 오류: " + recorder.Error.Message, recorder.Error);
                    await Task.Delay(StatusInterval);
                }
            }
        }
        catch (Exception error) { state = state with { Error = error.Message }; }
        finally
        {
            // 녹화기의 모든 출력 파일을 닫은 뒤 최종 색인과 상태를 남긴다.
            try
            {
                recorder?.Dispose();
                if (recorder != null)
                {
                    state = state with { Frames = recorder.FrameCount, ElapsedMs = recorder.Elapsed.TotalMilliseconds,
                        Error = state.Error ?? recorder.Error?.Message };
                    FreeplayRecordingIndex.Write(store, sessionId, freePlay: false);
                }
            }
            catch (Exception error) { state = state with { Error = state.Error ?? error.Message }; }
            finally { recordingLock?.Dispose(); }
            SaveState(store, state with { State = state.Error == null ? "stopped" : "failed" });
            if (File.Exists(stopPath)) File.Delete(stopPath);
        }
        return state.Error == null ? 0 : 2;
    }
}
