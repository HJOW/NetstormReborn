using System.Buffers.Binary;
using System.IO.Compression;
using System.Text;

namespace Netstorm.AnalyzeManager.Tests;

/// <summary>운영체제 그림 API 없이 쓰는 PNG·캔버스 도우미(리눅스 관찰표 경로)를 검사한다.</summary>
public sealed class PortableImageTests
{
    /// <summary>만든 PNG 를 다시 풀면 같은 픽셀이 나오고 머리의 크기도 맞다</summary>
    [Fact]
    public void EncodeRgbPng_RoundTripsPixelsAndSize()
    {
        const int width = 7;
        const int height = 3;
        var rgb = new byte[width * height * 3];
        // 위치마다 다른 값을 넣어 행·열 뒤바뀜을 잡는다
        for (int i = 0; i < rgb.Length; i++)
        {
            rgb[i] = (byte)(i * 37 % 251);
        }
        byte[] png = PortableImage.EncodeRgbPng(rgb, width, height);
        Assert.Equal((width, height), PortableImage.PngSize(png));
        Assert.Equal(rgb, DecodeUnfiltered(png, width, height));
    }

    /// <summary>모든 청크의 CRC 가 PNG 표준 값(예: IEND = AE426082)과 맞다</summary>
    [Fact]
    public void EncodeRgbPng_WritesStandardCrc()
    {
        byte[] png = PortableImage.EncodeRgbPng(PortableImage.Canvas(1, 1, 0, 0, 0), 1, 1);
        Assert.Equal(new byte[] { 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82 }, png[^8..]);
    }

    /// <summary>PNG 가 아닌 바이트는 크기 읽기에서 거부한다</summary>
    [Fact]
    public void PngSize_RejectsNonPng()
    {
        Assert.Throws<InvalidDataException>(() => PortableImage.PngSize(Encoding.ASCII.GetBytes("GIF89a-not-a-png-file-at-all")));
    }

    /// <summary>캔버스 밖으로 나가는 복사는 잘려서 안쪽만 바뀐다</summary>
    [Fact]
    public void Blit_ClipsOutsideCanvas()
    {
        byte[] canvas = PortableImage.Canvas(4, 4, 0, 0, 0);
        byte[] source = PortableImage.Canvas(3, 3, 9, 9, 9);
        PortableImage.Blit(canvas, 4, source, 3, 3, 2, -1);
        // (2..3, 0..1) 만 칠해진다
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                byte expected = x >= 2 && y <= 1 ? (byte)9 : (byte)0;
                Assert.Equal(expected, canvas[(y * 4 + x) * 3]);
            }
        }
    }

    /// <summary>글자는 5×7 점을 배율대로 칠하고, 없는 문자는 '?' 모양으로 그린다</summary>
    [Fact]
    public void DrawText_PaintsGlyphsAndFallsBack()
    {
        byte[] colon = PortableImage.Canvas(12, 14, 0, 0, 0);
        PortableImage.DrawText(colon, 12, ":", 0, 0, 2, 255, 255, 255);
        // ':' 의 (1,1) 점은 칠해지고 (0,0) 점은 비어 있다
        Assert.Equal(255, colon[(2 * 12 + 2) * 3]);
        Assert.Equal(0, colon[0]);
        byte[] unknown = PortableImage.Canvas(6, 7, 0, 0, 0);
        byte[] question = PortableImage.Canvas(6, 7, 0, 0, 0);
        PortableImage.DrawText(unknown, 6, "가", 0, 0, 1, 1, 2, 3);
        PortableImage.DrawText(question, 6, "?", 0, 0, 1, 1, 2, 3);
        Assert.Equal(question, unknown);
    }

    /// <summary>필터 0 행만 있는 IDAT 를 풀어 RGB 를 돌려준다 (EncodeRgbPng 출력 검사용)</summary>
    private static byte[] DecodeUnfiltered(byte[] png, int width, int height)
    {
        using var idat = new MemoryStream();
        int offset = 8;
        // 청크를 차례로 읽어 IDAT 데이터만 모은다
        while (offset < png.Length)
        {
            int length = BinaryPrimitives.ReadInt32BigEndian(png.AsSpan(offset, 4));
            string type = Encoding.ASCII.GetString(png, offset + 4, 4);
            if (type == "IDAT")
            {
                idat.Write(png, offset + 8, length);
            }
            offset += 12 + length;
        }
        idat.Position = 0;
        using var zlib = new ZLibStream(idat, CompressionMode.Decompress);
        using var raw = new MemoryStream();
        zlib.CopyTo(raw);
        byte[] rows = raw.ToArray();
        var rgb = new byte[width * height * 3];
        // 행마다 필터 바이트(0)를 확인하고 픽셀만 옮긴다
        for (int y = 0; y < height; y++)
        {
            Assert.Equal(0, rows[y * (width * 3 + 1)]);
            Buffer.BlockCopy(rows, y * (width * 3 + 1) + 1, rgb, y * width * 3, width * 3);
        }
        return rgb;
    }
}
