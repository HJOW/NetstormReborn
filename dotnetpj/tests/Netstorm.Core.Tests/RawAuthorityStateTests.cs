// 실제 서버/권한 두 논리 전역의 초기값·독립성·관문 쓰기를 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawAuthorityState를 검증하는 테스트 클래스다. 2026-11-07 F2d 서피스 월드 권한 이식에서 추가됐다.
public sealed class RawAuthorityStateTests
{
    /// <summary>두 PE의 초기 DWORD 1을 논리 true로 시작한다. 2026-11-07 F2d 서피스 월드 권한 이식에서 추가됐다.</summary>
    [Fact]
    public void DefaultsAreBothTrue()
    {
        var state = new RawAuthorityState();
        Assert.True(state.Server);
        Assert.True(state.Boss);
    }

    /// <summary>인스턴스마다 독립 전역이다. 2026-11-07 F2d 서피스 월드 권한 이식에서 추가됐다.</summary>
    [Fact]
    public void InstancesAreIndependent()
    {
        var first = new RawAuthorityState();
        var second = new RawAuthorityState();
        first.Server = false;
        first.Boss = false;
        Assert.True(second.Server);
        Assert.True(second.Boss);
    }

    /// <summary>서버·권한 관문이 현재값을 읽고 쓴다. 2026-11-07 F2d 서피스 월드 권한 이식에서 추가됐다.</summary>
    [Fact]
    public void GatesReadCurrentValues()
    {
        var state = new RawAuthorityState { Server = false, Boss = true };
        Assert.False(state.Server);
        Assert.True(state.Boss);
        state.Server = true;
        state.Boss = false;
        Assert.True(state.Server);
        Assert.False(state.Boss);
    }
}
