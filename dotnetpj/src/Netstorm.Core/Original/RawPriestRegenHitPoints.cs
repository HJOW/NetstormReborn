// 실제 회복 HP 부모의 Getter·현재 번호 조회 순서와 signed ADD 감김을 보존한다.
namespace Netstorm.Core.Original;

/// <summary>도달한 조회 위치에서만 선택하는 현재 HP·타입 getter다.</summary>
public sealed class PriestRegenHitPointHooks
{
    /// <summary>같은 사제의 현재 signed HP를 읽는다.</summary>
    public Func<int>? Current { get; set; }
    /// <summary>같은 객체의 현재 최대 HP를 읽는다.</summary>
    public Func<int>? Maximum { get; set; }
    /// <summary>각 증분마다 현재 사제TYPE를 읽는다.</summary>
    public Func<uint>? PriestType { get; set; }
    /// <summary>고정한 TYPE 번호의 HP getter를 실행한다.</summary>
    public Func<uint, int>? TypeMaximum { get; set; }
}

/// <summary>회복 Setter에 넘길 값만 계산하며 빈 결과는 초기HP가 최대HP 이상인 경우다.</summary>
public static class RawPriestRegenHitPoints
{
    /// <summary>선택된 함수만 사본으로 확보하여 자기 연결 변경 중에도 호출 수명을 유지한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function
        ?? throw new InvalidOperationException("회복 HP 부모의 선택된 실제 장치 미연결");
    /// <summary>원문 signed ADD의 하위32비트를 유지한다.</summary>
    private static int Add(int value, int increment) => unchecked((int)((uint)value + (uint)increment));
    /// <summary>PATCH는 HP를 타입 getter 전에, CD는 그 뒤에 읽는다. 번호는 두 판본 모두 HP보다 먼저 고정한다.</summary>
    public static int? Read(OriginalEdition edition, PriestRegenHitPointHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        int initial = Need(hooks.Current)(); int maximum = Need(hooks.Maximum)();
        if (initial >= maximum) return null;
        // 후보 계산과 최종 비포화 계산은 현재 번호·현재 HP를 각각 다시 조회한다.
        int Increment()
        {
            uint number = Need(hooks.PriestType)();
            if (edition == OriginalEdition.Patch1078)
            {
                int current = Need(hooks.Current)(); int typeHp = Need(hooks.TypeMaximum)(number);
                return Add(current, typeHp / 6);
            }
            int cdTypeHp = Need(hooks.TypeMaximum)(number); int cdCurrent = Need(hooks.Current)();
            return Add(cdCurrent, cdTypeHp / 6);
        }
        int candidate = Increment(); int currentMaximum = Need(hooks.Maximum)();
        if (candidate > currentMaximum) return Need(hooks.Maximum)();
        return Increment();
    }
}
