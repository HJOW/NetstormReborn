namespace Netstorm.AnalyzeManager;

// CLI·MCP·리눅스용 YouTube 전용 빌드(portable/)가 함께 쓰는 요청·결과 형식이다.

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
    /// <summary>분석·녹화 FPS. 생략하면 세션 값(새 세션 기본 30)을 사용한다.</summary>
    public int? Fps { get; set; }
    /// <summary>변화 감지 간격(ms). 0이면 FPS를 사용하고 양수이면 이전 방식의 간격을 명시한다.</summary>
    public int PollMs { get; set; }
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
