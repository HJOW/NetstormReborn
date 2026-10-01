using System.Buffers.Binary;
using System.IO.Compression;
using System.Text;

namespace Netstorm.AnalyzeManager;

/// <summary>
/// 운영체제 그림 API(GDI+ 등) 없이 쓰는 최소 그림 도구: PNG 크기 읽기, RGB 24bit 캔버스·사각형·글자, PNG 저장.
/// 리눅스용 YouTube 전용 빌드의 관찰표와, 모든 빌드의 프레임 크기 읽기에 쓴다.
/// </summary>
public static class PortableImage
{
    /// <summary>PNG 파일 서명 8바이트</summary>
    private static readonly byte[] PngSignature = [137, 80, 78, 71, 13, 10, 26, 10];

    /// <summary>내장 글꼴 한 글자의 폭(점)</summary>
    public const int GlyphWidth = 5;

    /// <summary>내장 글꼴 한 글자의 높이(점)</summary>
    public const int GlyphHeight = 7;

    /// <summary>
    /// 관찰표 글자에 필요한 문자만 담은 5×7 비트맵 글꼴. 각 문자열은 위 행부터 5점씩 7행(35자, 1 = 점).
    /// 영문 소문자는 대문자 모양으로 그린다(SponsorBlock 분류 이름 표시용).
    /// </summary>
    private static readonly Dictionary<char, string> Glyphs = new()
    {
        [' '] = "00000000000000000000000000000000000",
        ['0'] = "01110100011001110101110011000101110",
        ['1'] = "00100011000010000100001000010001110",
        ['2'] = "01110100010000100010001000100011111",
        ['3'] = "11111000100010000010000011000101110",
        ['4'] = "00010001100101010010111110001000010",
        ['5'] = "11111100001111000001000011000101110",
        ['6'] = "00110010001000011110100011000101110",
        ['7'] = "11111000010001000100010000100001000",
        ['8'] = "01110100011000101110100011000101110",
        ['9'] = "01110100011000101111000010001001100",
        [':'] = "00000011000110000000011000110000000",
        ['.'] = "00000000000000000000000000110001100",
        ['['] = "01110010000100001000010000100001110",
        [']'] = "01110000100001000010000100001001110",
        ['_'] = "00000000000000000000000000000011111",
        ['-'] = "00000000000000011111000000000000000",
        ['?'] = "01110100010000100010001000000000100",
        ['A'] = "01110100011000111111100011000110001",
        ['B'] = "11110100011000111110100011000111110",
        ['C'] = "01110100011000010000100001000101110",
        ['D'] = "11100100101000110001100011001011100",
        ['E'] = "11111100001000011110100001000011111",
        ['F'] = "11111100001000011110100001000010000",
        ['G'] = "01110100011000010111100011000101111",
        ['H'] = "10001100011000111111100011000110001",
        ['I'] = "01110001000010000100001000010001110",
        ['J'] = "00111000100001000010000101001001100",
        ['K'] = "10001100101010011000101001001010001",
        ['L'] = "10000100001000010000100001000011111",
        ['M'] = "10001110111010110101100011000110001",
        ['N'] = "10001100011100110101100111000110001",
        ['O'] = "01110100011000110001100011000101110",
        ['P'] = "11110100011000111110100001000010000",
        ['Q'] = "01110100011000110001101011001001101",
        ['R'] = "11110100011000111110101001001010001",
        ['S'] = "01111100001000001110000010000111110",
        ['T'] = "11111001000010000100001000010000100",
        ['U'] = "10001100011000110001100011000101110",
        ['V'] = "10001100011000110001100010101000100",
        ['W'] = "10001100011000110101101011010101010",
        ['X'] = "10001100010101000100010101000110001",
        ['Y'] = "10001100011000101010001000010000100",
        ['Z'] = "11111000010001000100010001000011111",
    };

    /// <summary>PNG 머리(IHDR)에서 폭·높이를 읽는다</summary>
    /// <param name="png">PNG 파일 바이트</param>
    public static (int Width, int Height) PngSize(byte[] png)
    {
        if (png.Length < 24 || !png.AsSpan(0, 8).SequenceEqual(PngSignature) || Encoding.ASCII.GetString(png, 12, 4) != "IHDR")
        {
            throw new InvalidDataException("PNG 형식이 아닙니다.");
        }
        return (BinaryPrimitives.ReadInt32BigEndian(png.AsSpan(16, 4)), BinaryPrimitives.ReadInt32BigEndian(png.AsSpan(20, 4)));
    }

    /// <summary>RGB 24bit 캔버스를 한 색으로 채운다</summary>
    /// <param name="width">폭</param>
    /// <param name="height">높이</param>
    /// <param name="r">빨강</param>
    /// <param name="g">초록</param>
    /// <param name="b">파랑</param>
    public static byte[] Canvas(int width, int height, byte r, byte g, byte b)
    {
        var pixels = new byte[checked(width * height * 3)];
        // 픽셀마다 같은 색을 쓴다
        for (int i = 0; i < pixels.Length; i += 3)
        {
            pixels[i] = r;
            pixels[i + 1] = g;
            pixels[i + 2] = b;
        }
        return pixels;
    }

    /// <summary>RGB 24bit 그림을 캔버스의 (x, y)에 복사한다 (캔버스 밖은 잘라냄)</summary>
    /// <param name="canvas">대상 캔버스</param>
    /// <param name="canvasWidth">캔버스 폭</param>
    /// <param name="source">원본 RGB</param>
    /// <param name="sourceWidth">원본 폭</param>
    /// <param name="sourceHeight">원본 높이</param>
    /// <param name="x">놓을 왼쪽</param>
    /// <param name="y">놓을 위쪽</param>
    public static void Blit(byte[] canvas, int canvasWidth, byte[] source, int sourceWidth, int sourceHeight, int x, int y)
    {
        int canvasHeight = canvas.Length / 3 / canvasWidth;
        // 원본의 행마다 캔버스 안에 들어가는 부분만 복사한다
        for (int row = 0; row < sourceHeight; row++)
        {
            int ty = y + row;
            if (ty < 0 || ty >= canvasHeight)
            {
                continue;
            }
            int left = Math.Max(0, -x);
            int right = Math.Min(sourceWidth, canvasWidth - x);
            if (right <= left)
            {
                return;
            }
            Buffer.BlockCopy(source, (row * sourceWidth + left) * 3, canvas, (ty * canvasWidth + x + left) * 3, (right - left) * 3);
        }
    }

    /// <summary>
    /// 내장 5×7 글꼴로 글자를 쓴다. 없는 문자는 '?'로 그린다. 글자 간격은 (5 + 1) × scale 점이다.
    /// </summary>
    /// <param name="canvas">대상 캔버스</param>
    /// <param name="canvasWidth">캔버스 폭</param>
    /// <param name="text">쓸 글자</param>
    /// <param name="x">왼쪽</param>
    /// <param name="y">위쪽</param>
    /// <param name="scale">점 하나의 크기(픽셀)</param>
    /// <param name="r">빨강</param>
    /// <param name="g">초록</param>
    /// <param name="b">파랑</param>
    public static void DrawText(byte[] canvas, int canvasWidth, string text, int x, int y, int scale, byte r, byte g, byte b)
    {
        int canvasHeight = canvas.Length / 3 / canvasWidth;
        int penX = x;
        // 글자마다 7행 5열의 점을 scale 배로 칠한다
        foreach (char raw in text)
        {
            char c = char.ToUpperInvariant(raw);
            string glyph = Glyphs.TryGetValue(c, out string? found) ? found : Glyphs['?'];
            // 글꼴의 점을 하나씩 확인한다
            for (int i = 0; i < GlyphWidth * GlyphHeight; i++)
            {
                if (glyph[i] != '1')
                {
                    continue;
                }
                int px = penX + i % GlyphWidth * scale;
                int py = y + i / GlyphWidth * scale;
                // 점 하나를 scale × scale 사각형으로 칠한다
                for (int dy = 0; dy < scale; dy++)
                {
                    for (int dx = 0; dx < scale; dx++)
                    {
                        int tx = px + dx;
                        int ty = py + dy;
                        if (tx < 0 || ty < 0 || tx >= canvasWidth || ty >= canvasHeight)
                        {
                            continue;
                        }
                        int offset = (ty * canvasWidth + tx) * 3;
                        canvas[offset] = r;
                        canvas[offset + 1] = g;
                        canvas[offset + 2] = b;
                    }
                }
            }
            penX += (GlyphWidth + 1) * scale;
        }
    }

    /// <summary>RGB 24bit 캔버스를 PNG(8bit 트루컬러, 필터 없음, zlib 압축)로 만든다</summary>
    /// <param name="rgb">픽셀(행 우선, RGB 순서)</param>
    /// <param name="width">폭</param>
    /// <param name="height">높이</param>
    public static byte[] EncodeRgbPng(byte[] rgb, int width, int height)
    {
        if (width <= 0 || height <= 0 || rgb.Length != width * height * 3)
        {
            throw new ArgumentException("RGB 크기가 폭·높이와 맞지 않습니다.");
        }
        using var compressed = new MemoryStream();
        using (var zlib = new ZLibStream(compressed, CompressionLevel.Optimal, leaveOpen: true))
        {
            // 각 행 앞에 필터 종류 0(없음)을 붙인다
            for (int row = 0; row < height; row++)
            {
                zlib.WriteByte(0);
                zlib.Write(rgb, row * width * 3, width * 3);
            }
        }
        var header = new byte[13];
        BinaryPrimitives.WriteInt32BigEndian(header.AsSpan(0, 4), width);
        BinaryPrimitives.WriteInt32BigEndian(header.AsSpan(4, 4), height);
        header[8] = 8; // 채널당 비트 수
        header[9] = 2; // 색 형식: 트루컬러(RGB)
        using var png = new MemoryStream();
        png.Write(PngSignature);
        WriteChunk(png, "IHDR", header);
        WriteChunk(png, "IDAT", compressed.ToArray());
        WriteChunk(png, "IEND", []);
        return png.ToArray();
    }

    /// <summary>PNG 청크(길이·종류·데이터·CRC)를 쓴다</summary>
    private static void WriteChunk(Stream output, string type, byte[] data)
    {
        Span<byte> number = stackalloc byte[4];
        BinaryPrimitives.WriteInt32BigEndian(number, data.Length);
        output.Write(number);
        byte[] typeBytes = Encoding.ASCII.GetBytes(type);
        output.Write(typeBytes);
        output.Write(data);
        uint crc = Crc32(Crc32(0xFFFFFFFFu, typeBytes), data) ^ 0xFFFFFFFFu;
        BinaryPrimitives.WriteUInt32BigEndian(number, crc);
        output.Write(number);
    }

    /// <summary>PNG 표준 CRC-32 표 (다항식 0xEDB88320)</summary>
    private static readonly uint[] CrcTable = BuildCrcTable();

    /// <summary>CRC-32 표를 만든다</summary>
    private static uint[] BuildCrcTable()
    {
        var table = new uint[256];
        // 바이트 값마다 8비트를 차례로 나눈 나머지를 구한다
        for (uint n = 0; n < 256; n++)
        {
            uint c = n;
            // 비트마다 다항식으로 나눈다
            for (int k = 0; k < 8; k++)
            {
                c = (c & 1) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            }
            table[n] = c;
        }
        return table;
    }

    /// <summary>진행 중인 CRC 값에 바이트들을 반영한다 (시작값 0xFFFFFFFF, 끝에 반전)</summary>
    private static uint Crc32(uint crc, byte[] bytes)
    {
        // 바이트마다 표를 찾아 갱신한다
        foreach (byte value in bytes)
        {
            crc = CrcTable[(crc ^ value) & 0xFF] ^ (crc >> 8);
        }
        return crc;
    }
}
