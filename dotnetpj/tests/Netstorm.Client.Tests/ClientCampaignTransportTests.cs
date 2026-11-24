// 수송 생성·재결합·Publish의 실제 원문 순서를 같은 분배기에 잇는 부모를 검증한다.
// cpppj/src/client/ClientCampaignTransport.cpp 의 MakeClientCampaignTransport(004b4710)와
// CompositeMissionTransport::Create·SharesNetwork·RebindWorld·SharedNetwork를 대조한다.
// 0.967에서 R4-6 전송·수신(수송 팩토리) 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Display;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 수송 팩토리의 실제 설치·후크·재결합·Publish를 확인한다.
public sealed class ClientCampaignTransportTests
{
    /// <summary>같은 장기 Client 장치와 실제 CS 소유자를 묶는다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 두 구독자 DWORD다.</summary>
        public readonly ClientSubscriberGlobals Subscribers = new();

        /// <summary>같은 실제 Player/권한/현재 세션 소유자다.</summary>
        public readonly ClientGameplayGlobals Game;

        /// <summary>같은 실제 State/동기화 장기 주소다.</summary>
        public readonly ClientLocalStateGlobals Local;

        /// <summary>같은 실제 연결 Registry/Source 장부다.</summary>
        public readonly ClientConnectionGlobals Connections;

        /// <summary>같은 실제 표면/시계/커널이다.</summary>
        public readonly ClientSurfaceGlobals Surface;

        /// <summary>창이 없어도 존재하는 실제 마스크/현재 UI 슬롯이다.</summary>
        public readonly ClientChatWindowGlobals Chat = new();

        /// <summary>같은 Client 수명으로 유지하는 실제 CS 슬롯·할당 owner다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();

        /// <summary>원점/정지 상태를 복제하지 않는 실제 Client 시계다.</summary>
        public readonly OriginalGameClock Clock = new(0);

        /// <summary>실제 장기 Client 장치만 읽으며 실제 수신 UI/진단/활동 장치를 묶는다.</summary>
        public Scene()
        {
            Game = new(Subscribers); Local = new(Game, new()); Connections = new(Local); Surface = new(Game, new(Local), new Kernel(), () => 1000);
        }

        /// <summary>같은 장기 Client 장치로 실제 수송을 만든다.</summary>
        public CompositeMissionTransport Make(
            CompositeTransportRole role = CompositeTransportRole.InternalMission,
            Func<uint>? wallMilliseconds = null,
            List<(bool Outgoing, PacketConnection Node, byte[] Packet, ushort Length)>? records = null,
            MissionNetwork? network = null)
        {
            wallMilliseconds ??= () => 100u;
            records ??= [];
            return ClientCampaignTransport.Make(Game, Connections, Surface, Chat, _ => { }, (_, _, _) => { },
                new() { WallSeconds = () => 100.0, FrameNumber = () => 100, Server = () => Game.Authority.Server, EraseStatus = (_, _) => { } },
                Challenge, role, Clock, wallMilliseconds,
                (outgoing, node, packet, length) => records.Add((outgoing, node, packet.ToArray(), length)), network);
        }
    }

    /// <summary>수신 방향 관찰기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        var hooks = new PacketDispatchHooks { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } };
        Assert.Throws<ArgumentNullException>(() => ClientCampaignTransport.Make(null!, scene.Connections, scene.Surface, scene.Chat, _ => { }, (_, _, _) => { }, new(), scene.Challenge, CompositeTransportRole.InternalMission, scene.Clock, () => 0, (_, _, _, _) => { }, null));
        Assert.Throws<ArgumentNullException>(() => ClientCampaignTransport.Make(scene.Game, null!, scene.Surface, scene.Chat, _ => { }, (_, _, _) => { }, new(), scene.Challenge, CompositeTransportRole.InternalMission, scene.Clock, () => 0, (_, _, _, _) => { }, null));
        Assert.Throws<ArgumentNullException>(() => new CompositeMissionTransport(CompositeTransportRole.InternalMission, new byte[1620], null!, hooks, scene.Connections.Registry, new object()));
        Assert.Null(new MissionNetwork().Composite);
        Assert.Null(new MissionNetwork().Lifetime);
    }

    /// <summary>지원하지 않는 역할은 명시 실패하며 표를 바꾸지 않는다. 0.967에서 추가됐다.</summary>
    [Theory]
    [InlineData(CompositeTransportRole.Editor)]
    [InlineData(CompositeTransportRole.LanHost)]
    [InlineData(CompositeTransportRole.LanClient)]
    public void UnsupportedRoleFailsExplicitly(CompositeTransportRole role)
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        var hooks = new PacketDispatchHooks { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } };
        Assert.Throws<NotSupportedException>(() => scene.Make(role));
        Assert.Throws<NotSupportedException>(() => new CompositeMissionTransport(role, new byte[1620], () => false, hooks, scene.Connections.Registry, new object()));
    }

    /// <summary>미초기화 Player로는 수송을 만들지 않는다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void UninitializedPlayersFailExplicitly()
    {
        var scene = new Scene();
        Assert.False(scene.Game.PlayerRecords.Initialized);
        Assert.Throws<InvalidOperationException>(() => scene.Make());
    }

    /// <summary>같은 기본 표를 채운 새 수송을 만들고 Source 장부에 공개한다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void CreatePathInstallsDefaultsAndPublishes()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        var transport = scene.Make();
        Assert.False(transport.Released);
        Assert.Equal(CompositeTransportRole.InternalMission, transport.Role);
        Assert.Same(scene.Connections.Registry, transport.Registry);
        Assert.Equal(0x0042dc90u, transport.Dispatch.SnapshotReceiver(0x41)!.NativeEntry);
        Assert.Equal(0x004b4790u, transport.Dispatch.SnapshotReceiver(0x7b)!.NativeEntry);
        Assert.Equal(0x004b2cc0u, transport.Dispatch.SnapshotReceiver(0x7c)!.NativeEntry);
        Assert.Equal(0x0048f7f0u, transport.Dispatch.SnapshotReceiver(0x77)!.NativeEntry);
        Assert.Equal(0x00444af0u, transport.Dispatch.SnapshotReceiver(0x3e)!.NativeEntry);
        Assert.Equal(0x0044dc20u, transport.Dispatch.SnapshotReceiver(0x32)!.NativeEntry);
        Assert.NotNull(transport.Dispatch.SnapshotReceiver(0x0b));
        // 같은 Source 장부의 최신 경로로 선택된다.
        using (Assert.IsType<NativeBattleSource>(scene.Connections.PacketSources.Resolve(transport.InternalTransport.Client))) { }
        transport.Dispose();
        Assert.True(transport.Released);
    }

    /// <summary>송신 후크는 방향1 기록을 남긴다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void SentHookRecordsOutgoingDirection()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        scene.Game.Authority.Server = true;
        var records = new List<(bool Outgoing, PacketConnection Node, byte[] Packet, ushort Length)>();
        using var transport = scene.Make(records: records);
        transport.InternalTransport.RegisterLogin((_, _, _) => { });
        transport.Serve(6799);
        transport.Connect(new byte[20], 6799);
        Assert.Equal(0, transport.Poll());
        transport.Assign(-1, 1);
        byte[] packet = [1, 0x24, 5, 0, 77];
        Assert.True(transport.Send(9, packet));
        var sent = Assert.Single(records);
        Assert.True(sent.Outgoing);
        Assert.Equal(5, sent.Length);
        Assert.Equal((byte)0x24, sent.Packet[1]);
    }

    /// <summary>수신 후크는 방향0 기록을 남긴다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void ReceiveHookRecordsIncomingDirection()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        var records = new List<(bool Outgoing, PacketConnection Node, byte[] Packet, ushort Length)>();
        using var transport = scene.Make(records: records);
        transport.Dispatch.RegisterCanonicalDirect(0x24, 0x9999, static (_, _, _) => { });
        var connection = new PacketConnection();
        byte[] packet = [1, 0x24, 5, 0, 77];
        Assert.True(transport.Dispatch.Dispatch(connection, packet));
        var received = Assert.Single(records);
        Assert.False(received.Outgoing);
        Assert.Equal(5, received.Length);
    }

    /// <summary>벽시계 후크는 같은 실제 시계로 현재 DWORD를 만든다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void NowHookUsesClientClock()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        uint wall = 123456u;
        using var transport = scene.Make(wallMilliseconds: () => wall);
        transport.InternalTransport.RegisterLogin((_, _, _) => { });
        transport.Serve(6799);
        transport.Connect(new byte[20], 6799);
        uint expected = unchecked((uint)scene.Clock.WallSeconds(wall));
        Assert.Equal(expected, transport.InternalTransport.Client.Received);
        Assert.Equal(expected, transport.InternalTransport.Client.Heartbeat);
    }

    /// <summary>종료 후크는 원문 flags 0x40만 세운다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void TerminateHookSetsCloseFlag()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        var records = new List<(bool Outgoing, PacketConnection Node, byte[] Packet, ushort Length)>();
        using var transport = scene.Make(records: records);
        transport.Dispatch.Version(0x41, 9, 8);
        var connection = new PacketConnection();
        Assert.False(transport.Dispatch.Dispatch(connection, new byte[] { 1, 0x41, 4, 0 }));
        Assert.Equal(0x40u, connection.Flags & 0x40u);
        Assert.Empty(records);
    }

    /// <summary>닫기·반납 후크는 R11 backend 없이 명시 실패한다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void CloseAndReleaseHooksFailExplicitly()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        transport.Dispatch.RegisterCanonicalDirect(0x24, 0x9999, static (_, node, _) => node!.Flags |= 0x40u);
        var error = Assert.Throws<InvalidOperationException>(() => transport.Dispatch.Dispatch(new PacketConnection(), new byte[] { 1, 0x24, 5, 0, 0 }));
        Assert.Contains("R11", error.Message);
    }

    /// <summary>같은 owner 재진입은 같은 수송을 돌려주고 후크를 바꾼다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void RebindPathReusesTransportAndSwapsHooks()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        scene.Game.Authority.Server = true;
        var first = new List<(bool Outgoing, PacketConnection Node, byte[] Packet, ushort Length)>();
        using var transport = scene.Make(records: first);
        transport.InternalTransport.RegisterLogin((_, _, _) => { });
        transport.Serve(6799);
        transport.Connect(new byte[20], 6799);
        Assert.Equal(0, transport.Poll());
        transport.Assign(-1, 1);
        var second = new List<(bool Outgoing, PacketConnection Node, byte[] Packet, ushort Length)>();
        var network = transport.SharedNetwork();
        Assert.True(transport.SharesNetwork(network));
        var rebound = scene.Make(records: second, network: network);
        Assert.Same(transport, rebound);
        byte[] packet = [1, 0x24, 5, 0, 77];
        Assert.True(rebound.Send(9, packet));
        Assert.Empty(first);
        Assert.Single(second);
    }

    /// <summary>다른 Registry·경로는 재결합을 거부한다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void RebindWithForeignOwnerFailsExplicitly()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        var other = new Scene();
        other.Game.PlayerRecords.Initialize((_, _) => { });
        var foreign = new MissionNetwork { Composite = transport };
        Assert.Throws<InvalidOperationException>(() => scene.Make(network: foreign));
        var stranger = new Scene();
        stranger.Game.PlayerRecords.Initialize((_, _) => { });
        var otherTransport = stranger.Make();
        try
        {
            var crossed = new MissionNetwork { Composite = otherTransport, Lifetime = otherTransport.SharedNetwork().Lifetime };
            Assert.Throws<InvalidOperationException>(() => scene.Make(network: crossed));
        }
        finally
        {
            otherTransport.Dispose();
        }
    }

    /// <summary>같은 network 식별 규칙을 공유한다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void SharesNetworkMatchesSameCoordinator()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        Assert.False(transport.SharesNetwork(null));
        Assert.False(transport.SharesNetwork(new MissionNetwork()));
        var network = transport.SharedNetwork();
        Assert.Same(transport, network.Composite);
        Assert.True(transport.SharesNetwork(network));
        using var other = scene.Make();
        Assert.False(transport.SharesNetwork(other.SharedNetwork()));
        Assert.False(transport.SharesNetwork(new MissionNetwork { Composite = transport, Lifetime = new object() }));
    }

    /// <summary>콜백 진행 중 재인계는 명시 실패한다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void RebindDuringCallbackFailsExplicitly()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        using var lease = transport.BindSession(scene.Game);
        using var pin = lease.Pin();
        var hooks = new Netstorm.Net.PacketDispatchHooks { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } };
        Assert.Throws<InvalidOperationException>(() => transport.RebindWorld(CompositeTransportRole.InternalMission, new byte[1620], () => false, hooks, new object()));
    }

    /// <summary>재결합 인자 누락·역할·길이는 원문 순서로 거부한다. 0.967에서 추가됐다.</summary>
    [Fact]
    public void RebindValidatesArgumentsExplicitly()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        var hooks = new Netstorm.Net.PacketDispatchHooks { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } };
        Assert.Throws<ArgumentNullException>(() => transport.RebindWorld(CompositeTransportRole.InternalMission, new byte[1620], null!, hooks, new object()));
        Assert.Throws<ArgumentNullException>(() => transport.RebindWorld(CompositeTransportRole.InternalMission, new byte[1620], () => false, null!, new object()));
        Assert.Throws<ArgumentNullException>(() => transport.RebindWorld(CompositeTransportRole.InternalMission, new byte[1620], () => false, hooks, null!));
        Assert.Throws<NotSupportedException>(() => transport.RebindWorld(CompositeTransportRole.LanHost, new byte[1620], () => false, hooks, new object()));
        Assert.Throws<ArgumentException>(() => transport.RebindWorld(CompositeTransportRole.InternalMission, new byte[10], () => false, hooks, new object()));
        var broken = new Netstorm.Net.PacketDispatchHooks { Now = null, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } };
        Assert.Throws<ArgumentException>(() => transport.RebindWorld(CompositeTransportRole.InternalMission, new byte[1620], () => false, broken, new object()));
    }

    /// <summary>수송 팩토리도 같은 기본 표에 2번 연결 종료를 공유한다. 0.969에서 추가됐다.</summary>
    [Fact]
    public void FactorySharesConnectionEndedEntry()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        Assert.Equal(0x004b6660u, transport.Dispatch.SnapshotReceiver(2)!.NativeEntry);
        Assert.Null(transport.Dispatch.Registration(2).Observer);
        Assert.Null(transport.Dispatch.Registration(2).PacketObserver);
    }

    /// <summary>수송 팩토리도 같은 기본 표에 1a/1b 전투 시작을 공유한다. 0.970에서 추가됐다.</summary>
    [Fact]
    public void FactorySharesBattleStartEntries()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        Assert.Equal(0x004b4df0u, transport.Dispatch.SnapshotReceiver(0x1a)!.NativeEntry);
        Assert.Equal(0x004b8b20u, transport.Dispatch.SnapshotReceiver(0x1b)!.NativeEntry);
        Assert.NotNull(transport.Dispatch.Registration(0x1a).PacketObserver);
        Assert.NotNull(transport.Dispatch.Registration(0x1b).Observer);
    }

    /// <summary>수송 팩토리도 같은 기본 표에 0x66 서버 발견·0x26 메타 geometry를 공유한다. 0.971에서 추가됐다.</summary>
    [Fact]
    public void FactorySharesDiscoveryAndGeometryEntries()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        Assert.Equal(0x004b8a10u, transport.Dispatch.SnapshotReceiver(0x66)!.NativeEntry);
        Assert.Equal(12, transport.Dispatch.Registration(0x66).Length);
        Assert.Equal(0x004b45a0u, transport.Dispatch.SnapshotReceiver(0x26)!.NativeEntry);
        Assert.Equal(ushort.MaxValue, transport.Dispatch.Registration(0x26).Length);
    }

    /// <summary>수송 팩토리도 같은 기본 표에 0x1d 챌린지 로그인 응답을 공유한다. 0.973에서 추가됐다.</summary>
    [Fact]
    public void FactorySharesChallengeReplyEntry()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        Assert.Equal(0x004b8ac0u, transport.Dispatch.SnapshotReceiver(0x1d)!.NativeEntry);
        Assert.Equal(13, transport.Dispatch.Registration(0x1d).Length);
        Assert.NotNull(transport.Dispatch.Registration(0x1d).Observer);
    }

    /// <summary>수송 팩토리도 같은 기본 표에 0x0f Root 로그인을 공유한다. 0.974에서 추가됐다.</summary>
    [Fact]
    public void FactorySharesRootReplyEntry()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        Assert.Equal(0x004b4250u, transport.Dispatch.SnapshotReceiver(0x0f)!.NativeEntry);
        Assert.Equal(ushort.MaxValue, transport.Dispatch.Registration(0x0f).Length);
        Assert.NotNull(transport.Dispatch.Registration(0x0f).PacketObserver);
    }

    /// <summary>수송 팩토리도 같은 기본 표에 0x71 전투 설명·0x21 참가자 제거를 공유한다. 0.975에서 추가됐다.</summary>
    [Fact]
    public void FactorySharesDescriptionAndRemoveEntries()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        Assert.Equal(0x004b49c0u, transport.Dispatch.SnapshotReceiver(0x71)!.NativeEntry);
        Assert.Equal(ushort.MaxValue, transport.Dispatch.Registration(0x71).Length);
        Assert.Equal(0x004b46d0u, transport.Dispatch.SnapshotReceiver(0x21)!.NativeEntry);
        Assert.Equal(ushort.MaxValue, transport.Dispatch.Registration(0x21).Length);
        Assert.NotNull(transport.Dispatch.Registration(0x21).PacketObserver);
    }

    /// <summary>수송 팩토리도 같은 기본 표에 0x88 참가자 이름·0x67 전투 옵션을 공유한다. 0.977에서 추가됐다.</summary>
    [Fact]
    public void FactorySharesNameAndOptionsEntries()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        Assert.Equal(0x004b4680u, transport.Dispatch.SnapshotReceiver(0x88)!.NativeEntry);
        Assert.Equal(98, transport.Dispatch.Registration(0x88).Length);
        Assert.Equal(0x004b4900u, transport.Dispatch.SnapshotReceiver(0x67)!.NativeEntry);
        Assert.Equal(ushort.MaxValue, transport.Dispatch.Registration(0x67).Length);
    }

    /// <summary>수송 팩토리도 같은 기본 표에 0x20 참가자 추가·0x22 참가자 이동을 공유한다. 0.978에서 추가됐다.</summary>
    [Fact]
    public void FactorySharesAddAndMoveEntries()
    {
        var scene = new Scene();
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        using var transport = scene.Make();
        Assert.Equal(0x004b4710u, transport.Dispatch.SnapshotReceiver(0x20)!.NativeEntry);
        Assert.Equal(ushort.MaxValue, transport.Dispatch.Registration(0x20).Length);
        Assert.Equal(0x004b4820u, transport.Dispatch.SnapshotReceiver(0x22)!.NativeEntry);
        Assert.Equal(17, transport.Dispatch.Registration(0x22).Length);
    }
}
