using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

// 같은 dirty·지도 사각형의 월드 환산과 지형 행 제한을 현재 분수 카메라로 계산한다.
// 2026-11-26 1.063에서 R5-2a용으로 추가했다.
public sealed class RawRendererViewportTests
{
    /// <summary>원점 카메라는 화면 오프셋만으로 월드를 환산한다.</summary>
    [Fact]
    public void WorldRectKeepsFractionalOrigin()
    {
        var view = new RawCameraViewState();
        RawCameraView.Initialize(view);
        RenderWorldRect rect = RawRendererViewport.WorldRect(new(10, 20, 30, 40), view);
        Assert.Equal((float)((10 + 16) * 0.0625f), rect.Left);
        Assert.Equal((float)((20 + 11) * BitConverter.UInt32BitsToSingle(0x3dba2e8c)), rect.Top);
        Assert.Equal((float)((30 + 16) * 0.0625f), rect.Right);
        Assert.Equal((float)((40 + 11) * BitConverter.UInt32BitsToSingle(0x3dba2e8c)), rect.Bottom);
    }

    /// <summary>분수 원점은 픽셀 원점에서 재구성하지 않고 그대로 더한다.</summary>
    [Fact]
    public void WorldRectPreservesWorldBits()
    {
        var view = new RawCameraViewState();
        RawCameraView.Initialize(view);
        RawCameraView.SetWorld(view, 5.25f, -3.5f);
        RenderWorldRect rect = RawRendererViewport.WorldRect(new(0, 0, 16, 11), view);
        Assert.Equal(1f + 5.25f, rect.Left, 5);
        Assert.Equal(1f - 3.5f, rect.Top, 5);
        Assert.Equal(2f + 5.25f, rect.Right, 5);
        Assert.Equal(2f - 3.5f, rect.Bottom, 5);
    }

    /// <summary>좌표별 제한 순서를 유지한다.</summary>
    [Fact]
    public void ClipWorldClampsEachAxis()
    {
        var clipped = RawRendererViewport.ClipWorld(new(-5, -6, 100, 200), new(0, 0, 10, 20));
        Assert.Equal(new RenderWorldRect(0, 0, 10, 20), clipped);
    }

    /// <summary>수집 viewport는 dirty 환산을 지도 환산 안에 제한한다.</summary>
    [Fact]
    public void CollectionViewportClipsDirtyToMap()
    {
        var view = new RawCameraViewState();
        RawCameraView.Initialize(view);
        RenderWorldRect viewport = RawRendererViewport.CollectionViewport(new(0, 0, 1024, 768), new(10, 20, 800, 600), view);
        RenderWorldRect map = RawRendererViewport.WorldRect(new(10, 20, 800, 600), view);
        Assert.Equal(map, viewport);
    }

    /// <summary>행 제한은 원문 32비트 저장 뒤 signed 나눗셈 순서다.</summary>
    [Fact]
    public void CollectionRowLimitKeepsIntegerOrder()
    {
        Assert.Equal((1024 / 16 * (768 + 10)) / 11, RawRendererViewport.CollectionRowLimit(1024, 768));
        Assert.Equal((800 / 16 * (600 + 10)) / 11, RawRendererViewport.CollectionRowLimit(800, 600));
    }
}
