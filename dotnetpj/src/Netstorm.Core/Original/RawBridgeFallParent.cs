// 현재 다리 낙하 진단 부모와 실제 Regular 예약을 담는다.
namespace Netstorm.Core.Original;

/// <summary>checking·현재 genus·복귀 진단·낙하 자식 예약의 선택 장치다.</summary>
public sealed class BridgeFallParentHooks
{
    /// <summary>checking이 비영일 때만 현재 부모의 TYPE genus를 읽는다.</summary>
    public Func<int, uint>? Genus { get; set; }
    /// <summary>진단 복귀 뒤 원래 SID와 좌표 비트로 같은 실제 낙하 자식을 예약한다.</summary>
    public Action<int, float, float>? Schedule { get; set; }
    /// <summary>Bridge.cpp323의 진단이다. 정상 복귀하면 저장한 원래 인자로 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>cpppj/src/o/RawBridgeFallParent.cpp의 PATCH00421f90 진단 부모와 동일 풀 예약을 실행한다.</summary>
public static class RawBridgeFallParent
{
    /// <summary>선택된 실제 장치만 요구한다. checking0에서는 genus와 진단 장치가 없어도 된다.</summary>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("다리 낙하 부모의 선택된 실제 장치 미연결");

    /// <summary>checking은 한 번 읽고, 선택한 genus/진단 뒤에도 같은 SID·좌표로 예약한다.</summary>
    public static void Run(int bridge, float x, float y, RawCheckingFlag checking, BridgeFallParentHooks hooks)
    {
        if (checking && (Need(hooks.Genus)(bridge) & TypeFlag2.Bridge) == 0)
            Need(hooks.Assertion)("s.getGenus() & gBRIDGE", @"\Ns\O\Bridge.cpp", 0x323);
        Need(hooks.Schedule)(bridge, x, y);
    }

    /// <summary>같은 Host/Pool의 현재 TYPE를 연결해 기존 실제 Regular를 예약한다. CD는 PATCH 진단 부모 없이 자식으로 간다.</summary>
    public static RegularProcess? ScheduleChecked(SquidProcessHost host, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types,
        int bridge, float x, float y, RawCheckingFlag checking, Action<string, string, uint>? assertion = null)
    {
        if (!ReferenceEquals(host.Pool(), pool)) throw new ArgumentException("다리 낙하 부모의 실제 Host/Pool 불일치", nameof(pool));
        if (pool.Edition != OriginalEdition.Patch1078) return SquidProcessHost.ScheduleBridgeFall(host, bridge, x, y);
        RegularProcess? result = null;
        var hooks = new BridgeFallParentHooks
        {
            Genus = sid =>
            {
                byte type = pool.Slot(sid)[10];
                if (type >= types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "다리 낙하 진단의 현재 TYPE 물리 범위");
                return types[type].Flags2;
            },
            Schedule = (sid, px, py) => result = SquidProcessHost.ScheduleBridgeFall(host, sid, px, py),
            Assertion = assertion ?? ((condition, source, line) => throw new InvalidOperationException($"원문 다리 낙하 진단 {source}:{line}: {condition}")),
        };
        Run(bridge, x, y, checking, hooks);
        return result;
    }
}
