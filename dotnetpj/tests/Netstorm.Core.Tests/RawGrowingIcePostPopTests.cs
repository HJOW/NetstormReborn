using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 성장형 아이스 최초 후처리의 원문 P212행을 다른 성장/삭제/공격 몸체와 구별해 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>최초 확보 뒤 현재 TYPE/프레임 수와 같은 물리/공통 부모 순서를 검사한다.</summary>
public sealed class RawGrowingIcePostPopTests
{
    /// <summary>명시 입력 개수의 원문 프레임 코드 표를 만든다.</summary>
    private static RiftTypeFrames Frames(uint count) => new(Enumerable.Range(0, (int)count).Select(index => new RiftFrameCode((byte)'A', 0, (byte)index, 0)));
    /// <summary>실제 raw DWORD 하나를 저장한다.</summary>
    private static void Put(RawSidPool pool, int sid, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid)[offset..], value);
    /// <summary>212개 원문 P 행의 전체 trace·세 슬롯·반환 기록을 그대로 비교한다.</summary>
    [Fact]
    public void NativeGrowingPostPop212()
    {
        int count = 0;
        // 원문 최초 후처리만 선택하며 다른 성장/파괴/재생성 행은 실행하지 않는다.
        foreach (string[] row in X86Fixture.Read("ice-tower"))
        {
            if (row[0] != "P") continue;
            Assert.Equal(5, row.Length); uint[] input = row[1].Split(',').Select(X86Fixture.UInt).ToArray(); Assert.Equal(5, input.Length);
            var pool = new RawSidPool(32768); var frames = Enumerable.Range(0, 188).Select(_ => Frames(0)).ToArray(); frames[150] = Frames(input[2]); frames[151] = Frames(9);
            // 원문에 제공한 세 슬롯의 전체50BYTE 입력이다.
            foreach (int sid in new[] { 50, 60, 61 })
            {
                Span<byte> raw = pool.RawSlot(sid); raw.Clear(); Put(pool, sid, 0, sid == 61 ? 0x12200100u : 0x12200000u);
                raw[10] = sid == 50 ? (byte)150 : (byte)0; raw[11] = sid == 61 ? (byte)1 : (byte)0; raw[34] = sid == 61 ? (byte)0 : (byte)2;
                if (sid == 50)
                {
                    BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], 0xa55a); Put(pool, sid, 14, BitConverter.SingleToUInt32Bits(20.25f)); Put(pool, sid, 18, BitConverter.SingleToUInt32Bits(21.5f)); raw[35] = 0x44; raw[40] = (byte)input[1]; Put(pool, sid, 36, 4);
                }
            }
            var events = new List<string>();
            RawGrowingIcePostPop.PostPop(pool, frames, 50, input[0],
                () => { events.Add("N:40"); if (input[4] == 1) pool.RawSlot(50)[10] = 151; return input[3] != 0; },
                (sid, value, period) => { events.Add($"R:{value}:{sid}:{BitConverter.SingleToUInt32Bits(period)}"); if (input[4] == 10) { Put(pool, 50, 14, BitConverter.SingleToUInt32Bits(61)); Put(pool, 50, 18, BitConverter.SingleToUInt32Bits(62)); } },
                (sid, flags) => { Assert.Equal(50, sid); events.Add("P:" + flags); });
            Assert.Equal(row[2], string.Join(';', events)); Assert.Equal(row[3], string.Concat(new[] { 50, 60, 61 }.Select(sid => Convert.ToHexStringLower(pool.Slot(sid))))); Assert.Equal("0", row[4]); count++;
        }
        Assert.Equal(212, count);
    }
    /// <summary>확보 실패는 현재 TYPE/프레임·생성자 누락을 요구하지 않고 공통 부모로 간다.</summary>
    [Fact]
    public void FailedReserveSkipsCurrentTypeAndConstructor()
    {
        var pool = new RawSidPool(24000); pool.RawSlot(50)[10] = 255; var frames = Enumerable.Range(0, 188).Select(_ => Frames(0)).ToArray(); var events = new List<string>();
        RawGrowingIcePostPop.PostPop(pool, frames, 50, 1, () => { events.Add("A"); pool.RawSlot(50)[11] = 9; return false; }, null, (_, _) => events.Add("P"));
        Assert.Equal(new[] { "A", "P" }, events); Assert.Equal(9, pool.Slot(50)[11]);
    }
    /// <summary>현재 프레임 TYPE 오류는 성공 확보 뒤 생성자 누락보다 먼저 발생한다.</summary>
    [Fact]
    public void CurrentTypeLookupPrecedesMissingConstructor()
    {
        var pool = new RawSidPool(24000); var frames = Enumerable.Range(0, 188).Select(_ => Frames(0)).ToArray(); int calls = 0;
        Assert.Throws<ArgumentOutOfRangeException>(() => RawGrowingIcePostPop.PostPop(pool, frames, 50, 1, () => { calls++; pool.RawSlot(50)[10] = 255; return true; }, null, null)); Assert.Equal(1, calls);
    }
    /// <summary>최초가 아니면 아직 초기화하지 않은 raw와 확보 장치를 읽지 않는다.</summary>
    [Fact]
    public void NoFirstSkipsReserveAndSlot()
    {
        var pool = new RawSidPool(24000); var frames = Enumerable.Range(0, 188).Select(_ => Frames(0)).ToArray(); int calls = 0;
        RawGrowingIcePostPop.PostPop(pool, frames, 50, 4, null, null, (_, flags) => { Assert.Equal(4u, flags); calls++; }); Assert.Equal(1, calls);
    }
}
