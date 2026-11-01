using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Client.Tests;

/// <summary>표시 정렬의 전체 16바이트 항목 순서를 독립 원본 CRT 관측과 대조한다.</summary>
public sealed class DrawSortNativeTests
{
    /// <summary>원본 little-endian 항목 배열을 숫자 변환 없이 같은 비트로 읽는다.</summary>
    private static RenderCollectionEntry[] Read(string hex)
    {
        if (hex == "-") return [];
        byte[] bytes = Convert.FromHexString(hex); Assert.Equal(0, bytes.Length % 16);
        var entries = new RenderCollectionEntry[bytes.Length / 16];
        // 각 항목의 float 비트와 부가 바이트를 원본 필드 위치에서 읽는다.
        for (int i = 0; i < entries.Length; i++)
        {
            var part = bytes.AsSpan(i * 16, 16);
            entries[i] = new(BitConverter.Int32BitsToSingle(BinaryPrimitives.ReadInt32LittleEndian(part)),
                BitConverter.Int32BitsToSingle(BinaryPrimitives.ReadInt32LittleEndian(part[4..])),
                BinaryPrimitives.ReadUInt32LittleEndian(part[8..]), BinaryPrimitives.ReadInt16LittleEndian(part[12..]), part[14], part[15]);
        }
        return entries;
    }

    /// <summary>정렬된 필드 전체를 원본 관측과 같은 바이트 배열로 직렬화한다.</summary>
    private static string Pack(RenderCollectionEntry[] entries)
    {
        if (entries.Length == 0) return "-";
        byte[] bytes = new byte[entries.Length * 16];
        // 항목별 키와 토큰·깊이·종류·tail을 모두 기록한다.
        for (int i = 0; i < entries.Length; i++)
        {
            var part = bytes.AsSpan(i * 16, 16); var entry = entries[i];
            BinaryPrimitives.WriteInt32LittleEndian(part, BitConverter.SingleToInt32Bits(entry.X));
            BinaryPrimitives.WriteInt32LittleEndian(part[4..], BitConverter.SingleToInt32Bits(entry.Y));
            BinaryPrimitives.WriteUInt32LittleEndian(part[8..], entry.Token); BinaryPrimitives.WriteInt16LittleEndian(part[12..], entry.Depth);
            part[14] = entry.Kind; part[15] = entry.Tail;
        }
        return Convert.ToHexString(bytes).ToLowerInvariant();
    }

    /// <summary>세 원본의 정렬 및 wrapper 끔 행을 전체 출력 바이트와 대조한다.</summary>
    [Theory]
    [InlineData("originals", 325)]
    [InlineData("originalCD", 415)]
    [InlineData("original1037", 415)]
    public void OriginalCrtOrderMatches(string version, int expected)
    {
        int count = 0;
        // 해당 판본의 모든 독립 관측을 실행하고 원본이 정렬을 생략한 wrapper도 보존한다.
        foreach (string[] row in X86Fixture.Read("drawsort").Where(row => row[0] == version))
        {
            Assert.Equal(8, row.Length); var input = row[2].Split(','); var entries = Read(row[3]);
            Assert.Equal(int.Parse(input[0]), entries.Length);
            if (row[1] != "Wrapper" || input[3] != "0") DrawSort.Sort(entries, version == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            Assert.Equal(row[4], Pack(entries)); count++;
        }
        Assert.Equal(expected, count);
    }

    /// <summary>유효하지 않은 좌표가 뒤에 있어도 정렬 전에 전체 입력이 보존된다.</summary>
    [Fact]
    public void InvalidCoordinateDoesNotPartiallySort()
    {
        var entries = new[] { new RenderCollectionEntry(0, 0, 1, 0, 2, 3), new RenderCollectionEntry(float.NaN, 1, 2, 5, 4, 6) };
        string before = Pack(entries); Assert.Throws<ArgumentException>(() => DrawSort.Sort(entries)); Assert.Equal(before, Pack(entries));
    }
}
