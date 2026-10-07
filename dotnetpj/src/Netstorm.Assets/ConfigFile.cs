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
        // 원본 Config::LoadBytes는 서명을 남겨 첫 키를 가린다. 클론은 InstallDir을 조회하지 않고 VFS 루트를 쓰며,
        // analyzeManager의 복사본 편집·바이트 왕복을 위해 기존의 서명 제거 API를 유지한다.
        return new ConfigFile(TextEncoding.GetString(plain, Signature.Length, plain.Length - Signature.Length));
    }

    /// <summary>파일을 읽어 복호화한다</summary>
    /// <param name="path">.cfg 경로</param>
    public static ConfigFile Load(string path) => Decode(File.ReadAllBytes(path));

    /// <summary>서명을 뗀 단일 파일 본문에 서명을 붙여 인코딩한다. analyzeManager의 복사본 설정 편집도 이 API를 쓴다.</summary>
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
    /// 원본 004406b0의 저장 형식: 파일 이름 섹션 + END 본문을 XOR 인코딩한다.
    /// 파일 이름 섹션이 없으면 서명도 붙이지 않는다. Encode와 구분하여 기존 분석기 동작을 보존한다.
    /// </summary>
    public byte[] EncodeSections(string fileName)
    {
        string name = fileName.Replace('\\', '/').Split('/')[^1];
        var config = new ConfigText(Text);
        string? body = config.Section(name);
        string plain = body == null ? "" : body.StartsWith("mQdsT", StringComparison.Ordinal) ? body : "mQdsT" + body;
        plain += config.Section("END") ?? "";
        byte[] data = TextEncoding.GetBytes(plain);
        XorCipher.Apply(data);
        return data;
    }

    /// <summary>
    /// 키의 원시 값(따옴표 제거, 치환 전)을 읽는다. 'key = "값"' 과 'key=값' 두 형식을 모두 지원하며 키는 대소문자를 무시한다.
    /// 원본(Config.cpp)과 같이 같은 키가 여러 번 나오면 <b>처음</b> 나온 값을 쓴다. 주석 처리된 줄(//)은 무시한다.
    /// 끝의 공백·탭은 잘라 낸다. 규칙: <see cref="ConfigText"/>
    /// </summary>
    /// <param name="key">설정 키</param>
    /// <returns>값, 없으면 null</returns>
    public string? Get(string key) => new ConfigText(Text).GetRaw(key)?.TrimEnd(' ', '\t');

    /// <summary>
    /// 'key = "값"' 형식으로 값을 바꾼다. 처음으로 키가 나온 줄(조회에 쓰이는 줄)을 통째로 바꾸며,
    /// 키가 없으면 끝에 새 줄로 추가한다. 줄 앞 들여쓰기와 키 표기는 유지하고, 그 줄의 주석은 사라진다.
    /// 원본 Set은 같은 키를 모두 지우고 END 뒤에 쓴다. 이 함수는 analyzeManager의 복사본 편집용으로 첫 줄 교체를 유지한다.
    /// </summary>
    /// <param name="key">설정 키</param>
    /// <param name="value">새 값</param>
    public void Set(string key, string value)
    {
        var regex = new Regex($@"^([ \t]*)({Regex.Escape(key)})[ \t]*=[^\r\n]*", RegexOptions.Multiline | RegexOptions.IgnoreCase);
        if (regex.IsMatch(Text))
        {
            Text = regex.Replace(Text, m => $"{m.Groups[1].Value}{m.Groups[2].Value} = \"{value}\"", 1);
            return;
        }
        string newline = Text.Contains("\r\n", StringComparison.Ordinal) ? "\r\n" : "\n";
        if (Text.Length > 0 && !Text.EndsWith('\n'))
        {
            Text += newline;
        }
        Text += $"{key} = \"{value}\"{newline}";
    }
}
