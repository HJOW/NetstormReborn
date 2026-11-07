// 프로세스 상태의 독립값·빌림 전환을 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 프로세스 상태를 검증하는 테스트 클래스다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.
public sealed class RawSquidProcessStateTests
{
    /// <summary>독립 입력은 원문 PE 초기값으로 시작한다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    [Fact]
    public void DefaultsAreNativeInitials()
    {
        var state = new SquidProcessState();
        Assert.Equal(0.0, state.Now);
        Assert.True(state.Boss);
        Assert.True(state.Server);
        Assert.Equal(0u, state.TypeRotation);
        Assert.Equal(122u, state.PollType);
        Assert.Equal(46u, state.RegularType());
        Assert.Equal(61u, state.SharedRegularType());
        Assert.Equal(63u, state.ProcessTypeEnd());
        Assert.Equal(0u, state.PostPopDepth);
        Assert.Null(state.Assertion);
        Assert.Equal(2u, state.ProcessFormType());
        Assert.Equal(35u, state.FireAnimationType());
        Assert.Equal(36u, state.RotateAnimationType());
        Assert.Null(state.SharedAuthority);
        Assert.Null(state.SharedNow);
        Assert.Null(state.SharedPollType);
    }

    /// <summary>독립 값은 인스턴스마다 읽고 쓴다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    [Fact]
    public void IndependentValuesReadAndWrite()
    {
        var state = new SquidProcessState
        {
            TypeRotation = 7u,
            PostPopDepth = 3u,
        };
        state.Now = 12.5;
        state.Boss = false;
        state.Server = false;
        state.PollType = 130u;
        Assert.Equal(12.5, state.Now);
        Assert.False(state.Boss);
        Assert.False(state.Server);
        Assert.Equal(130u, state.PollType);
        Assert.Equal(7u, state.TypeRotation);
        Assert.Equal(3u, state.PostPopDepth);
        var other = new SquidProcessState();
        Assert.Equal(0.0, other.Now);
        Assert.True(other.Boss);
        Assert.Equal(122u, other.PollType);
    }

    /// <summary>빌린 권한은 같은 물리를 직접 읽고 쓴다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    [Fact]
    public void SharedAuthorityPinsBothFlags()
    {
        var authority = new RawAuthorityState { Boss = false, Server = false };
        var state = new SquidProcessState { SharedAuthority = authority };
        Assert.False(state.Boss);
        Assert.False(state.Server);
        state.Boss = true;
        state.Server = true;
        Assert.True(authority.Boss);
        Assert.True(authority.Server);
        authority.Boss = false;
        Assert.False(state.Boss);
        state.SharedAuthority = null;
        Assert.True(state.Boss);
    }

    /// <summary>빌린 시각은 같은 주소를 직접 읽고 쓴다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    [Fact]
    public void SharedNowPinsCurrentTime()
    {
        var box = new DoubleBox { Value = 100.0 };
        var state = new SquidProcessState { SharedNow = box };
        Assert.Equal(100.0, state.Now);
        state.Now = 200.0;
        Assert.Equal(200.0, box.Value);
        box.Value = 300.0;
        Assert.Equal(300.0, state.Now);
        state.SharedNow = null;
        state.Now = 400.0;
        Assert.Equal(400.0, state.Now);
        Assert.Equal(300.0, box.Value);
    }

    /// <summary>빌린 간헐천 타입은 같은 장기 DWORD를 직접 읽고 쓴다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    [Fact]
    public void SharedPollTypePinsLongLivedDword()
    {
        var box = new UIntBox { Value = 90u };
        var state = new SquidProcessState { SharedPollType = box };
        Assert.Equal(90u, state.PollType);
        state.PollType = 91u;
        Assert.Equal(91u, box.Value);
        state.SharedPollType = null;
        Assert.Equal(122u, state.PollType);
    }

    /// <summary>빌린 번호 상자들은 getter 호출마다 현재 값을 읽는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentBoxesReadLiveValues()
    {
        var regular = new UIntBox { Value = 47u };
        var shared = new UIntBox { Value = 62u };
        var end = new UIntBox { Value = 64u };
        var form = new UIntBox { Value = 5u };
        var fire = new UIntBox { Value = 37u };
        var rotate = new UIntBox { Value = 38u };
        var state = new SquidProcessState
        {
            CurrentRegularType = regular,
            CurrentSharedRegularType = shared,
            CurrentProcessTypeEnd = end,
            CurrentProcessFormType = form,
            CurrentFireAnimationType = fire,
            CurrentRotateAnimationType = rotate,
        };
        Assert.Equal(47u, state.RegularType());
        Assert.Equal(62u, state.SharedRegularType());
        Assert.Equal(64u, state.ProcessTypeEnd());
        Assert.Equal(5u, state.ProcessFormType());
        Assert.Equal(37u, state.FireAnimationType());
        Assert.Equal(38u, state.RotateAnimationType());
        regular.Value = 48u;
        Assert.Equal(48u, state.RegularType());
    }
}
