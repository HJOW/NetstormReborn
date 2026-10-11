using System.Net;
using System.Text.RegularExpressions;

namespace Netstorm.Assets;

/// <summary>도움말의 본문 조각. 링크·삽화와 색을 유지하며 중첩 굵은 태그는 Bold로 별도 보존한다.</summary>
public sealed record HelpTextRun(string Text, TutorialTextStyle Style, TutorialTextBreak BreakBefore,
    string? Link = null, string? Picture = null, bool Bold = false);

/// <summary>원본 도움말 HTML 부분집합과 색·강조 제어 문자를 화면용 조각으로 해석한다.</summary>
public static partial class HelpDocument
{
    /// <summary>태그와 일반 글자를 구분한다.</summary>
    [GeneratedRegex("(<[^>]*>)")]
    private static partial Regex Tags();
    /// <summary>링크·그림의 따옴표 안 값을 읽는다.</summary>
    [GeneratedRegex("\"([^\"]*)\"")]
    private static partial Regex Quoted();
    /// <summary>원본 줄바꿈·공백을 본문의 한 칸으로 합친다.</summary>
    [GeneratedRegex(@"\s+")]
    private static partial Regex Spaces();
    /// <summary>원본 텍스트 제어 문자. ~~는 별도 보호하고 나머지 코드는 표시에서 제외한다.</summary>
    [GeneratedRegex(@"~(?:\[[^\]]*\]|[A-Za-z.]|[0-9]+)")]
    private static partial Regex Controls();
    /// <summary>미션 안에서만 보이는 목표 다시 보기 안내를 찾는다.</summary>
    [GeneratedRegex(@"<\?\{global\.inMission\}>(.*?)</\?>", RegexOptions.IgnoreCase | RegexOptions.Singleline)]
    private static partial Regex MissionOnly();

    /// <summary>원본의 링크·제목·단락·인용·그림을 보존하면서 표시 조각을 만든다.</summary>
    public static IReadOnlyList<HelpTextRun> Parse(string html, bool inMission = false)
    {
        html = MissionOnly().Replace(html, match => inMission ? match.Groups[1].Value : "");
        var runs = new List<HelpTextRun>();
        TutorialTextStyle style = TutorialTextStyle.Body;
        TutorialTextBreak gap = TutorialTextBreak.None;
        string? link = null;
        // 중첩된 원본 <b><c>와 잘못 반복된 닫는 태그도 색과 굵기를 각각 보존한다.
        int boldDepth = 0, colorDepth = 0, emphasisDepth = 0, headingDepth = 0;
        // ~l ... ~. 로 감싼 구간은 링크와 같은 색 글씨다 (도움말 첫 문장의 "blue").
        bool linkColor = false;
        // 원본은 일부 닫는 태그가 잘못되어 있어 알 수 없는 태그는 글자로 노출하지 않는다.
        foreach (string piece in Tags().Split(html))
        {
            if (piece.StartsWith('<'))
            {
                string tag = piece.Trim().ToLowerInvariant();
                if (tag.StartsWith("<a ") && tag.Contains("href")) link = Quoted().Match(piece).Groups[1].Value;
                else if (tag == "</a>") link = null;
                else if (tag.StartsWith("<!\""))
                {
                    runs.Add(new("", style, gap, link, Quoted().Match(piece).Groups[1].Value, boldDepth > 0));
                    gap = TutorialTextBreak.None;
                }
                else if (tag is "<p>" or "</p>") gap = TutorialTextBreak.Paragraph;
                else if (tag is "<br>" or "<br/>") gap = TutorialTextBreak.Line;
                else if (tag.StartsWith("<h") && tag.Length > 2 && char.IsDigit(tag[2])) { headingDepth++; gap = TutorialTextBreak.Paragraph; }
                else if (tag.StartsWith("</h") && tag.Length > 3 && char.IsDigit(tag[3])) { headingDepth = Math.Max(0, headingDepth - 1); gap = TutorialTextBreak.Paragraph; }
                else if (tag == "<b>") boldDepth++;
                else if (tag == "</b>") boldDepth = Math.Max(0, boldDepth - 1);
                else if (tag == "<c>") colorDepth++;
                else if (tag == "</c>") colorDepth = Math.Max(0, colorDepth - 1);
                else if (tag is "<i>" or "<q>") emphasisDepth++;
                else if (tag is "</i>" or "</q>") emphasisDepth = Math.Max(0, emphasisDepth - 1);
                style = CurrentStyle();
                continue;
            }
            string raw = WebUtility.HtmlDecode(piece.Replace("~~", "\u0001"));
            int cursor = 0;
            // ~l 은 링크 색 글씨를 켜고 ~. 은 끈다. 그 밖의 제어 문자는 표시에서 지운다. 구간마다 따로 조각을 만든다.
            foreach (Match control in Controls().Matches(raw))
            {
                AddText(raw[cursor..control.Index]);
                cursor = control.Index + control.Length;
                if (control.Value == "~l") linkColor = true;
                else if (control.Value == "~.") linkColor = false;
            }
            AddText(raw[cursor..]);

            // 줄/문단 앞의 공백은 버리되 인라인 태그 사이 한 칸은 보존하여 서로 다른 단어가 붙지 않게 한다.
            void AddText(string segment)
            {
                string text = Spaces().Replace(segment.Replace('\u0001', '~'), " ");
                if (text.Length == 0 || (string.IsNullOrWhiteSpace(text) && (runs.Count == 0 || gap != TutorialTextBreak.None))) return;
                runs.Add(new(text, linkColor && style == TutorialTextStyle.Body ? TutorialTextStyle.LinkColor : style, gap, link, Bold: boldDepth > 0));
                gap = TutorialTextBreak.None;
            }
        }
        return runs;

        // 원본 태그의 색·기울임·제목을 현재 중첩 상태에서 고른다. 굵기는 별도 플래그로 함께 전달한다.
        TutorialTextStyle CurrentStyle() => headingDepth > 0 ? TutorialTextStyle.Heading
            : colorDepth > 0 ? TutorialTextStyle.ColorText
            : emphasisDepth > 0 ? TutorialTextStyle.Emphasis
            : boldDepth > 0 ? TutorialTextStyle.Highlight : TutorialTextStyle.Body;
    }
}

/// <summary>도움말의 내부 링크와 방문 기록. 외부 주소·게임 명령은 내부 앵커로 실행하지 않는다.</summary>
public sealed class HelpNavigation(HelpTopics topics)
{
    private readonly Stack<(string Anchor, int Scroll)> _history = new();
    /// <summary>현재 앵커. null이면 창이 닫혀 있다.</summary>
    public string? Anchor { get; private set; }
    /// <summary>현재 본문의 스크롤 픽셀.</summary>
    public int Scroll { get; set; }
    /// <summary>이전 주제로 되돌아갈 수 있는지.</summary>
    public bool CanGoBack => _history.Count > 0;
    /// <summary>창을 새로 열 때 방문 기록을 초기화한다.</summary>
    public bool Open(string anchor)
    {
        if (topics.Find(anchor) == null) return false;
        _history.Clear(); Anchor = anchor; Scroll = 0; return true;
    }
    /// <summary>존재하는 내부 앵커만 열고 이전 페이지의 스크롤 위치를 기억한다.</summary>
    public bool Follow(string target)
    {
        string anchor = target.TrimStart('#');
        if (target.Contains(':') || topics.Find(anchor) == null || Anchor == null) return false;
        _history.Push((Anchor, Scroll)); Anchor = anchor; Scroll = 0; return true;
    }
    /// <summary>Back 버튼으로 이전 주제와 그 스크롤 위치를 복원한다.</summary>
    public bool Back()
    {
        if (!_history.TryPop(out var previous)) return false;
        (Anchor, Scroll) = previous; return true;
    }
    /// <summary>OK 버튼으로 창과 방문 기록을 닫는다.</summary>
    public void Close() { Anchor = null; Scroll = 0; _history.Clear(); }
}
