using Netstorm.Client.Display;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 원문 장기 값과 현재 실제 Renderer/지도 getter의 연결 위치를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>장치 설치/현재 조회/빈 취소/선택된 실패와 같은 물리 공유 검사다.</summary>
public sealed class ClientRendererDevicesTests
{
    /// <summary>창/파일/Session 없이 실제 물리와 두 현재 출력기를 조립한다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 카메라/커서 소유자다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>같은 실제 구름 토큰/redraw 소유자다.</summary>
        public readonly ClientKencloudGlobals Cloud = new();
        /// <summary>같은 실제 Pool/TYPE/표시 소유자다.</summary>
        public readonly ClientSurfaceGlobals Surface;
        /// <summary>첫 실제 출력기다.</summary>
        public readonly Renderer First = new(128, 96);
        /// <summary>교체/명시 대체 출력기다.</summary>
        public readonly Renderer Second = new(128, 96);
        /// <summary>현재 getter가 반환할 실제 장치다.</summary>
        public Renderer Current;
        /// <summary>현재 실제 지도 입력이다.</summary>
        public ScreenRect View = new(3, 4, 120, 90);
        /// <summary>명시 지도 미설치 입력이다.</summary>
        public bool MissingMap;
        /// <summary>각 소비자 getter의 실제 호출 수다.</summary>
        public int RendererReads, MapReads;
        /// <summary>같은 실제 장치 연결기다.</summary>
        public readonly ClientRendererDevices Devices;
        /// <summary>기존 물리/타입만 준비하며 현재 장치/카메라/시각은 조회하지 않는다.</summary>
        public Scene()
        {
            var clock = new ClientClockGlobals(new ClientLocalStateGlobals(Game, new())); Surface = new(Game, clock, new(), () => throw new IOException("예기치않은시각조회")); Current = First;
            Surface.Storage.Types.AddRange(Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()));
            Devices = new(Game, Surface, Cloud, () => { RendererReads++; return Current; }, () => { MapReads++; if (MissingMap) throw new IOException("지도미설치"); return View; });
        }
        /// <summary>실제 첫 그리기/출력을 마쳐 초기 full 상태만 소비한다.</summary>
        public static void Settle(Renderer renderer) { renderer.Draw(new byte[128 * 96], 128); renderer.Present(_ => { }); }
    }
    /// <summary>같은 장치는 조기 반환하고 교체/반납/재결합은 원문의 정확한 opaque32 순서를 사용한다.</summary>
    [Fact]
    public void CloudRendererBindingKeepsIdentityTokensAndRedrawAlias()
    {
        var cloud = new ClientKencloudGlobals(); var first = new Renderer(8, 4); var second = new Renderer(8, 4); Assert.Equal(0u, cloud.RendererToken); cloud.WorldRedraw = 73;
        cloud.BindRenderer(first); Assert.Equal(0x3b000000u, cloud.RendererToken); cloud.BindRenderer(first); Assert.Equal(0x3b000000u, cloud.RendererToken); cloud.BindRenderer(second); Assert.Equal(0x3b001000u, cloud.RendererToken); cloud.BindRenderer(null); Assert.Equal(0u, cloud.RendererToken); cloud.BindRenderer(first); Assert.Equal(0x3b002000u, cloud.RendererToken);
        Assert.Equal(73u, cloud.WorldRedrawBox.Value); cloud.WorldRedrawBox.Value = uint.MaxValue; Assert.Equal(uint.MaxValue, cloud.WorldRedraw); cloud.WorldRedraw = 7; Assert.Equal(7u, cloud.WorldRedrawBox.Value);
    }
    /// <summary>구성/표시 설치는 getter를 조회하지 않고 명시 초기화만 실제 세계 크기/redraw/revision을 쓴다.</summary>
    [Fact]
    public void BindingIsLazyAndInitializationUsesActualLongLivedWords()
    {
        var scene = new Scene(); scene.Devices.BindSurfaceDisplay(); Assert.Equal(0, scene.RendererReads); Assert.Equal(0, scene.MapReads); Assert.Equal(256, scene.Game.WorldCells.Value);
        scene.Game.WorldCells.Value = 257; scene.Game.MapRevision.Value = uint.MaxValue; scene.Cloud.WorldRedrawBox.Value = 0x12345678; scene.Devices.InitializeCamera(128, 96);
        Assert.Equal(1, scene.RendererReads); Assert.Equal(0, scene.MapReads); Assert.Equal(0x3b000000u, scene.Cloud.RendererToken); Assert.Equal(1u, scene.Cloud.WorldRedraw); Assert.Equal(0u, scene.Game.MapRevision.Value); Assert.Equal(new[] { 3984, 2731 }, scene.Game.Camera.Target.Limits); Assert.Equal(65536u, scene.Game.Camera.Snapshot.Zoom);
    }
    /// <summary>같은 장기 표면의 현재 뷰는 매 호출 현재 snapshot/지도 값을 읽으며 Session을 요구하지 않는다.</summary>
    [Fact]
    public void SurfaceViewReadsCurrentSnapshotAndActualMap()
    {
        var scene = new Scene(); scene.Devices.InitializeCamera(128, 96); scene.Devices.BindSurfaceDisplay(); RawCameraView.SetWorld(scene.Game.Camera.Snapshot, 5, 5);
        SquidDisplayView view = scene.Surface.Storage.CurrentDisplayView(); Assert.Equal(80, view.CameraX); Assert.Equal(55, view.CameraY); Assert.Equal(65536, view.Zoom); Assert.Equal(3, view.Viewport.Left); Assert.Equal(90, view.Viewport.Bottom);
        scene.View = new(7, 8, 110, 80); RawCameraView.SetWorld(scene.Game.Camera.Snapshot, 6, 6); scene.Game.Camera.Snapshot.Zoom = 0x80000001; view = scene.Surface.Storage.CurrentDisplayView(); Assert.Equal(96, view.CameraX); Assert.Equal(66, view.CameraY); Assert.Equal(unchecked((int)0x80000001), view.Zoom); Assert.Equal(7, view.Viewport.Left); Assert.Equal(80, view.Viewport.Bottom); Assert.Equal(2, scene.MapReads); Assert.Throws<InvalidOperationException>(() => scene.Game.RequireSession());
    }
    /// <summary>첫메뉴 빈 Cancel도 같은 실제 Prepare를 실행하고 장치 교체 뒤 현재 출력기에 dirty를 보낸다.</summary>
    [Fact]
    public void EmptyCancelUsesCurrentRendererAndDoesNotCallOuterPreparation()
    {
        var scene = new Scene(); scene.Devices.InitializeCamera(128, 96); Scene.Settle(scene.First); Scene.Settle(scene.Second); RawCameraView.SetWorld(scene.Game.Camera.Snapshot, 5, 5); scene.Game.Cursor.X = scene.Game.Cursor.Y = 10;
        scene.Game.CursorGlobals.Placement.SharedScreenOrigin = new int[] { 999, 888 }; Action wrong = () => throw new IOException("바깥준비호출금지"); var effects = new CursorLifecycleHooks { PrepareCursor = wrong }; scene.Devices.CancelCursor(effects);
        Assert.Same(wrong, effects.PrepareCursor); Assert.Equal(80, scene.Game.CursorGlobals.Placement.CurrentOriginX()); Assert.Single(scene.First.CameraDirtyEntries); Assert.Equal(new ScreenRect(78, 53, 86, 69), scene.First.CameraDirtyEntries[0].Rect); Assert.Empty(scene.Second.CameraDirtyEntries);
        scene.Current = scene.Second; RawCameraView.SetWorld(scene.Game.Camera.Snapshot, 6, 6); scene.Devices.CancelCursor(new()); Assert.Single(scene.Second.CameraDirtyEntries); Assert.Equal(new ScreenRect(62, 42, 70, 58), scene.Second.CameraDirtyEntries[0].Rect); Assert.Equal(0, scene.MapReads); Assert.Equal(1u, scene.Game.Cursor.LockDepth);
    }
    /// <summary>미설치 지도는 실제 뷰 소비에서 실패하며 지도와 무관한 빈 Prepare를 막지 않는다.</summary>
    [Fact]
    public void MissingMapFailsAtViewConsumptionWithoutInventingViewport()
    {
        var scene = new Scene { MissingMap = true }; scene.Devices.InitializeCamera(128, 96); scene.Devices.BindSurfaceDisplay(); Scene.Settle(scene.First); scene.Devices.CancelCursor(new()); Assert.Equal(0, scene.MapReads);
        Assert.Equal("지도미설치", Assert.Throws<IOException>(() => scene.Surface.Storage.CurrentDisplayView()).Message); Assert.Equal(1, scene.MapReads);
    }
    /// <summary>실제 준비 뒤 선택된 미리보기 삭제 장치가 없으면 현재 목록/선행 dirty를 유지한다.</summary>
    [Fact]
    public void MissingSelectedDestroyKeepsNativePreparePrefix()
    {
        var scene = new Scene(); scene.Devices.InitializeCamera(128, 96); Scene.Settle(scene.First);
        // 실제 client 풀에서 미리보기 번호까지 같은 저장소를 확보한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, scene.Surface.Storage.Pool.Allocate(SidAllocation.Client));
        scene.Game.CursorGlobals.Lifecycle.PreviewCount = 1; scene.Game.CursorGlobals.Lifecycle.Previews[0] = 50;
        Assert.Throws<InvalidOperationException>(() => scene.Devices.CancelCursor(new())); Assert.Equal(1, scene.Game.CursorGlobals.Lifecycle.PreviewCount); Assert.Equal(50u, scene.Game.CursorGlobals.Lifecycle.Previews[0]); Assert.NotEmpty(scene.First.CameraDirtyEntries); Assert.Equal(1u, scene.Game.Cursor.LockDepth);
    }
}
