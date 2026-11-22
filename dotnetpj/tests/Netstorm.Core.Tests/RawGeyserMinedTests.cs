using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using static Netstorm.Core.Tests.RawPathProcessTests;

// 두 보호 PE의 독립 관측을 같은 원문 입력으로 재생한다.
namespace Netstorm.Core.Tests;

/// <summary>채집 handler의 전체 raw/생성물/난수/효과 순서를 원본 관측에 대조한다.</summary>
public sealed class RawGeyserMinedTests
{
    /// <summary>선택한 DWORD만 little endian으로 쓴다.</summary>
    private static void Dword(RawSidPool pool, int sid, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid)[offset..], value);
    /// <summary>선택한 수량 WORD만 쓴다.</summary>
    private static void Quantity(RawSidPool pool, int sid, uint value) => BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(sid)[12..], unchecked((ushort)value));
    /// <summary>음향 이후 변이의 입력 좌표다. 원본 기대 출력은 계산하지 않는다.</summary>
    private static (float X, float Y) MutationPoint(uint mutation) => mutation switch
    {
        4 => (-.25f, 255.75f), 5 => (256f, 41.25f), 6 => (BitConverter.UInt32BitsToSingle(0x7fc00000), 41.25f),
        7 => (4294967296f, 0), 8 => (-4294967296f, 0), 9 => (float.PositiveInfinity, 0), _ => throw new ArgumentOutOfRangeException(nameof(mutation)),
    };
    /// <summary>판본마다1156입력의 슬롯 전체(격리VT 제외)와 생성물·난수·외부 순서가 원본과 같다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    public void Native2312ObservationsKeepWordFtolRandomAndCurrentEffects(string editionName)
    {
        string[][] rows = Rows("geysermined-x86.tsv").Where(row => row[0] == editionName).ToArray(); Assert.Equal(1156, rows.Length);
        // 기대 결과는 독립 PE 자료에서만 읽고 각 입력마다 실제 풀/표/난수를 새로 만든다.
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length); uint[] input = row[1].Split(',').Select(text => uint.Parse(text, CultureInfo.InvariantCulture)).ToArray(); Assert.Equal(7, input.Length);
            uint quantity = input[0], cost = input[1], seed = input[2], free = input[3], boss = input[4], surface = input[5], mutation = input[6]; OriginalEdition edition = Edition(editionName);
            int nativeWidth = edition == OriginalEdition.Patch1078 ? RawSidPool.Stride : RawSidPool.CdStride;
            var pool = new RawSidPool(32768, edition: edition); var hash = new RawSquidHash(); var types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
            // 원본 입력과 같은5..8 client 슬롯을 순서대로 확보한다.
            for (int sid = 5; sid <= 8; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
            pool.RawSlot(7).Clear(); pool.RawSlot(6).Clear(); pool.RawSlot(6)[10] = 122; pool.RawSlot(6)[11] = (byte)free;
            Quantity(pool, 6, quantity); Dword(pool, 6, 14, BitConverter.SingleToUInt32Bits(40.75f)); Dword(pool, 6, 18, BitConverter.SingleToUInt32Bits(41.25f));
            types[154].Cost = BitConverter.UInt32BitsToSingle(cost); pool.RawSlot(8)[10] = (byte)surface; hash.Cell(0, 40, 41) = surface != 0 ? (ushort)8 : (ushort)0;
            // 기존 관리 CD 풀은50BYTE이므로 원본36BYTE 뒤14BYTE를 별도 보호 영역으로 검증한다.
            pool.RawSlot(6)[nativeWidth..].Fill(0xa5); pool.RawSlot(7)[nativeWidth..].Fill(0xa5);
            var random = new GameRandom(seed); var state = new GeyserMinedState { Boss = boss != 0 }; var events = new List<string>();
            var handler = new RawGeyserMined(pool, hash, types, random, state, new()
            {
                Sound = (x, y, name, flags) =>
                {
                    Assert.StartsWith("geyserMined", name); Assert.EndsWith(".wav", name); Assert.Equal(0u, flags);
                    events.Add($"S:{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)},{name[11]},{flags}");
                    if (mutation == 1) { Quantity(pool, 6, 400); state.NuggetType = 155; types[155].Cost = 199.5f; state.Boss = true; }
                    else if (mutation == 2) { Dword(pool, 6, 14, BitConverter.SingleToUInt32Bits(42.25f)); Dword(pool, 6, 18, BitConverter.SingleToUInt32Bits(43.75f)); Quantity(pool, 6, 200); }
                    else if (mutation is >= 4 and <= 9) { var point = MutationPoint(mutation); Dword(pool, 6, 14, BitConverter.SingleToUInt32Bits(point.X)); Dword(pool, 6, 18, BitConverter.SingleToUInt32Bits(point.Y)); Quantity(pool, 6, 200); }
                    else if (mutation >= 10) { pool.RawSlot(6)[10] = 82; pool.RawSlot(6)[edition == OriginalEdition.Patch1078 ? 40 : 35] = mutation >= 12 ? (byte)8 : (byte)0; state.Checking.Set(mutation is 11 or 12); }
                },
                Destroy = (sid, flags) =>
                {
                    Assert.Equal(6, sid); Assert.Equal(0x1000u, flags); events.Add($"D:{flags}"); pool.RawSlot(6)[11] |= 2;
                    if (mutation == 3) { int x = (int)BinaryPrimitives.ReadSingleLittleEndian(pool.Slot(6)[14..]), y = (int)BinaryPrimitives.ReadSingleLittleEndian(pool.Slot(6)[18..]); hash.Cell(0, x, y) = surface == 97 ? (ushort)0 : (ushort)8; pool.RawSlot(8)[10] = 97; }
                },
                Create = (type, flags) => { Assert.Equal(84u, type); Assert.Equal(0u, flags); events.Add($"C:{type},{flags}"); pool.RawSlot(7)[..nativeWidth].Clear(); pool.RawSlot(7)[10] = (byte)type; pool.RawSlot(7)[11] = 4; return 7; },
                Pop = (sid, x, y, flags) => { Assert.Equal(7, sid); Assert.Equal(0u, flags); events.Add($"P:{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)},{flags}"); Dword(pool, sid, 14, BitConverter.SingleToUInt32Bits(x)); Dword(pool, sid, 18, BitConverter.SingleToUInt32Bits(y)); pool.RawSlot(sid)[11] = 0; },
            });
            if (mutation is >= 4 and <= 9) { int x = mutation == 5 ? 40 : 0, y = mutation == 4 ? 255 : mutation <= 6 ? 41 : 0; hash.Cell(0, x, y) = surface != 0 ? (ushort)8 : (ushort)0; }
            byte[] packet = [0xab, 0xcd, 0xef, 0xff, 6, 0];
            if (row[3].EndsWith(";A", StringComparison.Ordinal)) { Assert.Throws<InvalidOperationException>(() => handler.HandlePacket(packet)); events.Add("A"); } else handler.HandlePacket(packet);
            Assert.Equal(uint.Parse(row[2], CultureInfo.InvariantCulture), random.State()); Assert.Equal(row[3], events.Count == 0 ? "-" : string.Join(';', events));
            byte[] expected = Convert.FromHexString(row[4]), born = Convert.FromHexString(row[5]); Assert.Equal(nativeWidth, expected.Length); Assert.Equal(nativeWidth, born.Length);
            Assert.Equal(expected.AsSpan(4).ToArray(), pool.Slot(6).Slice(4, nativeWidth - 4).ToArray()); Assert.Equal(born.AsSpan(4).ToArray(), pool.Slot(7).Slice(4, nativeWidth - 4).ToArray());
            Assert.All(pool.Slot(6)[nativeWidth..].ToArray(), value => Assert.Equal((byte)0xa5, value)); Assert.All(pool.Slot(7)[nativeWidth..].ToArray(), value => Assert.Equal((byte)0xa5, value));
        }
    }
}
