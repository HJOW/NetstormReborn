using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 원본 Renderer의 8비트 합성(<see cref="RendererDraw.DrawIndexedImage"/>) 계약을 고정한다.
/// 근거: cpppj/src/client/Renderer.cpp 의 DrawIndexedImage. 대조 fixture가 없어 L1이 아니며 R5에서 끝까지 대조한다. (R2d-1, 2026-10-28)
/// </summary>
public sealed class RendererDrawTests
{
    /// <summary>투명 마스크가 0인 픽셀은 기존 화면 값을 유지한다.</summary>
    [Fact]
    public void TransparentKeepsDestination()
    {
        var image = new IndexedImage { Width = 2, Height = 1, Indices = [7, 8], Opacity = [255, 0] };
        var canvas = new byte[] { 1, 2, 3, 4, 5, 6 };
        RendererDraw.DrawIndexedImage(canvas, 3, 2, image, 0, 0, new ScreenRect(0, 0, 3, 2));
        Assert.Equal(new byte[] { 7, 2, 3, 4, 5, 6 }, canvas);
    }

    /// <summary>색 표가 있으면 불투명 소스 번호를 표에서 변환한다.</summary>
    [Fact]
    public void ColorMapTranslatesSource()
    {
        var image = new IndexedImage { Width = 1, Height = 1, Indices = [10], Opacity = [255] };
        var canvas = new byte[] { 0 };
        var colors = new byte[256];
        // 표 항목마다 번호를 그대로 둔다
        for (int i = 0; i < 256; i++)
        {
            colors[i] = (byte)i;
        }
        colors[10] = 42;
        RendererDraw.DrawIndexedImage(canvas, 1, 1, image, 0, 0, new ScreenRect(0, 0, 1, 1), colors);
        Assert.Equal(new byte[] { 42 }, canvas);
    }

    /// <summary>그림자는 소스 색 대신 대상 화면 색을 표에서 변환한다.</summary>
    [Fact]
    public void ShadowTranslatesDestination()
    {
        var image = new IndexedImage { Width = 1, Height = 1, Indices = [10], Opacity = [255] };
        var canvas = new byte[] { 20 };
        var colors = new byte[256];
        // 표 항목마다 번호를 그대로 둔다
        for (int i = 0; i < 256; i++)
        {
            colors[i] = (byte)i;
        }
        colors[20] = 33;
        RendererDraw.DrawIndexedImage(canvas, 1, 1, image, 0, 0, new ScreenRect(0, 0, 1, 1), colors, shadow: true);
        Assert.Equal(new byte[] { 33 }, canvas);
    }

    /// <summary>그림자는 표 없이 거부한다.</summary>
    [Fact]
    public void ShadowWithoutMapRejected()
    {
        var image = new IndexedImage { Width = 1, Height = 1, Indices = [1], Opacity = [255] };
        var canvas = new byte[] { 0 };
        Assert.Throws<ArgumentException>(() =>
            RendererDraw.DrawIndexedImage(canvas, 1, 1, image, 0, 0, new ScreenRect(0, 0, 1, 1), null, shadow: true));
    }

    /// <summary>클립 밖 그림은 화면을 바꾸지 않는다.</summary>
    [Fact]
    public void OutsideClipKeepsCanvas()
    {
        var image = new IndexedImage { Width = 1, Height = 1, Indices = [9], Opacity = [255] };
        var canvas = new byte[] { 4, 5, 6, 7 };
        RendererDraw.DrawIndexedImage(canvas, 2, 2, image, 5, 5, new ScreenRect(0, 0, 2, 2));
        Assert.Equal(new byte[] { 4, 5, 6, 7 }, canvas);
    }
}
