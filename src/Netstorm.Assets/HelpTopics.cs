using System.Text.RegularExpressions;

namespace Netstorm.Assets;

/// <summary>
/// 도움말 원문(`d/help.english` 등)의 앵커 절. 원본 지식 상세창의 본문은 `&lt;a name="&lt;타입&gt;Type"&gt;` 절과 같다
/// (2026-09-30 사용자 플레이 녹화의 Rain Generator 상세창과 `rainBatteryType` 절 대조, docs/exe/show-technology.md).
/// 절은 앵커 줄부터 줄 전체가 `&lt;/a&gt;` 인 줄 앞까지다. 앵커 여러 개가 연달아 붙은 절(예: runeType·altarType)은 같은 본문을 공유한다.
/// </summary>
public sealed partial class HelpTopics
{
    /// <summary>앵커 이름 → 본문 (대소문자 무시)</summary>
    private readonly Dictionary<string, string> _topics = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>앵커 시작 태그 (&lt;a name="…"&gt;)</summary>
    [GeneratedRegex("""<a\s+name\s*=\s*"([^"]+)"\s*>""", RegexOptions.IgnoreCase)]
    private static partial Regex AnchorTag();

    /// <summary>원본 도움말 본문 머리의 정보 칸 표시 태그 (그림·수치 칸은 게임이 따로 그린다)</summary>
    [GeneratedRegex(@"<info>", RegexOptions.IgnoreCase)]
    private static partial Regex InfoTag();

    /// <summary>도움말 파일 내용을 앵커 절로 나눈다</summary>
    /// <param name="text">도움말 원문 (UTF-8 로 바꾼 것)</param>
    public HelpTopics(string text)
    {
        string[] lines = text.Replace("\r\n", "\n").Split('\n');
        var pending = new List<string>();
        var body = new List<string>();
        bool inBody = false;
        // 줄을 차례로 읽어, 앵커만 있는 줄은 이름을 모으고 본문이 시작되면 닫는 줄까지 모은다
        foreach (string line in lines)
        {
            string trimmed = line.Trim();
            Match anchor = AnchorTag().Match(trimmed);
            if (!inBody && anchor.Success && anchor.Index == 0)
            {
                pending.Add(anchor.Groups[1].Value);
                string rest = trimmed[anchor.Length..].Trim();
                if (rest.Length > 0)
                {
                    inBody = true;
                    body.Add(rest);
                }
                continue;
            }
            if (pending.Count == 0)
            {
                continue;
            }
            if (trimmed.Equals("</a>", StringComparison.OrdinalIgnoreCase))
            {
                Store();
                continue;
            }
            inBody = true;
            body.Add(line);
        }
        Store();

        /// <summary>모은 앵커 이름마다 본문을 기록하고 상태를 비운다.</summary>
        void Store()
        {
            string joined = InfoTag().Replace(string.Join('\n', body), "").Trim();
            // 같은 본문을 공유하는 앵커 이름을 모두 등록한다 (먼저 나온 절을 우선한다)
            foreach (string name in pending)
            {
                _topics.TryAdd(name, joined);
            }
            pending.Clear();
            body.Clear();
            inBody = false;
        }
    }

    /// <summary>앵커 수</summary>
    public int Count => _topics.Count;

    /// <summary>앵커 절의 원문 HTML (없으면 null)</summary>
    /// <param name="anchor">앵커 이름 (예: "rainBatteryType")</param>
    public string? Find(string anchor) => _topics.GetValueOrDefault(anchor);

    /// <summary>유닛 타입의 설명 절 (앵커 "&lt;타입 이름&gt;Type", 없으면 null)</summary>
    /// <param name="typeName">.type 이름 (예: "rainBattery")</param>
    public string? ForType(string typeName) => Find(typeName + "Type");

    /// <summary>절을 표시용 구간으로 바꾼다 (미션 안내 창과 같은 HTML 부분집합 규칙)</summary>
    /// <param name="html">절 원문</param>
    public static IReadOnlyList<TutorialTextRun> ToRuns(string html) => TutorialDialogScript.ParseHtml(html);
}
