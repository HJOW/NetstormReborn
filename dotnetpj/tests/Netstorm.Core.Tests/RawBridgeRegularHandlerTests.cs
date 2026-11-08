// 다리 Regular 합성 처리기의 계약을 확인한다.
using Netstorm.Core.Bridges;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 연결 처리기 모듈을 검증하는 테스트 클래스다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.
public sealed class RawBridgeRegularHandlerTests
{
    /// <summary>다리 슬롯을 만든다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    private static int BridgeSlot(RawSidPool pool, uint table)
    {
        int sid = pool.Allocate();
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = 82;
        for (int i = 0; i < 4; i++)
        {
            raw[i] = (byte)(table >> (8 * i));
        }

        return sid;
    }

    /// <summary>사건 처리기를 만든다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    private static RawBridgeEvents Events(RawSidPool pool, List<(int Bridge, uint Flags)> calls) => new(
        pool,
        new BridgeEventState { Authority = true },
        new BridgeEventHooks
        {
            Frames = _ => [],
            FirstNeighbor = _ => 0,
            NotifySurface = _ => { },
            Create = _ => 0,
            Destroy = (sid, flags) => calls.Add((sid, flags)),
            SetOwner = (_, _) => { },
            Pop = (_, _, _, _) => { },
        });

    /// <summary>다리 슬롯은 사건 처리기로 연결되는지 검증한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    [Fact]
    public void BridgeSlotUsesHandler()
    {
        var pool = new RawSidPool(32768);
        var calls = new List<(int Bridge, uint Flags)>();
        var handler = RawBridgeEvents.MakeBridgeRegularHandler(pool, Events(pool, calls));
        int sid = BridgeSlot(pool, 0x005034C8u);
        Assert.Equal(-1f, handler(sid, RawBridgeEvents.DestroyEvent, 3u, 0.5f));
        Assert.Equal([(sid, 0u)], calls);
    }

    /// <summary>다리가 아니면 대체 처리기로 연결되는지 검증한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    [Fact]
    public void NonBridgeSlotUsesFallback()
    {
        var pool = new RawSidPool(32768);
        var calls = new List<(int Bridge, uint Flags)>();
        var handler = RawBridgeEvents.MakeBridgeRegularHandler(pool, Events(pool, calls), (_, _, _, payload) => payload + 1f);
        int sid = BridgeSlot(pool, 0x00501DD0u);
        Assert.Equal(1.5f, handler(sid, RawBridgeEvents.DestroyEvent, 3u, 0.5f));
        Assert.Empty(calls);
    }

    /// <summary>대체 처리기가 없으면 원본 값을 돌려주는지 검증한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    [Fact]
    public void NonBridgeSlotWithoutFallbackKeepsPayload()
    {
        var pool = new RawSidPool(32768);
        var calls = new List<(int Bridge, uint Flags)>();
        var handler = RawBridgeEvents.MakeBridgeRegularHandler(pool, Events(pool, calls));
        int sid = BridgeSlot(pool, 0x00501DD0u);
        Assert.Equal(0.5f, handler(sid, 999u, 3u, 0.5f));
        Assert.Empty(calls);
    }
}
