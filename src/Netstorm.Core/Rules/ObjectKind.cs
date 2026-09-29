using Netstorm.Assets;

namespace Netstorm.Core.Rules;

/// <summary>
/// 게임 규칙에서 쓰는 오브젝트 분류 (docs/gameplay/island-ownership.md "용어", docs/sources/game-manual.md 1절).
/// 템플·워크샵·알타·아웃포스트는 "건물"이며 사제가 짓는다. 그 밖의 작은 건물과 이동체가 "유닛"이다.
/// </summary>
public enum ObjectKind
{
    /// <summary>분류하지 않는 오브젝트 (지형·장식·효과 등)</summary>
    Other,

    /// <summary>템플(신전, vortex). 섬 소유·다리/골렘 공급·에너지 공급</summary>
    Temple,

    /// <summary>워크샵(factory). 유닛을 덱에 등록한다</summary>
    Workshop,

    /// <summary>아웃포스트 (멀티플레이 중립 섬 소유용 건물)</summary>
    Outpost,

    /// <summary>알타(제단, altar·dais)</summary>
    Altar,

    /// <summary>High Priest</summary>
    Priest,

    /// <summary>Generator (class "Source of Energy"). 건물형 유닛이면서 에너지 공급원</summary>
    Generator,

    /// <summary>건물형 유닛 (포대·방벽·탑·기지, emplacement)</summary>
    Emplacement,

    /// <summary>이동형 수송 유닛 (지상 walker·공중 balloon)</summary>
    Transport,

    /// <summary>기지가 만드는 공중 공격체 (flyer: Whirligig·Man o' War·Dust Devil) — 비용·에너지 없음</summary>
    Flyer,

    /// <summary>다리 조각</summary>
    Bridge,

    /// <summary>주문 (bomb)</summary>
    Spell,

    /// <summary>가이저 (Storm Power 원천)</summary>
    Geyser,

    /// <summary>Storm Crystal (결정 1개 = 200 SP)</summary>
    Nugget,
}

/// <summary>타입 정의로 오브젝트 분류를 판정한다.</summary>
public static class ObjectKinds
{
    /// <summary>Generator 의 .type class 값</summary>
    private const string GeneratorClass = "Source of Energy";

    /// <summary>
    /// 타입 플래그·class·이름으로 분류한다. 아웃포스트는 factory 플래그를 갖지만 사제가 짓는 건물이므로 이름으로 구분한다.
    /// </summary>
    /// <param name="type">로딩 목록의 타입</param>
    public static ObjectKind Of(TypeInfo type)
    {
        uint f2 = type.Flags2;
        // 플래그 비트(Flag2Words)와 이름으로 먼저 건물·특수 타입을 가린다.
        if ((f2 & TypeFlagBits.Flag2Words["vortex"]) != 0) return ObjectKind.Temple;
        if (type.Name.Equals("outpost", StringComparison.OrdinalIgnoreCase)) return ObjectKind.Outpost;
        if ((f2 & TypeFlagBits.Factory) != 0) return ObjectKind.Workshop;
        if ((f2 & (TypeFlagBits.Flag2Words["altar"] | TypeFlagBits.Flag2Words["dais"])) != 0) return ObjectKind.Altar;
        if ((f2 & TypeFlagBits.Flag2Words["priest"]) != 0) return ObjectKind.Priest;
        if ((f2 & TypeFlagBits.Bridge) != 0) return ObjectKind.Bridge;
        if ((f2 & TypeFlagBits.Flag2Words["bomb"]) != 0) return ObjectKind.Spell;
        if ((f2 & TypeFlagBits.Flag2Words["geyser"]) != 0) return ObjectKind.Geyser;
        if ((f2 & TypeFlagBits.Flag2Words["nugget"]) != 0) return ObjectKind.Nugget;
        if (string.Equals(type.Definition.GetString("class"), GeneratorClass, StringComparison.OrdinalIgnoreCase))
        {
            return ObjectKind.Generator;
        }
        if ((f2 & TypeFlagBits.Flag2Words["flyer"]) != 0) return ObjectKind.Flyer;
        if ((f2 & (TypeFlagBits.Flag2Words["walker"] | TypeFlagBits.Flag2Words["balloon"])) != 0) return ObjectKind.Transport;
        if ((f2 & TypeFlagBits.Flag2Words["emplacement"]) != 0) return ObjectKind.Emplacement;
        return ObjectKind.Other;
    }

    /// <summary>사제의 Construct 메뉴로 짓는 "건물"인지 (템플·워크샵·알타·아웃포스트)</summary>
    /// <param name="kind">분류</param>
    public static bool IsBuilding(ObjectKind kind) =>
        kind is ObjectKind.Temple or ObjectKind.Workshop or ObjectKind.Altar or ObjectKind.Outpost;

    /// <summary>워크샵에서 덱에 등록해 생산하는 "유닛"인지 (Generator·건물형 유닛·수송 유닛)</summary>
    /// <param name="kind">분류</param>
    public static bool IsProducibleUnit(ObjectKind kind) =>
        kind is ObjectKind.Generator or ObjectKind.Emplacement or ObjectKind.Transport;

    /// <summary>에너지 공급원인지 (템플·Generator)</summary>
    /// <param name="kind">분류</param>
    public static bool IsEnergySource(ObjectKind kind) => kind is ObjectKind.Temple or ObjectKind.Generator;
}
