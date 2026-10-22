using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 패치판 삭제·해체 보상(RawSquidReward)을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/RewardTests.cpp 의 Replay 와 같다:
/// 행마다 타입 3개·객체 슬롯·장부·전역·AI 지갑·SP 저장소를 입력값으로 되돌리고 Refund 한 번을 실행한다.
/// 보상 결과로 바뀐 샘 풀·지갑 9개·SP 9개·난수 상태·배열 체크섬·offset·표시/가상 호출 수를 대조한다.
/// 기준 판본은 10.78 이며 originalCD·original1037 행은 읽지 않는다 (W05 선행 조각, 2026-10-22).
/// </summary>
public sealed class X86RawSquidRewardTests
{
    /// <summary>10.78 행의 판본 이름.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 한 판본의 행 수.</summary>
    private const int RowsPerEdition = 2400;

    /// <summary>보상 대상 슬롯 번호.</summary>
    private const int Target = 6;

    /// <summary>fixture 의 열 번호.</summary>
    private enum Column
    {
        /// <summary>판본 이름.</summary>
        Edition,

        /// <summary>대상 타입 번호.</summary>
        Number,

        /// <summary>대상 타입의 cost 비트·HP·flags1·flags2.</summary>
        Type,

        /// <summary>타입 147(rainBlocker) 값.</summary>
        SilentType,

        /// <summary>타입 164(altar) 값.</summary>
        AltarType,

        /// <summary>객체 상태 BYTE.</summary>
        State,

        /// <summary>객체 추가 비트 BYTE(+40).</summary>
        Extra,

        /// <summary>객체 HP DWORD.</summary>
        HitPoints,

        /// <summary>보상 수신자(0~8).</summary>
        Recipient,

        /// <summary>로컬 소유자.</summary>
        LocalOwner,

        /// <summary>해체 환불 여부(1/0).</summary>
        Salvage,

        /// <summary>삭제 보상 비율.</summary>
        Percent,

        /// <summary>샘 풀 시작값.</summary>
        GeyserPool,

        /// <summary>전역 플래그 묶음.</summary>
        Flags,

        /// <summary>AI 부착 마스크(비트 0~8).</summary>
        AiMask,

        /// <summary>AI 지갑 시작값 9개.</summary>
        AiWallets,

        /// <summary>SP 저장소 시작값 9개.</summary>
        SpValues,

        /// <summary>SP 저장소 재시드.</summary>
        Seed,

        /// <summary>SP 저장소 fill.</summary>
        Fill,

        /// <summary>가상 개수 스크립트("base" 또는 목록).</summary>
        Script,

        /// <summary>기대 샘 풀.</summary>
        ExpectedGeyser,

        /// <summary>기대 AI 지갑 9개.</summary>
        ExpectedWallets,

        /// <summary>기대 SP 9개.</summary>
        ExpectedSp,

        /// <summary>Refund 전 난수 상태.</summary>
        StateBefore,

        /// <summary>Refund 전 배열 체크섬.</summary>
        AdlerBefore,

        /// <summary>Refund 뒤 난수 상태.</summary>
        StateAfter,

        /// <summary>Refund 뒤 배열 체크섬.</summary>
        AdlerAfter,

        /// <summary>Refund 뒤 offset.</summary>
        OffsetAfter,

        /// <summary>SP 표시 갱신 호출 수.</summary>
        RefreshCalls,

        /// <summary>가상 개수 호출 수.</summary>
        CountCalls,

        /// <summary>열 수.</summary>
        Count,
    }

    /// <summary>쉼표 목록을 부호 있는 64비트로 읽는다.</summary>
    /// <param name="cell">쉼표로 나눈 목록.</param>
    private static long[] Longs(string cell) => cell.Split(',').Select(value => long.Parse(value)).ToArray();

    /// <summary>부호 있는 64비트 값을 32비트 비트 패턴으로 그대로 줄인다.</summary>
    /// <param name="value">줄일 값.</param>
    private static uint Bits(long value) => unchecked((uint)value);

    /// <summary>부호 있는 64비트 값을 32비트 signed 값으로 그대로 줄인다.</summary>
    /// <param name="value">줄일 값.</param>
    private static int Signed(long value) => unchecked((int)value);

    /// <summary>DWORD 배열을 리틀 엔디언 바이트로 펼친다.</summary>
    /// <param name="pools">32×9 DWORD 배열.</param>
    private static byte[] PoolBytes(ReadOnlySpan<uint> pools)
    {
        var bytes = new byte[pools.Length * 4];
        // 각 DWORD 를 낮은 바이트부터 펼친다
        for (int index = 0; index < pools.Length; index++)
        {
            uint word = pools[index];
            bytes[index * 4] = (byte)word;
            bytes[index * 4 + 1] = (byte)(word >> 8);
            bytes[index * 4 + 2] = (byte)(word >> 16);
            bytes[index * 4 + 3] = (byte)(word >> 24);
        }
        return bytes;
    }

    /// <summary>타입 레코드 네 값을 fixture 목록에서 채운다.</summary>
    /// <param name="types">대상 타입 표.</param>
    /// <param name="index">채울 타입 번호.</param>
    /// <param name="cell">cost 비트·HP·flags1·flags2 목록.</param>
    private static void SetType(OriginalTypeRecord[] types, int index, string cell)
    {
        long[] values = Longs(cell);
        Assert.Equal(4, values.Length);
        OriginalTypeRecord record = types[index];
        record.Cost = BitConverter.UInt32BitsToSingle(Bits(values[0]));
        record.MaxHitPoints = Signed(values[1]);
        record.Flags1 = Bits(values[2]);
        record.Flags2 = Bits(values[3]);
        record.Group = 10;
    }

    /// <summary>
    /// 보상 fixture 의 10.78 몫을 재생한다.
    /// </summary>
    [Fact]
    public void RewardMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("reward");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            Assert.Equal((int)Column.Count, row.Length);
            int number = int.Parse(row[(int)Column.Number]);
            var pool = new RawSidPool(32768, true);
            var types = Enumerable.Range(0, OriginalConstructors.TypeCount)
                .Select(_ => new OriginalTypeRecord()).ToArray();
            // 원본 테스트와 같은 client 번호 5~7 을 확보한다
            for (int expected = 5; expected <= 7; expected++)
            {
                Assert.Equal(expected, pool.Allocate(SidAllocation.Client));
            }
            SetType(types, number, row[(int)Column.Type]);
            SetType(types, 147, row[(int)Column.SilentType]);
            SetType(types, 164, row[(int)Column.AltarType]);
            Span<byte> raw = pool.AllocatedBytes(Target);
            raw.Clear();
            raw[10] = (byte)number;
            raw[11] = (byte)X86Fixture.UInt(row[(int)Column.State]);
            raw[40] = (byte)X86Fixture.UInt(row[(int)Column.Extra]);
            uint hitPoints = Bits(long.Parse(row[(int)Column.HitPoints]));
            raw[26] = (byte)hitPoints;
            raw[27] = (byte)(hitPoints >> 8);
            raw[28] = (byte)(hitPoints >> 16);
            raw[29] = (byte)(hitPoints >> 24);
            var bookkeeping = new RawPostPopState
            {
                LocalOwner = (byte)X86Fixture.UInt(row[(int)Column.LocalOwner]),
            };
            uint aiMask = X86Fixture.UInt(row[(int)Column.AiMask]);
            for (int owner = 0; owner < RawPostPopState.OwnerCount; owner++)
            {
                bookkeeping.AiAttached[owner] = ((aiMask >> owner) & 1) != 0;
            }
            var state = new SquidRewardState
            {
                Percent = X86Fixture.UInt(row[(int)Column.Percent]),
                GeyserPool = Signed(long.Parse(row[(int)Column.GeyserPool])),
            };
            uint flags = X86Fixture.UInt(row[(int)Column.Flags]);
            state.FixedSpA = (flags & 1) != 0;
            state.FixedSpB = (flags & 2) != 0;
            state.HalfHealthMode = (flags & 4) != 0;
            state.AiMirrorDisabled = (flags & 8) != 0;
            state.WalletClamp = (flags & 0x10) != 0;
            state.PriestQuarterHp = (flags & 0x20) != 0;
            state.DebugAsserts = (flags & 0x40) != 0;
            long[] wallets = Longs(row[(int)Column.AiWallets]);
            Assert.Equal(RawPostPopState.OwnerCount, wallets.Length);
            for (int owner = 0; owner < wallets.Length; owner++)
            {
                state.AiWallet[owner] = Signed(wallets[owner]);
            }
            uint[] sp = Longs(row[(int)Column.SpValues]).Select(Bits).ToArray();
            Assert.Equal(RawSpStore.Owners, sp.Length);
            uint seed = X86Fixture.UInt(row[(int)Column.Seed]);
            uint fill = X86Fixture.UInt(row[(int)Column.Fill]);
            var rng = new NetstormRandom(0);
            var store = new RawSpStore(rng, () => seed, fill);
            for (uint owner = 0; owner < RawSpStore.Owners; owner++)
            {
                store.Set(owner, sp[owner]);
            }
            // 원본과 같은 출발 난수 상태·배열 체크섬을 확인한다
            Assert.Equal(X86Fixture.UInt(row[(int)Column.StateBefore]), rng.State);
            Assert.Equal(X86Fixture.UInt(row[(int)Column.AdlerBefore]), X86Fixture.Adler32(PoolBytes(store.Pools)));
            // 가상 개수 스크립트가 있으면 호출마다 다음 값을 준다
            var script = new List<int>();
            if (row[(int)Column.Script] != "base")
            {
                script.AddRange(Longs(row[(int)Column.Script]).Select(Signed));
            }
            uint refreshCalls = 0;
            int cursor = 0;
            var hooks = new SquidRewardHooks
            {
                RefreshSp = () => refreshCalls++,
            };
            if (row[(int)Column.Script] != "base")
            {
                hooks.VirtualCount = _ =>
                {
                    Assert.True(cursor < script.Count);
                    return script[cursor++];
                };
            }
            var reward = new RawSquidReward(pool, types, bookkeeping, state, store, hooks);
            reward.Refund(Target, X86Fixture.UInt(row[(int)Column.Recipient]), row[(int)Column.Salvage] == "1");
            Assert.Equal(X86Fixture.UInt(row[(int)Column.ExpectedGeyser]), unchecked((uint)state.GeyserPool));
            long[] expectedWallets = Longs(row[(int)Column.ExpectedWallets]);
            Assert.Equal(RawPostPopState.OwnerCount, expectedWallets.Length);
            for (int owner = 0; owner < expectedWallets.Length; owner++)
            {
                Assert.Equal(unchecked((uint)Signed(expectedWallets[owner])), unchecked((uint)state.AiWallet[owner]));
            }
            uint[] expectedSp = Longs(row[(int)Column.ExpectedSp]).Select(Bits).ToArray();
            Assert.Equal(RawSpStore.Owners, expectedSp.Length);
            for (uint owner = 0; owner < RawSpStore.Owners; owner++)
            {
                Assert.Equal(expectedSp[owner], store.Get(owner));
            }
            Assert.Equal(X86Fixture.UInt(row[(int)Column.StateAfter]), rng.State);
            Assert.Equal(X86Fixture.UInt(row[(int)Column.AdlerAfter]), X86Fixture.Adler32(PoolBytes(store.Pools)));
            Assert.Equal(X86Fixture.UInt(row[(int)Column.OffsetAfter]), store.Offset);
            Assert.Equal(X86Fixture.UInt(row[(int)Column.RefreshCalls]), refreshCalls);
            Assert.Equal(X86Fixture.UInt(row[(int)Column.CountCalls]), reward.VirtualCountCalls);
            count++;
            return true;
        });
        Assert.Equal(RowsPerEdition, count);
    }

    /// <summary>
    /// 손상 입력은 상태를 바꾸기 전에 거부하고, 패치판 SP 저장소 없이는 만들 수 없다.
    /// </summary>
    [Fact]
    public void RewardRejectsBadInputsBeforeWriting()
    {
        var pool = new RawSidPool(32768, true);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount)
            .Select(_ => new OriginalTypeRecord()).ToArray();
        pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(5);
        raw[10] = 82;
        types[82].Cost = 100f;
        var bookkeeping = new RawPostPopState();
        var state = new SquidRewardState();
        var rng = new NetstormRandom(0);
        var store = new RawSpStore(rng, () => 1u);
        Assert.Throws<ArgumentException>(() => new RawSquidReward(pool, types, bookkeeping, state, null));
        var reward = new RawSquidReward(pool, types, bookkeeping, state, store);
        Assert.Throws<ArgumentOutOfRangeException>(() => reward.Refund(5, 9, false));
        Assert.Throws<ArgumentOutOfRangeException>(() => reward.Refund(0, 1, false));
        Assert.False(store.Initialized);
        Assert.Equal(0, state.GeyserPool);
        // 비유한 SP 시작값은 Set 전에 거부한다
        store.Set(1, 0x7fc00000u);
        var finite = new RawSquidReward(pool, types, bookkeeping, state, store);
        Assert.Throws<InvalidOperationException>(() => finite.AddSp(1, 1f));
    }

    /// <summary>
    /// 제단·삭제 보상의 rainBlocker·성장형 rainBlocker 는 상태를 바꾸지 않거나 비용만 대체한다.
    /// </summary>
    [Fact]
    public void PlanSkipsAltarAndSilentAndAliasesCost()
    {
        var pool = new RawSidPool(32768, true);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount)
            .Select(_ => new OriginalTypeRecord()).ToArray();
        pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(5);
        var bookkeeping = new RawPostPopState();
        var state = new SquidRewardState();
        var store = new RawSpStore(new NetstormRandom(0), () => 1u);
        var reward = new RawSquidReward(pool, types, bookkeeping, state, store);
        raw[10] = 164;
        Assert.False(reward.Plan(5, 1, false).Applied);
        types[147].Cost = 40f;
        raw[10] = 147;
        Assert.False(reward.Plan(5, 1, false).Applied);
        Assert.True(reward.Plan(5, 1, true).Applied);
        // 성장형 rainBlocker 는 비용을 rainBlocker 것으로 쓰고 해체 환불은 0 이다
        types[150].Cost = 100f;
        raw[10] = 150;
        SquidRewardResult plan = reward.Plan(5, 1, false);
        Assert.True(plan.Applied);
        Assert.Equal(40, plan.Cost);
        Assert.Equal(10, plan.Refund);
        Assert.Equal(0, reward.Plan(5, 1, true).Refund);
    }

    /// <summary>
    /// 절반 HP 미만 해체 환불은 0 이고, 지갑은 clamp·미부착·비활성 규칙을 지킨다.
    /// </summary>
    [Fact]
    public void SalvageHalfHealthAndWalletRules()
    {
        var pool = new RawSidPool(32768, true);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount)
            .Select(_ => new OriginalTypeRecord()).ToArray();
        pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(5);
        raw[10] = 82;
        types[82].Cost = 100f;
        types[82].MaxHitPoints = 100;
        types[82].Flags1 = 0x10;
        raw[11] = 0;
        uint hp = 40;
        raw[26] = (byte)hp;
        raw[27] = (byte)(hp >> 8);
        var bookkeeping = new RawPostPopState();
        bookkeeping.AiAttached[1] = true;
        var state = new SquidRewardState { HalfHealthMode = true, WalletClamp = true };
        state.AiWallet[1] = -100;
        var store = new RawSpStore(new NetstormRandom(0), () => 1u);
        var reward = new RawSquidReward(pool, types, bookkeeping, state, store);
        Assert.Equal(0, reward.Plan(5, 1, true).Refund);
        state.HalfHealthMode = false;
        // 100 비용 × 0.25 = 25, HP 40/100 비율이면 10 이다
        Assert.Equal(10, reward.Plan(5, 1, true).Refund);
        // 지갑 clamp 는 0x4a6 아래로 내려가지 않는다
        reward.CreditWallet(1, -100000);
        Assert.Equal(0x4a6, state.AiWallet[1]);
        // 같은 최소값일 때도 clamp 가 적용된다
        state.AiWallet[1] = 0x4a5;
        reward.CreditWallet(1, 0);
        Assert.Equal(0x4a6, state.AiWallet[1]);
        // 부착되지 않은 소유자는 지갑이 바뀌지 않는다
        reward.CreditWallet(2, 500);
        Assert.Equal(0, state.AiWallet[2]);
        state.AiMirrorDisabled = true;
        reward.CreditWallet(1, 500);
        Assert.Equal(0x4a6, state.AiWallet[1]);
    }

    /// <summary>
    /// SP 고정 모드는 지급액 대신 고정값과의 차이를 쓰고, 표시 갱신은 SP 가산마다 한 번이다.
    /// </summary>
    [Fact]
    public void FixedSpModeAndRefreshCount()
    {
        var pool = new RawSidPool(32768, true);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount)
            .Select(_ => new OriginalTypeRecord()).ToArray();
        pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(5);
        raw[10] = 82;
        types[82].Cost = 100f;
        raw[11] = 0;
        var bookkeeping = new RawPostPopState();
        var state = new SquidRewardState { FixedSpA = true };
        var store = new RawSpStore(new NetstormRandom(0), () => 1u);
        uint refresh = 0;
        var reward = new RawSquidReward(pool, types, bookkeeping, state, store,
            new SquidRewardHooks { RefreshSp = () => refresh++ });
        store.Set(1, BitConverter.SingleToUInt32Bits(10f));
        reward.AddSp(1, 5f);
        Assert.Equal(194162f, BitConverter.UInt32BitsToSingle(store.Get(1)));
        Assert.Equal(1u, refresh);
        // 디버그 assert 는 HP 없는 타입의 HP 조회를 거부한다
        state.DebugAsserts = true;
        Assert.Throws<InvalidOperationException>(() => reward.TypeHitPoints(82));
    }
}
