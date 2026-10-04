using Netstorm.Assets;

namespace Netstorm.Core.Rules;

/// <summary>지식 창(View Netstorm Knowledge) 카드 하나.</summary>
/// <param name="Type">유닛 타입</param>
/// <param name="Element">원소 (행)</param>
/// <param name="Group">.type group 값 (행 안 순서)</param>
/// <param name="Title">카드 이름 (.type description, 없으면 타입 이름)</param>
public sealed record KnowledgeCard(TypeInfo Type, Element Element, string Group, string Title);

/// <summary>지식 창의 원소 행 하나. 카드가 없어도 행은 있다.</summary>
/// <param name="Element">원소</param>
/// <param name="Cards">행 안 카드 (왼쪽부터)</param>
public sealed record KnowledgeRow(Element Element, IReadOnlyList<KnowledgeCard> Cards);

/// <summary>
/// 지식 창의 카드 격자 규칙 (docs/exe/show-technology.md, docs/videos/the-war-begins-record-play-20260930.md 3절 6번).
/// <list type="bullet">
/// <item>행은 SUN · WIND · RAIN · THUN. 순서다 (원본 화면).</item>
/// <item>카드는 플레이어가 아는 타입 중 원소가 있는 생산 유닛(Generator·건물형·수송)이다. 템플이 공급하는 골렘(sunWalker)은 뺀다
/// (The War Begins! 의 .fort Technology 26개 중 sunWalker 만 격자에 없다 — exe 는 플래그 조건으로 거르며 정확한 비트 뜻은 미확인).</item>
/// <item>행 안 순서는 .type group 순서 battery → cannon → archer → walker → blocker → fence → aviary → balloon (원본 화면 25장과 일치).
/// 같은 group 이 둘 이상이면 타입 로딩 순서를 따른다(원본 예시 없음, 클론 규칙).</item>
/// </list>
/// </summary>
public static class KnowledgeCatalog
{
    /// <summary>원본 화면의 행 순서 (Element 열거 순서와 다르다)</summary>
    public static readonly IReadOnlyList<Element> RowOrder = [Element.Sun, Element.Wind, Element.Rain, Element.Thunder];

    /// <summary>원본 화면의 행 안 group 순서</summary>
    public static readonly IReadOnlyList<string> GroupOrder = ["battery", "cannon", "archer", "walker", "blocker", "fence", "aviary", "balloon"];

    /// <summary>템플이 공급하는 골렘 타입 이름 (지식 창에 나오지 않는다)</summary>
    public const string GolemTypeName = "sunWalker";

    /// <summary>.fort Technology 목록 플래그 중 "지식 있음" 비트 (exe 0x4acac0(플레이어, 4, 타입) 과 같은 값)</summary>
    public const byte KnownListFlag = 4;

    /// <summary>
    /// .fort Technology 섹션에서 지식 플래그가 켜진 타입 이름을 저장 순서대로 뽑는다.
    /// 캠페인 미션 맵은 이 목록이 지식 창 내용이다(생산 등록 가능 목록은 미션 myTech 로 따로 정해진다).
    /// </summary>
    /// <param name="fort">미션·요새 맵</param>
    public static IReadOnlyList<string> KnownFromFort(FortFile fort) =>
        [.. fort.Technology.Where(t => (t.ListFlags & KnownListFlag) != 0).Select(t => t.Type.Name)];

    /// <summary>아는 타입 이름으로 원소별 카드 행 네 개를 만든다.</summary>
    /// <param name="types">타입 목록</param>
    /// <param name="known">아는 타입 이름 (대소문자 무시, 중복 허용)</param>
    public static IReadOnlyList<KnowledgeRow> Rows(TypeCatalog types, IEnumerable<string> known)
    {
        var names = new HashSet<string>(known, StringComparer.OrdinalIgnoreCase);
        var cards = new List<KnowledgeCard>();
        // 로딩 순서대로 타입을 훑어 조건에 맞는 카드를 만든다
        foreach (TypeInfo type in types.Types)
        {
            if (!names.Contains(type.Name) || type.Name.Equals(GolemTypeName, StringComparison.OrdinalIgnoreCase)
                || ProducibleUnit.FromType(type) is not { } unit)
            {
                continue;
            }
            string group = type.Definition.GetString("group") ?? "";
            cards.Add(new KnowledgeCard(type, unit.Element, group, type.Definition.GetString("description") ?? type.Name));
        }
        return [.. RowOrder.Select(element => new KnowledgeRow(element,
            [.. cards.Where(c => c.Element == element).OrderBy(c => GroupRank(c.Group))
                .ThenBy(c => c.Type.LoadIndex)]))];
    }

    /// <summary>group 의 행 안 순위 (모르는 group 은 맨 뒤)</summary>
    /// <param name="group">.type group 값</param>
    public static int GroupRank(string group)
    {
        // 표에서 대소문자 무시로 찾는다
        for (int i = 0; i < GroupOrder.Count; i++)
        {
            if (GroupOrder[i].Equals(group, StringComparison.OrdinalIgnoreCase))
            {
                return i;
            }
        }
        return GroupOrder.Count;
    }
}
