using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 공유 전역 판정(GetAuthority·GetRecoveryAllowed·GetBoss·GetPriests)을 직접 대조한다.
/// 세션 연결 없이 값형 입력과 공유 값의 전환을 확인한다. 기준 판본은 10.78 이다 (R0-4 재대조, 2026-10-28).
/// </summary>
public sealed class X86RawPriestStateTests
{
    /// <summary>
    /// 장기 Player 기록이 있으면 신전 DWORD의 0 아님을 읽고, 없으면 기존 배열을 쓴다.
    /// </summary>
    [Fact]
    public void RecoveryAllowedReadsPlayerRecordsOrFallsBack()
    {
        var mode = new PriestHitPointMode();
        mode.RecoveryAllowed[1] = true;
        mode.RecoveryAllowed[2] = false;
        Assert.True(mode.GetRecoveryAllowed(1));
        Assert.False(mode.GetRecoveryAllowed(2));
        Assert.Throws<ArgumentOutOfRangeException>(() => mode.GetRecoveryAllowed(10));
        var records = new byte[10 * 0xb4];
        void Put(int owner, uint temple)
        {
            int offset = owner * 0xb4 + 0x7c;
            for (int i = 0; i < 4; i++)
            {
                records[offset + i] = (byte)(temple >> (8 * i));
            }
        }
        Put(1, 0x50a748u);
        Put(2, 0u);
        mode.PlayerRecords = records;
        Assert.True(mode.GetRecoveryAllowed(1));
        Assert.False(mode.GetRecoveryAllowed(2));
        Assert.Throws<ArgumentOutOfRangeException>(() => mode.GetRecoveryAllowed(10));
        mode.PlayerRecords = new byte[10 * 0xb4 - 1];
        Assert.Throws<ArgumentException>(() => mode.GetRecoveryAllowed(1));
    }

    /// <summary>
    /// 공유 권한이 연결되면 값형 입력을 대신 읽는다.
    /// </summary>
    [Fact]
    public void AuthorityReadsSharedFlag()
    {
        var mode = new PriestHitPointMode { Authority = true };
        Assert.True(mode.GetAuthority());
        bool flag = false;
        mode.SharedAuthority = () => flag;
        Assert.False(mode.GetAuthority());
        flag = true;
        Assert.True(mode.GetAuthority());
        var damage = new PriestDamageState { Boss = true };
        Assert.True(damage.GetBoss());
        damage.SharedBoss = () => false;
        Assert.False(damage.GetBoss());
    }

    /// <summary>
    /// 공유 사제 목록이 연결되면 그 목록을 직접 쓴다.
    /// </summary>
    [Fact]
    public void PriestsReturnsSharedList()
    {
        var state = new PriestPostPopState();
        Assert.Same(state.Priests, state.GetPriests());
        var shared = new RawPostPopList { Entries = new uint[16] };
        state.SharedPriests = shared;
        Assert.Same(shared, state.GetPriests());
    }
}
