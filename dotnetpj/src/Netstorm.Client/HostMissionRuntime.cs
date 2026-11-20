using Netstorm.Client.Host;
using Netstorm.Net;
using Netstorm.Core.Original;

// 현재 실제 Session/풀/장치를 한 외곽 수명으로 보유한다.
namespace Netstorm.Client;

/// <summary>명목 토큰 대신 실제 제품 세션과 같은 물리/장치 묶음을 보유하는 Runtime이다.</summary>
public sealed class HostMissionRuntime : IDisposable
{
    /// <summary>실제 현재 Player/월드/프로세스/장치를 소유하는 세션이다.</summary>
    public HostSession Session { get; }
    /// <summary>같은 실제 풀을 제공한 외곽 물리 저장소다.</summary>
    public RawSurfaceStorage Storage { get; }
    /// <summary>실제 음향/표시/UI 객체를 배송 끝까지 보유한다. 반납 효과는 같은 호스트가 수행한다.</summary>
    private readonly object[] _devices;
    /// <summary>외부 owner와 실제 호출 대여의 결정적 수명이다.</summary>
    private readonly ConnectionDeviceLifetime _lifetime;
    /// <summary>같은 현재 Gameplay의 준비된 대여다.</summary>
    private ClientGameplayGlobals.GameplayRuntimeUse? _use;
    /// <summary>실제 마지막 참조 뒤 공개 반납을 마쳤는지다.</summary>
    public bool Released { get; private set; }
    /// <summary>외부 소유자가 구성되기 전에는 현재 Session을 공개하지 않는다.</summary>
    private HostMissionRuntime(HostSession session, RawSurfaceStorage storage, object[] devices)
    {
        ArgumentNullException.ThrowIfNull(session); ArgumentNullException.ThrowIfNull(storage); ArgumentNullException.ThrowIfNull(devices);
        if (!ReferenceEquals(session.Pool, storage.Pool)) throw new ArgumentException("Runtime의 실제 Session/물리 Pool 불일치", nameof(storage));
        // 실제 장치 참조만 보유하며 NULL/가짜 성공을 위한 대체 객체를 만들지 않는다.
        foreach (object device in devices) ArgumentNullException.ThrowIfNull(device);
        Session = session; Storage = storage; _devices = devices.ToArray(); _lifetime = new(this, static owner => ((HostMissionRuntime)owner).Finish());
    }
    /// <summary>실제 전체 owner를 먼저 구성하고 준비 대여→Session공개→weak owner 순서로 연결한다.</summary>
    public static HostMissionRuntime Create(HostSession session, RawSurfaceStorage storage, params object[] actualDevices)
    {
        var runtime = new HostMissionRuntime(session, storage, actualDevices);
        try
        {
            runtime._use = session.Gameplay.AcquirePrepared(); runtime._use.Publish(session); session.Gameplay.BindRuntimeOwner(session, runtime); return runtime;
        }
        catch { runtime.Dispose(); throw; }
    }
    /// <summary>현재 실제 owner의 같은 장치 전체를 배송 끝까지 보유한다.</summary>
    internal ConnectionDevicePin Pin() => _lifetime.Pin();
    /// <summary>실제 호출이 모두 끝난 뒤 같은 공개/대여를 반납하고 장치 참조 수명을 마친다.</summary>
    private void Finish()
    {
        try { _use?.Dispose(); _use = null; Released = true; }
        finally { GC.KeepAlive(_devices); GC.KeepAlive(Storage); GC.KeepAlive(Session); }
    }
    /// <summary>외부 owner만 반납한다. 호출 대여가 있으면 마지막 복귀까지 실제 반납을 미룬다.</summary>
    public void Dispose() => _lifetime.Dispose();
}
