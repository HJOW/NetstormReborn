using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>실제 세션의 현재 Player 전역 조회와 효과 사이의 재조회를 검사한다.</summary>
public sealed class PlayerParticipationCurrentInputsTests
{
    /// <summary>부분 바인딩은 기존 조회를 보존하며 로그인은 현재 로컬 전송 값을 읽는다.</summary>
    [Fact]
    public void BindingIsAtomicAndLoginReadsCurrentTransport()
    {
        bool local = true;
        int assigned = 0;
        var participant = new PlayerParticipation(new byte[9 * 180], new uint[81], new byte[9], new(),
            new() { AssignConnection = (_, _) => assigned++ });
        participant.BindCurrentInputs(new() { LocalOwner = () => 1, PlayerKnown = () => 0, Battle = () => false,
            Metamap = () => false, LocalTransport = () => local });
        Assert.Throws<ArgumentException>(() => participant.BindCurrentInputs(new() { LocalOwner = () => 2 }));
        Assert.True(participant.CheckLogin(1, 1));
        local = false;
        Assert.True(participant.CheckLogin(2, 2));
        Assert.Equal(1, assigned);
    }

    /// <summary>Chat 초기화 후 현재 번호를 다시 읽고 각 진단 뒤 현재 metamap을 다시 확인한다.</summary>
    [Fact]
    public void AllianceReadsLiveValuesAfterCallbacks()
    {
        uint owner = 1;
        bool metamap = false;
        int diagnostics = 0;
        var channels = new List<(uint Subscriber, uint Channel)>();
        var players = new byte[9 * 180];
        players[2 * 180 + PlayerParticipation.SubscriberOffset] = 42;
        var allies = new uint[81];
        allies[2 * 9 + 2] = 1;
        var hidden = new byte[9];
        hidden[0] = 1;
        var participant = new PlayerParticipation(players, allies, hidden, new(), new()
        {
            ClearChat = channel => { if (channel == 0x80) { owner = 2; metamap = true; } },
            Assertion = _ => { diagnostics++; metamap = false; },
            AddChat = (subscriber, _, channel) => channels.Add((subscriber, channel))
        });
        participant.BindCurrentInputs(new() { LocalOwner = () => owner, PlayerKnown = () => 1, Battle = () => false,
            Metamap = () => metamap, LocalTransport = () => false });
        participant.SetAlliance(1, 0);
        Assert.Equal(1, diagnostics);
        Assert.Contains((42U, 0x40U), channels);
        Assert.Contains((0U, 0x80U), channels);
    }

    /// <summary>첫 진단이 전투 모드를 바꾸면 변경된 값을 읽고 두 번째 진단도 현재 모드로 판정한다.</summary>
    [Fact]
    public void AddAllyReadsBattleAfterAssertion()
    {
        bool battle = false;
        bool metamap = true;
        int diagnostics = 0;
        int sent = 0;
        var participant = new PlayerParticipation(new byte[9 * 180], new uint[81], new byte[9], new(), new()
        {
            Assertion = _ => { diagnostics++; battle = true; metamap = false; },
            SendAlliance = (_, _, _) => sent++
        });
        participant.BindCurrentInputs(new() { LocalOwner = () => 1, PlayerKnown = () => 0, Battle = () => battle,
            Metamap = () => metamap, LocalTransport = () => false });
        participant.AddAlly(1, 2);
        Assert.Equal(1, diagnostics);
        Assert.Equal(1, sent);
    }
}
