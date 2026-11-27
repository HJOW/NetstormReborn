using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 객체의 현재 가상표와 슬롯으로 다섯 프레임 본체를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>기본·다리·공장과 전용 객체의 현재 프레임 조회 경계다.</summary>
public sealed class SpriteVirtualFramesTests
{
    /// <summary>지정한 실제 슬롯에 가상표·타입·현재 프레임을 기록한다.</summary>
    private static void Set(RawSidPool pool, int sid, uint table, byte type, int frame, byte flags = 0)
    {
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Clear();
        BinaryPrimitives.WriteUInt32LittleEndian(raw, table);
        raw[10] = type;
        raw[40] = flags;
        BinaryPrimitives.WriteInt32LittleEndian(raw[36..], frame);
    }

    /// <summary>같은 슬롯의 가상표와 현재 데이터에 따라 다섯 원문 몸체를 선택한다.</summary>
    [Fact]
    public void DispatchesCurrentVirtualFrameAndChecksActualType()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var definitions = Enumerable.Range(0, 118).Select(_ => new RiftTypeDefinition()).ToArray();
        definitions[0].SpecialFrames.BaseFrame = 9;
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        frames[85] = new([new((byte)'A', 0, 0, 0), new((byte)'B', 0, 0, 0)]);
        frames[131] = new([new((byte)'A', 0, 0, 0), new((byte)'B', 0, 0, 0), new((byte)'B', 0, 1, 0)]);
        var draw = new SpriteVirtualFrames(pool, definitions, frames);

        Set(pool, sid, 0x00501dd0, 70, 4);
        Assert.Equal(9, draw.Read(sid));
        Set(pool, sid, 0x005034c8, 82, -3);
        Assert.Equal(-3, draw.Read(sid));
        Set(pool, sid, 0x00509148, 85, 3);
        pool.AllocatedBytes(sid)[11] = 0x20;
        Assert.Equal(1, draw.Read(sid));
        Set(pool, sid, 0x00509930, 72, 11);
        Assert.Equal(2, draw.Read(sid));
        Set(pool, sid, 0x00509930, 72, 23);
        Assert.Equal(10, draw.Read(sid));
        Set(pool, sid, 0x00509930, 72, 23, 1);
        Assert.Equal(23, draw.Read(sid));
        Set(pool, sid, 0x00509930, 72, 10);
        Assert.Equal(-1, draw.Read(sid));
        Set(pool, sid, 0x0050bad0, 131, 2);
        Assert.Equal(1, draw.Read(sid));
        Set(pool, sid, 0x0050bad0, 131, 3);
        Assert.Throws<ArgumentOutOfRangeException>(() => draw.Read(sid));
        Set(pool, sid, 0x00509930, 73, 11);
        Assert.Throws<ArgumentException>(() => draw.Read(sid));
    }
}
