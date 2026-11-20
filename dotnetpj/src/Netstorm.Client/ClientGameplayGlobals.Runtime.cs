using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 장기 Gameplay의 weak 공개와 배송 중 실제 전체 Runtime 보유를 구별한다.
namespace Netstorm.Client;

/// <summary>현재 실제 제품 HostSession을 선택하는 준비 대여/실행 pin 부분이다.</summary>
public sealed partial class ClientGameplayGlobals
{
    /// <summary>현재 실제 준비 대여이며 게임 성공 값을 대신하지 않는다.</summary>
    private GameplayRuntimeUse? _runtimeUse;
    /// <summary>같은 현재 Session의 약한 정체성이다. 전역이 짧은 세션을 소유하지 않는다.</summary>
    private WeakReference<HostSession>? _currentSession;
    /// <summary>실제 세션/물리/장치 전체를 소유하는 외곽 Runtime의 weak 공개다.</summary>
    private WeakReference<HostMissionRuntime>? _runtimeOwner;
    /// <summary>공개된 외곽 owner와 일치해야 하는 현재 Session이다.</summary>
    private WeakReference<HostSession>? _runtimeOwnerSession;
    /// <summary>대기 큐와 별개인 실행 중 배송 수다.</summary>
    private ulong _runtimeCalls;
    /// <summary>실제 준비 대여가 아직 주소를 빌리는지다.</summary>
    public bool Borrowed() => _runtimeUse is not null;
    /// <summary>명시 Native prefix/현재값을 재초기화하지 않고 단일 대여만 획득한다.</summary>
    public GameplayRuntimeUse AcquirePrepared()
    {
        if (_runtimeUse is not null) throw new InvalidOperationException("현재 게임 전역 중복 대여"); var use = new GameplayRuntimeUse(this); _runtimeUse = use; return use;
    }
    /// <summary>실제 현재 세션이 필요한 선택 위치에서만 같은 공개 대상을 반환한다.</summary>
    public HostSession RequireSession() => _currentSession is { } current && current.TryGetTarget(out var session) ? session : throw new InvalidOperationException("현재 게임 Runtime 미연결");
    /// <summary>이미 선택한 Session을 실제로 소유한 같은 외곽 Runtime만 weak로 공개한다.</summary>
    public void BindRuntimeOwner(HostSession session, HostMissionRuntime owner)
    {
        ArgumentNullException.ThrowIfNull(session); ArgumentNullException.ThrowIfNull(owner);
        if (!ReferenceEquals(RequireSession(), session) || !ReferenceEquals(owner.Session, session) || owner.Released) throw new ArgumentException("현재 Session과 실제 Runtime owner 공개 불일치");
        if (_runtimeCalls != 0) throw new InvalidOperationException("실제 Runtime 배송 중 owner 재공개");
        _runtimeOwner = new(owner); _runtimeOwnerSession = new(session); var selected = new WeakReference<HostSession>(session);
        session.BindRuntimePin(() => { HostSession current = RequireSession(); if (!selected.TryGetTarget(out var expected) || !ReferenceEquals(current, expected)) throw new InvalidOperationException("전역 대여pin의 현재 Session 교체"); return PinCurrentRuntime(current); });
    }
    /// <summary>현재 Session/실제 owner 일치를 확인한 뒤 실제 전체 owner를 이번 호출 끝까지 보유한다.</summary>
    public IDisposable PinCurrentRuntime(HostSession session)
    {
        if (!ReferenceEquals(RequireSession(), session) || _runtimeOwnerSession is null || !_runtimeOwnerSession.TryGetTarget(out var expected) || !ReferenceEquals(expected, session)) throw new InvalidOperationException("현재 Runtime의 실제 owner 미공개");
        if (_runtimeOwner is null || !_runtimeOwner.TryGetTarget(out var owner) || owner.Released) throw new InvalidOperationException("현재 Runtime의 실제 owner 수명 종료");
        return new RuntimeCall(this, owner);
    }
    /// <summary>같은 실제 현재 owner의 실행 중 배송만 세며 weak 공개는 정리 관문을 잠그지 않는다.</summary>
    public bool RuntimeCallActive(HostSession session) => _runtimeOwnerSession is { } selected && selected.TryGetTarget(out var current) && ReferenceEquals(current, session) && _runtimeCalls != 0;
    /// <summary>현재 공개와 실행 owner를 게임 값/시계/Player 재초기화 없이 분리한다.</summary>
    private void ClearRuntime() { _runtimeOwner = null; _runtimeOwnerSession = null; _currentSession = null; }
    /// <summary>완성한 실제 Session의 공개/반납을 담당하는 단일 준비 대여다.</summary>
    public sealed class GameplayRuntimeUse : IDisposable
    {
        /// <summary>같은 실제 장기 Gameplay다.</summary>
        private readonly ClientGameplayGlobals _globals;
        /// <summary>이미 대여가 끝났는지다.</summary>
        private bool _disposed;
        /// <summary>최초 대여 객체 할당만 수행하며 게임 값을 쓰지 않는다.</summary>
        internal GameplayRuntimeUse(ClientGameplayGlobals globals) => _globals = globals;
        /// <summary>같은 Gameplay의 완성한 실제 HostSession만 현재 대상으로 공개한다.</summary>
        public void Publish(HostSession session)
        {
            ObjectDisposedException.ThrowIf(_disposed, this); ArgumentNullException.ThrowIfNull(session);
            if (!ReferenceEquals(session.Gameplay, _globals) || !ReferenceEquals(_globals._runtimeUse, this)) throw new ArgumentException("현재 게임 전역/Session 소유자 불일치", nameof(session));
            if (_globals._currentSession is not null) throw new InvalidOperationException("현재 게임 Runtime 중복 연결"); _globals._currentSession = new(session);
        }
        /// <summary>모듈 정리 전에 같은 실제 현재 공개만 해제한다. 실행 배송 중에는 거부한다.</summary>
        public void Withdraw(HostSession session)
        {
            ObjectDisposedException.ThrowIf(_disposed, this); if (!ReferenceEquals(_globals.RequireSession(), session)) throw new InvalidOperationException("현재 게임 Runtime 반납 불일치");
            if (_globals.RuntimeCallActive(session)) throw new InvalidOperationException("실제 Runtime 배송 종료 전 공개 반납"); _globals.ClearRuntime();
        }
        /// <summary>준비 대여를 한 번 반납하며 실행 중 명시 반납은 현재 대여를 유지한 채 거부한다.</summary>
        public void Dispose()
        {
            if (_disposed) return; if (_globals._runtimeCalls != 0) throw new InvalidOperationException("실제 Runtime 배송 종료 전 대여 반납");
            if (!ReferenceEquals(_globals._runtimeUse, this)) throw new InvalidOperationException("다른 현재 게임 전역 대여 반납"); _globals.ClearRuntime(); _globals._runtimeUse = null; _disposed = true;
        }
    }
    /// <summary>실제 전체 Runtime을 보유한 후 호출 수를 올리고 마지막 자식 참조보다 먼저 호출 수를 반납한다.</summary>
    private sealed class RuntimeCall : IRawRuntimeLease
    {
        /// <summary>실제 호출 수를 보유하는 같은 장기 Gameplay다.</summary>
        private ClientGameplayGlobals? _globals;
        /// <summary>실제 같은 외곽 세션/물리/장치 묶음의 대여다.</summary>
        private ConnectionDevicePin? _pin;
        /// <summary>호출 수 반납까지 실제 전체 owner를 강하게 유지한다.</summary>
        private HostMissionRuntime? _owner;
        /// <summary>owner 선택/할당 실패가 호출 수를 변경하지 않게 한다.</summary>
        public RuntimeCall(ClientGameplayGlobals globals, HostMissionRuntime owner)
        {
            if (globals._runtimeCalls == ulong.MaxValue) throw new OverflowException("현재 Runtime 배송 호출 수 넘침"); _pin = owner.Pin(); _owner = owner; _globals = globals; globals._runtimeCalls++;
        }
        /// <summary>콜백 종료를 먼저 기록한 뒤 실제 owner를 반납하며 중복 반납을 생략한다.</summary>
        public void Dispose()
        {
            ClientGameplayGlobals? globals = _globals; ConnectionDevicePin? pin = _pin; HostMissionRuntime? owner = _owner; _globals = null; _pin = null; _owner = null;
            if (globals is null) return; globals._runtimeCalls--; try { pin?.Dispose(); } finally { GC.KeepAlive(owner); }
        }
    }
}
