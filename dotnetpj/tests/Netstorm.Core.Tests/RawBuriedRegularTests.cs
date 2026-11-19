// 오벨리스크 정기(프레임 진행·지연)의 분기·나머지·난수를 실제 슬롯·프레임으로 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// 매몰 정기를 검사하는 클래스다. 2026-11-19 0.659에서 R4-4 정기 처리용으로 추가했다.
// 근거: cpppj o/RawBuriedRegular.cpp의 Handle(원문 004622f0)다.
public sealed class RawBuriedRegularTests
{
    /// <summary>실제 풀의 슬롯 수다. 0.659에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 0.659에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int i = 0; i < 4; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>매몰 장면을 만든다. 0.659에서 추가했다.</summary>
    private static (RawSidPool Pool, List<RiftTypeFrames> Frames, SquidFrame Setter, GameRandom Random, int Sid) BuildScene(byte type = 30, uint seed = 1u)
    {
        var pool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Patch1078);
        var types = new OriginalTypeRecord[188];
        // 모든 타입 기록을 빈 값으로 채우는 반복문이다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        var frames = new List<RiftTypeFrames>(188);
        // 모든 타입 번호마다 빈 코드 표를 넣는 반복문이다.
        for (int i = 0; i < 188; ++i)
        {
            frames.Add(new RiftTypeFrames([]));
        }

        var codes = new List<RiftFrameCode>
        {
            new((byte)'A', (byte)'0', 0, 0),
            new((byte)'A', (byte)'0', 1, 0),
            new((byte)'B', (byte)'0', 2, 0),
            new((byte)'B', (byte)'0', 3, 0),
        };
        frames[type] = new RiftTypeFrames(codes);
        var setter = new SquidFrame(pool, types, new SquidFrameHooks
        {
            FrameSize = (_, _) => [1.0f, 1.0f],
            Update = (_, _) => { },
            Unpop = (_, _) => { },
            Pop = (_, _, _, _) => { },
        });
        int sid = pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x0050a748u);
        raw[RawSidPool.TypeOffset] = type;
        raw[RawSidPool.StateOffset] = 0;
        raw[0x21] = 1;
        return (pool, frames, setter, new GameRandom(seed), sid);
    }

    /// <summary>나머지가 있으면 B구간 첫 프레임부터 1씩 진행하고 0.1을 돌려준다. 0.659에서 추가했다.</summary>
    [Fact]
    public void NonZeroRemainderAdvances()
    {
        (RawSidPool pool, List<RiftTypeFrames> frames, SquidFrame setter, GameRandom random, int sid) = BuildScene();
        var body = new RawBuriedRegular(pool, frames, setter, random);
        // B구간 수 2+1=3이며 개수 4의 나머지 1이다. 첫 프레임 2에서 2가 된다.
        Put(pool.AllocatedBytes(sid), 36, 4u);
        Assert.Equal(0.1f, body.Handle(sid, 0, 4, 0.5f));
        Assert.Equal(2, BinaryPrimitives.ReadInt32LittleEndian(pool.Slot(sid).Slice(36, 4)));
        Assert.Equal(random.State(), new GameRandom(1u).State());
    }

    /// <summary>나머지가 0이면 A구간 첫 프레임으로 돌리고 난수를 한 번 소비한다. 0.659에서 추가했다.</summary>
    [Fact]
    public void ZeroRemainderRearmsAndConsumesRandom()
    {
        (RawSidPool pool, List<RiftTypeFrames> frames, SquidFrame setter, GameRandom random, int sid) = BuildScene();
        var body = new RawBuriedRegular(pool, frames, setter, random);
        // B구간 수 2+1=3이며 개수 3의 나머지 0이다. 시드 1의 첫 Next(6)은 1이므로 3.0을 돌려준다.
        Put(pool.AllocatedBytes(sid), 36, 9u);
        Assert.Equal(3.0f, body.Handle(sid, 0, 3, 0.5f));
        Assert.Equal(0, BinaryPrimitives.ReadInt32LittleEndian(pool.Slot(sid).Slice(36, 4)));
        Assert.NotEqual(new GameRandom(1u).State(), random.State());
    }

    /// <summary>0이 아닌 사건은 payload를 그대로 돌려준다. 0.659에서 추가했다.</summary>
    [Fact]
    public void NonZeroEventPassesThrough()
    {
        (RawSidPool pool, List<RiftTypeFrames> frames, SquidFrame setter, GameRandom random, int sid) = BuildScene();
        var body = new RawBuriedRegular(pool, frames, setter, random);
        Assert.Equal(0.9f, body.Handle(sid, 1, 0, 0.9f));
        Assert.Equal(new GameRandom(1u).State(), random.State());
    }

    /// <summary>다른 가상표·CD 판본은 거부한다. 0.659에서 추가했다.</summary>
    [Fact]
    public void OtherVtableAndCdThrow()
    {
        (RawSidPool pool, List<RiftTypeFrames> frames, SquidFrame setter, GameRandom random, int sid) = BuildScene();
        var body = new RawBuriedRegular(pool, frames, setter, random);
        Assert.True(RawBuriedRegular.Handles(pool, sid));
        Put(pool.AllocatedBytes(sid), 0, 0x0050da88u);
        Assert.False(RawBuriedRegular.Handles(pool, sid));
        Assert.Throws<ArgumentException>(() => body.Handle(sid, 0, 0, 0.1f));
        var cdPool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Cd1072);
        Assert.Throws<ArgumentException>(() => new RawBuriedRegular(cdPool, frames, setter, new GameRandom(1u)));
    }

    /// <summary>연결되지 않은 실제 장치는 명시 거부한다. 0.659에서 추가했다.</summary>
    [Fact]
    public void InvalidInputsThrow()
    {
        (RawSidPool pool, List<RiftTypeFrames> frames, SquidFrame setter, GameRandom random, int sid) = BuildScene();
        Assert.Throws<ArgumentNullException>(() => new RawBuriedRegular(pool, null!, setter, random));
        Assert.Throws<ArgumentNullException>(() => new RawBuriedRegular(pool, frames, null!, random));
        var body = new RawBuriedRegular(pool, frames, setter, random);
        Assert.Throws<ArgumentOutOfRangeException>(() => body.Handle(PoolCapacity + 1, 0, 0, 0.1f));
    }
}
