using System.ComponentModel;
using System.Text.Json;
using ModelContextProtocol.Protocol;
using ModelContextProtocol.Server;

namespace Netstorm.AnalyzeManager;

/// <summary>
/// YouTube 영상 분석 MCP 도구 (원본 게임을 실행하지 않음, 인터넷 사용).
/// Windows 빌드는 게임 도구(<see cref="ExplorerTools"/>)와 함께, 리눅스용 YouTube 전용 빌드는 이것만 등록한다.
/// </summary>
[McpServerToolType]
public sealed class YouTubeTools
{
    /// <summary>YouTube 분석 엔진</summary>
    private readonly YouTubeAnalyzer _youtube;

    /// <summary>엔진을 연결한다.</summary>
    /// <param name="youtube">YouTube 분석 엔진</param>
    public YouTubeTools(YouTubeAnalyzer youtube) => _youtube = youtube;

    /// <summary>도구 결과를 MCP 텍스트/구조화 데이터/선택적 이미지로 변환한다 (게임 도구와 공용).</summary>
    /// <param name="result">엔진 결과</param>
    /// <param name="cancellation">취소</param>
    public static async Task<CallToolResult> ToCallToolResult(AnalysisResult result, CancellationToken cancellation)
    {
        var response = new CallToolResult
        {
            IsError = result.IsError,
            StructuredContent = JsonSerializer.SerializeToElement(result.Data, SessionStore.Json),
            Content = [new TextContentBlock { Text = JsonSerializer.Serialize(result.Data, SessionStore.Json) }],
        };
        if (result.ImagePath != null)
            response.Content.Add(ImageContentBlock.FromBytes(await File.ReadAllBytesAsync(result.ImagePath, cancellation), "image/png"));
        return response;
    }

    /// <summary>YouTube 도구를 실행해 MCP 결과로 바꾼다 (오류도 결과로)</summary>
    private async Task<CallToolResult> Run(string name, AnalysisRequest request, CancellationToken cancellation)
        => await ToCallToolResult(await _youtube.ExecuteSafeAsync(name, request, cancellation), cancellation);

    /// <summary>YouTube 영상의 메타데이터·챕터·광고 성격 구간을 조회해 기록 폴더를 만든다.</summary>
    [McpServerTool(Name = "youtube_probe", OpenWorld = true, Destructive = false)]
    [Description("YouTube 영상 주소(또는 11자 ID)의 제목·길이·화질·챕터와 SponsorBlock 구간(업로더가 영상 안에 넣은 협찬·홍보·인트로 등)을 조회해 extracted/youtube/<ID>/info.json 에 저장합니다. 원본 게임을 실행하지 않으므로 개발자 확인이 필요 없습니다. youtube_frames·youtube_clip 전에 먼저 호출하세요. 재생기 광고는 원본 스트림에 없으므로 영상 시각은 업로드 원본 기준입니다.")]
    public Task<CallToolResult> YouTubeProbe([Description("영상 주소(watch?v=·youtu.be·shorts·embed) 또는 videoId")] string url,
        CancellationToken cancellation = default)
        => Run("youtube_probe", new() { Url = url }, cancellation);

    /// <summary>채널·재생목록의 영상 목록을 얻는다.</summary>
    [McpServerTool(Name = "youtube_list", ReadOnly = true, OpenWorld = true)]
    [Description("YouTube 채널(@핸들, /channel/ID 등, 탭 videos·streams·shorts·playlists) 또는 재생목록(playlist?list=) 주소의 영상 ID·제목·길이 목록을 반환합니다. 분석할 원본 게임 플레이 영상을 찾을 때 씁니다. limit 기본 100, 최대 500.")]
    public Task<CallToolResult> YouTubeList(string url, int limit = 0, CancellationToken cancellation = default)
        => Run("youtube_list", new() { Url = url, Limit = limit }, cancellation);

    /// <summary>원격 스트림에서 지정 시각의 프레임을 뽑아 PNG·관찰표로 돌려준다.</summary>
    [McpServerTool(Name = "youtube_frames", OpenWorld = true, Destructive = false)]
    [Description("영상 전체를 내려받지 않고 원격 스트림에서 지정 시각의 프레임만 PNG로 뽑습니다(한 장당 약 3~5초). times='612.5, 10:12.5, 1:02:03' 또는 start·step·count(최대 60장). 여러 장이면 시각이 적힌 관찰표 한 장을 이미지로 반환하고 각 프레임의 경로·sha256 도 돌려줍니다. region='x,y,width,height'는 원본 해상도 기준 잘라내기, maxHeight 기본 1080. 스트림이 메타데이터보다 길면(서버 삽입 광고 의심) 거부하며 allowDurationMismatch=true 로만 진행합니다. SponsorBlock 구간 안의 시각은 nonContentSegment 로 표시됩니다.")]
    public Task<CallToolResult> YouTubeFrames([Description("영상 주소 또는 videoId")] string url, string times = "",
        double? start = null, double? step = null, int count = 0, string region = "", int maxHeight = 0, int columns = 0,
        int cellWidth = 0, bool allowDurationMismatch = false, bool includeImage = true, CancellationToken cancellation = default)
        => Run("youtube_frames", new() { Url = url, Times = times, Start = start, Step = step, Count = count, Region = region,
            MaxHeight = maxHeight, Columns = columns, CellWidth = cellWidth, AllowDurationMismatch = allowDurationMismatch,
            IncludeImage = includeImage }, cancellation);

    /// <summary>짧은 구간을 원본 프레임률 그대로 mp4 로 저장한다.</summary>
    [McpServerTool(Name = "youtube_clip", OpenWorld = true, Destructive = false)]
    [Description("start~end(초, 최대 300초) 구간을 원본 프레임률 그대로 extracted/youtube/<ID>/clips/ 에 mp4 로 저장합니다. 시작 프레임을 다시 인코딩해 clip 안 시각 t = 영상 시각 start + t 입니다. 애니메이션·건설·이동 속도처럼 프레임 단위 측정용이며, 저장 뒤 ffmpeg/tools/videoframes.py 등으로 분석합니다. 파일은 50 MB 미만이어야 하며 maxHeight 기본 720, includeAudio 로 소리를 넣습니다.")]
    public Task<CallToolResult> YouTubeClip([Description("영상 주소 또는 videoId")] string url, double start, double end,
        int maxHeight = 0, bool includeAudio = false, bool allowDurationMismatch = false, CancellationToken cancellation = default)
        => Run("youtube_clip", new() { Url = url, Start = start, End = end, MaxHeight = maxHeight, IncludeAudio = includeAudio,
            AllowDurationMismatch = allowDurationMismatch }, cancellation);

    /// <summary>영상 관찰 메모를 영상 기록에 남긴다.</summary>
    [McpServerTool(Name = "youtube_note", OpenWorld = false, Destructive = false)]
    [Description("AI가 해석한 영상 관찰을 extracted/youtube/<ID>/notes.jsonl 과 report.md 에 남깁니다. time 은 영상 시각(초), evidenceHash 는 youtube_frames 가 돌려준 프레임 sha256 입니다. note 최대 8000자. 메모는 해석이며 검증된 게임 규칙과 구분됩니다.")]
    public Task<CallToolResult> YouTubeNote([Description("영상 주소 또는 videoId")] string url, string note, double? time = null,
        string evidenceHash = "", CancellationToken cancellation = default)
        => Run("youtube_note", new() { Url = url, Note = note, Time = time, EvidenceHash = evidenceHash }, cancellation);

    /// <summary>이미 조회한 영상 기록 목록.</summary>
    [McpServerTool(Name = "youtube_videos", ReadOnly = true, OpenWorld = false)]
    [Description("youtube_probe 로 조회해 둔 영상의 ID·제목·길이·보고서 경로를 반환합니다. 재접속한 AI가 이전 분석을 이어갈 때 씁니다.")]
    public Task<CallToolResult> YouTubeVideos(CancellationToken cancellation = default)
        => Run("youtube_videos", new(), cancellation);
}
