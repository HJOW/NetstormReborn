using System.Diagnostics;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace Netstorm.AnalyzeManager;

/// <summary>CLI와 MCP가 공유하는 요청. 알 수 없는 JSON 필드는 거부한다.</summary>
[System.Text.Json.Serialization.JsonUnmappedMemberHandling(System.Text.Json.Serialization.JsonUnmappedMemberHandling.Disallow)]
public sealed class AnalysisRequest
{
    public string SessionId { get; set; } = "";
    public string Label { get; set; } = "";
    public string Region { get; set; } = "";
    public string Kind { get; set; } = "click";
    public int X { get; set; }
    public int Y { get; set; }
    public int ToX { get; set; }
    public int ToY { get; set; }
    public string Button { get; set; } = "left";
    public string Key { get; set; } = "";
    public int DurationMs { get; set; } = 80;
    public int SettleMs { get; set; } = 300;
    public int TimeoutMs { get; set; } = 5000;
    public int PollMs { get; set; } = 200;
    public double Threshold { get; set; } = 0.01;
    public string Note { get; set; } = "";
    public string Steps { get; set; } = "";
    public string EvidenceHash { get; set; } = "";
    public bool Force { get; set; }
    public bool IncludeImage { get; set; } = true;
    // 아래는 YouTube 영상 분석(youtube_*) 전용 인자다 (docs/analyze-manager.md "YouTube 영상 분석").
    /// <summary>영상·채널·재생목록 주소</summary>
    public string Url { get; set; } = "";
    /// <summary>영상 ID (url 대신)</summary>
    public string VideoId { get; set; } = "";
    /// <summary>프레임 시각 쉼표 목록 (예: "612.5, 10:12.5")</summary>
    public string Times { get; set; } = "";
    /// <summary>시작 시각(초): 프레임 간격 지정·구간 저장</summary>
    public double? Start { get; set; }
    /// <summary>프레임 간격(초)</summary>
    public double? Step { get; set; }
    /// <summary>프레임 수 (start·step 과 함께)</summary>
    public int Count { get; set; }
    /// <summary>구간 끝 시각(초)</summary>
    public double? End { get; set; }
    /// <summary>메모를 붙일 영상 시각(초)</summary>
    public double? Time { get; set; }
    /// <summary>받을 스트림의 최대 세로 해상도 (0 = 기본)</summary>
    public int MaxHeight { get; set; }
    /// <summary>관찰표 열 수 (0 = 자동)</summary>
    public int Columns { get; set; }
    /// <summary>관찰표 칸 폭 (0 = 기본)</summary>
    public int CellWidth { get; set; }
    /// <summary>목록 조회 개수 (0 = 기본)</summary>
    public int Limit { get; set; }
    /// <summary>스트림이 메타데이터보다 길어도(서버 삽입 광고 의심) 진행</summary>
    public bool AllowDurationMismatch { get; set; }
    /// <summary>구간 저장에 소리 포함</summary>
    public bool IncludeAudio { get; set; }
}

/// <summary>프로토콜과 독립적인 도구 결과. 이미지는 CLI에서는 경로, MCP에서는 이미지 콘텐츠로 제공한다.</summary>
public sealed record AnalysisResult(object Data, string? ImagePath = null, bool IsError = false);

/// <summary>게임 조작·변화 관찰·문서 생성을 같은 경로로 수행하는 분석 엔진.</summary>
public sealed class AnalysisEngine
{
    public SessionStore Store { get; }

    /// <summary>원본 게임 대신 YouTube 영상을 읽는 도구 (게임 실행·데스크톱 잠금과 무관)</summary>
    public YouTubeAnalyzer YouTube { get; }

    /// <summary>저장소를 지정해 기록 관리자와 연결한다.</summary>
    public AnalysisEngine(string repository)
    {
        Store = new SessionStore(repository);
        YouTube = new YouTubeAnalyzer(repository);
    }

    /// <summary>제한된 명령만 실행하고 오류도 AI가 재시도에 쓸 수 있는 구조화된 결과로 돌려준다.</summary>
    public async Task<AnalysisResult> ExecuteAsync(string tool, AnalysisRequest request, CancellationToken cancellation = default)
    {
        if (tool.StartsWith("youtube_", StringComparison.Ordinal))
        {
            // 영상 분석은 게임을 실행하지 않으므로 데스크톱 잠금 없이 처리한다 (내려받는 동안 게임 도구를 막지 않도록).
            try
            {
                return await YouTube.ExecuteAsync(tool, request, cancellation);
            }
            catch (Exception error)
            {
                return new(new { error = error.Message, tool, cancelled = error is OperationCanceledException }, IsError: true);
            }
        }
        AnalysisSession? session = null;
        try
        {
            using FileStream desktopLock = Store.LockDesktop();
            if (tool == "list_sessions") return ListSessions();
            if (tool == "start_session")
            {
                // 동일 데스크톱을 사용하는 분석 게임이 있으면 새 실행을 중복 시작하지 않는다.
                foreach (AnalysisSession previous in Store.Sessions())
                {
                    using Process? running = Store.OwnedProcess(previous);
                    if (running != null) throw new InvalidOperationException($"실행 중인 sessionId={previous.Id}를 계속 사용하거나 종료하세요.");
                }
                session = await Store.CreateAsync(request.Label, cancellation);
                return await StartAsync(session, cancellation);
            }
            session = Store.Load(request.SessionId);
            if (session.EventCount >= SessionStore.MaximumEvents - 1 && tool is not ("game_status" or "end_session"))
                throw new InvalidOperationException("세션 이벤트 한도입니다. end_session 후 새 세션을 시작하세요.");
            try
            {
                return tool switch
                {
                    "game_status" => Status(session),
                    "capture_state" => await CaptureAsync(session, request, cancellation),
                    "game_input" => await InputAsync(session, request, cancellation),
                    "wait_for_change" => await WaitAsync(session, request, cancellation),
                    "record_observation" => Observe(session, request),
                    "set_guide_steps" => SetGuideSteps(session, request),
                    "end_session" => await EndAsync(session, request.Force, cancellation),
                    _ => throw new ArgumentException($"알 수 없는 도구: {tool}"),
                };
            }
            catch (Exception error)
            {
                // 입력 실패나 취소도 같은 잠금 안에서 기록하여 다음 세션의 판단 근거로 남긴다.
                if (session.EventCount < SessionStore.MaximumEvents - 1)
                    Store.Append(session, "error", new { tool, error = error.Message, cancelled = error is OperationCanceledException });
                throw;
            }
        }
        catch (Exception error)
        {
            return new(new { error = error.Message, sessionId = session?.Id, cancelled = error is OperationCanceledException }, IsError: true);
        }
    }

    /// <summary>최근 세션의 복구 가능한 ID와 문서 경로를 돌려준다.</summary>
    private AnalysisResult ListSessions()
    {
        var items = new List<object>();
        // 과거 기록 전체 대신 최근 20개만 응답하여 MCP 대화 크기를 제한한다.
        foreach (AnalysisSession session in Store.Sessions().OrderByDescending(s => s.Id).Take(20))
        {
            using Process? process = Store.OwnedProcess(session);
            items.Add(new { sessionId = session.Id, session.Label, session.Phase, running = process != null, report = ReportPath(session) });
        }
        return new(new { sessions = items });
    }

    /// <summary>게임을 실행하고 최대 15초 안에 보이는 창을 얻는다.</summary>
    private async Task<AnalysisResult> StartAsync(AnalysisSession session, CancellationToken cancellation)
    {
        try
        {
            using Process process = Process.Start(new ProcessStartInfo(Store.GamePath(session))
            {
                WorkingDirectory = Path.GetDirectoryName(Store.GamePath(session)),
                // Wine의 ShellExecute가 MCP/CLI 표준 핸들을 게임에 넘겨 EOF를 지연시키므로 별도 파이프로 격리한다.
                UseShellExecute = false,
                RedirectStandardInput = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                WindowStyle = ProcessWindowStyle.Normal,
            }) ?? throw new InvalidOperationException("원본 게임 프로세스를 시작하지 못했습니다.");
            // 게임의 진단 출력이 파이프를 가득 채우지 않도록 버리고, 입력 쪽은 즉시 닫는다.
            process.StandardInput.Close();
            process.BeginOutputReadLine();
            process.BeginErrorReadLine();
            session.ProcessId = process.Id;
            session.ProcessStartedUtcTicks = process.StartTime.ToUniversalTime().Ticks;
            session.Phase = "running";
            Store.Append(session, "started", new { session.ProcessId, session.ExeSha256, resolution = "1024x768", windowed = true });
            var timer = Stopwatch.StartNew();
            string lastError = "표시 창을 기다리는 중";
            // 시작 화면이 준비될 때까지 짧게 기다리며 취소와 프로세스 종료를 감시한다.
            while (timer.ElapsedMilliseconds < 15_000)
            {
                cancellation.ThrowIfCancellationRequested();
                if (process.HasExited) throw new InvalidOperationException($"원본 게임이 시작 도중 종료되었습니다: {process.ExitCode}");
                try
                {
                    await WindowsGame.FocusAsync(process.Id, cancellation);
                    await Task.Delay(500, cancellation);
                    return await CaptureAsync(session, new AnalysisRequest(), cancellation);
                }
                catch (InvalidOperationException error) { lastError = error.Message; }
                await Task.Delay(200, cancellation);
            }
            throw new TimeoutException($"시작 화면을 확인하지 못했습니다: {lastError}. game_status/end_session으로 이어갈 수 있습니다.");
        }
        catch (Exception error)
        {
            Store.Append(session, "start_failed", new { error = error.Message });
            throw;
        }
    }

    /// <summary>소유한 게임만 반환하고 종료되었거나 PID가 바뀌었으면 거부한다.</summary>
    private Process RequireProcess(AnalysisSession session) => Store.OwnedProcess(session)
        ?? throw new InvalidOperationException("이 세션의 게임이 종료되었거나 프로세스 식별 정보가 다릅니다.");

    /// <summary>포커스를 건드리지 않고 프로세스와 창 상태를 읽는다.</summary>
    private AnalysisResult Status(AnalysisSession session)
    {
        using Process? process = Store.OwnedProcess(session);
        GameWindow? window = null;
        string? windowError = null;
        if (process != null)
        {
            try { window = WindowsGame.FindWindow(process.Id); }
            catch (InvalidOperationException error) { windowError = error.Message; }
        }
        return new(new { sessionId = session.Id, session.Phase, running = process != null, window, windowError,
            session.EventCount, report = ReportPath(session) });
    }

    /// <summary>게임 창을 전면에 두고 필요한 영역의 화면과 증거 경로를 반환한다.</summary>
    private async Task<AnalysisResult> CaptureAsync(AnalysisSession session, AnalysisRequest request, CancellationToken cancellation)
    {
        using Process process = RequireProcess(session);
        GameWindow window = await WindowsGame.FocusAsync(process.Id, cancellation);
        CapturedFrame frame = WindowsGame.Capture(window, request.Region);
        ScreenshotEvidence evidence = Store.StoreFrame(session, frame);
        Store.Append(session, "capture", new { region = request.Region }, evidence);
        return FrameResult(session, evidence, request.IncludeImage, new { sessionId = session.Id, evidence, report = ReportPath(session) });
    }

    /// <summary>원본 입력을 보내기 전에 인자를 검사하고 전후 화면을 모두 기록한다.</summary>
    private async Task<AnalysisResult> InputAsync(AnalysisSession session, AnalysisRequest request, CancellationToken cancellation)
    {
        if (request.Kind is not ("move" or "click" or "drag" or "key")) throw new ArgumentException("kind는 move/click/drag/key 중 하나입니다.");
        if (request.DurationMs is < 1 or > 2000 || request.SettleMs is < 0 or > 5000)
            throw new ArgumentException("durationMs는 1~2000, settleMs는 0~5000입니다.");
        if (request.Button is not ("left" or "right" or "middle")) throw new ArgumentException("지원하지 않는 마우스 버튼입니다.");
        if (request.Kind == "key") WindowsGame.ParseKeys(request.Key);
        if (session.EventCount > SessionStore.MaximumEvents - 5) throw new InvalidOperationException("새 세션으로 입력 기록을 이어가세요.");
        using Process process = RequireProcess(session);
        GameWindow window = await WindowsGame.FocusAsync(process.Id, cancellation);
        if (request.Kind != "key" && (!Inside(window, request.X, request.Y)
            || (request.Kind == "drag" && !Inside(window, request.ToX, request.ToY))))
            throw new ArgumentException("입력 좌표가 클라이언트 영역 밖입니다.");
        var action = new InputRequest(request.Kind, request.X, request.Y, request.ToX, request.ToY,
            request.Button, request.Key, request.DurationMs, request.SettleMs);
        CapturedFrame before = WindowsGame.Capture(window, "");
        ScreenshotEvidence beforeEvidence = Store.StoreFrame(session, before);
        Store.Append(session, "input_requested", action, beforeEvidence);
        await WindowsGame.InputAsync(window, action, cancellation);
        // OS가 받아들인 입력과 게임의 실제 반응은 별도 기록한다.
        Store.Append(session, "input_sent", action);
        await Task.Delay(request.SettleMs, cancellation);
        GameWindow afterWindow = WindowsGame.FindWindow(process.Id);
        CapturedFrame after = WindowsGame.Capture(afterWindow, "");
        ScreenshotEvidence afterEvidence = Store.StoreFrame(session, after);
        double changedRatio = after.Difference(before);
        Store.Append(session, "input_result", new { changedRatio, action }, afterEvidence);
        return FrameResult(session, afterEvidence, request.IncludeImage,
            new { sessionId = session.Id, before = beforeEvidence, after = afterEvidence, changedRatio, report = ReportPath(session) });
    }

    /// <summary>좌표 계산에서 음수와 우측/하단 경계를 제외한다.</summary>
    private static bool Inside(GameWindow window, int x, int y) => x >= 0 && y >= 0 && x < window.Width && y < window.Height;

    /// <summary>기준 화면 대비 지정 비율 이상 바뀔 때까지 관찰하되, 중간 프레임을 모두 저장하지 않는다.</summary>
    private async Task<AnalysisResult> WaitAsync(AnalysisSession session, AnalysisRequest request, CancellationToken cancellation)
    {
        if (request.TimeoutMs is < 1 or > 30_000 || request.PollMs is < 50 or > 5000
            || !double.IsFinite(request.Threshold) || request.Threshold is <= 0 or > 1)
            throw new ArgumentException("timeoutMs=1~30000, pollMs=50~5000, threshold=0 초과~1 이하여야 합니다.");
        using Process process = RequireProcess(session);
        GameWindow window = await WindowsGame.FocusAsync(process.Id, cancellation);
        CapturedFrame baseline = WindowsGame.Capture(window, request.Region);
        ScreenshotEvidence before = Store.StoreFrame(session, baseline);
        Store.Append(session, "wait_started", new { request.TimeoutMs, request.PollMs, request.Threshold, request.Region }, before);
        var timer = Stopwatch.StartNew();
        CapturedFrame current = baseline;
        double changedRatio = 0;
        int samples = 0;
        // 30초 이내의 제한된 관찰만 허용하며 실제 게임의 프레임 간격으로 해석하지 않는다.
        while (timer.ElapsedMilliseconds < request.TimeoutMs)
        {
            await Task.Delay((int)Math.Max(0, Math.Min(request.PollMs, request.TimeoutMs - timer.ElapsedMilliseconds)), cancellation);
            current = WindowsGame.Capture(WindowsGame.FindWindow(process.Id), request.Region);
            samples++;
            changedRatio = current.Difference(baseline);
            if (changedRatio >= request.Threshold) break;
        }
        ScreenshotEvidence after = Store.StoreFrame(session, current);
        bool matched = changedRatio >= request.Threshold;
        var result = new { sessionId = session.Id, matched, timedOut = !matched, changedRatio, samples,
            elapsedMs = timer.ElapsedMilliseconds, before, after, report = ReportPath(session) };
        Store.Append(session, "wait_finished", result, after);
        return FrameResult(session, after, request.IncludeImage, result);
    }

    /// <summary>AI의 해석은 사실 기록과 구분하고 이미 저장된 캡처 해시만 인용하게 한다.</summary>
    private AnalysisResult Observe(AnalysisSession session, AnalysisRequest request)
    {
        if (string.IsNullOrWhiteSpace(request.Note) || request.Note.Length > 8000) throw new ArgumentException("note는 1~8000자입니다.");
        string? evidence = null;
        if (request.EvidenceHash.Length != 0)
        {
            if (!Regex.IsMatch(request.EvidenceHash, "^[a-f0-9]{64}$")) throw new ArgumentException("evidenceHash는 저장된 캡처의 SHA-256입니다.");
            evidence = $"screens/{request.EvidenceHash}.png";
            if (!File.Exists(Path.Combine(Store.SessionDirectory(session.Id), evidence))) throw new ArgumentException("이 세션에 없는 캡처입니다.");
        }
        Store.Append(session, "ai_observation", new { note = request.Note, evidence, source = "AI의 해석; 원본 화면과 대조 필요" });
        return new(new { sessionId = session.Id, report = ReportPath(session), eventNumber = session.EventCount });
    }

    /// <summary>AI가 만든 안내 순서를 세션에 저장해 사용자 직접 조작 창에서 읽게 한다.</summary>
    private AnalysisResult SetGuideSteps(AnalysisSession session, AnalysisRequest request)
    {
        byte[] bytes = System.Text.Encoding.UTF8.GetBytes(request.Steps);
        if (bytes.Length is < 1 or > 64_000 || !request.Steps.Split('\n').Any(line => !string.IsNullOrWhiteSpace(line)))
            throw new ArgumentException("steps는 비어 있지 않은 UTF-8 텍스트여야 하며 64 KB 이하여야 합니다.");
        string path = Path.Combine(Store.SessionDirectory(session.Id), "guide-steps.txt");
        SessionStore.WriteSmallFile(path, bytes);
        Store.Append(session, "guided_steps_updated", new { count = request.Steps.Split('\n').Count(line => !string.IsNullOrWhiteSpace(line)) });
        return new(new { sessionId = session.Id, path, report = ReportPath(session) });
    }

    /// <summary>종료 확인 대화상자가 남으면 그 상태를 돌려주고, force일 때만 소유한 프로세스를 끝낸다.</summary>
    private async Task<AnalysisResult> EndAsync(AnalysisSession session, bool force, CancellationToken cancellation)
    {
        using Process? process = Store.OwnedProcess(session);
        if (process != null)
        {
            if (force) process.Kill();
            else WindowsGame.RequestClose(WindowsGame.FindWindow(process.Id));
            var timer = Stopwatch.StartNew();
            // 종료 창이 열려 있으면 무한 대기하지 않고 AI가 다음 입력을 선택하게 한다.
            while (!process.HasExited && timer.ElapsedMilliseconds < 2000) await Task.Delay(100, cancellation);
            if (!process.HasExited)
            {
                if (session.EventCount < SessionStore.MaximumEvents - 1) Store.Append(session, "close_pending", new { force });
                return new(new { sessionId = session.Id, closed = false, reason = "종료 확인 창이 남아 있습니다. capture_state로 확인하거나 force=true로 종료하세요.", report = ReportPath(session) });
            }
        }
        session.Phase = "ended";
        Store.Save(session);
        if (session.EventCount < SessionStore.MaximumEvents) Store.Append(session, "ended", new { force });
        return new(new { sessionId = session.Id, closed = true, report = ReportPath(session) });
    }

    /// <summary>이미지 반환 여부는 AI가 고를 수 있고 증거 기록에는 영향을 주지 않는다.</summary>
    private AnalysisResult FrameResult(AnalysisSession session, ScreenshotEvidence image, bool includeImage, object data) =>
        new(data, includeImage ? Path.Combine(Store.SessionDirectory(session.Id), image.Path) : null);

    /// <summary>문서 인덱스의 절대 경로를 호출자에게 제공한다.</summary>
    private string ReportPath(AnalysisSession session) => Path.Combine(Store.SessionDirectory(session.Id), "report.md");
}
