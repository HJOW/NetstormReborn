using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 원본 동적 이미지(<see cref="RawImage"/>) 계약을 고정한다.
/// 근거: cpppj/src/client/RawImage.cpp (원본 0046d790·0046d900·0046d8a0). 대조 fixture가 없어 L1이 아니며 R4에서 끝까지 대조한다. (R2d-1, 2026-10-28)
/// </summary>
public sealed class RawImageTests
{
    /// <summary>빈 그림은 원문 sentinel 그대로 40바이트 shape를 남긴다.</summary>
    [Fact]
    public void EmptyBitmapKeepsFortyBytes()
    {
        var image = new RawImage();
        image.Convert(new TerrainBitmapInput(2, 2, [0, 0, 0, 0]), crop: false);
        Assert.Equal(2, image.Width());
        Assert.Equal(2, image.Height());
        Assert.Equal(40u, image.ShapeSize());
        Assert.Equal(40, image.Shape().Length);
        Assert.True(image.Bits().IsEmpty);
    }

    /// <summary>전부 0이면 crop도 원문과 같은 최소 1x1을 남긴다.</summary>
    [Fact]
    public void EmptyCropLeavesOneByOne()
    {
        var image = new RawImage();
        image.Convert(new TerrainBitmapInput(3, 2, new byte[6]), crop: true);
        Assert.Equal(1, image.Width());
        Assert.Equal(1, image.Height());
    }

    /// <summary>오른쪽·아래 여백만 잘라내고 왼쪽·위쪽 여백은 남긴다.</summary>
    [Fact]
    public void CropTrimsRightAndBottomOnly()
    {
        var image = new RawImage();
        image.Convert(new TerrainBitmapInput(3, 3, [7, 0, 0, 0, 0, 0, 0, 0, 0]), crop: true);
        Assert.Equal(1, image.Width());
        Assert.Equal(1, image.Height());
    }

    /// <summary>이미 변환된 그림에 다시 변환하면 원문대로 내용을 덮어쓴다(소비된 bits_는 비어 있어 진단을 타지 않는다).</summary>
    [Fact]
    public void DoubleConvertReplacesContent()
    {
        var image = new RawImage();
        image.Convert(new TerrainBitmapInput(1, 1, [1]), crop: false);
        image.Convert(new TerrainBitmapInput(2, 1, [1, 2]), crop: false);
        Assert.Equal(2, image.Width());
        Assert.Equal(1, image.Height());
    }

    /// <summary>CopyFrom은 크기·shape 바이트를 깊게 복사한다.</summary>
    [Fact]
    public void CopyFromClonesShape()
    {
        var source = new RawImage();
        source.Convert(new TerrainBitmapInput(1, 1, [4]), crop: false);
        var copy = new RawImage();
        copy.CopyFrom(source);
        Assert.Equal(source.Width(), copy.Width());
        Assert.Equal(source.ShapeSize(), copy.ShapeSize());
        Assert.True(source.Shape().ToArray().SequenceEqual(copy.Shape().ToArray()));
    }

    /// <summary>Destroy는 버퍼·크기를 지우되 shape 바이트 수는 남긴다.</summary>
    [Fact]
    public void DestroyKeepsShapeSize()
    {
        var image = new RawImage();
        image.Convert(new TerrainBitmapInput(1, 1, [4]), crop: false);
        uint size = image.ShapeSize();
        image.Destroy();
        image.Destroy();
        Assert.Equal(0, image.Width());
        Assert.Equal(size, image.ShapeSize());
    }

    /// <summary>단일 불투명 픽셀은 1.10 매직과 행 끝 토큰을 갖는다.</summary>
    [Fact]
    public void SinglePixelEncodesMagicAndTerminator()
    {
        var image = new RawImage();
        image.Convert(new TerrainBitmapInput(1, 1, [4]), crop: false);
        byte[] shape = image.Shape().ToArray();
        Assert.Equal(0x30312e31u, BitConverter.ToUInt32(shape, 0));
        Assert.Equal(0, shape[^1]);
    }
}
