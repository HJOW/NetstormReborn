using System.Text;
using Netstorm.Assets;
using Netstorm.Client;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 같은 Client 고리와 VFS 경로 고리의 공유를 대조한다.

// cpppj client/ClientStaticStrings와 SavedStateFiles의 실제 소비자 경로 계약이다. 0.985에서 추가됐다.
public sealed class ClientStaticStringsTests
{
    /// <summary>같은 고리의 네 DWORD가 0에서 시작한다.</summary>
    [Fact]
    public void GlobalsStartAtZero()
    {
        var owner = new ClientStaticStrings();
        Assert.Equal(0u, owner.Globals().Size);
        Assert.Equal(0u, owner.Globals().Base);
        Assert.Equal(0u, owner.Globals().Current);
        Assert.Equal(0u, owner.Globals().Last);
    }

    /// <summary>실제 Client는 VFS의 같은 경로 고리를 빌린다.</summary>
    [Fact]
    public void SharesFileNameOwnerWithVfs()
    {
        var files = new RawFileName();
        var owner = new ClientStaticStrings(files);
        Assert.Same(files, owner.FileNames());
    }

    /// <summary>초기화 뒤 예약은 끝까지 같은 물리를 빌린다.</summary>
    [Fact]
    public void ReserveReturnsTailPhysics()
    {
        var owner = new ClientStaticStrings();
        owner.Initialize(64);
        Memory<byte> tail = owner.Reserve(8);
        Assert.True(tail.Length >= 8);
        tail.Span[0] = 0xab;
        Assert.Equal(0xab, tail.Span[0]);
        Assert.Equal(0xd5, owner.Reserve(0).Span[0]);
    }

    /// <summary>정수 서식은 원문 CRT와 같은 한 변환만 수행한다.</summary>
    [Fact]
    public void FormatIntegerWritesSingleConversion()
    {
        var owner = new ClientStaticStrings();
        owner.Initialize(256);
        ReadOnlyMemory<byte> pattern = "%d items"u8.ToArray();
        ReadOnlyMemory<byte> result = owner.FormatInteger(32, pattern, 42);
        int nul = result.Span.IndexOf((byte)0);
        Assert.Equal("42 items", Encoding.Latin1.GetString(result.Span[..nul]));
    }

    /// <summary>서식 밖 %는 연결하지 않는다.</summary>
    [Fact]
    public void FormatIntegerRejectsUnsupportedPattern()
    {
        var owner = new ClientStaticStrings();
        owner.Initialize(256);
        Assert.Throws<InvalidOperationException>(() => { owner.FormatInteger(32, "%s"u8.ToArray(), 1); });
        Assert.Throws<InvalidOperationException>(() => { owner.FormatInteger(32, "%d %d"u8.ToArray(), 1); });
    }

    /// <summary>완료 안내는 고정 Wrote 서식으로 두 파일명을 쓴다.</summary>
    [Fact]
    public void FormatCaptureRangeWritesFixedMessage()
    {
        var owner = new ClientStaticStrings();
        owner.Initialize(10240);
        ReadOnlyMemory<byte> result = owner.FormatCaptureRange("a.dat", "b.dat");
        int nul = result.Span.IndexOf((byte)0);
        Assert.Equal("Wrote a.dat through b.dat.\n", Encoding.Latin1.GetString(result.Span[..nul]));
    }

    /// <summary>같은 경로 고리로 원문 상대 이름을 계산하고 사용자 파일로 바꾼다.</summary>
    [Fact]
    public void SavedStatePathUsesSameRingAndUserFile()
    {
        var formatter = new RawFileName();
        string gameDirectory = Path.GetFullPath(".");
        string userFile = Path.Combine(Path.GetTempPath(), "unit-!state.dat");
        string path = SavedStateFiles.SavedStateFilePath(gameDirectory, formatter, userFile);
        Assert.Equal(Path.GetFullPath(userFile), path);
        string install = SavedStateFiles.SavedStateFilePath(gameDirectory, formatter);
        Assert.Equal(Path.GetFullPath(Path.Combine(gameDirectory, "d", "!state.dat")), install);
    }

    /// <summary>같은 고리의 Format 결과를 사용자 경로 계산이 덮어쓰지 않는다.</summary>
    [Fact]
    public void SavedStatePathKeepsRingNumber()
    {
        var formatter = new RawFileName();
        int before = formatter.Index;
        SavedStateFiles.SavedStateFilePath(Path.GetFullPath("."), formatter, Path.GetTempFileName());
        Assert.Equal((before + 1) % 10, formatter.Index);
    }
}
