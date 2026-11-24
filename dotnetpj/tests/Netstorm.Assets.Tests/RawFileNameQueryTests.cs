using System.Text;
using Netstorm.Assets;

namespace Netstorm.Tests; // 같은 장기 고리의 나머지 이름/디렉터리/전체 조회 물리를 대조한다.

/// <summary>459b20 이름·459990 디렉터리·459890 전체의 원문 바이트·고리·실패 계약이다.</summary>
public sealed class RawFileNameQueryTests
{
    /// <summary>같은 고리의 현재 칸 번호를 읽는다.</summary>
    private static int IndexOf(RawFileName names) => names.Index;

    /// <summary>원문 파일명은 마지막 구분자·colon 다음을 빌린다.</summary>
    [Fact]
    public void NameTakesAfterLastSeparatorOrColon()
    {
        var names = new RawFileName();
        Assert.Equal("game.exe"u8.ToArray(), names.Name("C:\\d\\game.exe"u8.ToArray()).ToArray());
        Assert.Equal("game.exe"u8.ToArray(), names.Name("C:game.exe"u8.ToArray()).ToArray());
        Assert.Equal(2, IndexOf(names));
    }

    /// <summary>구분자가 없으면 첫 BYTE만 NUL인 빈 뷰를 둔다.</summary>
    [Fact]
    public void NameWithoutSeparatorIsEmpty()
    {
        var names = new RawFileName();
        Assert.Empty(names.Name("game.exe"u8.ToArray()).ToArray());
        Assert.Equal(0, names.CurrentBytes.ToArray()[0]);
    }

    /// <summary>끝 확장자 제거는 459550 규칙으로 마지막 dot만 지운다.</summary>
    [Fact]
    public void NameStripsOnlyLastExtension()
    {
        var names = new RawFileName();
        Assert.Equal("archive.tar"u8.ToArray(), names.Name("C:\\d\\archive.tar.gz"u8.ToArray(), false).ToArray());
        Assert.Equal("archive.tar.gz"u8.ToArray(), names.Name("C:\\d\\archive.tar.gz"u8.ToArray(), true).ToArray());
        Assert.Equal(".profile"u8.ToArray(), names.Name("C:\\.profile"u8.ToArray(), false).ToArray());
    }

    /// <summary>첫 NUL 뒤는 읽지 않고 256BYTE 이상은 거부한다.</summary>
    [Fact]
    public void NameTruncatesAtNulAndRejectsOverflow()
    {
        var names = new RawFileName();
        byte[] input = [.. "C:\\a.exe"u8.ToArray(), 0, .. "garbage"u8.ToArray()];
        Assert.Equal("a.exe"u8.ToArray(), names.Name(input).ToArray());
        Assert.Throws<ArgumentOutOfRangeException>(() => names.Name(Enumerable.Repeat((byte)'x', 256).ToArray()));
    }

    /// <summary>원문 부분 단계 경계와 UNC 시작을 그대로 둔다.</summary>
    [Fact]
    public void DirectoryFollowsRawStepsAndUnc()
    {
        var names = new RawFileName();
        Assert.Equal("\\a\\b\\"u8.ToArray(), names.Directory("C:\\a\\b\\c.txt"u8.ToArray(), -1).ToArray());
        Assert.Equal("\\a\\"u8.ToArray(), names.Directory("C:\\a\\b\\c.txt"u8.ToArray(), 0).ToArray());
        Assert.Equal("\\b\\"u8.ToArray(), names.Directory("C:\\a\\b\\c.txt"u8.ToArray(), 1).ToArray());
        Assert.Equal("\\share\\"u8.ToArray(), names.Directory("\\\\srv\\share\\f.txt"u8.ToArray(), -1).ToArray());
    }

    /// <summary>구분자가 하나도 없으면 현재 칸을 보존한 채 거부한다.</summary>
    [Fact]
    public void DirectoryWithoutSeparatorThrows()
    {
        var names = new RawFileName();
        Assert.Throws<ArgumentException>(() => names.Directory("game.exe"u8.ToArray(), -1));
        Assert.Throws<ArgumentOutOfRangeException>(() => names.Directory(Enumerable.Repeat((byte)'x', 256).ToArray(), -1));
    }

    /// <summary>원문 전체 복사는 .\\인지 검사하지 않는다.</summary>
    [Fact]
    public void FullCopiesWholeOrPlusTwoBytes()
    {
        var names = new RawFileName();
        Assert.Equal("C:\\a.txt"u8.ToArray(), names.Full("C:\\a.txt"u8.ToArray()).ToArray());
        Assert.Equal("C:\\a.txt"u8.ToArray(), names.Full("XXC:\\a.txt"u8.ToArray(), false).ToArray());
        Assert.Equal("C:\\a.txt"u8.ToArray(), names.Full(Encoding.Latin1.GetBytes("C:\\a.txt\0junk")).ToArray());
        Assert.Throws<ArgumentException>(() => names.Full(new byte[] { (byte)'X' }, false));
        Assert.Throws<ArgumentOutOfRangeException>(() => names.Full(Enumerable.Repeat((byte)'x', 256).ToArray()));
    }
}
