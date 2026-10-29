using System.Buffers.Binary;
using System.Security.Cryptography;
using Netstorm.Assets;
using Netstorm.Client.Gump;
using Gif = Netstorm.Client.Display.GifImage;

namespace Netstorm.Tests; // 독립 원본 GIF 픽셀 관찰과 읽기 전용 실제 자산을 대조한다.

/// <summary>기대값을 C#으로 생성하지 않고 원본 LZW의 픽셀/입력 SHA와 비교한다.</summary>
public sealed class BackgroundGifAssetsNativeTests
{
    /// <summary>실행 출력 폴더에서 보호 원본의 저장소 루트를 찾는다.</summary>
    internal static string Root()
    {
        // 실제 저장소 경계까지 부모를 따라가며 원본 파일은 읽기만 한다.
        for (DirectoryInfo? directory = new(AppContext.BaseDirectory); directory is not null; directory = directory.Parent)
            if (File.Exists(Path.Combine(directory.FullName, "originals", "Netstorm.exe"))) return directory.FullName;
        throw new DirectoryNotFoundException("BackgroundGif 보호 자산 루트 없음");
    }
    /// <summary>동일 입력의 SHA-256을 소문자로 관측한다.</summary>
    private static string Sha(byte[] bytes) => Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant();
    /// <summary>독립 원본 fixture와 동일한 FNV-1a64 지문을 관측한다.</summary>
    private static string Fingerprint(byte[] bytes)
    {
        ulong value = 14695981039346656037;
        // 순서가 있는 전체 원시 색 번호를 한 번씩 누적한다.
        foreach (byte item in bytes) value = unchecked((value ^ item) * 1099511628211);
        return value.ToString("x16");
    }
    /// <summary>cpppj 검사와 같은 명시 입력 머리만 만들며 기대 픽셀은 만들지 않는다.</summary>
    internal static byte[] Variant(byte[] original, string name)
    {
        int descriptor = 13 + (3 << ((original[10] & 7) + 1));
        if (name == "localPalette")
        {
            var data = new List<byte>(original.AsSpan(0, 13).ToArray()); data[10] &= 0x7f; data.AddRange(original.AsSpan(descriptor, 10).ToArray()); data[22] = (byte)((data[22] & 0x78) | 0x80 | (original[10] & 7)); data.AddRange(original.AsSpan(13, descriptor - 13).ToArray()); data.AddRange(original.AsSpan(descriptor + 10).ToArray()); return data.ToArray();
        }
        Assert.Contains(name, new[] { "logicalOffsets", "smallCanvas", "interlaced" }); byte[] copy = original.ToArray(); bool small = name == "smallCanvas"; BinaryPrimitives.WriteUInt16LittleEndian(copy.AsSpan(6), small ? (ushort)1 : (ushort)683); BinaryPrimitives.WriteUInt16LittleEndian(copy.AsSpan(8), small ? (ushort)1 : (ushort)521); BinaryPrimitives.WriteUInt16LittleEndian(copy.AsSpan(descriptor + 1), small ? (ushort)40000 : (ushort)17); BinaryPrimitives.WriteUInt16LittleEndian(copy.AsSpan(descriptor + 3), small ? (ushort)40000 : (ushort)19); if (name == "interlaced") copy[descriptor + 9] |= 0x40; return copy;
    }
    /// <summary>소스 SHA·색 번호 SHA/FNV·치수와 독립 원본 실행 메타데이터를 대조한다.</summary>
    private static void Check(string[] row, byte[] source, Netstorm.Client.Display.IndexedImage image)
    {
        Assert.Equal(8, row.Length); Assert.Equal(row[5], Sha(source)); Assert.Equal(X86Fixture.Int(row[1]), image.Width); Assert.Equal(X86Fixture.Int(row[2]), image.Height); Assert.Equal(image.Width * image.Height, image.Indices.Length); Assert.Equal(row[3], Fingerprint(image.Indices)); Assert.Equal(row[4], Sha(image.Indices)); Assert.Equal(image.Height, X86Fixture.Int(row[6])); Assert.True(X86Fixture.ULong(row[7]) > 0);
    }
    /// <summary>전체34개 보호 GIF를 실제 VFS/소유 저장소로 읽고 독립 원본 픽셀과 비교한다.</summary>
    [Fact]
    public void All34ProtectedGifSourcesAndNativePixelsMatch()
    {
        var files = GameFileSystem.Open(Path.Combine(Root(), "originals")); var storage = new BackgroundGifStorage(files.ReadAllBytes); int count = 0;
        // 첫 열 경로만 입력으로 쓰고 뒤 SHA/치수는 비교에만 사용한다.
        foreach (string[] row in X86Fixture.Read("background-gif-assets").Skip(1))
        {
            byte[] source = files.ReadAllBytes(row[0]); int width = -1, height = -1; uint token = storage.Load(row[0], ref width, ref height); var image = storage.Bitmap(token); Check(row, source, image); Assert.Equal((image.Width, image.Height), (width, height)); storage.Release(token); Assert.Equal(0, storage.ImageCount); count++;
        }
        Assert.Equal(34, count);
    }
    /// <summary>논리 화면/오프셋 무시·로컬 표·네 인터레이스 패스의 원본4개 입력과 픽셀을 비교한다.</summary>
    [Fact]
    public void All4NativeDescriptorInterlaceAndLocalPaletteVariantsMatch()
    {
        byte[] original = GameFileSystem.Open(Path.Combine(Root(), "originals")).ReadAllBytes("d/titleMenu.gif"); int count = 0;
        // 원본 fixture의 변형 이름만 입력으로 쓰며 해시는 항상 독립 관찰값이다.
        foreach (string[] row in X86Fixture.Read("background-gif-variants").Skip(1))
        {
            byte[] source = Variant(original, row[0]), before = source.ToArray(); var image = Gif.DecodeBackgroundGif(source); Check(row, source, image); Assert.Equal(before, source); if (row[0] == "logicalOffsets") { Assert.Equal(683, Gif.DecodeGif(source).Width); Assert.Equal(639, image.Width); } count++;
        }
        Assert.Equal(4, count);
    }
    /// <summary>일반 GIF의 투명 확장은 허용하되 배경 판독기는 바로 다음 descriptor만 받는다.</summary>
    [Fact]
    public void BackgroundRejectsExtensionBeforeFirstDescriptorWhileGeneralKeepsOpacity()
    {
        byte[] source = GameFileSystem.Open(Path.Combine(Root(), "originals")).ReadAllBytes("d/titleMenu.gif"); int descriptor = 13 + (3 << ((source[10] & 7) + 1)); byte[] extension = [0x21, 0xf9, 4, 1, 0, 0, 0, 0]; byte[] modified = [.. source.AsSpan(0, descriptor), .. extension, .. source.AsSpan(descriptor)]; var general = Gif.DecodeGif(modified); Assert.Equal(639, general.Width); Assert.Contains((byte)0, general.Indices); Assert.Equal(general.Indices.Select(index => index == 0 ? (byte)0 : (byte)255), general.Opacity); Assert.Throws<InvalidDataException>(() => Gif.DecodeBackgroundGif(modified)); Assert.Throws<InvalidDataException>(() => Gif.DecodeBackgroundGif([]));
    }
}
