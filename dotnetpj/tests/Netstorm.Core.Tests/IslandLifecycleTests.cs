// 섬 받침 사전 파괴와 표면 사후 파괴의 경계·분배를 직접 검사한다.
using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawIslandLifecycle의 훅 검증·추상 건너뜀·걷는 유닛 분배를 검증하는 테스트 클래스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class IslandLifecycleTests
{
    /// <summary>검사 풀 용량이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Capacity = 24000;

    /// <summary>받침 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Island = 50;

    /// <summary>표면 칸 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Surface = 50;

    /// <summary>걷는 유닛 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Walker = 60;

    /// <summary>걷지 않는 이웃 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Other = 61;

    /// <summary>걷는 유닛 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int WalkerType = 70;

    /// <summary>일반 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int PlainType = 71;

    /// <summary>값을 리틀 엔디언으로 쓴다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓰는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < width; ++i)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>풀·해시·타입·프레임의 빈 장면을 만든다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <returns>풀·해시·점유·타입·프레임 묶음이다.</returns>
    private static (RawSidPool Pool, RawSquidHash Hash, byte[] Spots, OriginalTypeRecord[] Types, List<IReadOnlyList<FrameCode>> Frames) Scene()
    {
        RawSidPool pool = new(Capacity, false);
        RawSquidHash hash = new();
        byte[] spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
        OriginalTypeRecord[] types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 모든 타입 번호에 빈 타입 레코드를 채우는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        List<IReadOnlyList<FrameCode>> frames = [];
        // 모든 타입 번호마다 빈 프레임 목록을 넣는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < types.Length; ++i)
        {
            frames.Add([]);
        }

        return (pool, hash, spots, types, frames);
    }

    /// <summary>훅 누락과 다른 풀의 탐색기는 생성 때 거부한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void RejectsMissingHooksAndForeignPool()
    {
        var (pool, hash, spots, types, frames) = Scene();
        RawSquidNeighbors neighbors = new(pool, hash, spots, types, frames);
        IslandLifecycleHooks hooks = new()
        {
            ScheduleFall = (sid, x, y) => { },
            FallWalker = sid => { },
            BasePreDestroy = (sid, flags) => { },
            BasePostDestroy = (sid, flags) => { },
        };
        IslandLifecycleHooks broken = new()
        {
            ScheduleFall = null,
            FallWalker = hooks.FallWalker,
            BasePreDestroy = hooks.BasePreDestroy,
            BasePostDestroy = hooks.BasePostDestroy,
        };
        Assert.Throws<ArgumentException>(() => new RawIslandLifecycle(pool, neighbors, broken));
        RawSidPool other = new(Capacity, false);
        Assert.Throws<ArgumentException>(() => new RawIslandLifecycle(other, neighbors, hooks));
    }

    /// <summary>추상·매립 받침은 이웃을 읽지 않고 공통 사전 파괴만 부른다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void AbstractIslandSkipsWalkAndCallsBaseOnly()
    {
        var (pool, hash, spots, types, frames) = Scene();
        pool.RawSlot(Island)[40] = 9;
        RawSquidNeighbors neighbors = new(pool, hash, spots, types, frames);
        List<string> events = [];
        RawIslandLifecycle lifecycle = new(pool, neighbors, new IslandLifecycleHooks
        {
            ScheduleFall = (sid, x, y) => events.Add("fall"),
            FallWalker = sid => events.Add("walker"),
            BasePreDestroy = (sid, flags) => events.Add("pre:" + sid + ":" + flags),
            BasePostDestroy = (sid, flags) => events.Add("post"),
        });
        lifecycle.IslandPreDestroy(Island, 0u);
        Assert.Equal(["pre:50:0"], events);
    }

    /// <summary>표면 칸의 걷는 유닛에만 가상 낙하를 부른다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void SurfacePostDestroyFallsWalkersOnly()
    {
        var (pool, hash, spots, types, frames) = Scene();
        types[WalkerType].Flags2 = TypeFlag2.Walker;
        types[WalkerType].FootX = 1;
        types[WalkerType].FootY = 1;
        types[PlainType].Flags2 = 0u;
        types[PlainType].FootX = 1;
        types[PlainType].FootY = 1;
        Span<byte> surface = pool.RawSlot(Surface);
        surface.Clear();
        surface[RawSidPool.TypeOffset] = (byte)PlainType;
        Put(surface, 14, BitConverter.SingleToUInt32Bits(20.75f), 4);
        Put(surface, 18, BitConverter.SingleToUInt32Bits(21.9f), 4);
        Span<byte> walker = pool.RawSlot(Walker);
        walker.Clear();
        walker[RawSidPool.TypeOffset] = (byte)WalkerType;
        Put(walker, 14, BitConverter.SingleToUInt32Bits(20.5f), 4);
        Put(walker, 18, BitConverter.SingleToUInt32Bits(21.5f), 4);
        Span<byte> other = pool.RawSlot(Other);
        other.Clear();
        other[RawSidPool.TypeOffset] = (byte)PlainType;
        Put(other, 14, BitConverter.SingleToUInt32Bits(20.6f), 4);
        Put(other, 18, BitConverter.SingleToUInt32Bits(21.4f), 4);
        Put(walker, 4, (uint)Other, 2);
        Put(other, 4, 0u, 2);
        hash.Cell(1, 10, 10) = (ushort)Walker;
        RawSquidNeighbors neighbors = new(pool, hash, spots, types, frames);
        List<string> events = [];
        RawIslandLifecycle lifecycle = new(pool, neighbors, new IslandLifecycleHooks
        {
            ScheduleFall = (sid, x, y) => events.Add("fall"),
            FallWalker = sid => events.Add("walker:" + sid),
            BasePreDestroy = (sid, flags) => events.Add("pre"),
            BasePostDestroy = (sid, flags) => events.Add("post:" + sid + ":" + flags),
        });
        lifecycle.SurfacePostDestroy(Surface, 0u);
        Assert.Equal(["walker:60", "post:50:0"], events);
    }

    /// <summary>추상·매립 표면은 칸 탐색을 건너뛰고 공통 사후 파괴만 부른다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void AbstractSurfaceSkipsScanAndCallsBaseOnly()
    {
        var (pool, hash, spots, types, frames) = Scene();
        Span<byte> surface = pool.RawSlot(Surface);
        surface.Clear();
        surface[40] = 9;
        RawSquidNeighbors neighbors = new(pool, hash, spots, types, frames);
        List<string> events = [];
        RawIslandLifecycle lifecycle = new(pool, neighbors, new IslandLifecycleHooks
        {
            ScheduleFall = (sid, x, y) => events.Add("fall"),
            FallWalker = sid => events.Add("walker"),
            BasePreDestroy = (sid, flags) => events.Add("pre"),
            BasePostDestroy = (sid, flags) => events.Add("post:" + sid + ":" + flags),
        });
        lifecycle.SurfacePostDestroy(Surface, 7u);
        Assert.Equal(["post:50:7"], events);
    }
}
