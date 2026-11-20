// 실제 배송값과 대기 기간의 약한 Source 경로를 분리한다.
namespace Netstorm.Net;

/// <summary>배송 동안 같은 실제 장치 전체와 참조 수명을 보유한다.</summary>
public sealed class NativeBattleSource : IDisposable
{
    /// <summary>명목 token이 아닌 같은 실제 coordinator다.</summary>
    public CompositeMissionTransport Device { get; }
    /// <summary>이 배송값이 소유한 실제 Source 대여다.</summary>
    private ConnectionDevicePin? _pin;
    /// <summary>실제 장치를 이미 선택한 대여만 받아 새 Source를 합성하지 않는다.</summary>
    internal NativeBattleSource(CompositeMissionTransport device, ConnectionDevicePin pin) { Device = device; _pin = pin; }
    /// <summary>같은 실제 장치의 새 노드/동기 호출 대여를 제공한다.</summary>
    internal ConnectionDevicePin Pin() => _pin?.Fork() ?? throw new ObjectDisposedException(nameof(NativeBattleSource));
    /// <summary>배송을 마친 동일 대여 하나만 반납한다.</summary>
    public void Dispose() { ConnectionDevicePin? pin = _pin; _pin = null; pin?.Dispose(); }
}

/// <summary>next7 대기 동안 실제 Source를 강하게 소유하지 않는 경로다.</summary>
public sealed class WeakNativeBattleSource
{
    /// <summary>원래 실제 장치의 약한 정체성이다.</summary>
    private readonly WeakReference<CompositeMissionTransport> _device;
    /// <summary>현재 배송값의 같은 실제 장치만 약하게 기억한다.</summary>
    public WeakNativeBattleSource(NativeBattleSource source) { ArgumentNullException.ThrowIfNull(source); _device = new(source.Device); }
    /// <summary>실제 장치가 살아 있을 때만 같은 배송 수명을 복구한다.</summary>
    public NativeBattleSource? Lock() => _device.TryGetTarget(out var current) ? current.Borrow() : null;
}

/// <summary>Client 수명의 약한 장부이며 실제 노드의 주소로 최신 Source를 고른다.</summary>
public sealed class ClientPacketSources
{
    /// <summary>강한 callback 캡처 없이 같은 실제 장치만 보관한다.</summary>
    private readonly List<WeakReference<CompositeMissionTransport>> _routes = [];
    /// <summary>바인딩한 같은 장기 Registry이며 다른 표의 경로를 거부한다.</summary>
    private RawConnectionRegistry? _registry;
    /// <summary>생존 실제 경로를 판별하며 끝난 weak 장부만 제거한다.</summary>
    private static bool Alive(WeakReference<CompositeMissionTransport> route) => route.TryGetTarget(out var device) && !device.Released;
    /// <summary>노드가 등록될 때부터 실제 Source 대여를 유지하도록 같은 Registry에 연결한다.</summary>
    public void BindRegistry(RawConnectionRegistry registry)
    {
        ArgumentNullException.ThrowIfNull(registry); if (_registry is not null && !ReferenceEquals(_registry, registry)) throw new InvalidOperationException("다른 Registry로 Source 장부 재바인딩"); _registry = registry; registry.BindDevicePin(PinFor, true);
    }
    /// <summary>실제 Source 생성 뒤 listener/Client 등록 전에 약한 경로를 공개한다.</summary>
    public void Publish(CompositeMissionTransport source)
    {
        ArgumentNullException.ThrowIfNull(source); if (source.Released || (_registry is not null && !ReferenceEquals(_registry, source.Registry))) throw new ArgumentException("실제 Source/Registry 경로 불일치", nameof(source));
        _routes.RemoveAll(route => !Alive(route)); _routes.Add(new(source));
    }
    /// <summary>현재 실제 같은 노드의 최신 장치만 등록/동기 호출 수명으로 선택한다.</summary>
    private ConnectionDevicePin? PinFor(PacketConnection connection)
    {
        // 최신 실제 coordinator 경로를 우선하며 번호/phase로 추정하지 않는다.
        for (int index = _routes.Count - 1; index >= 0; index--) if (_routes[index].TryGetTarget(out var source) && !source.Released && source.OwnsConnection(connection)) return source.Pin();
        return null;
    }
    /// <summary>실제 노드 주소와 일치한 최신 장치만 배송값으로 빌린다.</summary>
    public NativeBattleSource? Resolve(PacketConnection connection)
    {
        ArgumentNullException.ThrowIfNull(connection);
        // 살아 있는 같은 노드의 실제 소유자만 뒤에서부터 선택한다.
        for (int index = _routes.Count - 1; index >= 0; index--) if (_routes[index].TryGetTarget(out var source) && !source.Released && source.OwnsConnection(connection)) return source.Borrow();
        return null;
    }
    /// <summary>실제 Source를 보유한 채 App 최종 노드→listener 정리 뒤에만 약한 장부를 비운다.</summary>
    public void Shutdown(Action closeNodes)
    {
        ArgumentNullException.ThrowIfNull(closeNodes); var selected = new List<NativeBattleSource>();
        try
        {
            // closeNodes가 참조 순환을 끊기 전에 현재 실제 장치를 모두 보유한다.
            foreach (var route in _routes.ToArray()) if (route.TryGetTarget(out var source) && source.Borrow() is { } current) selected.Add(current);
            closeNodes();
            // 모든 실제 장치를 유지한 채 자기 listener/내부 끝을 정리한다.
            foreach (NativeBattleSource source in selected) source.Device.CloseRoom();
            _routes.Clear();
        }
        finally
        {
            // 장부를 비운 뒤 실제 배송 대여를 모두 반납한다.
            foreach (NativeBattleSource source in selected) source.Dispose();
        }
    }
}
