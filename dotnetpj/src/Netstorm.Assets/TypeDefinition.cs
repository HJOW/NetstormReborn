using System.Globalization;
using System.Text;
using System.Text.RegularExpressions;

namespace Netstorm.Assets;

/// <summary>클러스터 줄의 그림 참조 하나 ("그림.gif" #번호). 런타임에는 쓰이지 않는 제작 시점 정보</summary>
/// <param name="Image">원본 그림 파일 이름</param>
/// <param name="Frame">그림 안의 프레임 번호</param>
public sealed record ClusterImageRef(string Image, int Frame);

/// <summary>클러스터(애니메이션 프레임 정의) 하나</summary>
/// <param name="Name">클러스터 이름 (예: "N00", "AA01")</param>
/// <param name="Flags">클러스터 플래그 (예: "default", "rim fringe")</param>
/// <param name="Layers">레이어별 그림 참조 (0 = 본체, 1 = 그림자)</param>
public sealed record Cluster(string Name, IReadOnlyList<string> Flags, IReadOnlyList<ClusterImageRef> Layers);

/// <summary>
/// .type 오브젝트 정의 스크립트 해석 결과. 포맷: docs/formats/type.md
/// </summary>
public sealed partial class TypeDefinition
{
    /// <summary>원본 텍스트 인코딩 (Windows-1252 범위)</summary>
    private static readonly Encoding TextEncoding = Encoding.Latin1;

    /// <summary>타입 이름 (typename)</summary>
    public string Name { get; }

    /// <summary>typename 뒤의 수식어 ("constructor", "client constructor" 또는 빈 문자열)</summary>
    public string Modifier { get; }

    /// <summary>typeflags 목록</summary>
    public IReadOnlyList<string> Flags { get; }

    /// <summary>{ } 안의 속성 (키는 대소문자 무시, 값은 따옴표를 뗀 원문)</summary>
    public IReadOnlyDictionary<string, string> Properties { get; }

    /// <summary>속성이 나온 순서. 중복과 별칭의 적용 순서를 원본 0049c3b0대로 보존한다.</summary>
    public IReadOnlyList<KeyValuePair<string, string>> PropertySequence { get; }

    /// <summary>클러스터 목록 (파일에 나온 순서 = 셰이프 블록 안 순번)</summary>
    public IReadOnlyList<Cluster> Clusters { get; }

    /// <summary>원본 로더 규칙의 프레임 코드 표 (처음 사용할 때 만든다)</summary>
    public TypeFrameTable Frames => _frames ??= new TypeFrameTable(this);

    /// <summary>Frames 캐시</summary>
    private TypeFrameTable? _frames;

    /// <summary>클러스터 줄에 나오는 최대 레이어 수</summary>
    public int LayerCount => Clusters.Count == 0 ? 0 : Clusters.Max(c => c.Layers.Count);

    /// <summary>해석 결과로 만든다</summary>
    private TypeDefinition(string name, string modifier, IReadOnlyList<string> flags,
        IReadOnlyDictionary<string, string> properties, IReadOnlyList<KeyValuePair<string, string>> propertySequence, IReadOnlyList<Cluster> clusters)
    {
        Name = name;
        Modifier = modifier;
        Flags = flags;
        Properties = properties;
        PropertySequence = propertySequence;
        Clusters = clusters;
    }

    /// <summary>플래그 포함 여부 (대소문자 무시)</summary>
    /// <param name="flag">플래그 이름</param>
    public bool HasFlag(string flag) => Flags.Contains(flag, StringComparer.OrdinalIgnoreCase);

    /// <summary>속성을 정수로 읽는다 (없거나 정수가 아니면 null)</summary>
    /// <param name="key">속성 키 (대소문자 무시)</param>
    public int? GetInt(string key) =>
        Properties.TryGetValue(key, out string? v) && int.TryParse(v, NumberStyles.Integer, CultureInfo.InvariantCulture, out int n)
            ? n : null;

    /// <summary>속성을 실수로 읽는다 (없거나 숫자가 아니면 null)</summary>
    /// <param name="key">속성 키 (대소문자 무시)</param>
    public double? GetDouble(string key) =>
        Properties.TryGetValue(key, out string? v) && double.TryParse(v, NumberStyles.Float, CultureInfo.InvariantCulture, out double d)
            ? d : null;

    /// <summary>속성 문자열 (없으면 null)</summary>
    /// <param name="key">속성 키 (대소문자 무시)</param>
    public string? GetString(string key) => Properties.TryGetValue(key, out string? v) ? v : null;

    /// <summary>원본 바이트(Windows-1252 텍스트)를 해석한다</summary>
    /// <param name="data">.type 파일 내용</param>
    public static TypeDefinition Parse(byte[] data) => Parse(TextEncoding.GetString(data));

    /// <summary>.type 텍스트를 해석한다</summary>
    /// <param name="text">.type 파일 텍스트</param>
    public static TypeDefinition Parse(string text)
    {
        string clean = CommentRegex().Replace(text, "");

        Match head = TypeNameRegex().Match(clean);
        string name = head.Success ? head.Groups[1].Value : "";
        string modifier = head.Success ? head.Groups[2].Value.Trim() : "";
        // typename 과 같은 줄에 typeflags 가 붙은 파일이 있으므로 수식어에서 제외
        if (modifier.StartsWith("typeflags", StringComparison.OrdinalIgnoreCase))
        {
            modifier = "";
        }

        Match flagMatch = TypeFlagsRegex().Match(clean);
        string[] flags = flagMatch.Success
            ? flagMatch.Groups[1].Value.Split((char[]?)null, StringSplitOptions.RemoveEmptyEntries)
            : [];

        var properties = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        var propertySequence = new List<KeyValuePair<string, string>>();
        Match body = BodyRegex().Match(clean);
        if (body.Success)
        {
            // { } 안의 "키 = 값;" 을 차례로 읽는다 (같은 키가 반복되면 마지막 값)
            foreach (Match p in PropertyRegex().Matches(body.Groups[1].Value))
            {
                string key = p.Groups[1].Value, value = Unquote(p.Groups[2].Value.Trim());
                properties[key] = value;
                propertySequence.Add(new KeyValuePair<string, string>(key, value));
            }
        }

        return new TypeDefinition(name, modifier, flags, properties, propertySequence, ParseClusters(text));
    }

    /// <summary>클러스터 줄을 순서대로 읽는다 (줄 단위로 주석을 떼고 검사)</summary>
    /// <param name="text">.type 원문</param>
    private static List<Cluster> ParseClusters(string text)
    {
        var clusters = new List<Cluster>();
        // 줄마다 클러스터 정의인지 검사
        foreach (string rawLine in text.Split('\n'))
        {
            int comment = rawLine.IndexOf("//", StringComparison.Ordinal);
            string line = comment >= 0 ? rawLine[..comment] : rawLine;
            Match m = ClusterLineRegex().Match(line);
            if (!m.Success)
            {
                continue;
            }
            string[] flags = m.Groups[2].Value.Split((char[]?)null, StringSplitOptions.RemoveEmptyEntries);
            var layers = new List<ClusterImageRef>();
            // 그림 참조("파일" #번호)를 레이어 순서대로 모은다
            foreach (Match r in ImageRefRegex().Matches(m.Groups[3].Value))
            {
                layers.Add(new ClusterImageRef(r.Groups[1].Value, int.Parse(r.Groups[2].Value, CultureInfo.InvariantCulture)));
            }
            clusters.Add(new Cluster(m.Groups[1].Value, flags, layers));
        }
        return clusters;
    }

    /// <summary>앞뒤 큰따옴표를 뗀다</summary>
    /// <param name="value">원문 값</param>
    private static string Unquote(string value) =>
        value.Length >= 2 && value[0] == '"' && value[^1] == '"' ? value[1..^1] : value;

    /// <summary>줄 주석 (// 부터 줄 끝까지)</summary>
    [GeneratedRegex(@"//[^\n]*")]
    private static partial Regex CommentRegex();

    /// <summary>머리 줄: typename 이름 [수식어]</summary>
    [GeneratedRegex(@"typename\s+(\S+)([^\n]*)")]
    private static partial Regex TypeNameRegex();

    /// <summary>플래그 줄: typeflags ... ;</summary>
    [GeneratedRegex(@"typeflags\s+([^;]*);")]
    private static partial Regex TypeFlagsRegex();

    /// <summary>속성 블록 { ... }</summary>
    [GeneratedRegex(@"\{(.*?)\}", RegexOptions.Singleline)]
    private static partial Regex BodyRegex();

    /// <summary>속성 하나: 키 = 값 ;</summary>
    [GeneratedRegex(@"(\w+)\s*=\s*([^;]*);")]
    private static partial Regex PropertyRegex();

    /// <summary>클러스터 줄: 이름 : 플래그 : 그림 참조들</summary>
    [GeneratedRegex(@"^\s*([A-Za-z]+\d+)\s*:([^:]*):(.*)$")]
    private static partial Regex ClusterLineRegex();

    /// <summary>그림 참조: "파일" #번호</summary>
    [GeneratedRegex("\"([^\"]+)\"\\s*#\\s*(\\d+)")]
    private static partial Regex ImageRefRegex();
}
