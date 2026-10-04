using System.Buffers.Binary;

namespace Netstorm.Assets;

/// <summary>원본 RT_CURSOR의 단색 비트맵. SDL의 색·마스크 비트로 투명·배경 반전을 그대로 보존한다.</summary>
public sealed record CursorBitmap(int Width, int Height, int HotX, int HotY, byte[] Data, byte[] Mask)
{
    /// <summary>핫스팟 4바이트, BITMAPINFOHEADER, 흑백 팔레트, 아래부터 저장된 XOR·AND 비트맵을 읽는다.</summary>
    public static CursorBitmap Parse(ReadOnlySpan<byte> source)
    {
        if (source.Length < 52) throw new InvalidDataException("커서 리소스 머리가 잘렸습니다.");
        int headerSize = BinaryPrimitives.ReadInt32LittleEndian(source[4..]);
        int width = BinaryPrimitives.ReadInt32LittleEndian(source[8..]);
        int doubledHeight = BinaryPrimitives.ReadInt32LittleEndian(source[12..]);
        int height = doubledHeight / 2;
        if (headerSize != 40 || width <= 0 || width > 256 || width % 8 != 0 || height <= 0 || height > 256
            || doubledHeight % 2 != 0 || BinaryPrimitives.ReadInt16LittleEndian(source[18..]) != 1
            || BinaryPrimitives.ReadInt32LittleEndian(source[20..]) != 0)
            throw new InvalidDataException("지원하지 않는 단색 커서 형식입니다.");
        int hotX = BinaryPrimitives.ReadUInt16LittleEndian(source);
        int hotY = BinaryPrimitives.ReadUInt16LittleEndian(source[2..]);
        if (hotX >= width || hotY >= height) throw new InvalidDataException("커서 핫스팟이 그림 밖에 있습니다.");
        int rowBytes = (width + 31) / 32 * 4;
        int bitmapStart = 4 + headerSize + 8;
        if (source.Length < bitmapStart + rowBytes * height * 2)
            throw new InvalidDataException("커서 비트맵이 잘렸습니다.");
        // 원본은 팔레트의 0번이 검정, 1번이 흰색이다. 임의 팔레트를 잘못 반전시키지 않도록 확인한다.
        if (!source.Slice(44, 3).SequenceEqual(new byte[] { 0, 0, 0 })
            || !source.Slice(48, 3).SequenceEqual(new byte[] { 255, 255, 255 }))
            throw new InvalidDataException("원본 흑백 커서 팔레트가 아닙니다.");
        int pitch = width / 8;
        var data = new byte[pitch * height];
        var mask = new byte[data.Length];
        // DIB의 아래쪽 행부터 저장된 픽셀을 SDL의 위쪽 행부터 저장된 비트로 바꾼다.
        for (int y = 0; y < height; y++)
        {
            int row = (height - 1 - y) * rowBytes;
            // XOR/AND의 네 조합을 SDL의 검정·흰색·투명·반전 조합으로 옮긴다.
            for (int x = 0; x < pitch; x++)
            {
                byte xor = source[bitmapStart + row + x];
                byte and = source[bitmapStart + rowBytes * height + row + x];
                data[y * pitch + x] = (byte)~(xor ^ and);
                mask[y * pitch + x] = (byte)~and;
            }
        }
        return new CursorBitmap(width, height, hotX, hotY, data, mask);
    }
}
