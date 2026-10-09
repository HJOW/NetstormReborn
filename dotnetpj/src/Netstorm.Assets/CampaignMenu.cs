using System.Text.RegularExpressions;

namespace Netstorm.Assets;

/// <summary>캠페인 선택창 목록의 한 줄.</summary>
/// <param name="Label">표시 문구 (장 제목 또는 미션 이름)</param>
/// <param name="Action">고르면 실행할 명령 (Tell·MissionBegin 등, 원본 표기 그대로)</param>
/// <param name="Argument">명령 인자 (Tell 이면 "파일.섹션" 또는 섹션, MissionBegin 이면 미션 파일 이름)</param>
/// <param name="Checked">완료 표시(파란 원)를 붙일지. 표시 자체가 없는 줄($Menu)은 null</param>
/// <param name="Enabled">고를 수 있는지 (원본 조건식의 결과)</param>
/// <param name="Separator">글자 없이 구분선만 긋는 줄인지 (제목이 "--" 로 시작하는 장)</param>
public sealed record CampaignItem(string Label, string Action, string Argument, bool? Checked, bool Enabled, bool Separator = false);

/// <summary>캠페인 선택창 한 장: 제목·본문·목록·버튼.</summary>
/// <param name="Target">이 창을 연 Tell 대상 ("UCampaign" 또는 "offical3.overview")</param>
/// <param name="Content">제목·본문 구간·하단 버튼</param>
/// <param name="Items">목록 줄 (장 목록 또는 미션 목록, 스크립트에 적힌 순서)</param>
/// <param name="TrailingBlankLines">본문 끝에 남는 빈 줄 수 (본문이 &lt;br&gt; 로 끝나면 그 수만큼 목록이 내려간다)</param>
public sealed record CampaignPage(string Target, TutorialDialogContent Content, IReadOnlyList<CampaignItem> Items, int TrailingBlankLines);

/// <summary>
/// 원본 캠페인 선택창의 내용을 스크립트에서 읽는다. 클론이 장·미션 목록이나 완료 표시를 따로 정하지 않고 원본 자료를 그대로 쓴다.
/// <list type="bullet">
/// <item>첫 창은 tell 스크립트의 [UCampaign] 이고, <c>$Menu=@{guideSpec},Tell,{cur.file}.overview</c> 가 guideSpec(<c>offical*.언어</c>)에
/// 맞는 파일을 이름순으로 늘어놓는다. 줄 문구는 그 파일의 머리 값 title, title 이 "--" 로 시작하면 구분선이다.</item>
/// <item>장을 고르면 그 파일의 [Overview] 를 연다. <c>$Checked=문구,명령,인자,완료식,가능식</c> 한 줄이 미션 하나이고,
/// 완료식·가능식은 설정 치환 뒤의 정수 값이 0 이 아니면 참이다 (예: <c>{DoneTheWarBegins}</c> — 값이 없으면 치환되지 않아 0).</item>
/// </list>
/// 사용법: <c>CampaignPage? root = CampaignMenu.Open(resources, CampaignMenu.Root);</c> 뒤 줄의 Tell 인자로 다시 <see cref="Open"/> 을 부른다.
/// </summary>
/// <remarks>
/// 원본 Htmlgump 004c83b0·004ce580·004cf270, 조건 0046c090·0046c810 (docs/exe/cpp-menu-reconstruction.md,
/// 기준 구현 cpppj/src/client/DialogScript.cpp·UberGump.cpp 의 ExpandMenus). 2026-10-10 추가 (LEFT_JOBS.dotnetpj.md 5-6):
/// 화면(MainMenuView 의 캠페인·미션 목록)은 아직 장 여섯 개와 한 장의 미션 여섯 개를 문구째 고정해 쓰고 있어, 이 클래스로 바꾸기 위한 자료 읽기를 먼저 만들었다.
/// 추가한 날에는 컴파일만 확인했고 단위 시험과 화면 연결은 하지 않았다.
/// </remarks>
public static partial class CampaignMenu
{
    /// <summary>캠페인 선택의 첫 창 (tell 스크립트의 섹션 이름).</summary>
    public const string Root = "UCampaign";

    /// <summary>Tell 대상에 파일 이름이 없을 때 섹션을 찾는 공용 스크립트.</summary>
    private const string CommonScript = "tell";

    /// <summary>파일 목록 줄에서 지금 파일 이름으로 바뀌는 자리 표시.</summary>
    private const string CurrentFile = "{cur.file}";

    /// <summary>
    /// Tell 대상의 창을 연다. 대상이 "파일.섹션" 이면 그 스크립트의 섹션, 아니면 tell 스크립트의 섹션이다.
    /// 스크립트나 섹션이 없으면 null.
    /// </summary>
    /// <param name="resources">스크립트·설정·파일을 읽을 게임 자료</param>
    /// <param name="target">Tell 대상 (예: "UCampaign", "offical3.overview")</param>
    public static CampaignPage? Open(GameResources resources, string target)
    {
        string scriptName = CommonScript, section = target;
        int dot = target.IndexOf('.');
        if (dot > 0)
        {
            scriptName = target[..dot];
            section = target[(dot + 1)..];
        }
        MissionScript? script = resources.TryLoadMission(scriptName)?.Script;
        PreparedMissionSection? prepared = script?.PrepareSection(section, resources.Settings);
        if (prepared == null)
        {
            return null;
        }
        var items = new List<CampaignItem>();
        // 목록을 만드는 명령($Menu·$Checked)을 스크립트에 적힌 순서대로 줄로 바꾼다.
        foreach (MissionCommand command in prepared.Commands)
        {
            string Field(int index) => index < command.Fields.Count ? command.Fields[index].Trim() : "";
            if (command.Name.Equals("Menu", StringComparison.OrdinalIgnoreCase))
            {
                string label = Field(0);
                if (label.StartsWith('@') || label.StartsWith('%'))
                {
                    AddFileItems(resources, items, label[1..], Field(1), Field(2));
                }
                else
                {
                    items.Add(new CampaignItem(label, Field(1), Field(2), null, true));
                }
            }
            else if (command.Name.Equals("Checked", StringComparison.OrdinalIgnoreCase))
            {
                items.Add(new CampaignItem(Field(0), Field(1), Field(2),
                    MissionConditions.Evaluate(Field(3), resources.Settings), MissionConditions.Evaluate(Field(4), resources.Settings)));
            }
        }
        TutorialDialogContent content = TutorialDialogScript.PrepareContent(section, prepared);
        return new CampaignPage(target, content, items, TrailingBreaks().Match(StripCommands().Replace(prepared.Body, "")) is { Success: true } tail
            ? LineBreak().Matches(tail.Value).Count : 0);
    }

    /// <summary>
    /// 파일 지정식에 맞는 스크립트를 이름순으로 찾아 줄로 더한다. 선택한 언어의 파일이 하나도 없으면 영어 파일을 쓴다.
    /// </summary>
    /// <param name="resources">게임 자료</param>
    /// <param name="pattern">치환을 마친 파일 지정식 (예: "\D\offical*.english")</param>
    /// <param name="action">줄의 명령</param>
    /// <param name="argument">줄의 인자 ({cur.file} 은 파일 이름으로 바뀐다)</param>
    /// <param name="items">줄을 더할 목록</param>
    private static void AddFileItems(GameResources resources, List<CampaignItem> items, string pattern, string action, string argument)
    {
        IReadOnlyList<string> paths = resources.Files.Find(pattern);
        if (paths.Count == 0 && resources.Language != GameLanguage.English
            && pattern.EndsWith("." + resources.Language, StringComparison.OrdinalIgnoreCase))
        {
            paths = resources.Files.Find(pattern[..^resources.Language.Length] + GameLanguage.English);
        }
        // 원본처럼 파일 이름순으로 늘어놓는다 (offical1, offical2, offical2sep, offical3 …).
        foreach (string name in paths.Select(path => Path.GetFileNameWithoutExtension(path.Replace('\\', '/'))).OfType<string>()
                     .Distinct(StringComparer.OrdinalIgnoreCase).OrderBy(name => name, StringComparer.OrdinalIgnoreCase))
        {
            string title = resources.TryLoadMission(name)?.Script.GetHeader("title")?.Trim().Trim('"').Trim() ?? name;
            bool separator = title.StartsWith("--", StringComparison.Ordinal);
            items.Add(new CampaignItem(title, action, argument.Replace(CurrentFile, name, StringComparison.OrdinalIgnoreCase), null, !separator, separator));
        }
    }

    /// <summary>줄 명령($이름=…)을 지우는 정규식. 본문 끝의 줄바꿈 태그를 셀 때 명령 줄이 끼지 않게 한다.</summary>
    [GeneratedRegex(@"\$[A-Za-z]+=[^\n]*")]
    private static partial Regex StripCommands();

    /// <summary>본문 끝에 이어진 줄바꿈 태그(&lt;br&gt;)와 공백을 찾는 정규식.</summary>
    [GeneratedRegex(@"(?:\s*<br\s*/?>)+\s*$", RegexOptions.IgnoreCase)]
    private static partial Regex TrailingBreaks();

    /// <summary>줄바꿈 태그 하나를 찾는 정규식.</summary>
    [GeneratedRegex(@"<br\s*/?>", RegexOptions.IgnoreCase)]
    private static partial Regex LineBreak();
}
