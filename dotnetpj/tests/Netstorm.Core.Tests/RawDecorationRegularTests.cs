// 장식 정기(HP 단계 감소·삭제·공통 분배)의 분기·부호·반환을 실제 슬롯으로 대조한다.
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// 장식 정기를 검사하는 클래스다. 2026-11-19 0.642에서 R4-4 정기 처리용으로 추가했다.
// 근거: cpppj o/RawDecorationRegular.cpp의 RunDecorationRegular(원문 00462420)다.
public sealed class RawDecorationRegularTests
{
    /// <summary>실제 풀의 슬롯 수다. 0.642에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>슬롯 안 HP 필드의 위치다. 0.642에서 추가했다.</summary>
    private const int HealthOffset = 26;

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 0.642에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int i = 0; i < 4; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>장식 슬롯 장면을 만든다. 0.642에서 추가했다.</summary>
    private static (RawSidPool Pool, int Sid) BuildScene(uint vtable = 0x0050a820u, byte type = 20)
    {
        var pool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Patch1078);
        int sid = pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, vtable);
        raw[RawSidPool.TypeOffset] = type;
        return (pool, sid);
    }

    /// <summary>미연결 갈고리를 거부하는 기본 묶음을 만든다. 0.642에서 추가했다.</summary>
    private static DecorationRegularHooks StrictHooks() => new()
    {
        MaxHitPoints = _ => throw new InvalidOperationException("호출되면 안 된다"),
        HitPoints = (_, _, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
        Destroy = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
        Damageable = (_, _, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
    };

    /// <summary>두 번째 가상표도 장식으로 처리한다. 0.642에서 추가했다.</summary>
    [Fact]
    public void SecondVtableHandles()
    {
        (RawSidPool pool, int sid) = BuildScene(0x0050a8e8u);
        Assert.True(RawDecorationRegular.Handles(pool, sid));
        float result = RawDecorationRegular.Run(pool, true, sid, 7, 0, 0.4f, new DecorationRegularHooks
        {
            MaxHitPoints = _ => throw new InvalidOperationException("호출되면 안 된다"),
            HitPoints = (_, _, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Destroy = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Damageable = (parent, ev, count, payload) =>
            {
                Assert.Equal(sid, parent);
                Assert.Equal(7u, ev);
                return payload;
            },
        });
        Assert.Equal(0.4f, result);
    }

    /// <summary>개수 3 이상에서 서버는 실제 삭제를 호출하고 -1을 돌려준다. 0.642에서 추가했다.</summary>
    [Fact]
    public void EventZeroCountThreeBossDestroys()
    {
        (RawSidPool pool, int sid) = BuildScene();
        int destroyed = 0;
        float result = RawDecorationRegular.Run(pool, true, sid, 0, 3, 0.2f, new DecorationRegularHooks
        {
            MaxHitPoints = _ => throw new InvalidOperationException("호출되면 안 된다"),
            HitPoints = (_, _, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Destroy = (parent, flags) => { Assert.Equal(sid, parent); Assert.Equal(0u, flags); destroyed++; },
            Damageable = (_, _, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
        });
        Assert.Equal(-1.0f, result);
        Assert.Equal(1, destroyed);
    }

    /// <summary>개수 3 이상이어도 비서버는 삭제 없이 공통 분배로 넘긴다. 0.642에서 추가했다.</summary>
    [Fact]
    public void EventZeroCountThreeNonBossFallsThrough()
    {
        (RawSidPool pool, int sid) = BuildScene();
        float result = RawDecorationRegular.Run(pool, false, sid, 0, 4, 0.3f, new DecorationRegularHooks
        {
            MaxHitPoints = _ => throw new InvalidOperationException("호출되면 안 된다"),
            HitPoints = (_, _, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Destroy = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Damageable = (parent, ev, count, payload) =>
            {
                Assert.Equal(sid, parent);
                Assert.Equal(0u, ev);
                Assert.Equal(4u, count);
                return payload;
            },
        });
        Assert.Equal(0.3f, result);
    }

    /// <summary>임계값이 현재 HP보다 낮으면 차이만큼 가상98을 호출한다. 0.642에서 추가했다.</summary>
    [Fact]
    public void EventZeroBelowThresholdHits()
    {
        (RawSidPool pool, int sid) = BuildScene();
        // 최대 900·개수 0이면 곱 2700·임계값 900이다. 현재 1000이면 100을 호출한다.
        Put(pool.AllocatedBytes(sid), HealthOffset, 1000u);
        int hit = 0;
        float result = RawDecorationRegular.Run(pool, true, sid, 0, 0, 0.5f, new DecorationRegularHooks
        {
            MaxHitPoints = parent => { Assert.Equal(sid, parent); return 900; },
            HitPoints = (parent, amount, owner, attacker) =>
            {
                Assert.Equal(sid, parent);
                Assert.Equal(100, amount);
                Assert.Equal(0u, owner);
                Assert.Equal(0u, attacker);
                hit++;
            },
            Destroy = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Damageable = (_, _, _, payload) => payload,
        });
        Assert.Equal(0.5f, result);
        Assert.Equal(1, hit);
    }

    /// <summary>임계값이 현재 HP와 같거나 높으면 가상98을 호출하지 않는다. 0.642에서 추가했다.</summary>
    [Fact]
    public void EventZeroAtThresholdSkipsHit()
    {
        (RawSidPool pool, int sid) = BuildScene();
        // 최대 900·개수 0이면 임계값 900이다. 현재 900이면 호출하지 않는다.
        Put(pool.AllocatedBytes(sid), HealthOffset, 900u);
        float result = RawDecorationRegular.Run(pool, true, sid, 0, 0, 0.6f, new DecorationRegularHooks
        {
            MaxHitPoints = _ => 900,
            HitPoints = (_, _, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Destroy = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Damageable = (_, _, _, payload) => payload,
        });
        Assert.Equal(0.6f, result);
    }

    /// <summary>0이 아닌 사건은 HP·삭제에 닿지 않고 공통 분배로 넘긴다. 0.642에서 추가했다.</summary>
    [Fact]
    public void NonZeroEventPassesThrough()
    {
        (RawSidPool pool, int sid) = BuildScene();
        Put(pool.AllocatedBytes(sid), HealthOffset, 10u);
        float result = RawDecorationRegular.Run(pool, true, sid, 5, 9, 0.9f, new DecorationRegularHooks
        {
            MaxHitPoints = _ => throw new InvalidOperationException("호출되면 안 된다"),
            HitPoints = (_, _, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Destroy = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Damageable = (parent, ev, count, payload) =>
            {
                Assert.Equal(sid, parent);
                Assert.Equal(5u, ev);
                Assert.Equal(9u, count);
                return payload;
            },
        });
        Assert.Equal(0.9f, result);
    }

    /// <summary>CD 판본은 거부한다. 0.642에서 추가했다.</summary>
    [Fact]
    public void CdEditionThrows()
    {
        var pool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Cd1072);
        int sid = pool.Allocate(SidAllocation.Client);
        Assert.False(RawDecorationRegular.Handles(pool, sid));
        Assert.Throws<ArgumentException>(() => RawDecorationRegular.Run(pool, true, sid, 0, 0, 0.1f, StrictHooks()));
    }

    /// <summary>장식이 아닌 가상표는 거부한다. 0.642에서 추가했다.</summary>
    [Fact]
    public void OtherVtableThrows()
    {
        (RawSidPool pool, int sid) = BuildScene(0x0050da88u);
        Assert.False(RawDecorationRegular.Handles(pool, sid));
        Assert.Throws<ArgumentException>(() => RawDecorationRegular.Run(pool, true, sid, 0, 0, 0.1f, StrictHooks()));
    }

    /// <summary>연결되지 않은 실제 장치는 명시 거부한다. 0.642에서 추가했다.</summary>
    [Fact]
    public void MissingHooksThrow()
    {
        (RawSidPool pool, int sid) = BuildScene();
        Put(pool.AllocatedBytes(sid), HealthOffset, 1000u);
        // 임계값 900보다 현재 HP가 높아 가상98에 닿으므로 미연결에서 거부된다.
        Assert.Throws<InvalidOperationException>(() => RawDecorationRegular.Run(pool, true, sid, 0, 0, 0.1f, new DecorationRegularHooks
        {
            MaxHitPoints = _ => 900,
            Damageable = (_, _, _, payload) => payload,
        }));
        Assert.Throws<ArgumentNullException>(() => RawDecorationRegular.Run(null!, true, sid, 0, 0, 0.1f, StrictHooks()));
        Assert.Throws<ArgumentNullException>(() => RawDecorationRegular.Run(pool, true, sid, 0, 0, 0.1f, null!));
    }
}
