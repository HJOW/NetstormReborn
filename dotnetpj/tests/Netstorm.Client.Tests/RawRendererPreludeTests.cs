using Netstorm.Client.Display;

namespace Netstorm.Client.Tests;

// 화면 flags/override/구름 행 스캔과 전체 dirty 준비 순서를 대조한다.
// 2026-11-26 1.067에서 R5-2d2용으로 추가했다.
public sealed class RawRendererPreludeTests
{
    private sealed class Scene
    {
        public Scene()
        {
            WidthBox = [1024]; HeightBox = [768]; InitialBox = [768]; CameraYBox = [100];
            LeftBox = [0]; TopBox = [0]; RightBox = [64]; BottomBox = [64];
            Globals = new RendererPreludeGlobals
            {
                Width = new(WidthBox, 0), Height = new(HeightBox, 0), InitialHeight = new(InitialBox, 0),
                CameraY = new(CameraYBox, 0),
                Left = new(LeftBox, 0), Top = new(TopBox, 0), Right = new(RightBox, 0), Bottom = new(BottomBox, 0),
            };
        }

        public int[] WidthBox { get; }
        public int[] HeightBox { get; }
        public int[] InitialBox { get; }
        public int[] CameraYBox { get; }
        public int[] LeftBox { get; }
        public int[] TopBox { get; }
        public int[] RightBox { get; }
        public int[] BottomBox { get; }
        public RendererPreludeGlobals Globals { get; }
    }

    /// <summary>모드가 바뀌면 구름 dirty를 표시하고 이전 모드를 갱신한다.</summary>
    [Fact]
    public void ModeChangeQueuesFull()
    {
        var scene = new Scene();
        var calls = new List<string>();
        var state = new RendererPreludeState(new uint[19]);
        var hooks = new RendererPreludeHooks
        {
            MarkCloudDirty = () => calls.Add("M"),
            QueueCloudDirty = rect => calls.Add($"Q{rect.Left},{rect.Top},{rect.Right},{rect.Bottom}"),
            OverrideActive = () => 1,
        };
        scene.Globals.Mode = 1;
        RawRendererPrelude.PrepareRendererFrame(state, scene.Globals, hooks);
        Assert.Equal(["M", "Q0,0,1024,768"], calls);
        Assert.Equal(1u, state.Scan[18]);
        Assert.Equal(0u, state.DrawCounter);
    }

    /// <summary>세계 요청은 전체를 표시하고 마지막에 전체 dirty를 추가한다.</summary>
    [Fact]
    public void ForceRedrawInvalidatesFull()
    {
        var scene = new Scene();
        var calls = new List<string>();
        var state = new RendererPreludeState(new uint[19]);
        var hooks = new RendererPreludeHooks
        {
            MarkCloudDirty = () => calls.Add("M"),
            QueueCloudDirty = rect => calls.Add($"Q{rect.Right}"),
            OverrideActive = () => 1,
            ClearDirty = () => calls.Add("C"),
            Invalidate = (rect, flags) => calls.Add($"I{rect.Right},{flags}"),
        };
        scene.Globals.Mode = 0;
        scene.Globals.Force = 2;
        state.DirtyCount = 9;
        RawRendererPrelude.PrepareRendererFrame(state, scene.Globals, hooks);
        Assert.Equal(["M", "Q1024", "C", "I1024,0"], calls);
        Assert.Equal(0u, state.DirtyCount);
        Assert.Equal(0u, state.DrawCounter);
    }

    /// <summary>구름 행 스캔은 현재 30행 구간의 첫 dirty에서 멈춘다.</summary>
    [Fact]
    public void CloudRowScanInvalidatesFirstDirty()
    {
        var scene = new Scene();
        var calls = new List<string>();
        var scan = new uint[19];
        scan[17] = 1;
        scan[18] = 40;
        scan[0] = 0;
        scan[1] = 100;
        var state = new RendererPreludeState(scan);
        var hooks = new RendererPreludeHooks
        {
            MarkCloudDirty = () => calls.Add("M"),
            QueueCloudDirty = rect => calls.Add("Q"),
            OverrideActive = () => 0,
            UpdateClouds = () => calls.Add("U"),
            RowDirty = (row, cameraY) => { calls.Add($"R{row},{cameraY}"); return (uint)(row == 100 ? 1 : 0); },
            Invalidate = (rect, flags) => calls.Add($"I{rect.Top},{rect.Bottom}"),
        };
        scene.Globals.Mode = 40;
        RawRendererPrelude.PrepareRendererFrame(state, scene.Globals, hooks);
        Assert.Equal(["U", "R100,100", "I64,64", "M"], calls);
        Assert.Equal(1u, scan[0]);
        Assert.Equal(0u, state.DrawCounter);
    }

    /// <summary>비DIB 전체화면 시차는 별도 배경 dirty를 처리한다.</summary>
    [Fact]
    public void ParallaxFullscreenRefreshesBackground()
    {
        var scene = new Scene();
        var calls = new List<string>();
        var state = new RendererPreludeState(new uint[19]);
        state.Scan[18] = 25;
        var hooks = new RendererPreludeHooks
        {
            MarkCloudDirty = () => calls.Add("M"),
            QueueCloudDirty = rect => calls.Add("Q"),
            OverrideActive = () => 0,
            InvalidateBackground = () => calls.Add("B"),
        };
        scene.Globals.Mode = 25;
        RawRendererPrelude.PrepareRendererFrame(state, scene.Globals, hooks);
        Assert.Equal(["M", "B"], calls);
        Assert.Equal(1u, state.BackgroundRedraw);
    }

    /// <summary>override가 있으면 구름 블록을 건너뛴다.</summary>
    [Fact]
    public void OverrideSkipsCloudBlock()
    {
        var scene = new Scene();
        var state = new RendererPreludeState(new uint[19]);
        state.Scan[18] = 40;
        var hooks = new RendererPreludeHooks { OverrideActive = () => 3 };
        scene.Globals.Mode = 40;
        RawRendererPrelude.PrepareRendererFrame(state, scene.Globals, hooks);
        Assert.Equal(0u, state.DrawCounter);
        Assert.Equal(0u, state.BackgroundRedraw);
    }
}
