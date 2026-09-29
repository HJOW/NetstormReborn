using Netstorm.Assets;

namespace Netstorm.Core.Rules;

/// <summary>
/// 유닛 하나를 건설·생산할 때 그 위치에 필요한 원소 에너지.
/// 원본은 타입 구조체 +0xA0 의 문자열(예: "tts")로 보관한다 (docs/exe/energy-requirements.md).
/// 필요 에너지 1개마다 서로 다른 공급원 1개가 필요하며, Sun 은 아무 원소 공급원으로 채운다.
/// </summary>
public sealed class EnergyRequirement
{
    /// <summary>원본이 요구 문자열을 저장하는 버퍼 한도 (0xfd 에서 자름). 이보다 긴 문자열은 잘라 쓴다.</summary>
    private const int MaxLetters = 0xFD;

    /// <summary>필요 에너지가 없는 요구값 (신전·워크샵·알타·사제·기지가 만드는 비행체 등)</summary>
    public static EnergyRequirement None { get; } = new("");

    /// <summary>원본 요구 문자열 (w·r·t·s 로만 구성, 소문자)</summary>
    public string Letters { get; }

    /// <summary>필요한 에너지 개수 (= 서로 다른 공급원 개수)</summary>
    public int Count => Letters.Length;

    /// <summary>원소별 필요 개수. Sun 은 "아무 원소" 개수다.</summary>
    /// <param name="element">원소</param>
    public int CountOf(Element element) => Letters.Count(c => c == Elements.ToLetter(element));

    /// <summary>문자열로 만든다. 알 수 없는 글자는 버린다.</summary>
    /// <param name="letters">요구 문자열</param>
    private EnergyRequirement(string letters)
    {
        Letters = letters;
    }

    /// <summary>
    /// 원본 요구 문자열을 해석한다 (.type 의 mana 속성과 같은 형식).
    /// 글자 순서는 유지하고, w·r·t·s 가 아닌 글자는 무시한다.
    /// </summary>
    /// <param name="letters">예: "s", "ts", "rrs"</param>
    public static EnergyRequirement Parse(string? letters)
    {
        if (string.IsNullOrWhiteSpace(letters))
        {
            return None;
        }
        // 원소 글자만 소문자로 남긴다 (원본 문자 표의 대문자도 같은 원소로 본다).
        string clean = new(letters.Where(c => Elements.FromLetter(c) != null).Select(char.ToLowerInvariant).ToArray());
        return clean.Length == 0 ? None : new EnergyRequirement(clean.Length > MaxLetters ? clean[..MaxLetters] : clean);
    }

    /// <summary>
    /// 원본 FUN_0049b0d0 의 기본 요구값: 원소 유닛 L1 = 원소 1, L2 = 원소 1 + Sun 1, L3 = 원소 2 + Sun 1,
    /// Sun 유닛 = Sun × 레벨. 레벨이 없거나 1~3 이 아니면 요구값이 없다.
    /// </summary>
    /// <param name="element">유닛 원소 (theme)</param>
    /// <param name="level">.type 의 level</param>
    public static EnergyRequirement Default(Element element, int level)
    {
        if (level is < 1 or > 3)
        {
            return None;
        }
        if (element == Element.Sun)
        {
            return new EnergyRequirement(new string('s', level));
        }
        char own = Elements.ToLetter(element);
        return level switch
        {
            1 => new EnergyRequirement($"{own}"),
            2 => new EnergyRequirement($"{own}s"),
            _ => new EnergyRequirement($"{own}{own}s"),
        };
    }

    /// <summary>
    /// 타입의 실제 요구값: .type 에 mana 가 있으면 그 문자열(Generator 3종·Outpost 의 "s"),
    /// 없으면 theme·level 로 만든 기본값. 원본 타입 로더가 mana 를 먼저 복사하고 비었을 때만 기본값을 만드는 순서와 같다.
    /// </summary>
    /// <param name="definition">.type 정의</param>
    public static EnergyRequirement ForType(TypeDefinition definition)
    {
        string? mana = definition.GetString("mana");
        if (!string.IsNullOrWhiteSpace(mana))
        {
            return Parse(mana);
        }
        Element? element = Elements.FromTheme(definition.GetString("theme"));
        int? level = definition.GetInt("level");
        return element == null || level == null ? None : Default(element.Value, level.Value);
    }

    /// <summary>사람이 읽는 설명 (예: "Thunder 2 + 아무 1"). 요구값이 없으면 "없음".</summary>
    public string Describe()
    {
        if (Count == 0)
        {
            return "없음";
        }
        var parts = new List<string>();
        // 원소 순서대로 개수를 적고 Sun 은 "아무"로 표시한다.
        foreach (Element element in new[] { Element.Rain, Element.Wind, Element.Thunder, Element.Sun })
        {
            int n = CountOf(element);
            if (n > 0)
            {
                parts.Add(element == Element.Sun ? $"아무 {n}" : $"{element} {n}");
            }
        }
        return string.Join(" + ", parts);
    }

    /// <inheritdoc />
    public override string ToString() => Letters;
}
