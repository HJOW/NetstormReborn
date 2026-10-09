using System.Security.Cryptography;

namespace Netstorm.Assets.Tests;

/// <summary>
/// 배경 GIF 색 번호 해독(<see cref="GifImage"/>) 테스트. 실제 타이틀·구름 그림의 색 번호 전체와
/// 작은 합성 GIF 의 인터레이스·투명·이미지 위치·손상 거부를 검사한다 (LEFT_JOBS.dotnetpj.md 5-5).
/// </summary>
public sealed class GifImageTests
{
    /// <summary>합성 GIF 의 최소 LZW 코드 크기 (8색).</summary>
    private const int TestCodeSize = 3;

    /// <summary>
    /// 실제 배경 GIF 의 색 번호 전체가 독립 해독기(Pillow)의 결과와 같다. 큰 그림이라 LZW 코드 폭 증가와 사전 재설정을 함께 지난다.
    /// 기대 SHA-256 은 Pillow 로 푼 색 번호(행 우선)의 해시다 (2026-10-10 산출).
    /// </summary>
    /// <param name="path">자료 폴더 기준 경로</param>
    /// <param name="width">논리 화면 폭</param>
    /// <param name="height">논리 화면 높이</param>
    /// <param name="sha256">색 번호 전체의 SHA-256 (소문자 16진수)</param>
    [Theory]
    [InlineData("d/titleMenu.gif", 639, 480, "d653adbd23234d6a09a443391154cb50a365b4a0d8b35b1bffdd04b9fe706510")]
    [InlineData("d/Gifcloud.gif", 512, 512, "d963b1d8d721067b370f32d8ded66eb92532e70bb36714a46f5859cc62e5799c")]
    [InlineData("d/Gifcloud2.GIF", 512, 512, "ffac56e2bc115b52ffd899476d5c72e4e58aee3fb22ea705ec136f8ee694a063")]
    [InlineData("d/victory.gif", 640, 480, "b07dd5df03b80004c440ea0047079a53ddab9e9a611d641535fe8fd734b604e0")]
    [InlineData("d/Defeat.gif", 640, 480, "9f394419dbde55e9ec8025b445ad7748c0b5ea4dced9039c06b79187494551f0")]
    public void OriginalBackgrounds_DecodeToExpectedIndices(string path, int width, int height, string sha256)
    {
        IndexedImage image = GifImage.Decode(File.ReadAllBytes(OriginalData.RequireFile(path)));
        Assert.Equal((width, height), (image.Width, image.Height));
        Assert.Equal(sha256, Convert.ToHexStringLower(SHA256.HashData(image.Indices)));
        Assert.All(image.Opaque, Assert.True);
    }

    /// <summary>
    /// 타이틀 그림의 내장 RGB 표는 게임 팔레트와 다르므로(쓰는 번호 181개 중 177개) 색 번호를 게임 팔레트로 칠해야 한다.
    /// 구름 그림은 243번 하나만 다르다. 이 수가 0 이 되면 색 번호 해독 경로가 필요 없다는 뜻이므로 근거를 다시 본다.
    /// </summary>
    /// <param name="path">자료 폴더 기준 경로</param>
    /// <param name="differing">쓰는 색 번호 가운데 내장 표와 게임 팔레트의 RGB 가 다른 번호 수</param>
    [Theory]
    [InlineData("d/titleMenu.gif", 177)]
    [InlineData("d/Gifcloud.gif", 1)]
    public void EmbeddedColorTable_DiffersFromGamePalette(string path, int differing)
    {
        byte[] bytes = File.ReadAllBytes(OriginalData.RequireFile(path));
        IndexedImage image = GifImage.Decode(bytes);
        Palette palette = Palette.Load(OriginalData.RequireFile("d/" + Palette.GameCol));
        // 머리말 13바이트 바로 뒤가 256색 전역 색 표다 (두 그림 모두 전역 표가 있다).
        Assert.Equal(0x87, bytes[10] & 0x87);
        var used = new bool[Palette.ColorCount];
        // 그림이 실제로 쓰는 색 번호를 표시한다.
        foreach (byte index in image.Indices)
        {
            used[index] = true;
        }
        int count = 0;
        // 쓰는 번호마다 내장 표의 RGB 와 게임 팔레트를 비교한다.
        for (int i = 0; i < Palette.ColorCount; i++)
        {
            Rgb embedded = new(bytes[13 + i * 3], bytes[14 + i * 3], bytes[15 + i * 3]);
            if (used[i] && embedded != palette[i]) count++;
        }
        Assert.Equal(differing, count);
    }

    /// <summary>인터레이스 그림의 네 패스가 실제 행 순서로 놓인다.</summary>
    [Fact]
    public void Interlaced_RowsReturnToDisplayOrder()
    {
        const int width = 3, height = 9;
        var expected = new byte[width * height];
        // 행마다 다른 값을 넣어 행 순서가 틀리면 드러나게 한다.
        for (int i = 0; i < expected.Length; i++)
        {
            expected[i] = (byte)((i / width + i % width) % 8);
        }
        IndexedImage image = GifImage.Decode(Build(width, height, 0, 0, width, height, expected, interlaced: true));
        Assert.Equal(expected, image.Indices);
        Assert.All(image.Opaque, Assert.True);
    }

    /// <summary>화면보다 작은 이미지는 제 위치에 놓이고, 나머지는 배경 번호이며 투명 번호인 픽셀과 사각형 밖은 불투명하지 않다.</summary>
    [Fact]
    public void OffsetImage_FillsBackgroundAndMarksTransparency()
    {
        byte[] pixels = [1, 2, 3, 2];
        IndexedImage image = GifImage.Decode(Build(4, 3, 1, 1, 2, 2, pixels, background: 5, transparent: 2,
            globalTable: true, localTable: true));
        Assert.Equal((4, 3), (image.Width, image.Height));
        Assert.Equal(new byte[] { 5, 5, 5, 5, 5, 1, 2, 5, 5, 3, 2, 5 }, image.Indices);
        Assert.Equal(new[] { false, false, false, false, false, true, false, false, false, true, false, false }, image.Opaque);
    }

    /// <summary>서명이 다르거나 잘린 파일, 화면 밖 이미지, 픽셀 수가 맞지 않는 자료를 거부한다.</summary>
    [Fact]
    public void RejectsCorruptFiles()
    {
        byte[] pixels = [1, 2, 3, 4];
        byte[] valid = Build(2, 2, 0, 0, 2, 2, pixels);
        Assert.Equal(pixels, GifImage.Decode(valid).Indices);
        Assert.Throws<InvalidDataException>(() => GifImage.Decode([]));
        byte[] bad = (byte[])valid.Clone();
        bad[0] = (byte)'X';
        Assert.Throws<InvalidDataException>(() => GifImage.Decode(bad));
        // 끝에서부터 잘라 가며 어느 길이에서도 예외 종류가 같음을 확인한다.
        for (int length = 13; length < valid.Length - 1; length++)
        {
            Assert.Throws<InvalidDataException>(() => GifImage.Decode(valid.AsSpan(0, length)));
        }
        Assert.Throws<InvalidDataException>(() => GifImage.Decode(Build(2, 2, 1, 0, 2, 2, pixels)));
        Assert.Throws<InvalidDataException>(() => GifImage.Decode(Build(2, 2, 0, 0, 2, 2, [1, 2, 3])));
        Assert.Throws<InvalidDataException>(() => GifImage.Decode(Build(2, 2, 0, 0, 2, 2, [1, 2, 3, 4, 5])));
    }

    /// <summary>
    /// 검사용 GIF89a 한 장을 만든다. 압축은 사전이 자라기 전에 재설정 코드를 넣어 코드 폭을 4비트로 고정한 LZW 다.
    /// </summary>
    /// <param name="screenWidth">논리 화면 폭</param>
    /// <param name="screenHeight">논리 화면 높이</param>
    /// <param name="left">이미지 왼쪽 위치</param>
    /// <param name="top">이미지 위쪽 위치</param>
    /// <param name="width">이미지 폭</param>
    /// <param name="height">이미지 높이</param>
    /// <param name="pixels">표시 순서(위에서 아래)의 색 번호 0~7. 개수가 폭×높이와 다르면 손상 파일이 된다</param>
    /// <param name="interlaced">인터레이스 순서로 저장할지</param>
    /// <param name="background">배경 색 번호</param>
    /// <param name="transparent">투명 색 번호 (없으면 -1)</param>
    /// <param name="globalTable">전역 색 표(8색)를 넣을지</param>
    /// <param name="localTable">지역 색 표(8색)를 넣을지</param>
    private static byte[] Build(int screenWidth, int screenHeight, int left, int top, int width, int height, byte[] pixels,
        bool interlaced = false, int background = 0, int transparent = -1, bool globalTable = false, bool localTable = false)
    {
        var file = new List<byte>();
        file.AddRange("GIF89a"u8.ToArray());
        AddWord(file, screenWidth);
        AddWord(file, screenHeight);
        file.Add((byte)(globalTable ? 0x82 : 0));
        file.Add((byte)background);
        file.Add(0);
        if (globalTable) file.AddRange(new byte[24]);
        if (transparent >= 0) file.AddRange([0x21, 0xF9, 4, 1, 0, 0, (byte)transparent, 0]);
        file.Add(0x2C);
        AddWord(file, left);
        AddWord(file, top);
        AddWord(file, width);
        AddWord(file, height);
        file.Add((byte)((interlaced ? 0x40 : 0) | (localTable ? 0x82 : 0)));
        if (localTable) file.AddRange(new byte[24]);
        byte[] stored = pixels;
        if (interlaced)
        {
            var ordered = new List<byte>();
            int[] starts = [0, 4, 2, 1], steps = [8, 8, 4, 2];
            // 네 패스의 저장 순서대로 행을 모은다.
            for (int pass = 0; pass < 4; pass++)
            {
                // 이번 패스에 속한 행.
                for (int y = starts[pass]; y < height; y += steps[pass])
                {
                    ordered.AddRange(pixels.AsSpan(y * width, width).ToArray());
                }
            }
            stored = [.. ordered];
        }
        file.Add(TestCodeSize);
        byte[] packed = Pack(stored);
        // 255바이트 이하의 서브 블록으로 나눈다.
        for (int offset = 0; offset < packed.Length; offset += 255)
        {
            int size = Math.Min(255, packed.Length - offset);
            file.Add((byte)size);
            file.AddRange(packed.AsSpan(offset, size).ToArray());
        }
        file.Add(0);
        file.Add(0x3B);
        return [.. file];
    }

    /// <summary>색 번호열을 4비트 고정 폭 LZW 코드(재설정 8·종료 9)로 낮은 비트부터 채운다.</summary>
    /// <param name="pixels">저장 순서의 색 번호 0~7</param>
    private static byte[] Pack(byte[] pixels)
    {
        const int clear = 1 << TestCodeSize, end = clear + 1, codeWidth = TestCodeSize + 1;
        var codes = new List<int>();
        // 네 글자마다 재설정 코드를 넣어 사전이 16개에 닿지 않게(코드 폭이 늘지 않게) 한다.
        for (int i = 0; i < pixels.Length; i++)
        {
            if (i % 4 == 0) codes.Add(clear);
            codes.Add(pixels[i]);
        }
        if (pixels.Length == 0) codes.Add(clear);
        codes.Add(end);
        var bytes = new byte[(codes.Count * codeWidth + 7) / 8];
        // 코드를 낮은 비트부터 이어 붙인다.
        for (int i = 0; i < codes.Count; i++)
        {
            int bit = i * codeWidth;
            bytes[bit / 8] |= (byte)(codes[i] << (bit % 8));
        }
        return bytes;
    }

    /// <summary>리틀 엔디언 16비트 정수를 덧붙인다.</summary>
    /// <param name="file">쓰는 중인 파일 내용</param>
    /// <param name="value">값</param>
    private static void AddWord(List<byte> file, int value)
    {
        file.Add((byte)value);
        file.Add((byte)(value >> 8));
    }
}
