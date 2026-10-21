using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 다리 사전·사후 파괴(<see cref="RawBridgeLifecycle"/>)의 경계·판정을 합성 입력으로 확인한다.
/// 원본 재생(X86RawDestroyTests 의 Bridge 64행)이 다루지 않는 지연 페이로드·연결 판정·훅 거부를
/// cpppj/tests/BridgeLifecycleTests.cpp 의 경계 의미를 따라 10.78 경로로 검사한다 (W03 삭제·수명 둘째 조각, 2026-10-21).
/// </summary>
public sealed class RawBridgeLifecycleTests
{
    /// <summary>다리 기본 타입 번호.</summary>
    private const int BridgeType = 74;

    /// <summary>연결 타입 번호.</summary>
    private const int LinkType = 75;

    /// <summary>풀·해시·점유 지도·타입 표·다리 삭제를 묶어 돌려준다.</summary>
    private static (RawSidPool Pool, RawSquidHash Hash, byte[] Spots, OriginalTypeRecord[] Types, RawBridgeLifecycle Bridge) Setup()
    {
        var pool = new RawSidPool(32768, true);
        var hash = new RawSquidHash();
        var spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 타입 번호 전체에 빈 타입 레코드를 채운다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }
        return (pool, hash, spots, types, new RawBridgeLifecycle(pool, types));
    }

    /// <summary>슬롯을 기본값(가상 표·타입·상태·좌표)으로 채운다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="sid">채울 번호.</param>
    /// <param name="type">타입 번호.</param>
    /// <param name="state">상태 바이트.</param>
    /// <param name="extra">추가 비트 바이트.</param>
    private static void Fill(RawSidPool pool, int sid, int type, int state, int extra = 0)
    {
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Clear();
        Put(raw, 0, OriginalConstructors.BaseVtable);
        raw[10] = (byte)type;
        raw[11] = (byte)state;
        raw[40] = (byte)extra;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f));
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>
    /// 지연 추락 페이로드가 원본 ftol 묶음과 같다. 기대 비트는 파이썬 struct 로 독립 계산했다.
    /// </summary>
    [Fact]
    public void DelayedFallPayloadPacksTruncatedCoordinates()
    {
        Assert.Equal(0x1514U, BitConverter.SingleToUInt32Bits(RawBridgeLifecycle.DelayedFallPayload(20.75f, 21.9f)));
        Assert.Equal(0U, BitConverter.SingleToUInt32Bits(RawBridgeLifecycle.DelayedFallPayload(0f, 0f)));
        Assert.Equal(0x12CFFU, BitConverter.SingleToUInt32Bits(RawBridgeLifecycle.DelayedFallPayload(-1.5f, 300.25f)));
        Assert.Equal(0x500U, BitConverter.SingleToUInt32Bits(RawBridgeLifecycle.DelayedFallPayload(float.NaN, 5f)));
        Assert.Equal(0x100U, BitConverter.SingleToUInt32Bits(RawBridgeLifecycle.DelayedFallPayload(1e20f, 1f)));
    }

    /// <summary>
    /// 연결 끊김 판정은 첫·둘째 참조와 죽음 비트로 정한다. 참조가 비었거나 수가 모자라면 거짓이다.
    /// </summary>
    [Fact]
    public void LinkNeedsDestroyChecksReferencesAndDeadBits()
    {
        var (pool, _, _, _, bridge) = Setup();
        int link = pool.Allocate(SidAllocation.Client);
        int first = pool.Allocate(SidAllocation.Client);
        int second = pool.Allocate(SidAllocation.Client);
        Fill(pool, link, LinkType, 0);
        Fill(pool, first, BridgeType, 0);
        Fill(pool, second, BridgeType, 0);
        Put(pool.AllocatedBytes(link), 12, (uint)first, 2);
        Put(pool.AllocatedBytes(link), 8, (uint)second, 2);
        Assert.False(bridge.LinkNeedsDestroy(link));
        pool.AllocatedBytes(first)[11] |= 2;
        Assert.True(bridge.LinkNeedsDestroy(link));
        pool.AllocatedBytes(second)[11] |= 2;
        Assert.True(bridge.LinkNeedsDestroy(link));
        Put(pool.AllocatedBytes(link), 12, 0, 2);
        Assert.False(bridge.LinkNeedsDestroy(link));
        Put(pool.AllocatedBytes(link), 12, (uint)first, 2);
        Put(pool.AllocatedBytes(link), 8, (uint)pool.Capacity, 2);
        Assert.False(bridge.LinkNeedsDestroy(link));
        Put(pool.AllocatedBytes(link), 8, (uint)second, 2);
        pool.AllocatedBytes(first)[40] = 8;
        Assert.False(bridge.LinkNeedsDestroy(link));
    }

    /// <summary>
    /// 훅이 비었으면 풀을 바꾸기 전에 거부한다. 일반 다리는 탐색기도 있어야 하고, 매립 다리는 Emit 만으로 돈다.
    /// </summary>
    [Fact]
    public void MissingHooksRejectedBeforeMutation()
    {
        var (pool, _, _, _, bridge) = Setup();
        int root = pool.Allocate(SidAllocation.Client);
        Fill(pool, root, BridgeType, 0);
        string before = Convert.ToHexStringLower(pool.Bytes.ToArray());
        Assert.ThrowsAny<Exception>(() => bridge.PreDestroy(root, 0, LinkType, new BridgeLifecycleHooks()));
        Assert.ThrowsAny<Exception>(() => bridge.PostDestroy(root, 0, new BridgeLifecycleHooks()));
        Assert.ThrowsAny<Exception>(() => bridge.PreDestroy(root, 0, LinkType, new BridgeLifecycleHooks { Emit = _ => { } }));
        Assert.ThrowsAny<Exception>(() => bridge.PostDestroy(root, 0, new BridgeLifecycleHooks { Emit = _ => { } }));
        Assert.Equal(before, Convert.ToHexStringLower(pool.Bytes.ToArray()));
        pool.AllocatedBytes(root)[40] = 8;
        var events = new List<BridgeLifecycleEvent>();
        bridge.PreDestroy(root, 0, LinkType, new BridgeLifecycleHooks { Emit = events.Add });
        bridge.PostDestroy(root, 0, new BridgeLifecycleHooks { Emit = events.Add });
        Assert.Equal(2, events.Count);
        Assert.Equal(BridgeLifecycleEffect.BasePreDestroy, events[0].Effect);
        Assert.Equal(BridgeLifecycleEffect.BasePostDestroy, events[1].Effect);
    }

    /// <summary>범위 밖 번호·넘치는 타입은 예외다. 풀은 그대로 둔다.</summary>
    [Fact]
    public void OutOfRangeSidAndTypeRejected()
    {
        var (pool, _, _, _, bridge) = Setup();
        Assert.ThrowsAny<Exception>(() => bridge.LinkNeedsDestroy(0));
        Assert.ThrowsAny<Exception>(() => bridge.LinkNeedsDestroy(pool.Capacity));
        Assert.ThrowsAny<Exception>(() => bridge.PreDestroy(0, 0, LinkType, new BridgeLifecycleHooks { Emit = _ => { }, Begin = _ => 0, Next = () => 0 }));
    }
}
