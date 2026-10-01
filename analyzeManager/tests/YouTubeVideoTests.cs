namespace Netstorm.AnalyzeManager.Tests;

/// <summary>YouTube 주소·시각 해석과 광고 관련 판정을 네트워크 없이 검사한다.</summary>
public sealed class YouTubeVideoTests
{
    /// <summary>여러 형식의 영상 주소에서 ID 와 시작 시각을 얻는다</summary>
    [Theory]
    [InlineData("https://www.youtube.com/watch?v=0p7VvzSxTAY", "0p7VvzSxTAY", null)]
    [InlineData("https://www.youtube.com/watch?v=0p7VvzSxTAY&t=480s", "0p7VvzSxTAY", 480.0)]
    [InlineData("https://youtu.be/CI3dCrUt4tY?t=1h2m3s", "CI3dCrUt4tY", 3723.0)]
    [InlineData("https://m.youtube.com/watch?feature=share&v=AMEzorbjQYQ", "AMEzorbjQYQ", null)]
    [InlineData("https://www.youtube.com/shorts/adR1Kap60hw", "adR1Kap60hw", null)]
    [InlineData("https://www.youtube-nocookie.com/embed/PUeIPg1BEzo?start=75", "PUeIPg1BEzo", 75.0)]
    [InlineData("zpZsx4dRac8", "zpZsx4dRac8", null)]
    public void TryParseVideo_AcceptsYouTubeForms(string input, string id, double? start)
    {
        Assert.True(YouTubeVideo.TryParseVideo(input, out string parsed, out double? seconds));
        Assert.Equal(id, parsed);
        Assert.Equal(start, seconds);
    }

    /// <summary>다른 사이트·잘못된 ID·채널 주소는 영상으로 받지 않는다 (임의 URL 을 내려받지 않도록)</summary>
    [Theory]
    [InlineData("https://example.com/watch?v=0p7VvzSxTAY")]
    [InlineData("https://www.youtube.com.evil.example/watch?v=0p7VvzSxTAY")]
    [InlineData("https://www.youtube.com/watch?v=short")]
    [InlineData("https://www.youtube.com/@netstormcampaigns2591")]
    [InlineData("file:///C:/video.mp4")]
    [InlineData("../0p7VvzSxTAY")]
    public void TryParseVideo_RejectsOthers(string input)
    {
        Assert.False(YouTubeVideo.TryParseVideo(input, out _, out _));
    }

    /// <summary>채널 핸들은 동영상 탭으로, 재생목록은 표준 주소로 바꾼다</summary>
    [Theory]
    [InlineData("https://www.youtube.com/@netstormcampaigns2591", "https://www.youtube.com/@netstormcampaigns2591/videos")]
    [InlineData("https://www.youtube.com/@netstormcampaigns2591/streams", "https://www.youtube.com/@netstormcampaigns2591/streams")]
    [InlineData("https://www.youtube.com/channel/UC123abc", "https://www.youtube.com/channel/UC123abc/videos")]
    [InlineData("https://www.youtube.com/playlist?list=PL0123456789", "https://www.youtube.com/playlist?list=PL0123456789")]
    public void TryParseList_Normalizes(string input, string expected)
    {
        Assert.True(YouTubeVideo.TryParseList(input, out string normalized));
        Assert.Equal(expected, normalized);
    }

    /// <summary>영상 주소·다른 사이트는 목록으로 받지 않는다</summary>
    [Theory]
    [InlineData("https://www.youtube.com/watch?v=0p7VvzSxTAY")]
    [InlineData("https://example.com/@someone")]
    [InlineData("https://www.youtube.com/@someone/community")]
    public void TryParseList_RejectsOthers(string input)
    {
        Assert.False(YouTubeVideo.TryParseList(input, out _));
    }

    /// <summary>시각 표기 여러 가지</summary>
    [Fact]
    public void ParseTimes_ReadsFormats()
    {
        Assert.Equal([612.5, 612.5, 3723, 3723, 75, 120], YouTubeVideo.ParseTimes("612.5, 10:12.5, 1:02:03, 1h2m3s, 75s, 2m"));
        Assert.Throws<ArgumentException>(() => YouTubeVideo.ParseTimes("10:75"));
        Assert.Throws<ArgumentException>(() => YouTubeVideo.ParseTimes("-3"));
        Assert.Throws<ArgumentException>(() => YouTubeVideo.ParseTimes("abc"));
        Assert.Equal("1:20:00.0", YouTubeVideo.FormatTime(4800));
        Assert.Equal("08:02.5", YouTubeVideo.FormatTime(482.5));
    }

    /// <summary>SponsorBlock 의 협찬·인트로 구간만 게임 화면 아님으로 보고, 업로더 챕터는 보지 않는다</summary>
    [Fact]
    public void NonContentAt_UsesAdLikeCategories()
    {
        VideoSegment[] segments =
        [
            new(0, 810, "chapter", "Breaking Through"),
            new(30, 45, "sponsor", "Sponsor"),
            new(100, 110, "intro", "Intro"),
        ];
        Assert.Null(YouTubeVideo.NonContentAt(segments, 10));
        Assert.Equal("sponsor", YouTubeVideo.NonContentAt(segments, 30)!.Category);
        Assert.Null(YouTubeVideo.NonContentAt(segments, 45));
        Assert.Equal("intro", YouTubeVideo.NonContentAt(segments, 105)!.Category);
    }

    /// <summary>스트림이 메타데이터보다 눈에 띄게 길 때만 서버 삽입 광고를 의심한다 (실측: 6093.5초 스트림 / 6094초 메타데이터)</summary>
    [Fact]
    public void PossiblyStitchedAds_ComparesDurations()
    {
        Assert.False(YouTubeVideo.PossiblyStitchedAds(6094, 6093.5));
        Assert.False(YouTubeVideo.PossiblyStitchedAds(2276, 2277.5));
        Assert.True(YouTubeVideo.PossiblyStitchedAds(2276, 2276 + 15));
        Assert.True(YouTubeVideo.PossiblyStitchedAds(6094, 6094 + 40));
        Assert.False(YouTubeVideo.PossiblyStitchedAds(0, 100));
    }

    /// <summary>스트림 주소의 expire 쿼리에서 만료 시각을 읽는다</summary>
    [Fact]
    public void StreamExpiry_ReadsQuery()
    {
        Assert.Equal(DateTimeOffset.FromUnixTimeSeconds(1790837865),
            YouTubeVideo.StreamExpiry("https://rr1---sn.googlevideo.com/videoplayback?expire=1790837865&ei=x"));
        Assert.Null(YouTubeVideo.StreamExpiry("https://rr1---sn.googlevideo.com/videoplayback?ei=x"));
    }

    /// <summary>영상 기록 폴더는 영상 ID 형식만 받고 extracted/youtube 아래에 둔다</summary>
    [Fact]
    public void VideoDirectory_RejectsTraversal()
    {
        string root = Path.Combine(Path.GetTempPath(), "NetstormYouTubeTests", Guid.NewGuid().ToString("N"));
        var analyzer = new YouTubeAnalyzer(root);
        Assert.Equal(Path.Combine(root, "extracted", "youtube", "0p7VvzSxTAY"), analyzer.VideoDirectory("0p7VvzSxTAY"));
        Assert.Throws<ArgumentException>(() => analyzer.VideoDirectory("../../../aa"));
        Assert.Throws<ArgumentException>(() => analyzer.VideoDirectory("0p7VvzSxTA"));
    }
}
