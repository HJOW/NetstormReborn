// 실제 전송 source의 반납을 등록 노드와 동기 호출의 마지막 참조 뒤로 미룬다.
namespace Netstorm.Net;

/// <summary>명목 token이 아니라 실제 전송 장치 전체를 소유하는 단일 스레드 참조 수명이다.</summary>
public sealed class ConnectionDeviceLifetime : IDisposable
{
    /// <summary>등록 노드/호출 pin이 실제로 보유하는 전송 source다.</summary>
    private object? _owner;
    /// <summary>마지막 실제 참조 뒤 한 번 수행하는 source 반납이다.</summary>
    private readonly Action<object> _release;
    /// <summary>최초 외부 소유권1과 현재 대여의 합이다.</summary>
    private uint _references = 1;
    /// <summary>최초 외부 소유권을 이미 반납했는지다.</summary>
    private bool _disposed;
    /// <summary>실제 장치와 최종 반납을 빌리며 생성 시 장치 효과는 실행하지 않는다.</summary>
    public ConnectionDeviceLifetime(object owner, Action<object> release)
    {
        ArgumentNullException.ThrowIfNull(owner); ArgumentNullException.ThrowIfNull(release); _owner = owner; _release = release;
    }
    /// <summary>등록 노드가 보유한 source도 다시 빌릴 수 있으며 완전 반납 뒤에는 거부한다.</summary>
    public ConnectionDevicePin Pin()
    {
        object owner = _owner ?? throw new ObjectDisposedException(nameof(ConnectionDeviceLifetime));
        if (_references == uint.MaxValue) throw new OverflowException("실제 전송 장치 pin 개수 소진");
        var pin = new ConnectionDevicePin(this, owner); _references++; return pin;
    }
    /// <summary>원문 부모의 후속 쓰기를 마친 실제 대여 하나만 반납한다.</summary>
    internal void ReturnPin()
    {
        if (_references == 0) throw new InvalidOperationException("실제 전송 장치 pin 중복 반납");
        if (--_references != 0) return;
        object owner = _owner ?? throw new InvalidOperationException("실제 전송 source 소유 누락");
        _owner = null; _release(owner);
    }
    /// <summary>외부 source owner를 한 번 반납한다. 등록/실행 대여는 실제 마지막 복귀까지 유지한다.</summary>
    public void Dispose() { if (_disposed) return; _disposed = true; ReturnPin(); }
}

/// <summary>같은 실제 source와 제어 블록을 호출 종료까지 보유하는 결정적 대여다.</summary>
public sealed class ConnectionDevicePin : IDisposable
{
    /// <summary>반납 전 동일 제어 블록이며 이미 반납됐으면NULL이다.</summary>
    private ConnectionDeviceLifetime? _lifetime;
    /// <summary>실제 전송 장치 전체의 강한 참조다.</summary>
    private object? _owner;
    /// <summary>성공한 실제 수명 선택만 담는다.개수 증가는 원래 소유자가 수행한다.</summary>
    internal ConnectionDevicePin(ConnectionDeviceLifetime lifetime, object owner) { _lifetime = lifetime; _owner = owner; }
    /// <summary>별칭 참조의 제어 블록 정체성을 비교한다.반납 뒤 사용은 거부한다.</summary>
    internal ConnectionDeviceLifetime Identity => _lifetime ?? throw new ObjectDisposedException(nameof(ConnectionDevicePin));
    /// <summary>실제 같은 source를 새 동기 호출에 대여한다.</summary>
    internal ConnectionDevicePin Fork() => Identity.Pin();
    /// <summary>동기 source 반납 콜백의 재진입 전에 자기 대여를 분리한다.</summary>
    public void Dispose()
    {
        ConnectionDeviceLifetime? lifetime = _lifetime; object? owner = _owner; _lifetime = null; _owner = null;
        try { lifetime?.ReturnPin(); } finally { GC.KeepAlive(owner); }
    }
}
