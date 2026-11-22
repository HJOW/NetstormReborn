// 실제빔수명본문과분리된원문nullable생성wrapper의반환종류를보존한다.
namespace Netstorm.Core.Original;

/// <summary>원문raw주소반환/NULL/실제생성반환을서로구별한다.</summary>
public enum LightningCreateStatus
{
    /// <summary>최초표적raw가free/dead여서그raw주소를돌려준분기다.</summary>
    TargetUnavailable,
    /// <summary>실제104BYTE생성경계가NULL을돌려준분기다.</summary>
    AllocationFailed,
    /// <summary>명시생성경계가반환한같은객체를보유한분기다.</summary>
    Created,
}
/// <summary>원문raw주소의SID와실제생성경계가반환한객체를혼용하지않는다.</summary>
/// <param name="Status">실제원문반환종류다.</param>
/// <param name="SkippedTarget">raw주소반환일때만의미있는최초표적SID다.</param>
/// <param name="Process">실제생성자가반환한동일객체다.</param>
/// <typeparam name="T">호출자가명시적으로공급하는실제생성객체타입이다.</typeparam>
public readonly record struct LightningCreateResult<T>(LightningCreateStatus Status, ushort SkippedTarget, T? Process) where T : class;

/// <summary>RawLightningLifetime.cpp의앞두생성wrapper만이식하며큰실제생성자는외부공급한다.</summary>
public static class RawLightningCreate
{
    /// <summary>실제표적생존검사뒤에만nullable생성자를요구하고같은반환객체를보존한다.</summary>
    public static LightningCreateResult<T> Create<T>(RawSidPool pool, ushort parent, ushort target, string? sound, Func<ushort, ushort, string?, T?>? create) where T : class
    {
        if (pool.Edition != OriginalEdition.Patch1078 || target >= pool.Capacity) throw new ArgumentException("빔 wrapper 판본/표적 오류"); if ((pool.Slot(target)[11] & 3) != 0) return new(LightningCreateStatus.TargetUnavailable, target, null); if (create is null) throw new ArgumentException("빔 wrapper 실제생성 경계미연결"); T? process = create(parent, target, sound); return new(process is null ? LightningCreateStatus.AllocationFailed : LightningCreateStatus.Created, 0, process);
    }
}
