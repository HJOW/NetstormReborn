using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 세션 카메라 조립과 실제 입력 경로의 Pan·저장 카메라 진입을 검증한다.
// 2026-11-26 1.050에서 DI-4d 카메라 결선용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>세션 카메라 장치 조립·실행과 메뉴 진입 결선을 같은 세션에서 대조한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>미조립 세션의 실행자는 거부·거짓이며 조립은 저장 초기화와 잠금을 잇는다.</summary>
    [Fact]
    public void CameraDevicesRequireBindingAndInitializeSavedPositions()
    {
        var (session, _) = Scene();
        Assert.False(session.ProcessCameraPan(new(0, 500, 300)));
        Assert.False(session.DispatchSavedCameraKey(new('5', 0, 0)));
        Assert.Throws<InvalidOperationException>(() => session.CommitCameraNow());
        Assert.Throws<InvalidOperationException>(() => session.PresentCameraNow(0));
        session.BuildCameraDevices();
        Assert.All(session.Gameplay.Camera.SavedPositions.Points, point => Assert.Equal(new ConstructionNoticePoint(-10000f, -10000f), point));
        Assert.Throws<InvalidOperationException>(() => session.BuildCameraDevices());
    }

    /// <summary>주입 갈고리로 Commit·Present 실행자가 같은 본체에 닿는다.</summary>
    [Fact]
    public void InjectedHooksDriveCommitAndPresentRunners()
    {
        var (session, _) = Scene();
        var calls = new List<string>();
        var dirtied = new List<ScreenRect>();
        var commit = new RawCameraCommitHooks
        {
            LockCursor = skip => calls.Add("L" + skip),
            UnlockCursor = () => calls.Add("U"),
            RedrawRoots = () => calls.Add("R"),
            Scroll = (x, y, w, h, dx, dy) => calls.Add($"S{x},{y},{w},{h},{dx},{dy}"),
            AddDirty = (rect, flags) => { calls.Add("D"); dirtied.Add(rect); },
            UpdateMouse = point => calls.Add($"M{point.X},{point.Y}"),
        };
        var present = new RawCameraPresentHooks { Flip = () => calls.Add("F") };
        session.BuildCameraDevices(commit, present);
        session.CameraViewport = new(0, 15, 1024, 768, new(0, 0, 1024, 768), new(0, 0, 1024, 768), new(5, 6));
        var camera = session.Gameplay.Camera;
        camera.InitializeNativeRenderer(1024, 768, 256, new(), session.Gameplay.MapRevision);
        // 목표를 현재와 다르게 두어 이동 분기를 탄다.
        camera.Target.Pixels[0] = 40; camera.Target.Pixels[1] = 15;
        session.CommitCameraNow();
        Assert.Equal(new[] { 40, 15 }, camera.RendererView.Pixels);
        Assert.Equal(1u, session.CameraRedraw.Value);
        Assert.Equal(new[] { "L0", "R", "S0,0,1024,768,40,15", "M5,6", "D", "D", "U", "R" }, calls);
        Assert.Equal(new[] { new ScreenRect(984, 0, 1024, 768), new ScreenRect(0, 753, 1024, 768) }, dirtied);
        // Present는 Renderer 한계·세계 한계를 순서대로 적용한 뒤 같은 Commit에 닿는다.
        calls.Clear();
        camera.Target.Pixels[0] = 5000; camera.Target.Pixels[1] = -50;
        session.PresentCameraNow(0);
        Assert.Equal(new[] { 3072, 0 }, camera.Target.Pixels);
        Assert.Equal(new[] { 3072, 0 }, camera.RendererView.Pixels);
        Assert.Contains("F", calls);
        // 진단 1은 뒤집기·적용 없이 목표만 둔다.
        calls.Clear();
        session.PresentCameraNow(1);
        Assert.Empty(calls);
    }

    /// <summary>주입 갈고리로 Pan·저장 실행자가 같은 본체에 닿는다.</summary>
    [Fact]
    public void InjectedHooksDrivePanAndSavedRunners()
    {
        var (session, _) = Scene();
        var requests = new List<string>();
        var pan = new CameraPanHooks
        {
            Poll = _ => new((uint)InputCode.Release, 0, 0),
            Globals = () => new() { ScrollWhileMouseOut = 0, Dialog = 0, Active = 1, ClientWidth = 1024, ClientHeight = 768, ViewX = 0, ViewY = 0, ViewWidth = 1024, ViewHeight = 768 },
            ClosePopups = () => requests.Add("K"),
            Request = (dx, dy) => requests.Add($"R{dx},{dy}"),
        };
        var saved = new SavedCameraKeyHooks
        {
            Paused = () => false,
            Language = () => 0,
            Sound = () => requests.Add("S"),
            Capture = () => new(12.5f, 30.25f),
            Center = (x, y) => requests.Add($"C{x},{y}"),
        };
        session.BuildCameraDevices(pan: pan, saved: saved);
        Assert.True(session.ProcessCameraPan(new(0, 512, 384)));
        Assert.Contains("K", requests);
        Assert.True(requests.Exists(item => item.StartsWith("R", StringComparison.Ordinal)));
        Assert.True(session.DispatchSavedCameraKey(new('%' | InputCode.Shift, 0, 0)));
        Assert.Equal(new ConstructionNoticePoint(12.5f, 30.25f), session.Gameplay.Camera.SavedPositions.Points[4]);
        Assert.True(session.DispatchSavedCameraKey(new('5', 0, 0)));
        Assert.Contains("C12.5,30.25", requests);
    }

    /// <summary>제품 결선처럼 묶은 메뉴 진입이 미조립이면 종전 흐름을, 조립이면 세션 실행자에 닿는다.</summary>
    [Fact]
    public void MenuEntriesReachSessionRunnersLikeProductWiring()
    {
        var (session, _) = Scene();
        var (_, _, router, changes) = CursorInputMenuScene();
        // 제품 진입 경로와 같은 결선으로 미션 세션 실행자에 맡긴다. 미조립이면 거짓으로 종전 흐름을 유지한다.
        router.CameraPan = pointer => session.ProcessCameraPan(pointer);
        router.SavedCameraKey = input => session.DispatchSavedCameraKey(input);
        int before = changes();
        Assert.False(router.Dispatch(new('5', 0, 0)));
        Assert.Equal(before, changes());
        Assert.NotNull(router.NotifyPointer(new(InputCode.Release, 100, 200)));
        // 저장 카메라를 조립하면 숫자 입력이 소비되고 Pan 이동 경로가 overlay를 건너뛴다.
        var saved = new SavedCameraKeyHooks
        {
            Paused = () => false,
            Language = () => 0,
            Sound = () => { },
            Capture = () => new(1f, 2f),
            Center = (_, _) => { },
        };
        var pan = new CameraPanHooks
        {
            Poll = _ => new((uint)InputCode.Release, 0, 0),
            Globals = () => new() { ScrollWhileMouseOut = 1, Dialog = 0, Active = 1, ClientWidth = 1024, ClientHeight = 768, ViewX = 0, ViewY = 0, ViewWidth = 1024, ViewHeight = 768 },
            ClosePopups = () => { },
            Request = (_, _) => { },
        };
        session.BuildCameraDevices(pan: pan, saved: saved);
        Assert.False(router.Dispatch(new((uint)'%' | InputCode.Shift, 0, 0)));
        Assert.True(changes() > before);
        Assert.Equal(new ConstructionNoticePoint(1f, 2f), session.Gameplay.Camera.SavedPositions.Points[4]);
        Assert.Null(router.NotifyPointer(new(0, 512, 384)));
    }
}
