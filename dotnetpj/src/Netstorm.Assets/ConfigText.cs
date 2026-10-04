using System.Text;

namespace Netstorm.Assets;

/// <summary>
/// 원본 설정 형식 텍스트(setup.cfg, options.cfg, config.&lt;언어&gt;, 미션 스크립트 머리)를 원본과 같은 규칙으로 조회한다.
/// 원본 Config.cpp 분석 결과 (docs/formats/config.md "설정 조회 규칙"):
/// <list type="bullet">
/// <item>줄 앞 공백·탭을 건너뛴 뒤 "키 [공백] =" 로 시작하는 줄을 찾는다. 키는 대소문자를 무시하며 공백을 포함할 수 있다.</item>
/// <item>같은 키가 여러 번 있으면 <b>처음 나온 줄</b>을 쓴다 (섹션 [..] 구분은 무시하고 텍스트 전체를 훑는다).</item>
/// <item>값: '=' 뒤 공백을 건너뛰고 줄 끝까지. 따옴표 밖의 "//" 부터는 주석. 큰따옴표는 떼어 내며, 앞에 '`' 가 붙은 따옴표는 그대로 둔다.</item>
/// <item>'`' 이스케이프는 조회 단계에서는 남겨 두고 치환(<see cref="ConfigStore.Expand"/>) 단계에서 푼다.</item>
/// </list>
/// </summary>
public sealed class ConfigText
{
    /// <summary>이스케이프 문자 (원본 DAT_00532538)</summary>
    public const char EscapeChar = '`';

    /// <summary>따옴표 밖 주석 시작 (원본 PTR_DAT_00532540)</summary>
    private const string CommentMarker = "//";

    /// <summary>줄 단위로 나눈 텍스트 (CR 제거)</summary>
    private readonly string[] _lines;

    /// <summary>원본 텍스트</summary>
    public string Text { get; }

    /// <summary>텍스트로부터 만든다</summary>
    /// <param name="text">설정 텍스트</param>
    public ConfigText(string text)
    {
        Text = text;
        _lines = text.Replace("\r", "", StringComparison.Ordinal).Split('\n');
    }

    /// <summary>
    /// 원본 설정 파일 바이트를 읽는다. 원본(FUN_00440380)처럼 첫 바이트와 세 번째 바이트가 0 이면
    /// XOR 인코딩된 파일로 보고 복호화한 뒤 서명 "mQdsT" 를 떼어 낸다. 평문 파일(config.english 등)은 그대로 읽는다.
    /// </summary>
    /// <param name="data">파일 내용</param>
    public static ConfigText FromFileBytes(byte[] data)
    {
        if (data.Length >= 3 && data[0] == 0 && data[2] == 0)
        {
            return new ConfigText(ConfigFile.Decode(data).Text);
        }
        return new ConfigText(OriginalText.Decode(data));
    }

    /// <summary>
    /// 키의 원시 값(따옴표 제거, 이스케이프·치환 전)을 찾는다. 처음 나온 줄을 쓴다.
    /// </summary>
    /// <param name="key">설정 키 (대소문자 무시)</param>
    /// <param name="value">원시 값</param>
    public bool TryGetRaw(string key, out string value)
    {
        // 처음으로 키가 일치하는 줄을 찾는다
        foreach (string line in _lines)
        {
            int valueStart = MatchKey(line, key);
            if (valueStart >= 0)
            {
                value = ReadValue(line, valueStart);
                return true;
            }
        }
        value = "";
        return false;
    }

    /// <summary>키의 원시 값을 돌려준다. 없으면 null</summary>
    /// <param name="key">설정 키 (대소문자 무시)</param>
    public string? GetRaw(string key) => TryGetRaw(key, out string value) ? value : null;

    /// <summary>
    /// 값이 있는 모든 키를 나온 순서대로 돌려준다 (같은 키는 처음 것만). 키는 원본 표기 그대로다.
    /// 주석 줄(//), 섹션 줄([..]), '=' 가 없는 줄은 제외한다.
    /// </summary>
    public IReadOnlyList<string> Keys()
    {
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var keys = new List<string>();
        // 줄마다 '=' 앞부분을 키로 본다
        foreach (string line in _lines)
        {
            string trimmed = line.TrimStart(' ', '\t');
            int eq = trimmed.IndexOf('=');
            if (eq <= 0 || trimmed.StartsWith(CommentMarker, StringComparison.Ordinal) || trimmed.StartsWith('['))
            {
                continue;
            }
            string key = trimmed[..eq].TrimEnd(' ', '\t');
            if (key.Length > 0 && seen.Add(key))
            {
                keys.Add(key);
            }
        }
        return keys;
    }

    /// <summary>
    /// 줄이 키로 시작하면 값 시작 위치('=' 다음)를 돌려준다. 아니면 -1.
    /// 원본 FUN_0043fd70: 앞 공백 건너뜀 → 키 길이만큼 대소문자 무시 비교 → 공백 건너뜀 → '=' 확인
    /// </summary>
    /// <param name="line">검사할 줄</param>
    /// <param name="key">설정 키</param>
    private static int MatchKey(string line, string key)
    {
        int pos = 0;
        // 줄 앞 공백·탭 건너뛰기
        while (pos < line.Length && line[pos] is ' ' or '\t')
        {
            pos++;
        }
        if (key.Length == 0 || pos + key.Length > line.Length
            || string.Compare(line, pos, key, 0, key.Length, StringComparison.OrdinalIgnoreCase) != 0)
        {
            return -1;
        }
        pos += key.Length;
        // 키 뒤 공백·탭 건너뛰기
        while (pos < line.Length && line[pos] is ' ' or '\t')
        {
            pos++;
        }
        return pos < line.Length && line[pos] == '=' ? pos : -1;
    }

    /// <summary>
    /// 값 부분을 읽는다 (원본 FUN_00440150). '=' 와 공백을 건너뛴 뒤, 따옴표를 떼고 주석 전까지 복사한다.
    /// </summary>
    /// <param name="line">키가 일치한 줄</param>
    /// <param name="pos">'=' 위치</param>
    private static string ReadValue(string line, int pos)
    {
        // '=' · 공백 · 탭 건너뛰기
        while (pos < line.Length && line[pos] is '=' or ' ' or '\t')
        {
            pos++;
        }
        var sb = new StringBuilder();
        bool quoted = false;
        char previous = '\0';
        // 줄 끝 또는 따옴표 밖 주석까지 복사한다
        for (; pos < line.Length; pos++)
        {
            char c = line[pos];
            if (!quoted && string.CompareOrdinal(line, pos, CommentMarker, 0, CommentMarker.Length) == 0)
            {
                break;
            }
            if (c == '"' && previous != EscapeChar)
            {
                quoted = !quoted;
            }
            else
            {
                sb.Append(c);
            }
            previous = c;
        }
        return sb.ToString();
    }
}
