// 실제 UI 등록표와 짧은 World 통지를 같은 장기 GumpForm에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>현재 Gump 통지와 호출 동안 유지할 실제 소유자다.</summary>
public sealed class RawSurfaceGumpBindings
{
    /// <summary>현재 등록 ID와 삭제된 부모를 실제 UI에 전달한다.</summary>
    public GumpFormNotice? Notice { get; set; }
    /// <summary>통지 호출이 끝날 때까지 현재 소유자를 유지한다.</summary>
    public Func<object?>? Pin { get; set; }
}

/// <summary>장기 GumpForm에서 실제 World 또는 UI의 현재 통지 대여를 선택한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>현재 World가 소유하는 짧은 통지 대여다.</summary>
    private WeakReference<RawSurfaceGumpBindings>? _gumpRuntime;
    /// <summary>현재 화면이 소유하는 장기 UI 통지 대여다.</summary>
    private WeakReference<RawSurfaceGumpBindings>? _gumpUiRuntime;
    /// <summary>Client가 소유하는 실제 현재 Gump TYPE 주소다.</summary>
    private UIntBox? _gumpType;

    /// <summary>첫 장기 GumpForm 생성 전에 같은 Client의 현재 TYPE 주소를 빌린다.</summary>
    public void BindGumpType(UIntBox type)
    {
        ArgumentNullException.ThrowIfNull(type);
        if (!UsesNativeTransmissionDevices() || _gumpForms is not null || _gumpType is not null)
            throw new ArgumentException("장기 Gump의 최초 Native TYPE 주소 필요", nameof(type));
        _gumpType = type;
    }

    /// <summary>World 통지를 약하게 빌리며 살아 있는 다른 대여를 덮지 않는다.</summary>
    public void BindGumpRuntime(RawSurfaceGumpBindings runtime)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        if (!UsesNativeTransmissionDevices() || CurrentGumpRuntime() is not null)
            throw new ArgumentException("현재 Gump World 통지 대여 중복 또는 Native 장치 누락", nameof(runtime));
        _gumpRuntime = new(runtime);
    }

    /// <summary>실제 UI 등록표와 수명을 약하게 빌린다.</summary>
    public void BindGumpUiRuntime(RawSurfaceGumpBindings runtime)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        if (!UsesNativeTransmissionDevices() || runtime.Notice is null || runtime.Pin is null || CurrentGumpUiRuntime() is not null)
            throw new ArgumentException("현재 Gump UI 통지 또는 수명 대여 중복/누락", nameof(runtime));
        _gumpUiRuntime = new(runtime);
    }

    /// <summary>현재 World가 자기 통지 대여만 반납한다.</summary>
    public void ReleaseGumpRuntime(RawSurfaceGumpBindings runtime)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        if (!ReferenceEquals(CurrentGumpRuntime(), runtime)) throw new ArgumentException("같은 Gump World 대여 필요", nameof(runtime));
        _gumpRuntime = null;
    }

    /// <summary>현재 화면이 자기 UI 통지 대여만 반납한다.</summary>
    public void ReleaseGumpUiRuntime(RawSurfaceGumpBindings runtime)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        if (!ReferenceEquals(CurrentGumpUiRuntime(), runtime)) throw new ArgumentException("같은 Gump UI 대여 필요", nameof(runtime));
        _gumpUiRuntime = null;
    }

    /// <summary>살아 있는 현재 World 대여를 호출 동안 강하게 빌린다.</summary>
    private RawSurfaceGumpBindings? CurrentGumpRuntime() => _gumpRuntime is not null && _gumpRuntime.TryGetTarget(out var value) ? value : null;
    /// <summary>살아 있는 현재 UI 대여를 호출 동안 강하게 빌린다.</summary>
    private RawSurfaceGumpBindings? CurrentGumpUiRuntime() => _gumpUiRuntime is not null && _gumpUiRuntime.TryGetTarget(out var value) ? value : null;

    /// <summary>World 통지가 있으면 우선하고 없으면 실제 UI 등록표에 전달한다.</summary>
    private void DeliverGumpNotice(uint owner, int parent)
    {
        RawSurfaceGumpBindings? world = CurrentGumpRuntime();
        RawSurfaceGumpBindings? ui = CurrentGumpUiRuntime();
        object? worldOwner = null;
        object? uiOwner = null;
        try
        {
            if (world?.Notice is not null) worldOwner = world.Pin?.Invoke();
            uiOwner = ui?.Pin?.Invoke();
            GumpFormNotice notice = world?.Notice ?? ui?.Notice ?? throw new InvalidOperationException("현재 Gump 실제 UI 통지 미연결");
            notice(owner, parent);
        }
        finally
        {
            try { RawRuntimePin.Release(uiOwner); }
            finally { RawRuntimePin.Release(worldOwner); }
        }
    }
}
