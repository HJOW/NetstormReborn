using System.Net;
using System.Text.RegularExpressions;

namespace Netstorm.Assets;

/// <summary>튜토리얼 본문의 글자 강조 방식.</summary>
public enum TutorialTextStyle
{
    /// <summary>일반 본문.</summary>
    Body,
    /// <summary>제목.</summary>
    Heading,
    /// <summary>인용·강조 문장.</summary>
    Emphasis,
    /// <summary>조작법이나 중요한 단어.</summary>
    Highlight,
    /// <summary>
    /// 글 사이에 넣는 원본 그림 (<c>~[I타입.프레임]</c>). 구간의 Text 는 "타입.프레임" 이며
    /// <see cref="InlinePicture"/> 로 타입과 클러스터를 찾는다.
    /// </summary>
    Picture,
}

/// <summary>본문 앞의 줄 간격.</summary>
public enum TutorialTextBreak
{
    /// <summary>앞 글자에 이어 붙임.</summary>
    None,
    /// <summary>다음 줄에서 시작.</summary>
    Line,
    /// <summary>문단 간격을 두고 시작.</summary>
    Paragraph,
}

/// <summary>화면에 표시할 본문 한 구간.</summary>
public sealed record TutorialTextRun(string Text, TutorialTextStyle Style, TutorialTextBreak BreakBefore);

/// <summary>미션 스크립트의 $Button 명령으로 만든 버튼.</summary>
public sealed record TutorialDialogButton(string Label, string Action, string Argument);

/// <summary>한 안내 섹션의 표시 내용과 버튼.</summary>
public sealed record TutorialDialogContent(string Section, string Title, IReadOnlyList<TutorialTextRun> Runs,
    IReadOnlyList<TutorialDialogButton> Buttons)
{
    /// <summary>
    /// 제목이 본문 흐름 안에 그대로 들어 있는지. 제목 앞에 글 사이 그림이 있는 본문(TEST01 의
    /// <c>~[IsunBalloon.a1]&lt;h2&gt;TEST01&lt;/h2&gt;</c>)은 원본이 제목을 그림 옆에 이어 쓰므로, 화면은 창 위쪽에 제목을 따로 그리지 않는다.
    /// </summary>
    public bool TitleInBody { get; init; }
}

/// <summary>버튼을 눌렀을 때 화면 바깥에서 처리할 동작.</summary>
public enum TutorialDialogActionKind
{
    /// <summary>같은 안내 창 안에서 다른 섹션으로 이동함.</summary>
    Navigate,
    /// <summary>안내 창을 닫음.</summary>
    Close,
    /// <summary>현재 미션을 떠남.</summary>
    LeaveBattle,
    /// <summary>다른 미션을 시작함.</summary>
    MissionBegin,
    /// <summary>안내 창을 그대로 둔 채 지식 창(ShowTechnology)을 겹쳐 엶.</summary>
    ShowKnowledge,
    /// <summary>
    /// 미션을 떠날지 묻는 확인 창을 엶 (MissionAbort,0 — exe FUN_00463e40 이 tell.english 의 [ABORT] 를 Tell 한다.
    /// [ABORT] 의 버튼은 Main Menu·Replay Mission·Continue Mission 이다).
    /// </summary>
    ConfirmLeave,
    /// <summary>현재 미션을 처음부터 다시 시작함 (MissionRestart).</summary>
    RestartMission,
    /// <summary>아직 지원하지 않는 명령 또는 없는 섹션.</summary>
    Unsupported,
}

/// <summary>튜토리얼 버튼 결과. Argument는 MissionBegin의 미션 이름 또는 오류 설명이다.</summary>
public sealed record TutorialDialogAction(TutorialDialogActionKind Kind, string Argument = "");

/// <summary>
/// 준비된 미션 섹션을 안내 창 내용으로 바꾸고 $Button의 Tell 이동·닫기·미션 이동을 관리한다.
/// F8은 보정 안내나 MORE/BACK으로 옮긴 페이지가 아닌 최신 단계의 시작 섹션을 다시 연다.
/// </summary>
public sealed class TutorialDialogScript
{
    /// <summary>첫 안내를 되살릴 때 사용할 단계 섹션.</summary>
    private const string FirstStageSection = "A.";

    /// <summary>모든 표시 명령을 지우는 정규식. 본문 속 $Button 등이 사용자에게 그대로 보이지 않게 한다.</summary>
    private static readonly Regex LineCommand = new(@"\$[A-Za-z]+=[^\n]*", RegexOptions.Compiled);

    /// <summary>첫 HTML 제목을 찾는 정규식.</summary>
    private static readonly Regex FirstHeading = new(@"<h[1-4][^>]*>(.*?)</h[1-4]\s*>",
        RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);

    /// <summary>본문의 HTML 태그와 일반 글자를 나누는 정규식.</summary>
    private static readonly Regex Tag = new(@"(<[^>]*>)", RegexOptions.Compiled);

    /// <summary>
    /// 글 사이 그림 표시 <c>~[I타입.프레임]</c> 을 찾는 정규식. 구분 문자는 '.' 이고 일부 스크립트는 ',' 를 쓴다
    /// (예: <c>~[IwindWalker,9]</c>). 프레임은 숫자, 글자+숫자(D1), '*' 가 온다.
    /// </summary>
    private static readonly Regex PictureMark = new(@"(~\[I[A-Za-z0-9_]+[.,][A-Za-z0-9*]+\])", RegexOptions.Compiled);

    /// <summary>원본 줄바꿈·연속 공백을 한 칸으로 바꾸는 정규식.</summary>
    private static readonly Regex Whitespace = new(@"\s+", RegexOptions.Compiled);

    private readonly MissionScript _script;
    private readonly ConfigStore _settings;

    /// <summary>미션 스크립트에 없는 섹션(실패 창 Continue 의 TryAgain 등)을 찾을 공용 메뉴 스크립트 (tell.english, 없으면 null)</summary>
    private readonly MissionScript? _commonScript;

    /// <summary>{mission.title}·{mission.fileName} 처럼 현재 미션을 가리키는 치환 값 (없으면 null)</summary>
    private readonly IReadOnlyDictionary<string, string>? _missionValues;

    private string? _reviewSection;

    /// <summary>현재 열린 안내. null이면 닫혀 있다.</summary>
    public TutorialDialogContent? Current { get; private set; }

    /// <summary>원본 미션과 그 미션의 설정 치환표를 연결한다.</summary>
    /// <param name="script">미션 스크립트</param>
    /// <param name="settings">설정 치환표</param>
    /// <param name="commonScript">미션에 없는 섹션을 찾을 공용 메뉴 스크립트 (tell.english)</param>
    /// <param name="missionValues">현재 미션의 {mission.키} 값 (title, fileName)</param>
    public TutorialDialogScript(MissionScript script, ConfigStore settings, MissionScript? commonScript = null,
        IReadOnlyDictionary<string, string>? missionValues = null)
    {
        _script = script;
        _settings = settings;
        _commonScript = commonScript;
        _missionValues = missionValues;
    }

    /// <summary>새 튜토리얼 단계의 안내를 열고 F8의 복귀 지점으로 기록한다.</summary>
    public bool OpenStage(string section)
    {
        if (!OpenSection(section))
        {
            return false;
        }
        _reviewSection = section;
        return true;
    }

    /// <summary>
    /// 미션을 열 때의 초기 브리핑(섹션 A.)을 연다. 캠페인·튜토리얼 스크립트의 A. 가 이에 해당한다.
    /// 설정 명령(&lt;$Config,…&gt;)만 있고 보이는 본문이 없는 섹션(대회용 스크립트 등)은 빈 창이 미션을 멈춰 세우지 않도록 열지 않는다.
    /// </summary>
    /// <returns>창이 열렸으면 true</returns>
    public bool OpenBriefing()
    {
        if (!OpenStage(FirstStageSection))
        {
            return false;
        }
        if (Current!.Runs.Count == 0)
        {
            Close();
            return false;
        }
        return true;
    }

    /// <summary>보정 안내 또는 Tell 버튼의 섹션을 연다. 없는 섹션은 현재 창을 유지한다.</summary>
    public bool OpenSection(string section)
    {
        // 미션 스크립트에 섹션이 없으면 공용 스크립트에서 찾는다 (성공·실패 창의 Tell,TryAgain 은 tell.english 에만 있다)
        PreparedMissionSection? prepared = Prepare(_script, section) ?? (_commonScript == null ? null : Prepare(_commonScript, section));
        if (prepared == null || string.IsNullOrWhiteSpace(prepared.Body))
        {
            return false;
        }
        Current = PrepareContent(section, prepared);
        return true;
    }

    /// <summary>
    /// 섹션을 준비한다. {mission.키} 값이 있으면 그 섹션을 치환하는 동안만 이름 있는 설정 층("mission")으로 쌓았다가 걷어 낸다
    /// (설정 저장소를 다른 곳과 공유하므로 층이 남지 않게 한다).
    /// </summary>
    private PreparedMissionSection? Prepare(MissionScript script, string section)
    {
        if (_missionValues == null || _missionValues.Count == 0)
        {
            return script.PrepareSection(section, _settings);
        }
        _settings.Push(ConfigStore.FromPairs(_missionValues), "mission");
        try
        {
            return script.PrepareSection(section, _settings);
        }
        finally
        {
            _settings.Pop();
        }
    }

    /// <summary>F8로 최신 단계 안내를 다시 연다. 첫 단계 이전이면 A.을 연다.</summary>
    public bool Review() => OpenSection(_reviewSection ?? FirstStageSection);

    /// <summary>현재 안내를 닫는다.</summary>
    public void Close() => Current = null;

    /// <summary>현재 페이지의 버튼을 실행한다. 미션 전환은 호출자가 처리한다.</summary>
    public TutorialDialogAction Choose(int index)
    {
        TutorialDialogContent content = Current ?? throw new InvalidOperationException("열린 튜토리얼 안내가 없습니다.");
        if (index < 0 || index >= content.Buttons.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(index));
        }
        TutorialDialogButton button = content.Buttons[index];
        if (button.Action.Equals("Tell", StringComparison.OrdinalIgnoreCase))
        {
            return OpenSection(button.Argument)
                ? new(TutorialDialogActionKind.Navigate)
                : new(TutorialDialogActionKind.Unsupported, $"안내 섹션이 없습니다: {button.Argument}");
        }
        if (button.Action.Equals("DoNothing", StringComparison.OrdinalIgnoreCase))
        {
            Close();
            return new(TutorialDialogActionKind.Close);
        }
        if (button.Action.Equals("LeaveBattle", StringComparison.OrdinalIgnoreCase))
        {
            Close();
            return new(TutorialDialogActionKind.LeaveBattle);
        }
        if (button.Action.Equals("MissionAbort", StringComparison.OrdinalIgnoreCase))
        {
            // exe FUN_00463e40: 인자가 0 이면 [ABORT] 확인 창, 아니면 곧바로 LeaveBattle 사건을 보낸다 (성공 창의 Leave Missions,MissionAbort,1)
            Close();
            return IsZero(button.Argument) ? new(TutorialDialogActionKind.ConfirmLeave) : new(TutorialDialogActionKind.LeaveBattle);
        }
        if (button.Action.Equals("MissionRestart", StringComparison.OrdinalIgnoreCase))
        {
            Close();
            return new(TutorialDialogActionKind.RestartMission);
        }
        if (button.Action.Equals("MissionBegin", StringComparison.OrdinalIgnoreCase) && button.Argument.Length > 0)
        {
            Close();
            return new(TutorialDialogActionKind.MissionBegin, button.Argument);
        }
        if (button.Action.Equals("ShowTechnology", StringComparison.OrdinalIgnoreCase))
        {
            // 원본 디스패처는 인자(55)를 넘기지 않고 F6 지식 창 함수를 부르므로 인자는 무시한다.
            return new(TutorialDialogActionKind.ShowKnowledge);
        }
        return new(TutorialDialogActionKind.Unsupported, $"아직 지원하지 않는 안내 버튼: {button.Action}");
    }

    /// <summary>버튼 인자가 0 인지 (비었거나 숫자가 아니면 0 으로 본다 — 원본 인자 해석은 atol)</summary>
    private static bool IsZero(string argument) =>
        !int.TryParse(argument.Trim(), System.Globalization.NumberStyles.Integer, System.Globalization.CultureInfo.InvariantCulture, out int value)
        || value == 0;

    /// <summary>조건·변수 치환을 마친 본문에서 제목·버튼·표시용 HTML 구간을 만든다.</summary>
    private static TutorialDialogContent PrepareContent(string section, PreparedMissionSection prepared)
    {
        Match heading = FirstHeading.Match(prepared.Body);
        // 제목 앞에 그림이 있으면(TEST01 의 "~[IsunBalloon.a1]<h2>TEST01</h2>") 원본은 제목을 그림 옆에 이어 쓴다.
        // 그런 본문은 제목을 따로 떼지 않고 본문 흐름에 그대로 둔다.
        bool inlineHeading = heading.Success && PictureMark.IsMatch(prepared.Body[..heading.Index]);
        string title = heading.Success
            ? WebUtility.HtmlDecode(Tag.Replace(heading.Groups[1].Value, "")).Trim()
            : $"튜토리얼 {section}";
        string body = heading.Success && !inlineHeading ? prepared.Body.Remove(heading.Index, heading.Length) : prepared.Body;
        body = LineCommand.Replace(body, "");
        var buttons = new List<TutorialDialogButton>();
        // 조건 평가를 통과한 Button 명령만 화면 순서대로 만든다.
        foreach (MissionCommand command in prepared.Commands)
        {
            if (!command.Name.Equals("Button", StringComparison.OrdinalIgnoreCase) || command.Fields.Count < 2)
            {
                continue;
            }
            buttons.Add(new TutorialDialogButton(command.Fields[0].Trim(), command.Fields[1].Trim(),
                command.Fields.Count > 2 ? string.Join(',', command.Fields.Skip(2)).Trim() : ""));
        }
        if (buttons.Count == 0)
        {
            // 원본 스크립트의 [F1.]처럼 버튼 없는 안내도 닫을 수 있게 한다.
            buttons.Add(new TutorialDialogButton("닫기", "DoNothing", "0"));
        }
        return new TutorialDialogContent(section, title, ParseText(body), buttons) { TitleInBody = inlineHeading };
    }

    /// <summary>
    /// HTML 부분집합의 문단·줄바꿈·강조를 표시 구간으로 변환한다. 도움말 절(<see cref="HelpTopics"/>)도 같은 규칙을 쓴다.
    /// 링크(&lt;a href&gt;)는 원본 도움말처럼 글자색만 바꾸고 이동은 하지 않는다.
    /// </summary>
    /// <param name="body">조건 치환을 마친 HTML 본문</param>
    public static IReadOnlyList<TutorialTextRun> ParseHtml(string body) => ParseText(body);

    /// <summary>HTML 부분집합의 문단·줄바꿈·강조를 표시 구간으로 변환한다.</summary>
    private static IReadOnlyList<TutorialTextRun> ParseText(string body)
    {
        var runs = new List<TutorialTextRun>();
        TutorialTextBreak pendingBreak = TutorialTextBreak.None;
        bool heading = false;
        bool emphasis = false;
        bool highlight = false;
        // 태그와 일반 텍스트를 번갈아 읽어 원본 문장의 순서와 인라인 강조를 보존한다.
        foreach (string part in Tag.Split(body))
        {
            if (part.Length == 0)
            {
                continue;
            }
            if (part[0] != '<')
            {
                // 글 사이 그림 표시는 그림 구간으로, 나머지는 글자로 붙인다.
                foreach (string piece in PictureMark.Split(part))
                {
                    if (piece.StartsWith("~[I", StringComparison.Ordinal)) AppendPicture(piece[3..^1].Replace(',', '.'));
                    else AppendText(piece);
                }
                continue;
            }
            string name = part[1..^1].Trim().TrimEnd('/').ToLowerInvariant();
            if (name.StartsWith("$", StringComparison.Ordinal))
            {
                continue;
            }
            if (name.StartsWith('!'))
            {
                string picture = part[2..^1].Trim('"', '\'', ' ');
                picture = picture.Split('.')[0];
                AppendText($"[그림: {(picture.Length == 0 ? "원본" : picture)}]");
                continue;
            }
            switch (name)
            {
                case "p":
                    pendingBreak = TutorialTextBreak.Paragraph;
                    break;
                case "br":
                    if (pendingBreak < TutorialTextBreak.Line) pendingBreak = TutorialTextBreak.Line;
                    break;
                case "h1" or "h2" or "h3" or "h4":
                    // 그림 바로 뒤의 제목은 원본처럼 그림 옆(같은 줄)에 이어 쓴다.
                    if (runs.Count == 0 || runs[^1].Style != TutorialTextStyle.Picture || pendingBreak != TutorialTextBreak.None)
                        pendingBreak = TutorialTextBreak.Paragraph;
                    heading = true;
                    break;
                case "/h1" or "/h2" or "/h3" or "/h4":
                    pendingBreak = TutorialTextBreak.Paragraph;
                    heading = false;
                    break;
                case "i" or "q":
                    emphasis = true;
                    break;
                case "/i" or "/q":
                    emphasis = false;
                    break;
                case "c" or "b":
                    highlight = true;
                    break;
                case "/c" or "/b" or "/a":
                    highlight = false;
                    break;
                default:
                    // 링크 시작(<a href="…">)은 강조 색으로 표시한다. 앵커(<a name>)는 표시하지 않는다.
                    if (name.StartsWith("a ", StringComparison.Ordinal) && name.Contains("href", StringComparison.Ordinal))
                    {
                        highlight = true;
                    }
                    break;
            }
        }
        return runs;

        /// <summary>그림 구간 하나를 현재 줄 간격으로 붙인다 (Text = "타입.프레임").</summary>
        void AppendPicture(string spec)
        {
            runs.Add(new TutorialTextRun(spec, TutorialTextStyle.Picture, pendingBreak));
            pendingBreak = TutorialTextBreak.None;
        }

        /// <summary>텍스트 조각 하나를 현재 스타일과 줄 간격으로 붙인다.</summary>
        void AppendText(string source)
        {
            string text = WebUtility.HtmlDecode(Whitespace.Replace(source, " "));
            if (text.Trim().Length == 0)
            {
                return;
            }
            TutorialTextStyle style = heading ? TutorialTextStyle.Heading
                : highlight ? TutorialTextStyle.Highlight
                : emphasis ? TutorialTextStyle.Emphasis : TutorialTextStyle.Body;
            runs.Add(new TutorialTextRun(text, style, pendingBreak));
            pendingBreak = TutorialTextBreak.None;
        }
    }
}
