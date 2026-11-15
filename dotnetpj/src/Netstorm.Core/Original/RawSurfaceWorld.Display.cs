using Netstorm.Core.Display;

// World의 실제 표시 관찰/카메라 대여만 장기 Storage에 약하게 공개한다.
namespace Netstorm.Core.Original;

/// <summary>현재 World의 표시 뷰·짧은 장치와 전체 Runtime pin을 관리한다.</summary>
public sealed partial class RawSurfaceWorld
{
    /// <summary>독립 World의 기존 초기 뷰이며 SetDisplay에서 값 복사로 교체한다.</summary>
    private SquidDisplayView _displayView = new() { CameraX = 0, CameraY = 0, Zoom = 65536, Viewport = new DisplayRect(0, 0, 640, 480) };
    /// <summary>World만 소유하고 Storage는 약하게 빌리는 현재 표시 장치다.</summary>
    private RawSurfaceDisplayBindings? _displayRuntime;
    /// <summary>현재 실제 전체 Runtime owner를 선택 호출 동안 유지하는 장치다.</summary>
    private Func<object?>? _runtimePin;

    /// <summary>같은 현재 뷰/억제/dirty 관찰만 대여하고 Storage의 유일한 표시 실행기를 빌린다.</summary>
    private SquidDisplay BindStorageDisplay()
    {
        var runtime = new RawSurfaceDisplayBindings
        {
            Changed = () => _changed = true, View = () => _displayView,
            Suppressed = () => _displayHooks.Suppressed?.Invoke() ?? false,
            Invalidate = (rect, flags) => { _changed = true; _displayHooks.Invalidate?.Invoke(rect, flags); },
            Pin = _runtimePin,
        };
        _storage.BindDisplayRuntime(runtime); _displayRuntime = runtime; return _storage.DisplayExecutor();
    }
    /// <summary>현재 World가 가진 같은 표시 대여만 반납하고 다음 World/장기 Client 장치를 덮지 않는다.</summary>
    private void ReleaseStorageDisplay()
    {
        if (_displayRuntime is null) return;
        _storage.ReleaseDisplayRuntime(_displayRuntime); _displayRuntime = null;
    }
    /// <summary>실제 전체 Runtime의 선택 호출 수명을 현재 표시/공장/후처리에 연결하거나 반납한다.</summary>
    public void BindRuntimePin(Func<object?>? pin)
    {
        _runtimePin = pin;
        if (_displayRuntime is not null) _displayRuntime.Pin = pin;
        if (_factoryRuntime is not null) _factoryRuntime.Pin = pin;
        _postPop?.BindRuntimePin(pin);
    }
}
