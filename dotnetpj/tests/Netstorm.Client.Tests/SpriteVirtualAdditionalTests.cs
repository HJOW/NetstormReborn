using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 객체 가상표에 따른 추가 그림 경계를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>원본 빈 함수와 선택된 공장·다리·메타 소비자 분배를 검사한다.</summary>
public sealed class SpriteVirtualAdditionalTests
{
    /// <summary>테스트에서 같은 슬롯의 현재 가상표와 타입을 변경한다.</summary>
    private static void Set(RawSidPool pool, int sid, uint table, byte type, int frame = 0)
    {
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Clear();
        BinaryPrimitives.WriteUInt32LittleEndian(raw, table);
        raw[10] = type;
        BinaryPrimitives.WriteInt32LittleEndian(raw[36..], frame);
    }

    /// <summary>빈 함수만 자연 반환하고 실제 다리·메타 장치와 미복원 함수는 구별한다.</summary>
    [Fact]
    public void RoutesOnlyKnownAdditionalFunctions()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var calls = new List<(string Kind, int Sid, ScreenPoint Point)>();
        var draw = new SpriteVirtualAdditional(pool, frames, _ => throw new InvalidOperationException("예상하지 않은 그림"),
            (id, point) => calls.Add(("bridge", id, point)), (id, point) => calls.Add(("meta", id, point)));
        var point = new ScreenPoint(12, 34);
        Set(pool, sid, 0x00501dd0, 70);
        draw.Draw(sid, point);
        Assert.Empty(calls);
        Set(pool, sid, 0x005034c8, 82);
        draw.Draw(sid, point);
        Set(pool, sid, 0x0050c9f0, 94);
        draw.Draw(sid, point);
        Assert.Equal(new[] { ("bridge", sid, point), ("meta", sid, point) }, calls);
        Set(pool, sid, 0x00502370, 117);
        Assert.Contains("0041E3D0", Assert.Throws<InvalidOperationException>(() => draw.Draw(sid, point)).Message);
        var missing = new SpriteVirtualAdditional(pool, frames, _ => { });
        Set(pool, sid, 0x005034c8, 82);
        Assert.Throws<InvalidOperationException>(() => missing.Draw(sid, point));
    }

    /// <summary>공장 본체의 실제 억제와 현재 프레임을 같은 픽셀 그림 소비자에 전달한다.</summary>
    [Fact]
    public void FactoryAdditionalUsesActualCurrentFrameAndSuppression()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        frames[85] = new([new((byte)'A', 0, 0, 0), new((byte)'A', 0, 1, 0)]);
        var shapes = new List<SpriteShapeRequest>();
        var draw = new SpriteVirtualAdditional(pool, frames, shapes.Add);
        Set(pool, sid, 0x00509148, 85, 1);
        draw.Draw(sid, new(7, 8));
        Assert.Equal(new SpriteShapeRequest(85, 1, new(7, 8), SpriteShapeMode.Plain), Assert.Single(shapes));
        pool.AllocatedBytes(sid)[40] = 1;
        draw.Draw(sid, new(7, 8));
        Assert.Single(shapes);
        Set(pool, sid, 0x00509148, 85, -2);
        Assert.Throws<ArgumentOutOfRangeException>(() => draw.Draw(sid, new(7, 8)));
    }
}
