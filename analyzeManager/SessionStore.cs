using System.Diagnostics;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using Netstorm.Assets;

namespace Netstorm.AnalyzeManager;

/// <summary>CLI 재실행과 MCP 재접속 뒤에도 같은 게임을 찾아갈 수 있는 세션 정보.</summary>
public sealed class AnalysisSession
{
    public string Id { get; set; } = "";
    public string Label { get; set; } = "";
    public DateTimeOffset CreatedUtc { get; set; } = DateTimeOffset.UtcNow;
    public long StartedCounter { get; set; } = Stopwatch.GetTimestamp();
    public int? ProcessId { get; set; }
    public long? ProcessStartedUtcTicks { get; set; }
    public string ExeSha256 { get; set; } = "";
    public string Phase { get; set; } = "preparing";
    public int EventCount { get; set; }
    public int LogPart { get; set; } = 1;
    public int ReportPart { get; set; } = 1;
}

/// <summary>동일 PNG는 파일 하나를 재사용하고 각 관찰 시각은 별도 이벤트로 남긴다. Method는 캡처 방식(CapturedFrame 참고).</summary>
public sealed record ScreenshotEvidence(string Path, string Sha256, int Width, int Height,
    CaptureRegion Region, bool Reused, GameWindow Window, string Method = "screen");

/// <summary>원본 복사본과 작은 증거 파일을 관리한다. 원본 폴더에 쓰는 경로는 제공하지 않는다.</summary>
public sealed class SessionStore
{
    /// <summary>TODO의 파일별 50 MB 미만 제한. MB는 1,000,000바이트 기준이다.</summary>
    public const int FileLimitBytes = 50_000_000;
    /// <summary>이벤트/문서 파일은 4 MB가 되기 전에 다음 파일로 분할한다.</summary>
    public const int DefaultPartBytes = 4_000_000;
    /// <summary>대화나 무한 반복으로 증거가 끝없이 쌓이지 않도록 한 세션 한도를 둔다.</summary>
    public const int MaximumEvents = 10_000;
    /// <summary>한글을 UTF-8로 저장하며 외부 프로토콜과 맞추는 JSON 설정.</summary>
    public static readonly JsonSerializerOptions Json = new(JsonSerializerDefaults.Web)
    {
        Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping,
    };

    public string Repository { get; }
    public string Root { get; }
    private readonly int _partBytes;

    /// <summary>증거 저장 위치를 저장소의 git 제외 디렉토리 안에 고정한다.</summary>
    public SessionStore(string repository, int partBytes = DefaultPartBytes)
    {
        Repository = Path.GetFullPath(repository);
        Root = Path.Combine(Repository, "extracted", "analyzeManager");
        _partBytes = partBytes is > 0 and < FileLimitBytes ? partBytes : throw new ArgumentOutOfRangeException(nameof(partBytes));
        RejectReparse(Root);
        Directory.CreateDirectory(Root);
    }

    /// <summary>CLI와 여러 MCP 요청이 데스크톱 입력과 파일을 동시에 변경하지 못하게 잠근다.</summary>
    public FileStream LockDesktop() => new(Path.Combine(Root, "desktop.lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);

    /// <summary>사용자가 지정한 세션 ID로 다른 경로를 읽지 못하게 제한한다.</summary>
    public string SessionDirectory(string id)
    {
        if (!Regex.IsMatch(id, "^[0-9]{8}T[0-9]{9}Z-[a-f0-9]{12}$"))
            throw new ArgumentException("유효한 sessionId가 아닙니다.");
        string directory = Path.Combine(Root, id);
        RejectReparse(directory);
        return directory;
    }

    /// <summary>자유 플레이 녹화물을 세션별 playingVideos 폴더에 두고 경로 우회를 막는다.</summary>
    public string FreeplayDirectory(string id)
    {
        _ = SessionDirectory(id);
        string directory = Path.Combine(Repository, "playingVideos", id);
        RejectReparse(directory);
        return directory;
    }

    /// <summary>복사본의 실행 파일 경로는 매번 계산하여 manifest의 임의 경로를 신뢰하지 않는다.</summary>
    public string GamePath(AnalysisSession session) => Path.Combine(SessionDirectory(session.Id), "game", "Netstorm.exe");

    /// <summary>작업 폴더를 만들고 원본 구성 전체를 복사한 뒤, 복사본의 창 모드/저장 경로만 바꾼다.</summary>
    public async Task<AnalysisSession> CreateAsync(string label, CancellationToken cancellation)
    {
        if (label.Length > 200) throw new ArgumentException("label은 200자 이하여야 합니다.");
        var session = new AnalysisSession { Id = $"{DateTime.UtcNow:yyyyMMddTHHmmssfffZ}-{Guid.NewGuid():N}"[..32], Label = label };
        string directory = SessionDirectory(session.Id);
        Directory.CreateDirectory(directory);
        Save(session);
        try
        {
            string source = Path.Combine(Repository, "originals");
            string destination = Path.GetDirectoryName(GamePath(session))!;
            await CopyTreeAsync(source, destination, cancellation);
            // 두 설정 파일을 함께 수정하여 저장 경로와 창 모드의 기본값도 복사본에 한정한다.
            foreach (string name in new[] { "options.cfg", "setup.cfg" })
            {
                string file = Path.Combine(destination, "d", name);
                ConfigFile config = ConfigFile.Load(file);
                config.Set("InstallDir", destination);
                config.Set("startInFullScreen", "0");
                config.Set("SCREENW", "1024");
                config.Set("SCREENH", "768");
                WriteSmallFile(file, config.Encode());
            }
            session.ExeSha256 = Convert.ToHexString(SHA256.HashData(await File.ReadAllBytesAsync(GamePath(session), cancellation))).ToLowerInvariant();
            session.Phase = "prepared";
            Save(session);
            return session;
        }
        catch (Exception error)
        {
            session.Phase = "prepare_failed";
            Append(session, "prepare_failed", new { error = error.Message });
            throw new InvalidOperationException($"복사본 준비 실패. sessionId={session.Id}: {error.Message}", error);
        }
    }

    /// <summary>심볼릭 링크를 따라가지 않고 50 MB 미만 파일만 독립 복사한다.</summary>
    private static async Task CopyTreeAsync(string source, string destination, CancellationToken cancellation)
    {
        RejectReparse(source);
        Directory.CreateDirectory(destination);
        // 먼저 현재 디렉토리의 파일을 복사하여 원본의 쓰기 속성을 물려받지 않는다.
        foreach (string file in Directory.EnumerateFiles(source))
        {
            cancellation.ThrowIfCancellationRequested();
            RejectReparse(file);
            if (new FileInfo(file).Length >= FileLimitBytes) throw new InvalidOperationException($"50 MB 이상 원본 파일: {file}");
            await using var input = File.OpenRead(file);
            await using var output = new FileStream(Path.Combine(destination, Path.GetFileName(file)), FileMode.CreateNew);
            await input.CopyToAsync(output, cancellation);
        }
        // 하위 디렉토리도 링크를 거부하며 같은 규칙으로 복사한다.
        foreach (string child in Directory.EnumerateDirectories(source))
            await CopyTreeAsync(child, Path.Combine(destination, Path.GetFileName(child)), cancellation);
    }

    /// <summary>
    /// 기존 경로와 부모 경로에 junction이나 심볼릭 링크가 있으면 거부한다.
    /// 드라이브 루트는 검사하지 않는다. Wine은 리눅스 루트에 연결된 <c>Z:\</c>를 링크로 보고하지만,
    /// 루트 자체는 다른 위치로 우회되는 중간 경로가 아니기 때문이다.
    /// </summary>
    public static void RejectReparse(string path)
    {
        string? current = Path.GetFullPath(path);
        string? root = Path.GetPathRoot(current);
        // 드라이브 루트를 제외한 존재하는 모든 부모를 검사하여 작업 폴더가 저장소 밖으로 우회되지 않게 한다.
        while (current != null && !string.Equals(current, root, StringComparison.OrdinalIgnoreCase))
        {
            if ((File.Exists(current) || Directory.Exists(current)) && (File.GetAttributes(current) & FileAttributes.ReparsePoint) != 0)
                throw new InvalidOperationException($"분석 경로에 링크를 사용할 수 없습니다: {current}");
            current = Path.GetDirectoryName(current);
        }
    }

    /// <summary>완전한 임시 파일을 교체하여 세션 상태가 중간에 잘리는 일을 줄인다.</summary>
    public void Save(AnalysisSession session)
    {
        string path = Path.Combine(SessionDirectory(session.Id), "session.json");
        WriteSmallFile(path + ".tmp", JsonSerializer.SerializeToUtf8Bytes(session, Json));
        File.Move(path + ".tmp", path, true);
    }

    /// <summary>세션을 읽고 폴더 이름과 manifest의 ID가 같은지 확인한다.</summary>
    public AnalysisSession Load(string id)
    {
        string path = Path.Combine(SessionDirectory(id), "session.json");
        RejectReparse(path);
        if (new FileInfo(path).Length > 64_000) throw new InvalidDataException("session.json 크기 오류.");
        AnalysisSession session = JsonSerializer.Deserialize<AnalysisSession>(File.ReadAllText(path, Encoding.UTF8), Json)
            ?? throw new InvalidDataException("빈 세션 정보입니다.");
        if (session.Id != id) throw new InvalidDataException("세션 폴더와 ID가 다릅니다.");
        return session;
    }

    /// <summary>도구가 만든 세션만 열거한다.</summary>
    public IEnumerable<AnalysisSession> Sessions()
    {
        // 중단된 준비 세션도 남겨 두어 이후 원인을 조사할 수 있다.
        foreach (string directory in Directory.EnumerateDirectories(Root))
        {
            if (File.Exists(Path.Combine(directory, "session.json"))) yield return Load(Path.GetFileName(directory));
        }
    }

    /// <summary>PID 재사용이나 다른 실행 파일을 같은 게임으로 오인하지 않는다.</summary>
    public Process? OwnedProcess(AnalysisSession session)
    {
        if (!session.ProcessId.HasValue || !session.ProcessStartedUtcTicks.HasValue) return null;
        Process? process = null;
        try
        {
            process = Process.GetProcessById(session.ProcessId.Value);
            if (!process.HasExited && process.StartTime.ToUniversalTime().Ticks == session.ProcessStartedUtcTicks
                && string.Equals(process.MainModule?.FileName, GamePath(session), StringComparison.OrdinalIgnoreCase)) return process;
        }
        catch (ArgumentException) { }
        catch (InvalidOperationException) { }
        catch (System.ComponentModel.Win32Exception) { }
        process?.Dispose();
        return null;
    }

    /// <summary>PNG 해시로 중복을 제거하고 총 용량/파일 개수 제한을 적용한다.</summary>
    public ScreenshotEvidence StoreFrame(AnalysisSession session, CapturedFrame frame)
    {
        if (frame.Png.Length >= FileLimitBytes) throw new InvalidOperationException("단일 PNG는 50 MB 미만이어야 합니다. 더 작은 영역을 지정하세요.");
        string hash = Convert.ToHexString(SHA256.HashData(frame.Png)).ToLowerInvariant();
        string directory = Path.Combine(SessionDirectory(session.Id), "screens");
        RejectReparse(directory);
        Directory.CreateDirectory(directory);
        string path = Path.Combine(directory, hash + ".png");
        RejectReparse(path);
        bool reused = File.Exists(path);
        if (reused)
        {
            // 같은 이름의 파일이 중간에 잘렸거나 바뀌었다면 증거로 재사용하지 않는다.
            if (new FileInfo(path).Length != frame.Png.Length)
                throw new InvalidDataException($"저장된 화면 증거의 크기가 일치하지 않습니다: {path}");
            byte[] existing = File.ReadAllBytes(path);
            if (!string.Equals(Convert.ToHexString(SHA256.HashData(existing)), hash, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException($"저장된 화면 증거의 SHA-256이 일치하지 않습니다: {path}");
        }
        else
        {
            // 임시 파일을 완성한 뒤 옮겨 캡처 도중 실패한 바이트를 최종 해시 이름으로 남기지 않는다.
            string temporary = Path.Combine(directory, $".{hash}-{Guid.NewGuid():N}.tmp");
            try
            {
                WriteSmallFile(temporary, frame.Png);
                File.Move(temporary, path);
            }
            finally
            {
                if (File.Exists(temporary)) File.Delete(temporary);
            }
        }
        return new($"screens/{hash}.png", hash, frame.Region.Width, frame.Region.Height, frame.Region, reused, frame.Window, frame.Method);
    }

    /// <summary>기계용 이벤트와 사람이 읽는 한국어 문서에 같은 순서/시각/증거를 남긴다.</summary>
    public void Append(AnalysisSession session, string kind, object data, params ScreenshotEvidence[] evidence)
    {
        if (session.EventCount >= MaximumEvents) throw new InvalidOperationException("세션 이벤트 한도에 도달했습니다.");
        int number = ++session.EventCount;
        DateTimeOffset now = DateTimeOffset.UtcNow;
        var item = new { number, utc = now, elapsedMs = Stopwatch.GetElapsedTime(session.StartedCounter).TotalMilliseconds, kind, data, evidence };
        string json = JsonSerializer.Serialize(item, Json);
        string directory = SessionDirectory(session.Id);
        session.LogPart = AppendPart(directory, "events", "jsonl", session.LogPart, json + "\n");
        var markdown = new StringBuilder($"\n## {number}. {kind} — {now:O}\n\n```json\n{json}\n```\n");
        // 요약 문서에는 각 캡처 파일의 참조만 두고 PNG를 중복 저장하지 않는다.
        foreach (ScreenshotEvidence image in evidence)
            markdown.Append($"\n![관찰 화면 {number}]({image.Path})\n");
        session.ReportPart = AppendPart(directory, "report", "md", session.ReportPart, markdown.ToString());
        string links = string.Join("\n", Enumerable.Range(1, session.ReportPart).Select(i => $"- [기록 {i}](report-{i:0000}.md)"));
        string index = $"# 원본 게임 분석 기록\n\n세션: `{session.Id}`\n\n게임 실행 파일 SHA-256: `{session.ExeSha256}`\n\n관찰 메모는 호출한 AI의 해석입니다. 입력과 화면 증거는 JSONL에 보존합니다.\n\n{links}\n";
        WriteSmallFile(Path.Combine(directory, "report.md"), Encoding.UTF8.GetBytes(index));
        Save(session);
    }

    /// <summary>UTF-8 바이트 길이로 판단하여 이벤트와 Markdown을 경계에서 분할한다.</summary>
    private int AppendPart(string directory, string prefix, string extension, int part, string text)
    {
        byte[] bytes = Encoding.UTF8.GetBytes(text);
        if (bytes.Length >= FileLimitBytes || bytes.Length > _partBytes) throw new InvalidOperationException("단일 기록이 파일 한도보다 큽니다.");
        string path = Path.Combine(directory, $"{prefix}-{part:0000}.{extension}");
        RejectReparse(path);
        if (File.Exists(path) && new FileInfo(path).Length + bytes.Length > _partBytes)
            path = Path.Combine(directory, $"{prefix}-{++part:0000}.{extension}");
        RejectReparse(path);
        using var stream = new FileStream(path, FileMode.Append, FileAccess.Write);
        stream.Write(bytes);
        return part;
    }

    /// <summary>바이너리·설정·JSON 모두 쓰기 전에 파일별 용량과 링크를 확인한다.</summary>
    public static void WriteSmallFile(string path, byte[] bytes)
    {
        if (bytes.Length >= FileLimitBytes) throw new InvalidOperationException("파일은 50 MB 미만이어야 합니다.");
        RejectReparse(path);
        File.WriteAllBytes(path, bytes);
    }
}
