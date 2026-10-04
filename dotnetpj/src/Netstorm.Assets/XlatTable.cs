namespace Netstorm.Assets;

/// <summary>
/// UI 문자열 번역표 xlat.&lt;언어&gt;. 영어 원문이 곧 키다. 포맷: docs/formats/xlat.md
/// 원본 Xlat.cpp(FUN_004de260, FUN_004de710)와 같은 규칙으로 읽는다:
/// <list type="bullet">
/// <item>파일 전체에서 CR 을 지운다.</item>
/// <item>'*' 로 시작하는 줄 다음 줄부터 '-' 로 시작하는 줄 앞까지가 원문, 그다음 줄부터 '=' 로 시작하는 줄 앞까지가 번역문.
/// 원문과 번역문 끝의 줄바꿈 하나는 떼어 낸다. 그 밖의 줄(주석, 구분선)은 무시한다.</item>
/// <item>원문 비교는 대소문자를 구분하는 완전 일치. 같은 원문이 다시 나오면 뒤의 번역문이 이긴다.</item>
/// </list>
/// </summary>
public sealed class XlatTable
{
    /// <summary>원문 → 번역문</summary>
    private readonly Dictionary<string, string> _entries;

    /// <summary>고유 원문 수 (같은 원문이 여러 번 나오면 하나로 센다)</summary>
    public int Count => _entries.Count;

    /// <summary>
    /// 파일에서 읽은 번역 블록 수 (중복 포함). 원본 파일은 "NEW TRANSLATIONS" 와 "PRESERVED TRANSLATIONS" 구간에
    /// 같은 원문을 다시 담는 경우가 있어 <see cref="Count"/> 보다 크다 (예: 독일어 871블록 / 고유 774개).
    /// </summary>
    public int BlockCount { get; }

    /// <summary>모든 항목 (원문 → 번역문)</summary>
    public IReadOnlyDictionary<string, string> Entries => _entries;

    /// <summary>항목 사전으로 만든다</summary>
    /// <param name="entries">원문 → 번역문</param>
    /// <param name="blockCount">읽은 블록 수 (중복 포함)</param>
    private XlatTable(Dictionary<string, string> entries, int blockCount)
    {
        _entries = entries;
        BlockCount = blockCount;
    }

    /// <summary>빈 번역표 (영어처럼 번역하지 않는 언어용)</summary>
    public static XlatTable Empty { get; } = new([], 0);

    /// <summary>파일 바이트를 읽는다 (Windows-1252 또는 UTF-8 자동 판별)</summary>
    /// <param name="data">파일 내용</param>
    public static XlatTable FromFileBytes(byte[] data) => Parse(OriginalText.Decode(data));

    /// <summary>번역표 텍스트를 해석한다</summary>
    /// <param name="text">파일 텍스트</param>
    public static XlatTable Parse(string text)
    {
        string[] lines = text.Replace("\r", "", StringComparison.Ordinal).Split('\n');
        var entries = new Dictionary<string, string>(StringComparer.Ordinal);
        // 상태: 0 = '*' 줄 찾는 중, 1 = 원문 모으는 중, 2 = 번역문 모으는 중
        int state = 0;
        int blockStart = 0;
        int blockCount = 0;
        string key = "";
        // 줄마다 첫 글자로 상태를 바꾼다
        for (int i = 0; i < lines.Length; i++)
        {
            string line = lines[i];
            char first = line.Length > 0 ? line[0] : '\0';
            if (state == 0 && first == '*')
            {
                state = 1;
                blockStart = i + 1;
            }
            else if (state == 1 && first == '-')
            {
                key = string.Join('\n', lines, blockStart, i - blockStart);
                state = 2;
                blockStart = i + 1;
            }
            else if (state == 2 && first == '=')
            {
                entries[key] = string.Join('\n', lines, blockStart, i - blockStart);
                blockCount++;
                state = 0;
            }
        }
        return new XlatTable(entries, blockCount);
    }

    /// <summary>원문의 번역문을 찾는다</summary>
    /// <param name="english">영어 원문</param>
    /// <param name="translated">번역문</param>
    public bool TryTranslate(string english, out string translated) =>
        _entries.TryGetValue(english, out translated!);

    /// <summary>원문을 번역한다. 번역이 없으면 원문을 그대로 돌려준다 (원본 FUN_004de9a0)</summary>
    /// <param name="english">영어 원문</param>
    public string Translate(string english) => TryTranslate(english, out string t) ? t : english;
}
