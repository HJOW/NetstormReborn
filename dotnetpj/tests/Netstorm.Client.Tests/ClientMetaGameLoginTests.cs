// 0x7c 메타 게임 로그인의 상태 분기와 0x1f 마스터 알림·고정 F 설치를 대조한다.
// cpppj/src/client/ClientCampaignTransport.cpp 의 ReceiveClientMetaGameLogin(004b2c00)/NotifyClientMetaMaster/InstallClientMetaGameLoginReceiver(0x7c 0x004b2cc0)를 대조한다.
// 0.946에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 004b2c60·004b2c00 메타 게임 로그인의 실제 CS·연결 효과를 기록한다.
public sealed class ClientMetaGameLoginTests
{
    /// <summary>같은 실제 CS와 송신·연결·서버·캐시 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();
        /// <summary>현재 전투 소유자 번호다.</summary>
        public uint Owner = 1;
        /// <summary>현재 구독자 번호다.</summary>
        public uint Subscriber = 0x1234;
        /// <summary>실제 송신한 연결 번호와 패킷 사본이다.</summary>
        public readonly List<(int Id, byte[] Packet)> Sent = [];
        /// <summary>실제 끊은 연결 번호다.</summary>
        public readonly List<int> Disconnected = [];
        /// <summary>실제 재지정한 pending·target·서버 여부다.</summary>
        public readonly List<(int Pending, int Target, bool Server)> Assigned = [];
        /// <summary>현재 서버 여부다.</summary>
        public bool Server;
        /// <summary>서버 여부의 쓰기 순서다.</summary>
        public readonly List<bool> ServerSets = [];
        /// <summary>현재 캐시한 참가자 번호다.</summary>
        public uint CachedPeer;

        /// <summary>같은 실제 CS를 준비한다.</summary>
        public Scene()
        {
            Challenge.Prepare();
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientMetaGameLoginHooks Hooks() => new()
        {
            Challenge = Challenge,
            BattleOwner = () => Owner,
            CurrentSubscriber = () => Subscriber,
            Send = (id, packet) => { Sent.Add((id, packet.ToArray())); return true; },
            Disconnect = id => Disconnected.Add(id),
            Assign = (pending, target, server) => Assigned.Add((pending, target, server)),
            GetServer = () => Server,
            SetServer = value => { ServerSets.Add(value); Server = value; },
            SetCachedPeer = value => CachedPeer = value,
        };

        /// <summary>실제 참가자의 전투 번호를 쓴다.</summary>
        public void BattleOf(uint participant, uint battle) =>
            BinaryPrimitives.WriteUInt32LittleEndian(Challenge.Participant(participant).Span.Slice(0xa4, 4), battle);

        /// <summary>실제 참가자의 구독자 번호를 쓴다.</summary>
        public void SubscriberOf(uint participant, uint subscriber) =>
            BinaryPrimitives.WriteUInt32LittleEndian(Challenge.Participant(participant).Span.Slice(4, 4), subscriber);

        /// <summary>실제 참가자의 자리 번호를 쓴다.</summary>
        public void SeatOf(uint participant, uint seat) =>
            BinaryPrimitives.WriteUInt32LittleEndian(Challenge.Participant(participant).Span.Slice(0xa8, 4), seat);

        /// <summary>실제 참가자의 flags DWORD를 쓴다.</summary>
        public void FlagsOf(uint participant, uint flags) =>
            BinaryPrimitives.WriteUInt32LittleEndian(Challenge.Participant(participant).Span.Slice(0x64, 4), flags);

        /// <summary>실제 전투 기록의 8칸에 후보를 쓴다.</summary>
        public void CandidatesOf(uint battle, params uint[] candidates)
        {
            Span<byte> span = Challenge.Battle(battle).Span;
            for (int slot = 0; slot < candidates.Length && slot < 8; slot++)
            {
                BinaryPrimitives.WriteUInt32LittleEndian(span.Slice(0x38 + slot * 4, 4), candidates[slot]);
            }
        }

        /// <summary>원문 0x7c 28BYTE 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(uint status, uint first, uint second, uint third)
        {
            byte[] packet = new byte[28];
            packet.AsSpan().Fill(0xff);
            packet[0] = 1;
            packet[1] = 0x7c;
            packet[2] = 28;
            packet[3] = 0;
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), status);
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(8, 4), first);
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(12, 4), second);
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(16, 4), third);
            return packet;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>미연결 장치와 소유자 불일치는 일반 성공으로 꾸미지 않는다. 0.946에서 추가됐다.</summary>
    [Fact]
    public void NullDevicesAndOwnerMismatchFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientMetaGameLogin.Receive(null!, 0, new byte[28]));
        Assert.Throws<ArgumentNullException>(() => ClientMetaGameLogin.NotifyMaster(null!));
        var bare = new ClientMetaGameLoginHooks();
        Assert.Throws<InvalidOperationException>(() => ClientMetaGameLogin.Receive(bare, 0, new byte[28]));
        Assert.Throws<ArgumentNullException>(() => ClientMetaGameLogin.Install(null!, new(), new(new(new(), new())), new(), new()));
        var game = new ClientGameplayGlobals();
        var other = new ClientGameplayGlobals();
        var connections = new ClientConnectionGlobals(new ClientLocalStateGlobals(other, new()));
        var dispatch = Dispatch();
        var surface = new ClientSurfaceGlobals(game, new(new ClientLocalStateGlobals(game, new())), new Kernel(), () => 0);
        Assert.Throws<ArgumentException>(() => ClientMetaGameLogin.Install(dispatch, game, connections, scene.Challenge, surface.Storage));
    }

    /// <summary>원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.946에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var game = new ClientGameplayGlobals();
        var local = new ClientLocalStateGlobals(game, new());
        var connections = new ClientConnectionGlobals(local);
        var surface = new ClientSurfaceGlobals(game, new(local), new Kernel(), () => 0);
        var dispatch = Dispatch();
        ClientMetaGameLogin.Install(dispatch, game, connections, scene.Challenge, surface.Storage);
        var row = dispatch.Registration(0x7c);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(28, row.Length);
        Assert.NotNull(row.Observer);
        Assert.Null(row.PacketObserver);
        Assert.Equal(0x004b2cc0u, dispatch.SnapshotReceiver(0x7c)!.NativeEntry);
        dispatch.InvokeDirect(0x7c, 9, null, Scene.Packet(2, 0, 0, 0));
        Assert.Equal(0xffffffffu, local.CachedPeerIndex.Value);
    }

    /// <summary>상태 2 이외는 캐시한 DWORD만 쓰고 송신·연결을 건드리지 않는다. 0.946에서 추가됐다.</summary>
    [Fact]
    public void OtherStatusSetsCachedPeerOnly()
    {
        var scene = new Scene();
        ClientMetaGameLogin.Receive(scene.Hooks(), 7, Scene.Packet(2, 0, 0, 0));
        Assert.Equal(0xffffffffu, scene.CachedPeer);
        Assert.Empty(scene.Sent);
        Assert.Empty(scene.Disconnected);
        Assert.Empty(scene.Assigned);
        Assert.False(scene.Server);
    }

    /// <summary>master 불일치는 거절 응답만 보내고 재지정하지 않는다. 0.946에서 추가됐다.</summary>
    [Fact]
    public void Status0MasterMismatchRepliesRejected()
    {
        var scene = new Scene();
        scene.BattleOf(1, 2);
        scene.CandidatesOf(2, 5);
        scene.FlagsOf(5, 4);
        ClientMetaGameLogin.Receive(scene.Hooks(), 7, Scene.Packet(0, 0, 1, 0));
        Assert.Empty(scene.Disconnected);
        Assert.Empty(scene.Assigned);
        Assert.False(scene.Server);
        var sent = Assert.Single(scene.Sent);
        Assert.Equal(7, sent.Id);
        Assert.Equal(28, sent.Packet.Length);
        Assert.Equal((byte)1, sent.Packet[0]);
        Assert.Equal((byte)0x7c, sent.Packet[1]);
        Assert.Equal((byte)28, sent.Packet[2]);
        Assert.Equal((byte)0, sent.Packet[3]);
        Assert.Equal(2u, BinaryPrimitives.ReadUInt32LittleEndian(sent.Packet.AsSpan(4, 4)));
        Assert.Equal(0x1234u, BinaryPrimitives.ReadUInt32LittleEndian(sent.Packet.AsSpan(8, 4)));
        Assert.All(sent.Packet.AsSpan(12).ToArray(), value => Assert.Equal(0xff, value));
    }

    /// <summary>전체 일치는 수락 응답 뒤 끊고 재지정하며 서버를 되돌린다. 0.946에서 추가됐다.</summary>
    [Fact]
    public void Status0FullMatchAcceptsAndReassigns()
    {
        var scene = new Scene();
        scene.BattleOf(1, 2);
        scene.CandidatesOf(2, 1);
        scene.FlagsOf(1, 4);
        scene.SubscriberOf(1, 0x5555);
        scene.SeatOf(1, 9);
        byte[] packet = Scene.Packet(0, 0, 1, 0x5555);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(20, 4), 2);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(24, 4), 9);
        ClientMetaGameLogin.Receive(scene.Hooks(), 7, packet);
        var sent = Assert.Single(scene.Sent);
        Assert.Equal(7, sent.Id);
        Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(sent.Packet.AsSpan(4, 4)));
        Assert.Equal(0x1234u, BinaryPrimitives.ReadUInt32LittleEndian(sent.Packet.AsSpan(8, 4)));
        Assert.Equal([10], scene.Disconnected);
        Assert.Equal([(7, 10, true)], scene.Assigned);
        Assert.Equal([true, false], scene.ServerSets);
        Assert.False(scene.Server);
    }

    /// <summary>상태 1에서 master가 없으면 그대로 돌아간다. 0.946에서 추가됐다.</summary>
    [Fact]
    public void Status1MasterMissingReturns()
    {
        var scene = new Scene();
        scene.BattleOf(1, 0);
        ClientMetaGameLogin.Receive(scene.Hooks(), 7, Scene.Packet(1, 0x5555, 0, 0));
        Assert.Empty(scene.Sent);
        Assert.Equal(0u, scene.CachedPeer);
    }

    /// <summary>상태 1에서 구독자가 맞을 때만 0x1f 마스터 알림을 보낸다. 0.946에서 추가됐다.</summary>
    [Fact]
    public void Status1SubscriberMatchNotifiesOnlyOnMatch()
    {
        var scene = new Scene();
        scene.BattleOf(1, 2);
        scene.CandidatesOf(2, 5);
        scene.FlagsOf(5, 4);
        scene.SubscriberOf(5, 0x5555);
        scene.FlagsOf(1, 0x100);
        ClientMetaGameLogin.Receive(scene.Hooks(), 7, Scene.Packet(1, 0x9999, 0, 0));
        Assert.Empty(scene.Sent);
        ClientMetaGameLogin.Receive(scene.Hooks(), 7, Scene.Packet(1, 0x5555, 0, 0));
        var sent = Assert.Single(scene.Sent);
        Assert.Equal(11, sent.Id);
        Assert.Equal((byte)1, sent.Packet[0]);
        Assert.Equal((byte)0x1f, sent.Packet[1]);
        Assert.Equal((byte)12, sent.Packet[2]);
        Assert.Equal((byte)0, sent.Packet[3]);
        Assert.Equal(0x1234u, BinaryPrimitives.ReadUInt32LittleEndian(sent.Packet.AsSpan(4, 4)));
        Assert.Equal(0x2100u, BinaryPrimitives.ReadUInt32LittleEndian(sent.Packet.AsSpan(8, 4)));
        Assert.Equal(0x2100u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Participant(1).Span.Slice(0x64, 4)));
    }
}
