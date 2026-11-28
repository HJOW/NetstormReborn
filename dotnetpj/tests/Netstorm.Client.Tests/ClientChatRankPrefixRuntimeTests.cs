using System.Text;
using Netstorm.Core.Original;

// 실제 Client manager와 현재 Core 관문을 전체 HUD의 기술 표시가 공유한다.
namespace Netstorm.Client.Tests;

/// <summary>원문 기술 표시 wrapper의 자연 관문과 현재 manager 재조회를 실제 할당에서 검사한다.</summary>
public sealed class ClientChatRankPrefixRuntimeTests
{
    /// <summary>manager0 또는 Core 비활성에서는 하위 Core와 아이콘 자산을 조회하지 않는다.</summary>
    [Fact]
    public void InactiveOwnersDoNotReadCoreOrIcons()
    {
        var manager = new ClientChatManagerGlobals();
        var subscriber = new ClientSubscriberGlobals();
        var active = new UIntBox { Value = 1 };
        // 자연 관문이 열리지 않았을 때 실제 조회가 생기면 실패한다.
        void Refresh() => ClientChatRuntime.RefreshClientChatRankPrefix(manager, subscriber, active,
            () => throw new InvalidOperationException("미선택 Core"), () => throw new InvalidOperationException("미선택 아이콘"),
            () => throw new InvalidOperationException("미선택 랭크"), _ => throw new InvalidOperationException("미선택 진단"));
        Refresh();
        manager.Current.Value = manager.Allocate(RawChatManagerPrefix.ManagerBytes);
        active.Value = 0;
        Refresh();
    }

    /// <summary>Core 조회 중 바뀐 현재 manager에 현재 랭크 아이콘을 쓰고 이전 manager는 보존한다.</summary>
    [Fact]
    public void RankRefreshUsesCurrentManagerAfterActualCoreRead()
    {
        var manager = new ClientChatManagerGlobals();
        uint first = manager.Allocate(RawChatManagerPrefix.ManagerBytes);
        uint second = manager.Allocate(RawChatManagerPrefix.ManagerBytes);
        Array.Fill(manager.Bytes(first), (byte)0x5a);
        Array.Fill(manager.Bytes(second), (byte)0x6b);
        manager.Current.Value = first;
        byte[] core = new byte[16];
        core[4] = 7;
        ClientChatRuntime.RefreshClientChatRankPrefix(manager, new(), new() { Value = 1 },
            () => { manager.Current.Value = second; return core; }, () => 4, () => 11, _ => throw new InvalidOperationException("예기치 않은 랭크 진단"));
        int prefix = (int)RawChatManagerPrefix.RankPrefixOffset;
        byte[] expected = Encoding.Latin1.GetBytes("~[Iicon.C0]~[Iicon.B3]\0");
        Assert.Equal(expected, manager.Bytes(second).AsSpan(prefix, expected.Length).ToArray());
        Assert.All(manager.Bytes(first), value => Assert.Equal((byte)0x5a, value));
        Assert.Equal((byte)0x6b, manager.Bytes(second)[prefix - 1]);
        Assert.Equal((byte)0x6b, manager.Bytes(second)[prefix + expected.Length]);
    }
}
