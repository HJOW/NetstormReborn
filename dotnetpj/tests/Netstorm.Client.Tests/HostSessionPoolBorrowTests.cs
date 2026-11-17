// 개발 호스트의 외부 풀 빌림(별도 풀 전환 첫 단계)을 검사한다.
// RawCampaignSession의 저장소 빌림과 같은 빌림 관례를 HostSession에 둔다.
// 0.594에서 추가됐다. (R4-1 잔여 중 HostSession 별도 풀 전환)
namespace Netstorm.Client.Tests;

/// <summary>외부 풀 빌림의 정체성·덤프 중립·계약 거부를 확인한다.</summary>
public sealed class HostSessionPoolBorrowTests
{
    /// <summary>기본 세션은 미션 크기·10.78 판본의 전용 풀을 만든다.</summary>
    [Fact]
    public void DefaultCreatesOwnPool()
    {
        var first = new Netstorm.Client.Host.HostSession(new Netstorm.Core.Original.Kernel(), new Netstorm.Core.Original.GameRandom(), new Netstorm.Core.Original.CrtRandom());
        var second = new Netstorm.Client.Host.HostSession(new Netstorm.Core.Original.Kernel(), new Netstorm.Core.Original.GameRandom(), new Netstorm.Core.Original.CrtRandom());
        Assert.Equal(Netstorm.Client.Host.HostSession.MissionPoolCapacity, first.Pool.Capacity);
        Assert.Equal(Netstorm.Core.Original.OriginalEdition.Patch1078, first.Pool.Edition);
        Assert.NotSame(first.Pool, second.Pool);
    }

    /// <summary>빌린 풀은 그대로 쓰고 덤프 바이트를 바꾸지 않는다.</summary>
    [Fact]
    public void BorrowedPoolIsUsedWithSameBytes()
    {
        var borrowed = new Netstorm.Core.Original.RawSidPool(Netstorm.Client.Host.HostSession.MissionPoolCapacity, true);
        var session = new Netstorm.Client.Host.HostSession(new Netstorm.Core.Original.Kernel(), new Netstorm.Core.Original.GameRandom(), new Netstorm.Core.Original.CrtRandom(), pool: borrowed);
        Assert.Same(borrowed, session.Pool);
        // 같은 크기의 새 풀은 덤프 바이트가 같다(풀 행 해시 중립).
        var fresh = new Netstorm.Core.Original.RawSidPool(Netstorm.Client.Host.HostSession.MissionPoolCapacity, true);
        Assert.Equal(
            Netstorm.Client.Automation.SessionStateDump.SessionDumpHash(fresh.Bytes.ToArray()),
            Netstorm.Client.Automation.SessionStateDump.SessionDumpHash(session.Pool.Bytes.ToArray()));
        // 빌린 풀에 할당한 번호는 세션 풀에서 보인다.
        int sid = session.Pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        Assert.Equal(5, sid);
        Assert.Equal(6, borrowed.FirstFree(true));
    }

    /// <summary>명시 null은 전용 풀을 만들고 판본·크기가 다르면 거부한다.</summary>
    [Fact]
    public void NullBuildsOwnAndMismatchRejects()
    {
        var own = new Netstorm.Client.Host.HostSession(new Netstorm.Core.Original.Kernel(), new Netstorm.Core.Original.GameRandom(), new Netstorm.Core.Original.CrtRandom(), pool: null);
        Assert.Equal(Netstorm.Client.Host.HostSession.MissionPoolCapacity, own.Pool.Capacity);
        // CD 판본 풀은 거부한다.
        var cd = new Netstorm.Core.Original.RawSidPool(Netstorm.Client.Host.HostSession.MissionPoolCapacity, true, edition: Netstorm.Core.Original.OriginalEdition.Cd1072);
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Client.Host.HostSession(new Netstorm.Core.Original.Kernel(), new Netstorm.Core.Original.GameRandom(), new Netstorm.Core.Original.CrtRandom(), pool: cd));
        // 크기가 다른 풀은 거부한다.
        var small = new Netstorm.Core.Original.RawSidPool(40000, true);
        Assert.Throws<System.ArgumentException>(() => new Netstorm.Client.Host.HostSession(new Netstorm.Core.Original.Kernel(), new Netstorm.Core.Original.GameRandom(), new Netstorm.Core.Original.CrtRandom(), pool: small));
    }
}
