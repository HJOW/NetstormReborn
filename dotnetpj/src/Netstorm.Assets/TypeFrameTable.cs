using System.Text.RegularExpressions;

namespace Netstorm.Assets;

/// <summary>
/// 원본 Rifttype.cpp(0049d0xx 로더)가 클러스터 줄의 플래그 단어를 저장하는 프레임 코드 4번째 바이트 비트.
/// 목록에 없는 단어(dirt, solid 등)는 원본에서도 무시된다.
/// </summary>
[Flags]
public enum FrameCodeFlags : byte
{
    /// <summary>플래그 없음</summary>
    None = 0,
    /// <summary>"fringe" — 절벽 가장자리 프레임</summary>
    Fringe = 0x01,
    /// <summary>"suck" — 흡수(thundercannon) 프레임</summary>
    Suck = 0x02,
    /// <summary>"rim" — 섬 테두리 프레임</summary>
    Rim = 0x04,
    /// <summary>"lit" — 밝은(전투 조명) 프레임</summary>
    Lit = 0x08,
    /// <summary>"unlit" — 어두운 프레임</summary>
    Unlit = 0x10,
    /// <summary>"cracked" — 금 간 다리 프레임</summary>
    Cracked = 0x20,
    /// <summary>"hard" — 단단해진(경화) 다리 프레임</summary>
    Hard = 0x40,
}

/// <summary>
/// 원본이 클러스터마다 만드는 4바이트 프레임 코드.
/// 이름 "N00" → (측면 방향 'N', 변형 'P', 번호 0), "AA01" → ('A', 'A', 1).
/// </summary>
/// <param name="Side">첫 글자 (원본 assert 이름: sideOrientation, 'A'~'Z')</param>
/// <param name="Variant">둘째 글자. 한 글자 이름이면 원본처럼 'P'</param>
/// <param name="Number">글자 뒤의 번호 (원본은 실수로 읽어 _ftol 로 정수화, char 로 저장)</param>
/// <param name="Flags">클러스터 플래그 비트</param>
public readonly record struct FrameCode(char Side, char Variant, int Number, FrameCodeFlags Flags)
{
    /// <summary>개발용 표기 (예: "A P 3 rim")</summary>
    public override string ToString() =>
        Flags == FrameCodeFlags.None ? $"{Side} {Variant} {Number}" : $"{Side} {Variant} {Number} {Flags}";
}

/// <summary>
/// 타입 하나의 프레임 코드 표와 특수 프레임 번호. 원본 타입 구조체(500바이트)의
/// +0x114 프레임 수, +0x118 기본, +0x11c gump, +0x120 도움말, +0x124 코드 표, +0x128 base 에 해당한다.
/// 검색 함수는 원본 0049a940·0049a9a0·0049a9e0·0049aa30 과 같은 순차 검색(첫 일치)이다.
/// </summary>
public sealed partial class TypeFrameTable
{
    /// <summary>원본이 한 글자 클러스터 이름의 둘째 글자로 채우는 값</summary>
    public const char DefaultVariant = 'P';

    /// <summary>원본 플래그 단어 → 비트 (대소문자 무시 비교)</summary>
    private static readonly (string Word, FrameCodeFlags Flag)[] FlagWords =
    [
        ("fringe", FrameCodeFlags.Fringe),
        ("suck", FrameCodeFlags.Suck),
        ("rim", FrameCodeFlags.Rim),
        ("lit", FrameCodeFlags.Lit),
        ("unlit", FrameCodeFlags.Unlit),
        ("cracked", FrameCodeFlags.Cracked),
        ("hard", FrameCodeFlags.Hard),
    ];

    /// <summary>클러스터 순서대로의 프레임 코드</summary>
    public IReadOnlyList<FrameCode> Codes { get; }

    /// <summary>기본 프레임 (+0x118). 원본은 "default" 가 나올 때마다 덮어써 마지막 것이 남는다. 없으면 0</summary>
    public int DefaultFrame { get; }

    /// <summary>gump(요새 화면) 프레임 (+0x11c). 없으면 0</summary>
    public int GumpFrame { get; }

    /// <summary>도움말 프레임 (+0x120). "help" 가 있으면 마지막 help, 없으면 마지막 default, 둘 다 없으면 -1</summary>
    public int HelpFrame { get; }

    /// <summary>base 프레임 (+0x128). 없으면 -1</summary>
    public int BaseFrame { get; }

    /// <summary>클러스터 목록으로 원본 로더와 같은 규칙의 표를 만든다</summary>
    /// <param name="definition">.type 해석 결과</param>
    public TypeFrameTable(TypeDefinition definition)
    {
        var codes = new FrameCode[definition.Clusters.Count];
        int defaultFrame = 0;
        int gumpFrame = 0;
        int helpFrame = -1;
        int baseFrame = -1;
        bool helpSeen = false;
        // 클러스터 줄마다 이름을 코드로 바꾸고 플래그 단어를 순서대로 적용한다
        for (int i = 0; i < codes.Length; i++)
        {
            Cluster cluster = definition.Clusters[i];
            FrameCodeFlags flags = FrameCodeFlags.None;
            // 원본 로더의 단어 비교 순서를 그대로 따른다 (default 다음에 help 를 본다)
            foreach (string word in cluster.Flags)
            {
                flags |= ToFlag(word);
                if (word.Equals("baseframe", StringComparison.OrdinalIgnoreCase))
                {
                    baseFrame = i;
                }
                if (word.Equals("gumpframe", StringComparison.OrdinalIgnoreCase))
                {
                    gumpFrame = i;
                }
                if (word.Equals("default", StringComparison.OrdinalIgnoreCase))
                {
                    defaultFrame = i;
                    if (!helpSeen)
                    {
                        helpFrame = i;
                    }
                }
                if (word.Equals("help", StringComparison.OrdinalIgnoreCase))
                {
                    helpFrame = i;
                    helpSeen = true;
                }
            }
            codes[i] = ParseName(cluster.Name, flags);
        }
        Codes = codes;
        DefaultFrame = defaultFrame;
        GumpFrame = gumpFrame;
        HelpFrame = helpFrame;
        BaseFrame = baseFrame;
    }

    /// <summary>클러스터 이름을 코드로 바꾼다 (글자 1~2개 + 번호)</summary>
    /// <param name="name">클러스터 이름 (예: "AA01")</param>
    /// <param name="flags">플래그 비트</param>
    public static FrameCode ParseName(string name, FrameCodeFlags flags = FrameCodeFlags.None)
    {
        Match m = NameRegex().Match(name);
        if (!m.Success)
        {
            throw new InvalidDataException($"클러스터 이름 형식이 아닙니다: {name}");
        }
        string letters = m.Groups[1].Value;
        return new FrameCode(letters[0], letters.Length > 1 ? letters[1] : DefaultVariant,
            int.Parse(m.Groups[2].Value, System.Globalization.CultureInfo.InvariantCulture), flags);
    }

    /// <summary>플래그 단어 하나를 비트로 바꾼다 (모르는 단어는 0)</summary>
    /// <param name="word">클러스터 플래그 단어</param>
    private static FrameCodeFlags ToFlag(string word)
    {
        // 원본 비교 목록에서 일치하는 단어를 찾는다
        foreach ((string name, FrameCodeFlags flag) in FlagWords)
        {
            if (word.Equals(name, StringComparison.OrdinalIgnoreCase))
            {
                return flag;
            }
        }
        return FrameCodeFlags.None;
    }

    /// <summary>(측면, 변형, 번호)가 같은 첫 프레임 (원본 0049a9a0). 없으면 -1</summary>
    public int Find(char side, char variant, int number) => FindFirst(c =>
        c.Side == side && c.Variant == variant && c.Number == number);

    /// <summary>(측면, 변형, 번호)가 같고 플래그가 mask 와 겹치는 첫 프레임 (원본 0049a9e0). 없으면 -1</summary>
    public int Find(char side, char variant, int number, FrameCodeFlags mask) => FindFirst(c =>
        c.Side == side && c.Variant == variant && c.Number == number && (c.Flags & mask) != 0);

    /// <summary>(측면, 변형)이 같고 플래그가 정확히 같은 첫 프레임 (원본 0049aa30). 없으면 -1</summary>
    public int FindExactFlags(char side, char variant, FrameCodeFlags flags) => FindFirst(c =>
        c.Side == side && c.Variant == variant && c.Flags == flags);

    /// <summary>
    /// 측면·변형이 같은 프레임을 파일 순서로 모은다. 원본 0049aa90 의 1차 후보 목록과 같으며
    /// 개발용 뷰어는 이 목록을 한 동작의 애니메이션 순서로 재생한다.
    /// </summary>
    public IReadOnlyList<int> Sequence(char side, char variant)
    {
        var frames = new List<int>();
        // 코드 표 전체에서 같은 측면·변형을 찾는다
        for (int i = 0; i < Codes.Count; i++)
        {
            if (Codes[i].Side == side && Codes[i].Variant == variant)
            {
                frames.Add(i);
            }
        }
        return frames;
    }

    /// <summary>지정 프레임이 속한 동작(측면·변형)의 프레임 목록</summary>
    /// <param name="frame">클러스터 번호</param>
    public IReadOnlyList<int> SequenceOf(int frame) => Sequence(Codes[frame].Side, Codes[frame].Variant);

    /// <summary>조건에 맞는 첫 프레임 번호 (없으면 -1)</summary>
    private int FindFirst(Func<FrameCode, bool> match)
    {
        // 원본처럼 앞에서부터 순차 검색한다
        for (int i = 0; i < Codes.Count; i++)
        {
            if (match(Codes[i]))
            {
                return i;
            }
        }
        return -1;
    }

    /// <summary>클러스터 이름: 영문자 1~2개 + 번호</summary>
    [GeneratedRegex(@"^([A-Za-z]{1,2})(\d+)$")]
    private static partial Regex NameRegex();
}
