// Player 정기(소어 루저 검사·Template 표시)의 분기·순서·반환을 실제 슬롯으로 대조한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// Player 정기를 검사하는 클래스다. 2026-11-19 0.643에서 R4-4 정기 처리용으로 추가했다.
// 근거: cpppj o/RawPlayerRegular.cpp의 RunPlayerRegular(원문 PATCH 00490810·CD 00407ef0)다.
public sealed class RawPlayerRegularTests
{
    /// <summary>실제 풀의 슬롯 수다. 0.643에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 0.643에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int i = 0; i < 4; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>Player 슬롯 장면을 만든다. 0.643에서 추가했다.</summary>
    private static (RawSidPool Pool, int Sid) BuildScene(byte owner = 1)
    {
        var pool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Patch1078);
        int sid = pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x0050ee08u);
        raw[34] = owner;
        return (pool, sid);
    }

    /// <summary>미연결 갈고리를 거부하는 기본 묶음을 만든다. 0.643에서 추가했다.</summary>
    private static PlayerRegularHooks StrictHooks() => new()
    {
        Assertion = _ => throw new InvalidOperationException("호출되면 안 된다"),
        Log = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
        ReserveRegular = () => throw new InvalidOperationException("호출되면 안 된다"),
        AddRegular = (_, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
        TemplateState = _ => throw new InvalidOperationException("호출되면 안 된다"),
        TemplateChanged = _ => throw new InvalidOperationException("호출되면 안 된다"),
    };

    /// <summary>로컬 소유자가 아니면 개수와 무관하게 원래 payload를 돌려준다. 0.643에서 추가했다.</summary>
    [Fact]
    public void NonLocalOwnerPassesThrough()
    {
        (RawSidPool pool, int sid) = BuildScene(2);
        float result = RawPlayerRegular.Run(pool, 1, 1, true, sid, 1, 5, 5.0f, StrictHooks());
        Assert.Equal(5.0f, result);
    }

    /// <summary>개수 0이면 효과 없이 원래 payload를 돌려준다. 0.643에서 추가했다.</summary>
    [Fact]
    public void ZeroCountPassesThrough()
    {
        (RawSidPool pool, int sid) = BuildScene();
        float result = RawPlayerRegular.Run(pool, 1, 1, true, sid, 1, 0, 5.0f, StrictHooks());
        Assert.Equal(5.0f, result);
    }

    /// <summary>사건 1·전투 중이면 로그 두 줄 뒤 새 정기 2를 예약하고 0을 돌려준다. 0.643에서 추가했다.</summary>
    [Fact]
    public void EventOneBattleReservesEventTwo()
    {
        (RawSidPool pool, int sid) = BuildScene();
        var logs = new List<string>();
        int reserved = 0, added = 0;
        float result = RawPlayerRegular.Run(pool, 1, 1, true, sid, 1, 1, 5.0f, new PlayerRegularHooks
        {
            Assertion = _ => throw new InvalidOperationException("호출되면 안 된다"),
            Log = (group, text) => logs.Add(group + ":" + text),
            ReserveRegular = () => { reserved++; return true; },
            AddRegular = (parent, ev, payload) =>
            {
                Assert.Equal(sid, parent);
                Assert.Equal(2u, ev);
                Assert.Equal(60.0f, payload);
                added++;
            },
            TemplateState = _ => throw new InvalidOperationException("호출되면 안 된다"),
            TemplateChanged = _ => throw new InvalidOperationException("호출되면 안 된다"),
        });
        Assert.Equal(0.0f, result);
        Assert.Equal(["29:SORE LOSER TEST: ", "29:STARTING.\n"], logs);
        Assert.Equal(1, reserved);
        Assert.Equal(1, added);
    }

    /// <summary>사건 1·비전투이면 예약 없이 미시작 로그 뒤 원래 payload를 돌려준다. 0.643에서 추가했다.</summary>
    [Fact]
    public void EventOneNonBattleLogsOnly()
    {
        (RawSidPool pool, int sid) = BuildScene();
        var logs = new List<string>();
        float result = RawPlayerRegular.Run(pool, 1, 1, false, sid, 1, 1, 5.0f, new PlayerRegularHooks
        {
            Assertion = _ => throw new InvalidOperationException("호출되면 안 된다"),
            Log = (group, text) => logs.Add(group + ":" + text),
            ReserveRegular = () => throw new InvalidOperationException("호출되면 안 된다"),
            AddRegular = (_, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
            TemplateState = _ => throw new InvalidOperationException("호출되면 안 된다"),
            TemplateChanged = _ => throw new InvalidOperationException("호출되면 안 된다"),
        });
        Assert.Equal(5.0f, result);
        Assert.Equal(["29:SORE LOSER TEST: ", "29:NOT STARTING YET.\n"], logs);
    }

    /// <summary>사건 2·전투 중이면 Template 칸에 표시 비트를 세우고 0을 돌려준다. 0.643에서 추가했다.</summary>
    [Fact]
    public void EventTwoBattleMarksTemplate()
    {
        (RawSidPool pool, int sid) = BuildScene();
        byte[] store = new byte[180];
        var changed = new List<uint>();
        float result = RawPlayerRegular.Run(pool, 1, 3, true, sid, 2, 1, 60.0f, new PlayerRegularHooks
        {
            Assertion = _ => throw new InvalidOperationException("호출되면 안 된다"),
            Log = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
            ReserveRegular = () => throw new InvalidOperationException("호출되면 안 된다"),
            AddRegular = (_, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
            TemplateState = slot => { Assert.Equal(3u, slot); return store.AsMemory(60, 20); },
            TemplateChanged = slot => changed.Add(slot),
        });
        Assert.Equal(0.0f, result);
        Assert.Equal(4, store[60] & 4);
        Assert.Equal([3u], changed);
    }

    /// <summary>사건 2·비전투이면 Template에 닿지 않고 0을 돌려준다. 0.643에서 추가했다.</summary>
    [Fact]
    public void EventTwoNonBattleSkipsTemplate()
    {
        (RawSidPool pool, int sid) = BuildScene();
        float result = RawPlayerRegular.Run(pool, 1, 1, false, sid, 2, 1, 60.0f, new PlayerRegularHooks
        {
            Assertion = _ => throw new InvalidOperationException("호출되면 안 된다"),
            Log = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
            ReserveRegular = () => throw new InvalidOperationException("호출되면 안 된다"),
            AddRegular = (_, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
            TemplateState = _ => throw new InvalidOperationException("호출되면 안 된다"),
            TemplateChanged = _ => throw new InvalidOperationException("호출되면 안 된다"),
        });
        Assert.Equal(0.0f, result);
    }

    /// <summary>알 수 없는 사건은 진단 뒤 원래 payload를 돌려준다. 0.643에서 추가했다.</summary>
    [Fact]
    public void UnknownEventAssertsAndPassesThrough()
    {
        (RawSidPool pool, int sid) = BuildScene();
        var asserted = new List<PlayerRegularAssertion>();
        float result = RawPlayerRegular.Run(pool, 1, 1, true, sid, 9, 1, 1.5f, new PlayerRegularHooks
        {
            Assertion = kind => asserted.Add(kind),
            Log = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
            ReserveRegular = () => throw new InvalidOperationException("호출되면 안 된다"),
            AddRegular = (_, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
            TemplateState = _ => throw new InvalidOperationException("호출되면 안 된다"),
            TemplateChanged = _ => throw new InvalidOperationException("호출되면 안 된다"),
        });
        Assert.Equal(1.5f, result);
        Assert.Equal([PlayerRegularAssertion.UnknownEvent], asserted);
    }

    /// <summary>Player가 아닌 가상표·빈 Template·미연결 장치는 거부한다. 0.643에서 추가했다.</summary>
    [Fact]
    public void InvalidInputsThrow()
    {
        (RawSidPool pool, int sid) = BuildScene();
        Put(pool.AllocatedBytes(sid), 0, 0x0050da88u);
        Assert.False(RawPlayerRegular.Handles(pool, sid));
        Assert.Throws<ArgumentException>(() => RawPlayerRegular.Run(pool, 1, 1, true, sid, 1, 1, 5.0f, StrictHooks()));
        (RawSidPool pool2, int sid2) = BuildScene();
        Assert.True(RawPlayerRegular.Handles(pool2, sid2));
        Assert.Throws<InvalidOperationException>(() => RawPlayerRegular.Run(pool2, 1, 1, true, sid2, 1, 1, 5.0f, new PlayerRegularHooks()));
        Assert.Throws<ArgumentNullException>(() => RawPlayerRegular.Run(null!, 1, 1, true, sid2, 1, 1, 5.0f, StrictHooks()));
        Assert.Throws<ArgumentNullException>(() => RawPlayerRegular.Run(pool2, 1, 1, true, sid2, 1, 1, 5.0f, null!));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawPlayerRegular.Run(pool2, 1, 1, true, sid2, 2, 1, 60.0f, new PlayerRegularHooks
        {
            Assertion = _ => { },
            Log = (_, _) => { },
            ReserveRegular = () => true,
            AddRegular = (_, _, _) => { },
            TemplateState = _ => Memory<byte>.Empty,
            TemplateChanged = _ => { },
        }));
    }
}
