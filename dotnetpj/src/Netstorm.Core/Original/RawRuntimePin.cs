// 관리소유자보유와명시반납이필요한한호출대여를구별한다.
namespace Netstorm.Core.Original;

/// <summary>Pin함수가이번호출만을위해발급한대여다. 일반IDisposable소유자와구별하여복귀때만반납한다.</summary>
public interface IRawRuntimeLease : IDisposable
{
}

/// <summary>기존강한소유자보유계약을유지하며표시된호출대여만명시반납한다.</summary>
public static class RawRuntimePin
{
    /// <summary>일반소유자는끝까지보유하고명시호출대여는마지막후속효과뒤반납한다.</summary>
    public static void Release(object? owner)
    {
        try { if (owner is IRawRuntimeLease lease) lease.Dispose(); }
        finally { GC.KeepAlive(owner); }
    }
}
