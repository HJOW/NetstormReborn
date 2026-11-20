// 같은 장기 Pool/Graph/공통 삭제로 표면 수명을 소유하고 큰 파생 효과만 현재 Runtime에서 빌린다.
// cpppj/src/client/RawSurfaceLifetime.cpp 의 BindSurfaceLifetimeRuntime/SurfaceLifetime 장기 본문을 이식한다.
// 0.577에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Surface 우선)
namespace Netstorm.Core.Original;

/// <summary>현재 World가 소유하고 Storage가 약하게 빌리는 큰 표면 효과 경계다.</summary>
public sealed class RawSurfaceLifetimeBindings
{
    /// <summary>다리 위 보행자의 실제 가상 낙하를 같은 전체 Runtime에서 빌린다. 0.577에서 추가됐다.</summary>
    public Action<int>? FallWalker { get; set; }

    /// <summary>다리 파편 생성 요청과 낙하 소리를 같은 전체 Runtime에서 빌린다. 0.577에서 추가됐다.</summary>
    public Action<BridgeLifecycleEvent>? BridgeEffect { get; set; }

    /// <summary>선택 호출의 정상/예외 복귀까지 실제 전체 owner를 보유한다. 0.577에서 추가됐다.</summary>
    public Func<object?>? Pin { get; set; }
}

/// <summary>World 없이 같은 장기 공통 삭제/Host/TYPE로 표면 수명을 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>현재 World 반납 뒤 큰 파생 종단 주소를 남기지 않는 약한 표면 수명 대여다. 0.577에서 추가됐다.</summary>
    private WeakReference<RawSurfaceLifetimeBindings>? _surfaceLifetimeRuntime;

    /// <summary>같은 공통 삭제/Host/TYPE를 사용하는 실제 표면 수명이다. 0.577에서 추가됐다.</summary>
    private RawSurfaceLifecycle? _surfaceLifetime;

    /// <summary>짧은 World의 실제 효과 소유권만 연결하며 다른 활성 대여가 있으면 거부한다. 0.577에서 추가됐다.</summary>
    /// <param name="runtime">현재 World가 소유하는 실제 표면 효과 대여다.</param>
    public void BindSurfaceLifetimeRuntime(RawSurfaceLifetimeBindings runtime)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        if (!UsesNativeTransmissionDevices() || runtime.FallWalker is null || runtime.BridgeEffect is null)
        {
            throw new ArgumentException("현재 표면 수명 실제 종단의 중복/누락", nameof(runtime));
        }

        if (_surfaceLifetimeRuntime is not null && _surfaceLifetimeRuntime.TryGetTarget(out _))
        {
            throw new ArgumentException("현재 표면 수명 실제 종단의 중복/누락", nameof(runtime));
        }

        _surfaceLifetimeRuntime = new(runtime);
    }

    /// <summary>현재 살아 있는 같은 표면 대여만 선택 호출 동안 강하게 빌린다. 0.577에서 추가됐다.</summary>
    private RawSurfaceLifetimeBindings? SurfaceLifetimeRuntime()
    {
        if (_surfaceLifetimeRuntime is not null && _surfaceLifetimeRuntime.TryGetTarget(out RawSurfaceLifetimeBindings? current))
        {
            return current;
        }

        return null;
    }

    /// <summary>같은 장기 Pool/Graph/공통 삭제로 표면 수명을 소유한다. 0.577에서 추가됐다.</summary>
    public RawSurfaceLifecycle SurfaceLifetime()
    {
        if (!UsesNativeTransmissionDevices())
        {
            throw new InvalidOperationException("장기 표면 수명의 실제 Native owner 미연결");
        }

        if (_surfaceLifetime is null)
        {
            var hooks = new SurfaceLifecycleHooks
            {
                ScheduleFall = (sid, x, y) => { _ = RawBridgeFallParent.ScheduleChecked(Processes().Host(), Pool, Types, sid, x, y, Checking()); },
                Destroy = (sid, flags) => { DestroyAsset(sid, flags); },
                FallWalker = sid =>
                {
                    RawSurfaceLifetimeBindings? current = SurfaceLifetimeRuntime();
                    if (current?.FallWalker is null)
                    {
                        throw new InvalidOperationException("표면의 선택된 실제 큰 Runtime 미연결");
                    }

                    Func<object?>? pin = current.Pin;
                    if (pin is null)
                    {
                        throw new InvalidOperationException("표면의 선택된 실제 전체 Runtime pin 미연결");
                    }

                    object? owner = pin();
                    try
                    {
                        Action<int>? call = current.FallWalker;
                        if (call is null)
                        {
                            throw new InvalidOperationException("표면의 선택된 실제 큰 효과 미연결");
                        }

                        call(sid);
                    }
                    finally
                    {
                        RawRuntimePin.Release(owner);
                    }
                },
                BridgeEffect = @event =>
                {
                    RawSurfaceLifetimeBindings? current = SurfaceLifetimeRuntime();
                    if (current?.BridgeEffect is null)
                    {
                        throw new InvalidOperationException("표면의 선택된 실제 큰 Runtime 미연결");
                    }

                    Func<object?>? pin = current.Pin;
                    if (pin is null)
                    {
                        throw new InvalidOperationException("표면의 선택된 실제 전체 Runtime pin 미연결");
                    }

                    object? owner = pin();
                    try
                    {
                        Action<BridgeLifecycleEvent>? call = current.BridgeEffect;
                        if (call is null)
                        {
                            throw new InvalidOperationException("표면의 선택된 실제 큰 효과 미연결");
                        }

                        call(@event);
                    }
                    finally
                    {
                        RawRuntimePin.Release(owner);
                    }
                },
            };
            var body = new RawSurfaceLifecycle(Pool, NeighborsExecutor(), Links.ConnectorType, CommonDeletion().Hooks(), hooks);
            body.BindCurrentConnectorType(Links.ConnectorTypeBox);
            _surfaceLifetime = body;
        }

        return _surfaceLifetime;
    }
}
