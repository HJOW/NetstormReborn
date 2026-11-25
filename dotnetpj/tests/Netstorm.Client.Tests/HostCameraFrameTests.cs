using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 제품 루프와 같은 지도 영역 전역으로 카메라 목표·현재값을 적용한다.
// 2026-11-26 1.062에서 R5-1용으로 추가했다.
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>실제 지도 영역 전역과 기록 갈고리로 세션 카메라를 조립한다.</summary>
    private static (HostSession Session, List<string> Calls, List<ScreenRect> Dirtied) CameraFrameScene(int targetX, int targetY)
    {
        var (session, types) = Scene();
        session.BuildPlayerWorldHooks();
        var calls = new List<string>();
        var dirtied = new List<ScreenRect>();
        session.BuildCameraDevices(
            new RawCameraCommitHooks
            {
                Scroll = (x, y, width, height, dx, dy) => calls.Add($"S{x},{y},{width},{height},{dx},{dy}"),
                AddDirty = (rect, _) => { calls.Add("D"); dirtied.Add(rect); },
                RedrawRoots = () => calls.Add("R"),
            },
            new RawCameraPresentHooks { Flip = () => calls.Add("F") });
        // 전체 화면 대체 없이 실제 지도 영역을 쓴다.
        session.CameraViewport = new RawCameraCommitGlobals(0, 15, 1024, 768, new(10, 20, 800, 600), new(0, 0, 1024, 768), new(5, 6));
        session.Gameplay.Camera.InitializeNativeRenderer(1024, 768, 256, new(), session.Gameplay.MapRevision);
        session.Gameplay.Camera.Target.Pixels[0] = targetX; session.Gameplay.Camera.Target.Pixels[1] = targetY;
        return (session, calls, dirtied);
    }

    /// <summary>제품 루프와 같이 Present가 뒤집기·제한 뒤 같은 Commit에 닿는다.</summary>
    [Fact]
    public void MapViewportFrameAppliesPresentThenCommit()
    {
        var (session, calls, dirtied) = CameraFrameScene(40, 15);
        session.PresentCameraNow(0);
        Assert.Equal(["F", "R", "S10,20,800,600,40,15", "S0,0,10,768,0,0", "S10,600,1014,168,0,0", "D", "D", "R"], calls);
        Assert.Equal(new ScreenPoint(5, 6), new(session.Gameplay.Cursor.ScreenX, session.Gameplay.Cursor.ScreenY));
        Assert.Equal(new[] { new ScreenRect(770, 20, 810, 620), new ScreenRect(10, 605, 810, 620) }, dirtied);
        Assert.Equal(new[] { 40, 15 }, session.Gameplay.Camera.RendererView.Pixels);
        Assert.Equal(new[] { 40, 15 }, session.Gameplay.Camera.Snapshot.Pixels);
        Assert.Equal(1u, session.CameraRedraw.Value);
        Assert.Equal(0u, session.CameraWorldRedraw.Value);
        // 진단 1은 뒤집기·적용 없이 목표만 둔다.
        calls.Clear();
        session.Gameplay.Camera.Target.Pixels[0] = 60;
        session.PresentCameraNow(1);
        Assert.Empty(calls);
        Assert.Equal(new[] { 40, 15 }, session.Gameplay.Camera.RendererView.Pixels);
    }

    /// <summary>지도 영역이 없으면 실제 영역 요구로 거부된다.</summary>
    [Fact]
    public void MissingViewportIsExplicit()
    {
        var (session, _) = Scene();
        session.BuildPlayerWorldHooks();
        session.BuildCameraDevices();
        var missing = Assert.Throws<InvalidOperationException>(() => session.CommitCameraNow());
        Assert.Contains("실제 지도 영역", missing.Message);
        Assert.Throws<InvalidOperationException>(() => session.PresentCameraNow(0));
    }

    /// <summary>정지 프레임은 픽셀만 다시 쓰고 마우스·dirty·전체를 건드리지 않는다.</summary>
    [Fact]
    public void StillFrameKeepsRedrawClear()
    {
        var (session, calls, dirtied) = CameraFrameScene(0, 0);
        session.CommitCameraNow();
        Assert.Equal(["S10,20,800,600,0,0", "S0,0,10,768,0,0", "S10,600,1014,168,0,0"], calls);
        Assert.Empty(dirtied);
        Assert.Equal(0u, session.CameraRedraw.Value);
    }
}
