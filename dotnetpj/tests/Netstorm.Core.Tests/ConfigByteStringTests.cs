using Netstorm.Core.Original;

// 고정 BYTE 목적지의 strcmp/strcpy 시점과 부분 효과를 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>ConfigInterface의 원문 BYTE 문자열 읽기를 실제 설정 저장소로 검사한다.</summary>
public sealed class ConfigByteStringTests
{
    /// <summary>누락 값은 목적지를 읽지 않으며 같은 문자열은 첫NUL 뒤 물리를 요구하지 않는다.</summary>
    [Fact]
    public void MissingAndEqualValuesDoNotReadUnusedDestination()
    {
        var config = new ConfigInterface(null);
        Assert.False(config.ReadStringBytes("missing", Span<byte>.Empty));
        config.Set("address", "abc");
        byte[] destination = [97, 98, 99, 0, 0xa5];
        Assert.False(config.ReadStringBytes("address", destination.AsSpan(0, 4)));
        Assert.Equal(0xa5, destination[4]);
        config.Set("empty", "");
        Assert.False(config.ReadStringBytes("empty", new byte[1]));
        Assert.Throws<ArgumentOutOfRangeException>(() => config.ReadStringBytes("empty", []));
    }
    /// <summary>첫 불일치 뒤 Windows1252/NUL만 복사하고 나머지 꼬리를 보존한다.</summary>
    [Fact]
    public void ChangedValueCopiesCp1252AndOnlyThroughFirstNul()
    {
        var config = new ConfigInterface(null); config.Set("address", "é€");
        byte[] destination = [1, 2, 3, 4, 5];
        Assert.True(config.ReadStringBytes("address", destination));
        Assert.Equal(new byte[] { 0xe9, 0x80, 0, 4, 5 }, destination);
        config.Set("address", "");
        Assert.True(config.ReadStringBytes("address", destination));
        Assert.Equal(new byte[] { 0, 0x80, 0, 4, 5 }, destination);
    }
    /// <summary>비교 물리 실패는 쓰지 않고 복사 물리 실패는 앞서 쓴 접두를 되돌리지 않는다.</summary>
    [Fact]
    public void ShortDestinationKeepsComparisonAndCopyFailurePrefixes()
    {
        var config = new ConfigInterface(null); config.Set("address", "abcd");
        byte[] equalPrefix = [97, 98, 99];
        Assert.Throws<ArgumentOutOfRangeException>(() => config.ReadStringBytes("address", equalPrefix));
        Assert.Equal(new byte[] { 97, 98, 99 }, equalPrefix);
        byte[] different = [0xa5, 0xa5, 0xa5];
        Assert.Throws<ArgumentOutOfRangeException>(() => config.ReadStringBytes("address", different));
        Assert.Equal(new byte[] { 97, 98, 99 }, different);
    }
}
