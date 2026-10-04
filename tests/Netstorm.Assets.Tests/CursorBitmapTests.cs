using System.Buffers.Binary;

namespace Netstorm.Assets.Tests;

/// <summary>원본 커서의 네 픽셀 조합, 행 순서, 핫스팟과 손상 데이터 거부를 검사한다.</summary>
public sealed class CursorBitmapTests
{
    /// <summary>검정·흰색·투명·반전이 같은 행에 들어 있는 작은 원본 형식 커서.</summary>
    private static byte[] Fixture()
    {
        var data = new byte[68];
        BinaryPrimitives.WriteUInt16LittleEndian(data, 3);
        BinaryPrimitives.WriteUInt16LittleEndian(data.AsSpan(2), 1);
        BinaryPrimitives.WriteInt32LittleEndian(data.AsSpan(4), 40);
        BinaryPrimitives.WriteInt32LittleEndian(data.AsSpan(8), 8);
        BinaryPrimitives.WriteInt32LittleEndian(data.AsSpan(12), 4);
        BinaryPrimitives.WriteUInt16LittleEndian(data.AsSpan(16), 1);
        BinaryPrimitives.WriteUInt16LittleEndian(data.AsSpan(18), 1);
        data[48] = data[49] = data[50] = 255;
        // DIB는 아래 행부터 저장한다. 위 행 앞 네 픽셀에 네 가지 조합을 배치한다.
        data[56] = 0x50;
        data[60] = 255;
        data[64] = 0x30;
        return data;
    }

    /// <summary>투명과 반전을 구분하며 아래 행을 위 행으로 오인하지 않는다.</summary>
    [Fact]
    public void Parse_PreservesFourPixelModesAndHotspot()
    {
        CursorBitmap cursor = CursorBitmap.Parse(Fixture());
        Assert.Equal((8, 2, 3, 1), (cursor.Width, cursor.Height, cursor.HotX, cursor.HotY));
        Assert.Equal(new byte[] { 0x9f, 0 }, cursor.Data);
        Assert.Equal(new byte[] { 0xcf, 0 }, cursor.Mask);
    }

    /// <summary>잘린 데이터와 그림 밖 핫스팟을 거부한다.</summary>
    [Fact]
    public void Parse_RejectsTruncationAndInvalidHotspot()
    {
        byte[] bytes = Fixture();
        Assert.Throws<InvalidDataException>(() => CursorBitmap.Parse(bytes.AsSpan(0, 50)));
        Assert.Throws<InvalidDataException>(() => CursorBitmap.Parse(bytes.AsSpan(0, 67)));
        bytes[0] = 8;
        Assert.Throws<InvalidDataException>(() => CursorBitmap.Parse(bytes));
    }

    /// <summary>동봉된 다섯 원본 리소스의 크기와 픽셀 대조로 확정한 핫스팟을 확인한다.</summary>
    [Theory]
    [InlineData(5, 16, 16)]
    [InlineData(6, 16, 16)]
    [InlineData(7, 16, 16)]
    [InlineData(8, 10, 6)]
    [InlineData(20, 16, 16)]
    public void OriginalResources_KeepTheirDimensionsAndHotspots(int resource, int hotX, int hotY)
    {
        CursorBitmap cursor = CursorBitmap.Parse(File.ReadAllBytes(OriginalData.RequireFile($"cursors/RT_CURSOR_{resource}.bin")));
        Assert.Equal((32, 32, hotX, hotY), (cursor.Width, cursor.Height, cursor.HotX, cursor.HotY));
        Assert.Equal(128, cursor.Data.Length);
        Assert.Equal(128, cursor.Mask.Length);
    }
}
