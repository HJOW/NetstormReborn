// 사제 정기494580의 선택 분기와601 반환·삭제·Carrier 부모 순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>사제 정기가 선택한 실제 후단만 제공하는 현재 함수 경계다.</summary>
public sealed class PriestRegularHooks
{
    /// <summary>601의count가0이 아닐 때 실제 가상 삭제를flags0x100000으로 요청한다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>602회복/603추락/604의식프레임/605기도의 실제 고유 후단이다.</summary>
    public RegularHandler? Unique { get; set; }
    /// <summary>나머지 모든 사건의 직접Carrier 부모다.</summary>
    public RegularHandler? Carrier { get; set; }
}

/// <summary>선택한 실제 경계만 요구하며 사건/개수/payload 원문 비트를 그대로 전달한다.</summary>
public static class RawPriestRegular
{
    /// <summary>601의최초3초 예약/후속삭제,602~605고유후단,나머지Carrier를 원문 순서로 분배한다.</summary>
    public static float Run(int sid, uint ev, uint count, float payload, PriestRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (ev == 601)
        {
            if (count == 0) return 3;
            if (hooks.Destroy is null) throw new InvalidOperationException("사제601 실제 삭제 미연결");
            hooks.Destroy(sid, 0x100000); return -1;
        }
        if (ev >= 602 && ev <= 605)
        {
            if (hooks.Unique is null) throw new InvalidOperationException("사제 고유 정기 실제 후단 미연결");
            return hooks.Unique(sid, ev, count, payload);
        }
        if (hooks.Carrier is null) throw new InvalidOperationException("사제 정기 실제 Carrier 부모 미연결");
        return hooks.Carrier(sid, ev, count, payload);
    }
}
