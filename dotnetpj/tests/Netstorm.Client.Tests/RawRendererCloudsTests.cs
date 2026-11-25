using Netstorm.Client.Display;

namespace Netstorm.Client.Tests;

// 구름 4칸 저장·시차 갱신·배경 선택을 대조한다.
// 2026-11-26 1.066에서 R5-2d1용으로 추가했다.
public sealed class RawRendererCloudsTests
{
    private static (RendererCloudQueue Queue, RendererCloudHooks Hooks, List<string> Calls) Scene()
    {
        var calls = new List<string>();
        var hooks = new RendererCloudHooks
        {
            Assertion = line => calls.Add($"A{line:x}"),
            Clear = (rect, color) => calls.Add($"C{rect.Left},{rect.Top},{rect.Right},{rect.Bottom},{color}"),
            Draw = (rect, x, y, alternate) => calls.Add($"D{rect.Left},{rect.Top},{rect.Right},{rect.Bottom},{x},{y},{alternate}"),
        };
        return (new RendererCloudQueue(), hooks, calls);
    }

    /// <summary>묶음 숨김은 검정으로 지우고 UI 전용·덮임보다 먼저 실행한다.</summary>
    [Fact]
    public void HideGroupsClearsBlackFirst()
    {
        var (queue, hooks, calls) = Scene();
        var globals = new RendererBackdropGlobals { HideGroups = 1, Black = 7, Mode = 0 };
        RawRendererClouds.DrawRendererBackdrop(new(1, 2, 30, 40), 4, 1, globals, hooks);
        Assert.Equal(["C1,2,30,40,7"], calls);
        Assert.Equal(0u, queue.Count.Value);
    }

    /// <summary>구름이 없으면 날씨색으로 지운다.</summary>
    [Fact]
    public void NoCloudClearsTint()
    {
        var (_, hooks, calls) = Scene();
        var globals = new RendererBackdropGlobals { Tint = 5, Mode = 0 };
        RawRendererClouds.DrawRendererBackdrop(new(0, 0, 10, 10), 0, 0, globals, hooks);
        Assert.Equal(["C0,0,10,10,5"], calls);
    }

    /// <summary>비DIB 전체화면 시차는 흰색으로 지운다.</summary>
    [Fact]
    public void FullscreenParallaxClearsWhite()
    {
        var (_, hooks, calls) = Scene();
        var globals = new RendererBackdropGlobals { Mode = 0x19, Tint = 5, Black = 7 };
        RawRendererClouds.DrawRendererBackdrop(new(0, 0, 10, 10), 0, 0, globals, hooks);
        Assert.Equal(["C0,0,10,10,255"], calls);
    }

    /// <summary>시차 구름은 Y→X 순서로 나눈 원점으로 그린다.</summary>
    [Fact]
    public void ParallaxDrawsDividedOrigin()
    {
        var (_, hooks, calls) = Scene();
        var globals = new RendererBackdropGlobals { Mode = 0x18, CameraX = 40, CameraY = 22, Denominator = 15 };
        RawRendererClouds.DrawRendererBackdrop(new(2, 3, 12, 13), 0, 0, globals, hooks);
        Assert.Equal(["D2,3,12,13,2,1,False"], calls);
        // 분모 0은 가짜 0 성공으로 바꾸지 않는다
        var bad = new RendererBackdropGlobals { Mode = 0x18, Denominator = 0 };
        Assert.Throws<DivideByZeroException>(() => RawRendererClouds.DrawRendererBackdrop(new(0, 0, 1, 1), 0, 0, bad, hooks));
    }

    /// <summary>4칸 저장·진단·시차 갱신 뒤 count를 비운다.</summary>
    [Fact]
    public void QueueAppendsAndRefreshes()
    {
        var (queue, hooks, calls) = Scene();
        RawRendererClouds.AppendRendererCloudDirty(queue, new(1, 1, 2, 2), 0, hooks);
        RawRendererClouds.AppendRendererCloudDirty(queue, new(3, 3, 4, 4), 1, hooks);
        Assert.Equal(2u, queue.Count.Value);
        // 진단 DWORD가 켜 있고 4칸이 차면 진단 행을 요구한다
        queue.Count.Value = 4;
        var asserted = Assert.Throws<InvalidOperationException>(() =>
            RawRendererClouds.AppendRendererCloudDirty(queue, new(0, 0, 1, 1), 1, new RendererCloudHooks()));
        Assert.Contains("구름진단장치", asserted.Message);
        queue.Count.Value = 2;
        RawRendererClouds.RefreshRendererCloudBackground(queue, 30, 45, 15, hooks);
        Assert.Equal(["D1,1,2,2,2,3,True", "D3,3,4,4,2,3,True"], calls);
        Assert.Equal(0u, queue.Count.Value);
    }
}
