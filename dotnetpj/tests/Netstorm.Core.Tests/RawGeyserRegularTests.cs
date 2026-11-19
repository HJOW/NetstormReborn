// 간헐천 정기의 비율 구간·프레임 진행·표시·피해 위임을 실제 슬롯으로 대조한다.
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// 간헐천 정기를 검사하는 클래스다. 2026-11-19 0.634에서 R4-4 정기 처리용으로 추가했다.
// 근거: cpppj o/RawAssetRegular.cpp의 RunNativeGeyserRegular(원문 485640)다. 난수를 소비하지 않는다.
public sealed class RawGeyserRegularTests
{
    /// <summary>실제 풀의 슬롯 수다. 2026-11-19 0.634에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 2026-11-19 0.634에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다.
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>간헐천 슬롯 장면을 만든다. 비용·수량·프레임은 호출자가 둔다. 2026-11-19 0.634에서 추가했다.</summary>
    private static (RawSidPool Pool, OriginalTypeRecord[] Types, int Sid) BuildScene(float cost, uint quantity, int frame)
    {
        var pool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Patch1078);
        var types = new OriginalTypeRecord[188];
        // 모든 타입 기록을 빈 값으로 채우는 반복문이다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        types[122].Cost = cost;
        int sid = pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(sid);
        Put(raw, 0, 0x0050da88u);
        raw[RawSidPool.TypeOffset] = 122;
        Put(raw, 12, quantity, 2);
        Put(raw, 36, unchecked((uint)frame));
        return (pool, types, sid);
    }

    /// <summary>결정량이 가득하면 프레임을 올리고 0.03초를 돌려준다. 2026-11-19 0.634에서 추가했다.</summary>
    [Fact]
    public void FullGeyserAdvancesFrame()
    {
        (RawSidPool pool, OriginalTypeRecord[] types, int sid) = BuildScene(100f, 80u, 5);
        int displays = 0;
        float result = RawGeyserRegular.Run(pool, types, sid, 0, 1, 0.1f, new GeyserRegularHooks
        {
            DisplayUpdate = (_, _) => displays++,
            Damageable = (_, _, _, value) => value,
        });
        Assert.Equal(6, BitConverter.ToInt32(pool.Slot(sid).Slice(36, 4)));
        Assert.Equal(BitConverter.UInt32BitsToSingle(0x3cf5c28fu), result);
        Assert.Equal(2, displays);
    }

    /// <summary>절반 구간에서는 17부터 다시 시작하고 0.06초를 돌려준다. 2026-11-19 0.634에서 추가했다.</summary>
    [Fact]
    public void HalfGeyserWrapsAtThirtyTwo()
    {
        (RawSidPool pool, OriginalTypeRecord[] types, int sid) = BuildScene(100f, 60u, 32);
        int displays = 0;
        float result = RawGeyserRegular.Run(pool, types, sid, 0, 0, 0.1f, new GeyserRegularHooks
        {
            DisplayUpdate = (_, _) => displays++,
            Damageable = (_, _, _, value) => value,
        });
        Assert.Equal(17, BitConverter.ToInt32(pool.Slot(sid).Slice(36, 4)));
        Assert.Equal(BitConverter.UInt32BitsToSingle(0x3d75c28fu), result);
        Assert.Equal(2, displays);
    }

    /// <summary>고갈 구간에서는 33부터 다시 시작하고 0.09초를 돌려준다. 2026-11-19 0.634에서 추가했다.</summary>
    [Fact]
    public void EmptyGeyserWrapsAtFortyEight()
    {
        (RawSidPool pool, OriginalTypeRecord[] types, int sid) = BuildScene(100f, 10u, 48);
        float result = RawGeyserRegular.Run(pool, types, sid, 0, 0, 0.1f, new GeyserRegularHooks
        {
            DisplayUpdate = (_, _) => { },
            Damageable = (_, _, _, value) => value,
        });
        Assert.Equal(33, BitConverter.ToInt32(pool.Slot(sid).Slice(36, 4)));
        Assert.Equal(BitConverter.UInt32BitsToSingle(0x3db851ecu), result);
    }

    /// <summary>0이 아닌 사건은 프레임 손질 없이 피해 분배로 넘긴다. 2026-11-19 0.634에서 추가했다.</summary>
    [Fact]
    public void OtherEventFallsThroughToDamageable()
    {
        (RawSidPool pool, OriginalTypeRecord[] types, int sid) = BuildScene(100f, 80u, 5);
        int displays = 0;
        float result = RawGeyserRegular.Run(pool, types, sid, 3, 2, 0.7f, new GeyserRegularHooks
        {
            DisplayUpdate = (_, _) => displays++,
            Damageable = (_, _, _, value) => value + 1.0f,
        });
        Assert.Equal(5, BitConverter.ToInt32(pool.Slot(sid).Slice(36, 4)));
        Assert.Equal(1.7f, result);
        Assert.Equal(1, displays);
    }

    /// <summary>간헐천 가상표가 아니면 거부한다. 2026-11-19 0.634에서 추가했다.</summary>
    [Fact]
    public void RejectsNonGeyserVtable()
    {
        (RawSidPool pool, OriginalTypeRecord[] types, int sid) = BuildScene(100f, 80u, 5);
        Put(pool.AllocatedBytes(sid), 0, 0x00509148u);
        Assert.Throws<ArgumentException>(() => RawGeyserRegular.Run(pool, types, sid, 0, 0, 0.1f, new GeyserRegularHooks
        {
            DisplayUpdate = (_, _) => { },
            Damageable = (_, _, _, value) => value,
        }));
        Assert.False(RawGeyserRegular.Handles(pool, sid));
    }

    /// <summary>없는 풀·타입·장치는 실제 호출 전에 거부한다. 2026-11-19 0.634에서 추가했다.</summary>
    [Fact]
    public void RejectsMissingDevices()
    {
        (RawSidPool pool, OriginalTypeRecord[] types, int sid) = BuildScene(100f, 80u, 5);
        var hooks = new GeyserRegularHooks
        {
            DisplayUpdate = (_, _) => { },
            Damageable = (_, _, _, value) => value,
        };
        Assert.Throws<ArgumentNullException>(() => RawGeyserRegular.Run(null!, types, sid, 0, 0, 0.1f, hooks));
        Assert.Throws<ArgumentNullException>(() => RawGeyserRegular.Run(pool, null!, sid, 0, 0, 0.1f, hooks));
        Assert.Throws<ArgumentNullException>(() => RawGeyserRegular.Run(pool, types, sid, 0, 0, 0.1f, null!));
        Assert.Throws<InvalidOperationException>(() => RawGeyserRegular.Run(pool, types, sid, 0, 0, 0.1f, new GeyserRegularHooks()));
    }
}
