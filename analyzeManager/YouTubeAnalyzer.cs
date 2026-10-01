using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Globalization;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace Netstorm.AnalyzeManager;

/// <summary>
/// YouTube 영상을 원본 게임 대신 분석 자료로 쓰는 도구 모음 (`youtube_*`). 원본 게임을 실행하지 않는다.
/// <list type="bullet">
/// <item>yt-dlp 로 메타데이터·스트림 주소를 얻고, ffmpeg 가 원격 스트림에서 필요한 시각만 읽어 프레임을 만든다(영상 전체를 내려받지 않는다).</item>
/// <item>광고: YouTube 재생기가 앞·중간에 트는 광고는 별도 영상이라 스트림에 없다. 서버가 광고를 이어 붙인 스트림은 길이가 늘어나므로
/// 메타데이터 길이와 비교해 막는다. 업로더가 영상 안에 넣은 협찬·홍보 구간은 SponsorBlock 구간으로 표시한다.</item>
/// <item>결과는 Git 에서 제외되는 <c>extracted/youtube/&lt;영상 ID&gt;/</c> 에 남긴다.</item>
/// </list>
/// 사용법: docs/analyze-manager.md "YouTube 영상 분석".
/// </summary>
public sealed class YouTubeAnalyzer
{
    /// <summary>yt-dlp 실행 파일 위치를 바꾸는 환경 변수</summary>
    public const string YtDlpVariable = "NETSTORM_YTDLP";

    /// <summary>ffmpeg 실행 파일 위치를 바꾸는 환경 변수</summary>
    public const string FfmpegVariable = "NETSTORM_FFMPEG";

    /// <summary>ffprobe 실행 파일 위치를 바꾸는 환경 변수</summary>
    public const string FfprobeVariable = "NETSTORM_FFPROBE";

    /// <summary>한 번에 뽑는 프레임 수 상한 (대화 크기·요청 시간 제한)</summary>
    public const int MaximumFrames = 60;

    /// <summary>구간 저장 길이 상한(초). 긴 시간 측정은 여러 번 나눠 받는다.</summary>
    public const double MaximumClipSeconds = 300;

    /// <summary>프레임·구간의 최대 세로 해상도 상한</summary>
    public const int MaximumHeight = 1080;

    /// <summary>기본 세로 해상도 (원본 게임 최대 해상도 768 이상을 담는 1080)</summary>
    public const int DefaultFrameHeight = 1080;

    /// <summary>구간 저장 기본 세로 해상도 (파일 크기 50 MB 미만을 맞추기 위해 720)</summary>
    public const int DefaultClipHeight = 720;

    /// <summary>목록 조회 기본·최대 개수</summary>
    public const int DefaultListLimit = 100;

    /// <summary>목록 조회 최대 개수</summary>
    public const int MaximumListLimit = 500;

    /// <summary>스트림 주소를 만료 이만큼 전에 새로 받는다(분)</summary>
    private const int StreamRefreshMarginMinutes = 10;

    /// <summary>메타데이터·목록 조회 제한 시간(초)</summary>
    private const int ProbeTimeoutSeconds = 180;

    /// <summary>프레임 하나 추출 제한 시간(초)</summary>
    private const int FrameTimeoutSeconds = 90;

    /// <summary>관찰표 한 칸 기본 폭(픽셀)</summary>
    private const int DefaultCellWidth = 480;

    /// <summary>관찰표 칸 아래 시각 글자 영역 높이</summary>
    private const int LabelHeight = 22;

    /// <summary>메모 최대 길이</summary>
    private const int MaximumNoteLength = 8000;

    /// <summary>저장소 루트</summary>
    private readonly string _repository;

    /// <summary>영상별 기록 폴더의 부모 (extracted/youtube)</summary>
    public string Root { get; }

    /// <summary>저장소를 지정해 기록 위치를 정한다</summary>
    /// <param name="repository">NetstormReborn 저장소</param>
    public YouTubeAnalyzer(string repository)
    {
        _repository = Path.GetFullPath(repository);
        Root = Path.Combine(_repository, "extracted", "youtube");
        SessionStore.RejectReparse(Root);
    }

    /// <summary>`youtube_*` 도구를 실행한다. 오류는 호출자가 구조화된 결과로 바꾼다.</summary>
    /// <param name="tool">도구 이름</param>
    /// <param name="request">요청</param>
    /// <param name="cancellation">취소</param>
    public Task<AnalysisResult> ExecuteAsync(string tool, AnalysisRequest request, CancellationToken cancellation) => tool switch
    {
        "youtube_probe" => ProbeAsync(request, cancellation),
        "youtube_list" => ListAsync(request, cancellation),
        "youtube_frames" => FramesAsync(request, cancellation),
        "youtube_clip" => ClipAsync(request, cancellation),
        "youtube_note" => Task.FromResult(Note(request)),
        "youtube_videos" => Task.FromResult(Videos()),
        _ => throw new ArgumentException($"알 수 없는 도구: {tool}"),
    };

    /// <summary>영상 ID 의 기록 폴더 (링크 우회 금지)</summary>
    /// <param name="id">영상 ID</param>
    public string VideoDirectory(string id)
    {
        if (!YouTubeVideo.IsVideoId(id))
        {
            throw new ArgumentException("유효한 YouTube 영상 ID 가 아닙니다.");
        }
        string directory = Path.Combine(Root, id);
        SessionStore.RejectReparse(directory);
        return directory;
    }

    /// <summary>요청의 url 또는 videoId 에서 영상 ID 를 얻는다</summary>
    private static string RequireVideoId(AnalysisRequest request)
    {
        string input = request.Url.Length > 0 ? request.Url : request.VideoId;
        if (!YouTubeVideo.TryParseVideo(input, out string id, out _))
        {
            throw new ArgumentException("url 에 YouTube 영상 주소(watch?v=·youtu.be·shorts 등) 또는 videoId 에 11자 영상 ID 를 주세요.");
        }
        return id;
    }

    /// <summary>
    /// 메타데이터·챕터·SponsorBlock 구간을 받아 info.json 으로 저장한다. 프레임·구간 도구보다 먼저 부른다.
    /// </summary>
    private async Task<AnalysisResult> ProbeAsync(AnalysisRequest request, CancellationToken cancellation)
    {
        string id = RequireVideoId(request);
        YouTubeVideo.TryParseVideo(request.Url, out _, out double? urlStart);
        ProcessOutput output = await RunAsync(Tool(YtDlpVariable, "yt-dlp"),
            ["--no-warnings", "--no-playlist", "--skip-download", "--sponsorblock-mark", "all", "-J", WatchUrl(id)],
            TimeSpan.FromSeconds(ProbeTimeoutSeconds), cancellation);
        output.ThrowIfFailed("yt-dlp 메타데이터 조회");
        JsonNode info = JsonNode.Parse(output.StdoutText) ?? throw new InvalidDataException("yt-dlp 가 빈 JSON 을 돌려줬습니다.");
        VideoInfo video = VideoInfo.From(info);
        var warnings = new List<string>(video.Warnings);
        if (output.StderrText.Trim().Length > 0)
        {
            warnings.Add("yt-dlp 경고: " + Truncate(output.StderrText.Trim(), 600));
        }
        string directory = VideoDirectory(id);
        Directory.CreateDirectory(directory);
        using FileStream videoLock = LockVideo(directory);
        SessionStore.WriteSmallFile(Path.Combine(directory, "info.json"),
            JsonSerializer.SerializeToUtf8Bytes(video with { Warnings = warnings }, IndentedJson));
        AppendReport(directory, $"## 조회 {DateTimeOffset.UtcNow:yyyy-MM-dd HH:mm:ss} UTC\n\n"
            + $"- 제목: {video.Title}\n- 채널: {video.Channel}\n- 길이: {YouTubeVideo.FormatTime(video.Duration)} ({video.Duration:0.#}초)\n"
            + $"- 원본 화질: {video.Width}×{video.Height} {video.Fps:0.#}fps\n- 게임 화면이 아닌 구간: {video.Segments.Count(s => YouTubeVideo.NonContentCategories.Contains(s.Category))}개\n"
            + string.Concat(warnings.Select(w => $"- 주의: {w}\n")) + "\n");
        return new(new
        {
            videoId = id,
            video.Title,
            video.Channel,
            video.UploadDate,
            durationSeconds = video.Duration,
            resolution = $"{video.Width}x{video.Height}",
            video.Fps,
            video.Availability,
            video.LiveStatus,
            urlStartSeconds = urlStart,
            chapters = video.Chapters,
            nonContentSegments = video.Segments.Where(s => YouTubeVideo.NonContentCategories.Contains(s.Category)),
            ads = AdPolicyText,
            warnings,
            directory,
            report = Path.Combine(directory, "report.md"),
        });
    }

    /// <summary>광고 처리 방식 요약 (응답에 넣어 AI 가 영상 시각을 잘못 해석하지 않게 한다)</summary>
    private const string AdPolicyText =
        "재생기 광고(앞·중간·끝)는 별도 영상이라 원본 스트림에 없다. 영상 시각 = 업로드 원본 시각이다. "
        + "서버가 광고를 이어 붙였으면 스트림이 메타데이터보다 길어지므로 youtube_frames 가 막는다. "
        + "업로더가 영상 안에 넣은 협찬·홍보·인트로 등은 SponsorBlock 구간(nonContentSegments)으로 표시한다.";

    /// <summary>채널·재생목록의 영상 목록 (평면 조회, 영상별 상세는 받지 않음)</summary>
    private async Task<AnalysisResult> ListAsync(AnalysisRequest request, CancellationToken cancellation)
    {
        if (!YouTubeVideo.TryParseList(request.Url, out string url))
        {
            throw new ArgumentException("url 에 채널(@핸들·/channel/·/c/·/user/, 탭 videos·streams·shorts·playlists) 또는 재생목록(playlist?list=) 주소를 주세요.");
        }
        int limit = request.Limit == 0 ? DefaultListLimit : request.Limit;
        if (limit is < 1 or > MaximumListLimit)
        {
            throw new ArgumentException($"limit 는 1~{MaximumListLimit} 입니다.");
        }
        ProcessOutput output = await RunAsync(Tool(YtDlpVariable, "yt-dlp"),
            ["--no-warnings", "--flat-playlist", "--playlist-end", limit.ToString(CultureInfo.InvariantCulture), "-J", url],
            TimeSpan.FromSeconds(ProbeTimeoutSeconds), cancellation);
        output.ThrowIfFailed("yt-dlp 목록 조회");
        JsonNode info = JsonNode.Parse(output.StdoutText) ?? throw new InvalidDataException("yt-dlp 가 빈 JSON 을 돌려줬습니다.");
        var entries = new List<object>();
        // 평면 목록의 항목에서 영상 ID·제목·길이만 옮긴다
        foreach (JsonNode? entry in info["entries"]?.AsArray() ?? [])
        {
            string? id = entry?["id"]?.GetValue<string>();
            if (id == null || !YouTubeVideo.IsVideoId(id))
            {
                continue;
            }
            entries.Add(new
            {
                videoId = id,
                title = entry!["title"]?.GetValue<string>(),
                durationSeconds = Number(entry["duration"]),
                url = WatchUrl(id),
            });
        }
        return new(new { source = url, title = info["title"]?.GetValue<string>(), count = entries.Count, limit, entries });
    }

    /// <summary>
    /// 지정 시각의 프레임을 원격 스트림에서 뽑아 PNG 로 저장하고, 여러 장이면 시각을 적은 관찰표 한 장을 만든다.
    /// 시각은 times(쉼표 목록) 또는 start·step·count 로 준다. region 은 원본 해상도 기준 x,y,width,height 잘라내기다.
    /// </summary>
    private async Task<AnalysisResult> FramesAsync(AnalysisRequest request, CancellationToken cancellation)
    {
        string id = RequireVideoId(request);
        string directory = VideoDirectory(id);
        VideoInfo video = LoadInfo(directory);
        IReadOnlyList<double> times = FrameTimes(request, video.Duration);
        int height = CheckHeight(request.MaxHeight, DefaultFrameHeight);
        string crop = CropFilter(request.Region);
        Directory.CreateDirectory(Path.Combine(directory, "frames"));
        using FileStream videoLock = LockVideo(directory);
        StreamInfo stream = await StreamAsync(directory, id, height, video, request.AllowDurationMismatch, cancellation);
        var frames = new List<FrameRecord>();
        bool refreshed = false;
        // 시각마다 원격 스트림을 그 위치로 찾아가 한 장만 디코딩한다
        foreach (double time in times)
        {
            try
            {
                frames.Add(await FrameAsync(directory, height, crop, time, video, stream, cancellation));
            }
            catch (InvalidOperationException) when (!refreshed)
            {
                // 스트림 주소가 만료·거부됐을 수 있으므로 한 번만 새로 받아 다시 시도한다
                refreshed = true;
                stream = await StreamAsync(directory, id, height, video, request.AllowDurationMismatch, cancellation, forceRefresh: true);
                frames.Add(await FrameAsync(directory, height, crop, time, video, stream, cancellation));
            }
        }
        string imagePath;
        string? sheet = null;
        if (frames.Count == 1)
        {
            imagePath = Path.GetFullPath(Path.Combine(directory, frames[0].Path));
        }
        else
        {
            int columns = request.Columns is > 0 and <= 12 ? request.Columns : Math.Min(4, frames.Count);
            int cellWidth = request.CellWidth is >= 120 and <= 1920 ? request.CellWidth : DefaultCellWidth;
            sheet = ContactSheet(directory, frames, columns, cellWidth);
            imagePath = Path.GetFullPath(Path.Combine(directory, sheet));
        }
        AppendReport(directory, $"### 프레임 {DateTimeOffset.UtcNow:yyyy-MM-dd HH:mm:ss} UTC ({stream.Width}×{stream.Height}, format {stream.FormatId})\n\n"
            + string.Concat(frames.Select(f => $"- {YouTubeVideo.FormatTime(f.Time)} → [{f.Sha256[..12]}]({f.Path}){(f.Segment != null ? $" (게임 화면 아닐 수 있음: {f.Segment.Category})" : "")}\n"))
            + (sheet != null ? $"- 관찰표: [{Path.GetFileName(sheet)}]({sheet})\n" : "") + "\n");
        var data = new
        {
            videoId = id,
            stream = new { stream.FormatId, stream.Width, stream.Height, stream.Fps, stream.StreamDuration, metadataDuration = video.Duration },
            frames = frames.Select(f => new
            {
                time = f.Time,
                label = YouTubeVideo.FormatTime(f.Time),
                path = Path.GetFullPath(Path.Combine(directory, f.Path)),
                f.Sha256,
                f.Width,
                f.Height,
                f.Reused,
                nonContentSegment = f.Segment,
            }),
            sheet = sheet == null ? null : Path.GetFullPath(Path.Combine(directory, sheet)),
            report = Path.Combine(directory, "report.md"),
        };
        return new(data, request.IncludeImage ? imagePath : null);
    }

    /// <summary>
    /// start~end 구간을 원본 프레임률 그대로 mp4 로 저장한다 (애니메이션·이동 속도처럼 프레임 단위 측정용).
    /// 시작을 정확히 맞추려고 다시 인코딩하며, 파일은 50 MB 미만이어야 한다.
    /// </summary>
    private async Task<AnalysisResult> ClipAsync(AnalysisRequest request, CancellationToken cancellation)
    {
        string id = RequireVideoId(request);
        string directory = VideoDirectory(id);
        VideoInfo video = LoadInfo(directory);
        double start = request.Start ?? throw new ArgumentException("start(초) 가 필요합니다.");
        double end = request.End ?? throw new ArgumentException("end(초) 가 필요합니다.");
        if (start < 0 || end <= start || end - start > MaximumClipSeconds || (video.Duration > 0 && end > video.Duration))
        {
            throw new ArgumentException($"0 ≤ start < end ≤ 영상 길이({video.Duration:0.#}초), 길이 {MaximumClipSeconds}초 이하여야 합니다.");
        }
        int height = CheckHeight(request.MaxHeight, DefaultClipHeight);
        var warnings = video.Segments.Where(s => YouTubeVideo.NonContentCategories.Contains(s.Category) && s.Start < end && s.End > start)
            .Select(s => $"게임 화면이 아닐 수 있는 구간과 겹침: {s.Category} {YouTubeVideo.FormatTime(s.Start)}~{YouTubeVideo.FormatTime(s.End)}")
            .ToList();
        Directory.CreateDirectory(Path.Combine(directory, "clips"));
        using FileStream videoLock = LockVideo(directory);
        StreamInfo stream = await StreamAsync(directory, id, height, video, request.AllowDurationMismatch, cancellation);
        string name = string.Create(CultureInfo.InvariantCulture,
            $"clips/{start:000000.000}-{end:000000.000}-h{stream.Height}{(request.IncludeAudio ? "-a" : "")}.mp4");
        string path = Path.GetFullPath(Path.Combine(directory, name));
        SessionStore.RejectReparse(path);
        bool reused = File.Exists(path);
        if (!reused)
        {
            string temporary = path + ".part.mp4";
            var args = new List<string> { "-v", "error", "-y", "-ss", Invariant(start), "-i", stream.Url };
            if (request.IncludeAudio && stream.AudioUrl != null)
            {
                args.AddRange(["-ss", Invariant(start), "-i", stream.AudioUrl]);
            }
            args.AddRange(["-t", Invariant(end - start), "-map", "0:v:0"]);
            if (request.IncludeAudio && stream.AudioUrl != null)
            {
                args.AddRange(["-map", "1:a:0", "-c:a", "aac", "-b:a", "96k"]);
            }
            args.AddRange(["-c:v", "libx264", "-preset", "veryfast", "-crf", "26", "-pix_fmt", "yuv420p",
                "-fs", (SessionStore.FileLimitBytes - 1_000_000).ToString(CultureInfo.InvariantCulture), temporary]);
            ProcessOutput output = await RunAsync(Tool(FfmpegVariable, "ffmpeg"), args,
                TimeSpan.FromSeconds(120 + (end - start) * 4), cancellation);
            output.ThrowIfFailed("ffmpeg 구간 저장");
            double saved = await DurationAsync(temporary, cancellation);
            if (saved < (end - start) - 1)
            {
                File.Delete(temporary);
                throw new InvalidOperationException($"50 MB 한도로 {saved:0.#}초에서 잘렸습니다. 구간을 줄이거나 maxHeight 를 낮추세요.");
            }
            File.Move(temporary, path);
        }
        AppendReport(directory, $"### 구간 저장 {YouTubeVideo.FormatTime(start)}~{YouTubeVideo.FormatTime(end)} → [{Path.GetFileName(name)}]({name})\n\n"
            + string.Concat(warnings.Select(w => $"- 주의: {w}\n")) + "\n");
        return new(new
        {
            videoId = id,
            path,
            start,
            end,
            bytes = new FileInfo(path).Length,
            reused,
            stream = new { stream.FormatId, stream.Width, stream.Height, stream.Fps },
            timeMapping = "clip 안의 시각 t 는 영상 시각 start + t 와 같다 (시작 프레임을 다시 인코딩해 맞춤)",
            warnings,
        });
    }

    /// <summary>AI 의 해석 메모를 영상 기록에 남긴다 (영상 시각·증거 PNG 해시 연결)</summary>
    private AnalysisResult Note(AnalysisRequest request)
    {
        string id = RequireVideoId(request);
        string directory = VideoDirectory(id);
        if (string.IsNullOrWhiteSpace(request.Note) || request.Note.Length > MaximumNoteLength)
        {
            throw new ArgumentException($"note 는 1~{MaximumNoteLength}자입니다.");
        }
        if (!File.Exists(Path.Combine(directory, "info.json")))
        {
            throw new InvalidOperationException("먼저 youtube_probe 로 영상을 조회하세요.");
        }
        string? evidence = null;
        if (request.EvidenceHash.Length != 0)
        {
            if (!System.Text.RegularExpressions.Regex.IsMatch(request.EvidenceHash, "^[a-f0-9]{64}$"))
            {
                throw new ArgumentException("evidenceHash 는 youtube_frames 가 돌려준 sha256 입니다.");
            }
            evidence = $"frames/{request.EvidenceHash}.png";
            if (!File.Exists(Path.Combine(directory, evidence)))
            {
                throw new ArgumentException("이 영상에서 뽑은 프레임이 아닙니다.");
            }
        }
        using FileStream videoLock = LockVideo(directory);
        var entry = new { utc = DateTimeOffset.UtcNow, time = request.Time, note = request.Note, evidence, source = "AI 의 해석; 영상 프레임과 대조 필요" };
        AppendLine(Path.Combine(directory, "notes.jsonl"), JsonSerializer.Serialize(entry, SessionStore.Json));
        AppendReport(directory, $"### 메모{(request.Time is double t ? $" @ {YouTubeVideo.FormatTime(t)}" : "")}\n\n{request.Note}\n"
            + (evidence != null ? $"\n증거: [{request.EvidenceHash[..12]}]({evidence})\n" : "") + "\n");
        return new(new { videoId = id, notes = Path.Combine(directory, "notes.jsonl"), report = Path.Combine(directory, "report.md") });
    }

    /// <summary>이미 조회한 영상 목록 (기록 폴더 기준)</summary>
    private AnalysisResult Videos()
    {
        var items = new List<object>();
        if (Directory.Exists(Root))
        {
            // 영상 ID 이름의 폴더 중 info.json 이 있는 것만 나열한다
            foreach (string directory in Directory.GetDirectories(Root).OrderBy(d => d, StringComparer.Ordinal))
            {
                string id = Path.GetFileName(directory);
                string info = Path.Combine(directory, "info.json");
                if (!YouTubeVideo.IsVideoId(id) || !File.Exists(info))
                {
                    continue;
                }
                VideoInfo video = LoadInfo(directory);
                items.Add(new { videoId = id, video.Title, durationSeconds = video.Duration, report = Path.Combine(directory, "report.md") });
            }
        }
        return new(new { root = Root, videos = items });
    }

    /// <summary>요청에서 프레임 시각 목록을 만들고 범위를 검사한다</summary>
    private static IReadOnlyList<double> FrameTimes(AnalysisRequest request, double duration)
    {
        IReadOnlyList<double> times;
        if (request.Times.Length > 0)
        {
            times = YouTubeVideo.ParseTimes(request.Times);
        }
        else if (request.Start is double start && request.Step is double step && request.Count > 0)
        {
            if (step <= 0)
            {
                throw new ArgumentException("step 은 0 보다 커야 합니다.");
            }
            times = [.. Enumerable.Range(0, request.Count).Select(i => start + step * i)];
        }
        else
        {
            throw new ArgumentException("times(쉼표 목록) 또는 start·step·count 가 필요합니다.");
        }
        if (times.Count is 0 or > MaximumFrames)
        {
            throw new ArgumentException($"프레임은 1~{MaximumFrames}장입니다.");
        }
        if (duration > 0 && times.Any(t => t >= duration))
        {
            throw new ArgumentException($"영상 길이({duration:0.#}초)를 넘는 시각이 있습니다.");
        }
        return times;
    }

    /// <summary>세로 해상도 인자를 검사한다 (0 이면 기본값)</summary>
    private static int CheckHeight(int requested, int fallback)
    {
        int height = requested == 0 ? fallback : requested;
        return height is >= 144 and <= MaximumHeight ? height : throw new ArgumentException($"maxHeight 는 144~{MaximumHeight} 입니다.");
    }

    /// <summary>region "x,y,width,height" 를 ffmpeg crop 필터로 바꾼다 (빈 값이면 잘라내지 않음)</summary>
    private static string CropFilter(string region)
    {
        if (string.IsNullOrWhiteSpace(region))
        {
            return "";
        }
        int[] values = region.Split(',').Select(v => int.TryParse(v.Trim(), out int n) ? n : -1).ToArray();
        if (values.Length != 4 || values.Any(v => v < 0) || values[2] == 0 || values[3] == 0)
        {
            throw new ArgumentException("region 은 원본 해상도 기준 x,y,width,height 입니다.");
        }
        return $"crop={values[2]}:{values[3]}:{values[0]}:{values[1]}";
    }

    /// <summary>
    /// 지정 높이 이하의 영상 전용 스트림 주소를 얻는다(캐시, 만료 10분 전 갱신). 처음 받을 때 스트림 길이를 재서
    /// 서버 삽입 광고가 의심되면(메타데이터보다 길면) 막는다.
    /// </summary>
    /// <param name="forceRefresh">캐시를 무시하고 새로 받는다</param>
    private async Task<StreamInfo> StreamAsync(string directory, string id, int height, VideoInfo video, bool allowMismatch,
        CancellationToken cancellation, bool forceRefresh = false)
    {
        string cache = Path.Combine(directory, $"stream-h{height}.json");
        if (File.Exists(cache) && !forceRefresh)
        {
            StreamInfo? cached = JsonSerializer.Deserialize<StreamInfo>(File.ReadAllBytes(cache), IndentedJson);
            if (cached != null && cached.ExpiresUtc > DateTimeOffset.UtcNow.AddMinutes(StreamRefreshMarginMinutes))
            {
                return CheckStitchedAds(cached, video, allowMismatch);
            }
        }
        // avc1(H.264)을 먼저 고른다: av01·vp9 보다 디코딩이 빨라 원격 탐색이 빠르다. HLS(m3u8)보다 https 단일 파일이 탐색에 유리하다.
        string selector = $"bestvideo[height<={height}][protocol=https][vcodec^=avc1]/bestvideo[height<={height}][protocol=https]"
            + $"/best[height<={height}][protocol=https]/best[height<={height}]";
        ProcessOutput output = await RunAsync(Tool(YtDlpVariable, "yt-dlp"),
            ["--no-warnings", "--no-playlist", "-f", selector, "-J", WatchUrl(id)],
            TimeSpan.FromSeconds(ProbeTimeoutSeconds), cancellation);
        output.ThrowIfFailed("yt-dlp 스트림 주소 조회");
        JsonNode info = JsonNode.Parse(output.StdoutText) ?? throw new InvalidDataException("yt-dlp 가 빈 JSON 을 돌려줬습니다.");
        JsonNode chosen = info["requested_formats"]?.AsArray().FirstOrDefault(f => f?["vcodec"]?.GetValue<string>() != "none") ?? info;
        string url = chosen["url"]?.GetValue<string>() ?? throw new InvalidDataException("스트림 주소가 없습니다.");
        string? audio = AudioUrl(info);
        double streamDuration = await DurationAsync(url, cancellation);
        var stream = new StreamInfo(url, audio, chosen["format_id"]?.GetValue<string>() ?? "?",
            (int)(Number(chosen["width"]) ?? 0), (int)(Number(chosen["height"]) ?? 0), Number(chosen["fps"]) ?? 0,
            streamDuration, YouTubeVideo.StreamExpiry(url) ?? DateTimeOffset.UtcNow.AddHours(1));
        SessionStore.WriteSmallFile(cache, JsonSerializer.SerializeToUtf8Bytes(stream, IndentedJson));
        return CheckStitchedAds(stream, video, allowMismatch);
    }

    /// <summary>오디오 전용 스트림 주소 (구간 저장에 소리를 넣을 때). 영상 형식 목록에서 가장 좋은 https 오디오를 고른다.</summary>
    private static string? AudioUrl(JsonNode info)
    {
        JsonNode? best = null;
        double bestRate = -1;
        // 형식 목록에서 영상이 없는 https 오디오 중 비트레이트가 가장 높은 것을 고른다
        foreach (JsonNode? format in info["formats"]?.AsArray() ?? [])
        {
            if (format?["vcodec"]?.GetValue<string>() != "none" || format["acodec"]?.GetValue<string>() is null or "none"
                || format["protocol"]?.GetValue<string>() != "https")
            {
                continue;
            }
            double rate = Number(format["abr"]) ?? 0;
            if (rate > bestRate)
            {
                best = format;
                bestRate = rate;
            }
        }
        return best?["url"]?.GetValue<string>();
    }

    /// <summary>스트림이 메타데이터보다 길면(서버 삽입 광고 의심) 막는다. allowMismatch 면 통과시킨다.</summary>
    private static StreamInfo CheckStitchedAds(StreamInfo stream, VideoInfo video, bool allowMismatch)
    {
        if (YouTubeVideo.PossiblyStitchedAds(video.Duration, stream.StreamDuration) && !allowMismatch)
        {
            throw new InvalidOperationException(
                $"스트림 길이({stream.StreamDuration:0.#}초)가 영상 길이({video.Duration:0.#}초)보다 깁니다. 서버가 광고를 이어 붙였을 수 있어 "
                + "영상 시각이 어긋납니다. 잠시 뒤 다시 시도하거나(스트림 캐시 파일 stream-h*.json 삭제) allowDurationMismatch=true 로 위험을 감수하고 진행하세요.");
        }
        return stream;
    }

    /// <summary>프레임 한 장을 뽑아 해시 이름으로 저장한다 (같은 시각·화질·잘라내기는 다시 받지 않음)</summary>
    private async Task<FrameRecord> FrameAsync(string directory, int height, string crop, double time, VideoInfo video,
        StreamInfo stream, CancellationToken cancellation)
    {
        string key = $"{Invariant(time)}|h{height}|{crop}";
        string index = Path.Combine(directory, "frames.jsonl");
        VideoSegment? segment = YouTubeVideo.NonContentAt(video.Segments, time);
        if (File.Exists(index))
        {
            // 이전에 같은 조건으로 뽑은 프레임이 있으면 그 파일을 쓴다
            foreach (string line in File.ReadLines(index).Reverse())
            {
                FrameIndexEntry? entry = JsonSerializer.Deserialize<FrameIndexEntry>(line, SessionStore.Json);
                if (entry?.Key == key && File.Exists(Path.Combine(directory, $"frames/{entry.Sha256}.png")))
                {
                    return new FrameRecord(time, $"frames/{entry.Sha256}.png", entry.Sha256, entry.Width, entry.Height, true, segment);
                }
            }
        }
        byte[] png = await ExtractPngAsync(stream.Url, time, crop, cancellation);
        string sha = Convert.ToHexString(SHA256.HashData(png)).ToLowerInvariant();
        string relative = $"frames/{sha}.png";
        string path = Path.Combine(directory, relative);
        if (!File.Exists(path))
        {
            SessionStore.WriteSmallFile(path, png);
        }
        using var image = new Bitmap(new MemoryStream(png));
        AppendLine(index, JsonSerializer.Serialize(new FrameIndexEntry(key, time, sha, image.Width, image.Height, stream.FormatId), SessionStore.Json));
        return new FrameRecord(time, relative, sha, image.Width, image.Height, false, segment);
    }

    /// <summary>
    /// ffmpeg 로 원격 스트림의 지정 시각 한 장을 PNG 로 받는다. 스트림 주소가 거부되면(만료 등) 오류로 알린다.
    /// </summary>
    private async Task<byte[]> ExtractPngAsync(string url, double time, string crop, CancellationToken cancellation)
    {
        var args = new List<string> { "-v", "error", "-ss", Invariant(time), "-i", url, "-frames:v", "1" };
        if (crop.Length > 0)
        {
            args.AddRange(["-vf", crop]);
        }
        args.AddRange(["-f", "image2pipe", "-vcodec", "png", "-"]);
        ProcessOutput output = await RunAsync(Tool(FfmpegVariable, "ffmpeg"), args, TimeSpan.FromSeconds(FrameTimeoutSeconds), cancellation);
        output.ThrowIfFailed($"ffmpeg 프레임 추출({YouTubeVideo.FormatTime(time)}) — 스트림 주소가 만료됐으면 stream-h*.json 을 지우고 다시 시도");
        return output.Stdout.Length > 0 ? output.Stdout : throw new InvalidOperationException($"{YouTubeVideo.FormatTime(time)} 에서 프레임을 얻지 못했습니다.");
    }

    /// <summary>ffprobe 로 파일·원격 스트림의 길이(초)를 잰다</summary>
    private async Task<double> DurationAsync(string input, CancellationToken cancellation)
    {
        ProcessOutput output = await RunAsync(Tool(FfprobeVariable, "ffprobe"),
            ["-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0", input], TimeSpan.FromSeconds(FrameTimeoutSeconds), cancellation);
        output.ThrowIfFailed("ffprobe 길이 측정");
        return double.TryParse(output.StdoutText.Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out double seconds) ? seconds : 0;
    }

    /// <summary>프레임들을 시각·구간 표시와 함께 한 장으로 모은다 (해시 이름으로 sheets/ 에 저장)</summary>
    private static string ContactSheet(string directory, IReadOnlyList<FrameRecord> frames, int columns, int cellWidth)
    {
        FrameRecord first = frames[0];
        int cellHeight = (int)Math.Round(cellWidth * (double)first.Height / first.Width);
        int rows = (frames.Count + columns - 1) / columns;
        using var sheet = new Bitmap(columns * cellWidth, rows * (cellHeight + LabelHeight), PixelFormat.Format24bppRgb);
        using (Graphics graphics = Graphics.FromImage(sheet))
        using (var font = new Font("Segoe UI", 10f, FontStyle.Bold, GraphicsUnit.Pixel))
        using (var warning = new SolidBrush(Color.Orange))
        {
            graphics.Clear(Color.FromArgb(32, 32, 32));
            graphics.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBilinear;
            // 프레임을 왼쪽 위부터 시간순으로 놓고 아래에 시각을 쓴다
            for (int i = 0; i < frames.Count; i++)
            {
                int x = i % columns * cellWidth;
                int y = i / columns * (cellHeight + LabelHeight);
                using (var image = new Bitmap(Path.Combine(directory, frames[i].Path)))
                {
                    graphics.DrawImage(image, new Rectangle(x, y, cellWidth, cellHeight));
                }
                string label = YouTubeVideo.FormatTime(frames[i].Time) + (frames[i].Segment != null ? $"  [{frames[i].Segment!.Category}]" : "");
                graphics.DrawString(label, font, frames[i].Segment != null ? warning : Brushes.White, x + 4, y + cellHeight + 3);
            }
        }
        using var buffer = new MemoryStream();
        sheet.Save(buffer, ImageFormat.Png);
        byte[] png = buffer.ToArray();
        string relative = $"sheets/{Convert.ToHexString(SHA256.HashData(png)).ToLowerInvariant()}.png";
        Directory.CreateDirectory(Path.Combine(directory, "sheets"));
        string path = Path.Combine(directory, relative);
        if (!File.Exists(path))
        {
            SessionStore.WriteSmallFile(path, png);
        }
        return relative;
    }

    /// <summary>info.json 을 읽는다 (없으면 조회부터 하라고 알린다)</summary>
    private static VideoInfo LoadInfo(string directory)
    {
        string path = Path.Combine(directory, "info.json");
        if (!File.Exists(path))
        {
            throw new InvalidOperationException("먼저 youtube_probe 로 영상을 조회하세요 (길이·광고 판정 기준이 필요합니다).");
        }
        return JsonSerializer.Deserialize<VideoInfo>(File.ReadAllBytes(path), IndentedJson)
            ?? throw new InvalidDataException("info.json 을 읽지 못했습니다.");
    }

    /// <summary>같은 영상 폴더를 두 요청이 동시에 쓰지 못하게 잠근다</summary>
    private static FileStream LockVideo(string directory)
    {
        Directory.CreateDirectory(directory);
        try
        {
            return new FileStream(Path.Combine(directory, "video.lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
        }
        catch (IOException)
        {
            throw new InvalidOperationException("같은 영상에 대한 다른 요청이 진행 중입니다. 끝난 뒤 다시 시도하세요.");
        }
    }

    /// <summary>report.md 에 Markdown 을 덧붙인다 (없으면 머리글부터)</summary>
    private static void AppendReport(string directory, string markdown)
    {
        string path = Path.Combine(directory, "report.md");
        SessionStore.RejectReparse(path);
        if (!File.Exists(path))
        {
            File.WriteAllText(path, $"# YouTube 영상 분석 기록: {Path.GetFileName(directory)}\n\n원본 게임을 실행하지 않은 영상 관찰이다. "
                + "영상 시각은 업로드 원본 기준이며 재생기 광고는 포함되지 않는다. AI 메모는 해석이므로 프레임과 대조한다.\n\n", new UTF8Encoding(false));
        }
        File.AppendAllText(path, markdown, new UTF8Encoding(false));
    }

    /// <summary>JSONL 파일에 한 줄 덧붙인다</summary>
    private static void AppendLine(string path, string line)
    {
        SessionStore.RejectReparse(path);
        File.AppendAllText(path, line + "\n", new UTF8Encoding(false));
    }

    /// <summary>영상 ID 의 표준 시청 주소</summary>
    private static string WatchUrl(string id) => $"https://www.youtube.com/watch?v={id}";

    /// <summary>소수점 표기를 문화권과 무관하게 쓴다</summary>
    private static string Invariant(double value) => value.ToString("0.###", CultureInfo.InvariantCulture);

    /// <summary>긴 문자열을 잘라 응답 크기를 제한한다</summary>
    private static string Truncate(string text, int length) => text.Length <= length ? text : text[..length] + "…";

    /// <summary>JSON 숫자 노드를 double 로 (없거나 숫자가 아니면 null)</summary>
    internal static double? Number(JsonNode? node) =>
        node is JsonValue value && value.TryGetValue(out double number) ? number
        : node is JsonValue other && other.TryGetValue(out long whole) ? whole : null;

    /// <summary>들여쓰기 JSON 설정 (기록 파일용)</summary>
    private static readonly JsonSerializerOptions IndentedJson = new(SessionStore.Json) { WriteIndented = true };

    /// <summary>
    /// 외부 프로그램 위치: 환경 변수 → PATH 순서. 없으면 준비 방법을 알린다.
    /// </summary>
    private static string Tool(string variable, string name)
    {
        string? configured = Environment.GetEnvironmentVariable(variable);
        if (!string.IsNullOrWhiteSpace(configured))
        {
            return File.Exists(configured) ? configured : throw new InvalidOperationException($"{variable} 가 가리키는 파일이 없습니다: {configured}");
        }
        // PATH 의 폴더마다 실행 파일을 찾는다
        foreach (string folder in (Environment.GetEnvironmentVariable("PATH") ?? "").Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries))
        {
            string candidate = Path.Combine(folder.Trim('"'), name + ".exe");
            if (File.Exists(candidate))
            {
                return candidate;
            }
        }
        throw new InvalidOperationException($"{name} 를 찾지 못했습니다. PREPARE.ps1 의 yt-dlp·FFmpeg 항목으로 설치하거나 {variable} 로 경로를 지정하세요.");
    }

    /// <summary>
    /// 외부 프로그램을 셸 없이 실행하고 표준 출력(바이트)·표준 오류를 모은다. 제한 시간·취소 때 프로세스 트리를 끝낸다.
    /// yt-dlp 가 UTF-8 로 쓰도록 파이썬 입출력 인코딩을 지정한다.
    /// </summary>
    private static async Task<ProcessOutput> RunAsync(string exe, IReadOnlyList<string> arguments, TimeSpan timeout, CancellationToken cancellation)
    {
        var start = new ProcessStartInfo(exe)
        {
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            RedirectStandardInput = true,
            CreateNoWindow = true,
            StandardErrorEncoding = Encoding.UTF8,
        };
        // 인자는 하나씩 넘겨 따옴표·공백 해석 문제를 피한다
        foreach (string argument in arguments)
        {
            start.ArgumentList.Add(argument);
        }
        start.Environment["PYTHONIOENCODING"] = "utf-8";
        start.Environment["PYTHONUTF8"] = "1";
        using var process = Process.Start(start) ?? throw new InvalidOperationException($"{Path.GetFileName(exe)} 를 시작하지 못했습니다.");
        process.StandardInput.Close();
        using var timer = CancellationTokenSource.CreateLinkedTokenSource(cancellation);
        timer.CancelAfter(timeout);
        using var stdout = new MemoryStream();
        Task copy = process.StandardOutput.BaseStream.CopyToAsync(stdout, timer.Token);
        Task<string> stderr = process.StandardError.ReadToEndAsync(timer.Token);
        try
        {
            await Task.WhenAll(copy, stderr, process.WaitForExitAsync(timer.Token));
        }
        catch (OperationCanceledException)
        {
            try { process.Kill(entireProcessTree: true); } catch (InvalidOperationException) { }
            if (cancellation.IsCancellationRequested) throw;
            throw new TimeoutException($"{Path.GetFileName(exe)} 가 {timeout.TotalSeconds:0}초 안에 끝나지 않았습니다.");
        }
        return new ProcessOutput(process.ExitCode, stdout.ToArray(), await stderr);
    }

    /// <summary>외부 프로그램 실행 결과</summary>
    private sealed record ProcessOutput(int ExitCode, byte[] Stdout, string StderrText)
    {
        /// <summary>표준 출력을 UTF-8 문자열로</summary>
        public string StdoutText => Encoding.UTF8.GetString(Stdout);

        /// <summary>실패면 표준 오류 끝부분을 담아 예외를 던진다</summary>
        public void ThrowIfFailed(string what)
        {
            if (ExitCode != 0)
            {
                throw new InvalidOperationException($"{what} 실패 (종료 코드 {ExitCode}): {Truncate(StderrText.Trim(), 800)}");
            }
        }
    }

    /// <summary>스트림 캐시 (stream-h&lt;높이&gt;.json)</summary>
    public sealed record StreamInfo(string Url, string? AudioUrl, string FormatId, int Width, int Height, double Fps,
        double StreamDuration, DateTimeOffset ExpiresUtc);

    /// <summary>frames.jsonl 한 줄: 같은 조건 프레임 재사용용</summary>
    private sealed record FrameIndexEntry(string Key, double Time, string Sha256, int Width, int Height, string FormatId);

    /// <summary>뽑은 프레임 한 장</summary>
    private sealed record FrameRecord(double Time, string Path, string Sha256, int Width, int Height, bool Reused, VideoSegment? Segment);
}

/// <summary>info.json 에 남기는 영상 메타데이터 (yt-dlp -J 에서 필요한 것만)</summary>
/// <param name="Id">영상 ID</param>
/// <param name="Title">제목</param>
/// <param name="Channel">채널</param>
/// <param name="UploadDate">올린 날 (YYYYMMDD)</param>
/// <param name="Duration">길이(초)</param>
/// <param name="Width">최고 화질 폭</param>
/// <param name="Height">최고 화질 높이</param>
/// <param name="Fps">최고 화질 프레임률</param>
/// <param name="Availability">공개 상태</param>
/// <param name="LiveStatus">생방송 상태</param>
/// <param name="Chapters">업로더 챕터</param>
/// <param name="Segments">SponsorBlock 구간</param>
/// <param name="Warnings">분석에 영향을 주는 주의 사항</param>
public sealed record VideoInfo(string Id, string Title, string Channel, string UploadDate, double Duration, int Width, int Height,
    double Fps, string Availability, string LiveStatus, IReadOnlyList<VideoSegment> Chapters, IReadOnlyList<VideoSegment> Segments,
    IReadOnlyList<string> Warnings)
{
    /// <summary>yt-dlp -J 결과에서 만든다</summary>
    /// <param name="info">yt-dlp 정보 JSON</param>
    public static VideoInfo From(JsonNode info)
    {
        string Text(string key) => info[key] is JsonValue value && value.TryGetValue(out string? text) ? text ?? "" : "";
        var warnings = new List<string>();
        string live = Text("live_status");
        if (live is "is_live" or "is_upcoming" or "post_live")
        {
            warnings.Add($"생방송 상태({live}): 길이·시각이 확정되지 않았을 수 있다.");
        }
        if (YouTubeAnalyzer.Number(info["age_limit"]) is > 0)
        {
            warnings.Add("연령 제한 영상: 로그인 쿠키 없이는 스트림을 못 받을 수 있다.");
        }
        if (info["_has_drm"] is JsonValue drm && drm.TryGetValue(out bool hasDrm) && hasDrm)
        {
            warnings.Add("DRM 보호 형식이 있다: 프레임 추출이 실패할 수 있다.");
        }
        var chapters = new List<VideoSegment>();
        // 업로더 챕터를 구간으로 옮긴다
        foreach (JsonNode? chapter in info["chapters"]?.AsArray() ?? [])
        {
            chapters.Add(new VideoSegment(YouTubeAnalyzer.Number(chapter?["start_time"]) ?? 0, YouTubeAnalyzer.Number(chapter?["end_time"]) ?? 0,
                "chapter", chapter?["title"]?.GetValue<string>() ?? ""));
        }
        var segments = new List<VideoSegment>();
        // SponsorBlock 구간을 옮긴다 (yt-dlp sponsorblock_chapters: start_time·end_time·category·title)
        foreach (JsonNode? segment in info["sponsorblock_chapters"]?.AsArray() ?? [])
        {
            segments.Add(new VideoSegment(YouTubeAnalyzer.Number(segment?["start_time"]) ?? 0, YouTubeAnalyzer.Number(segment?["end_time"]) ?? 0,
                segment?["category"]?.GetValue<string>() ?? "", segment?["title"]?.GetValue<string>() ?? ""));
        }
        return new VideoInfo(Text("id"), Text("title"), Text("channel"), Text("upload_date"), YouTubeAnalyzer.Number(info["duration"]) ?? 0,
            (int)(YouTubeAnalyzer.Number(info["width"]) ?? 0), (int)(YouTubeAnalyzer.Number(info["height"]) ?? 0),
            YouTubeAnalyzer.Number(info["fps"]) ?? 0, Text("availability"), live, chapters, segments, warnings);
    }
}
