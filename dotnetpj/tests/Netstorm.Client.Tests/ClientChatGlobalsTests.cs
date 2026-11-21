using Netstorm.Client.Chat;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 장기 구독자 주소/이름/채팅이 실제 월드와 Session 수명 사이에서 공유되는지 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>PE 초기값과 독립 fallback·현재 getter·Session 반납 경계를 검증한다.</summary>
public sealed class ClientChatGlobalsTests
{
    /// <summary>외부 구독자를 명시 제공한 제품 조합만 같은 두 DWORD를 양방향으로 빌린다.</summary>
    [Fact]
    public void ProductWorldBorrowsNativeSubscribersAndCopyRemainsIndependent()
    {
        var subscribers = new ClientSubscriberGlobals(); var game = new ClientGameplayGlobals(subscribers);
        Assert.Same(subscribers, game.Subscribers); Assert.Equal(uint.MaxValue, game.World.MySubscriber); Assert.Equal(1u, game.World.LoginSubscriber); Assert.Equal(64, subscribers.Handle.Length); Assert.All(subscribers.Handle, value => Assert.Equal(0, value));
        subscribers.MySubscriber.Value = 0x80000001; game.World.LoginSubscriber = 0xfedcba98;
        Assert.Equal(0x80000001u, game.World.MySubscriber); Assert.Equal(0xfedcba98u, subscribers.LoginSubscriber.Value);
        var copy = new CampaignWorldGlobals(game.World); game.World.MySubscriber = game.World.LoginSubscriber; subscribers.LoginSubscriber.Value = 73;
        Assert.Equal(0xfedcba98u, subscribers.MySubscriber.Value); Assert.Equal(73u, game.World.LoginSubscriber); Assert.Equal(0x80000001u, copy.MySubscriber); Assert.Equal(0xfedcba98u, copy.LoginSubscriber);
        var other = new ClientGameplayGlobals(subscribers); Assert.Equal(0xfedcba98u, other.World.MySubscriber); Assert.Equal(73u, other.World.LoginSubscriber);
    }
    /// <summary>구독자 없는 기존 독립 조합은0/0과 Session별 이름 물리를 유지한다.</summary>
    [Fact]
    public void StandaloneGamesAndSessionsKeepIndependentFallbacks()
    {
        var game = new ClientGameplayGlobals(); var first = new HostSession(new(), new(17), new(23), sharedGameplay: game); var second = new HostSession(new(), new(19), new(29), sharedGameplay: game);
        Assert.Null(game.Subscribers); Assert.Equal(0u, game.World.MySubscriber); Assert.Equal(0u, game.World.LoginSubscriber); Assert.NotSame(first.HandleBuffer, second.HandleBuffer);
        first.FillHandleName([(byte)'A', 0]); Assert.Equal((byte)'A', first.HandleBuffer[0]); Assert.Equal(0, second.HandleBuffer[0]); Assert.Equal(0u, first.WorldGlobals.MySubscriber);
    }
    /// <summary>실제 Runtime 반납 뒤 새 Session도 같은 이름 꼬리와 두 DWORD를 그대로 빌린다.</summary>
    [Fact]
    public void PreparedSessionRetirementPreservesClientHandleAndSubscribers()
    {
        var subscribers = new ClientSubscriberGlobals(); var game = new ClientGameplayGlobals(subscribers); var local = new ClientLocalStateGlobals(game, new()); var surface = new ClientSurfaceGlobals(game, new(local), new Kernel(), () => 0);
        subscribers.Handle.AsSpan().Fill(0xa5); subscribers.LoginSubscriber.Value = 0x80000000;
        var first = new HostSession(new(), new(17), new(23), sharedGameplay: game, pool: surface.Storage.Pool);
        Assert.Same(subscribers.Handle, first.HandleBuffer); first.FillHandleName([(byte)'N', (byte)'S', 0]);
        using (var runtime = HostMissionRuntime.Create(first, surface.Storage)) { first.WorldGlobals.MySubscriber = 0xfedcba98; }
        var second = new HostSession(new(), new(19), new(29), sharedGameplay: game, pool: surface.Storage.Pool);
        Assert.Same(first.HandleBuffer, second.HandleBuffer); Assert.Equal(new byte[] { (byte)'N', (byte)'S', 0, 0xa5 }, second.HandleBuffer[..4]); Assert.Equal(0xa5, second.HandleBuffer[63]); Assert.Equal(0xfedcba98u, second.WorldGlobals.MySubscriber); Assert.Equal(0x80000000u, second.WorldGlobals.LoginSubscriber);
    }
    /// <summary>월드가 없어도 현재 전체 DWORD로 자기 채팅을 거르고 나중의 변경을 즉시 읽는다.</summary>
    [Fact]
    public void ChatUsesCurrentSubscriberBeforeAndAfterWorldWrites()
    {
        var subscribers = new ClientSubscriberGlobals(); var chat = new ClientChatGlobals(OriginalEdition.Patch1078, () => subscribers.MySubscriber.Value); var game = new ClientGameplayGlobals(subscribers);
        Assert.Same(chat.Records, chat.Channels.Records); Assert.Equal(4800, chat.Records.Length); Assert.Equal(-1, chat.Channels.Add(uint.MaxValue, "self", 0xff)); Assert.All(chat.Records, value => Assert.Equal(0, value));
        game.World.MySubscriber = 7; Assert.Equal(-1, chat.Channels.Add(7, "self", 0xff)); Assert.Equal(0, chat.Channels.Add(uint.MaxValue, "Other", 0xff)); byte[] before = (byte[])chat.Records.Clone();
        subscribers.MySubscriber.Value = uint.MaxValue; Assert.Equal(-1, chat.Channels.Add(uint.MaxValue, "changed", 0)); Assert.Equal(before, chat.Records); Assert.Equal(1, chat.Channels.Add(0x80000000, "Peer", 0));
    }
    /// <summary>CD는 기록만 소유하고 PATCH 본체를 대신 만들거나 조회를 미리 호출하지 않는다.</summary>
    [Fact]
    public void CdKeepsBssRecordsAndFailsOnlyWhenChannelsAreSelected()
    {
        int reads = 0; var chat = new ClientChatGlobals(OriginalEdition.Cd1072, () => { reads++; return 1; });
        Assert.Equal(0, reads); Assert.Equal(4800, chat.Records.Length); chat.Records[17] = 0xa5; Assert.Throws<InvalidOperationException>(() => chat.Channels); Assert.Equal(0, reads); Assert.Equal(0xa5, chat.Records[17]);
    }
}
