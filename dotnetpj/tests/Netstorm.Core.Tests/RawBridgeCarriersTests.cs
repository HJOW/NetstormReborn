using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 PE의 독립 관측으로 실제 탐색·대피 선택을 확인한다.
namespace Netstorm.Core.Tests;

/// <summary>CPP와 같은 258개 물리 입력의 사건 순서와 네 전체 슬롯을 대조한다.</summary>
public sealed class RawBridgeCarriersTests
{
    /// <summary>원문 입력의 비정렬 DWORD를 little endian으로 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>원문 입력의 현재 위치를 비트 손실 없이 읽는다.</summary>
    private static float Position(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[offset..]));

    /// <summary>스캔 도중 칸 효과와 주기 조회가 바꾼 같은 프레임 시각을 마지막 예약이 읽는다.</summary>
    [Fact]
    public void DecayScanReadsCurrentFrameAfterCellAndPeriodEffects()
    {
        var pool = new RawSidPool(24000);
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        types[82].Flags2 = 4;
        pool.RawSlot(50).Clear(); pool.RawSlot(50)[10] = 82;
        var state = new Netstorm.Core.Original.BridgeDecayScanState { First = 50, Last = 51, Cursor = 50, Next = 1 };
        var clock = new FrameTimeBox { Value = new FrameTime { Game = 2, Delta = 0.1 } };
        int visits = 0;
        var scan = new RawBridgeDecayScan(pool, types, state,
            () => { clock.Value.Game = 30; return 7; },
            () => Netstorm.Core.Bridges.BridgeDecayMode.SinglePlayer,
            _ => { visits++; clock.Value.Game = 20; state.Next = 15; });
        scan.Frame(clock, false);
        Assert.Equal(1, visits); Assert.Equal(50, state.Cursor); Assert.Equal(37, state.Next);
    }

    /// <summary>기대 선택점을 재계산하지 않고 동결된 사건열·바이트와 모든 행을 대조한다.</summary>
    [Fact]
    public void IndependentNative258PreservesNestedFindersAndSlots()
    {
        string[][] rows = X86Fixture.Read("bridge-carrier-retreat");
        Assert.Equal(258, rows.Length);
        // 각 행은 원문 생성기가 사용한 명시 입력만 독립적으로 설치한다.
        foreach (string[] row in rows)
        {
            uint[] c = row[0].Split(',').Select(X86Fixture.UInt).ToArray();
            Assert.Equal(13, c.Length);
            var pool = new RawSidPool(24000);
            var hash = new RawSquidHash();
            OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
            float[][] positions = [[22,21,20,23], [21,21,20,22], [21,22,19,20], [22,21,18,21], [20.125f,21,20,22.125f], [20,21,22,23]];
            float[] points = positions[c[4]];
            int[] ids = [50, 60, 61, 62];
            if (c[10] != 0) (ids[2], ids[3]) = (ids[3], ids[2]);
            // 같은 TYPE·좌표·수명을 심은 뒤 지정 순서로 해시 머리에 삽입한다.
            foreach (int id in ids)
            {
                Span<byte> raw = pool.RawSlot(id); raw.Clear(); Put(raw, 0, 0x11018000);
                uint kind = id == 50 ? 82u : id == 60 ? c[2] : id == 61 ? 70u : 71u;
                raw[10] = (byte)kind;
                types[kind].FootX = types[kind].FootY = 1;
                types[kind].Flags1 = id == 60 ? 0u : id == 61 ? c[6] : 0x800u;
                types[kind].Flags2 = id == 60 ? 0x10000u : id == 62 ? 2u : 4u;
                float x = id == 50 ? 20f : id == 60 ? BitConverter.UInt32BitsToSingle(c[11]) : id == 61 ? points[0] : points[2];
                float y = id == 50 ? 21f : id == 60 ? BitConverter.UInt32BitsToSingle(c[12]) : id == 61 ? points[1] : points[3];
                Put(raw, 14, BitConverter.SingleToUInt32Bits(x)); Put(raw, 18, BitConverter.SingleToUInt32Bits(y));
                raw[30] = (byte)(id == 60 ? 254u : id == 61 ? c[7] : 1u);
                BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], (ushort)(id == 50 ? 40u : id == 61 ? c[5] * 8u : 0u));
                ref ushort head = ref hash.Bucket(id == 60 ? 1 : 0, x, y);
                BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], head); head = (ushort)id;
            }
            var events = new List<string>();
            var hooks = new BridgeCarrierHooks
            {
                GraphFor = sid =>
                {
                    ReadOnlySpan<byte> raw = pool.Slot(sid);
                    if ((types[raw[10]].Flags1 & 0x800u) != 0) return raw[30];
                    int x = (int)((double)Position(raw, 14) + 0.9999f), y = (int)((double)Position(raw, 18) + 0.9999f);
                    ushort head = hash.Cell(0, x, y);
                    if (head != 0 && (types[pool.Slot(head)[10]].Flags1 & 0x800u) == 0) events.Add("A:1849");
                    return head != 0 ? pool.Slot(head)[30] : 254u;
                },
                MovementType = type => { events.Add($"T:{type}:0:0"); return 46; },
                MovementForm = (sid, type) => { events.Add($"P:{sid}:{type}:0:0"); return c[3] != 0 ? 900 : 0; },
                Start = (sid, point) => { events.Add($"M:{sid}:{BitConverter.SingleToUInt32Bits(point.X)}:{BitConverter.SingleToUInt32Bits(point.Y)}:0:0:0:0:0"); return c[8] != 0; },
                Valid = point => point.X > 0 && point.Y > 0 && point.X < 256 && point.Y < 256,
                Assertion = line => events.Add($"A:{line}"),
                WontWalkOffEdge = type => type is 158 or 156 or 137 or 135 or 187,
                Scan = area => events.Add($"F:{unchecked((uint)area.Left)}:{unchecked((uint)area.Top)}:{unchecked((uint)area.Right)}:{unchecked((uint)area.Bottom)}"),
            };
            var carriers = new RawBridgeCarriers(pool, hash, types, () => c[1] != 0, new RawCheckingFlag(() => c[9]), () => 158, () => 156, () => 7, hooks);
            if (c[0] != 0) carriers.Check(50); else carriers.Retreat(60);
            Assert.True(string.Join(';', events) == (row[1] == "-" ? "" : row[1]), $"입력 {row[0]}\n{string.Join(';', events)}\n{row[1]}");
            string[] wanted = row[2].Split(',');
            int[] order = [50, 60, 61, 62];
            // 이동 요청의 전후로 원문이 관측한 네 전체 슬롯의 비트도 확인한다.
            for (int index = 0; index < order.Length; index++) Assert.Equal(Convert.FromHexString(wanted[index]), pool.Slot(order[index]).ToArray());
        }
    }
}
