using System.Text;
using System.Text.RegularExpressions;

namespace Netstorm.Assets;

/// <summary>
/// 원본 설정 파일(options.cfg, setup.cfg). 파일 전체가 XOR 인코딩되어 있고,
/// 평문은 5바이트 서명 "mQdsT" 로 시작한다. 포맷: docs/formats/config.md
/// </summary>
public sealed partial class ConfigFile
{
    /// <summary>복호화된 평문 앞의 서명</summary>
    private static ReadOnlySpan<byte> Signature => "mQdsT"u8;

    /// <summary>원본 텍스트 인코딩 (Windows-1252 범위)</summary>
    private static readonly Encoding TextEncoding = Encoding.Latin1;

    /// <summary>서명을 뺀 설정 텍스트 (원본 줄바꿈 유지)</summary>
    public string Text { get; private set; }

    /// <summary>텍스트로부터 만든다</summary>
    /// <param name="text">서명을 뺀 설정 텍스트</param>
    public ConfigFile(string text) => Text = text;

    /// <summary>인코딩된 파일 내용을 복호화한다</summary>
    /// <param name="encoded">파일 내용</param>
    public static ConfigFile Decode(byte[] encoded)
    {
        byte[] plain = (byte[])encoded.Clone();
        XorCipher.Apply(plain);
        if (!plain.AsSpan().StartsWith(Signature))
        {
            throw new InvalidDataException("설정 파일 서명(mQdsT)이 맞지 않습니다.");
        }
        return new ConfigFile(TextEncoding.GetString(plain, Signature.Length, plain.Length - Signature.Length));
    }

    /// <summary>파일을 읽어 복호화한다</summary>
    /// <param name="path">.cfg 경로</param>
    public static ConfigFile Load(string path) => Decode(File.ReadAllBytes(path));

    /// <summary>서명을 붙여 인코딩한 파일 내용을 돌려준다 (원본과 바이트 단위로 같게 복원된다)</summary>
    public byte[] Encode()
    {
        byte[] body = TextEncoding.GetBytes(Text);
        var data = new byte[Signature.Length + body.Length];
        Signature.CopyTo(data);
        body.CopyTo(data, Signature.Length);
        XorCipher.Apply(data);
        return data;
    }

    /// <summary>
    /// 키의 값을 읽는다. 'key = "값"' 과 'key=값' 두 형식을 모두 지원하며 키는 대소문자를 무시한다.
    /// 같은 키가 여러 번 나오면 마지막 값을 쓴다 (setup.cfg 에 주석 처리 후 재정의한 예가 있음).
    /// </summary>
    /// <param name="key">설정 키</param>
    /// <returns>값, 없으면 null</returns>
    public string? Get(string key)
    {
        string? result = null;
        // 모든 줄을 검사해 마지막으로 나온 값을 취한다
        foreach (string rawLine in Text.Split('\n'))
        {
            int comment = rawLine.IndexOf("//", StringComparison.Ordinal);
            string line = (comment >= 0 ? rawLine[..comment] : rawLine).Trim();
            Match m = EntryRegex().Match(line);
            if (m.Success && string.Equals(m.Groups[1].Value, key, StringComparison.OrdinalIgnoreCase))
            {
                result = m.Groups[2].Success && m.Groups[2].Value.Length > 0 ? m.Groups[2].Value : m.Groups[3].Value.Trim();
            }
        }
        return result;
    }

    /// <summary>
    /// 'key = "값"' 형식으로 값을 바꾼다. 키가 없으면 끝에 새 줄로 추가한다.
    /// </summary>
    /// <param name="key">설정 키</param>
    /// <param name="value">새 값</param>
    public void Set(string key, string value)
    {
        var regex = new Regex($@"^(\s*{Regex.Escape(key)}\s*=\s*)""[^""]*""", RegexOptions.Multiline | RegexOptions.IgnoreCase);
        if (regex.IsMatch(Text))
        {
            Text = regex.Replace(Text, m => $"{m.Groups[1].Value}\"{value}\"", 1);
            return;
        }
        string newline = Text.Contains("\r\n", StringComparison.Ordinal) ? "\r\n" : "\n";
        Text += $"{key} = \"{value}\"{newline}";
    }

    /// <summary>설정 줄: 키 = "값" 또는 키 = 값</summary>
    [GeneratedRegex(@"^([A-Za-z_][\w.]*)\s*=\s*(?:""([^""]*)""|(.*))$")]
    private static partial Regex EntryRegex();
}
