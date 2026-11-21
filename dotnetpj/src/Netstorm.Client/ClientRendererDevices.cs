using Netstorm.Client.Display;
using Netstorm.Client.Input;
using Netstorm.Core.Display;
using Netstorm.Core.Original;

// 짧은 Frame/Session 대신 같은 Client의 현재 Renderer/지도 getter만 보관한다.
namespace Netstorm.Client;

/// <summary>ClientRendererFrame.BindSurfaceDisplay와 실제 커서 Prepare 장치의 소유자 연결 부분이다.</summary>
public sealed class ClientRendererDevices
{
    /// <summary>실제 카메라/커서/checking의 단일 소유자다.</summary>
    private readonly ClientGameplayGlobals _game;
    /// <summary>같은 실제 물리와 기본 표시 장치를 보유한다.</summary>
    private readonly ClientSurfaceGlobals _surface;
    /// <summary>실제 opaque 토큰과 같은0059a8b0 소유자다.</summary>
    private readonly ClientKencloudGlobals _cloud;
    /// <summary>장치 교체 뒤에도 호출 위치에서 현재 Renderer를 조회한다.</summary>
    private readonly Func<Renderer> _renderer;
    /// <summary>현재 Root 지도 영역만 조회하며 화면 전체 기본값을 만들지 않는다.</summary>
    private readonly Func<ScreenRect> _mapView;
    /// <summary>참조/함수만 보관하며 카메라/장치/지도/Frame을 조회하거나 초기화하지 않는다.</summary>
    public ClientRendererDevices(ClientGameplayGlobals game, ClientSurfaceGlobals surface, ClientKencloudGlobals cloud, Func<Renderer> renderer, Func<ScreenRect> mapView)
    {
        ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(cloud); ArgumentNullException.ThrowIfNull(renderer); ArgumentNullException.ThrowIfNull(mapView);
        if (!ReferenceEquals(game, surface.Gameplay)) throw new ArgumentException("표시장치실제Gameplay불일치", nameof(surface)); _game = game; _surface = surface; _cloud = cloud; _renderer = renderer; _mapView = mapView;
    }
    /// <summary>도달한 소비 위치에서만 현재 실제 Renderer의 존재를 검사한다.</summary>
    private Renderer Renderer() => _renderer() ?? throw new InvalidOperationException("현재실제Renderer장치미설치");
    /// <summary>CPP의 Renderer 생성/공개 뒤 같은 실제 세계 크기/redraw/revision으로 명시 초기화한다.</summary>
    public void InitializeCamera(int width, int height)
    {
        _cloud.BindRenderer(Renderer()); _game.Camera.InitializeNativeRenderer(width, height, _game.WorldCells.Value, _cloud.WorldRedrawBox, _game.MapRevision);
    }
    /// <summary>첫 World 전에 현재 getter만 설치하며 지도/카메라/그리기/게임 진행을 실행하지 않는다.</summary>
    public void BindSurfaceDisplay()
    {
        _surface.Storage.BindDisplayDevices(new()
        {
            View = () => { ScreenRect viewport = _mapView(); var camera = _game.Camera.Snapshot; return new() { CameraX = camera.Pixels[0], CameraY = camera.Pixels[1], Zoom = unchecked((int)camera.Zoom), Viewport = new(viewport.Left, viewport.Top, viewport.Right, viewport.Bottom) }; },
            Suppressed = () => Renderer().FullRedrawPending,
            Invalidate = (rect, flags) => Renderer().Invalidate(new(rect.Left, rect.Top, rect.Right, rect.Bottom), flags),
        });
    }
    /// <summary>현재 snapshot 투영과 현재 Renderer dirty만 제공하며 Query/GUI를 성공으로 채우지 않는다.</summary>
    public CursorPlacementHooks CursorPrepareHooks() => new()
    {
        Project = (x, y) => RawCameraView.Project(_game.Camera.Snapshot, x, y),
        Invalidate = (rect, flags) => Renderer().Invalidate(rect, flags),
    };
    /// <summary>같은 실제 hash 물리와 현재 카메라 픽셀을 빌려 Source Cancel/Prepare를 실행한다.</summary>
    public void CancelCursor(CursorLifecycleHooks selectedEffects)
    {
        _game.CursorGlobals.Placement.SharedScreenOrigin = _game.Camera.Snapshot.Pixels;
        ClientCursorStateCleanup.Cancel(_surface.Storage.Pool, _surface.Storage.Types, _game.Cursor, _game.CursorGlobals, selectedEffects, CursorPrepareHooks(), _surface.Storage.Hash.BorrowEntries(0));
    }
}
