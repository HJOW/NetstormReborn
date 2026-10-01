using System.Globalization;
using System.Text.RegularExpressions;

namespace Netstorm.AnalyzeManager;

/// <summary>영상 안에서 게임 화면이 아닌 구간 하나 (업로더 챕터·SponsorBlock 등).</summary>
/// <param name="Start">시작(초)</param>
/// <param name="End">끝(초)</param>
/// <param name="Category">분류 (SponsorBlock: sponsor·selfpromo·interaction·intro·outro·preview·filler·music_offtopic 등)</param>
/// <param name="Title">표시 이름</param>
public sealed record VideoSegment(double Start, double End, string Category, string Title);

/// <summary>
/// YouTube 주소·시각 문자열 해석과 광고 관련 판정. 네트워크·외부 프로그램을 쓰지 않으므로 단위 테스트로 검사한다.
/// </summary>
public static partial class YouTubeVideo
{
    /// <summary>
    /// 게임 화면이 아니라고 보는 SponsorBlock 분류. 업로더가 영상 안에 직접 넣은 협찬·자기 홍보·구독 요청·인트로·아웃트로 등이다.
    /// (YouTube 가 재생기에서 따로 트는 광고는 영상 스트림에 없다 — docs/analyze-manager.md "YouTube 영상 분석")
    /// </summary>
    public static readonly IReadOnlySet<string> NonContentCategories = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
    {
        "sponsor", "selfpromo", "interaction", "intro", "outro", "preview", "filler", "music_offtopic", "exclusive_access",
    };

    /// <summary>스트림 길이가 메타데이터 길이보다 이만큼(초) 넘게 길면 서버 삽입 광고를 의심한다.</summary>
    public const double StitchedAdToleranceSeconds = 2.0;

    /// <summary>위 허용치에 더해 영상 길이 대비 비율 허용치 (긴 영상의 반올림 차이 흡수)</summary>
    public const double StitchedAdToleranceRatio = 0.005;

    /// <summary>YouTube 영상 ID (11자, 영문·숫자·-·_)</summary>
    [GeneratedRegex("^[A-Za-z0-9_-]{11}$")]
    private static partial Regex VideoIdPattern();

    /// <summary>시각 표기 "1h2m3s" / "2m" / "75s"</summary>
    [GeneratedRegex(@"^(?:(\d+)h)?(?:(\d+)m)?(?:(\d+(?:\.\d+)?)s)?$", RegexOptions.IgnoreCase)]
    private static partial Regex UnitTimePattern();

    /// <summary>문자열이 영상 ID 형식인지</summary>
    /// <param name="value">검사할 값</param>
    public static bool IsVideoId(string value) => VideoIdPattern().IsMatch(value);

    /// <summary>
    /// 영상 주소에서 ID 와 시작 시각(t=·start=)을 꺼낸다. 받는 형식: youtube.com/watch?v=, youtu.be/ID, /shorts/ID, /embed/ID, /live/ID,
    /// m.·music.·www. 하위 도메인, youtube-nocookie.com, 또는 ID 자체. 그 밖의 사이트는 거부한다(임의 URL 을 내려받지 않도록).
    /// </summary>
    /// <param name="input">주소 또는 ID</param>
    /// <param name="id">영상 ID</param>
    /// <param name="startSeconds">주소에 있던 시작 시각 (없으면 null)</param>
    public static bool TryParseVideo(string input, out string id, out double? startSeconds)
    {
        id = "";
        startSeconds = null;
        string text = input.Trim();
        if (IsVideoId(text))
        {
            id = text;
            return true;
        }
        if (!TryYouTubeUri(text, out Uri? uri))
        {
            return false;
        }
        string host = uri!.Host.ToLowerInvariant();
        string[] parts = uri.AbsolutePath.Trim('/').Split('/', StringSplitOptions.RemoveEmptyEntries);
        Dictionary<string, string> query = ParseQuery(uri.Query);
        string? candidate = null;
        if (host == "youtu.be")
        {
            candidate = parts.FirstOrDefault();
        }
        else if (parts.Length == 1 && parts[0] == "watch")
        {
            candidate = query.GetValueOrDefault("v");
        }
        else if (parts.Length >= 2 && parts[0] is "shorts" or "embed" or "live" or "v")
        {
            candidate = parts[1];
        }
        if (candidate == null || !IsVideoId(candidate))
        {
            return false;
        }
        id = candidate;
        string? time = query.GetValueOrDefault("t") ?? query.GetValueOrDefault("start");
        if (time != null && TryParseTime(time, out double seconds))
        {
            startSeconds = seconds;
        }
        return true;
    }

    /// <summary>
    /// 채널·재생목록 주소인지 (영상 목록 조회용). 채널 핸들만 준 주소는 동영상 탭 주소로 바꿔 돌려준다.
    /// </summary>
    /// <param name="input">주소</param>
    /// <param name="normalized">목록 조회에 쓸 주소</param>
    public static bool TryParseList(string input, out string normalized)
    {
        normalized = "";
        if (!TryYouTubeUri(input.Trim(), out Uri? uri) || uri!.Host.ToLowerInvariant() == "youtu.be")
        {
            return false;
        }
        string[] parts = uri.AbsolutePath.Trim('/').Split('/', StringSplitOptions.RemoveEmptyEntries);
        Dictionary<string, string> query = ParseQuery(uri.Query);
        if (parts.Length == 1 && parts[0] == "playlist" && query.TryGetValue("list", out string? list)
            && Regex.IsMatch(list, "^[A-Za-z0-9_-]{2,64}$"))
        {
            normalized = $"https://www.youtube.com/playlist?list={list}";
            return true;
        }
        if (parts.Length == 0)
        {
            return false;
        }
        bool handle = parts[0].StartsWith('@') && Regex.IsMatch(parts[0], "^@[A-Za-z0-9._-]{1,100}$");
        bool named = parts.Length >= 2 && parts[0] is "channel" or "c" or "user" && Regex.IsMatch(parts[1], "^[A-Za-z0-9._-]{1,100}$");
        if (!handle && !named)
        {
            return false;
        }
        string root = handle ? parts[0] : $"{parts[0]}/{parts[1]}";
        string tab = parts.Length > (handle ? 1 : 2) ? parts[handle ? 1 : 2] : "videos";
        if (tab is not ("videos" or "streams" or "shorts" or "playlists"))
        {
            return false;
        }
        normalized = $"https://www.youtube.com/{root}/{tab}";
        return true;
    }

    /// <summary>YouTube 도메인의 http(s) 주소만 받는다</summary>
    private static bool TryYouTubeUri(string text, out Uri? uri)
    {
        uri = null;
        if (!Uri.TryCreate(text, UriKind.Absolute, out Uri? parsed) || parsed.Scheme is not ("https" or "http"))
        {
            return false;
        }
        string host = parsed.Host.ToLowerInvariant();
        bool allowed = host is "youtu.be" or "youtube.com" or "youtube-nocookie.com"
            || host.EndsWith(".youtube.com", StringComparison.Ordinal) || host.EndsWith(".youtube-nocookie.com", StringComparison.Ordinal);
        uri = allowed ? parsed : null;
        return allowed;
    }

    /// <summary>쿼리 문자열을 이름 → 값으로 나눈다 (처음 나온 값 우선)</summary>
    private static Dictionary<string, string> ParseQuery(string query)
    {
        var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        // "a=1&b=2" 를 차례로 나눈다
        foreach (string pair in query.TrimStart('?').Split('&', StringSplitOptions.RemoveEmptyEntries))
        {
            int equals = pair.IndexOf('=');
            string key = Uri.UnescapeDataString(equals < 0 ? pair : pair[..equals]);
            string value = equals < 0 ? "" : Uri.UnescapeDataString(pair[(equals + 1)..].Replace('+', ' '));
            result.TryAdd(key, value);
        }
        return result;
    }

    /// <summary>
    /// 시각 하나를 초로 읽는다. "612.5", "10:12.5", "1:02:03", "1h2m3s", "75s" 형식.
    /// </summary>
    /// <param name="text">시각 문자열</param>
    /// <param name="seconds">초</param>
    public static bool TryParseTime(string text, out double seconds)
    {
        seconds = 0;
        string value = text.Trim();
        if (value.Length == 0)
        {
            return false;
        }
        if (double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out seconds))
        {
            return seconds >= 0 && double.IsFinite(seconds);
        }
        if (value.Contains(':'))
        {
            string[] fields = value.Split(':');
            if (fields.Length > 3)
            {
                return false;
            }
            double total = 0;
            // 시·분·초를 왼쪽부터 60 진법으로 더한다 (마지막만 소수 허용)
            for (int i = 0; i < fields.Length; i++)
            {
                bool last = i == fields.Length - 1;
                if (!double.TryParse(fields[i], NumberStyles.Float, CultureInfo.InvariantCulture, out double part) || part < 0
                    || (!last && part != Math.Floor(part)) || (i > 0 && part >= 60))
                {
                    return false;
                }
                total = total * 60 + part;
            }
            seconds = total;
            return true;
        }
        Match match = UnitTimePattern().Match(value);
        if (!match.Success || match.Length == 0 || !(match.Groups[1].Success || match.Groups[2].Success || match.Groups[3].Success))
        {
            return false;
        }
        double hours = match.Groups[1].Success ? double.Parse(match.Groups[1].Value, CultureInfo.InvariantCulture) : 0;
        double minutes = match.Groups[2].Success ? double.Parse(match.Groups[2].Value, CultureInfo.InvariantCulture) : 0;
        double secs = match.Groups[3].Success ? double.Parse(match.Groups[3].Value, CultureInfo.InvariantCulture) : 0;
        seconds = hours * 3600 + minutes * 60 + secs;
        return true;
    }

    /// <summary>쉼표로 나눈 시각 목록을 초 목록으로 읽는다. 하나라도 틀리면 예외.</summary>
    /// <param name="text">"600, 10:12.5, 1h2m"</param>
    public static IReadOnlyList<double> ParseTimes(string text)
    {
        var result = new List<double>();
        // 쉼표로 나눈 항목을 차례로 해석한다
        foreach (string item in text.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
        {
            if (!TryParseTime(item, out double seconds))
            {
                throw new ArgumentException($"시각을 읽을 수 없습니다: {item} (예: 612.5, 10:12.5, 1:02:03, 1h2m3s)");
            }
            result.Add(seconds);
        }
        return result;
    }

    /// <summary>초를 "h:mm:ss.s" 또는 "mm:ss.s" 로 쓴다</summary>
    /// <param name="seconds">초</param>
    public static string FormatTime(double seconds)
    {
        var span = TimeSpan.FromSeconds(Math.Max(0, seconds));
        string tenths = (span.Milliseconds / 100).ToString(CultureInfo.InvariantCulture);
        return span.TotalHours >= 1
            ? $"{(int)span.TotalHours}:{span.Minutes:00}:{span.Seconds:00}.{tenths}"
            : $"{span.Minutes:00}:{span.Seconds:00}.{tenths}";
    }

    /// <summary>시각이 들어가는 게임 화면 아닌 구간 (없으면 null). 끝 시각은 포함하지 않는다.</summary>
    /// <param name="segments">구간 목록</param>
    /// <param name="seconds">시각</param>
    public static VideoSegment? NonContentAt(IEnumerable<VideoSegment> segments, double seconds) =>
        segments.FirstOrDefault(s => NonContentCategories.Contains(s.Category) && seconds >= s.Start && seconds < s.End);

    /// <summary>
    /// 받은 스트림의 실제 길이가 메타데이터 길이보다 눈에 띄게 길면 서버 쪽에서 광고를 이어 붙였을 가능성이 있다.
    /// 그 경우 영상 시각과 게임 시각이 어긋나므로 프레임 분석을 막는다.
    /// </summary>
    /// <param name="metadataSeconds">YouTube 가 알려 준 영상 길이</param>
    /// <param name="streamSeconds">스트림에서 잰 길이</param>
    public static bool PossiblyStitchedAds(double metadataSeconds, double streamSeconds) =>
        metadataSeconds > 0 && streamSeconds - metadataSeconds
            > Math.Max(StitchedAdToleranceSeconds, metadataSeconds * StitchedAdToleranceRatio);

    /// <summary>
    /// 스트림 URL 의 만료 시각 (쿼리 expire = 유닉스 초). 없으면 null.
    /// </summary>
    /// <param name="streamUrl">googlevideo 스트림 주소</param>
    public static DateTimeOffset? StreamExpiry(string streamUrl)
    {
        if (!Uri.TryCreate(streamUrl, UriKind.Absolute, out Uri? uri))
        {
            return null;
        }
        return ParseQuery(uri.Query).TryGetValue("expire", out string? value) && long.TryParse(value, out long unix)
            ? DateTimeOffset.FromUnixTimeSeconds(unix) : null;
    }
}
