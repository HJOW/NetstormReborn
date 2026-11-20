using System.Buffers.Binary;
using System.Runtime.InteropServices;
using Netstorm.Core.Original;

// 최초 Native fixture의 전체 물리로 작은 State 자식의 폭과 이웃 보존을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>Zone10입력과 색/붕괴스캔64입력을 기대 재계산 없이 전수 대조한다.</summary>
public sealed class StateStorageResetTests
{
    /// <summary>단일 원문 fixture의 비주석 행을 읽는다.</summary>
    private static IEnumerable<string[]> Rows(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name)).Where(l => l.Length != 0 && l[0] != '#').Select(l => l.Split('\t'));
    /// <summary>같은 입력64비트를 double로 해석한다.</summary>
    private static double Double(byte[] bytes, int offset) => BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(bytes.AsSpan(offset, 8)));

    /// <summary>원문10행의 전체24BYTE와 짧은 뷰의 부분DWORD 효과를 검사한다.</summary>
    [Fact]
    public void ZoneMatchesTenNativeRowsAndShortStorePrefixes()
    {
        int rows = 0;
        // 두 제어어와 모든 최초 입력의 전체 관측 바이트를 비교한다.
        foreach (var row in Rows("state-zone-reset-x86.tsv"))
        {
            int seed = int.Parse(row[0]); byte[] bytes = Enumerable.Range(0, 24).Select(i => unchecked((byte)(seed + i * 17))).ToArray(); RawStateZoneReset.Reset(bytes);
            Assert.Equal(row[7], Convert.ToHexString(bytes).ToLowerInvariant()); Assert.Equal("0:4:0;4:2:0", row[9]); rows++;
        }
        Assert.Equal(10, rows);
        // 각 실제 store가 완료한 폭만 남기고 인접WORD를 덮지 않는다.
        for (int length = 0; length <= 8; length++)
        {
            byte[] bytes = Enumerable.Repeat((byte)0xa5, 10).ToArray(); if (length < 6) Assert.Throws<ArgumentOutOfRangeException>(() => RawStateZoneReset.Reset(bytes.AsSpan(0, length))); else RawStateZoneReset.Reset(bytes.AsSpan(0, length));
            int changed = length < 4 ? 0 : length < 6 ? 4 : 6;
            Assert.Equal(Enumerable.Range(0, 10).Select(i => i < changed ? (byte)0 : (byte)0xa5), bytes);
        }
    }

    /// <summary>원문64입력의 전체212BYTE와 색/붕괴스캔 저장 폭·주소를 비교한다.</summary>
    [Fact]
    public void BridgeResetsMatchSixtyFourNativeRowsIncludingNeighbors()
    {
        int rows = 0;
        // 실제 입력212BYTE의 변경 영역만 직역 몸체에 연결한다.
        foreach (var row in Rows("state-bridge-reset-x86.tsv"))
        {
            byte[] actual = Convert.FromHexString(row[3]); Assert.Equal(212, actual.Length); var stores = row[5].Split(';');
            if (row[0] == "colors")
            {
                RawStateBridgeReset.ResetOwnerColors(MemoryMarshal.Cast<byte, int>(actual.AsSpan(16, 36))); Assert.Equal(9, stores.Length);
                // 최초 관측의 주소/폭도 모든 아홉칸에서 확인한다.
                for (int i = 0; i < 9; i++) { var store = stores[i].Split(':'); Assert.Equal(0x531b08u + (uint)i * 4, uint.Parse(store[0])); Assert.Equal("4", store[1]); }
            }
            else
            {
                Assert.Equal("scan", row[0]); var scan = new BridgeDecayScanState { First = BinaryPrimitives.ReadInt32LittleEndian(actual.AsSpan(88, 4)), Last = BinaryPrimitives.ReadInt32LittleEndian(actual.AsSpan(92, 4)), Cursor = BinaryPrimitives.ReadInt32LittleEndian(actual.AsSpan(124, 4)), Next = Double(actual, 140) };
                float period = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(actual.AsSpan(84, 4))); RawStateBridgeReset.ResetDecayScan(scan, period, Double(actual, 188));
                BinaryPrimitives.WriteInt32LittleEndian(actual.AsSpan(88, 4), scan.First); BinaryPrimitives.WriteInt32LittleEndian(actual.AsSpan(92, 4), scan.Last); BinaryPrimitives.WriteInt32LittleEndian(actual.AsSpan(124, 4), scan.Cursor); BinaryPrimitives.WriteUInt64LittleEndian(actual.AsSpan(140, 8), BitConverter.DoubleToUInt64Bits(scan.Next));
                uint[] addresses = [0x52f96c, 0x52f970, 0x5453a8, 0x5453b8]; Assert.Equal(4, stores.Length);
                // 원문 세DWORD 뒤 QWORD의 실제 쓰기 순서를 대조한다.
                for (int i = 0; i < 4; i++) { var store = stores[i].Split(':'); Assert.Equal(addresses[i], uint.Parse(store[0])); Assert.Equal(i == 3 ? "8" : "4", store[1]); }
            }
            Assert.Equal(row[4], Convert.ToHexString(actual).ToLowerInvariant()); rows++;
        }
        Assert.Equal(64, rows); int[] shortColors = Enumerable.Repeat(-123, 10).ToArray(); Assert.Throws<ArgumentException>(() => RawStateBridgeReset.ResetOwnerColors(shortColors.AsSpan(0, 8))); Assert.All(shortColors, v => Assert.Equal(-123, v)); RawStateBridgeReset.ResetOwnerColors(shortColors); Assert.Equal(-123, shortColors[9]);
    }
}
