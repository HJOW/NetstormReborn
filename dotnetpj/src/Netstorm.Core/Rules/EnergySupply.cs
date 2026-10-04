namespace Netstorm.Core.Rules;

/// <summary>
/// 에너지 공급원 하나 (신전 또는 Generator). 자기 원소 에너지 1개분을 반지름 안에 공급한다.
/// </summary>
/// <param name="Id">공급원을 구분하는 번호 (오브젝트 번호 등, 결과 표시용)</param>
/// <param name="Element">공급하는 원소</param>
/// <param name="CenterX">발자국 중심 x (칸 좌표)</param>
/// <param name="CenterY">발자국 중심 y (칸 좌표)</param>
/// <param name="Owner">소유 플레이어 번호</param>
public sealed record EnergySource(int Id, Element Element, double CenterX, double CenterY, int Owner);

/// <summary>배치 위치의 에너지 판정 결과.</summary>
/// <param name="Satisfied">필요 에너지를 모두 채웠는지</param>
/// <param name="Assigned">요구 글자 순서대로 배정된 공급원 (채우지 못한 글자는 null)</param>
/// <param name="Covering">위치를 덮는 (소유·동맹) 공급원 전체</param>
public sealed record EnergyCheck(bool Satisfied, IReadOnlyList<EnergySource?> Assigned, IReadOnlyList<EnergySource> Covering);

/// <summary>
/// 건설 위치의 에너지 공급 판정 (원본 Mana.cpp FUN_004730c0·FUN_00473330·FUN_004734d0, docs/exe/energy-requirements.md).
/// <list type="bullet">
/// <item><description>위치를 덮는 공급원: 자기 또는 동맹 소유이고, 공급원 중심과 배치 발자국 중심의 칸 거리² ≤ 반지름².</description></item>
/// <item><description>원소 글자(w·r·t)는 같은 원소 공급원에만, 's'(Sun)는 아직 쓰지 않은 아무 공급원에 대응한다.</description></item>
/// <item><description>공급원 하나는 한 글자에만 쓰인다 (소모 개념은 아니므로 다른 유닛 판정에는 다시 쓰인다).</description></item>
/// </list>
/// 원본은 배치 대상 크기((x1 − x0) × 0.7071)도 공급원 판정 함수에 넘기지만 그 사용 방식은 미확인이라 중심 거리만 쓴다.
/// </summary>
public static class EnergySupply
{
    /// <summary>
    /// 한 위치를 덮는 공급원을 고른다.
    /// </summary>
    /// <param name="sources">맵의 모든 공급원</param>
    /// <param name="centerX">배치 발자국 중심 x</param>
    /// <param name="centerY">배치 발자국 중심 y</param>
    /// <param name="radiusSquared">공급 반지름² (BattleOptions.GeneratorRadiusSquared)</param>
    /// <param name="isFriendly">공급원 소유자를 쓸 수 있는지 (자기 또는 동맹)</param>
    public static IReadOnlyList<EnergySource> Covering(IEnumerable<EnergySource> sources, double centerX, double centerY,
        double radiusSquared, Func<int, bool> isFriendly)
    {
        var result = new List<EnergySource>();
        // 소유 조건과 거리 조건을 모두 만족하는 공급원만 모은다.
        foreach (EnergySource source in sources)
        {
            double dx = source.CenterX - centerX;
            double dy = source.CenterY - centerY;
            if (isFriendly(source.Owner) && dx * dx + dy * dy <= radiusSquared)
            {
                result.Add(source);
            }
        }
        return result;
    }

    /// <summary>
    /// 덮는 공급원으로 요구값을 채울 수 있는지 판정한다.
    /// 원소 글자를 먼저 같은 원소 공급원에 배정하고, 남은 공급원으로 's' 를 채운다
    /// (원본 기본 요구 문자열은 원소 글자가 앞에 오므로 결과가 같고, 사용자 지정 순서에서도 불리해지지 않는다).
    /// </summary>
    /// <param name="requirement">필요 에너지</param>
    /// <param name="covering">위치를 덮는 공급원</param>
    public static EnergyCheck Match(EnergyRequirement requirement, IReadOnlyList<EnergySource> covering)
    {
        var assigned = new EnergySource?[requirement.Count];
        var used = new bool[covering.Count];
        // 1단계: 원소 글자(w·r·t)를 같은 원소의 쓰지 않은 공급원에 배정한다.
        for (int i = 0; i < requirement.Count; i++)
        {
            Element need = Elements.FromLetter(requirement.Letters[i])!.Value;
            if (need != Element.Sun)
            {
                assigned[i] = Take(covering, used, s => s.Element == need);
            }
        }
        // 2단계: Sun 글자('s')를 남은 아무 공급원에 배정한다.
        for (int i = 0; i < requirement.Count; i++)
        {
            if (Elements.FromLetter(requirement.Letters[i]) == Element.Sun)
            {
                assigned[i] = Take(covering, used, _ => true);
            }
        }
        return new EnergyCheck(assigned.All(a => a != null), assigned, covering);
    }

    /// <summary>위치를 덮는 공급원을 찾고 요구값을 판정하는 한 번에 쓰는 도우미</summary>
    /// <param name="requirement">필요 에너지</param>
    /// <param name="sources">맵의 모든 공급원</param>
    /// <param name="target">배치할 발자국</param>
    /// <param name="radiusSquared">공급 반지름²</param>
    /// <param name="isFriendly">공급원 소유자를 쓸 수 있는지</param>
    public static EnergyCheck Check(EnergyRequirement requirement, IEnumerable<EnergySource> sources, Footprint target,
        double radiusSquared, Func<int, bool> isFriendly) =>
        Match(requirement, Covering(sources, target.CenterX, target.CenterY, radiusSquared, isFriendly));

    /// <summary>조건에 맞는 첫 번째 미사용 공급원을 사용 표시하고 돌려준다. 없으면 null.</summary>
    private static EnergySource? Take(IReadOnlyList<EnergySource> covering, bool[] used, Func<EnergySource, bool> accept)
    {
        // 목록 순서대로 첫 후보를 고른다 (결정론적).
        for (int i = 0; i < covering.Count; i++)
        {
            if (!used[i] && accept(covering[i]))
            {
                used[i] = true;
                return covering[i];
            }
        }
        return null;
    }
}
