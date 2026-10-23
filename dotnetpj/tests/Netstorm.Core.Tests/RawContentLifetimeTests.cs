using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 내용물(너겟) 삭제가 실제 SP 저장소와 AI 지갑에 연결되는지 확인한다.
/// 장면은 cpppj/tests/ContentLifetimeTests.cpp 의 ContentLifetime_ActualSpAndWalletIntegration 과 같다:
/// 부모(타입 90, 소유자 1, 좌표 20·30)의 생산 칸에 너겟(종류 154)을 넣고 지우면,
/// 선지급 지갑 50(절삭) → SP 50.75 → 지갑 50 이 순서대로 더해져 SP 1050.75, 지갑 100 이 된다.
/// 기준 판본은 10.78 이며 CD 10.72 의 지갑 전용 지급은 옮기지 않았다 (W03 내용물 수명 조각, 2026-10-23).
/// </summary>
public sealed class RawContentLifetimeTests
{
    /// <summary>장면 풀의 슬롯 수.</summary>
    private const int PoolCapacity = 32768;

    /// <summary>너겟 내용물 삭제 플래그(원본 관찰과 같다).</summary>
    private const uint TestFlags = 0x10;

    /// <summary>슬롯의 16비트 값을 리틀 엔디언으로 읽는다.</summary>
    /// <param name="pool">읽을 풀.</param>
    /// <param name="sid">슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    private static ushort Word(RawSidPool pool, int sid, int offset)
    {
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        return (ushort)(raw[offset] | (raw[offset + 1] << 8));
    }

    /// <summary>16비트 값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="raw">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void PutWord(Span<byte> raw, int offset, uint value)
    {
        raw[offset] = (byte)value;
        raw[offset + 1] = (byte)(value >> 8);
    }

    /// <summary>32비트 값을 리틀 엔디언으로 쓴다(좌표 float 비트 입력용).</summary>
    /// <param name="raw">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void PutDword(Span<byte> raw, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 너겟 삭제가 공유 SP 저장소·AI 지갑을 갱신하고, 부모 알림 2회(부착·제거)와 위치 소리·SP 표시 갱신을
    /// 정확히 한 번씩 요구하는지 확인한다.
    /// </summary>
    [Fact]
    public void NuggetDestroyCreditsSpAndWallet()
    {
        var pool = new RawSidPool(PoolCapacity);
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 타입 표는 값 객체 레코드로 채운다
        for (int index = 0; index < types.Length; index++)
        {
            types[index] = new OriginalTypeRecord();
        }
        types[154].Cost = 50.75f;
        types[90].FootX = 3;
        types[90].FootY = 5;
        var hash = new RawSquidHash();
        var factory = new RawSquidFactory(pool, types);
        var unpop = new RawUnpop(pool, hash, new byte[65536]);
        var destroy = new RawSquidDestroy(pool, unpop, types);
        var random = new NetstormRandom(7);
        var store = new RawSpStore(random, () => 42u);
        store.Initialize();
        store.Set(1, BitConverter.SingleToUInt32Bits(1000f));
        var bookkeeping = new RawPostPopState { LocalOwner = 1 };
        bookkeeping.AiAttached[1] = true;
        var rewardState = new SquidRewardState();
        uint refreshed = 0;
        var reward = new RawSquidReward(pool, types, bookkeeping, rewardState, store,
            new SquidRewardHooks { RefreshSp = () => refreshed++ });
        // 부모는 클라이언트 영역에 만들고 소유자·좌표를 관찰 입력과 같게 쓴다
        int parent = factory.Create(90, SidAllocation.Client);
        Span<byte> parentRaw = pool.AllocatedBytes(parent);
        parentRaw[34] = 1;
        PutDword(parentRaw, 14, BitConverter.SingleToUInt32Bits(20f));
        PutDword(parentRaw, 18, BitConverter.SingleToUInt32Bits(30f));
        var reasons = new List<uint>();
        var forms = new RawContentFormHost(pool, types, (objectSid, form, reason) =>
        {
            Assert.Equal(parent, objectSid);
            reasons.Add(reason);
        });
        // 내용물은 실제 contained 생성기와 같은 경로(내장 Form 생성)로 만든다
        int value = factory.CreateContained(2);
        PutWord(pool.AllocatedBytes(value), 18, 154);
        PutWord(pool.AllocatedBytes(value), 20, 0x20);
        forms.Attach(value, parent, TestFlags);
        var state = new ContentLifetimeState();
        uint notified = 0;
        uint sounded = 0;
        var hooks = new ContentLifetimeHooks
        {
            LocalNotice = _ => notified++,
            Sound = (x, y) =>
            {
                sounded++;
                Assert.Equal(18.5f, x);
                Assert.Equal(27.5f, y);
            },
        };
        var lifetime = new RawContentLifetime(pool, types, forms, destroy, state, hooks, null, reward);
        lifetime.Destroy(value, TestFlags);
        // 선지급 지갑 50 뒤 SP 50.75 가 더해지고, SP 가산이 지갑에 50 을 다시 더한다
        Assert.Equal(1050.75f, BitConverter.UInt32BitsToSingle(store.Get(1)));
        Assert.Equal(100, rewardState.AiWallet[1]);
        Assert.Equal(1u, notified);
        Assert.Equal(1u, sounded);
        Assert.Equal(1u, refreshed);
        // 부착(1) → 제거(2) 순서로 같은 부모에 알린다
        Assert.Equal(new List<uint> { 1, 2 }, reasons);
        // 공통 Unpop 이 부모의 종속 머리를 비우고, 클라이언트 영역 반납 기록에 타입 6 이 남는다
        Assert.Equal(0, Word(pool, parent, 6));
        Assert.Equal(6u, pool.Deletions(true)[0].Type);
    }
}
