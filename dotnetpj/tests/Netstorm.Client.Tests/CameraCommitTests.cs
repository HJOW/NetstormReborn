using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 497960 카메라 Commit 본체의 배율·이동·정지 분기를 실제 상태와 대조한다.
// 2026-11-26 1.047에서 DI-4a 카메라 Commit용으로 추가했다.
public sealed class CameraCommitTests
{
    /// <summary>같은 초기 view·snapshot·목표와 기록 갈고리를 묶는다.</summary>
    /// <returns>상태·전역·갈고리와 호출 기록이다.</returns>
    private static (RawCameraCommitState State, RawCameraCommitGlobals Globals, RawCameraCommitHooks Hooks, List<string> Calls, List<ScreenRect> Dirtied) Scene(
        int viewX, int viewY, int targetX, int targetY, uint zoom, uint screenFlags, int denominator, List<DirtyRegion>? dirty = null)
    {
        var view = new RawCameraViewState(); RawCameraView.Initialize(view);
        var snapshot = new RawCameraViewState(); RawCameraView.Initialize(snapshot);
        RawCameraView.SetPixels(view, viewX, viewY); RawCameraView.SetPixels(snapshot, viewX, viewY);
        var target = new RawCameraTargetState(); target.Pixels[0] = targetX; target.Pixels[1] = targetY; target.Zoom = zoom;
        var redraw = new UIntBox(); var worldRedraw = new UIntBox();
        var entries = dirty ?? [];
        var state = new RawCameraCommitState(view, snapshot, target, redraw, worldRedraw, entries);
        var globals = new RawCameraCommitGlobals(screenFlags, denominator, 1024, 768, new(0, 0, 1024, 768), new(0, 0, 1024, 768), new(5, 6));
        var calls = new List<string>(); var dirtied = new List<ScreenRect>();
        var hooks = new RawCameraCommitHooks
        {
            LockCursor = skip => calls.Add("L" + skip),
            UnlockCursor = () => calls.Add("U"),
            RedrawRoots = () => calls.Add("R"),
            Scroll = (x, y, w, h, dx, dy) => calls.Add($"S{x},{y},{w},{h},{dx},{dy}"),
            ScrollBackground = (x, y, w, h, dx, dy) => calls.Add($"B{x},{y},{w},{h},{dx},{dy}"),
            AddDirty = (rect, flags) => { calls.Add($"D{flags}"); dirtied.Add(rect); },
            AddBackgroundDirty = rect => { calls.Add("G"); dirtied.Add(rect); },
            UpdateMouse = point => calls.Add($"M{point.X},{point.Y}"),
        };
        return (state, globals, hooks, calls, dirtied);
    }

    /// <summary>배율 분기는 view·snapshot의 칸 크기와 float 비트를 같이 바꾸고 전체 다시 그리기를 둔다.</summary>
    [Fact]
    public void ZoomChangeAppliesScaleToViewAndSnapshot()
    {
        var (state, globals, hooks, calls, _) = Scene(0, 0, 64, 44, 0x20000, 0, 15);
        RawCameraCommit.CommitCameraTarget(state, globals, hooks);
        Assert.Equal(new[] { "R", "R" }, calls);
        Assert.Equal(1u, state.WorldRedraw.Value);
        Assert.Equal(0u, state.Redraw.Value);
        Assert.Equal(new[] { 32, 22 }, state.Renderer.CellPixels);
        Assert.Equal(0x20000u, state.Renderer.Zoom);
        Assert.Equal(BitConverter.SingleToUInt32Bits(2f), state.Renderer.ZoomFloatBits);
        Assert.Equal(BitConverter.SingleToUInt32Bits(32f), state.Renderer.CellFloatBits[0]);
        Assert.Equal(BitConverter.SingleToUInt32Bits(22f), state.Renderer.CellFloatBits[1]);
        Assert.Equal(state.Renderer.CellPixels, state.Snapshot.CellPixels);
        Assert.Equal(state.Renderer.Zoom, state.Snapshot.Zoom);
        Assert.Equal(new[] { 64, 44 }, state.Renderer.Pixels);
        Assert.Equal(new[] { 64, 44 }, state.Snapshot.Pixels);
    }

    /// <summary>이동 분기는 화면을 밀고 dirty를 옮기거나 합치며 노출 띠를 더한다.</summary>
    [Fact]
    public void MoveScrollsShiftsDirtyAndNotifies()
    {
        var dirty = new List<DirtyRegion>
        {
            new(new(10, 10, 50, 60), 0),
            new(new(0, 0, 100, 100), 2),
            new(new(5, 5, 9, 9), 1),
        };
        var (state, globals, hooks, calls, dirtied) = Scene(100, 80, 140, 95, 0x10000, 0, 15, dirty);
        RawCameraCommit.CommitCameraTarget(state, globals, hooks);
        Assert.Equal(new[] { "L0", "R", "S0,0,1024,768,40,15", "M5,6", "D0", "D0", "U", "R" }, calls);
        Assert.Equal(new ScreenRect(-30, -5, 10, 45), state.Dirty[0].Rect);
        Assert.Equal(new ScreenRect(-40, -15, 100, 100), state.Dirty[1].Rect);
        Assert.Equal(new ScreenRect(5, 5, 9, 9), state.Dirty[2].Rect);
        Assert.Equal(new[] { new ScreenRect(984, 0, 1024, 768), new ScreenRect(0, 753, 1024, 768) }, dirtied);
        Assert.Equal(1u, state.Redraw.Value);
        Assert.Equal(new[] { 140, 95 }, state.Renderer.Pixels);
        Assert.Equal(new[] { 140, 95 }, state.Snapshot.Pixels);
    }

    /// <summary>정지 분기는 픽셀만 다시 쓰고 마우스·dirty·전체 다시 그리기를 건드리지 않는다.</summary>
    [Fact]
    public void StillFrameSetsPixelsWithoutMouseOrDirtyShift()
    {
        var dirty = new List<DirtyRegion> { new(new(10, 10, 50, 60), 0) };
        var (state, globals, hooks, calls, dirtied) = Scene(100, 80, 100, 80, 0x10000, 0, 15, dirty);
        RawCameraCommit.CommitCameraTarget(state, globals, hooks);
        Assert.Equal(new[] { "S0,0,1024,768,0,0" }, calls);
        Assert.Empty(dirtied);
        Assert.Equal(new ScreenRect(10, 10, 50, 60), state.Dirty[0].Rect);
        Assert.Equal(0u, state.Redraw.Value);
        Assert.Equal(RawCameraView.CameraFromPixels(100, 80).X, state.Renderer.WorldBits[0]);
    }

    /// <summary>시차 분모 0은 원문 IDIV 오류로 거부되며 선행 쓰기를 되돌리지 않는다.</summary>
    [Fact]
    public void ParallaxZeroDenominatorIsRejected()
    {
        var (state, globals, hooks, _, _) = Scene(10, 10, 20, 20, 0x10000, 16, 0);
        Assert.Throws<DivideByZeroException>(() => RawCameraCommit.CommitCameraTarget(state, globals, hooks));
    }
}
