using System.Globalization;
using System.Runtime.InteropServices;
using Netstorm.Assets;

namespace Netstorm.Tests; // 같은 장기 고리의 전체 장치 조회 물리를 대조한다.

/// <summary>독립32관측과 실제10칸 alias/실패 prefix다.</summary>
public sealed class RawFileNameDeviceTests
{
    /// <summary>Native32행의 현재256BYTE·같은 반환 주소·원시 입력 불변이다.</summary>
    [Fact]
    public void WholeNative32SlotsAndOriginalInput()
    {
        var rows = File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "file-name-device-x86.tsv")).Where(line => line.Length != 0 && line[0] != '#').Select(line => line.Split('\t')).ToArray(); Assert.Equal(32, rows.Length);
        // 원문 기대값은 제품 실행 뒤 생성하거나 정규화하지 않는다.
        foreach (var row in rows)
        {
            var names = new RawFileName(); byte[] input = row[1] == "-" ? [] : Convert.FromHexString(row[1]), original = input.ToArray(), expected = Convert.FromHexString(row[2]); var result = names.Device(input, byte.Parse(row[0], CultureInfo.InvariantCulture));
            Assert.Equal(1, names.Index); Assert.Equal(expected, names.CurrentBytes.ToArray()); Assert.Equal(expected.AsSpan(0, Array.IndexOf(expected, (byte)0)).ToArray(), result.ToArray()); Assert.Equal(original, input);
            Assert.True(MemoryMarshal.TryGetArray(names.CurrentBytes, out var slot)); Assert.True(MemoryMarshal.TryGetArray(result, out var returned)); Assert.Same(slot.Array, returned.Array); Assert.Equal(slot.Offset, returned.Offset);
        }
    }
    /// <summary>열번째 칸 alias는 선행 clear를 보고 물리 실패도 현재 칸을 전진/clear한다.</summary>
    [Fact]
    public void SameTenSlotAliasAndPhysicalOverflowKeepClear()
    {
        var names = new RawFileName(); var first = names.Device("C:\\first"u8.ToArray()); Assert.Equal("C:"u8.ToArray(), first.ToArray());
        for (int index = 0; index < 9; index++) names.Format([]); // 다른9칸 뒤 같은 출력 칸을 다시 선택한다.
        Assert.Equal(0, names.Index); var alias = names.Device(first); Assert.Empty(alias.ToArray()); Assert.Equal(1, names.Index); Assert.All(names.CurrentBytes.ToArray(), value => Assert.Equal(0, value));
        Assert.Throws<ArgumentOutOfRangeException>(() => names.Device(Enumerable.Repeat((byte)'x', 256).ToArray())); Assert.Equal(2, names.Index); Assert.All(names.CurrentBytes.ToArray(), value => Assert.Equal(0, value));
    }
}
