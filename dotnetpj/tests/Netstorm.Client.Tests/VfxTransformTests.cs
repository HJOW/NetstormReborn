using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 원본 VFX 변환(<see cref="VfxTransform.DrawScaledVfxImage"/>) 계약을 고정한다.
/// 근거: cpppj/src/client/VFXTransform.cpp (원본 004028cb의 angle=0 분기). 대조 fixture가 없어 L1이 아니며 R5에서 끝까지 대조한다. (R2d-1, 2026-10-28)
/// </summary>
public sealed class VfxTransformTests
{
    /// <summary>배율 1 그림을 1픽셀로 만든다. 프레임 영역과 기준점은 원본 호출 그대로 둔다.</summary>
    private static (IndexedImage Image, ShapeFrame Frame) SinglePixel(byte index)
    {
        var image = new IndexedImage { Width = 1, Height = 1, Indices = [index], Opacity = [255] };
        var frame = new ShapeFrame { Rect = new ShapeRect(0, 0, 0, 0) };
        return (image, frame);
    }

    /// <summary>배율 1은 일반 합성과 같은 픽셀을 낸다.</summary>
    [Fact]
    public void UnitScaleMatchesPlainDraw()
    {
        (IndexedImage image, ShapeFrame frame) = SinglePixel(11);
        var plain = new byte[16];
        var scaled = new byte[16];
        Array.Fill(plain, (byte)3);
        Array.Fill(scaled, (byte)3);
        var clip = new ScreenRect(0, 0, 4, 4);
        RendererDraw.DrawIndexedImage(plain, 4, 4, image, 1, 1, clip);
        VfxTransform.DrawScaledVfxImage(scaled, 4, 4, image, frame, new ScreenPoint(1, 1), 0x10000, 0x10000, clip);
        Assert.Equal(plain, scaled);
    }

    /// <summary>2배 확대는 원본 기울기 누적대로 샘플링한다(픽셀 중심 0x8000 시작·0x8000 간격).</summary>
    [Fact]
    public void DoubleScaleRepeatsPixels()
    {
        var image = new IndexedImage { Width = 2, Height = 2, Indices = [5, 6, 7, 8], Opacity = [255, 255, 255, 255] };
        var frame = new ShapeFrame { Rect = new ShapeRect(0, 0, 1, 1) };
        var canvas = new byte[3];
        VfxTransform.DrawScaledVfxImage(canvas, 3, 1, image, frame, new ScreenPoint(0, 0),
            0x20000, 0x10000, new ScreenRect(0, 0, 3, 1));
        Assert.Equal(new byte[] { 5, 6, 6 }, canvas);
    }

    /// <summary>색 치환 뒤 255는 임시 표면 규칙대로 기존 대상 픽셀을 유지한다.</summary>
    [Fact]
    public void MappedTransparentKeepsDestination()
    {
        (IndexedImage image, ShapeFrame frame) = SinglePixel(5);
        var colors = new byte[256];
        // 표 항목마다 번호를 그대로 둔다
        for (int i = 0; i < 256; i++)
        {
            colors[i] = (byte)i;
        }
        colors[5] = 255;
        var canvas = new byte[] { 9 };
        VfxTransform.DrawScaledVfxImage(canvas, 1, 1, image, frame, new ScreenPoint(0, 0),
            0x20000, 0x20000, new ScreenRect(0, 0, 1, 1), colors);
        Assert.Equal(new byte[] { 9 }, canvas);
    }

    /// <summary>위아래 끝이 같은 배율은 원본처럼 행을 그리지 않는다.</summary>
    [Fact]
    public void FlatHeightDrawsNothing()
    {
        (IndexedImage image, ShapeFrame frame) = SinglePixel(5);
        var canvas = new byte[] { 9 };
        VfxTransform.DrawScaledVfxImage(canvas, 1, 1, image, frame, new ScreenPoint(0, 0),
            0x10000, 0, new ScreenRect(0, 0, 1, 1));
        Assert.Equal(new byte[] { 9 }, canvas);
    }

    /// <summary>프레임 영역과 그림 크기가 다르면 거부한다.</summary>
    [Fact]
    public void FrameSizeMismatchRejected()
    {
        (IndexedImage image, _) = SinglePixel(5);
        var frame = new ShapeFrame { Rect = new ShapeRect(0, 0, 3, 3) };
        var canvas = new byte[16];
        Assert.Throws<ArgumentException>(() =>
            VfxTransform.DrawScaledVfxImage(canvas, 4, 4, image, frame, new ScreenPoint(0, 0),
                0x10000, 0x10000, new ScreenRect(0, 0, 4, 4)));
    }
}
