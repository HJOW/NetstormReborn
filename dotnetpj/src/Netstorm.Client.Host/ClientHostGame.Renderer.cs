using Netstorm.Client.Display;
using Netstorm.Client.Input;

// 호스트 화면 장치와 Native 카메라/표면/커서 장치를 같은 Client 수명에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>실제 장기 Renderer와 초기 Cancel의 Root 연결이다. Native Frame 수집/그리기는 후속이다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>논리 화면과 같은 크기의 실제 dirty/그리기 소유자다.</summary>
    private Renderer? _nativeRenderer;
    /// <summary>짧은 Frame/Session을 저장하지 않는 현재 장치 연결기다.</summary>
    private ClientRendererDevices? _nativeRendererDevices;
    /// <summary>실제 Root UI가 제공할 현재 MapViewRect getter다. 미설치를 화면 전체로 대체하지 않는다.</summary>
    private Func<ScreenRect>? _nativeMapView;
    /// <summary>같은 현재 Renderer를 조회하며 초기화/그리기/출력은 실행하지 않는다.</summary>
    public Renderer NativeRenderer => _nativeRenderer ?? throw new InvalidOperationException("현재실제RootRenderer미설치");
    /// <summary>현재 실제 RawButtonControls/상위 UI의 지도 getter만 명시 연결한다.</summary>
    public void BindNativeMapView(Func<ScreenRect> mapView) { ArgumentNullException.ThrowIfNull(mapView); _nativeMapView = mapView; }
    /// <summary>실제 지도 장치가 준비된 호출에서만 현재 값을 읽는다.</summary>
    private ScreenRect NativeMapView() => (_nativeMapView ?? throw new InvalidOperationException("현재Root지도영역장치미설치"))();
    /// <summary>첫 메뉴/World 전에 한 번 생성/공개/Native 초기화/표시 getter 설치를 원문 순서로 수행한다.</summary>
    private void PrepareNativeRenderer()
    {
        if (_nativeRenderer is not null || _startup is null) return;
        _nativeRenderer = new Renderer(_driver.Screen.Width, _driver.Screen.Height);
        _nativeRendererDevices = new(_gameplay, _surfaceGlobals, _startup.Cloud, () => NativeRenderer, NativeMapView);
        _nativeRendererDevices.InitializeCamera(_driver.Screen.Width, _driver.Screen.Height); _nativeRendererDevices.BindSurfaceDisplay();
    }
    /// <summary>Source의 선택된 실제 효과만 받고 빈 커서도 실제 Prepare를 실행한다. 누락 효과는 선택 위치에서 실패한다.</summary>
    public void CancelNativeCursorForState(CursorLifecycleHooks? selectedEffects = null) => (_nativeRendererDevices ?? throw new InvalidOperationException("현재Root커서Renderer장치미설치")).CancelCursor(selectedEffects ?? new());
    /// <summary>실제 장치를 반납하기 전에 opaque 연결과 압축 해제 캐시를 해제한다.</summary>
    private void ReleaseNativeRenderer()
    {
        _startup?.Cloud.BindRenderer(null); _nativeRenderer?.ClearDecodedImages(); _nativeRendererDevices = null; _nativeRenderer = null; _nativeMapView = null;
    }
}
