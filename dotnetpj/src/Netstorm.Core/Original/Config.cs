using Netstorm.Assets;

namespace Netstorm.Core.Original;

/// <summary>
/// 원본 설정 객체 층·치환·값 쓰기·섹션 (o/Config.h/.cpp).
/// 근거: cpppj/src/o/Config.h (0043fd70·00440150·00440380·객체 등록·00440760·00440a00·00440b60·00440240·004404e0·0043fe60·00440570·00440640).
/// (R3k-10, 2026-10-30)
/// 고정 길이 버퍼(패치 8KB, CD 4KB)를 C# 동적 문자열로 바꿨다. 바이트 단위 해석은 Latin1 1문자 1바이트로 보존한다.
/// </summary>
public static class ConfigTextOps
{
    /// <summary>원본 DAT_00532538~3e(CD 0051846c~) 설정 문법 특수 문자. 주석·괄호·섹션 규칙은 원본과 같다.</summary>
    public const char Escape = '`';

    /// <summary>치환 시작 문자.</summary>
    public const char OpenBrace = '{';

    /// <summary>치환 끝 문자.</summary>
    public const char CloseBrace = '}';

    /// <summary>기본값 구분 문자.</summary>
    public const char DefaultSeparator = '|';

    /// <summary>섹션 시작 문자.</summary>
    public const char OpenSection = '[';

    /// <summary>섹션 끝 문자.</summary>
    public const char CloseSection = ']';

    /// <summary>객체 접두부 구분 문자.</summary>
    public const char ObjectSeparator = '.';

    /// <summary>원본 저장 설정 파일의 평문 서명(DAT_00532564).</summary>
    public const string Signature = "mQdsT";
}

/// <summary>
/// 설정 문자열에서 키·값을 찾는 처리기이다.
/// 근거: cpppj/src/o/Config.h의 ConfigText. (R3k-10, 2026-10-30)
/// </summary>
public sealed class ConfigText
{
    /// <summary>복원·검증 요구가 읽는 버퍼. NUL 뒤는 버린다.</summary>
    private string _text;

    /// <summary>UTF-8 설정 버퍼를 받아 원본 NUL 앞에서 그대로 끊는다.</summary>
    /// <param name="text">설정 본문.</param>
    public ConfigText(string text)
    {
        int nul = text.IndexOf('\0');
        _text = nul < 0 ? text : text.Substring(0, nul);
    }

    /// <summary>원본 00440380: 첫째·셋째 바이트 0이면 XOR 파일을 감싼다. 서명이 없으면 거부한다.</summary>
    /// <param name="bytes">파일 바이트.</param>
    public static ConfigText FromBytes(byte[] bytes)
    {
        byte[] plain = (byte[])bytes.Clone();
        if (plain.Length >= 3 && plain[0] == 0 && plain[2] == 0)
        {
            XorCipher.Apply(plain);
            if (!StartsWithSignature(plain))
            {
                throw new InvalidDataException("인코딩된 설정 서명이 맞지 않습니다");
            }
            byte[] stripped = new byte[plain.Length - ConfigTextOps.Signature.Length];
            Array.Copy(plain, ConfigTextOps.Signature.Length, stripped, 0, stripped.Length);
            plain = stripped;
        }
        // 바이트 단위 Windows-1252 해석이 원본 DecodeOriginalText와 같다
        return new ConfigText(OriginalText.DecodeWindows1252(plain));
    }

    /// <summary>서명 바이트로 시작하는지 확인한다.</summary>
    private static bool StartsWithSignature(byte[] plain)
    {
        if (plain.Length < ConfigTextOps.Signature.Length)
        {
            return false;
        }
        // 서명 길이만큼 바이트를 견준다
        for (int i = 0; i < ConfigTextOps.Signature.Length; i++)
        {
            if (plain[i] != (byte)ConfigTextOps.Signature[i])
            {
                return false;
            }
        }
        return true;
    }

    /// <summary>원본 0043fd70: 버퍼 앞에서 키가 있는 줄의 시작 위치를 찾는다. NUL 뒤는 보지 않는다.</summary>
    /// <param name="key">찾을 키.</param>
    public int? FindKey(string key) => FindKeyIn(_text, key);

    /// <summary>원본 0043fd70: 버퍼 앞에서 키 위치를 찾는다.</summary>
    /// <param name="text">설정 버퍼.</param>
    /// <param name="key">찾을 키.</param>
    public static int? FindKeyIn(string text, string key)
    {
        text = CutNul(text);
        if (key.Length == 0)
        {
            return null;
        }
        int pos = 0;
        // 버퍼 안에서 키가 있는 줄의 시작 위치를 반환한다
        while (pos < text.Length)
        {
            // 앞의 공백·탭을 건너뛴다
            while (pos < text.Length && IsBlank(text[pos]))
            {
                pos++;
            }
            int start = pos;
            if (StartsWithFolded(text.Substring(pos), key))
            {
                pos += key.Length;
                // 공백·탭을 건너뛰어 '='를 확인한다
                while (pos < text.Length && IsBlank(text[pos]))
                {
                    pos++;
                }
                if (pos < text.Length && text[pos] == '=')
                {
                    return start;
                }
            }
            pos = NextLine(text, start);
        }
        return null;
    }

    /// <summary>원본 00440150: 버퍼 앞에서 키 위치의 원시 값을 읽는다.</summary>
    /// <param name="key">찾을 키.</param>
    public string? GetRaw(string key) => GetRawFrom(_text, key);

    /// <summary>원본 00440150: 따옴표 처리·따옴표 밖 주석 제거로 복사한다.</summary>
    /// <param name="text">설정 버퍼.</param>
    /// <param name="key">찾을 키.</param>
    public static string? GetRawFrom(string text, string key)
    {
        text = CutNul(text);
        int? found = FindKeyIn(text, key);
        if (found == null)
        {
            return null;
        }
        int pos = text.IndexOf('=', found.Value);
        // 원본은 첫 '=' 뒤의 공백을 건너뛴다
        while (pos < text.Length && (text[pos] == '=' || IsBlank(text[pos])))
        {
            pos++;
        }
        var result = new System.Text.StringBuilder();
        bool quoted = false;
        char previous = '\0';
        // 따옴표·따옴표 밖 주석 전까지 복사한다
        for (; pos < text.Length; pos++)
        {
            char c = text[pos];
            if (c == '\r' || c == '\n' || (!quoted && c == '/' && pos + 1 < text.Length && text[pos + 1] == '/'))
            {
                break;
            }
            if (c == '"' && previous != ConfigTextOps.Escape)
            {
                quoted = !quoted;
            }
            else
            {
                result.Append(c);
            }
            previous = c;
        }
        return result.ToString();
    }

    /// <summary>원본 0043ffa0: 입력 끝의 줄바꿈을 CRLF 하나로 정리해 이어 붙인다.</summary>
    /// <param name="text">이어 붙일 본문.</param>
    public void Append(string text)
    {
        int end = text.IndexOf('\0');
        if (end < 0)
        {
            end = text.Length;
        }
        // 입력 끝의 CR/LF를 정리한다
        while (end > 0 && (text[end - 1] == '\r' || text[end - 1] == '\n'))
        {
            end--;
        }
        _text += text.Substring(0, end);
        _text += "\r\n";
    }

    /// <summary>복원·검증 요구가 읽는 버퍼를 그대로 돌려준다.</summary>
    public string Text => _text;

    /// <summary>NUL 앞에서 자른다.</summary>
    private static string CutNul(string text)
    {
        int nul = text.IndexOf('\0');
        return nul < 0 ? text : text.Substring(0, nul);
    }

    /// <summary>공백·탭인지 확인한다.</summary>
    private static bool IsBlank(char c) => c == ' ' || c == '\t';

    /// <summary>줄 경계 공백인지 확인한다.</summary>
    private static bool IsLineGap(char c) => c == '\n' || c == '\r' || c == ' ' || c == '\t';

    /// <summary>다음 줄 첫 글자 위치를 구한다. LF가 없으면 끝이다.</summary>
    private static int NextLine(string text, int pos)
    {
        int newline = text.IndexOf('\n', pos);
        if (newline < 0)
        {
            return text.Length;
        }
        pos = newline;
        // 빈 줄과 다음 줄의 여백까지 함께 건너뛴다
        while (pos < text.Length && IsLineGap(text[pos]))
        {
            pos++;
        }
        return pos;
    }

    /// <summary>앞부분 대소문자 무시 비교. 짧으면 거짓이다.</summary>
    private static bool StartsWithFolded(string left, string right)
    {
        if (right.Length > left.Length)
        {
            return false;
        }
        // 양쪽을 같은 소문자로 견주어 비교한다
        for (int i = 0; i < right.Length; i++)
        {
            if (ToLowerAscii(left[i]) != ToLowerAscii(right[i]))
            {
                return false;
            }
        }
        return true;
    }

    /// <summary>ASCII 대문자를 소문자로 바꾼다.</summary>
    private static char ToLowerAscii(char c) => c >= 'A' && c <= 'Z' ? (char)(c + ('a' - 'A')) : c;

    /// <summary>ASCII 소문자를 대문자로 바꾼다.</summary>
    private static char ToUpperAscii(char c) => c >= 'a' && c <= 'z' ? (char)(c - ('a' - 'A')) : c;
}

/// <summary>
/// 원본 설정 객체 등록부. 조회는 나중에 등록한 설정 객체부터 본다.
/// 근거: cpppj/src/o/Config.h의 ConfigRegistry. (R3k-10, 2026-10-30)
/// </summary>
public sealed class ConfigRegistry
{
    /// <summary>위치 0043fc20 기준 `totalConfigs &lt; MAX_CONFIGS` 한도. CD는 0042a010에서 40이다.</summary>
    public const int MaxConfigs = 1000;

    /// <summary>현재 등록된 집합 계열의 객체 목록. 등록 순서(나중 것이 먼저 조회된다)를 유지한다.</summary>
    private readonly List<Config> _objects = [];

    /// <summary>등록 순서(나중 것이 먼저 조회된다) 설정 객체 목록.</summary>
    public IReadOnlyList<Config> Objects => _objects;

    /// <summary>`{_이름}` 환경 변수 조회(원본 getenv). 비어 있으면 찾지 못한 것으로 처리한다.</summary>
    public Func<string, string?>? Environment { get; set; }

    /// <summary>`{&amp;경로}` 저장소 조회(원본 004dda10). 비어 있으면 찾지 못한 것으로 처리한다.</summary>
    public Func<string, string?>? RegistryValue { get; set; }

    /// <summary>`{@미션.이름}`: 임시 설정 객체에 미션 스크립트를 읽어 넣는다. 원본은 missionSpec 경로를 따른다.</summary>
    public Action<Config, string>? LoadMission { get; set; }

    /// <summary>원본 0043fb90: 이름("이름.")과 같은 첫 객체. 없으면 null. 대소문자를 무시하고 비교한다.</summary>
    /// <param name="name">찾을 이름.</param>
    public Config? FindByName(string name)
    {
        string wanted = ToLowerAscii(name + ConfigTextOps.ObjectSeparator);
        // 먼저 등록한 객체부터 찾는다(원본은 0개 초과 검사다)
        foreach (Config config in _objects)
        {
            if (ToLowerAscii(config.Prefix) == wanted)
            {
                return config;
            }
        }
        return null;
    }

    /// <summary>원본 0043fc20: 경계 안의 배열에 넣는다. 원본처럼 한도를 넘기면 예외로 멈춘다.</summary>
    /// <param name="config">등록할 객체.</param>
    internal void Register(Config config)
    {
        if (_objects.Count >= MaxConfigs)
        {
            throw new InvalidOperationException("totalConfigs < MAX_CONFIGS");
        }
        _objects.Add(config);
    }

    /// <summary>원본 0043fab0: 같은 포인터를 모두 빼고 나머지는 앞으로 당긴다.</summary>
    /// <param name="config">빼낼 객체.</param>
    internal void Unregister(Config config)
    {
        // 같은 포인터를 모두 뺀다
        for (int i = _objects.Count - 1; i >= 0; i--)
        {
            if (ReferenceEquals(_objects[i], config))
            {
                _objects.RemoveAt(i);
            }
        }
    }

    /// <summary>ASCII 대소문자를 소문자로 바꾼 문자열을 만든다.</summary>
    private static string ToLowerAscii(string text)
    {
        char[] chars = text.ToCharArray();
        // ASCII 대문자만 소문자로 바꾼다
        for (int i = 0; i < chars.Length; i++)
        {
            if (chars[i] >= 'A' && chars[i] <= 'Z')
            {
                chars[i] = (char)(chars[i] + ('a' - 'A'));
            }
        }
        return new string(chars);
    }
}

/// <summary>
/// 원본 설정 객체(0x18바이트: +0 버퍼, +8 환경 변수 허용, +0xc "이름.", +0x10 이름 길이, +0x14 변경 표시).
/// 근거: cpppj/src/o/Config.h의 Config. (R3k-10, 2026-10-30)
/// </summary>
public sealed class Config : IDisposable
{
    /// <summary>원본 00440a00의 키·기본값 수집 한도(정치pinned 버퍼 0x2000바이트, CD는 0x1000).</summary>
    private const int LongestSymbol = 0x2000;

    /// <summary>치환 재귀 보호 한도. 이 깊이를 넘긴 참조는 찾지 못한 것으로 처리한다.</summary>
    private const int MaxSubstitutionDepth = 32;

    /// <summary>원본 설정 객체를 조회하는 등록부. 수명은 호출자가 관리한다.</summary>
    private readonly ConfigRegistry _registry;

    /// <summary>접두부("이름." 꼴) 또는 빈 문자열. 이름 없는 객체는 접두부 길이가 0이다.</summary>
    private readonly string _prefix = "";

    /// <summary>설정 버퍼. 없으면 조회·치환이 찾지 못한다.</summary>
    private string? _text;

    /// <summary>원본 0043fc20: 이름이 있으면 "이름." 접두부로 만들고 영역 목록에 등록한다.</summary>
    /// <param name="registry">등록부.</param>
    /// <param name="name">객체 이름. 없으면 접두부가 없다.</param>
    public Config(ConfigRegistry registry, string? name = null)
    {
        ArgumentNullException.ThrowIfNull(registry);
        _registry = registry;
        if (name != null)
        {
            _prefix = CutNul(name) + ConfigTextOps.ObjectSeparator;
        }
        _registry.Register(this);
    }

    /// <summary>원본 0043fcd0: 목록에서 뺀다. 버퍼 문자열 해제는 GC에 맡긴다.</summary>
    public void Dispose()
    {
        _registry.Unregister(this);
        GC.SuppressFinalize(this);
    }

    /// <summary>원본 `*this != 0` 검사("theConfiguration.isValid()"). 버퍼가 한 번이라도 만들어졌는지 본다.</summary>
    public bool IsValid() => _text != null;

    /// <summary>원본 0043fd40: 버퍼를 버린다. 이름·등록은 그대로다.</summary>
    public void Clear() => _text = null;

    /// <summary>C 문자열처럼 NUL 앞까지만 둔다. 원본처럼 버퍼 포인터를 직접 다루는 것에 해당한다.</summary>
    /// <param name="text">둘 본문.</param>
    public void Assign(string text) => _text = CutNul(text);

    /// <summary>버퍼가 없으면 새로 만들고 입력 끝의 CRLF를 하나로 붙인다.</summary>
    /// <param name="text">이어 붙일 본문.</param>
    public void Append(string text)
    {
        text = CutNul(text);
        int end = text.Length;
        // 입력 끝의 CR/LF를 정리한다
        while (end > 0 && (text[end - 1] == '\r' || text[end - 1] == '\n'))
        {
            end--;
        }
        _text ??= "";
        _text += text.Substring(0, end);
        _text += "\r\n";
    }

    /// <summary>원본 00440330: `[이름]` 섹션 줄을 붙인다. 이름이 '['로 시작하면 원본처럼 거부한다.</summary>
    /// <param name="name">섹션 이름.</param>
    public void AppendSection(string name)
    {
        if (name.Length != 0 && name[0] == ConfigTextOps.OpenSection)
        {
            throw new ArgumentException("sectionName[0] != startSection", nameof(name));
        }
        Append(ConfigTextOps.OpenSection + name + ConfigTextOps.CloseSection);
    }

    /// <summary>원본 00440380: 복호화한 XOR 감싼 뒤 버퍼에 붙인다. 서명 `mQdsT`가 있으면 처음 한 줄의 조회가 된다.</summary>
    /// <param name="bytes">파일 바이트.</param>
    public void LoadBytes(byte[] bytes)
    {
        byte[] plain = (byte[])bytes.Clone();
        if (plain.Length >= 3 && plain[0] == 0 && plain[2] == 0)
        {
            XorCipher.Apply(plain);
        }
        int nul = Array.IndexOf(plain, (byte)0);
        if (nul >= 0)
        {
            Array.Resize(ref plain, nul);
        }
        bool hasSignature = plain.Length >= ConfigTextOps.Signature.Length;
        // 서명 길이만큼 바이트를 견준다
        for (int i = 0; hasSignature && i < ConfigTextOps.Signature.Length; i++)
        {
            if (plain[i] != (byte)ConfigTextOps.Signature[i])
            {
                hasSignature = false;
            }
        }
        int skip = hasSignature ? ConfigTextOps.Signature.Length : 0;
        // ASCII 서명 앞부분과 바이트 단위 해석 뒷부분을 그대로 둔다
        Append(Latin1(plain, 0, skip) + OriginalText.DecodeWindows1252(plain.AsSpan(skip).ToArray()));
    }

    /// <summary>앞 바이트 구간을 일대일 문자로 바꾼다.</summary>
    private static string Latin1(byte[] plain, int offset, int count)
    {
        char[] chars = new char[count];
        // 한 바이트를 한 문자로 둔다
        for (int i = 0; i < count; i++)
        {
            chars[i] = (char)plain[offset + i];
        }
        return new string(chars);
    }

    /// <summary>
    /// 원본 00440410: '@'로 시작하는 낱말은 '@파일' 꼴 파일 이름으로 먼저 읽어 넣고, 나머지 글은 줄에 붙인다.
    /// 글마다 '@파일' 낱말은 ';' 구분자로 처리한다.
    /// </summary>
    /// <param name="text">명령 인자.</param>
    /// <param name="readFile">파일 읽기. 없으면 파일 낱말을 건너뛴다.</param>
    public void AppendArguments(string text, Func<string, byte[]?>? readFile)
    {
        text = CutNul(text);
        if (text.Length == 0)
        {
            return;
        }
        string inlineText = "";
        int pos = 0;
        // 글마다 '@파일' 낱말을 ';' 구분자로 처리한다
        while (pos < text.Length)
        {
            if (text[pos] == '@')
            {
                int begin = pos;
                // 낱말은 공백·탭·';' 없는 데까지다. 낱말 자체는 버퍼에 복사하지 않는다
                while (pos < text.Length && !IsBlank(text[pos]) && text[pos] != ';')
                {
                    pos++;
                }
                if (readFile != null)
                {
                    byte[]? bytes = readFile(text.Substring(begin, pos - begin));
                    if (bytes != null)
                    {
                        LoadBytes(bytes);
                    }
                }
                if (pos == text.Length)
                {
                    break;
                }
            }
            inlineText += text[pos] == ';' ? "\r\n" : text[pos].ToString();
            pos++;
        }
        if (inlineText.Length != 0)
        {
            Append(inlineText);
        }
    }

    /// <summary>버퍼가 있는 객체의 원본 0043fd70 검사에 걸려 찾지 못한다.</summary>
    /// <param name="key">찾을 키.</param>
    public string? GetRaw(string key)
    {
        if (_text == null)
        {
            return null;
        }
        return ConfigText.GetRawFrom(_text, key);
    }

    /// <summary>원본 004409d0: 버퍼가 없으면 null. 찾아도 치환한 값이 없으면 "Not Found" 표시를 내지 않는다.</summary>
    /// <param name="key">찾을 키.</param>
    public string? Get(string key)
    {
        if (_text == null)
        {
            return null;
        }
        var dest = new System.Text.StringBuilder();
        if (!Lookup(CutNul(key), "", dest, false, 0))
        {
            return null;
        }
        return dest.ToString();
    }

    /// <summary>원본 문자열에 치환 결과를 만든다.</summary>
    /// <param name="source">원본 문자열.</param>
    public string Substitute(string source)
    {
        var result = new System.Text.StringBuilder();
        SubstituteInto(result, CutNul(source), 0);
        return result.ToString();
    }

    /// <summary>원본 00440b60: 버퍼가 없거나 입력이 비면 아무것도 쓰지 않는다.</summary>
    /// <param name="dest">이어 쓸 곳.</param>
    /// <param name="source">원본 문자열.</param>
    /// <param name="depth">치환 재귀 깊이.</param>
    private void SubstituteInto(System.Text.StringBuilder dest, string source, int depth)
    {
        int start = dest.Length;
        if (_text == null || source.Length == 0)
        {
            return;
        }
        int pos = 0;
        // 입력 끝까지 글자씩 옮기며 이스케이프와 괄호를 푼다
        while (pos < source.Length)
        {
            char c = source[pos];
            if (c == ConfigTextOps.Escape)
            {
                pos++;
                if (pos < source.Length)
                {
                    // 다음 토큰의 이스케이프는 버린다
                    char escaped = source[pos++];
                    dest.Append(escaped == 'n' ? '\n' : escaped == 'r' ? '\r' : escaped == 't' ? '\t' : escaped);
                }
            }
            else if (c == ConfigTextOps.OpenBrace)
            {
                ExpandBrace(dest, source, ref pos, depth);
            }
            else
            {
                dest.Append(c);
                pos++;
            }
        }
        int end = dest.Length;
        // 끝 공백·탭을 지운다. 호출 전 글이 한 글자만 있을 때까지의 공백·탭은 지운다
        while (end > start + 1 && IsBlank(dest[end - 1]))
        {
            end--;
        }
        dest.Length = end;
    }

    /// <summary>원본 00440a00: 여는 중괄호·기본값·닫는 중괄호 한 쌍을 펼친다. 길이 한도는 원본 pinned 버퍼 크기다.</summary>
    /// <param name="dest">이어 쓸 곳.</param>
    /// <param name="source">원본 문자열.</param>
    /// <param name="pos">여는 중괄호 위치. 닫는 중괄호(또는 길이로 멈춘 글자) 뒤로 간다.</param>
    /// <param name="depth">치환 재귀 깊이.</param>
    private void ExpandBrace(System.Text.StringBuilder dest, string source, ref int pos, int depth)
    {
        pos++;
        var key = new System.Text.StringBuilder();
        // 닫는 중괄호·'|'까지. 길이 한도는 원본 pinned 버퍼 크기다
        while (pos < source.Length && source[pos] != ConfigTextOps.CloseBrace && source[pos] != ConfigTextOps.DefaultSeparator
            && key.Length < LongestSymbol)
        {
            if (source[pos] == ConfigTextOps.OpenBrace)
            {
                ExpandBrace(key, source, ref pos, depth);
            }
            else
            {
                key.Append(ToUpperAscii(source[pos++]));
            }
        }
        var fallback = new System.Text.StringBuilder();
        if (pos < source.Length && source[pos] == ConfigTextOps.DefaultSeparator)
        {
            pos++;
            // 기본값은 닫는 중괄호까지. '|' 자체는 보통 글자다
            while (pos < source.Length && source[pos] != ConfigTextOps.CloseBrace && fallback.Length < LongestSymbol)
            {
                if (source[pos] == ConfigTextOps.OpenBrace)
                {
                    ExpandBrace(fallback, source, ref pos, depth);
                }
                else
                {
                    fallback.Append(source[pos++]);
                }
            }
        }
        Lookup(key.ToString(), fallback.ToString(), dest, true, depth + 1);
        if (pos < source.Length)
        {
            pos++;
        }
    }

    /// <summary>원본 00440760. 반환값은 "찾았는지"이며, 기본값으로 채운 경우는 거짓이다.</summary>
    /// <param name="key">찾을 키(호출자가 NUL 앞에서 자른다).</param>
    /// <param name="fallback">찾지 못했을 때 치환할 기본값.</param>
    /// <param name="dest">이어 쓸 곳.</param>
    /// <param name="reportMissing">찾지 못했을 때 `{키}` 표시를 남길지.</param>
    /// <param name="depth">치환 재귀 깊이.</param>
    private bool Lookup(string key, string fallback, System.Text.StringBuilder dest, bool reportMissing, int depth)
    {
        bool found = false;
        string raw = "";
        if (depth > MaxSubstitutionDepth)
        {
            // 무한 재귀 보호: 여기서 멈추고 아래 "찾지 못함" 처리로 간다
        }
        else if (key.Length != 0 && key[0] == '&')
        {
            reportMissing = false;
            string? value = _registry.RegistryValue != null ? _registry.RegistryValue(key.Substring(1)) : null;
            if (value != null)
            {
                // 값의 괄호가 다시 치환되지 않게 괄호마다 이스케이프를 붙여 그대로 둔다(원본 00440090)
                foreach (char c in value)
                {
                    if (c == ConfigTextOps.OpenBrace || c == ConfigTextOps.CloseBrace)
                    {
                        dest.Append(ConfigTextOps.Escape);
                    }
                    dest.Append(c);
                }
                return true;
            }
        }
        else if (key.Length != 0 && key[0] == '@')
        {
            int dot = key.IndexOf(ConfigTextOps.ObjectSeparator, 1);
            if (dot < 0)
            {
                return true;
            }
            string mission = key.Substring(1, dot - 1);
            string missionKey = key.Substring(dot + 1);
            key = key.Substring(0, dot);
            {
                // 임시 객체는 미션 이름 접두부로 영역 목록에 다시 올라간다
                using var temporary = new Config(_registry, mission);
                if (_registry.LoadMission != null)
                {
                    _registry.LoadMission(temporary, mission);
                }
                string? value = temporary.GetRaw(missionKey);
                if (value != null)
                {
                    raw = value;
                    found = true;
                }
            }
        }
        else
        {
            if (key.Length != 0 && key[0] == '_' && EnvironmentEnabled && _registry.Environment != null)
            {
                string? value = _registry.Environment(key.Substring(1));
                if (value != null)
                {
                    raw = value;
                    found = true;
                }
            }
            IReadOnlyList<Config> objects = _registry.Objects;
            // 마지막에 등록한 객체부터, 이름 있는 객체는 접두부가 맞는 것만 받고 접두부가 없으면 나머지를 찾는다
            for (int i = objects.Count - 1; i >= 0 && !found; i--)
            {
                Config current = objects[i];
                string? value;
                if (current._prefix.Length == 0)
                {
                    value = current.GetRaw(key);
                }
                else if (StartsWithFolded(key, current._prefix))
                {
                    value = current.GetRaw(key.Substring(current._prefix.Length));
                }
                else
                {
                    continue;
                }
                if (value != null)
                {
                    raw = value;
                    found = true;
                }
            }
        }
        if (found)
        {
            SubstituteInto(dest, raw, depth);
            return true;
        }
        if (fallback.Length != 0)
        {
            SubstituteInto(dest, fallback, depth);
        }
        else if (reportMissing)
        {
            dest.Append(ConfigTextOps.OpenBrace);
            dest.Append(key);
            dest.Append(ConfigTextOps.CloseBrace);
        }
        return false;
    }

    /// <summary>원본 00440240: 찾을 때마다 그 줄(줄 시작부터 다음 줄 첫 글자까지)을 지운다.</summary>
    /// <param name="key">찾을 키.</param>
    public bool RemoveKey(string key)
    {
        if (_text == null)
        {
            return false;
        }
        bool removed = false;
        // 같은 키가 여러 줄이면 모두 지운다
        int? found;
        while ((found = ConfigText.FindKeyIn(_text, key)) != null)
        {
            _text = _text.Remove(found.Value, NextLine(_text, found.Value) - found.Value);
            removed = true;
        }
        return removed;
    }

    /// <summary>원본 004404e0. '.'이 있으면 뒤에 '.' 하나로 접두부가 맞는 마지막 객체를 고르고, 앞은 그 '.' 뒤로 바꾼다.</summary>
    /// <param name="key">쓸 키.</param>
    /// <param name="value">쓸 값.</param>
    public bool Set(string key, string value)
    {
        key = CutNul(key);
        Config target = this;
        int dot = key.IndexOf(ConfigTextOps.ObjectSeparator);
        if (dot >= 0)
        {
            IReadOnlyList<Config> objects = _registry.Objects;
            // 마지막에 등록한 객체부터 찾는다. 맞는 객체가 없으면 이 객체에 전역으로 둔다
            for (int i = objects.Count - 1; i >= 0; i--)
            {
                if (StartsWithFolded(key, objects[i]._prefix))
                {
                    target = objects[i];
                    key = key.Substring(dot + 1);
                    break;
                }
            }
        }
        bool existed = target.RemoveKey(key);
        // 원본 서식 "%s = \"%s\""(004402f0). 원본 256바이트 서식 버퍼 한도는 두지 않았다
        target.Append(key + " = \"" + CutNul(value) + "\"");
        target._changed = true;
        return existed;
    }

    /// <summary>원본 0043fe60: 줄의 첫 글자가 '[' 줄에서만, 그 줄의 모든 '[' 위치를 header와 비교한다.</summary>
    /// <param name="text">설정 버퍼.</param>
    /// <param name="from">찾기 시작 위치.</param>
    /// <param name="header">섹션 머리.</param>
    /// <param name="matchedName">'[' 다음부터 ']' 전까지. 없으면 null.</param>
    public static int? FindSection(string text, int from, string header, out string? matchedName) =>
        FindSectionCore(text, from, header, true, out matchedName);

    /// <summary>이름이 필요 없을 때 쓰는 형태(원본의 matchedName=nullptr 호출). 위치 계산만 한다.</summary>
    /// <param name="text">설정 버퍼.</param>
    /// <param name="from">찾기 시작 위치.</param>
    /// <param name="header">섹션 머리.</param>
    public static int? FindSection(string text, int from, string header) =>
        FindSectionCore(text, from, header, false, out _);    /// <summary>이름이 필요 없을 때는 ']' 탐색을 건너뛴다(원본의 nullptr 분기). 위치 계산만 한다.</summary>
    private static int? FindSectionCore(string text, int from, string header, bool wantName, out string? matchedName)
    {
        text = CutNul(text);
        matchedName = null;
        int pos = from;
        // 줄 단위로 찾는다
        while (pos < text.Length)
        {
            // 앞의 공백·탭을 건너뛴다
            while (pos < text.Length && IsBlank(text[pos]))
            {
                pos++;
            }
            int lineStart = pos;
            bool matched = false;
            if (pos < text.Length && text[pos] == ConfigTextOps.OpenSection)
            {
                // 이 줄의 '['마다 비교한다
                while (true)
                {
                    if (StartsWithFolded(text.Substring(pos), header))
                    {
                        matched = true;
                        break;
                    }
                    pos++;
                    // 같은 줄의 다음 '['까지 간다
                    while (pos < text.Length && text[pos] != '\n' && text[pos] != ConfigTextOps.OpenSection)
                    {
                        pos++;
                    }
                    if (pos >= text.Length || text[pos] == '\n')
                    {
                        break;
                    }
                }
            }
            if (!matched)
            {
                pos = NextLine(text, lineStart);
                continue;
            }
            if (wantName)
            {
                // '[' 다음부터 ']' 전까지. ']'가 없으면 버퍼 끝까지다
                int close = text.IndexOf(ConfigTextOps.CloseSection, pos + 1);
                if (close < 0)
                {
                    close = text.Length;
                }
                matchedName = text.Substring(pos + 1, close - pos - 1);
                pos = close;
            }
            int newline = text.IndexOf('\n', pos);
            if (newline < 0)
            {
                return null;
            }
            pos = newline + 1;
            if (pos < text.Length && text[pos] == '\r')
            {
                pos++;
            }
            // 본문이 비어 버퍼 끝이면 원본 호출들은 찾지 못한 것으로 다룬다
            if (pos >= text.Length)
            {
                return null;
            }
            return pos;
        }
        return null;
    }

    /// <summary>원본 00440570: `[이름]` 섹션 줄 다음부터 다음 섹션 전까지의 본문.</summary>
    /// <param name="name">섹션 이름. '['로 시작하면 그대로 쓴다.</param>
    public string? Section(string name)
    {
        if (_text == null)
        {
            return null;
        }
        string header = name.Length != 0 && name[0] == ConfigTextOps.OpenSection
            ? name
            : ConfigTextOps.OpenSection + name + ConfigTextOps.CloseSection;
        string text = _text;
        int? start = FindSectionCore(text, 0, header, false, out _);
        if (start == null)
        {
            return null;
        }
        int end = start.Value;
        // 줄의 첫 글자(LF·CR 뒤)가 '[' 곳이 다음 섹션이다
        while (end < text.Length && !(text[end] == ConfigTextOps.OpenSection && (text[end - 1] == '\n' || text[end - 1] == '\r')))
        {
            end++;
        }
        return text.Substring(start.Value, end - start.Value);
    }

    /// <summary>원본 00440640: `[접두부`로 찾은 섹션마다 이름을 ';'로 이어 붙인다.</summary>
    /// <param name="prefix">섹션 이름 접두부.</param>
    public string SectionNames(string prefix)
    {
        string result = "";
        if (_text == null)
        {
            return result;
        }
        string header = "" + ConfigTextOps.OpenSection + prefix;
        int pos = 0;
        // 찾은 줄의 다음 위치에서 이어서 찾는다
        int? next;
        while ((next = FindSection(_text, pos, header, out string? name)) != null)
        {
            result += name;
            result += ';';
            pos = next.Value;
        }
        return result;
    }

    /// <summary>접두부("이름." 꼴) 또는 빈 문자열을 그대로 돌려준다.</summary>
    public string Prefix => _prefix;

    /// <summary>버퍼가 없으면 빈 문자열이다.</summary>
    public string Text => _text ?? "";

    /// <summary>원본 +8: 참이면 `{_이름}`을 환경 변수에서 찾는다.</summary>
    public bool EnvironmentEnabled { get; set; }

    /// <summary>원본 +0x14: Set이 켠다. 저장 판단은 호출자의 몫이다.</summary>
    public bool Changed
    {
        get => _changed;
        set => _changed = value;
    }

    /// <summary>변경 표시 저장소.</summary>
    private bool _changed;

    /// <summary>C 문자열처럼 NUL 앞에서 자른다.</summary>
    private static string CutNul(string text)
    {
        int nul = text.IndexOf('\0');
        return nul < 0 ? text : text.Substring(0, nul);
    }

    /// <summary>공백·탭인지 확인한다.</summary>
    private static bool IsBlank(char c) => c == ' ' || c == '\t';

    /// <summary>줄 경계 공백인지 확인한다.</summary>
    private static bool IsLineGap(char c) => c == '\n' || c == '\r' || c == ' ' || c == '\t';

    /// <summary>다음 줄 첫 글자 위치를 구한다. LF가 없으면 끝이다.</summary>
    private static int NextLine(string text, int pos)
    {
        int newline = text.IndexOf('\n', pos);
        if (newline < 0)
        {
            return text.Length;
        }
        pos = newline;
        // 빈 줄과 다음 줄의 여백까지 함께 건너뛴다
        while (pos < text.Length && IsLineGap(text[pos]))
        {
            pos++;
        }
        return pos;
    }

    /// <summary>앞부분 대소문자 무시 비교. 짧으면 거짓이다.</summary>
    private static bool StartsWithFolded(string left, string right)
    {
        if (right.Length > left.Length)
        {
            return false;
        }
        // 양쪽을 같은 소문자로 견주어 비교한다
        for (int i = 0; i < right.Length; i++)
        {
            if (ToLowerAscii(left[i]) != ToLowerAscii(right[i]))
            {
                return false;
            }
        }
        return true;
    }

    /// <summary>ASCII 대문자를 소문자로 바꾼다.</summary>
    private static char ToLowerAscii(char c) => c >= 'A' && c <= 'Z' ? (char)(c + ('a' - 'A')) : c;

    /// <summary>ASCII 소문자를 대문자로 바꾼다.</summary>
    private static char ToUpperAscii(char c) => c >= 'a' && c <= 'z' ? (char)(c - ('a' - 'A')) : c;
}
