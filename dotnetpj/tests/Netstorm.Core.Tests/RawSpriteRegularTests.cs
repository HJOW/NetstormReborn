// 스프라이트 정기(프레임 진행)의 분기·나머지·반환을 실제 슬롯·프레임으로 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// 스프라이트 정기를 검사하는 클래스다. 2026-11-19 0.651에서 R4-4 정기 처리용으로 추가했다.
// 근거: cpppj o/RawSpriteRegular.cpp의 RunSpriteRegular(원문 004794b0)다.
public sealed class RawSpriteRegularTests
{
    /// <summary>실제 풀의 슬롯 수다. 0.651에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 0.651에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int i = 0; i < 4; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>스프라이트 장면을 만든다. 0.651에서 추가했다.</summary>
    private static (RawSidPool Pool, List<RiftTypeFrames> Frames, SquidFrame Setter, int Sid) BuildScene(byte type = 20, int codeCount = 5)
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

        var codes = new List<RiftFrameCode>();
        // 요청한 개수만큼 더미 코드를 넣는 반복문이다.
        for (int i = 0; i < codeCount; i++)
        {
            codes.Add(new RiftFrameCode((byte)'A', (byte)'0', (byte)i, 0));
        }

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
        Put(raw, 0, 0x0050cc08u);
        raw[RawSidPool.TypeOffset] = type;
        raw[RawSidPool.StateOffset] = 0;
        raw[0x21] = 1;
        return (pool, frames, setter, sid);
    }

    /// <summary>현재 프레임에 1을 더한 나머지로 지정하고 payload를 돌려준다. 0.651에서 추가했다.</summary>
    [Fact]
    public void EventZeroAdvancesAndWraps()
    {
        (RawSidPool pool, List<RiftTypeFrames> frames, SquidFrame setter, int sid) = BuildScene();
        Put(pool.AllocatedBytes(sid), 36, 3u);
        Assert.Equal(0.4f, RawSpriteRegular.Run(pool, frames, setter, sid, 0.4f));
        Assert.Equal(4, BinaryPrimitives.ReadInt32LittleEndian(pool.Slot(sid).Slice(36, 4)));
        Put(pool.AllocatedBytes(sid), 36, 4u);
        Assert.Equal(0.4f, RawSpriteRegular.Run(pool, frames, setter, sid, 0.4f));
        Assert.Equal(0, BinaryPrimitives.ReadInt32LittleEndian(pool.Slot(sid).Slice(36, 4)));
    }

    /// <summary>사건·개수와 무관하게 같은 분기로 처리한다. 0.651에서 추가했다.</summary>
    [Fact]
    public void NonZeroEventPassesThrough()
    {
        (RawSidPool pool, List<RiftTypeFrames> frames, SquidFrame setter, int sid) = BuildScene();
        Put(pool.AllocatedBytes(sid), 36, 1u);
        Assert.Equal(0.9f, RawSpriteRegular.Run(pool, frames, setter, sid, 0.9f));
        Assert.Equal(2, BinaryPrimitives.ReadInt32LittleEndian(pool.Slot(sid).Slice(36, 4)));
    }

    /// <summary>두 번째가 아닌 가상표·CD 판본은 거부한다. 0.651에서 추가했다.</summary>
    [Fact]
    public void OtherVtableAndCdThrow()
    {
        (RawSidPool pool, List<RiftTypeFrames> frames, SquidFrame setter, int sid) = BuildScene();
        Put(pool.AllocatedBytes(sid), 0, 0x0050da88u);
        Assert.False(RawSpriteRegular.Handles(pool, sid));
        Assert.Throws<ArgumentException>(() => RawSpriteRegular.Run(pool, frames, setter, sid, 0.1f));
        var cdPool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Cd1072);
        Assert.False(RawSpriteRegular.Handles(cdPool, sid));
    }

    /// <summary>코드 0개는 원본 IDIV 오류로 거부한다. 0.651에서 추가했다.</summary>
    [Fact]
    public void EmptyCodesThrow()
    {
        (RawSidPool pool, List<RiftTypeFrames> frames, SquidFrame setter, int sid) = BuildScene(type: 21, codeCount: 0);
        Put(pool.AllocatedBytes(sid), 36, 0u);
        Assert.Throws<DivideByZeroException>(() => RawSpriteRegular.Run(pool, frames, setter, sid, 0.1f));
    }

    /// <summary>연결되지 않은 실제 장치는 명시 거부한다. 0.651에서 추가했다.</summary>
    [Fact]
    public void InvalidInputsThrow()
    {
        (RawSidPool pool, List<RiftTypeFrames> frames, SquidFrame setter, int sid) = BuildScene();
        Assert.Throws<ArgumentNullException>(() => RawSpriteRegular.Run(null!, frames, setter, sid, 0.1f));
        Assert.Throws<ArgumentNullException>(() => RawSpriteRegular.Run(pool, null!, setter, sid, 0.1f));
        Assert.Throws<ArgumentNullException>(() => RawSpriteRegular.Run(pool, frames, null!, sid, 0.1f));
        Assert.Throws<ArgumentNullException>(() => RawSpriteRegular.Handles(null!, sid));
    }
}
