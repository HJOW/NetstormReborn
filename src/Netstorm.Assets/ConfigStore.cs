using System.Text;

namespace Netstorm.Assets;

/// <summary>
/// 여러 설정 텍스트를 층으로 쌓아 조회하고, 문자열 속 {키} 를 원본과 같은 규칙으로 치환한다.
/// 원본 Config.cpp 분석 결과 (docs/formats/config.md "치환 규칙"):
/// <list type="bullet">
/// <item>층은 마지막에 쌓은 것부터 조회한다. 이름이 있는 층(예: "local")은 "local.1" 처럼 접두어가 붙은 키만 받는다.</item>
/// <item>{키} → 값(값 안의 {..} 도 다시 치환), {키|기본값} → 없으면 기본값, 둘 다 없으면 "{Not Found:키(대문자)}".</item>
/// <item>'`' 이스케이프: `n 줄바꿈, `r CR, `t 탭, 그 밖의 문자는 그 문자 자체 (`" → ", `{ → {).</item>
/// <item>{@미션.키} → 해당 미션 스크립트(missionSpec)의 머리 값. {&amp;레지스트리} 는 지원하지 않는다(찾지 못함 처리).</item>
/// <item>치환 결과 끝의 공백·탭은 잘라 낸다 (첫 글자는 남긴다).</item>
/// </list>
/// </summary>
public sealed class ConfigStore
{
    /// <summary>치환 여는 괄호 (원본 DAT_00532539)</summary>
    private const char OpenBrace = '{';

    /// <summary>치환 닫는 괄호 (원본 DAT_0053253a)</summary>
    private const char CloseBrace = '}';

    /// <summary>기본값 구분자 (원본 DAT_0053253b)</summary>
    private const char DefaultSeparator = '|';

    /// <summary>찾지 못한 키 표시 접두어 (원본 문자열 "Not Found:")</summary>
    public const string NotFoundPrefix = "Not Found:";

    /// <summary>
    /// 치환 중첩 한도. 원본에는 없지만 값이 자기 자신을 참조하면 무한 반복되므로 막는다.
    /// </summary>
    private const int MaxDepth = 32;

    /// <summary>쌓인 층 (앞이 바닥). 접두어는 "local." 형식, 이름 없는 층은 ""</summary>
    private readonly List<(string Prefix, ConfigText Text)> _layers = [];

    /// <summary>
    /// {@미션.키} 조회에 쓸 미션 텍스트 공급자. 인자는 미션 이름, 결과는 미션 스크립트 설정 텍스트(없으면 null).
    /// </summary>
    public Func<string, ConfigText?>? MissionLoader { get; set; }

    /// <summary>현재 쌓인 층 수</summary>
    public int LayerCount => _layers.Count;

    /// <summary>층을 맨 위에 쌓는다</summary>
    /// <param name="text">설정 텍스트</param>
    /// <param name="scopeName">층 이름 (예: "local"). null 이면 모든 키를 받는 층</param>
    public void Push(ConfigText text, string? scopeName = null) =>
        _layers.Add((string.IsNullOrEmpty(scopeName) ? "" : scopeName + ".", text));

    /// <summary>맨 위 층을 걷어 낸다</summary>
    public void Pop() => _layers.RemoveAt(_layers.Count - 1);

    /// <summary>
    /// 키의 원시 값(치환 전)을 찾는다. 맨 위 층부터 조회하며 이름 있는 층은 접두어가 맞을 때만 조회한다.
    /// </summary>
    /// <param name="key">설정 키 (대소문자 무시)</param>
    /// <param name="raw">원시 값</param>
    public bool TryGetRaw(string key, out string raw)
    {
        if (key.StartsWith('@'))
        {
            return TryGetMissionValue(key, out raw);
        }
        if (key.StartsWith('&'))
        {
            raw = "";
            return false;
        }
        // 맨 위 층부터 차례로 조회한다
        for (int i = _layers.Count - 1; i >= 0; i--)
        {
            (string prefix, ConfigText text) = _layers[i];
            if (prefix.Length == 0)
            {
                if (text.TryGetRaw(key, out raw))
                {
                    return true;
                }
            }
            else if (key.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)
                     && text.TryGetRaw(key[prefix.Length..], out raw))
            {
                return true;
            }
        }
        raw = "";
        return false;
    }

    /// <summary>키의 값을 치환까지 마쳐 돌려준다. 없으면 null</summary>
    /// <param name="key">설정 키 (대소문자 무시)</param>
    public string? Get(string key) => TryGetRaw(key, out string raw) ? Expand(raw) : null;

    /// <summary>
    /// 경로 지정 키(예: "missionSpec")를 {local.1}, {local.2} … 인자와 함께 치환한다 (원본 FUN_004410f0).
    /// 예: ExpandSpec("missionSpec", "tutorial1") → "\D\tutorial1.english"
    /// </summary>
    /// <param name="specKey">지정 키</param>
    /// <param name="locals">local.1 부터 차례로 넣을 값</param>
    /// <returns>치환 결과, 키가 없으면 빈 문자열</returns>
    public string ExpandSpec(string specKey, params string[] locals)
    {
        var sb = new StringBuilder();
        // 인자를 1="값" 형식의 줄로 만든다
        for (int i = 0; i < locals.Length; i++)
        {
            sb.Append(i + 1).Append(" = \"").Append(EscapeValue(locals[i])).Append("\"\n");
        }
        Push(new ConfigText(sb.ToString()), "local");
        try
        {
            return Get(specKey) ?? "";
        }
        finally
        {
            Pop();
        }
    }

    /// <summary>문자열 속 {키} 와 '`' 이스케이프를 치환한다</summary>
    /// <param name="text">치환할 문자열</param>
    public string Expand(string text) => Expand(text, 0);

    /// <summary>
    /// 값에 넣을 문자열의 따옴표·이스케이프 문자·중괄호를 '`' 로 감싼다 (조회 후 치환하면 원래 문자열이 된다).
    /// </summary>
    /// <param name="value">원래 문자열</param>
    public static string EscapeValue(string value)
    {
        var sb = new StringBuilder(value.Length);
        // 특수 문자 앞에 이스케이프 문자를 붙인다
        foreach (char c in value)
        {
            if (c is '"' or ConfigText.EscapeChar or OpenBrace or CloseBrace)
            {
                sb.Append(ConfigText.EscapeChar);
            }
            sb.Append(c);
        }
        return sb.ToString();
    }

    /// <summary>키-값 쌍들로 설정 텍스트를 만든다 (값은 <see cref="EscapeValue"/> 로 감싼다)</summary>
    /// <param name="pairs">키와 값</param>
    public static ConfigText FromPairs(IEnumerable<KeyValuePair<string, string>> pairs)
    {
        var sb = new StringBuilder();
        // 한 쌍을 한 줄로 쓴다
        foreach ((string key, string value) in pairs)
        {
            sb.Append(key).Append(" = \"").Append(EscapeValue(value)).Append("\"\n");
        }
        return new ConfigText(sb.ToString());
    }

    /// <summary>치환 본체 (원본 FUN_00440b60)</summary>
    /// <param name="text">치환할 문자열</param>
    /// <param name="depth">현재 중첩 깊이</param>
    private string Expand(string text, int depth)
    {
        var sb = new StringBuilder(text.Length);
        int i = 0;
        // 문자를 하나씩 보며 이스케이프와 중괄호를 처리한다
        while (i < text.Length)
        {
            char c = text[i];
            if (c == ConfigText.EscapeChar)
            {
                if (i + 1 < text.Length)
                {
                    sb.Append(text[i + 1] switch
                    {
                        'n' => '\n',
                        'r' => '\r',
                        't' => '\t',
                        char other => other,
                    });
                }
                i += 2;
            }
            else if (c == OpenBrace)
            {
                i = ExpandBrace(text, i, sb, depth);
            }
            else
            {
                sb.Append(c);
                i++;
            }
        }
        TrimTrailingBlanks(sb);
        return sb.ToString();
    }

    /// <summary>
    /// '{' 위치부터 {키|기본값} 하나를 읽어 치환 결과를 붙이고, '}' 다음 위치를 돌려준다 (원본 FUN_00440a00).
    /// 키·기본값 안의 중첩된 {..} 는 먼저 치환한다. 키는 대문자로 모은다(대소문자 무시 조회이므로 표시에만 영향).
    /// </summary>
    /// <param name="text">전체 문자열</param>
    /// <param name="start">'{' 위치</param>
    /// <param name="output">결과를 붙일 곳</param>
    /// <param name="depth">현재 중첩 깊이</param>
    private int ExpandBrace(string text, int start, StringBuilder output, int depth)
    {
        int j = start + 1;
        var key = new StringBuilder();
        // 키: '}' 또는 '|' 까지 (중첩 괄호는 치환해서 붙인다)
        while (j < text.Length && text[j] != CloseBrace && text[j] != DefaultSeparator)
        {
            if (text[j] == OpenBrace)
            {
                j = ExpandBrace(text, j, key, depth + 1);
            }
            else
            {
                key.Append(char.ToUpperInvariant(text[j]));
                j++;
            }
        }
        var fallback = new StringBuilder();
        if (j < text.Length && text[j] == DefaultSeparator)
        {
            j++;
            // 기본값: '}' 까지 (중첩 괄호는 치환해서 붙인다)
            while (j < text.Length && text[j] != CloseBrace)
            {
                if (text[j] == OpenBrace)
                {
                    j = ExpandBrace(text, j, fallback, depth + 1);
                }
                else
                {
                    fallback.Append(text[j]);
                    j++;
                }
            }
        }
        Lookup(key.ToString(), fallback.ToString(), output, depth);
        return j < text.Length ? j + 1 : j;
    }

    /// <summary>
    /// 키를 찾아 치환한 값을 붙인다. 없으면 기본값, 기본값도 비었으면 "{Not Found:키}" (원본 FUN_00440760).
    /// </summary>
    /// <param name="key">키</param>
    /// <param name="fallback">기본값 (없으면 빈 문자열)</param>
    /// <param name="output">결과를 붙일 곳</param>
    /// <param name="depth">현재 중첩 깊이</param>
    private void Lookup(string key, string fallback, StringBuilder output, int depth)
    {
        bool registry = key.StartsWith('&');
        if (depth < MaxDepth && TryGetRaw(key, out string raw))
        {
            output.Append(Expand(raw, depth + 1));
        }
        else if (fallback.Length > 0)
        {
            output.Append(depth < MaxDepth ? Expand(fallback, depth + 1) : fallback);
        }
        else if (!registry)
        {
            output.Append(OpenBrace).Append(NotFoundPrefix).Append(key).Append(CloseBrace);
        }
    }

    /// <summary>{@미션.키}: 미션 스크립트를 불러와 머리 값을 읽는다</summary>
    /// <param name="key">"@미션.키" 형식</param>
    /// <param name="raw">원시 값</param>
    private bool TryGetMissionValue(string key, out string raw)
    {
        raw = "";
        int dot = key.IndexOf('.');
        if (dot < 0 || MissionLoader == null)
        {
            return false;
        }
        ConfigText? mission = MissionLoader(key[1..dot]);
        return mission != null && mission.TryGetRaw(key[(dot + 1)..], out raw);
    }

    /// <summary>끝의 공백·탭을 잘라 낸다. 원본처럼 첫 글자는 남긴다</summary>
    /// <param name="sb">대상</param>
    private static void TrimTrailingBlanks(StringBuilder sb)
    {
        // 두 글자 이상 남아 있는 동안 끝의 공백·탭을 지운다
        while (sb.Length > 1 && sb[^1] is ' ' or '\t')
        {
            sb.Length--;
        }
    }
}
